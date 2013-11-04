//
//  wd_configure.h
//
//  Created by gtliu on 7/18/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef DeviceWatchdog_wd_configure_h
#define DeviceWatchdog_wd_configure_h

#include "../common/mit_data_define.h"
#include <time.h>
#include <unistd.h>

struct monitor_thread_info {
    /* identifer of the monitored thread */
    int thread_id;
    /* the feed period of the monitored thread */
    short thread_period;
    /* last feed timestamp, unit is second */
    time_t last_feed_time;
    struct monitor_thread_info *next_node;
};

struct monitor_app_info {
    /* app's name */
    char *app_name;
    /* command line to start the app */
    char *cmd_line;
    /* the app's process id */
    pid_t app_pid;
    /* current monitor thread count */
    unsigned int monitored_threads_count;
    /* the time read from watchdog configure file or last be restarted time */
    time_t wd_init_time;
    /* the monitored threads list*/
    struct monitor_thread_info *thread_list_head;
    struct monitor_thread_info *thread_list_tail;
    struct monitor_app_info *next_node;
};

struct wd_configure {
    /* max missing feed times */
    unsigned long int max_missed_feed_times;
    /* default monitor period, unit is second */
    unsigned long int default_feed_period;
    /* process id of watchdog daemon */
    pid_t current_pid;
    /* current monitor apps count */
    unsigned int monitored_apps_count;
    /* monitored apps list */
    struct monitor_app_info *app_list_head;
    struct monitor_app_info *app_list_tail;
};

/*
 * Ues to initialize the watchdog configure info.
 * @returns On success the object will be returned
 *          else NULL will returned.
 */
struct wd_configure* get_wd_configure(void);

/*
 * Print the monitored apps's command line.
 * @param   head: the head of monitored apps list.
 */
void print_wd_configure(struct wd_configure *wd_conf);

/*
 * Free the watchdog configure object.
 */
void free_wd_configure(struct wd_configure *wd_conf);

/*
 * Start the watchdog function
 */
MITFuncRetValue start_libevent_udp_server(struct wd_configure *wd_conf);

#endif























