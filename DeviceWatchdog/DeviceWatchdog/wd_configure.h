//
//  wd_configure.h
//  DeviceWatchdog
//
//  Created by gtliu on 7/18/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef DeviceWatchdog_wd_configure_h
#define DeviceWatchdog_wd_configure_h

#include "mit_data_define.h"
#include <time.h>
#include <unistd.h>

#define WD_DEBUG                     1

#ifdef  WD_DEBUG
#define WD_FILE_PATH_APP             "./"
#define WD_FILE_PATH_LOG             "./logs/"
#else
#define WD_FILE_PATH_APP             "/sdcard/watchdog/"
#define WD_FILE_PATH_LOG             "/sdcard/log/"
#endif

#define WD_FILE_NAME_CONFIGURE       "watchdog.cfg"
#define WD_FILE_NAME_PORT            "port"
#define WD_FILE_NAME_PID             "pid"

#define    DEFAULT_FEED_PERIOD                  15  
#define    DEFAULT_UDP_PORT                     60000
#define    DEFAULT_MAX_MISSED_FEED_TIMES        3

struct monitor_app_info {
    /** command line to start the app */
    char *cmd_line;
    /** the feed period of the app */
    unsigned long int app_period;
    /** the app's process id */
    pid_t app_pid;
    /** last feed timestamp, unit is second */
    time_t app_last_feed_time;
};

struct monitor_app_info_node {
    struct monitor_app_info_node *next_node;
    struct monitor_app_info app_info;
};

struct wd_configure {
    /** default UDP listen port */
    unsigned long int default_udp_port;
    /** max missing feed times */
    unsigned long int max_missed_feed_times;
    /** default monitor period, unit is second */
    unsigned long int default_feed_period;
    /** process id of watchdog daemon */
    pid_t current_pid;
    /** monitored apps list */
    struct monitor_app_info_node *apps_list_head;
    struct monitor_app_info_node *apps_list_tail;
    /** monitored apps count */
    unsigned int monitored_apps_count;
};

/**
 * Ues to initialize the watchdog configure info.
 * @returns On success the object will be returned
 *          else NULL will returned.
 */
struct wd_configure* get_wd_configure(void);

/**
 * Print the monitored apps's command line.
 * @param   head: the head of monitored apps list.
 */
void print_wd_configure(struct wd_configure *wd_conf);

/**
 * Free the watchdog configure object.
 */
void free_wd_configure(struct wd_configure *wd_conf);

/**
 */
MITFuncRetValue start_libevent_udp_server(struct wd_configure *wd_conf);

#endif























