//
//  wslogger.c
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#define WS_derrprintf(fmt, args...) printf("%s %d ERROR:%s: "fmt"\n", __func__, __LINE__, strerror(errno), ##args)

//#define WS_DEBUG
#ifdef  WS_DEBUG
#define WS_dputs(str) printf("%s %d: %s\n", __func__, __LINE__, str)
#define WS_dprintf(fmt, args...) printf("%s %d: "fmt"\n", __func__, __LINE__, ##args)
#define WSLogEnter  printf("%s:%d %s\n", __func__, __LINE__, "Enter -->");
#define WSLogExit   printf("%s:%d %s\n", __func__, __LINE__, "<--Exist");
#else
#define WS_dputs(str)
#define WS_dprintf(fmt, args...)
#define WSLogEnter
#define WSLogExit
#endif

#include "WSLogModule.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#if WSLOG_DEBUG_ENABLE
static char const *WSLogLevelHeads[]  = {"[COMMON]", "[WARNING]", "[ERROR]"};

#else
static char const *WSLogFileSuffix[]  = {".comm", ".warn", ".err"};
static int WSLogFileMaxNum[] = {WSLOG_MAX_COMM_FILE_NUM, WSLOG_MAX_WARN_FILE_NUM,\
				 WSLOG_MAX_ERROR_FILE_NUM};

static long long WSLogBufferMaxSize[] = {WSLOG_MAX_COMM_BUFFER_SIZE,\
				 WSLOG_MAX_WARN_BUFFER_SIZE, WSLOG_MAX_ERROR_BUFFER_SIZE};

static char applicationName[WSLOG_MAX_APP_NAME_LEN];
static char *logFilePaths[WSLOG_FILE_INDEX_NUM];
static char *logFileBuffer[WSLOG_FILE_INDEX_NUM];

static pthread_rwlock_t bufferRWlock;     // use for the logFileBuffer

#endif
static FILE *originFilePointers[WSLOG_FILE_INDEX_NUM];


/*************************** Inner Tools Function ********************************/
#ifndef WSLOG_DEBUG_ENABLE
// return the log file path, exp: /data/logs/TestApp.comm
static inline void WSGetLogFilePathPrefix(WSLogFileIndex aryIndex, char *chrAry)
{
    const char *fileSuffix = WSLogFileSuffix[aryIndex];
    sprintf(chrAry, "%s%s%s", WSLOG_LOG_FILE_PATH, applicationName, fileSuffix);
}

// get origin file size and next should be updated log file's name
WSLogRetValue WSGetLogInfoByIndex(WSLogFileIndex aryIndex, char *nextStoreFile, long long *fileSize)
{
    const char *fileSuffix = WSLogFileSuffix[aryIndex];
    
    // pattern:  TestApp.comm.*
    char logNumPattern[WSLOG_MAX_LOG_FILE_LEN] = {0};
    sprintf(logNumPattern, "%s%s%s", applicationName, fileSuffix, ".*");
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
    long long partLen = strlen(applicationName) + strlen(fileSuffix) + 1;
    int currentMaxFileNum = 0;
    time_t minModifyTime = 0;
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
            WS_dputs(logfilepath);
            struct stat tstat;
            if (stat(logfilepath, &tstat) < 0) {
                WS_derrprintf("stat() %s faild", logfilepath);
                return WSLOG_RETV_FAIL;
            } else {
                *fileSize = tstat.st_size;
            }
        } else if (fnmatch(logNumPattern, dname, FNM_PATHNAME|FNM_PERIOD) == 0) {
            // get next store file
            int tt = atoi(&dname[partLen]);
            if (tt > currentMaxFileNum) {
                currentMaxFileNum = tt;
            }
            char logfilepath[WSLOG_MAX_FILE_NAME_PATH_LEN] = {0};
            sprintf(logfilepath, "%s%s", WSLOG_LOG_FILE_PATH, dname);
            WS_dputs(logfilepath);
            struct stat tstat;
            if (stat(logfilepath, &tstat) < 0) {
                WS_derrprintf("stat() %s faild", logfilepath);
                return WSLOG_RETV_FAIL;
            }
            if (minModifyTime == 0 || tstat.st_mtime < minModifyTime) {
                minModifyTime = tstat.st_mtime;
                memset(nextStoreFile, 0, WSLOG_MAX_FILE_NAME_PATH_LEN);
                strncpy(nextStoreFile, logfilepath, WSLOG_MAX_FILE_NAME_PATH_LEN);
            }
        }
    }
    WS_dprintf("currentMaxFileNum:%d  nextStoreFile:%s", currentMaxFileNum, nextStoreFile);
    if (currentMaxFileNum < WSLogFileMaxNum[aryIndex]) {
        ++currentMaxFileNum;
        memset(nextStoreFile, 0, WSLOG_MAX_FILE_NAME_PATH_LEN);
        sprintf(nextStoreFile, "%s%s%s.%d", WSLOG_LOG_FILE_PATH, applicationName,\
				 WSLogFileSuffix[aryIndex], currentMaxFileNum);
    }
    WS_dprintf("currentMaxFileNum:%d  nextStoreFile:%s", currentMaxFileNum, nextStoreFile);
    closedir(dirfd);
   
    return WSLOG_RETV_SUCCESS;
}

