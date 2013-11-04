#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sqlite3ext.h>
#include "sqlite_extension.h"
#include "tool_funcs.h"
#include "exec_sql.h"
#include "mprintf_value.h"
extern sqlite3_api_routines *sqlite3_api;
static char
*mprintf_row(sqlite3_stmt *pStmt, int * const error)
{
	char * result = NULL;
	int const column_count = sqlite3_column_count(pStmt);
	int i;
	for (i = 0; i < column_count; ++i)
	{
		sqlite3_value * const column_value = sqlite3_column_value(pStmt, i);
		char * const column_text = mprintf_value(column_value);
		if (NULL == column_text)
		{
			if (NULL != error)
				*error = SQLITE_NOMEM;
			if (NULL != result)
				sqlite3_free(result);
			log_message(SQLITE_NOMEM, "Cannot allocate memory. @ %s, %s:%d", __func__, __FILE__, __LINE__);
			return NULL;
		}
		if (NULL == result) /* the first value in the row */
			result = column_text;
		else
		{
			char * const tmp_result = sqlite3_mprintf("%s, %s", result, column_text);
			sqlite3_free(column_text);
			sqlite3_free(result);
			if (NULL == tmp_result)
			{
				if (NULL != error)
					*error = SQLITE_NOMEM;
				log_message(SQLITE_NOMEM, "Cannot allocate memory. @ %s, %s:%d", __func__, __FILE__, __LINE__);
				return result;
			}
			result = tmp_result;
		}
	}
	return result;
}
void
exec_sql(
	sqlite3_context *c
	, int argc
	, sqlite3_value **argv
)
{
	if (argc == 0) {
		sqlite3_result_text(c, "false", -1, NULL);
		log_message(SQLITE_MISUSE, "No SQL statement to execute. @ %s, %s:%d", __func__, __FILE__, __LINE__);
		return;
	}
	int const value_type = sqlite3_value_type(argv[0]);
	if (SQLITE3_TEXT != value_type) {
		sqlite3_result_text(c, "false", -1, NULL);
		log_message(SQLITE_MISUSE, "SQL statement to execute is expected to be of type text but a value of type %d has been passed. @ %s, %s:%d", value_type, __func__, __FILE__, __LINE__);
		return;
	}
	char const *zSql = sqlite3_value_text(argv[0]);
    assert(NULL != zSql);
    /* strip the sql rithg space from sql add by gtliu 2013-09-24 */
    char *stripedSql = strdup(zSql);
    if (NULL == stripedSql) {
        sqlite3_result_text(c, "false", -1, NULL);
		return;
    }
    zSql = strip_string_space(stripedSql);
    /* end */
	
	sqlite3 * const db = sqlite3_context_db_handle(c);
	assert(NULL != db);
	sqlite3_stmt * pStmt;
	char const * zTail;
	int rc = sqlite3_prepare_v2(
			db
			, zSql
			, -1
			, &pStmt
			, &zTail
			);
	if (SQLITE_OK != rc) {
		/* customer's demand is to return the word "false" in case
		of error until we explicitly know that the statement is
		a SELECT-type one and is going to return some rows. Then
		in case of error NULL should be returned. */
		sqlite3_result_text(c, "false", -1, NULL);
		log_message(rc, "%s; %s @ %s, %s:%d", sqlite3_errstr(rc), sqlite3_errmsg(db), __func__, __FILE__, __LINE__);
		free(stripedSql);
		return;
	}
	int const is_select = sqlite3_column_count(pStmt) > 0;
	int const parameter_count = sqlite3_bind_parameter_count(pStmt);
	if (parameter_count > argc - 1)
	{
		if (is_select)
			sqlite3_result_null(c);
		else
			sqlite3_result_text(c, "false", -1, NULL);
		log_message( SQLITE_MISUSE, "Procedure would consume at least %d arguments while only %d %s been passed. @ %s, %s:%d",
			     parameter_count, argc - 1, (argc - 1) > 1 ? "have" : "has",
			     __func__, __FILE__, __LINE__ );
		sqlite3_finalize(pStmt);
		free(stripedSql);
		return;
	}
	int i;
	for (i = 0; i < parameter_count; ++i)
	{
		rc = sqlite3_bind_value(pStmt, i + 1, argv[i + 1]);
		if (SQLITE_OK != rc) {
			if (is_select)
				sqlite3_result_null(c);
			else
				sqlite3_result_text(c, "false", -1, NULL);
			log_message( rc, "%s; %s Parameter #%d, value %s, value type #%d. @ %s, %s:%d",
				     sqlite3_errstr(rc), sqlite3_errmsg(db),
			             i + 1, sqlite3_value_text(argv[i + 1]),
				     sqlite3_value_type(argv[i + 1]),
				     __func__, __FILE__, __LINE__ );
			sqlite3_finalize(pStmt);
			free(stripedSql);
			return;
		}
	}
	char *result = NULL;
	while (SQLITE_ROW==(rc=sqlite3_step(pStmt)))
	{
		int lrc = SQLITE_OK;
		char * const row_text = mprintf_row(pStmt, &lrc);
		assert(NULL != row_text); /* that should never happen */
		if (SQLITE_OK != lrc)  /* I'll discard the result in case the processing was interrupted by error */
		{
			if (NULL != result)
			{
				if (is_select)
					sqlite3_result_null(c);
				else
					sqlite3_result_text(c, "false", -1, NULL);
				log_message( lrc, "%s; %s @ %s %s:%d", sqlite3_errstr(lrc), sqlite3_errmsg(db),
				            __func__, __FILE__, __LINE__ );
				sqlite3_free(row_text);
				sqlite3_free(result);
				sqlite3_finalize(pStmt);
				free(stripedSql);
				return;
			}
		}
		char * const parenthesised_row_text = sqlite3_mprintf("(%s)", row_text);
		sqlite3_free(row_text);
		if (NULL == parenthesised_row_text)
		{
			if (NULL != result)
				sqlite3_free(result);
			sqlite3_finalize(pStmt);
			if (is_select)
				sqlite3_result_null(c);
			else
				sqlite3_result_text(c, "false", -1, NULL);
			log_message(SQLITE_NOMEM, "Cannot allocate memory. @ %s, %s:%d", __func__, __FILE__, __LINE__);
			free(stripedSql);
			return;
		}
		if (NULL == result)
			result = parenthesised_row_text;
		else {
			char * const tmp_result = sqlite3_mprintf("%s\n, %s", result, parenthesised_row_text);
			sqlite3_free(parenthesised_row_text);
			sqlite3_free(result);
			if (NULL == tmp_result)
			{
				sqlite3_finalize(pStmt);
				if (is_select)
					sqlite3_result_null(c);
				else
					sqlite3_result_text(c, "false", -1, NULL);
				log_message(SQLITE_NOMEM, "Cannot allocate memory. @ %s, %s:%d", __func__, __FILE__, __LINE__);
				free(stripedSql);
				return;
			}
			result = tmp_result;
		}
	}
	if (SQLITE_DONE != rc) {
		if (is_select)
			sqlite3_result_null(c);
		else
			sqlite3_result_text(c, "false", -1, NULL);
		log_message(rc, "%s; %s @ %s, %s:%d", sqlite3_errstr(rc), sqlite3_errmsg(db), __func__, __FILE__, __LINE__);
		sqlite3_free(result);
	}
	else {
		if (is_select)
			sqlite3_result_text(c, result, -1, sqlite3_free);
		else
			sqlite3_result_text(c, "true", sizeof "true" - 1, NULL);
	}
	sqlite3_finalize(pStmt);
	free(stripedSql);
}
