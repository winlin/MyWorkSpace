//
//  wd_configure.c
//
//  Created by gtliu on 7/18/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

// for using local libevent include and static lib
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event-config.h>
#include <event2/util.h>

// for local include
#include "wd_configure.h"
#include "../common/mit_log_module.h"

#define CONF_KNAME_MISSED_TIMES       "max_missed_feed_times"
#define CONF_KNAME_FEED_PERIOD        "default_feed_period"
#define CONF_KANME_PROCESSES          "default_process"
#define CONF_KNAME_WD_PID             "current_pid"
#define CONF_KANME_APPS_LIST          "monitored_apps_list"
#define CONF_KNAME_APPS_COUNT         "monitored_apps_count"

static struct wd_configure *wd_configure;
static struct sockaddr_in addr_self;

struct wd_configure* get_wd_configure(void)
{
    long long int system_max_pid  = get_sys_max_pid();

    struct wd_configure *wd_conf = calloc(1, sizeof(struct wd_configure));
    if (wd_conf == NULL) {
        MITLog_DetPuts(MITLOG_LEVEL_ERROR, "calloc() struct wd_configure failed");
        return NULL;
    }
    /* init the configure struct */
    wd_conf->current_pid                = getpid();
    wd_conf->default_feed_period        = DEFAULT_FEED_PERIOD;
    wd_conf->max_missed_feed_times      = DEFAULT_MAX_MISSED_FEED_TIMES;

    FILE *conf_fp = fopen(CONF_PATH_WATCHD F_NAME_COMM_CONF, "r");
    if (conf_fp == NULL) {
        if (errno == ENOENT) {
            /* no such file or directory */
            /* keep the path exist */
            if(create_directory(CONF_PATH_WATCHD) != 0) {
                MITLog_DetErrPrintf("create_directory(%s) failed", CONF_PATH_WATCHD);
                goto FREE_CONFIGURE_TAG;
            }
            /* write info into configure file */
            conf_fp = fopen(CONF_PATH_WATCHD F_NAME_COMM_CONF, "w+");
            if (conf_fp == NULL) {
                MITLog_DetErrPrintf("fopen() %s failed", CONF_PATH_WATCHD F_NAME_COMM_CONF);
                goto FREE_CONFIGURE_TAG;
            }
            int ret = fprintf(conf_fp, "%s = %lu\n%s = %lu\n",
                    CONF_KNAME_MISSED_TIMES, wd_conf->max_missed_feed_times,
                    CONF_KNAME_FEED_PERIOD, wd_conf->default_feed_period);
            if (ret < 0) {
                MITLog_DetErrPrintf("fprintf() %s failed", CONF_PATH_WATCHD F_NAME_COMM_CONF);
                goto CLOSE_FILE_TAG;
            }
        } else {
            MITLog_DetErrPrintf("fopen() %s failed", CONF_PATH_WATCHD F_NAME_COMM_CONF);
            goto FREE_CONFIGURE_TAG;
        }
    }
    /* load configure info from file */
    char *line    = calloc(WD_CONFIG_FILE_LINE_MAX_LEN, sizeof(char));
    if (line == NULL) {
        MITLog_DetErrPrintf("calloc() %s failed");
        goto FREE_CONFIGURE_TAG;
    }
    ssize_t read  = 0;
    while ((read = fscanf(conf_fp, "%[^\n]", line)) != EOF) {
        /* eat the \n char */
        fgetc(conf_fp);
        /* strip the space char include:space, \f, \n, \r, \t, \v */
        read = strip_string_space(&line);
        /* ignore the empty line */
        if (read == 0) {
            memset(line, 0, WD_CONFIG_FILE_LINE_MAX_LEN);
            continue;
        }
        /* ignore the comments */
        if (line[0] == '#') {
            memset(line, 0, WD_CONFIG_FILE_LINE_MAX_LEN);
            continue;
        }
        /* get key and value */
        char *str, *tmpstr, *token;
        str = line;
        char *key_name = NULL;
        token = strtok_r(str, CONF_KEY_VALUE_DIVIDE_STR, &tmpstr);
        if (token) {
            key_name = strdup(token);
            if (key_name == NULL) {
                MITLog_DetErrPrintf("strdup() keyname failed");
                goto FREE_LINE_TAG;
            }
        } else {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                             "Confige file content error. Maybe lack a '=' between key and value");
            goto FREE_LINE_TAG;
        }