// use to copy file
void WSLogCopyFile(FILE *fromFP, FILE *toFP)
{
    char buf[1024*10];
    long long nread;
    // return to the start of the origin file
    fseek(fromFP, 0L, SEEK_SET);
    
    while ((nread=fread(buf, sizeof(char), 1024*10, fromFP))) {
        char *outp = buf;
        long long nwritten;
        do {
            nwritten = fwrite(outp, sizeof(char), nread, toFP);
            if (nwritten >= 0) {
                nread -= nwritten;
                outp += nwritten;
            } else if(errno != EINTR){
                break;
            }
        } while (nread > 0);
    }
}

// write buffer content into next store file
void WSLogWriteFile(WSLogFileIndex aryIndex, char *msgStr, long long msgSize)
{
    if (msgSize == 0) {
        return;
    }
    // 1. get log module info for special aryIndex
    char nextStoreFile[WSLOG_MAX_FILE_NAME_PATH_LEN] = {0};
    long long originFileSize  = 0;
    WSGetLogInfoByIndex(aryIndex, nextStoreFile, &originFileSize);
    long long fileLeftSize = WSLOG_MAX_FILE_SIZE - originFileSize;
    WS_dprintf("fileLeftSize:%lld  originFileWroteSize:%lld  msgSize:%lld", fileLeftSize,\
			 originFileSize, msgSize);
    
    // 2. check whether the origin file has enough space
    if (fileLeftSize < msgSize) {
        WS_dputs("Oringin file will be Written into next stroe file");
        // cp origin file(ex:TestApp.comm) to next stroe log file(ex:TestApp.comm)
        FILE *nextStoreFP = fopen(nextStoreFile, "w");
        if (nextStoreFP == NULL) {
            WS_derrprintf("fopen() %s faild", nextStoreFile);
        }
        WSLogCopyFile(originFilePointers[aryIndex], nextStoreFP);
        if (fflush(nextStoreFP) !=0 ) {
            WS_derrprintf("fflush() failed: %s", nextStoreFile);
        }
        fclose(nextStoreFP);
        // empty the origin file this step MUST be executed.
        ftruncate(fileno(originFilePointers[aryIndex]), 0);
    }
    
    // 3. write buffer into origin file
    char *outp = msgStr;
    long long num = msgSize;
    long long nwritten;
    do {
        nwritten = fwrite(outp, sizeof(char), num, originFilePointers[aryIndex]);
        if (nwritten >= 0) {
            num -= nwritten;
            outp += nwritten;
        } else if(errno != EINTR){
            break;
        }
    } while (num > 0);
    if (fflush(originFilePointers[aryIndex]) !=0 ) {
        WS_derrprintf("fflush() failed: %d", aryIndex);
    }
}
#endif
// get the right aryIndex for special log level
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

