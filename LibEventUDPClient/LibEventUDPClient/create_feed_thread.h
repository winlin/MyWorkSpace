//
//  create_feed_thread.h
//  LibEventUDPClient
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef CREATE_FEED_THREAD_H
#define CREATE_FEED_THREAD_H

#include "MITLogModule.h"
#include "mit_data_define.h"
#include <sys/types.h>

struct feed_thread_configure {
    pid_t               monitored_pid;
    unsigned long int   feed_period;
    char *              cmd_line;
};

/**
 * Create the feed thread.
 *
 * @param thread        : the pthread_t variable;
 * @param feed_conf     : the configuration of the package,
 *                        the thread is responsible to relase the object;
 * @return MITFuncRetValue
 */
MITFuncRetValue create_feed_thread(pthread_t *thread, struct feed_thread_configure *feed_conf);

#endif
