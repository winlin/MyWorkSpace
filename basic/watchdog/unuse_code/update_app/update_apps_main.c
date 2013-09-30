//
//  update_apps_main.c
//  UpdateAppsDaemon
//
//  Created by gtliu on 8/2/13.
//  Copyright (c) 2013 GT. All rights reserved.
//
#include "../include/mit_log_module.h"
#include "up_apps_module.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define APP_NAME_UPDATED    "up_apps_daemon"

int main(int argc, const char * argv[])
{
    /*
     * 1. convert self into a daemon
     *    don't change process's working directory to '/'
     *    don't redirect stdin/stdou/stderr
     */
    int ret = 0;
#ifndef MITLOG_DEBUG_ENABLE
	ret = daemon(1, 1);
	if(ret == -1) {
		perror("call daemon() failed!");
	}
#endif
    MITLogOpen(APP_NAME_UPDATED, LOG_FILE_PATH APP_NAME_UPDATED, _IOLBF);

    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "daemon ppid:%d pid:%d",  getppid(), getpid());

    char dir[1024];
    char *cwd_char = getcwd(dir, sizeof(dir));
    if (cwd_char == NULL) {
        MITLog_DetErrPrintf("getcwd() failed");
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s", dir);

    /* save pid and version info */
    char tmp_str[16] = {0};
    sprintf(tmp_str, "%d", getpid());
    if(save_app_pid_ver_info(APP_NAME_UPAPPSD, getpid(), VERSION_UPAPPSD) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_pid_ver_info() %s failed", APP_NAME_UPAPPSD);
        ret = -1;
        goto CLOSE_LOG_TAG;
    }
    struct up_app_info *head = NULL;

    start_app_update_func(&head);

    free_up_app_list(head);
CLOSE_LOG_TAG:
    MITLogClose();
    return ret;

}

