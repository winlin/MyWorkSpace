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
#include <event2/event.h>
#include <event2/event_struct.h>
#include <pthread.h>

/**
 * Use the macro temporary for test.
 * We read the port from the watchdog's port file late.
 */
#define UDP_PORT_SER       9999
#define UDP_PORT_SELF      9998
#define UDP_IP_SER         "127.0.0.1"

#define MAX_UDP_PG_SIZE    512


static struct sockaddr_in addr_self, addr_server;
static struct feed_thread_configure *feed_configure;

void wd_send_register_pg(int socket_fd)
{
    int pg_len = 0;
    void *pg_reg = wd_pg_register_new(&pg_len,
                                      feed_configure->monitored_pid,
                                      feed_configure->feed_period,
                                      (int)strlen(feed_configure->cmd_line),
                                      feed_configure->cmd_line);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "register pg len:%d  content:%s", pg_len, pg_reg);
    
    ssize_t ret = sendto(socket_fd, pg_reg,
                         (size_t)pg_len, 0,
                         (struct sockaddr *)&addr_server,
                         sizeof(addr_server));
    if (ret < 0) {
        MITLog_DetErrPrintf("sendto() failed");
    }
}

void socket_ev_r_handle(evutil_socket_t fd, short ev_type, void *data)
{
    if (ev_type & EV_READ) {
        char msg[MAX_UDP_PG_SIZE] = {0};
        ssize_t len = recvfrom(fd, msg, sizeof(msg)-1, 0, NULL, NULL);
        if (len > 0) {
            MITWatchdogPgCmd cmd = wd_get_net_package_cmd(msg);
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Server CMD:%d", cmd);
            if (cmd == WD_PG_CMD_REGISTER) {
                struct wd_pg_return_feed *ret_feed_pg = wd_pg_return_feed_unpg(msg, (int)len);
                if (ret_feed_pg == NULL ||
                    ret_feed_pg->error != WD_PG_ERR_SUCCESS) {
                    // re-send the register package
                    wd_send_register_pg(fd);
                } else {
                    // TODO: start a timer to feed regularly
                }
            } else if (cmd == WD_PG_CMD_FEED) {
                // TODO: check the error num
            }
        }
    }
}

void *start_libevent_udp_feed(void *arg)
{    
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        MITLog_DetErrPrintf("socket() failed");
        goto THREAD_EXIT_TAG;
    }
    
    memset(&addr_self, 0, sizeof(addr_self));
    memset(&addr_server, 0, sizeof(addr_server));
    
    addr_self.sin_family        = AF_INET;
    addr_self.sin_port          = htons(UDP_PORT_SELF);
    /** any local network card is ok */
    addr_self.sin_addr.s_addr   = inet_addr(INADDR_ANY);  
    
    addr_server.sin_family      = AF_INET;
    addr_server.sin_port        = htons(UDP_PORT_SER);
    addr_server.sin_addr.s_addr = inet_addr(UDP_IP_SER);
    
    if (bind(socket_fd, (struct sockaddr*)&addr_self, sizeof(addr_self)) < 0) {
        MITLog_DetErrPrintf("bind() failed");
        goto CLOSE_FD_TAG;
    }
    
    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        MITLog_DetErrPrintf("event_base_new() failed");
        goto CLOSE_FD_TAG;
    }
    
    struct event socket_ev_r;
    event_assign(&socket_ev_r, ev_base, socket_fd, EV_READ|EV_PERSIST, socket_ev_r_handle, &socket_ev_r);
    if (event_add(&socket_ev_r, NULL) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "couldn't add an event");
        goto EVENT_BASE_FREE_TAG;
    }
    
    // send the register package
    wd_send_register_pg(socket_fd);
    
    event_base_dispatch(ev_base);
    event_base_free(ev_base);
    
EVENT_BASE_FREE_TAG:
    event_base_free(ev_base);
CLOSE_FD_TAG:
    close(socket_fd);
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