/*************************** WSLog Module Function ********************************/
WSLogRetValue WSLogOpen(char *appName)
{    
#if WSLOG_DEBUG_ENABLE
    originFilePointers[WSLOG_INDEX_COMM_FILE] = stdout;
    originFilePointers[WSLOG_INDEX_WARN_FILE] = stderr;
    originFilePointers[WSLOG_INDEX_ERROR_FILE] = stderr;
    return WSLOG_RETV_SUCCESS;
#else
    if (strlen(appName) == 0) {
        return WSLOG_RETV_ARG_EMPTY;
    }
    strncpy(applicationName, appName, WSLOG_MAX_APP_NAME_LEN);
    // init the read/write lock
    int ret = pthread_rwlock_init(&bufferRWlock, NULL);
    if (ret != 0) {
        WS_dprintf("pthread_rwlock_init() failed:%d", ret);
        return WSLOG_RETV_FAIL;
    }
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
        logFileBuffer[i] = (char *)calloc(WSLogBufferMaxSize[i], sizeof(char));
        if (!logFileBuffer[i]) {
            for (int j=WSLOG_INDEX_COMM_FILE; j<WSLOG_INDEX_ERROR_FILE; ++j) {
                free(logFilePaths[j]);
                logFilePaths[j] = NULL;
            }
            for (int j=WSLOG_INDEX_COMM_FILE; j<i; ++j) {
                free(logFileBuffer[j]);
                logFileBuffer[j] = NULL;
            }
            WS_derrprintf("Allocate memroy Faild");
            return WSLOG_RETV_ALLOC_MEM_FAIL;
        }
    }
    // open files
    for (int i = WSLOG_INDEX_COMM_FILE; i<= WSLOG_INDEX_ERROR_FILE; ++i) {
        sprintf(logFilePaths[i], "%s%s%s", WSLOG_LOG_FILE_PATH, appName, WSLogFileSuffix[i]);
        originFilePointers[i] = fopen(logFilePaths[i], "a+");
        if (!originFilePointers[i]) {
            for (int j=WSLOG_INDEX_COMM_FILE; j<=WSLOG_INDEX_ERROR_FILE; ++j) {
                free(logFilePaths[j]);
                logFilePaths[j] = NULL;
                free(logFileBuffer[j]);
                logFileBuffer[j] = NULL;
            }
            for (int j=WSLOG_INDEX_COMM_FILE; j<i; ++j) {
                fclose(originFilePointers[j]);
                originFilePointers[j] = NULL;
            }
            WS_derrprintf("Open %s Faild", logFilePaths[i]);
            return WSLOG_RETV_OPEN_FILE_FAIL;
        }
    }
    
    return WSLOG_RETV_SUCCESS;
#endif
}

