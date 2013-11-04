#ifndef __SQLITE_EXTENSION_H__
#define __SQLITE_EXTENSION_H__
#define SQLITE_EXTENSION_LOG 
#ifdef SQLITE_EXTENSION_LOG
#define log_message(...) sqlite3_log(__VA_ARGS__)
#else
#define log_message(...) (void)0
#endif
#endif