        char *value_str = NULL;
        token = strtok_r(NULL, CONF_KEY_VALUE_DIVIDE_STR, &tmpstr);
        if (token) {
            value_str = strdup(token);
            if (value_str == NULL) {
                MITLog_DetErrPrintf("strdup() value_str failed");
                free(key_name);
                goto FREE_LINE_TAG;
            }
        } else {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                             "Confige file content error. Maybe lack a '=' between key and value");
            free(key_name);
            goto FREE_LINE_TAG;
        }

        strip_string_space(&key_name);
        strip_string_space(&value_str);
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "key:%s  value:%s", key_name, value_str);
        if (strcmp(CONF_KNAME_MISSED_TIMES, key_name) == 0) {
            wd_conf->max_missed_feed_times      = strtoul(value_str, NULL, 10);
        } else if (strcmp(CONF_KNAME_FEED_PERIOD, key_name) == 0) {
            wd_conf->default_feed_period        = strtoul(value_str, NULL, 10);
        } else if (strcmp(CONF_KANME_PROCESSES, key_name) == 0 && strlen(value_str) > 0) {
            struct monitor_app_info *node = calloc(1, sizeof(struct monitor_app_info));
            if (node == NULL) {
                MITLog_DetErrPrintf("calloc() monitor_app_info failed");
                free(key_name);
                free(value_str);
                goto FREE_LINE_TAG;
            }
            // get app name
            str = value_str;
            token = strtok_r(str, APP_NAME_CMDLINE_DIVIDE_STR, &tmpstr);
            if (token) {
                node->app_name = strdup(token);
                if (node->app_name == NULL) {
                    MITLog_DetErrPrintf("strdup() app_name failed");
                    free(key_name);
                    free(value_str);
                    free(node);
                    goto FREE_LINE_TAG;
                }
                strip_string_space(&node->app_name);
            } else {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                                 "Confige file content error. Maybe lack a ';' between appname and cmdline");
                free(key_name);
                free(value_str);
                free(node);
                goto FREE_LINE_TAG;
            }
            // if the app is running get the pid and set the app_last_feed_time
            node->app_pid = (pid_t)get_pid_with_comm(node->app_name, system_max_pid);
            if (node->app_pid > 0) {
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                 "get the app's pid:%d and update the app_last_feed_time",
                                 node->app_pid);
                // set the wd_init_time
                node->wd_init_time = time(NULL);
            }
            // get cmd line
            token = strtok_r(NULL, APP_NAME_CMDLINE_DIVIDE_STR, &tmpstr);
            if (token) {
                node->cmd_line = strdup(token);
                if (node->cmd_line == NULL) {
                    MITLog_DetErrPrintf("strdup() cmd_line failed");
                    free(key_name);
                    free(value_str);
                    free(node->app_name);
                    free(node);
                    goto FREE_LINE_TAG;
                }
                strip_string_space(&node->cmd_line);
            } else {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                                 "Confige file content error. Maybe lack a ';' between appname and cmdline");
                free(key_name);
                free(value_str);
                free(node->app_name);
                free(node);
                goto FREE_LINE_TAG;
            }
            if (wd_conf->app_list_head == NULL) {
                /* the first node */
                wd_conf->app_list_head = wd_conf->app_list_tail = node;
            } else {
                wd_conf->app_list_tail->next_node = node;
                wd_conf->app_list_tail = node;
            }
            wd_conf->monitored_apps_count++;
        }
        free(key_name);
        free(value_str);
        memset(line, 0, WD_CONFIG_FILE_LINE_MAX_LEN);
    }
    fclose(conf_fp);
    free(line);
    return wd_conf;

FREE_LINE_TAG:
    free(line);
CLOSE_FILE_TAG:
    fclose(conf_fp);
FREE_CONFIGURE_TAG:
    free(wd_conf);
    return NULL;
}

