//
//  app_main.c
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "create_feed_thread.h"
#include <sys/types.h>
#include <unistd.h>

#define APP_NUMBER    "11"

int main(int argc, const char * argv[])
{
    MITLogOpen("UDPClinet", "/tmp/logs/app"APP_NUMBER);
    
    char dir[1024];
    char *cwd_char = getcwd(dir, sizeof(dir));
    if (cwd_char == NULL) {
        MITLog_DetErrPrintf("getcwd() failed");
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s", dir);
    
    struct feed_thread_configure th_conf;
    th_conf.cmd_line = "/tmp/apps/app"APP_NUMBER;
    th_conf.app_name = "app"APP_NUMBER;
    th_conf.feed_period = 3;
    th_conf.monitored_pid = getpid();
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Start the feed thread:cmd=%s", th_conf.cmd_line);
    create_feed_thread(&th_conf);
    int i = 0;
    while (1) {
        MITLog_DetPuts(MITLOG_LEVEL_COMMON, "The main thread rotate one time per 3 seconds");
        sleep(3);
    }
    
    MITFuncRetValue ret = unregister_watchdog();
    if (ret != MIT_RETV_SUCCESS) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Unregister app failed! Watchdog will restart the app later");
    }
    
    MITLogClose();
    return 0;
}

