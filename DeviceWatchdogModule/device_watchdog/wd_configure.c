//
//  wd_configure.c
//
//  Created by gtliu on 7/18/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "../include/mit_log_module.h"
#include "wd_configure.h"
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
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event-config.h>
#include <event2/util.h>
#include <fcntl.h>
#include <signal.h>

#define CONF_KNAME_MISSED_TIMES       "max_missed_feed_times"
#define CONF_KNAME_FEED_PERIOD        "default_feed_period"
#define CONF_KANME_PROCESSES          "default_process"
#define CONF_KNAME_WD_PID             "current_pid"
#define CONF_KANME_APPS_LIST          "monitored_apps_list"
#define CONF_KNAME_APPS_COUNT         "monitored_apps_count"

static struct wd_configure *wd_configure;
static struct sockaddr_in addr_self;
static struct event timeout;
static struct timeval tv;

struct wd_configure* get_wd_configure(void)
{
    struct wd_configure *wd_conf = calloc(1, sizeof(struct wd_configure));
    if (wd_conf == NULL) {
        MITLog_DetPuts(MITLOG_LEVEL_ERROR, "calloc() struct wd_configure failed");
        return NULL;
    }
    /** init the configure struct */
    wd_conf->current_pid                = getpid();
    wd_conf->default_feed_period        = DEFAULT_FEED_PERIOD;
    wd_conf->max_missed_feed_times      = DEFAULT_MAX_MISSED_FEED_TIMES;
    
    FILE *configue_fp = fopen(WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE, "r");
    if (configue_fp == NULL) {
        if (errno == ENOENT) {
            /** no such file or directory */
            /** keep the path exist */
            int ret = mkdir(WD_FILE_PATH_APP, S_IRWXU|S_IRWXG|S_IRWXO);
            if (ret == -1 && errno != EEXIST) {
                MITLog_DetErrPrintf("mkdir() failed");
                goto FREE_CONFIGURE_TAG;
            }
            /** write info into configure file */
            configue_fp = fopen(WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE, "w+");
            if (configue_fp == NULL) {
                MITLog_DetErrPrintf("fopen() %s failed", WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE);
                goto FREE_CONFIGURE_TAG;
            }
            ret = fprintf(configue_fp, "%s = %lu\n%s = %lu\n",
                    CONF_KNAME_MISSED_TIMES, wd_conf->max_missed_feed_times,
                    CONF_KNAME_FEED_PERIOD, wd_conf->default_feed_period);	
            if (ret < 0) {
                MITLog_DetErrPrintf("fprintf() %s failed", WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE);
                goto CLOSE_FILE_TAG;
            }
        } else {
            MITLog_DetErrPrintf("fopen() %s failed", WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE);
            goto FREE_CONFIGURE_TAG;
        }
    }
    /** load configure info from file */
    char *line      = NULL;
    size_t len      = 0;
    ssize_t read    = 0;
    while ((read = getline(&line, &len, configue_fp)) != -1) {
        /** strip the space char include:space, \f, \n, \r, \t, \v */
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Read line: %d : %s", read, line);
        read = strip_string_space(&line);
        /** ignore the empty line */
        if (read == 0) {
            continue;
        }
        /** ignore the comments */
        if (line[0] == '#') {
            continue;
        }
        /** get key and value */
        char *str, *tmpstr, *token;
        str = line;
        char *key_name = NULL;
        token = strtok_r(str, CONF_KEY_VALUE_DIVIDE_STR, &tmpstr);
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "key_name token:%s", token);
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
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "value_str token:%s", token);
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
            struct monitor_app_info_node *node = calloc(1, sizeof(struct monitor_app_info_node));
            if (node == NULL) {
                MITLog_DetErrPrintf("calloc() monitor_app_info_node failed");
                free(key_name);
                free(value_str);
                goto FREE_LINE_TAG;
            }
            // get app name
            str = value_str;
            token = strtok_r(str, APP_NAME_CMDLINE_DIVIDE_STR, &tmpstr);
            if (token) {
                node->app_info.app_name = strdup(token);
                if (node->app_info.app_name == NULL) {
                    MITLog_DetErrPrintf("strdup() app_info.app_name failed");
                    free(key_name);
                    free(value_str);
                    free(node);
                    goto FREE_LINE_TAG;
                }
                strip_string_space(&node->app_info.app_name);
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "init app name:%s", node->app_info.app_name);
            } else {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                                 "Confige file content error. Maybe lack a ';' between appname and cmdline");
                free(key_name);
                free(value_str);
                free(node);
                goto FREE_LINE_TAG;
            }
            // if the app is running get the pid and set the app_last_feed_time
            node->app_info.app_pid = (pid_t)get_pid_with_comm(node->app_info.app_name);
            if (node->app_info.app_pid > 0) {
                node->app_info.app_last_feed_time = time(NULL);
            }
            
            // get cmd line
            token = strtok_r(NULL, APP_NAME_CMDLINE_DIVIDE_STR, &tmpstr);
            if (token) {
                node->app_info.cmd_line = strdup(token);
                if (node->app_info.cmd_line == NULL) {
                    MITLog_DetErrPrintf("strdup() app_info.cmd_line failed");
                    free(key_name);
                    free(value_str);
                    free(node->app_info.app_name);
                    free(node);
                    goto FREE_LINE_TAG;
                }
                strip_string_space(&node->app_info.cmd_line);
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "init command line:%s", node->app_info.cmd_line);
            } else {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                                 "Confige file content error. Maybe lack a ';' between appname and cmdline");
                free(key_name);
                free(value_str);
                free(node->app_info.app_name);
                free(node);
                goto FREE_LINE_TAG;
            }
       
            node->app_info.app_period = wd_conf->default_feed_period;
            if (wd_conf->apps_list_head == NULL) {
                /** the first node */
                wd_conf->apps_list_head = node;
                wd_conf->apps_list_tail = node;
            } else {
                wd_conf->apps_list_tail->next_node = node;
                wd_conf->apps_list_tail = node;
            }
            wd_conf->monitored_apps_count++;
        }
    free(key_name);
    free(value_str);
    free(line);
    line = NULL;
    }
    fclose(configue_fp);
    return wd_conf;