MITFuncRetValue save_monitor_apps_info()
{
    /* write info into configure file */
    FILE *conf_fp = fopen(CONF_PATH_WATCHD F_NAME_COMM_CONF, "w");
    if (conf_fp == NULL) {
        MITLog_DetErrPrintf("fopen() %s failed", CONF_PATH_WATCHD F_NAME_COMM_CONF);
        return MIT_RETV_OPEN_FILE_FAIL;
    }
    int ret = fprintf(conf_fp, "%s = %lu\n%s = %lu\n",
                  CONF_KNAME_MISSED_TIMES, wd_configure->max_missed_feed_times,
                  CONF_KNAME_FEED_PERIOD, wd_configure->default_feed_period);
    if (ret < 0) {
        MITLog_DetErrPrintf("fprintf() %s failed", CONF_PATH_WATCHD F_NAME_COMM_CONF);
        return MIT_RETV_FAIL;
    }
    /* write the monitored apps name and cmd line */
    struct monitor_app_info *tmp = wd_configure->app_list_head;
    while (tmp) {
        ret = fprintf(conf_fp, "%s = %s;%s\n",
                CONF_KANME_PROCESSES,
                tmp->app_name,
                tmp->cmd_line);
        if (ret < 0) {
           MITLog_DetErrPrintf("fprintf() %s failed", CONF_PATH_WATCHD F_NAME_COMM_CONF);
        }
        tmp = tmp->next_node;
    }
    fclose(conf_fp);
    return MIT_RETV_SUCCESS;
}

void print_wd_configure(struct wd_configure *wd_conf)
{
    MITLog_DetLogEnter
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                     "\n%-30s=%lu\n%-30s=%lu\n%-30s=%d\n%-30s=%u",
                     CONF_KNAME_MISSED_TIMES, wd_conf->max_missed_feed_times,
                     CONF_KNAME_FEED_PERIOD, wd_conf->default_feed_period,
                     CONF_KNAME_WD_PID, wd_conf->current_pid,
                     CONF_KNAME_APPS_COUNT, wd_conf->monitored_apps_count);

    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s:", CONF_KANME_APPS_LIST);
    struct monitor_app_info *tmp = wd_conf->app_list_head;
    while (tmp) {
        MITLogWrite(MITLOG_LEVEL_COMMON,
                    "AppName:%s CmdLine:%s",
                    tmp->app_name,
                    tmp->cmd_line);
        tmp = tmp->next_node;
    }
    MITLog_DetLogExit
}

void free_wd_configure(struct wd_configure *wd_conf)
{
    struct monitor_app_info *app_iter = wd_conf->app_list_head;
    while (app_iter) {
        struct monitor_app_info *t_iter = app_iter;
        app_iter = t_iter->next_node;

        struct monitor_thread_info *thread_iter = t_iter->thread_list_head;
        while(thread_iter) {
            struct monitor_thread_info *tt_iter = thread_iter;
            thread_iter = tt_iter->next_node;
            free(tt_iter);
        }
        free(t_iter->cmd_line);
        free(t_iter->app_name);
        free(t_iter);
    }
    free(wd_conf);
}

MITFuncRetValue update_monitored_app_time(struct wd_pg_action *action_pg)
{
    if (action_pg == NULL) {
        return MIT_RETV_PARAM_ERROR;
    }
    struct monitor_app_info *app_iter = wd_configure->app_list_head;
    while (app_iter) {
        if (app_iter->app_pid == action_pg->pid) {
            struct monitor_thread_info *th_iter = app_iter->thread_list_head;
            while(th_iter) {
                if(th_iter->thread_id == action_pg->thread_id) {
                    /*
                     MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                     "pid:thread_id(%d:%d) find the last feed time will update",
                                     action_pg->pid,
                                     action_pg->thread_id);
                     */
                    th_iter->last_feed_time = time(NULL);
                    th_iter->thread_period  = action_pg->period;
                    return MIT_RETV_SUCCESS;
                }
                th_iter = th_iter->next_node;
            }
        }
        app_iter = app_iter->next_node;
    }
    return MIT_RETV_FAIL;
}

