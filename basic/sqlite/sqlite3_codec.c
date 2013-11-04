#include "sqlite3.h"
#ifdef SQLITE_HAS_CODEC
SQLITE_API int sqlite3_key(
  sqlite3 *db,                   
  const void *pKey, int nKey     
) {
    return SQLITE_OK;
}

SQLITE_API int sqlite3_key_v2(
  sqlite3 *db,                   
  const char *zDbName,           
  const void *pKey, int nKey     
) {
    return SQLITE_OK;
}

SQLITE_API int sqlite3_rekey(
  sqlite3 *db,                   
  const void *pKey, int nKey     
) {
    return SQLITE_OK;
}

SQLITE_API int sqlite3_rekey_v2(
  sqlite3 *db,                   
  const char *zDbName,           
  const void *pKey, int nKey     
) {
    return SQLITE_OK;
}

SQLITE_API void sqlite3_activate_see(
  const char *zPassPhrase        
) {
    return;
}

int sqlite3CodecAttach(sqlite3* db, int a, const void* b, int c) {
    return SQLITE_OK;
}

void sqlite3CodecGetKey(sqlite3* db, int a, void** b, int* c) {
    return;
}

#endif