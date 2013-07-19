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

#define CONF_KNAME_UDP_PORT           "default_udp_port"
#define CONF_KNAME_MISSED_TIMES       "max_missed_feed_times"
#define CONF_KNAME_FEED_PERIOD        "default_feed_period"
#define CONF_KANME_PROCESSES          "default_process"
#define CONF_KNAME_WD_PID             "current_pid"
#define CONF_KANME_APPS_LIST          "monitored_apps_list"
#define CONF_KNAME_APPS_COUNT         "monitored_apps_count"

struct wd_configure* get_wd_configure(void)
{
    struct wd_configure *wd_conf = calloc(1, sizeof(struct wd_configure));
    if (wd_conf == NULL) {
        MITLog_DetPuts(MITLOG_LEVEL_ERROR, "calloc() struct wd_configure failed");
        return NULL;
    }
    /** init the configure struct */
    wd_conf->apps_list_head             = NULL;
    wd_conf->current_pid                = getpid();
    wd_conf->default_feed_period        = DEFAULT_FEED_PERIOD;
    wd_conf->default_udp_port           = DEFAULT_UDP_PORT;
    wd_conf->max_missed_feed_times      = DEFAULT_MAX_MISSED_FEED_TIMES;
    wd_conf->monitored_apps_count       = 0;
    
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
                        node->privious_node = NULL;
                    } else {
                        struct monitor_app_info_node *tmp = wd_conf->apps_list_head;
                        while (tmp->next_node != NULL) {
                            tmp = tmp->next_node;
                        }
                        tmp->next_node = node;
                    }
                    wd_conf->monitored_apps_count++;
                    break;
                }
            }
        }
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
}
