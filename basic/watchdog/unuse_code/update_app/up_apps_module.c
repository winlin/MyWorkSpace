//
//  up_apps_module.c
//  UpdateAppsDaemon
//
//  Created by gtliu on 8/2/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "../include/mit_log_module.h"
#include "../include/mit_data_define.h"
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

static struct up_app_info *app_list_head;
static struct event_base *ev_base;
static int    restart_flag;

MITFuncRetValue update_java_app(struct up_app_info *app_info)
{
    MITFuncRetValue f_ret = MIT_RETV_SUCCESS;
    /* check the verson number */
    char ver_str[30] = {0};
    get_app_version(app_info->app_name, ver_str);
    if (strlen(ver_str) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "get_app_version() failed");
    }

    //TODO: compare the verson info to decside whethe to update the app

    /* create the update lock file */
    if(create_update_lock_file(app_info->app_name) !=0 ) {
        MITLog_DetErrPrintf("create_update_lock_file(%s) failed", app_info->app_name);
        f_ret = MIT_RETV_FAIL;
        goto FUNC_RET_TAG;
    }
    /* backup the app */
    int japp_name_len = strlen(app_info->app_name) + strlen(JAVA_APP_SUFFIX) + 1;
    char *japp_name   = calloc(japp_name_len, sizeof(char));
    if(japp_name == NULL) {
        MITLog_DetErrPrintf("calloc() japp_name failed");
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }
    snprintf(japp_name, japp_name_len, "%s%s", app_info->app_name, JAVA_APP_SUFFIX);
    if(backup_application(japp_name) != 0) {
        MITLog_DetErrPrintf("backup_application(%s) failed", japp_name);
    }
    /* replace the app */
    if(replace_the_application(japp_name, app_info->new_app_path) != 0) {
        MITLog_DetErrPrintf("replace_the_application():%s failed", app_info->app_name);
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }
    free(japp_name);

    /* kill the app */
    long long int pid = get_pid_with_comm(app_info->app_name);
    if (pid > 0 && kill((pid_t)pid, SIGKILL) < 0) {
        MITLog_DetErrPrintf("kill() pid=%lld failed", pid);
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }
    /* update the java app */
    char upjava_cmd[MAX_AB_PATH_LEN*2] = {0};
    snprintf(upjava_cmd, MAX_AB_PATH_LEN*2, "pm install -r %s", app_info->new_app_path);
    if(posix_system(upjava_cmd) !=0) {
        MITLog_DetErrPrintf("%s failed", upjava_cmd);
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }

    //TODO: start the new verson app
    // if want to start the app we must have the cmd line
REMOVE_LOCK_FILE_TAG:
    /* remove the update lock file */
    if(remove_update_lock_file(app_info->app_name) != 0) {
        MITLog_DetErrPrintf("remove_update_lock_file():%s failed", app_info->app_name);
        f_ret = MIT_RETV_FAIL;
    }
FUNC_RET_TAG:
    return f_ret;
}

MITFuncRetValue update_kmodule_app(struct up_app_info *app_info)
{
MITFuncRetValue f_ret = MIT_RETV_SUCCESS;
    /* check the verson number */
    char ver_str[30] = {0};
    get_app_version(app_info->app_name, ver_str);
    if (strlen(ver_str) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "get_app_version() failed");
    }

    //TODO: compare the verson info to decside whethe to update the app

    /* create the update lock file */
    if(create_update_lock_file(app_info->app_name) !=0 ) {
        MITLog_DetErrPrintf("create_update_lock_file(%s) failed", app_info->app_name);
        f_ret = MIT_RETV_FAIL;
        goto FUNC_RET_TAG;
    }
    /* backup the kernel module */
    int kmod_name_len = strlen(app_info->app_name) + strlen(KMODULE_LIB_SUFFIX) + 1;
    char *kmod_name   = calloc(kmod_name_len, sizeof(char));
    if(kmod_name == NULL) {
        MITLog_DetErrPrintf("calloc() kmod_name failed");
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }
    snprintf(kmod_name, kmod_name_len, "%s%s", app_info->app_name, KMODULE_LIB_SUFFIX);
    if(backup_application(kmod_name) != 0) {
        MITLog_DetErrPrintf("backup_application(%s) failed", kmod_name);
    }
    /* replace the old kernel module */
    if(replace_the_application(kmod_name, app_info->new_app_path) != 0) {
        MITLog_DetErrPrintf("replace_the_application():%s failed", kmod_name);
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }
    free(kmod_name);

    /* remount the /system */
    if(posix_system("mount -o rw,remount /system") != 0) {
        MITLog_DetErrPrintf("remount /system rw failed");
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }
    /* copy the new version kernel module into special path */
    char cp_cmd[MAX_AB_PATH_LEN*2] = {0};
    snprintf(cp_cmd, MAX_AB_PATH_LEN*2, "cp -f %s /system/lib/modules/", app_info->new_app_path);
    if(posix_system(cp_cmd) != 0) {
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }
    /* remount the /system back*/
    if(posix_system("mount -o ro,remount /system") != 0) {
        MITLog_DetErrPrintf("remount /system rw failed");
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }

    /* save the new version of kernel module */
    if(save_app_pid_ver_info(app_info->app_name, 0, app_info->new_version) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_pid_ver_info(%s) failed", app_info->app_name);
    }

    //TODO: start the new verson app
    // if want to start the app we must have the cmd line

