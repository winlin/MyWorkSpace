//
//  create_feed_thread.c
//  LibEventUDPClient
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "create_feed_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event-config.h>
#include <event2/util.h>

/**
 * Use the macro temporary for test.
 * We read the port from the watchdog's port file late.
 */
#define UDP_PORT_SER       60000
#define UDP_PORT_SELF      60001
#define UDP_IP_SER         "127.0.0.1"
#define UDP_IP_CLIENT      "127.0.0.1"

static int app_socket_fd;
static struct sockaddr_in addr_server;
static struct feed_thread_configure *feed_configure;
static struct event timeout;
static struct timeval tv;

void wd_send_register_pg(evutil_socket_t fd, struct sockaddr_in *tar_addr)
{
    int pg_len = 0;
    void *pg_reg = wd_pg_register_new(&pg_len,
                                      feed_configure->monitored_pid,
                                      feed_configure->feed_period,
                                      (int)strlen(feed_configure->cmd_line),
                                      feed_configure->cmd_line);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "register pg len:%d  content:%s", pg_len, pg_reg);
    if (pg_reg > 0) {
        ssize_t ret = sendto(fd, pg_reg,
                             (size_t)pg_len, 0,
                             (struct sockaddr *)tar_addr,
                             sizeof(struct sockaddr_in));
        if (ret < 0) {
            MITLog_DetErrPrintf("sendto() failed");
        }
        free(pg_reg);
    } else {
         MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_register_new() failed");
    }
}

void timeout_cb(evutil_socket_t fd, short ev_type, void* data)
{
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "send feed package once");
    int pg_len = 0;
    struct wd_pg_feed *feed_pg = wd_pg_feed_new(&pg_len, feed_configure->monitored_pid);
    if (feed_pg) {
        ssize_t ret = sendto(app_socket_fd, feed_pg,
                             (size_t)pg_len, 0,
                             (struct sockaddr *)&addr_server,
                             sizeof(addr_server));
        if (ret < 0) {
            MITLog_DetErrPrintf("sendto() failed");
        }
        free(feed_pg);
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_feed_new() failed");
    }
}

void socket_ev_r_cb(evutil_socket_t fd, short ev_type, void *data)
{
    MITLog_DetPuts(MITLOG_LEVEL_COMMON, "There is a package recieved");
    if (ev_type & EV_READ) {
        struct event_base *ev_base = ((struct event *)data)->ev_base;
        char msg[MAX_UDP_PG_SIZE] = {0};
        struct sockaddr_in src_addr;
        socklen_t addrlen = sizeof(src_addr);
        ssize_t len = recvfrom(fd, msg, sizeof(msg)-1, 0, (struct sockaddr *)&src_addr, &addrlen);
        if (len > 0) {
            MITWatchdogPgCmd cmd = wd_get_net_package_cmd(msg);
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Server CMD:%d", cmd);
            
            struct wd_pg_return *ret_pg = wd_pg_return_unpg(msg, (int)len);
            if (ret_pg) {
                if (cmd == WD_PG_CMD_REGISTER) {
                    if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send rigister package failed, so package will be sent again");
                        wd_send_register_pg(fd, &src_addr);
                    } else {
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "great! rigister success, now feed timer will start");
                        event_assign(&timeout, ev_base, -1, EV_PERSIST, timeout_cb, &timeout);
                        evutil_timerclear(&tv);
                        tv.tv_sec = feed_configure->feed_period;
                        event_add(&timeout, &tv);
                    }
                } else if (cmd == WD_PG_CMD_FEED) {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Server Feed Back");
                    if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send feed package failed:%d", ret_pg->error);
                    } else {
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "send feed package success");
                    }
                }
                free(ret_pg);
            } else {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_return_unpg() failed");
            }
        }
    }
}

void *start_libevent_udp_feed(void *arg)
{    
    app_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (app_socket_fd < 0) {
        MITLog_DetErrPrintf("socket() failed");
        goto THREAD_EXIT_TAG;
    }
    int set_value = 1;
    if(setsockopt(app_socket_fd, SOL_SOCKET, SO_REUSEPORT, (void *)&set_value, sizeof(set_value)) < 0) {
        MITLog_DetErrPrintf("setsockopt() failed");
    }
    if (fcntl(app_socket_fd, F_SETFD, FD_CLOEXEC) < 0) {
        MITLog_DetErrPrintf("fcntl() failed");
    }
    
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family      = AF_INET;
    addr_server.sin_port        = htons(UDP_PORT_SER);
    addr_server.sin_addr.s_addr = inet_addr(UDP_IP_SER);
    
    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        MITLog_DetErrPrintf("event_base_new() failed");
        goto CLOSE_FD_TAG;
    }
    
    struct event socket_ev_r;
    event_assign(&socket_ev_r, ev_base, app_socket_fd, EV_READ|EV_PERSIST, socket_ev_r_cb, &socket_ev_r);
    if (event_add(&socket_ev_r, NULL) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "couldn't add an event");
        goto EVENT_BASE_FREE_TAG;
    }
    
    // send the register package
    wd_send_register_pg(app_socket_fd, &addr_server);
    
    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    
EVENT_BASE_FREE_TAG:
    event_base_free(ev_base);
CLOSE_FD_TAG:
    close(app_socket_fd);
THREAD_EXIT_TAG:
    pthread_exit(NULL);
}

MITFuncRetValue create_feed_thread(pthread_t *thread, struct feed_thread_configure *feed_conf)
{
    if (thread == NULL ||
        feed_conf->monitored_pid == 0 ||
        feed_conf->feed_period == 0 ||
        strlen(feed_conf->cmd_line) == 0) {
        MITLog_DetPuts(MITLOG_LEVEL_ERROR, "paramaters error");
        return MIT_RETV_PARAM_EMPTY;
    }
    feed_configure = feed_conf;
    int ret = pthread_create(thread, NULL, start_libevent_udp_feed, NULL);
    if (ret != 0) {
        MITLog_DetErrPrintf("pthread_create() failed");
        return MIT_RETV_FAIL;
    }
    return MIT_RETV_SUCCESS;
}