void unregister_monitored_app(struct wd_pg_action *action_pg)
{
    if (action_pg == NULL) {
        return;
    }
    unsigned int old_monitored_apps_count = wd_configure->monitored_apps_count;
    struct monitor_app_info *app_iter = wd_configure->app_list_head;
    struct monitor_app_info *pre_app_iter = NULL;
    while (app_iter) {
        if(app_iter->app_pid == action_pg->pid) {
            struct monitor_thread_info *th_iter  = app_iter->thread_list_head;
            struct monitor_thread_info *pre_th_iter = NULL;
            while(th_iter) {
                if(th_iter->thread_id == action_pg->thread_id) {
                    /* free the thread node */
                    if(app_iter->monitored_threads_count == 1) {
                        /*
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                         "pid=%d the thread:%d is the app's only one thread.",
                                         app_iter->app_pid,
                                         th_iter->thread_id);
                         */
                        if(wd_configure->monitored_apps_count == 1) {
                            /*
                            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                         "the pid:%d is the only one monitored app",
                                         app_iter->app_pid);
                             */
                            wd_configure->app_list_head = wd_configure->app_list_tail = NULL;
                        } else {
                            /*
                            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                         "the pid:%d is not only one monitored app",
                                         app_iter->app_pid);
                             */
                            if(pre_app_iter == NULL) {
                                wd_configure->app_list_head = app_iter->next_node;
                            } else {
                                pre_app_iter->next_node = app_iter->next_node;
                            }
                        }
                        /*
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                         "pid=%d the thread:%d is only thread and be free",
                                         app_iter->app_pid,
                                         th_iter->thread_id);
                         */
                        free(app_iter->app_name);
                        free(app_iter->cmd_line);
                        free(app_iter);
                        wd_configure->monitored_apps_count--;
                    } else {
                        if(pre_th_iter == NULL) {
                            /*
                            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                         "pid=%d the thread:%d is the first thread with many thread",
                                         app_iter->app_pid,
                                         th_iter->thread_id);
                             */
                            app_iter->thread_list_head = th_iter->next_node;
                        } else {
                            pre_th_iter->next_node = th_iter->next_node;
                        }
                        app_iter->monitored_threads_count--;
                    }
                    free(th_iter);
                    goto SAVE_APP_INFO_TAG;
                }
                pre_th_iter = th_iter;
                th_iter     = th_iter->next_node;
            }
        }
        pre_app_iter = app_iter;
        app_iter     = app_iter->next_node;
    }
    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "app(%d:%d) doesn't has be monitor. "
                     "WE ASLO CONSIDER THIS UNREGISTER SUCCEED",
                     action_pg->pid, action_pg->thread_id);
    
SAVE_APP_INFO_TAG:
    if (wd_configure->monitored_apps_count == 1) {
        wd_configure->app_list_tail = wd_configure->app_list_head;
    }
    if (old_monitored_apps_count != wd_configure->monitored_apps_count) {
        /* monitored app unregister, the configure file should be modified gtliu 2013-09-25 */
        if (save_monitor_apps_info() != MIT_RETV_SUCCESS) {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "save_monitor_apps_info() failed");
        }
    }
}

