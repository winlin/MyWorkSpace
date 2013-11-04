#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sqlite3ext.h>
#include "uuid.h"
#include "utils.h"
#include "exec_sql.h"
#include "table_info.h"
#include "exec_sp.h"
SQLITE_EXTENSION_INIT1
#ifdef _WIN32
__declspec(dllexport)
#endif

void ext_echo(sqlite3_context *context, int argc, sqlite3_value **argv) {
    int n = 1234567890;
    sqlite3_result_int(context, n);
}

void ext_uuid(sqlite3_context *context, int argc, sqlite3_value **argv)
{
	typedef unsigned char uuid_t[16];
	uuid_t u;
	char uuid[37];
	uuid_generate(u);	// generate guid with "-"
	uuid_unparse(u, uuid);
	char *uuid32 = ReplaceString(uuid, "-", "");

	sqlite3_result_text( context, uuid32, -1, SQLITE_TRANSIENT);
}

int
sqlite3_extension_init(
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
)
{
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);

  rc = sqlite3_create_function(db, "echo", 0, SQLITE_ANY, 0, ext_echo, 0, 0);
  rc = sqlite3_create_function(db, "uuid", 0, SQLITE_ANY, 0, ext_uuid, 0, 0);

  rc = sqlite3_create_function_v2(
		  db
		  , "exec_sql" /* function name */
		  , -1 /* number of arguments */
		  , SQLITE_ANY /* preferred parameter encoding */
		  , NULL /* application data */
		  , exec_sql /* function */
		  , NULL /* xStep */
		  , NULL /* xFinal */
		  , NULL /* xDestroy */);
  if (SQLITE_OK != rc)
	  return rc;
  rc = sqlite3_create_function_v2(
		  db
		  , "table_info" /* function name */
		  , 1 /* number of arguments */
		  , SQLITE_ANY /* preferred parameter encoding */
		  , NULL /* application data */
		  , table_info /* function */
		  , NULL /* xStep */
		  , NULL /* xFinal */
		  , NULL /* xDestroy */);
  if (SQLITE_OK != rc)
	  return rc;
  rc = sqlite3_create_function_v2(
		  db
		  , "exec_sp" /* function name */
		  , -1 /* number of arguments */
		  , SQLITE_ANY /* preferred parameter encoding */
		  , NULL /* application data */
		  , exec_sp /* function */
		  , NULL /* xStep */
		  , NULL /* xFinal */
		  , NULL /* xDestroy */);
  return rc;
}
