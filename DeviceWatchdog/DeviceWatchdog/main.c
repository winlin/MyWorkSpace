//
//  main.c
//  DeviceWatchdog
//
//  Created by gtliu on 7/18/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "MITLogModule.h"
#include "wd_configure.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, const char * argv[])
{
    MITLogOpen("DeviceWatchdog", "./logs");
    
    char dir[1024];
    getcwd(dir, sizeof(dir));
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s", dir);
    
    /**
     * 1. convert self into a daemon
     *    don't change process's working directory to '/'
     *    don't redirect stdin/stdou/stderr
     */ 
	int ret = daemon(1, 1);
	if(ret == -1) {
		MITLog_DetErrPrintf("call daemon() failed!");
	}
	MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "daemon ppid:%d pid:%d \n",  getppid(), getpid());
    
    struct wd_configure *wd_conf = get_wd_configure();
    if (wd_conf == NULL) {
        MITLog_DetPuts(MITLOG_LEVEL_ERROR, "get_wd_configure() failed");
        goto CLOSE_LOG_TAG;
    }
    print_wd_configure(wd_conf);
    
    
    
    
    
    
    
    free_wd_configure(wd_conf);
    MITLogClose();
    return 0;
    
CLOSE_LOG_TAG:
    MITLogClose();
    return -1;
}

