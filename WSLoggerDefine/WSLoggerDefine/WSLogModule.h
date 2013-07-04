//
//  wslogger.h
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef WSLoggerDefine_wslogger_h
#define WSLoggerDefine_wslogger_h

// Define log file path
#define WSLOG_LOG_FILE_PATH               "./"
#define WSLOG_MAX_FILE_NAME_LEN           150
#define WSLOG_COMM_FILE_TAIL              ".comm"
#define WSLOG_WARN_FILE_TAIL              ".warn"
#define WSLOG_ERROR_FILE_TAIL             ".err"
#define WSLOG_COMM_MSG_HEAD               "[COMMON]:"
#define WSLOG_WARN_MSG_HEAD               "[WARNING]:"
#define WSLOG_ERROR_MSG_HEAD              "[ERROR]:"

// Define erro macro
#define WSLOG_ARG_EMPTY_ERROR             -100
#define WSLOG_OPEN_FILE_FAIL              -101
#define WSLOG_OPEN_SUCCESS                0

// Define debug switch macro
#define WSLOG_DEBUG_ENABLE           1
#define WSLOG_DEBUG_DISABLE          0

typedef enum WSLoggerLevel {
    WSLOGGER_COMMON     = 1,
    WSLOGGER_WARNING    = 2,
    WSLOGGER_ERROR      = 3
} WSLoggerLevel;

int WSLoggerOpen(char *appName, int debugFlag);
int WSLoggerWrite(WSLoggerLevel level, const char *fmt, ...);
int WSLoggerClose(void);

#endif
