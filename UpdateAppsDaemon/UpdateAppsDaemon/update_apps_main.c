//
//  update_apps_main.c
//  UpdateAppsDaemon
//
//  Created by gtliu on 8/2/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "include/mit_log_module.h"
#include "include/mit_data_define.h"
#include "up_apps_module.h"
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
    MITLogOpen("UpdateAppsDaemon", LOG_FILE_PATH "up_apps_daemon");
    
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "daemon ppid:%d pid:%d",  getppid(), getpid());
    
    char dir[1024];
    char *cwd_char = getcwd(dir, sizeof(dir));
    if (cwd_char == NULL) {
        MITLog_DetErrPrintf("getcwd() failed");
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s", dir);
    
    /** save pid info */
	char pid_str[16] = {0};
    sprintf(pid_str, "%d", getpid());
    if (write_file(UP_CON_FILE_PATH FILE_NAME_PID, pid_str, strlen(pid_str)) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("write_file() %s failed", UP_CON_FILE_PATH FILE_NAME_PID);
        ret = -1;
        goto CLOSE_LOG_TAG;
    }
    
    start_libevent_udp_server(wd_conf);
    
    free_wd_configure(wd_conf);
CLOSE_LOG_TAG:
    MITLogClose();
    return ret;

}

