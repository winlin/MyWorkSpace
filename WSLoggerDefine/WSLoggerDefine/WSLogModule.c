//
//  wslogger.c
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "WSLogModule.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

static char const *WSLogFileTails[]   = {".comm", ".warn", ".err"};
static char const *WSLogLevelHeads[]  = {"[COMMON]:", "[WARNING]:", "[ERROR]:"};

static int currentWroteSize[WSLOG_FILE_INDEX_NUM];
static FILE *logFilePointers[WSLOG_FILE_INDEX_NUM];
static char *logFilePaths[WSLOG_FILE_INDEX_NUM];
static char *logFileBuffer[WSLOG_FILE_INDEX_NUM];

WSLogRetValue WSLogOpen(char *appName)
{
    if (strlen(appName) == 0) {
        return WSLOG_RETV_ARG_EMPTY;
    }
    
#if WSLOG_DEBUG_ENABLE
    logFilePointers[WSLOG_COMM_FILE_INDEX] = stdout;
    logFilePointers[WSLOG_WARN_FILE_INDEX] = stderr;
    logFilePointers[WSLOG_ERROR_FILE_INDEX] = stderr;
    return WSLOG_RETV_OPEN_SUCCESS;
#else
    // alloc memory
    for (int i = WSLOG_COMM_FILE_INDEX; i<= WSLOG_ERROR_FILE_INDEX; ++i) {
        logFilePaths[i] = (char *)calloc(WSLOG_MAX_FILE_NAME_LEN, sizeof(char));
        if (!logFilePaths[i]) {
            for (int j=WSLOG_COMM_FILE_INDEX; j<i; ++j) {
                free(logFilePaths[j]);
                logFilePaths[j] = NULL;
            }
            printf("%s%s:%d  Allocate memroy Faild\n", WSLogLevelHeads[WSLOG_LEVEL_ERROR], __func__, __LINE__);
            return WSLOG_RETV_ALLOC_MEM_FAIL;
        }
    }
    // open files
    for (int i = WSLOG_COMM_FILE_INDEX; i<= WSLOG_ERROR_FILE_INDEX; ++i) {
        sprintf(logFilePaths[i], "%s%s%s", WSLOG_LOG_FILE_PATH, appName, WSLogFileTails[i]);
        logFilePointers[i] = fopen(logFilePaths[i], "a");
        if (!logFilePointers[i]) {
            for (int j=WSLOG_COMM_FILE_INDEX; j<=WSLOG_ERROR_FILE_INDEX; ++j) {
                free(logFilePaths[j]);
                logFilePaths[j] = NULL;
            }
            for (int j=WSLOG_COMM_FILE_INDEX; j<i; ++j) {
                fclose(logFilePointers[j]);
                logFilePointers[j] = NULL;
            }
            printf("%s%s:%d Open %s Faild ERROR:%s\n", WSLogLevelHeads[WSLOG_LEVEL_ERROR], __func__, __LINE__, logFilePaths[i], strerror(errno));
            return WSLOG_RETV_OPEN_FILE_FAIL;
        }
    }
    // duplicate stdout 
    if (-1 == dup2(fileno(logFilePointers[WSLOG_COMM_FILE_INDEX]), fileno(stdout))) {        
        for (int j=WSLOG_COMM_FILE_INDEX; j<=WSLOG_ERROR_FILE_INDEX; ++j) {
            free(logFilePaths[j]);
            logFilePaths[j] = NULL;
            fclose(logFilePointers[j]);
            logFilePointers[j] = NULL;
        }            
        printf("%s%s:%d dup2() faild ERROR:%s\n", WSLogLevelHeads[WSLOG_LEVEL_ERROR], __func__, __LINE__, strerror(errno));
        return WSLOG_RETV_DUP_FAIL;
    }
    // duplicate stderr
    if (-1 == dup2(fileno(logFilePointers[WSLOG_ERROR_FILE_INDEX]), fileno(stderr))) {
        for (int j=WSLOG_COMM_FILE_INDEX; j<=WSLOG_ERROR_FILE_INDEX; ++j) {
            free(logFilePaths[j]);
            logFilePaths[j] = NULL;
            fclose(logFilePointers[j]);
            logFilePointers[j] = NULL;
        }
        printf("%s%s:%d dup2() faild ERROR:%s\n", WSLogLevelHeads[WSLOG_LEVEL_ERROR], __func__, __LINE__, strerror(errno));
        return WSLOG_RETV_DUP_FAIL;
    }
    return WSLOG_RETV_OPEN_SUCCESS;
#endif
}

int WSLogWrite(WSLogLevel level, const char *fmt, ...)
{
    
}

int WSLoggerClose(void)
{
    
}
