#include <stddef.h>
#include <string.h>
#include <sqlite3ext.h>
#include "mprintf_value.h"
extern sqlite3_api_routines *sqlite3_api;
char /* caller is responsible of freeing of the result */
*mprintf_value(sqlite3_value * const value)
{
	int const value_type = sqlite3_value_type(value);
	switch (value_type) {
		case SQLITE3_TEXT:
		{
			char const * const value_text = sqlite3_value_text(value);
			return sqlite3_mprintf("%Q", value_text);
		}
		case SQLITE_INTEGER:
		{
			int const value_integer = sqlite3_value_int(value);
			return sqlite3_mprintf("%d", value_integer);
		}
		case SQLITE_FLOAT:
		{
			double const value_double = sqlite3_value_double(value);
			return sqlite3_mprintf("%f", value_double);
		}
		case SQLITE_NULL:
		{
			char * const text_null = sqlite3_malloc(sizeof "NULL");
			if (NULL == text_null)
				return NULL;
			memcpy(text_null, "NULL", sizeof "NULL");
			return text_null;
		}
		case SQLITE_BLOB:
		{
			char * const blob = sqlite3_malloc(sizeof "<BLOB>");
			if (NULL == blob)
				return NULL;
			memcpy(blob, "<BLOB>", sizeof "<BLOB>");
			return blob;
		}
		default:
			return sqlite3_mprintf("<unsuppored value type %d>", value_type);
	}
}