FREE_LINE_TAG:
    free(line);
CLOSE_FILE_TAG:
    fclose(configue_fp);
FREE_CONFIGURE_TAG:
    free(wd_conf);
    return NULL;
}

MITFuncRetValue save_monitor_apps_info()
{
    /** write info into configure file */
    FILE *configue_fp = fopen(WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE, "w");
    if (configue_fp == NULL) {
        MITLog_DetErrPrintf("fopen() %s failed", WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE);
        return MIT_RETV_OPEN_FILE_FAIL;
    }
    int ret = fprintf(configue_fp, "%s = %lu\n%s = %lu\n",
                  CONF_KNAME_MISSED_TIMES, wd_configure->max_missed_feed_times,
                  CONF_KNAME_FEED_PERIOD, wd_configure->default_feed_period);
    if (ret < 0) {
        MITLog_DetErrPrintf("fprintf() %s failed", WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE);
        return MIT_RETV_FAIL;
    }
    /** write the monitored apps name and cmd line */
    struct monitor_app_info_node *tmp = wd_configure->apps_list_head;
    while (tmp) {
        ret = fprintf(configue_fp, "%s = %s;%s\n",
                CONF_KANME_PROCESSES,
                tmp->app_info.app_name,
                tmp->app_info.cmd_line);
        if (ret < 0) {
           MITLog_DetErrPrintf("fprintf() %s failed", WD_FILE_PATH_APP WD_FILE_NAME_CONFIGURE); 
        }
        tmp = tmp->next_node;
    }
    fclose(configue_fp);
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
    struct monitor_app_info_node *tmp = wd_conf->apps_list_head;
    while (tmp) {
        MITLogWrite(MITLOG_LEVEL_COMMON,
                    "AppName:%s CmdLine:%s",
                    tmp->app_info.app_name, 
                    tmp->app_info.cmd_line);
        tmp = tmp->next_node;
    }
    MITLog_DetLogExit
}