MITWatchdogPgError add_monitored_app(struct wd_pg_register *reg_pg)
{
    if (reg_pg == NULL) {
        return WD_PG_ERR_REGISTER_FAIL;
    }
    int ret = WD_PG_ERR_SUCCESS;
    /* Check whether the app has been registered
     * If it has existed just update the app_last_feed_time.
     */
    struct monitor_app_info *app_iter = wd_configure->app_list_head;
    while (app_iter) {
        //if (app_iter->app_pid == reg_pg->pid ||
        //    strcmp(app_iter->app_name, reg_pg->app_name) == 0) {
        /* only use the app's name to justfy the equal 2013-09-25 gtliu */
        int compare_ret = strcmp(app_iter->app_name, reg_pg->app_name);
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                         "app_iter->app_name:%s, reg_pg->app_name:%s compare_ret:%d",
                         app_iter->app_name, reg_pg->app_name,
                         compare_ret);
        if (compare_ret == 0) {
            /* update the app info */
            app_iter->app_pid = reg_pg->pid;
            struct monitor_thread_info *th_iter = app_iter->thread_list_head;
            while(th_iter) {
                if(th_iter->thread_id == reg_pg->thread_id) {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                     "find the pid:thread_id(%d:%d) so update last feed time",
                                     app_iter->app_pid,
                                     th_iter->thread_id);
                    th_iter->last_feed_time = time(NULL);
                    goto ERR_RETURN_TAG;
                }
                th_iter = th_iter->next_node;
            }
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                     "cannot find the thread id(%d:%d) so will add monitor thread",
                                     reg_pg->pid,
                                     reg_pg->thread_id);
            struct monitor_thread_info *new_th_node = calloc(1, sizeof(struct monitor_thread_info));
            if(new_th_node == NULL) {
                MITLog_DetErrPrintf("calloc() failed");
                ret = WD_PG_ERR_REGISTER_FAIL;
                goto ERR_RETURN_TAG;
            }
            new_th_node->last_feed_time = time(NULL);
            new_th_node->thread_period  = reg_pg->period <= 0 ? wd_configure->default_feed_period : reg_pg->period;
            new_th_node->thread_id      = reg_pg->thread_id;
            if(app_iter->thread_list_head == NULL) {
                app_iter->thread_list_head = app_iter->thread_list_tail = new_th_node;
            } else {
                app_iter->thread_list_tail->next_node = new_th_node;
                app_iter->thread_list_tail = new_th_node;
            }
            app_iter->monitored_threads_count++;
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s pid=%d monitored threads count=%u",
                             app_iter->app_name,
                             app_iter->app_pid,
                             app_iter->monitored_threads_count);
            goto ERR_RETURN_TAG;
        }
        app_iter = app_iter->next_node;
    }
    /* create a new node */
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                     "%s pid:thread_id(%d:%d) not exist so a new node will created",
                     reg_pg->app_name,
                     reg_pg->pid,
                     reg_pg->thread_id);

    struct monitor_app_info *node = calloc(1, sizeof(struct monitor_app_info));
    if (node == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        ret = WD_PG_ERR_REGISTER_FAIL;
        goto ERR_RETURN_TAG;
    }
    node->cmd_line = calloc(reg_pg->cmd_len + 1, sizeof(char));
    if (node->cmd_line == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        ret = WD_PG_ERR_REGISTER_FAIL;
        goto FREE_NODE_TAG;
    }
    strncpy(node->cmd_line, reg_pg->cmd_line, reg_pg->cmd_len);

    node->app_name = calloc(reg_pg->name_len + 1, sizeof(char));
    if (node->app_name == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        ret = WD_PG_ERR_REGISTER_FAIL;
        free(node->cmd_line);
        goto FREE_NODE_TAG;
    }
    strncpy(node->app_name, reg_pg->app_name, reg_pg->name_len);
    node->app_pid = reg_pg->pid;

    /* create a thread node */
    struct monitor_thread_info *new_th_node = calloc(1, sizeof(struct monitor_thread_info));
    if(new_th_node == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        ret = WD_PG_ERR_REGISTER_FAIL;
        goto ERR_RETURN_TAG;
    }
    new_th_node->last_feed_time = time(NULL);
    new_th_node->thread_period  = reg_pg->period <= 0 ? wd_configure->default_feed_period : reg_pg->period;
    new_th_node->thread_id      = reg_pg->thread_id;
    node->thread_list_head = node->thread_list_tail = new_th_node;
    new_th_node->next_node = NULL;

    node->monitored_threads_count++;
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s pid=%d monitored threads count=%u",
                    node->app_name,
                    node->app_pid,
                    node->monitored_threads_count);
    /* add to app list */
    if (wd_configure->app_list_head == NULL) {
        /* the first node */
        wd_configure->app_list_head = wd_configure->app_list_tail = node;
    } else {
        wd_configure->app_list_tail->next_node = node;
        wd_configure->app_list_tail = node;
    }
    wd_configure->monitored_apps_count++;

    if (save_monitor_apps_info() != MIT_RETV_SUCCESS) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "save_monitor_apps_info() failed");
    }
    return ret;

FREE_NODE_TAG:
    free(node);
ERR_RETURN_TAG:
    return ret;
}