WSLogRetValue WSLogWrite(WSLogLevel level, const char *fmt, ...)
{
    // 1. generate the string
    int n, size = 100;      // suppose the message need no more than 100 bytes
    char *tarp = NULL, *np = NULL;
    va_list ap;
    if ((tarp = malloc(size)) == NULL) {
        return WSLOG_RETV_ALLOC_MEM_FAIL;
    }
    while (1) {
        // try to print the allocated space
        va_start(ap, fmt);
        n = vsnprintf(tarp, size, fmt, ap);
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
        if ((np = realloc(tarp, size)) == NULL) {
            free(tarp);
            return WSLOG_RETV_ALLOC_MEM_FAIL;
        } else {
            tarp = np;
        }
    }
    // 2. add prefix and suffix to the message
    time_t now = time(NULL);
    char timeStr[48] = {0};
    snprintf(timeStr, 48, "[%ld]", now);
    long long sumLen = strlen(timeStr) + 1 + strlen(tarp) \
			+ strlen("\n") + 5;   // 5 keep enough space and last space is '\0'
    char *tarMessage = (char *)calloc(sumLen, sizeof(char));
    if (!tarMessage) {
        free(tarp);
        tarp = NULL;
        return WSLOG_RETV_ALLOC_MEM_FAIL;
    }
    sprintf(tarMessage, "%s %s\n", timeStr, tarp);
    sumLen = strlen(tarMessage);
    
    free(tarp);
    np = tarp = NULL;
    //WS_dprintf("target message:%s", tarMessage);
    WSLogFileIndex aryIndex = WSGetIndexForLevel(level);
    
#if WSLOG_DEBUG_ENABLE
    fprintf(originFilePointers[aryIndex], "%-10s%s", WSLogLevelHeads[aryIndex], tarMessage);
#else
    // 3. add target message into buffer or file
    pthread_rwlock_wrlock(&bufferRWlock);
    long long bufferUesedSize = strlen(logFileBuffer[aryIndex]);
    if (WSLogBufferMaxSize[aryIndex] - bufferUesedSize > sumLen) {
        // write into buffer
        strcat(logFileBuffer[aryIndex], tarMessage);
        pthread_rwlock_unlock(&bufferRWlock);
    } else {
        WS_dputs("String write into file*****************");
        pthread_rwlock_unlock(&bufferRWlock);
        
        pthread_rwlock_rdlock(&bufferRWlock);
        // 1. flush the buffer
        long long firstLen = strlen(logFileBuffer[aryIndex]);
        WSLogWriteFile(aryIndex, logFileBuffer[aryIndex], firstLen);
        pthread_rwlock_unlock(&bufferRWlock);
        
        pthread_rwlock_wrlock(&bufferRWlock);
        long long secondLen = strlen(logFileBuffer[aryIndex]);
        long long subNum = secondLen - firstLen;
        if (subNum == 0) {
            // empty the buffer
            memset(logFileBuffer[aryIndex], 0, firstLen);
        } else if(subNum > 0){
            memset(logFileBuffer[aryIndex], 0, firstLen);
            memmove(logFileBuffer[aryIndex], logFileBuffer[aryIndex]+firstLen, subNum);
            memset(logFileBuffer[aryIndex]+subNum, 0, WSLogBufferMaxSize[aryIndex]-subNum);
        }
        // 2. write into origin file or buffer
        if (sumLen < WSLogBufferMaxSize[aryIndex]) {
            // write into buffer
            strcat(logFileBuffer[aryIndex], tarMessage);
            pthread_rwlock_unlock(&bufferRWlock);
        } else {
            pthread_rwlock_unlock(&bufferRWlock);
            // write into origin file directly
            WSLogWriteFile(aryIndex, tarMessage, sumLen);
        }
    }
    
    free(tarMessage);
    tarMessage = NULL;
#endif
    return WSLOG_RETV_SUCCESS;
}

void WSLogFlush(void)
{
#ifndef WSLOG_DEBUG_ENABLE
    for (int i=WSLOG_INDEX_COMM_FILE; i<=WSLOG_INDEX_ERROR_FILE; ++i) {
        WSLogWriteFile(i, logFileBuffer[i], strlen(logFileBuffer[i]));
        fflush(originFilePointers[i]);
    }
#endif
}

void WSLogClose(void)
{
#ifndef WSLOG_DEBUG_ENABLE
    // 1. flush the buffers
    WSLogFlush();
    // 2. release the resources
    for (int j=WSLOG_INDEX_COMM_FILE; j<=WSLOG_INDEX_ERROR_FILE; ++j) {
        fclose(originFilePointers[j]);
        originFilePointers[j] = NULL;
        
        free(logFilePaths[j]);
        logFilePaths[j] = NULL;
        free(logFileBuffer[j]);
        logFileBuffer[j] = NULL;
    }
    // 3. destroy the rwlock
    int ret = pthread_rwlock_destroy(&bufferRWlock);
    if (ret != 0) {
        WS_dprintf("pthread_rwlock_destroy() failed:%d", ret);
    }
#endif
}






















