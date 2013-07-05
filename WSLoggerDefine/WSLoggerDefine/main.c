//
//  main.c
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//
#include <stdio.h>
#include <unistd.h>
#include "WSLogModule.h"
int main(int argc, const char * argv[])
{
    // insert code here...
    char dir[1024];
    getcwd(dir, sizeof(dir));
    printf("%s\n", dir);
    printf("Hello, World!\n");
    WSLogOpen("TestApp");
    
    for (int i=WSLOG_INDEX_COMM_FILE; i<=WSLOG_INDEX_ERROR_FILE; ++i) {
        for (int j=0; j < 1000; ++j) {
            WSLogWrite(WSLOG_LEVEL_COMMON, "This is for common:%d", j);
            WSLogWrite(WSLOG_LEVEL_WARNING, "This is for warning:%d", j);
            WSLogWrite(WSLOG_LEVEL_ERROR, "This is for error:%d", j);
        }
    }
    
    WSLogClose();
    
    return 0;
}

