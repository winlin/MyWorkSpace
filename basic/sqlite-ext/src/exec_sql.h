#ifndef __EXEC_SQL_H__
#define __EXEC_SQL_H__
#include <sqlite3ext.h>
void	exec_sql(sqlite3_context *, int, sqlite3_value **);
#endif