void free_wd_configure(struct wd_configure *wd_conf)
{
    struct monitor_app_info_node *iter_p = wd_conf->apps_list_head;
    while (iter_p) {
        struct monitor_app_info_node *tmp = iter_p;
        free(tmp->app_info.cmd_line);
        iter_p = tmp->next_node;
        free(tmp);
    }
    free(wd_conf);
}

MITFuncRetValue update_monitored_app_time(struct wd_pg_action *action_pg)
{
    if (action_pg == NULL) {
        return MIT_RETV_PARAM_EMPTY;
    }
    struct monitor_app_info_node *tmp = wd_configure->apps_list_head;
    while (tmp) {
        if (tmp->app_info.app_pid == action_pg->pid) {
            tmp->app_info.app_last_feed_time = time(NULL);
            return MIT_RETV_SUCCESS;
        }
        tmp = tmp->next_node;
    }
    return MIT_RETV_FAIL;
}

MITFuncRetValue unregister_monitored_app(struct wd_pg_action *action_pg)
{
    if (action_pg == NULL) {
        return MIT_RETV_PARAM_EMPTY;
    }
    struct monitor_app_info_node *tmp = wd_configure->apps_list_head;
    struct monitor_app_info_node *pre_p = NULL;
    while (tmp) {
        if (tmp->app_info.app_pid == action_pg->pid) {
            if (pre_p == NULL) {
                wd_configure->apps_list_head = tmp->next_node;
            } else {
                if (tmp == wd_configure->apps_list_tail) {
                    wd_configure->apps_list_tail = pre_p;
                }
                pre_p->next_node = tmp->next_node;
            }
            free(tmp->app_info.cmd_line);
            free(tmp->app_info.app_name);
            free(tmp);
            wd_configure->monitored_apps_count--;
            return MIT_RETV_SUCCESS;
        }
        pre_p = tmp;
        tmp = tmp->next_node;
    }
    if (save_monitor_apps_info() != MIT_RETV_SUCCESS) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "save_monitor_apps_info() failed");
    }
    return MIT_RETV_FAIL;
}

