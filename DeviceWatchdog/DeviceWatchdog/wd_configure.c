//
//  wd_configure.c
//  DeviceWatchdog
//
//  Created by gtliu on 7/18/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "wd_configure.h"
#include "MITLogModule.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
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

#define CONF_KNAME_UDP_PORT           "default_udp_port"
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
    wd_conf->default_udp_port           = DEFAULT_UDP_PORT;
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
            ret = fprintf(configue_fp, "%s = %lu\n%s = %lu\n%s = %lu\n%s = %s\n",
                    CONF_KNAME_UDP_PORT, wd_conf->default_udp_port,
                    CONF_KNAME_MISSED_TIMES, wd_conf->max_missed_feed_times,
                    CONF_KNAME_FEED_PERIOD, wd_conf->default_feed_period,
                    CONF_KANME_PROCESSES, " ");	
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
        /** ignore the empty line */
        if (read == 1) {
            continue;
        }
        /** ignore the comments */
        int comments_flag = 0;
        for (int i=0; i<read; ++i) {
            if (line[i] == '#') {
                comments_flag = 1;
                break;
            }
        }
        if (comments_flag == 1) {
            continue;
        }
        /** get key and value */
        char *key_name      = calloc(read, sizeof(char));
        if (key_name == NULL) {
            MITLog_DetErrPrintf("calloc() failed");
            goto CLOSE_FILE_TAG;
        }
        char *value_str     = calloc(read, sizeof(char));
        if (value_str == NULL) {
            free(key_name);
            MITLog_DetErrPrintf("calloc() failed");
            goto CLOSE_FILE_TAG;
        }
        sscanf(line, "%s = %s", key_name, value_str);
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "key:%s  value:%s", key_name, value_str);
        if (strcmp(CONF_KNAME_UDP_PORT, key_name) == 0) {
            wd_conf->default_udp_port           = strtoul(value_str, NULL, 10);
        } else if (strcmp(CONF_KNAME_MISSED_TIMES, key_name) == 0) {
            wd_conf->max_missed_feed_times      = strtoul(value_str, NULL, 10);
        } else if (strcmp(CONF_KNAME_FEED_PERIOD, key_name) == 0) {
            wd_conf->default_feed_period        = strtoul(value_str, NULL, 10);
        } else if (strcmp(CONF_KANME_PROCESSES, key_name) == 0) {
            for (int i=0; i<read; ++i) {
                if (line[i] == '[') {
                    struct monitor_app_info_node *node = calloc(1, sizeof(struct monitor_app_info_node));
                    if (node == NULL) {
                        MITLog_DetErrPrintf("calloc() failed");
                        goto CLOSE_FILE_TAG;
                    }
                    int j = i+1;
                    for (; j<read; ++j) {
                        if (line[j] == ']') {
                            break;
                        }
                    }
                    if (j >= read) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "configure process file lack a ']':%s", line);
                        goto CLOSE_FILE_TAG;
                    }
                    node->app_info.cmd_line = calloc(j-i, sizeof(char));
                    if (node->app_info.cmd_line == NULL) {
                        MITLog_DetErrPrintf("calloc() failed");
                        goto CLOSE_FILE_TAG;
                    }
                    strncpy(node->app_info.cmd_line, line+i+1, j-i-1);
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "init command line:%s", node->app_info.cmd_line);
                    if (wd_conf->apps_list_head == NULL) {
                        /** the first node */
                        wd_conf->apps_list_head = node;
                        wd_conf->apps_list_tail = node;
                    } else {
                        wd_conf->apps_list_tail->next_node = node;
                        wd_conf->apps_list_tail = node;
                    }
                    wd_conf->monitored_apps_count++;
                    break;
                }
            }
        }
        free(key_name);
        free(value_str);
    }
    free(line);
    line  = NULL;
    fclose(configue_fp);
    return wd_conf;

CLOSE_FILE_TAG:
    fclose(configue_fp);
FREE_CONFIGURE_TAG:
    free(wd_conf);
    return NULL;
}