MITFuncRetValue send_pg_back(evutil_socket_t fd,
                             struct sockaddr_in *tar_addr,
                             MITWatchdogPgCmd cmd,
                             MITWatchdogPgError err_num)
{
    MITFuncRetValue ret = MIT_RETV_SUCCESS;
    int pg_len = 0;
    void *back_pg = wd_pg_return_new(&pg_len, cmd, err_num);
    if (back_pg) {
        ssize_t sent_len = sendto(fd, back_pg,
                                  pg_len,
                                  0,
                                  (struct sockaddr *)tar_addr,
                                  sizeof(struct sockaddr_in));
        if (sent_len < 0) {
            MITLog_DetErrPrintf("sendto() failed");
            ret = MIT_RETV_FAIL;
        }
        free(back_pg);
    }
    return ret;
}

void socket_ev_r_cb(evutil_socket_t fd, short ev_type, void *data)
{
    if (ev_type & EV_READ) {
        char msg[MAX_UDP_PG_SIZE] = {0};
        struct sockaddr_in src_addr;
        socklen_t addrlen = sizeof(src_addr);
        ssize_t len = recvfrom(fd, msg, sizeof(msg)-1, 0, (struct sockaddr *)&src_addr, &addrlen);
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                         "addrlen:%d    src_port:%d",
                         addrlen, ntohs(src_addr.sin_port));
        if (len > 0) {
            short cmd = wd_get_net_package_cmd(msg);
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Client CMD:%d", cmd);
            if (cmd == WD_PG_CMD_REGISTER) {
                struct wd_pg_register *reg_pg = wd_pg_register_unpg(msg, (int)len);
                if (reg_pg == NULL) {
                    MITLog_DetPuts(MITLOG_LEVEL_ERROR, "Recieve reigister package is empty");
                    /* send register error package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_REGISTER, WD_PG_ERR_REGISTER_FAIL);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                } else {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                     "\nGet Client Register Package:\nPID:%d\nPERIOD:%d\nTHREAD_ID:%d\nAPPNAME:%s\nCMD:%s",
                                     reg_pg->pid, reg_pg->period,
                                     reg_pg->thread_id,
                                     reg_pg->app_name, reg_pg->cmd_line);
                    /* add app info into struct wd_configure.app_list_head */
                    MITWatchdogPgError ret = WD_PG_ERR_SUCCESS;
                    if((ret=add_monitored_app(reg_pg)) != WD_PG_ERR_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "add_monitored_app() failed:%d", ret);
                    } else {
                        print_wd_configure(wd_configure);
                    }
                    /* send register success package */
                    MITFuncRetValue r_ret = send_pg_back(fd, &src_addr, WD_PG_CMD_REGISTER, ret);
                    if (r_ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                    free(reg_pg);
                }
            } else if (cmd == WD_PG_CMD_FEED) {
                struct wd_pg_action *feed_pg = wd_pg_action_unpg(msg, (int)len);
                /* send feed back package */
                if (feed_pg == NULL) {
                    MITLog_DetPuts(MITLOG_LEVEL_ERROR, "Recieve feed package is empty");
                    /* send feed error package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_FEED, WD_PG_ERR_FEED_FAIL);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                } else {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Feed Cmd:%d PID:%d", feed_pg->cmd, feed_pg->pid);
                    MITWatchdogPgError err_num = WD_PG_ERR_SUCCESS;
                    if (update_monitored_app_time(feed_pg) != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "update_monitored_app_time() \
                                         failed: pid=%d hasn't been register", feed_pg->pid);
                        err_num = WD_PG_ERR_FEED_FAIL;
                    }
                    /* send feed success or fail package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_FEED, err_num);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                    free(feed_pg);
                }
            } else if (cmd == WD_PG_CMD_UNREGISTER) {
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Unregister Cmd");
                struct wd_pg_action *unreg_pg = wd_pg_action_unpg(msg, (int)len);
                /* send unregister back package */
                if (unreg_pg == NULL) {
                    MITLog_DetPuts(MITLOG_LEVEL_ERROR, "Recieve unregister package is empty");
                    /* send unregister error package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_UNREGISTER, WD_PG_ERR_UNREGISTER_FAIL);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                } else {
                    unregister_monitored_app(unreg_pg);
                    /* send unregister success or fail package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_UNREGISTER, WD_PG_ERR_SUCCESS);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                    free(unreg_pg);
                }
            } else {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Unknown cmd:%d\n", cmd);
            }
        }
    }
}

MITFuncRetValue remove_all_thread(struct monitor_app_info *app_info)
{
    if (app_info->monitored_threads_count > 0) {
        /* remove all the thread nodes */
        struct monitor_thread_info *thread_iter = app_info->thread_list_head;
        while (thread_iter) {
            struct monitor_thread_info *tmp = thread_iter;
            thread_iter=thread_iter->next_node;
            free(tmp);
            app_info->monitored_threads_count--;
        }
        if (app_info->monitored_threads_count != 0) {
            MITLog_DetErrPrintf("monitored_threads_count should be 0,but=%d", app_info->monitored_threads_count);
            return MIT_RETV_FAIL;
        }
        app_info->thread_list_head = app_info->thread_list_tail = NULL;
    }
    return MIT_RETV_SUCCESS;
}

void start_the_monitor_app(struct monitor_app_info *app_info)
{
    MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                         "process:%d will be killed and execvp(%s)",
                         app_info->app_pid,
                         app_info->cmd_line);
    /* at first kill that app */
    if (app_info->app_pid > 0) {
        char app_comm[MAX_F_NAME_LEN] = {0};
        /*
         * If the app_pid still belongs to the monitored app
         * then kill() will be called.
         */
        get_comm_with_pid(app_info->app_pid, app_comm);
        if (check_substring(app_info->app_name, app_comm) == 0) {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                             "%s with pid=%d will be killed",
                             app_info->app_name,
                             app_info->app_pid);
            int ret = kill(app_info->app_pid, SIGKILL);
            if (ret < 0) {
                MITLog_DetErrPrintf("kill() process:%d failed", app_info->app_pid);
            }
        } else {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                             "%s with pid=%d will be exec() without kill() as for the pid does not belong to the app",
                             app_info->app_name,
                             app_info->app_pid);
        }
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                             "%s with pid=%d will be exec() without kill() as for the pid <=0",
                             app_info->app_name,
                             app_info->app_pid);
    }
    /* remove all the thread nodes */
    if (remove_all_thread(app_info) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("remove_all_thread() failed");
        return;
    }
    /* at last restart the app */
    app_info->app_pid = start_app_with_cmd_line(app_info->cmd_line);
    if (app_info->app_pid == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "start_app_with_cmd_line() failed:%s", app_info->cmd_line);
    }
}

