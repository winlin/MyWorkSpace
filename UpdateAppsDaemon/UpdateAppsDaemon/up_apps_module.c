//
//  up_apps_module.c
//  UpdateAppsDaemon
//
//  Created by gtliu on 8/2/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "include/mit_log_module.h"
#include "include/mit_data_define.h"
#include "up_apps_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>
#include <event2/event-config.h>

static struct up_app_info_node *list_head;

MITFuncRetValue update_c_app(struct up_app_info *app_info)
{
    MITLog_DetLogEnter
    int ret = 0;
    /** check the verson number */
    char ver_str[30] = {0};
    get_app_version(APP_NAME_UPAPPSD, ver_str);
    if (strlen(ver_str) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "get_app_version() failed");
        return MIT_RETV_FAIL;
    }
    //TODO: compare the verson info to decside whethe to update the app
    
    char path_one[MAX_AB_PATH_LEN] = {0};
    char path_two[MAX_AB_PATH_LEN] = {0};
    char cmd_str[MAX_AB_PATH_LEN*3]       = {0};
    /** create the update lock file */
    snprintf(path_one, MAX_AB_PATH_LEN, "%s%s/%s", APP_CONF_PATH, app_info->app_name, F_NAME_COMM_UPLOCK);
    snprintf(cmd_str, ,MAX_AB_PATH_LEN*3, "touch %s", path_one);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "create update lock file cmd:%s", cmd_str);
    
    if (system(cmd_str) == -1) {
        MITLog_DetErrPrintf("system():%s failed", cmd_str);
        return MIT_RETV_FAIL;
    }
    /** backup the app */
    memset(path_one, 0, MAX_AB_PATH_LEN);
    memset(cmd_str, 0, MAX_AB_PATH_LEN);
    snprintf(path_one, MAX_AB_PATH_LEN, "%s%s", app_info->app_path, app_info->app_name);
    snprintf(path_two, MAX_AB_PATH_LEN, "%s%s", path_one, APP_BACKUP_SUFFIX);
    snprintf(cmd_str, MAX_AB_PATH_LEN*3, "cp -f %s %s", path_one, path_two);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "backup the app cmd_str:%s", cmd_str);
    
    if (system(cmd_str) == -1) {
        MITLog_DetErrPrintf("system():%s failed", cmd_str);
        return MIT_RETV_FAIL;
    }
    /** kill the app */
    long long int pid = get_pid_with_comm(app_info->app_name);
    if (pid > 0) {
        if(kill((pid_t)pid, SIGKILL) < 0)
            MITLog_DetErrPrintf("kill() pid=%lld failed", pid);
    }
    /** replace the app */
    memset(cmd_str, 0, MAX_AB_PATH_LEN);
    snprintf(cmd_str, MAX_AB_PATH_LEN*3, "cp -f %s %s%s", app_info->new_app_path, app_info->app_path, app_info->app_name);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "replace the app cmd_str:%s", cmd_str);
    
    if (system(cmd_str) == -1) {
        MITLog_DetErrPrintf("system():%s failed", cmd_str);
        return MIT_RETV_FAIL;
    }
    /** remove the update lock file */
    snprintf(path_one, MAX_AB_PATH_LEN, "%s%s/%s", APP_CONF_PATH, app_info->app_name, F_NAME_COMM_UPLOCK);
    snprintf(cmd_str, ,MAX_AB_PATH_LEN*3, "rm -f %s", path_one);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "remove update lock file cmd:%s", cmd_str);
    
    if (system(cmd_str) == -1) {
        MITLog_DetErrPrintf("system():%s failed", cmd_str);
        return MIT_RETV_FAIL;
    }
    //TODO: start the new verson app
    // if want to start the app we must have the cmd line
    MITLog_DetLogExit
    return MIT_RETV_SUCCESS;
}

void timeout_cb(evutil_socket_t fd, short ev_type, void *data)
{
    MITLog_DetLogEnter
    waitpid(-1, NULL, WNOHANG);
    struct up_app_info_node *iter = list_head;
    while (iter) {
        switch (iter->app_info.app_type) {
            case UPAPP_TYPE_C:
                //TODO: realize the update C app
                break;
            case UPAPP_TYPE_KMODULE:
                //TODO: realize the update kernel module
                break;
            case UPAPP_TYPE_JAVA:
                //TODO: realize the update java app
                break;
            default:
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Unknown update app type:%d", iter->app_info.app_type);
                break;
        }
        //TODO: if success release the node
    }
    MITLog_DetLogExit
}

MITFuncRetValue start_app_update_func(struct up_app_info_node **head)
{
    MITLog_DetLogEnter
    *head = list_head;
    MITFuncRetValue func_ret = MIT_RETV_SUCCESS;
    struct event_base *ev_base = event_base_new();
    if (ev_base == NULL) {
        MITLog_DetErrPrintf("event_base_new() failed");
        func_ret = MIT_RETV_FAIL;
        goto FUNC_EXIT_TAG;
    }
    /** add timer event */
    struct event timeout;
    struct timeval tv;
    event_assign(&timeout, ev_base, -1, EV_PERSIST, timeout_cb, &timeout);
    evutil_timerclear(&tv);
    tv.tv_sec = UP_APP_DAEMON_TIME_INTERVAL;
    event_add(&timeout, &tv);
    
    MITLog_DetPuts(MITLOG_LEVEL_COMMON, "start the event dispatch");
    event_base_dispatch(ev_base);
    MITLog_DetPuts(MITLOG_LEVEL_COMMON, "end the event dispatch");
    
    event_base_free(ev_base);
FUNC_EXIT_TAG:
    MITLog_DetLogExit
    return func_ret;
}

void free_up_app_list(struct up_app_info_node *head)
{
    MITLog_DetLogEnter
    struct up_app_info_node *iter = head;
    struct up_app_info_node *tmp = NULL;
    while (iter != NULL) {
        free(iter->app_info.app_name);
        free(iter->app_info.app_path);
        free(iter->app_info.backup_app_path);
        free(iter->app_info.cur_version);
        free(iter->app_info.new_app_path);
        free(iter->app_info.new_version);
        tmp = iter->next_node;
        free(iter);
        iter = tmp;
    }
    MITLog_DetLogExit
}