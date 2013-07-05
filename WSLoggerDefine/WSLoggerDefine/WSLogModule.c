//
//  wslogger.c
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#define WS_DEBUG
#ifdef WS_DEBUG
#define WS_dputs(str) printf("%s %d:  %s\n", __func__, __LINE__, str)
#define WS_dprintf(fmt, args...) printf("%s %d:  "fmt"\n", __func__, __LINE__, ##args)
#define WS_derrprintf(fmt, args...) printf("%s %d ERROR:%s:  "fmt"\n", __func__, __LINE__, strerror(errno), ##args)
#define WSLogEnter  printf(@"%s:%d %@", __FUNCTION__, __LINE__, @"Enter -->");
#define WSLogExit   printf(@"%s:%d %@", __FUNCTION__, __LINE__, @"<--Exist");
#else
#define WS_dputs(str)
#define WS_dprintf(fmt, args...)
#define WS_derrprintf(fmt, args...)
#define WSLogEnter
#define WSLogExit
#endif

#include "WSLogModule.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <time.h>
#include <stdarg.h>

static char const *WSLogFileSuffix[]  = {".comm", ".warn", ".err"};
static char const *WSLogLevelHeads[]  = {"[COMMON]:", "[WARNING]:", "[ERROR]:"};
static int WSLogFileMaxNum[] = {WSLOG_MAX_COMM_FILE_NUM, WSLOG_MAX_WARN_FILE_NUM, WSLOG_MAX_ERROR_FILE_NUM};
static size_t WSLogBufferMaxSize[] = {WSLOG_MAX_COMM_BUFFER_SIZE, WSLOG_MAX_WARN_BUFFER_SIZE, WSLOG_MAX_ERROR_BUFFER_SIZE};

static char nextStoreLogFileName[WSLOG_FILE_INDEX_NUM][WSLOG_MAX_FILE_NAME_PATH_LEN];
static off_t originFileWroteSize[WSLOG_FILE_INDEX_NUM];
static char applicationName[WSLOG_MAX_APP_NAME_LEN];

static FILE *logFilePointers[WSLOG_FILE_INDEX_NUM];
static char *logFilePaths[WSLOG_FILE_INDEX_NUM];
static char *logFileBuffer[WSLOG_FILE_INDEX_NUM];
static size_t bufferUesedSize[WSLOG_FILE_INDEX_NUM];

/*************************** Inner Tools Function ********************************/
static inline void WSGetLogFilePathPrefix(WSLogFileIndex index, char *chrAry)
{
    const char *fileSuffix = WSLogFileSuffix[index];
    sprintf(chrAry, "%s%s%s", WSLOG_LOG_FILE_PATH, applicationName, fileSuffix);
}
// get the right index for special log level
static inline WSLogFileIndex WSGetIndexForLevel(WSLogLevel level)
{
    switch (level) {
        case WSLOG_LEVEL_COMMON:
            return WSLOG_INDEX_COMM_FILE;
            break;
        case WSLOG_LEVEL_WARNING:
            return WSLOG_INDEX_WARN_FILE;
            break;
        case WSLOG_LEVEL_ERROR:
            return WSLOG_INDEX_ERROR_FILE;
            break;
        default:
            break;
    }
}

