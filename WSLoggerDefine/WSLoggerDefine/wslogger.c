//
//  wslogger.c
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "wslogger.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

static FILE *comFile;
static FILE *warnFile;
static FILE *errorFile;
static char comFilePath[WSLOG_MAX_FILE_NAME_LEN];
static char warnFilePath[WSLOG_MAX_FILE_NAME_LEN];
static char errorFilePath[WSLOG_MAX_FILE_NAME_LEN];

int WSLoggerOpen(char *appName, int debugFlag)
{
    if (strlen(appName) == 0) {
        return WSLOG_ARG_EMPTY_ERROR;
    }
    if (debugFlag == WSLOG_DEBUG_ENABLE) {
        comFile = stdout;
        warnFile = errorFile = stderr;
        return WSLOG_OPEN_SUCCESS;
    } else if (debugFlag == WSLOG_DEBUG_DISABLE) {
        sprintf(comFilePath, WSLOG_LOG_FILE_PATH"%s"WSLOG_COMM_FILE_TAIL, appName);
        comFile = fopen(comFilePath, "a");
        if (!comFile) {
            printf(WSLOG_ERROR_MSG_HEAD"%s:%d Open %s Faild ERROR:%s", __func__, __LINE__, comFilePath, strerror(errno));
            return WSLOG_OPEN_FILE_FAIL;
        }
        
        sprintf(warnFilePath, WSLOG_LOG_FILE_PATH"%s"WSLOG_WARN_FILE_TAIL, appName);
        warnFile = fopen(warnFilePath, "a");
        if (!warnFile) {
            printf(WSLOG_ERROR_MSG_HEAD"%s:%d Open %s Faild ERROR:%s", __func__, __LINE__, warnFilePath, strerror(errno));
            return WSLOG_OPEN_FILE_FAIL;
        }
        
        sprintf(errorFilePath, WSLOG_LOG_FILE_PATH"%s"WSLOG_ERROR_FILE_TAIL, appName);
        errorFile = fopen(errorFilePath, "a");
        if (!errorFile) {
            printf(WSLOG_ERROR_MSG_HEAD"%s:%d Open %s Faild ERROR:%s", __func__, __LINE__, errorFilePath, strerror(errno));
            return WSLOG_OPEN_FILE_FAIL;
        }
        return WSLOG_OPEN_SUCCESS;
    } else
        return WSLOG_OPEN_FILE_FAIL;
}

int WSLoggerWrite(WSLoggerLevel level, const char *fmt, ...)
{
    
}
int WSLoggerClose(void)
{
    
}