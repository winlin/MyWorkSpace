//
//  WSLogModule.h
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef WSLoggerDefine_wslogger_h
#define WSLoggerDefine_wslogger_h

// define debug switch macro
/****************************************************************************************
 * If define the marco all message will print into stdout/stderr.
 * It can be used when you want to debug application.
 *
 * If don't all message will write into common/warning/error log files.
 * It can be used when you want to relase application
 */
//#define WSLOG_DEBUG_ENABLE           1

#define WSLOG_FILE_INDEX_NUM    3
typedef enum WSLogFileIndex {
    WSLOG_COMM_FILE_INDEX       = 0,
    WSLOG_WARN_FILE_INDEX       = 1,
    WSLOG_ERROR_FILE_INDEX      = 2
} WSLogFileIndex;

typedef enum WSLogLevel {
    WSLOG_LEVEL_COMMON          = 0,
    WSLOG_LEVEL_WARNING         = 1,
    WSLOG_LEVEL_ERROR           = 2
} WSLogLevel;

// define log file
#define WSLOG_LOG_FILE_PATH               "./"         

typedef enum WSLogMaxSize {
    WSLOG_MAX_FILE_NAME_LEN         = 150,
    WSLOG_MAX_FILE_SIZE             = 1024*1024*2,   // 2MB
    WSLOG_MAX_COMM_BUFFER_SIZE      = 1024*3,        // 3KB
    WSLOG_MAX_WARN_BUFFER_SIZE      = 1024,          // 1KB
    WSLOG_MAX_ERROR_BUFFER_SIZE     = 1024           // 1KB
}WSLogMaxSize;

typedef enum WSLogRetValue {
    WSLOG_RETV_OPEN_SUCCESS         = 0,
    WSLOG_RETV_ARG_EMPTY            = -100,
    WSLOG_RETV_OPEN_FILE_FAIL       = -101,
    WSLOG_RETV_ALLOC_MEM_FAIL       = -102,
    WSLOG_RETV_DUP_FAIL             = -103
} WSLogRetValue;

/*********************************************************************************************************
 *DESC: This function should be called before use the WSLogger module.
 *PARA: appName     the name of application.
 *RETU: WSLOG_RETV_OPEN_SUCCESS
 *      WSLOG_RETV_ARG_EMPTY
 *      WSLOG_RETV_OPEN_FILE_FAIL
 *      WSLOG_RETV_ALLOC_MEM_FAIL
 *      WSLOG_RETV_DUP_FAIL
 *
 */
WSLogRetValue WSLogOpen(char *appName);

int WSLogWrite(WSLogLevel level, const char *fmt, ...);

int WSLogFlush(void);

WSLogRetValue WSLogClose(void);

#endif

























