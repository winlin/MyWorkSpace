//
//  watchdog_main.c
//
//  Created by gtliu on 7/18/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "../common/mit_log_module.h"
#include "../common/mit_data_define.h"
#include "wd_configure.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, const char * argv[])
{
    /*
     * Convert self into a daemon
     * don't change process's working directory to '/'
     * don't redirect stdin/stdou/stderr
     */
    int ret = 0;
#ifndef MITLOG_DEBUG_ENABLE
    ret = daemon(1, 1);
    if(ret == -1) {
        perror("call daemon() failed!");
    }
#endif
    MITLogOpen(APP_NAME_WATCHDOG, LOG_PATH_WATCHD, _IOLBF);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "daemon ppid:%d pid:%d",  getppid(), getpid());

    struct wd_configure *wd_conf = get_wd_configure();
    if (wd_conf == NULL) {
        MITLog_DetPuts(MITLOG_LEVEL_ERROR, "get_wd_configure() failed");
        ret = -1;
        goto CLOSE_LOG_TAG;
    }
    print_wd_configure(wd_conf);

    /* save pid and version info */
    if(save_app_pid_ver_info(APP_NAME_WATCHDOG, wd_conf->current_pid, VERSION_WD) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_pid_ver_info() %s failed", APP_NAME_WATCHDOG);
        ret = -1;
        goto CLOSE_LOG_TAG;
    }

    start_libevent_udp_server(wd_conf);

    free_wd_configure(wd_conf);
CLOSE_LOG_TAG:
    MITLogClose();
    return ret;
}

