/**
 *
 *  MITLogModule.h
 *  MITLogDefine
 *
 *  Created by gtliu on 7/3/13.
 *  Copyright (c) 2013 GT. All rights reserved.
 *  Email:  pcliuguangtao@163.com
 *
 *  Usage:
 *      #include "MITLogModule.h"
 *       int main() {
 *        MITLogOpen("TestApp");
 *         //...
 *        MITLogWrite(MITLOG_LEVEL_COMMON, "This is for common:%d Message:%s", 12, "Hello world");
 *         MITLogWrite(MITLOG_LEVEL_WARNING, "This is for warning:%d Message:%s", 12, "Hello world");
 *         MITLogFlush();
 *         MITLogWrite(MITLOG_LEVEL_ERROR, "This is for error:%d Message:%s", 12, "Hello world");
 *         //...
 *         MITLogClose();
 *         return 0;
 *     }
 */

#ifndef MITLogMoudle_H
#define MITLogMoudle_H

/** debug switch macro */
/**
 * If define the marco all message will print into stdout/stderr.
 * It can be used when you want to debug application.
 *
 * If don't all message will write into common/warning/error log files.
 * It can be used when you want to relase application
 */
//#define MITLOG_DEBUG_ENABLE      1

#define MITLOG_FILE_INDEX_NUM      3
typedef enum MITLogFileIndex {
    MITLOG_INDEX_COMM_FILE       = 0,                // num should start from 0 and keep continuately 
    MITLOG_INDEX_WARN_FILE       = 1,
    MITLOG_INDEX_ERROR_FILE      = 2
} MITLogFileIndex;

typedef enum MITLogLevel {
    MITLOG_LEVEL_COMMON          = 0,
    MITLOG_LEVEL_WARNING         = 1,
    MITLOG_LEVEL_ERROR           = 2
} MITLogLevel;

// define log file
#define MITLOG_LOG_FILE_PATH         "./"         

typedef enum MITLogMaxSize {
    MITLOG_MAX_FILE_NAME_PATH_LEN    = 150,           // absolute path
    MITLOG_MAX_APP_NAME_LEN          = 40,            // application name
    MITLOG_MAX_LOG_FILE_LEN          = 50,            // log file name max length
    MITLOG_MAX_FILE_SIZE             = 1024*1024*2,   /* 2MB not very exact as for system alloc
                                                       * disk space not by bytes(exp 4KB) */
    MITLOG_MAX_COMM_FILE_NUM         = 1,             // common type file num: appName.comm.1
    MITLOG_MAX_WARN_FILE_NUM         = 10,            // warning type file num: appName.warn.1 -- appName.warn.10
    MITLOG_MAX_ERROR_FILE_NUM        = 10,            // error type file num: appName.error.1 -- appName.error.10
    MITLOG_MAX_COMM_BUFFER_SIZE      = 1024*4,        // 4KB
    MITLOG_MAX_WARN_BUFFER_SIZE      = 1024,          // 1KB
    MITLOG_MAX_ERROR_BUFFER_SIZE     = 1024           // 1KB
}MITLogMaxSize;

typedef enum MITLogRetValue {
    MITLOG_RETV_SUCCESS              = 0,
    MITLOG_RETV_FAIL                 = -1,
    MITLOG_RETV_ARG_EMPTY            = -100,
    MITLOG_RETV_OPEN_FILE_FAIL       = -101,
    MITLOG_RETV_ALLOC_MEM_FAIL       = -102
} MITLogRetValue;

/**
 * This function should be called before use the MITLog module.
 * @param: appName     The name of application;
 *                     If the length bigger than MITLOG_MAX_APP_NAME_LEN,
 *                     the name will be truncated.
 * @returns: MITLOG_RETV_OPEN_SUCCESS
 *           MITLOG_RETV_ARG_EMPTY
 *           MITLOG_RETV_OPEN_FILE_FAIL
 *           MITLOG_RETV_ALLOC_MEM_FAIL
 *           MITLOG_RETV_DUP_FAIL
 *
 */
MITLogRetValue MITLogOpen(char *appName);

/**
 * This function log the message into files or stdout/stderr 
 *      which depends on the definition of MITLOG_DEBUG_ENABLE flag.
 * @param: level     the log level of message.
 * @returns: MITLOG_RETV_ALLOC_MEM_FAIL
 *           MITLOG_RETV_SUCCESS
 *
 */
MITLogRetValue MITLogWrite(MITLogLevel level, const char *fmt, ...);

/**
 * This function will flush all buffer into log files.
 * If doesn't define the MITLOG_DEBUG_ENABLE flag this function will do nothing.
 *
 */
void MITLogFlush(void);

/**
 * This function will close all open files and release the memory.
 * If doesn't define the MITLOG_DEBUG_ENABLE flag this function will do nothing.
 *
 */
void MITLogClose(void);

#endif

