void timeout_cb(evutil_socket_t fd, short ev_type, void* data)
{
    waitpid(-1, NULL, WNOHANG);
    /* check every app and decide whether special app should be restarted */
    /*
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                     "Current Monitored App Count:%d",
                     wd_configure->monitored_apps_count);
     */
    struct monitor_app_info *app_iter = wd_configure->app_list_head;
    for (; app_iter; app_iter=app_iter->next_node) {
        time_t now_time           = time(NULL);
        if(app_iter->app_pid == 0) {
            /* this condition for read app info from configure file */
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                             "%s init from configure file so start the app",
                             app_iter->app_name);
            /* start the app again */
            /* set the wd_init_time means the last start this app time */
            app_iter->wd_init_time = now_time;
            start_the_monitor_app(app_iter);
            return;
        } else if(app_iter->app_pid>0 && app_iter->monitored_threads_count == 0){
            /* this condition for when restart the app all the thread nodes will be deleted
             * so if after a period there is no register/feed, the app will be restarted again
             */
            time_t app_final_time = app_iter->wd_init_time +
                                    wd_configure->max_missed_feed_times *
                                    wd_configure->default_feed_period;
            if(now_time > app_final_time) {
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                 "%s has no thread and long time no feed so start the app",
                                 app_iter->app_name);
                app_iter->wd_init_time = now_time;
                /* start the app again */
                start_the_monitor_app(app_iter);
                return;
            }
        }
        struct monitor_thread_info *thread_iter = app_iter->thread_list_head;
        for(; thread_iter; thread_iter=thread_iter->next_node) {
            time_t thread_final_time  = (thread_iter->last_feed_time +
                                        wd_configure->max_missed_feed_times *
                                        thread_iter->thread_period);
            if(now_time > thread_final_time) {
                MITLog_DetPrintf(MITLOG_LEVEL_WARNING,
                             "now:%d thread last feed time:%d\n"
                             "restart %s(old pid:%d) cmdline:%s",
                             now_time,
                             thread_iter->last_feed_time,
                             app_iter->app_name,
                             app_iter->app_pid,
                             app_iter->cmd_line);
                /* update the last feed time to avoid doubly starting the app */
                thread_iter->last_feed_time = now_time;
                /* check whether the target is updating */
                if (check_update_lock_file(app_iter->app_name) == 0) {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s update_lock_file exist", app_iter->app_name);
                    /* for this app, the check should be stoped */
                    break;
                } else {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s update_lock_file not exist", app_iter->app_name);
                    /* start the app again */
                    app_iter->wd_init_time = now_time;
                    start_the_monitor_app(app_iter);
                    /* for this app, the check should be stoped */
                    break;
                }
            }
        }
    }
}

