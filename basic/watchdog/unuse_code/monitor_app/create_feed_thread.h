//
//  create_feed_thread.h
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef CREATE_FEED_THREAD_H
#define CREATE_FEED_THREAD_H

#include "../include/mit_log_module.h"
#include "../include/mit_data_define.h"
#include <sys/types.h>

/*
 * Create the feed thread.
 * !!! The function will call pthread_detach(), so it resource will be
 *     auto released by system; if pthread_detach() failed, pthread_kill(SIGKILL)
 *     will be called.
 *     Before feed thread exit please keep feed_conf must vaild.!!!
 * @param feed_conf     : the configuration of the package,
 *                        the thread is responsible to relase the object;
 * @return MITFuncRetValue
 */
MITFuncRetValue create_feed_thread(struct feed_thread_configure *feed_conf);

/*
 * Unregister the app from watchdog.
 */
MITFuncRetValue unregister_watchdog();

#endif