REMOVE_LOCK_FILE_TAG:
    /* remove the update lock file */
    if(remove_update_lock_file(app_info->app_name) != 0) {
        MITLog_DetErrPrintf("remove_update_lock_file():%s failed", app_info->app_name);
        f_ret = MIT_RETV_FAIL;
    }
FUNC_RET_TAG:
    return f_ret;
}

MITFuncRetValue update_c_app(struct up_app_info *app_info)
{
    MITFuncRetValue f_ret = MIT_RETV_SUCCESS;
    /* check the verson number */
    char ver_str[30] = {0};
    get_app_version(app_info->app_name, ver_str);
    if (strlen(ver_str) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "get_app_version() failed");
    }

    //TODO: compare the verson info to decside whethe to update the app

    /* create the update lock file */
    if(create_update_lock_file(app_info->app_name) !=0 ) {
        MITLog_DetErrPrintf("create_update_lock_file(%s) failed", app_info->app_name);
        f_ret = MIT_RETV_FAIL;
        goto FUNC_RET_TAG;
    }
    /* backup the app */
    if(backup_application(app_info->app_name) != 0) {
        MITLog_DetErrPrintf("backup_application(%s) failed", app_info->app_name);
    }
    /* kill the app */
    long long int pid = get_pid_with_comm(app_info->app_name);
    if (pid > 0 && kill((pid_t)pid, SIGKILL) < 0) {
        MITLog_DetErrPrintf("kill() pid=%lld failed", pid);
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }
    /* replace the app */
    if(replace_the_application(app_info->app_name, app_info->new_app_path) != 0) {
        MITLog_DetErrPrintf("replace_the_application():%s failed", app_info->app_name);
        f_ret = MIT_RETV_FAIL;
        goto REMOVE_LOCK_FILE_TAG;
    }

    //TODO: start the new verson app
    // if want to start the app we must have the cmd line

REMOVE_LOCK_FILE_TAG:
    /* remove the update lock file */
    if(remove_update_lock_file(app_info->app_name) != 0) {
        MITLog_DetErrPrintf("remove_update_lock_file():%s failed", app_info->app_name);
        f_ret = MIT_RETV_FAIL;
    }
FUNC_RET_TAG:
    return f_ret;
}