void print_wd_configure(struct wd_configure *wd_conf)
{
    MITLog_DetLogEnter
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                     "\n%-30s=%lu\n%-30s=%lu\n%-30s=%lu\n%-30s=%d\n%-30s=%u",
                     CONF_KNAME_UDP_PORT, wd_conf->default_udp_port,
                     CONF_KNAME_MISSED_TIMES, wd_conf->max_missed_feed_times,
                     CONF_KNAME_FEED_PERIOD, wd_conf->default_feed_period,
                     CONF_KNAME_WD_PID, wd_conf->current_pid,
                     CONF_KNAME_APPS_COUNT, wd_conf->monitored_apps_count);
    
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s:", CONF_KANME_APPS_LIST);
    struct monitor_app_info_node *tmp = wd_conf->apps_list_head;
    while (tmp) {
        MITLogWrite(MITLOG_LEVEL_COMMON, tmp->app_info.cmd_line);
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

MITFuncRetValue update_monitored_app_time(struct wd_pg_feed *feed_pg)
{
    if (feed_pg == NULL) {
        return MIT_RETV_PARAM_EMPTY;
    }
    struct monitor_app_info_node *tmp = wd_configure->apps_list_head;
    while (tmp) {
        if (tmp->app_info.app_pid == feed_pg->pid) {
            tmp->app_info.app_last_feed_time = time(NULL);
            return MIT_RETV_SUCCESS;
        }
        tmp = tmp->next_node;
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
        if (strcmp(tmp->app_info.cmd_line, reg_pg->cmd_line) == 0
            || tmp->app_info.app_pid == reg_pg->pid) {
            tmp->app_info.app_last_feed_time = time(NULL);
            tmp->app_info.app_pid            = reg_pg->pid;
            tmp->app_info.app_period         = reg_pg->period <= 0 ? wd_configure->default_feed_period : reg_pg->period;
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
                                     "\nPID:%d  PERIOD:%d\nCMD:%s\n",
                                     reg_pg->pid, reg_pg->period,
                                     reg_pg->cmd_line);
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
                struct wd_pg_feed *feed_pg = wd_pg_feed_unpg(msg, (int)len);
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
            } else {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Unknown cmd:%d\n", cmd);
            }
        }
    }
}

void timeout_cb(evutil_socket_t fd, short ev_type, void* data)
{
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "timeout_cb() call once");
    waitpid(-1, NULL, WNOHANG);
    // TODO: check every app and decide whether special app should be restarted
    
    struct monitor_app_info_node *tmp = wd_configure->apps_list_head;
    while (tmp) {
        time_t now_time         = time(NULL);
        time_t app_final_time   = tmp->app_info.app_last_feed_time \
                                    + wd_configure->max_missed_feed_times \
                                    * tmp->app_info.app_period;
        if (now_time >= app_final_time ) {
            MITLog_DetPrintf(MITLOG_LEVEL_WARNING,
                             "app:%d need to be restarted\n cmdline:%s",
                             tmp->app_info.app_pid,
                             tmp->app_info.cmd_line);
            // TODO: start the app again
            int pid = vfork();
            if (pid == 0) {
                MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                                 "child process:%d will execv(%s)",
                                 getpid(),
                                 tmp->app_info.cmd_line);
                int ret = execv(tmp->app_info.cmd_line, NULL);
                if (ret < 0) {
                    MITLog_DetErrPrintf("execv() failed");
                }
            } else if (pid < 0) {
                MITLog_DetErrPrintf("vfork() failed");
            }
        }
        tmp = tmp->next_node;
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
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, (void *)&set_value, sizeof(set_value)) < 0) {
        MITLog_DetErrPrintf("setsockopt() failed");
    }
    if (fcntl(socket_fd, F_SETFD, FD_CLOEXEC) < 0) {
        MITLog_DetErrPrintf("fcntl() failed");
    }
    
    memset(&addr_self, 0, sizeof(addr_self));
    addr_self.sin_family        = AF_INET;
    addr_self.sin_port          = htons(wd_conf->default_udp_port);
    /** any local network card is ok */
    addr_self.sin_addr.s_addr   = htonl(INADDR_ANY);
    
    if (bind(socket_fd, (struct sockaddr*)&addr_self, sizeof(addr_self)) < 0) {
        MITLog_DetErrPrintf("bind() failed");
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

































