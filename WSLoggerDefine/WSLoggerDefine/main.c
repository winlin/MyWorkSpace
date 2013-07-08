//
//  main.c
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include "WSLogModule.h"


void *tFucOne(void *arg);
void *tFuncTwo(void *arg);

int main(int argc, const char * argv[])
{
    char dir[1024];
    getcwd(dir, sizeof(dir));
    printf("%s\n", dir);
    
    // 1. usage in one thread demo
    // at first in main thread call WSLogOpen()
    WSLogOpen("TestApp");
    puts("just have a try and feel the speed");
    time_t starttime = time(NULL);
    for (int i=WSLOG_INDEX_COMM_FILE; i<=WSLOG_INDEX_ERROR_FILE; ++i) {
        for (int j=0; j < 100000; ++j) {
            WSLogWrite(WSLOG_LEVEL_COMMON, "This is for common:%d", j);
            WSLogWrite(WSLOG_LEVEL_WARNING, "This is for warning:%d", j);
            WSLogWrite(WSLOG_LEVEL_ERROR, "This is for error:%d", j);
        }
    }
    time_t closetime = time(NULL);
    printf("All time:%ld\n", closetime-starttime);
    // at last you should close the log module
    WSLogClose();
    
    // 2. test the thread safe
    WSLogOpen("ThreadTest");
    puts("start thread safe test");
    pthread_t oneThread, twoThread;
    int ret = 0;
    ret = pthread_create(&oneThread, NULL, tFucOne, NULL);
    if (ret != 0) {
        perror("Thread one start failed\n");
    }
    ret = pthread_create(&twoThread, NULL, tFuncTwo, NULL);
    if (ret != 0) {
        perror("Thread two start failed\n");
    }

    pthread_join(oneThread, NULL);
    pthread_join(twoThread, NULL);
    puts("thread safe test over, you can use 'wc -l ThreadTest.comm' command to check the data's integerity");
    WSLogClose();
    
    // 3. test the process safe
    puts("start process safe test");
    WSLogOpen("ProcessTest");
    
    pid_t pid;
    pid = fork();
    int pInteger = 1;
    int childEnd = 0;
    if (pid < 0) {
        perror("forl() failed");
    }
    else if (pid > 0) {
        // parent process
        while (pInteger <= 1000 || childEnd == 0) {
            if (pInteger <= 1000) {
                WSLogWrite(WSLOG_LEVEL_COMMON, "parent process :%d", pInteger);
                pInteger += 2;
            }
            usleep(random()%800);
            if (waitpid(-1, NULL, WNOHANG) > 0) {
                childEnd = 1;
            }
        }
        puts("end process safe test");
    } else if (pid == 0) {
        // child process
        for (int i=1; i<=1000; i+=2) {
            WSLogWrite(WSLOG_LEVEL_COMMON, "child process :%d", i);
            usleep(random()%800);
        }
    }
    WSLogClose();
    return 0;
}

void *tFucOne(void *arg)
{
    for (int i=1; i<=10000; i+=2) {
        WSLogWrite(WSLOG_LEVEL_COMMON, "thread one :%d", i);
        usleep(random()%800);
    }
}
void *tFuncTwo(void *arg)
{
    for (int i=2; i<=10000; i+=2) {
        WSLogWrite(WSLOG_LEVEL_COMMON, "thread two :%d", i);
        usleep(random()%800);
    }
}