MITFuncRetValue add_monitored_app(struct wd_pg_register *reg_pg)
{
    if (reg_pg == NULL) {
        return MIT_RETV_PARAM_EMPTY;
    }
    int ret = MIT_RETV_SUCCESS;
    
    /** Check whether the app has been registered
     *  If it has existed just update the app_last_feed_time.
     */
    struct monitor_app_info_node *tmp = wd_configure->apps_list_head;
    while (tmp) {
        if (tmp->app_info.app_pid == reg_pg->pid ||
            strcmp(tmp->app_info.app_name, reg_pg->app_name) == 0) {
            tmp->app_info.app_last_feed_time = time(NULL);
            tmp->app_info.app_pid            = reg_pg->pid;
            tmp->app_info.app_period         = reg_pg->period <= 0 ? wd_configure->default_feed_period : reg_pg->period;
            if (strcmp(tmp->app_info.cmd_line, reg_pg->cmd_line) != 0) {
                free(tmp->app_info.cmd_line);
                tmp->app_info.cmd_line = calloc(reg_pg->cmd_len+1, sizeof(char));
                if (tmp->app_info.cmd_line == NULL) {
                    MITLog_DetErrPrintf("calloc() failed");
                    ret = MIT_RETV_ALLOC_MEM_FAIL;
                    goto ERR_RETURN_TAG;
                } else {
                    strncpy(tmp->app_info.cmd_line, reg_pg->cmd_line, reg_pg->cmd_len);
                }
            }
            return ret;
        }
        tmp = tmp->next_node;
    }
    
    struct monitor_app_info_node *node = calloc(1, sizeof(struct monitor_app_info_node));
    if (node == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        ret = MIT_RETV_ALLOC_MEM_FAIL;
        goto ERR_RETURN_TAG;
    }
    node->app_info.cmd_line = calloc(reg_pg->cmd_len + 1, sizeof(char));
    if (node->app_info.cmd_line == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        ret = MIT_RETV_ALLOC_MEM_FAIL;
        goto FREE_NODE_TAG;
    }
    strncpy(node->app_info.cmd_line, reg_pg->cmd_line, reg_pg->cmd_len);
    
    node->app_info.app_name = calloc(reg_pg->name_len + 1, sizeof(char));
    if (node->app_info.app_name == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        ret = MIT_RETV_ALLOC_MEM_FAIL;
        free(node->app_info.cmd_line);
        goto FREE_NODE_TAG;
    }
    strncpy(node->app_info.app_name, reg_pg->app_name, reg_pg->name_len);
    
    node->app_info.app_pid              = reg_pg->pid;
    node->app_info.app_period           = reg_pg->period <= 0 ? wd_configure->default_feed_period : reg_pg->period;
    node->app_info.app_last_feed_time   = time(NULL);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "init command line:%s", node->app_info.cmd_line);
    if (wd_configure->apps_list_head == NULL) {
        /** the first node */
        wd_configure->apps_list_head = node;
        wd_configure->apps_list_tail = node;
    } else {
        wd_configure->apps_list_tail->next_node = node;
        wd_configure->apps_list_tail = node;
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

MITFuncRetValue send_pg_back(evutil_socket_t fd, struct sockaddr_in *tar_addr, MITWatchdogPgCmd cmd, MITWatchdogPgError err_num)
{
    MITFuncRetValue ret = MIT_RETV_SUCCESS;
    int pg_len = 0;
    void *back_pg = wd_pg_return_new(&pg_len, cmd, err_num);
    if (back_pg) {
        ssize_t sent_len = sendto(fd, back_pg,
                                  pg_len, 0,
                                  (struct sockaddr *)tar_addr, sizeof(struct sockaddr_in));
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
    MITLog_DetPuts(MITLOG_LEVEL_COMMON, "There is a package recieved");
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
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Server CMD:%d", cmd);
            if (cmd == WD_PG_CMD_REGISTER) {
                struct wd_pg_register *reg_pg = wd_pg_register_unpg(msg, (int)len);
                if (reg_pg == NULL) {
                    MITLog_DetPuts(MITLOG_LEVEL_ERROR, "Recieve reigister package is empty");
                    /** send register error package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_REGISTER, WD_PG_ERR_REGISTER_FAIL);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                } else {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                     "\nPID:%d  PERIOD:%d\nAPPNAME:%s\nCMD:%s",
                                     reg_pg->pid, reg_pg->period,
                                     reg_pg->app_name, reg_pg->cmd_line);
                    /** add app info into struct wd_configure.apps_list_head */
                    MITFuncRetValue ret = 0;
                    if((ret=add_monitored_app(reg_pg)) != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "add_monitored_app() failed:%d", ret);
                    }
                    print_wd_configure(wd_configure);
                    
                    /** send register success package */
                    ret = send_pg_back(fd, &src_addr, WD_PG_CMD_REGISTER, WD_PG_ERR_SUCCESS);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                    free(reg_pg);
                }
            } else if (cmd == WD_PG_CMD_FEED) {
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Feed Cmd");
                struct wd_pg_action *feed_pg = wd_pg_action_unpg(msg, (int)len);
                /** send feed back package */
                if (feed_pg == NULL) {
                    MITLog_DetPuts(MITLOG_LEVEL_ERROR, "Recieve feed package is empty");
                    /** send feed error package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_FEED, WD_PG_ERR_FEED_FAIL);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                } else {
                    MITWatchdogPgError err_num = WD_PG_ERR_SUCCESS;
                    if (update_monitored_app_time(feed_pg) != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "update_monitored_app_time() \
                                         failed: pid=%d hasn't been register", feed_pg->pid);
                        err_num = WD_PG_ERR_FEED_FAIL;
                    } 
                    /** send feed success or fail package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_FEED, err_num);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                    free(feed_pg);
                }
            } else if (cmd == WD_PG_CMD_UNREGISTER) {
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Unregister Cmd");
                struct wd_pg_action *unreg_pg = wd_pg_action_unpg(msg, (int)len);
                /** send unregister back package */
                if (unreg_pg == NULL) {
                    MITLog_DetPuts(MITLOG_LEVEL_ERROR, "Recieve unregister package is empty");
                    /** send unregister error package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_UNREGISTER, WD_PG_ERR_UNREGISTER_FAIL);
                    if (ret != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send_pg_back() failed");
                    }
                } else {
                    MITWatchdogPgError err_num = WD_PG_ERR_SUCCESS;
                    if (unregister_monitored_app(unreg_pg) != MIT_RETV_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "unregister_monitored_app() \
                                         failed: pid=%d hasn't been register", unreg_pg->pid);
                        err_num = WD_PG_ERR_UNREGISTER_FAIL;
                    }
                    /** send unregister success or fail package */
                    MITFuncRetValue ret = send_pg_back(fd, &src_addr, WD_PG_CMD_UNREGISTER, err_num);
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

void start_the_monitor_app(struct monitor_app_info *app_info)
{
    /** at first kill that app */
    if (app_info->app_pid > 0) {
        int ret = kill(app_info->app_pid, SIGSTOP);
        if (ret < 0) {
            MITLog_DetErrPrintf("kill() process:%d failed", app_info->app_pid);
        }
    }
    char *cmd_line = strdup(app_info->cmd_line);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "strdup() cmd_line:%s", cmd_line);
    if (cmd_line == NULL) {
        MITLog_DetErrPrintf("strdup() failed");
        return;
    }
    int unit_alloc_size   = 4;
    int sum_alloc_size    = 0;
    char **cmd_argvs = (char **)calloc(unit_alloc_size, sizeof(char *));
    sum_alloc_size += unit_alloc_size;
    char **np        = NULL;
    if (cmd_argvs == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        free(cmd_line);
        return;
    }
    char *str, *save_ptr, *token;
    int j=0;
    for (str=cmd_line; ; ++j, str=NULL) {
        token = strtok_r(str, " ", &save_ptr);  // cmd and param divide by " "
        if (j >= sum_alloc_size) {
            np = (char **)calloc(sum_alloc_size+unit_alloc_size, sizeof(char *));
            if (np == NULL) {
                MITLog_DetErrPrintf("calloc() failed");
                free(cmd_line);
                free(cmd_argvs);
                return;
            }
            memcpy(np, cmd_argvs, sum_alloc_size*sizeof(char *));
            sum_alloc_size += unit_alloc_size;
            free(cmd_argvs);
            cmd_argvs = np;
        }
        cmd_argvs[j] = token;
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "j:%d token:%s", j, cmd_argvs[j]);
        if (token == NULL) {
            break;
        }
    }
    int pid = vfork();
    if (pid == 0) {
        int ret = execvp(cmd_argvs[0], cmd_argvs);
        if (ret < 0) {
            MITLog_DetErrPrintf("execvp() failed");
        }
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        app_info->app_pid = pid;
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                         "child process:%d will execvp(%s)",
                         pid,
                         app_info->cmd_line);
    } else {
        MITLog_DetErrPrintf("vfork() failed");
    }
    free(cmd_argvs);
    free(cmd_line);
}

void timeout_cb(evutil_socket_t fd, short ev_type, void* data)
{
    waitpid(-1, NULL, WNOHANG);
    /** check every app and decide whether special app should be restarted */
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Current Monitored App Count:%d", wd_configure->monitored_apps_count);
    struct monitor_app_info_node *tmp = wd_configure->apps_list_head;
    for (; tmp; tmp=tmp->next_node) {
        time_t now_time         = time(NULL);
        time_t app_final_time   = (tmp->app_info.app_last_feed_time +
                                   wd_configure->max_missed_feed_times *
                                   tmp->app_info.app_period);
        
        if (now_time > app_final_time) {
            MITLog_DetPrintf(MITLOG_LEVEL_WARNING,
                             "app:%d need to be restarted\n cmdline:%s",
                             tmp->app_info.app_pid,
                             tmp->app_info.cmd_line);
            /** update the last feed time to avoid doubly starting the app */
            tmp->app_info.app_last_feed_time = now_time;
            /** check whether the target is updating */
            char *update_lock_file = calloc(
                                            strlen(WD_FILE_PATH_APP) +
                                            strlen(tmp->app_info.app_name) +
                                            strlen(APP_UPDATE_FILE_PREFIX) + 1,
                                            sizeof(char));
            if (update_lock_file == NULL) {
                MITLog_DetErrPrintf("calloc() falied");
                continue;
            }
            sprintf(update_lock_file, "%s%s%s", WD_FILE_PATH_APP, APP_UPDATE_FILE_PREFIX, tmp->app_info.app_name);
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "update_lock_file:%s", update_lock_file);
            int exist_flag = 0;
            if ((exist_flag = access(update_lock_file, F_OK)) < 0) {
                if (errno != ENOENT) {
                    MITLog_DetErrPrintf("access() %s failed", update_lock_file);
                    continue;
                }
            } else if (exist_flag == 0) {
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s is updating", tmp->app_info.app_name);
                continue;
            }
            /** start the app again */
            start_the_monitor_app(&tmp->app_info);
        }
    }
}

MITFuncRetValue start_libevent_udp_server(struct wd_configure *wd_conf)
{
    MITLog_DetLogEnter
    if (wd_conf == NULL) {
        return MIT_RETV_PARAM_EMPTY;
    }
    wd_configure = wd_conf;

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        MITLog_DetErrPrintf("socket() failed");
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
    /** any local network card is ok */
    addr_self.sin_addr.s_addr   = INADDR_ANY;
    
    if (bind(socket_fd, (struct sockaddr*)&addr_self, sizeof(addr_self)) < 0) {
        MITLog_DetErrPrintf("bind() failed");
        goto CLOSE_FD_TAG;
    }
    socklen_t addr_len = sizeof(addr_self);
    if (getsockname(socket_fd, (struct sockaddr *)&addr_self, &addr_len) < 0) {
        MITLog_DetErrPrintf("getsockname() failed");
        goto CLOSE_FD_TAG;
    }
    /** save port info */
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Port:%d", ntohs(addr_self.sin_port));
    char port_str[16] = {0};
    sprintf(port_str, "%d", ntohs(addr_self.sin_port));
    if (write_file(WD_FILE_PATH_APP WD_FILE_NAME_PORT, port_str, strlen(port_str)) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("write_file() %s failed", WD_FILE_PATH_APP WD_FILE_NAME_PID);
        goto CLOSE_FD_TAG;
    }
    
    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        MITLog_DetErrPrintf("event_base_new() failed");
        goto CLOSE_FD_TAG;
    }
    
    /** Add UDP server event */
    struct event socket_ev_r;
    event_assign(&socket_ev_r, ev_base, socket_fd, EV_READ|EV_PERSIST, socket_ev_r_cb, &socket_ev_r);
    if (event_add(&socket_ev_r, NULL) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "couldn't add an event");
        goto EVENT_BASE_FREE_TAG;
    }
    /** Add timer event */
    event_assign(&timeout, ev_base, -1, EV_PERSIST, timeout_cb, &timeout);
    evutil_timerclear(&tv);
    tv.tv_sec = 1;
    event_add(&timeout, &tv);
    
    event_base_dispatch(ev_base);
    
    MITLog_DetLogExit
    
EVENT_BASE_FREE_TAG:
    event_base_free(ev_base);
CLOSE_FD_TAG:
    close(socket_fd);
FUNC_EXIT_TAG:
    return MIT_RETV_FAIL;
}


































