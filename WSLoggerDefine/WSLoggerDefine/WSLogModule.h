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
    WSLOG_INDEX_COMM_FILE       = 0,
    WSLOG_INDEX_WARN_FILE       = 1,
    WSLOG_INDEX_ERROR_FILE      = 2
} WSLogFileIndex;

typedef enum WSLogLevel {
    WSLOG_LEVEL_COMMON          = 0,
    WSLOG_LEVEL_WARNING         = 1,
    WSLOG_LEVEL_ERROR           = 2
} WSLogLevel;

// define log file
#define WSLOG_LOG_FILE_PATH         "./"         

typedef enum WSLogMaxSize {
    WSLOG_MAX_FILE_NAME_LEN         = 150,           // absolute path
    WSLOG_MAX_APP_NAME_LEN          = 40,            // application name
    WSLOG_MAX_LOG_FILE_LEN          = 55,            // log file name
    WSLOG_MAX_FILE_SIZE             = 1024*1024*1,   // 1MB
    WSLOG_MAX_COMM_FILE_NUM         = 1,             // commen type file num(<128): appName.comm.1
    WSLOG_MAX_WARN_FILE_NUM         = 10,            // warning type file num(<128): appName.warn.1 -- appName.warn.10
    WSLOG_MAX_ERROR_FILE_NUM        = 10,            // error type file num(<128): appName.error.1 -- appName.error.10
    WSLOG_MAX_COMM_BUFFER_SIZE      = 1024*2,        // 2KB
    WSLOG_MAX_WARN_BUFFER_SIZE      = 1024,          // 1KB
    WSLOG_MAX_ERROR_BUFFER_SIZE     = 1024           // 1KB
}WSLogMaxSize;

typedef enum WSLogRetValue {
    WSLOG_RETV_SUCCESS              = 0,
    WSLOG_RETV_FAIL                 = -1,
    WSLOG_RETV_ARG_EMPTY            = -100,
    WSLOG_RETV_OPEN_FILE_FAIL       = -101,
    WSLOG_RETV_ALLOC_MEM_FAIL       = -102,
    WSLOG_RETV_DUP_FAIL             = -103,
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

WSLogRetValue WSLogWrite(WSLogLevel level, const char *fmt, ...);

WSLogRetValue WSLogFlush(void);

WSLogRetValue WSLogClose(void);


#endif

























