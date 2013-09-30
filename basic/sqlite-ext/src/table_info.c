#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <assert.h>
#include <sys/types.h>
#include <string.h>
#include <sqlite3ext.h>
#include "sqlite_extension.h"
#include "table_info.h"
extern sqlite3_api_routines *sqlite3_api; /* This is needed to separate functions from the extension loading code. */
/* The function should return a tuple like the following
   (tablename, tabletype, colname, coltype, colispk, coldefvalue, colnotnullon, colcollation) */
#define ROW_TEMPLATE_WITH_COMMENTS					\
			"(%Q -- table name\n"   			\
			", %Q -- object type\n" 			\
			", %Q -- column name\n" 			\
			", %Q -- column type\n" 			\
			", %Q -- column default value\n"		\
			", %d -- column has NOT NULL constraint\n" 	\
			", %Q -- column conflict resolution algorithm\n"\
			", %Q -- name of default collation sequence\n" 	\
			", %d -- column is part of primary key\n"	\
			")"
#define ROW_TEMPLATE_WITHOUT_COMMENTS "(%Q, %Q, %Q, %Q, %Q, %d, %Q, %Q, %d)"
#define ROW_TEMPLATE ROW_TEMPLATE_WITHOUT_COMMENTS
static char /* caller should release memory allocated for result */
*get_object_type_name(
		sqlite3 * const db
		, char const * const table_name
		, int * const error
		, void *(*allocator) (int)
		)
{
		sqlite3_stmt *pSt;
		char const *tail;

		int rc = sqlite3_prepare_v2(
				db
				, "SELECT type FROM SQLITE_MASTER "
				" WHERE type IN ('table', 'view') "
				" AND LOWER(name) = LOWER(?)"
				, -1
				, &pSt
				, &tail
				);
		if (SQLITE_OK != rc)
		{
			if (NULL != error)
				*error = rc;
			log_message(rc, "%s @ %s, %s:%d", sqlite3_errmsg(db), __func__, __FILE__, __LINE__);
			return NULL;
		}
		rc = sqlite3_bind_text(pSt, 1, table_name, -1, NULL);
		if (SQLITE_OK != rc)
		{
			if (NULL != error)
				*error = rc;
			log_message(rc, "%s %s, %s:%d", sqlite3_errmsg(db), __func__, __FILE__, __LINE__);
			return NULL;
		}
		char *object_type = NULL;
		while (SQLITE_ROW == (rc = sqlite3_step(pSt)))
		{
			/* this should be executed once */
			assert(NULL == object_type);
			char const * const tmp_object_type = (char const *)sqlite3_column_text(pSt, 0);
			assert(NULL != tmp_object_type);
			size_t const object_type_length = strlen(tmp_object_type);
			object_type = allocator(object_type_length + 1);
			if (NULL == object_type) {
				sqlite3_finalize(pSt);
				if (NULL != error)
					*error = SQLITE_NOMEM;
				log_message(SQLITE_NOMEM, "Could not allocate %d bytes. At %s, %s:%d", object_type_length + 1, __func__, __FILE__, __LINE__);
				return NULL;
			}
			strcpy(object_type, tmp_object_type);
		}
		sqlite3_finalize(pSt);
		if (SQLITE_DONE != rc) {
			if (NULL != object_type)
				sqlite3_free(object_type);
			if (NULL != error)
				*error = rc;
			log_message(rc, "%s %s, %s:%d", sqlite3_errmsg(db), __func__, __FILE__, __LINE__);
			return NULL;
		}
		return object_type;
}
static int
enumerate_table_columns(
	sqlite3_context * const c,
	char const * const table_name,
	void (*callback)(
		char const *, /* column database name */
		char const *, /* column declared type */
		char const *, /* column name */
		char const *, /* column origin name */
		char const *, /* column table name */
		int,	      /* column type */
		char const *, /* column default value */
		char const *, /* column conflict resolution algorithm */
		void *), /* application data */
	void * const app_data
)
{
	sqlite3_stmt * sth = NULL;
	char * const statement = sqlite3_mprintf("PRAGMA TABLE_INFO(%Q)", table_name);
	int rc;
	if (NULL == statement)
	{
		rc = SQLITE_NOMEM;
		goto end;
	}
	sqlite3 * const db = sqlite3_context_db_handle(c);
	assert(NULL != db);
	char const *zTail;
	rc = sqlite3_prepare_v2(
			db,
			statement,
			-1,
			&sth,
			&zTail
			);
	if (SQLITE_OK != rc)
		goto end;
	while (SQLITE_ROW == (rc = sqlite3_step(sth)))
	{
		/* I'll assign the column values to variables for the sake of clarity */
		int const column_number = sqlite3_column_int(sth, 0);
		char const * const column_name = (char const *)sqlite3_column_text(sth, 1); /* casting away unsignedness */
		char const * const column_declared_type = (char const *)sqlite3_column_text(sth, 2);
		int const column_null_allowed = sqlite3_column_int(sth, 3);
		char const * const column_default_value = (char const *)sqlite3_column_text(sth, 4);
		int const column_pk_index  = sqlite3_column_int(sth, 5);
		callback(
				NULL, /* column database name */
				column_declared_type,
				column_name,
				column_name, /* column origin name */
				table_name,
				-1,
				column_default_value, 
				NULL, /* column conflict resolution algorithm */
				app_data
				);
	}
end:
	if (NULL != statement)
		sqlite3_free(statement);
	if (NULL != sth)
		sqlite3_finalize(sth);
	if (SQLITE_DONE == rc)
		return SQLITE_OK;
#ifdef	SQLITE_EXTENSION_LOG
	if (SQLITE_OK != rc)
		log_message(rc, "%s %s, %s:%d", sqlite3_errstr(rc), __func__, __FILE__, __LINE__);
		
#endif
	return rc;
}
struct collect_table_information_app_data
{
	char *string;
	char const *object_type_name;
	int rc;
	sqlite3_context *c;
};
static void
collect_table_information_string(
		char const * const column_database_name,
		char const * const column_declared_type,
		char const * const column_name,
		char const * const column_origin_name,
		char const * const column_table_name,
		int const column_type,
		char const * const column_default_value,
		char const * const column_conflict_algo,
		void * const thunk
		)
{
	struct collect_table_information_app_data * const app_data = thunk;
	sqlite3 * const db = sqlite3_context_db_handle(app_data->c);
	assert(db);
	char const *zDataType = NULL, *zCollSeq = NULL;
	int notNull = -1, primaryKey = -1, autoInc = -1;
	if (NULL != column_origin_name) {
		app_data->rc = sqlite3_table_column_metadata(
			db,
			column_database_name,
			column_table_name,
			column_origin_name,
			&zDataType,
			&zCollSeq,
			&notNull,
			&primaryKey,
			&autoInc
		);
		if (SQLITE_OK != app_data->rc)
#ifdef SQLITE_EXTENSION_LOG
		{
			log_message(app_data->rc, "%s %s, %s:%d", sqlite3_errstr(app_data->rc), __func__, __FILE__, __LINE__);
#endif
			return;
#ifdef SQLITE_EXTENSION_LOG
		}
#endif
	}
	char * const temp_string = (NULL == app_data->string)
		?  sqlite3_mprintf(
			ROW_TEMPLATE,
			column_table_name,
			app_data->object_type_name,
			column_name,
			column_declared_type,
			column_default_value, /* column default value */ /* TODO: find the way to get it */
			notNull, /* column is disallowed of nulls */ /* TODO: find the way to get the value of the field */
			column_conflict_algo, /* column conflict resolution algorithm */ /* TODO: find the way to get it */ 
			zCollSeq, /* column default collation sequence */
			primaryKey
		)
		: sqlite3_mprintf(
			"%s,\n" ROW_TEMPLATE,
			app_data->string,
			column_table_name,
			app_data->object_type_name,
			column_name,
			column_declared_type,
			column_default_value, /* column default value */ /* TODO: find the way to get it */
			notNull, /* column is disallowed of nulls */ /* TODO: find the way to get the value of the field */
			column_conflict_algo, /* column conflict resolution algorithm */ /* TODO: find the way to get it */ 
			zCollSeq, /* column default collation sequence */
			primaryKey
		);
		if (NULL == temp_string) {
			log_message(SQLITE_NOMEM, "Could not allocate memory. @ %s, %s:%d ", __func__, __FILE__, __LINE__);
			app_data->rc = SQLITE_NOMEM;
			return;
		}
		if (NULL != app_data->string)
			sqlite3_free(app_data->string);
		app_data->string = temp_string;
}
void
table_info(
		sqlite3_context *c , int argc , sqlite3_value **argv )
{
	if (1 != argc) {
		sqlite3_result_null(c);
		log_message(SQLITE_MISUSE, "There should be exactly one argument: the name of the table to describe. %d have been passed. @ %s, %s:%d", argc, __func__, __FILE__, __LINE__);
		return;
	}
	if ( NULL == argv[0] ) { /* table name was NULL */
		log_message(SQLITE_MISUSE, "NULL passed. @ %s, %s:%d", __func__, __FILE__, __LINE__);
		sqlite3_result_null(c); /* errors should be indicated with returning NULL */
		return;
	}

	int const name_argument_type = sqlite3_value_type(argv[0]);
	if (SQLITE_TEXT != name_argument_type) {
		log_message(SQLITE_MISUSE, "Object name should be text. The value of type %d have been passed. @ %s, %s:%d", name_argument_type, __func__, __FILE__, __LINE__);
		sqlite3_result_null(c);
		return;
	}
	char const * const table_name = (char const *)sqlite3_value_text(argv[0]);

	/* Database handle will often be used so it is reasonable to extract it once, store in a temporary variable
	 and then use it instead of constantly calling the sqlite3_context_db_handle() */
	sqlite3 * const db = sqlite3_context_db_handle(c);

	int rc = SQLITE_OK;

	char * const object_type_name = get_object_type_name(db,  table_name, &rc, sqlite3_malloc);
	if (SQLITE_OK != rc) {
		if (NULL != object_type_name)
			sqlite3_free(object_type_name);
		log_message(rc, "%s @ %s, %s:%d", sqlite3_errstr(rc), __func__, __FILE__, __LINE__);
		sqlite3_result_null(c);
		return;
	}
	if (NULL == object_type_name) { /* Object with such a name has not been found. FIXME: that's not a reliable indicator of the condition. */ 
		log_message(SQLITE_WARNING, "Object %s has not been found. @ %s, %s:%d", table_name, __func__, __FILE__, __LINE__);
		sqlite3_result_null(c);
		return;
	}

	struct collect_table_information_app_data app_data;
	memset(&app_data, 0, sizeof app_data);
	app_data.rc = SQLITE_OK;
	app_data.string = NULL;
	app_data.object_type_name = object_type_name;
	app_data.c = c;
	rc = enumerate_table_columns(c, table_name, collect_table_information_string, &app_data);
#ifdef SQLITE_EXTENSION_LOG
	if (SQLITE_OK != rc)
		log_message(rc, "%s @ %s, %s:%d", sqlite3_errstr(rc), __func__, __FILE__, __LINE__);
#endif

	if (NULL != object_type_name)
		sqlite3_free(object_type_name);
	sqlite3_result_text(c, app_data.string, -1, sqlite3_free);
	return;
}