void timeout_cb(evutil_socket_t fd, short ev_type, void *data)
{
    MITLog_DetLogEnter
    waitpid(-1, NULL, WNOHANG);
    struct up_app_info *iter = app_list_head;
    struct up_app_info *pre_iter = NULL;
    while (iter) {
        MITFuncRetValue f_ret = MIT_RETV_FAIL;
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "APP UPDATE: TYPE=%d APP_NAME:%s", iter->app_type, iter->app_name);
        //check the APP_CONF_PATH/app_name directory exist if not create
        char conf_path[MAX_AB_PATH_LEN] = {0};
        snprintf(conf_path, MAX_AB_PATH_LEN, "%s%s", APP_CONF_PATH, iter->app_name);
        if(create_directory(conf_path) != 0) {
            MITLog_DetErrPrintf("create_directory(%s) failed", conf_path);
            continue;
        }
        switch (iter->app_type) {
            case UPAPP_TYPE_C:
                if((f_ret = update_c_app(iter)) != MIT_RETV_SUCCESS) {
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "update_c_app() failed");
                }
                break;
            case UPAPP_TYPE_KMODULE:
                if((f_ret = update_kmodule_app(iter)) != MIT_RETV_SUCCESS) {
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "update_kmodule_app() failed");
                }
                break;
            case UPAPP_TYPE_JAVA:
                if((f_ret = update_java_app(iter)) != MIT_RETV_SUCCESS) {
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "update_java_app() failed");
                }
                break;
            default:
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Unknown update app type:%d", iter->app_type);
                break;
        }
        if (f_ret == MIT_RETV_SUCCESS) {
            struct up_app_info *tmp = iter;
            iter = iter->next_node;
            if (tmp == app_list_head) {
                app_list_head = pre_iter = iter;
                if(app_list_head == NULL) {
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "set the restart flag update need reboot");
                    restart_flag = 1;
                }
            } else {
                pre_iter->next_node = iter;
            }
            free(tmp->app_name);
            free(tmp->new_app_path);
            free(tmp->new_version);
            free(tmp);
        } else {
           pre_iter = iter;
           iter = iter->next_node;
        }
    }
    if(restart_flag > 0) {
        posix_system("reboot");
    }
    MITLog_DetLogExit
}

MITFuncRetValue start_app_update_func(struct up_app_info **head)
{
    MITLog_DetLogEnter
    *head = app_list_head;
    restart_flag = 0;

    /* create a test update app */
    app_list_head = calloc(1, sizeof(struct up_app_info));
    if (app_list_head == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        return MIT_RETV_FAIL;
    }
    app_list_head->app_type = UPAPP_TYPE_C;
    app_list_head->app_name = strdup("app1");
    app_list_head->new_app_path = strdup(APP_STORE_FILE_PATH"app1");
    app_list_head->new_version = strdup("v1.0.3");
    app_list_head->next_node = NULL;
    struct up_app_info *sec_node = calloc(1, sizeof(struct up_app_info));
    if (sec_node == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
    } else {
        sec_node->app_type = UPAPP_TYPE_C;
        sec_node->app_name = strdup("app1");
        sec_node->new_app_path = strdup(APP_STORE_FILE_PATH"app1");
        sec_node->new_version = strdup("v1.0.3");
        sec_node->next_node = NULL;
    }
    app_list_head->next_node = sec_node;

    struct up_app_info *thrid_node = calloc(1, sizeof(struct up_app_info));
    if (thrid_node == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
    } else {
        thrid_node->app_type = UPAPP_TYPE_JAVA;
        thrid_node->app_name = strdup("com.example.posjnitest");
        thrid_node->new_app_path = strdup(APP_STORE_FILE_PATH"com.example.posjnitest.apk");
        thrid_node->new_version = strdup("v1.0.8");
        thrid_node->next_node = NULL;
    }
    sec_node->next_node = thrid_node;

    struct up_app_info *four_node = calloc(1, sizeof(struct up_app_info));
    if (four_node == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
    } else {
        four_node->app_type = UPAPP_TYPE_KMODULE;
        four_node->app_name = strdup("bcm4329");
        four_node->new_app_path = strdup(APP_STORE_FILE_PATH"bcm4329.ko");
        four_node->new_version = strdup("v1.0.8");
        four_node->next_node = NULL;
    }
    thrid_node->next_node = four_node;

    MITFuncRetValue func_ret = MIT_RETV_SUCCESS;
    ev_base = event_base_new();
    if (ev_base == NULL) {
        MITLog_DetErrPrintf("event_base_new() failed");
        func_ret = MIT_RETV_FAIL;
        goto FUNC_EXIT_TAG;
    }
    /* add timer event */
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

void free_up_app_list(struct up_app_info *head)
{
    MITLog_DetLogEnter
    struct up_app_info *iter = head;
    struct up_app_info *tmp = NULL;
    while (iter) {
        tmp = iter;
        iter = iter->next_node;
        free(tmp->app_name);
        free(tmp->new_app_path);
        free(tmp->new_version);
        free(tmp);
    }
    MITLog_DetLogExit
}
