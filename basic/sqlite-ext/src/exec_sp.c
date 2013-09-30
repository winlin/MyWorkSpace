#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <sqlite3ext.h>
extern sqlite3_api_routines *sqlite3_api;
#include "sqlite_extension.h"
#include "exec_sp.h"
#include "tool_funcs.h"
#include "mprintf_value.h"
#define SP_TABLE "tblsptable"
void
exec_sp(
		sqlite3_context *c
		, int argc
		, sqlite3_value **argv
		)
{
	if (argc < 1) {
		log_message( SQLITE_MISUSE, "The name of the stored procedure to execute has not been supplied. @ %s, %s:%d",
		             __func__, __FILE__, __LINE__ );
		sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
		return;
	}
	/* I will not check for NULL because it is a rare case and I guess it should be caught later anyway */
	sqlite3_stmt *pStmtSpTable;
	sqlite3 * const db = sqlite3_context_db_handle(c);
	char const *zTail;
	int rc = sqlite3_prepare_v2(
			db
			, "SELECT sp_text FROM " SP_TABLE " WHERE LOWER(sp_name) = LOWER(?)"
			, -1
			, &pStmtSpTable
			, &zTail
			);
	if (SQLITE_OK != rc) {
		/* customer's demand is to return the word "false" in case
		of error until we explicitly know that the statement is
		a SELECT-type one and is going to return some rows. Then
		in case of error NULL should be returned. */
		sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
		log_message( rc, "%s; %s while preparing the SQL statement to look up the stored procedure. @ %s, %s:%d", sqlite3_errstr(rc),
		             sqlite3_errmsg(db), __func__, __FILE__, __LINE__ );
		return;
	}
	char const * const sp_name = sqlite3_value_text(argv[0]);
	rc = sqlite3_bind_text(pStmtSpTable, 1, sp_name, -1, NULL);
	if (SQLITE_OK != rc)
	{
		sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
		log_message( rc, "%s; %s; Stored procedure %s @ %s, %s:%d", sqlite3_errstr(rc), sqlite3_errmsg(db),
		             sp_name, __func__, __FILE__, __LINE__ );
		sqlite3_finalize(pStmtSpTable);
		return;
	}
	rc = sqlite3_step(pStmtSpTable);
	if (SQLITE_DONE != rc && SQLITE_ROW != rc) /* the latter is the warning case: there should be only one row */
	{
		sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
		log_message( rc, "%s; %s; Strored procedure %s @ %s, %s:%d", sqlite3_errstr(rc), sqlite3_errmsg(db),
		             sp_name, __func__, __FILE__, __LINE__ );
		sqlite3_finalize(pStmtSpTable);
		return;
	}
	char const * sp_text = sqlite3_column_text(pStmtSpTable, 0);
	if (NULL == sp_text) /* Here we don't distinguish the case when there is no row in the SP table having such a key and when the row exists but the sp_text field is NULL. This case is described in the TicId:2246aa */
	{
		sqlite3_finalize(pStmtSpTable);
		log_message(SQLITE_MISUSE, "Stored procedure %s has not been found or has a NULL body. @ %s, %s:%d", sp_name, __func__, __FILE__, __LINE__);
		sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
		return;
	}
	size_t const sp_text_length = strlen(sp_text);
	assert(sp_text_length);
	char * const sp_text_copy = sqlite3_malloc(sp_text_length + 1);
	if (NULL == sp_text_copy) {
		log_message( SQLITE_NOMEM, "Could not allocate %d bytes. Stored procedure %s @ %s, %s:%d",
			     sp_text_length + 1, sp_name, __func__, __FILE__, __LINE__ );
		return;
	}
	strcpy(sp_text_copy, sp_text);
	sqlite3_finalize(pStmtSpTable);
	/* strip the right space chars from sql add by gtliu 2013-09-24 */
    zTail = strip_string_space(sp_text_copy);
    /* end */
    
	int parameters_left = argc - 1;
	sqlite3_value **pParameters = &argv[1];
	char * zResult = NULL;
	while (NULL != zTail && '\0' != *zTail)
	{
		char const * const zSql = zTail;
		sqlite3_stmt *pStmtSp;
		rc = sqlite3_prepare_v2(
				db
				, (char const *)zSql
				, -1
				, &pStmtSp
				, &zTail
				);
		int const is_select = sqlite3_column_count(pStmtSp) > 0;
		if (SQLITE_OK != rc) {
			sqlite3_free(sp_text_copy);
			if (is_select)
				sqlite3_result_null(c);
			else
				sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
				log_message( rc, "%s; %s; Stored procedure %s @ %s, %s:%d", sqlite3_errstr(rc), sqlite3_errmsg(db),
				             sp_name, __func__, __FILE__, __LINE__ );
			return;
		}
		int const parameter_count = sqlite3_bind_parameter_count(pStmtSp);
		int i;
		for (i = 0; i < parameter_count; ++i)
		{
			if (0 == parameters_left)
			{
				sqlite3_free(sp_text_copy);
				sqlite3_finalize(pStmtSp);
				if (is_select)
					sqlite3_result_null(c);
				else
					sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
				log_message( SQLITE_MISUSE,
					     "%d parameters %s been passed while the stored procedure %s would consume more. %s, %s:%d",
				              argc - 1, (argc - 1) > 1 ? "have" : "has", 
					      __func__, __FILE__, __LINE__ );
				return;
			}
			rc = sqlite3_bind_value(pStmtSp, i + 1, *(pParameters++));
			if (SQLITE_OK != rc) {
				sqlite3_free(sp_text_copy);
				sqlite3_finalize(pStmtSp);
				if (is_select)
					sqlite3_result_null(c);
				else
					sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
					log_message( rc, "%s; %s; Stored procedure %s @ %s, %s:%d", sqlite3_errstr(rc),
					             sqlite3_errmsg(db), sp_name, __func__, __FILE__, __LINE__ );
				return;
			}
			--parameters_left;
		}
		while (SQLITE_ROW == (rc = sqlite3_step(pStmtSp)))
		{
			if (NULL == zTail || '\0' == zTail[0] /* that's the last statament in the SP and we need to collect the result */
			    && 1 == sqlite3_column_count(pStmtSp)) /* if there is only one column */ /* TODO: do not call that repeatedly */
			{
				sqlite3_value * const column_value = sqlite3_column_value(pStmtSp, 0);
				char * const zValue = mprintf_value(column_value);
				if (NULL == zValue) {
					if (NULL != zResult)
						sqlite3_free(zResult);
					log_message( SQLITE_NOMEM, "Could not allocate memory. Stored procedure %s @ %s, %s:%d",
					             sp_name, __func__, __FILE__, __LINE__ );
					sqlite3_finalize(pStmtSp);
					sqlite3_result_null(c);
					return;
				}
				if (NULL == zResult) { /* this is the first row */
					zResult = sqlite3_mprintf("(%s", zValue);
					if (NULL == zResult) {
						sqlite3_finalize(pStmtSp);
						sqlite3_result_null(c);
						log_message( SQLITE_NOMEM, "Could not allocate memory. Stored procedure %s @ %s, %s:%d",
					                     sp_name, __func__, __FILE__, __LINE__ );
						return;
					}
				}
				else {
					char * const tmp_zResult = sqlite3_mprintf("%s\n, %s", zResult, zValue);
					if (NULL == tmp_zResult) {
						sqlite3_finalize(pStmtSp);
						sqlite3_result_null(c);
						log_message( SQLITE_NOMEM, "Could not allocate memory. Stored procedure %s @ %s, %s:%d",
					                     sp_name, __func__, __FILE__, __LINE__ );
					 	return;
					}
					sqlite3_free(zResult);
					zResult = tmp_zResult;
				}
			}
		}
		if (SQLITE_DONE != rc)
		{
			sqlite3_free(sp_text_copy);
			sqlite3_finalize(pStmtSp);
			if (is_select)
				sqlite3_result_null(c);
			else
				sqlite3_result_text(c, "false", sizeof "false" - 1, NULL);
			log_message( rc, "%s; %s; Stored procedure %s @ %s, %s:%d", sqlite3_errstr(rc), sqlite3_errmsg(db),
				     sp_name, __func__, __FILE__, __LINE__ );
			return;
		}
		sqlite3_finalize(pStmtSp);
	}
	sqlite3_free(sp_text_copy);
	if (NULL != zResult) {
		char * const zTmpResult = sqlite3_mprintf("%s)", zResult);
		sqlite3_free(zResult);
		if (NULL == zTmpResult)
#ifdef SQLITE_EXTENSION_LOG
		{
#endif
			sqlite3_result_null(c);
#ifdef SQLITE_EXTENSION_LOG
			log_message( SQLITE_NOMEM, "Could not allocate memory. Stored procedure %s @ %s, %s:%d",
			             sp_name, __func__, __FILE__, __LINE__ );
		}
#endif
		else
			sqlite3_result_text(c, zTmpResult, -1, sqlite3_free);
	}
	else
		sqlite3_result_text(c, "true", sizeof "true" - 1, NULL);
	return;
}
