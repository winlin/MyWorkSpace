//
//  app_main.c
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "create_feed_thread.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

/* the app name */
#define MONITOR_APP_NAME               "app1"
/* app's version number */
#define VERSION_MONITOR_APP            "v1.0.3"

int main(int argc, const char * argv[])
{
    MITLogOpen(MONITOR_APP_NAME, LOG_FILE_PATH MONITOR_APP_NAME, _IOLBF);
    int ret = 0;
    char dir[1024];
    char *cwd_char = getcwd(dir, sizeof(dir));
    if (cwd_char == NULL) {
        MITLog_DetErrPrintf("getcwd() failed");
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s", dir);

    struct feed_thread_configure th_conf;
    th_conf.cmd_line = "/data/apps/"MONITOR_APP_NAME;
    th_conf.app_name = MONITOR_APP_NAME;
    th_conf.feed_period = 3;
    th_conf.monitored_pid = getpid();

    /* save pid and version info */
    if(save_app_pid_ver_info(MONITOR_APP_NAME, th_conf.monitored_pid, VERSION_MONITOR_APP) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_pid_ver_info() %s failed", MONITOR_APP_NAME);
        ret = -1;
        goto CLOSE_LOG_TAG;
    }

    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Start the feed thread:cmd=%s", th_conf.cmd_line);
    create_feed_thread(&th_conf);

    while (1) {
        MITLog_DetPuts(MITLOG_LEVEL_COMMON, "The main thread rotate one time per 3 seconds");
        sleep(3);
    }

    MITFuncRetValue ret_t = unregister_watchdog();
    if (ret_t != MIT_RETV_SUCCESS) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Unregister app failed! Watchdog will restart the app later");
        ret = -1;
    }

CLOSE_LOG_TAG:
    MITLogClose();
    return ret;
}