WSLogRetValue getLogInfoByIndex(WSLogFileIndex index)
{
    const char *fileSuffix = WSLogFileSuffix[index];
    char *tarfile = nextStoreLogFileName[index];
    
    // pattern:  TestApp.comm.*
    char logNumPattern[WSLOG_MAX_LOG_FILE_LEN] = {0};
    sprintf(logNumPattern, "%s%s%s", applicationName, fileSuffix, ".*");
    WS_dputs(logNumPattern);
    // origin log file name
    char originFileName[WSLOG_MAX_LOG_FILE_LEN] = {0};
    sprintf(originFileName, "%s%s", applicationName, fileSuffix);
    
    DIR *dirfd = NULL;
    struct dirent *entry = NULL;
    
    if ((dirfd = opendir(WSLOG_LOG_FILE_PATH)) == NULL) {
        WS_derrprintf("opendir() faild");
        return WSLOG_RETV_FAIL;
    }
    // 1 mean the '.' between num and suffix. TestApp.comm.1
    size_t partLen = strlen(applicationName) + strlen(fileSuffix) + 1; 
    int currentMaxFileNum = 0;
    time_t minModifyTime = time(NULL);
    while ((entry = readdir(dirfd)) != NULL) {
        char *dname = entry->d_name;
        if (strcmp(dname, ".") == 0 ||
            strcmp(dname, "..") == 0) {
            continue;
        }
        // get origin log file size
        if (0 == strcmp(dname, originFileName)) {
            char logfilepath[WSLOG_MAX_FILE_NAME_PATH_LEN] = {0};
            sprintf(logfilepath, "%s%s", WSLOG_LOG_FILE_PATH, originFileName);
            //WS_dputs(logfilepath);
            struct stat tstat;
            if (stat(logfilepath, &tstat) < 0) {
                WS_derrprintf("stat() %s faild", logfilepath);
                return WSLOG_RETV_FAIL;
            } else {
                originFileWroteSize[index] = tstat.st_size;
            }
        }
        // get next store file
        if (fnmatch(logNumPattern, dname, FNM_PATHNAME|FNM_PERIOD) == 0) {
            int tt = atoi(&dname[partLen]);
            if (tt > currentMaxFileNum) {
                currentMaxFileNum = tt;
            }
            char logfilepath[WSLOG_MAX_FILE_NAME_PATH_LEN] = {0};
            sprintf(logfilepath, "%s%s", WSLOG_LOG_FILE_PATH, dname);
            //WS_dputs(logfilepath);
            struct stat tstat;
            if (stat(logfilepath, &tstat) < 0) {
                WS_derrprintf("stat() %s faild", logfilepath);
                return WSLOG_RETV_FAIL;
            }
            if (tstat.st_mtime < minModifyTime) {
                minModifyTime = tstat.st_mtime;
                memset(tarfile, 0, WSLOG_MAX_FILE_NAME_PATH_LEN);
                strncpy(tarfile, logfilepath, WSLOG_MAX_FILE_NAME_PATH_LEN);
            }
        }
    }
    if (currentMaxFileNum < WSLogFileMaxNum[index]) {
        ++currentMaxFileNum;
        memset(tarfile, 0, strlen(tarfile));
        sprintf(tarfile, "%s%s%s.%d", WSLOG_LOG_FILE_PATH, applicationName, WSLogFileSuffix[index], currentMaxFileNum);
    }
    WS_dprintf("currentMaxFileNum:%d", currentMaxFileNum);
    
    closedir(dirfd);
    
    return WSLOG_RETV_SUCCESS;
}

void getCurrentLogFilesInfo(void)
{
    // get current log file nums
    for (int i=WSLOG_INDEX_COMM_FILE; i<=WSLOG_INDEX_ERROR_FILE; ++i) {
        if(getLogInfoByIndex(i) != WSLOG_RETV_SUCCESS){
            char originFileName[WSLOG_MAX_LOG_FILE_LEN] = {0};
            sprintf(originFileName, "%s%s", applicationName, WSLogFileSuffix[i]);
            char logfilepath[WSLOG_MAX_FILE_NAME_PATH_LEN] = {0};
            sprintf(logfilepath, "%s%s", WSLOG_LOG_FILE_PATH, originFileName);
            
            WS_dprintf("WARNING: %s will be trucate into 0", logfilepath);
            truncate(logfilepath, 0);
            originFileWroteSize[i] = 0;
            
            WS_dputs("WARNING: nextStoreLogFileName will be reset to 1");
            strcat(logfilepath, ".1");
            strncpy(nextStoreLogFileName[i], logfilepath, WSLOG_MAX_FILE_NAME_PATH_LEN);
        }
        WS_dprintf("nextStoreLogFileName:%s", nextStoreLogFileName[i]);
        WS_dprintf("originFileWroteSize:%lld", originFileWroteSize[i]);
    }
}