MITFuncRetValue start_libevent_udp_server(struct wd_configure *wd_conf)
{
    MITLog_DetLogEnter

    MITFuncRetValue func_ret = MIT_RETV_SUCCESS;
    if (wd_conf == NULL) {
        return MIT_RETV_PARAM_ERROR;
    }
    wd_configure    = wd_conf;

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        MITLog_DetErrPrintf("socket() failed");
        func_ret = MIT_RETV_OPEN_FILE_FAIL;
        goto FUNC_EXIT_TAG;
    }

    int set_value = 1;
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&set_value, sizeof(set_value)) < 0) {
        MITLog_DetErrPrintf("setsockopt() failed");
    }
    if (fcntl(socket_fd, F_SETFD, FD_CLOEXEC) < 0) {
        MITLog_DetErrPrintf("fcntl() failed");
    }

    memset(&addr_self, 0, sizeof(addr_self));
    addr_self.sin_family        = AF_INET;
    addr_self.sin_port          = 0;
    /* any local network card is ok */
    addr_self.sin_addr.s_addr   = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr*)&addr_self, sizeof(addr_self)) < 0) {
        MITLog_DetErrPrintf("bind() failed");
        func_ret = MIT_RETV_FAIL;
        goto CLOSE_FD_TAG;
    }
    socklen_t addr_len = sizeof(addr_self);
    if (getsockname(socket_fd, (struct sockaddr *)&addr_self, &addr_len) < 0) {
        MITLog_DetErrPrintf("getsockname() failed");
        func_ret = MIT_RETV_FAIL;
        goto CLOSE_FD_TAG;
    }
    /* save port info */
    char port_str[16] = {0};
    sprintf(port_str, "%d", ntohs(addr_self.sin_port));
    if(save_app_conf_info(APP_NAME_WATCHDOG, F_NAME_COMM_PORT, port_str) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_conf_info() %s/%s failed", APP_NAME_WATCHDOG, F_NAME_COMM_PORT);
        func_ret = MIT_RETV_FAIL;
        goto CLOSE_FD_TAG;
    }
    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        MITLog_DetErrPrintf("event_base_new() failed");
        func_ret = MIT_RETV_FAIL;
        goto CLOSE_FD_TAG;
    }

    /* Add UDP server event */
    struct event socket_ev_r;
    event_assign(&socket_ev_r, ev_base, socket_fd, EV_READ|EV_PERSIST, socket_ev_r_cb, &socket_ev_r);
    if (event_add(&socket_ev_r, NULL) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "couldn't add an event");
        func_ret = MIT_RETV_FAIL;
        goto EVENT_BASE_FREE_TAG;
    }
    /* Add timer event */
    struct event timeout;
    struct timeval tv;
    event_assign(&timeout, ev_base, -1, EV_PERSIST, timeout_cb, &timeout);
    evutil_timerclear(&tv);
    tv.tv_sec = WD_CHECK_TIME_INTERVAL;
    event_add(&timeout, &tv);

    event_base_dispatch(ev_base);

EVENT_BASE_FREE_TAG:
    event_base_free(ev_base);
CLOSE_FD_TAG:
    close(socket_fd);
FUNC_EXIT_TAG:
    MITLog_DetLogExit
    return func_ret;
}


































