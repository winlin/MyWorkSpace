//
//  WSLogModule.h
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//  Email:  pcliuguangtao@163.com
//
//  Usage:
//      #include "WSLogModule.h"
//      int main() {
//          WSLogOpen("TestApp");
//          //...
//          WSLogWrite(WSLOG_LEVEL_COMMON, "This is for common:%d Message:%s", 12, "Hello world");
//          WSLogWrite(WSLOG_LEVEL_WARNING, "This is for warning:%d Message:%s", 12, "Hello world");
//          WSLogFlush();    
//          WSLogWrite(WSLOG_LEVEL_ERROR, "This is for error:%d Message:%s", 12, "Hello world");
//          //...
//          WSLogClose();
//          return 0;
//      }
//
//

#ifndef WSLoggerDefine_wslogger_h
#define WSLoggerDefine_wslogger_h

// debug switch macro
/****************************************************************************************
 * If define the marco all message will print into stdout/stderr.
 * It can be used when you want to debug application.
 *
 * If don't all message will write into common/warning/error log files.
 * It can be used when you want to relase application
 */
//#define WSLOG_DEBUG_ENABLE      1

#define WSLOG_FILE_INDEX_NUM      3
typedef enum WSLogFileIndex {
    WSLOG_INDEX_COMM_FILE       = 0,                // num should start from 0 and keep continuately 
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
    WSLOG_MAX_FILE_NAME_PATH_LEN    = 150,           // absolute path
    WSLOG_MAX_APP_NAME_LEN          = 40,            // application name
    WSLOG_MAX_LOG_FILE_LEN          = 50,            // log file name max length
    WSLOG_MAX_FILE_SIZE             = 1024*1024*2,   // 2MB not very exact as for system alloc \
						     	disk space not by bytes(exp 4KB)
    WSLOG_MAX_COMM_FILE_NUM         = 1,             // common type file num: appName.comm.1
    WSLOG_MAX_WARN_FILE_NUM         = 10,            // warning type file num: appName.warn.1 -- appName.warn.10
    WSLOG_MAX_ERROR_FILE_NUM        = 10,            // error type file num: appName.error.1 -- appName.error.10
    WSLOG_MAX_COMM_BUFFER_SIZE      = 1024*4,        // 4KB
    WSLOG_MAX_WARN_BUFFER_SIZE      = 1024,          // 1KB
    WSLOG_MAX_ERROR_BUFFER_SIZE     = 1024           // 1KB
}WSLogMaxSize;

typedef enum WSLogRetValue {
    WSLOG_RETV_SUCCESS              = 0,
    WSLOG_RETV_FAIL                 = -1,
    WSLOG_RETV_ARG_EMPTY            = -100,
    WSLOG_RETV_OPEN_FILE_FAIL       = -101,
    WSLOG_RETV_ALLOC_MEM_FAIL       = -102
} WSLogRetValue;

/***********************************************************************
 *DESC: This function should be called before use the WSLog module.
 *PARA: appName     the name of application; 
 *                  if the length bigger than WSLOG_MAX_APP_NAME_LEN, 
 *                  the name will be truncated.
 *RETU: WSLOG_RETV_OPEN_SUCCESS
 *      WSLOG_RETV_ARG_EMPTY
 *      WSLOG_RETV_OPEN_FILE_FAIL
 *      WSLOG_RETV_ALLOC_MEM_FAIL
 *      WSLOG_RETV_DUP_FAIL
 *
 */
WSLogRetValue WSLogOpen(char *appName);

/***********************************************************************
 *DESC: This function log the message into files or stdout/stderr 
 *      which depends on the definition of WSLOG_DEBUG_ENABLE flag.
 *PARA: level     the log level of message.
 *RETU: WSLOG_RETV_ALLOC_MEM_FAIL
 *      WSLOG_RETV_SUCCESS
 *
 */
WSLogRetValue WSLogWrite(WSLogLevel level, const char *fmt, ...);

/***********************************************************************************
 *DESC: This function will flush all buffer into log files.
 *      If doesn't define the WSLOG_DEBUG_ENABLE flag this function will do nothing.
 *
 */
void WSLogFlush(void);

/***********************************************************************************
 *DESC: This function will close all open files and release the memory.
 *      If doesn't define the WSLOG_DEBUG_ENABLE flag this function will do nothing.
 *
 */
void WSLogClose(void);

#endif

