/*************************** WSLog Module Function ********************************/
WSLogRetValue WSLogOpen(char *appName)
{
    if (strlen(appName) == 0) {
        return WSLOG_RETV_ARG_EMPTY;
    }
    strncpy(applicationName, appName, WSLOG_MAX_APP_NAME_LEN);
    
#if WSLOG_DEBUG_ENABLE
    logFilePointers[WSLOG_INDEX_COMM_FILE] = stdout;
    logFilePointers[WSLOG_INDEX_WARN_FILE] = stderr;
    logFilePointers[WSLOG_INDEX_ERROR_FILE] = stderr;
    return WSLOG_RETV_SUCCESS;
#else
    // alloc memory
    for (int i = WSLOG_INDEX_COMM_FILE; i<= WSLOG_INDEX_ERROR_FILE; ++i) {
        logFilePaths[i] = (char *)calloc(WSLOG_MAX_FILE_NAME_PATH_LEN, sizeof(char));
        if (!logFilePaths[i]) {
            for (int j=WSLOG_INDEX_COMM_FILE; j<i; ++j) {
                free(logFilePaths[j]);
                logFilePaths[j] = NULL;
            }
            WS_derrprintf("Allocate memroy Faild");
            return WSLOG_RETV_ALLOC_MEM_FAIL;
        }
    }
    // open files
    for (int i = WSLOG_INDEX_COMM_FILE; i<= WSLOG_INDEX_ERROR_FILE; ++i) {
        sprintf(logFilePaths[i], "%s%s%s", WSLOG_LOG_FILE_PATH, appName, WSLogFileSuffix[i]);
        logFilePointers[i] = fopen(logFilePaths[i], "a");
        if (!logFilePointers[i]) {
            for (int j=WSLOG_INDEX_COMM_FILE; j<=WSLOG_INDEX_ERROR_FILE; ++j) {
                free(logFilePaths[j]);
                logFilePaths[j] = NULL;
            }
            for (int j=WSLOG_INDEX_COMM_FILE; j<i; ++j) {
                fclose(logFilePointers[j]);
                logFilePointers[j] = NULL;
            }
            WS_derrprintf("Open %s Faild", logFilePaths[i]);
            return WSLOG_RETV_OPEN_FILE_FAIL;
        }
    }
    // init log module into
    getCurrentLogFilesInfo();	
    
    return WSLOG_RETV_SUCCESS;
#endif
}

WSLogRetValue WSLogWrite(WSLogLevel level, const char *fmt, ...)
{
    // generate the string
    int n, size = 100;  // suppose the message need no more than 100 bytes
    char *p = NULL, *np = NULL;
    va_list ap;
    if ((p = malloc(size)) == NULL) {
        return WSLOG_RETV_ALLOC_MEM_FAIL;
    }
    while (1) {
        // try to print the allocated space
        va_start(ap, fmt);
        n = vsnprintf(p, size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size) {
            break;
        }
        // try more space
        if (n > -1) {      // glibc 2.1
            size = n + 1;  // 1 is for the '\0'
        } else {           // glibc 2.0
            size = n * 2;  // twice the old size
        }
        if ((np = realloc(p, size)) == NULL) {
            free(p);
            return WSLOG_RETV_ALLOC_MEM_FAIL;
        } else {
            p = np;
        }
    }
    WS_dprintf("target message:%s", p);
    WSLogFileIndex index = WSGetIndexForLevel(level);
    bufferUesedSize[index] = strlen(logFileBuffer[index]);
    size_t msgLen = strlen(p);
    WS_dprintf("bufferUesedSize[%d]:%zd  tarmsg lenght:%zd", index, bufferUesedSize[index], msgLen);
    if (WSLogBufferMaxSize[index] - bufferUesedSize[index] > msgLen + 2) { // 2 means '\n' and '\0'
        strcat(logFileBuffer[index], p);
        strcat(logFileBuffer[index], "\n");
        bufferUesedSize[index] += msgLen + 2;
    } else {
        // flush the buffer
        WSLogFlush(index);
        if (msgLen + 2 <= WSLogBufferMaxSize[index]) {
            strcat(logFileBuffer[index], p);
            strcat(logFileBuffer[index], "\n");
            bufferUesedSize[index] += msgLen + 2;
        } else {
            // write into file directly
            
        }
    }
    
}

int WSLogWriteFile(WSLogFileIndex index, char *msgStr, size_t amountSize)
{
    size_t fileLeftSize = WSLOG_MAX_FILE_SIZE - originFileWroteSize[index];
    if (fileLeftSize < WSLogBufferMaxSize[index]) {
        int ret = remove(nextStoreLogFileName[index]);
        if (ret == -1) {
            WS_derrprintf("remove() %s failed", nextStoreLogFileName[index]);
            return 0;
        }
        
    }
}

WSLogRetValue WSLogFlush(WSLogFileIndex index)
{
    
}

WSLogRetValue WSLoggerClose(void)
{
    
}
