#ifndef __EXEC_SP_H__
#define __EXEC_SP_H__
#include <sqlite3ext.h>
void	exec_sp(sqlite3_context *, int, sqlite3_value **);
#endif
