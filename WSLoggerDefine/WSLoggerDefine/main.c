//
//  main.c
//  WSLoggerDefine
//
//  Created by gtliu on 7/3/13.
//  Copyright (c) 2013 GT. All rights reserved.
//
#include <stdio.h>
#include <unistd.h>
#include "wslogger.h"
int main(int argc, const char * argv[])
{
    // insert code here...
    char dir[1024];
    getcwd(dir, sizeof(dir));
    printf("%s\n", dir);
    printf("Hello, World!\n");
    WSLoggerOpen("TestApp", WSLOG_DEBUG_DISABLE);
    return 0;
}
