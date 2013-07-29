//
//  watchdog_main.c
//
//  Created by gtliu on 7/18/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "../include/mit_log_module.h"
#include "../include/mit_data_define.h"
#include "wd_configure.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, const char * argv[])
{    
    /**
     * 1. convert self into a daemon
     *    don't change process's working directory to '/'
     *    don't redirect stdin/stdou/stderr
     */
    int ret = 0;
	ret = daemon(1, 1);
	if(ret == -1) {
		MITLog_DetErrPrintf("call daemon() failed!");
	}
    MITLogOpen("DeviceWatchdog", WD_FILE_PATH_LOG);
    
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "daemon ppid:%d pid:%d",  getppid(), getpid());
    
    char dir[1024];
    char *cwd_char = getcwd(dir, sizeof(dir));
    if (cwd_char == NULL) {
        MITLog_DetErrPrintf("getcwd() failed");
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s", dir);
    
    struct wd_configure *wd_conf = get_wd_configure();
    if (wd_conf == NULL) {
        MITLog_DetPuts(MITLOG_LEVEL_ERROR, "get_wd_configure() failed");
        ret = -1;
        goto CLOSE_LOG_TAG;
    }
    print_wd_configure(wd_conf);
    /** save pid info */
	char pid_str[16] = {0};
    sprintf(pid_str, "%d", wd_conf->current_pid);
    if (write_file(WD_FILE_PATH_APP WD_FILE_NAME_PID, pid_str, strlen(pid_str)) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("write_file() %s failed", WD_FILE_PATH_APP WD_FILE_NAME_PID);
        ret = -1;
        goto CLOSE_LOG_TAG;
    }
    
    start_libevent_udp_server(wd_conf);
    
    free_wd_configure(wd_conf);    
CLOSE_LOG_TAG:
    MITLogClose();
    return ret;
}

