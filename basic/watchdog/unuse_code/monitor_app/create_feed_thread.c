//
//  create_feed_thread.c
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
#include <signal.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event-config.h>
#include <event2/util.h>

/*
 * Use the macro temporary for test.
 * We read the port from the watchdog's port file late.
 */
#define UDP_PORT_SER       60000
#define UDP_IP_SER         "127.0.0.1"
#define UDP_IP_CLIENT      "127.0.0.1"

static pthread_t thread;
static int app_socket_fd;
static struct sockaddr_in addr_server;
static struct feed_thread_configure *feed_configure;
static struct event timeout;
static struct timeval tv;
static int miss_feedback_times;

typedef enum WDConfirmedUnregFlag {
    WD_CONF_UNREG_SELF_NOT_SNED = 0,
    WD_CONF_UNREG_SER_NOT_CONF  = 1,
    WD_CONF_UNREG_SER_HAS_CONF  = 2,
    WD_CONF_REG_NOT_CONF        = 3,
    WD_CONF_REG_HAS_CONF        = 4
} WDConfirmedUnregFlag;
static WDConfirmedUnregFlag wd_confirmed_unreg_flag;
static WDConfirmedUnregFlag wd_confirmed_register;
static pthread_mutex_t conf_mutex;

void wd_send_register_pg(evutil_socket_t fd)
{
    // read from watchdog port file to get port
    FILE *fp = fopen(CONF_PATH_WATCHD F_NAME_COMM_PORT, "r");
    if (fp == NULL) {
        return;
    }
    int wd_port = 0;
    int scan_num = fscanf(fp, "%d", &wd_port);
    if (scan_num <= 0) {
        MITLog_DetErrPrintf("fscanf() failed");
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Watchdog port:%d", wd_port);
    if (wd_port > 0) {
        memset(&addr_server, 0, sizeof(addr_server));
        addr_server.sin_family      = AF_INET;
        addr_server.sin_port        = htons(wd_port);
        addr_server.sin_addr.s_addr = inet_addr(UDP_IP_SER);

        int pg_len = 0;
        void *pg_reg = wd_pg_register_new(&pg_len, feed_configure);
        if (pg_reg > 0) {
            ssize_t ret = sendto(fd, pg_reg,
                                 (size_t)pg_len, 0,
                                 (struct sockaddr *)&addr_server,
                                 sizeof(struct sockaddr_in));
            if (ret < 0) {
                MITLog_DetErrPrintf("sendto() failed");
            }
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "register pg len:%d", pg_len);
            free(pg_reg);
        } else {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_register_new() failed");
        }
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Get Watchdog port failed");
    }
}

void wd_send_action_pg(evutil_socket_t fd, MITWatchdogPgCmd cmd, struct sockaddr_in *tar_addr)
{
    int pg_len = 0;
    void *action_pg = wd_pg_action_new(&pg_len, cmd, feed_configure->monitored_pid);
    if (action_pg) {
        ssize_t ret = sendto(fd, action_pg,
                             (size_t)pg_len, 0,
                             (struct sockaddr *)tar_addr,
                             sizeof(*tar_addr));
        if (ret < 0) {
            MITLog_DetErrPrintf("sendto() failed");
        }
        free(action_pg);
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_action_new() failed");
    }
}

void timeout_cb(evutil_socket_t fd, short ev_type, void* data)
{
    if (wd_confirmed_register == WD_CONF_REG_NOT_CONF) {
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "send register package once");
        wd_send_register_pg(app_socket_fd);
    } else if (wd_confirmed_register == WD_CONF_REG_HAS_CONF) {
        if (miss_feedback_times < MAX_MISS_FEEDBACK_TIMES) {
            ++miss_feedback_times;
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "send feed package once");
            wd_send_action_pg(app_socket_fd, WD_PG_CMD_FEED, &addr_server);
        } else {
            wd_confirmed_register = WD_CONF_REG_NOT_CONF;
        }
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
                        wd_send_register_pg(fd);
                    } else {
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "great! rigister success");
                        wd_confirmed_register = WD_CONF_REG_HAS_CONF;
                        miss_feedback_times = 0;
                    }
                } else if (cmd == WD_PG_CMD_FEED) {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Server Feed Back");
                    if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send feed package failed:%d \
                                         and rigister package will resend", ret_pg->error);
                        wd_send_register_pg(fd);
                    } else {
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "send feed package success");
                        miss_feedback_times = 0;
                    }
                } else if (cmd == WD_PG_CMD_SELF_UNREG) {
                    MITLog_DetPuts(MITLOG_LEVEL_COMMON, "Get self unregister signal");
                    /* send unregister package */
                    if(pthread_mutex_lock(&conf_mutex) != 0) {
                        MITLog_DetErrPrintf("pthread_mutex_lock() failed");
                    }
                    wd_confirmed_unreg_flag = WD_CONF_UNREG_SER_NOT_CONF;
                    if(pthread_mutex_unlock(&conf_mutex) != 0) {
                        MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
                    }
                    wd_send_action_pg(app_socket_fd, WD_PG_CMD_UNREGISTER, &addr_server);
                } else if (cmd == WD_PG_CMD_UNREGISTER) {
                    MITLog_DetPuts(MITLOG_LEVEL_COMMON, "Get Server Unregister Feed Back");
                    /* handle unregister */
                    if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send unregister package failed:%d", ret_pg->error);
                        wd_send_action_pg(fd, WD_PG_CMD_UNREGISTER, &addr_server);
                    } else {
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "send unregister package success");
                        if(pthread_mutex_lock(&conf_mutex) != 0) {
                            MITLog_DetErrPrintf("pthread_mutex_lock() failed");
                        }
                        wd_confirmed_unreg_flag = WD_CONF_UNREG_SER_HAS_CONF;
                        if(pthread_mutex_unlock(&conf_mutex) != 0) {
                            MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
                        }
                        if(event_base_loopbreak(ev_base) < 0) {
                            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "event_base_loopbreak() failed.\
                                             thread will exit.");
                            pthread_exit(NULL);
                        }
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
    if(setsockopt(app_socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&set_value, sizeof(set_value)) < 0) {
        MITLog_DetErrPrintf("setsockopt() failed");
    }
    if (fcntl(app_socket_fd, F_SETFD, FD_CLOEXEC) < 0) {
        MITLog_DetErrPrintf("fcntl() failed");
    }

    /*
     * To get the app's local port so we bind first
     */
    struct sockaddr_in addr_self;
    memset(&addr_self, 0, sizeof(addr_self));
    addr_self.sin_family      = AF_INET;
    addr_self.sin_port        = 0;
    addr_self.sin_addr.s_addr = INADDR_ANY;
    if (bind(app_socket_fd, (struct sockaddr *)&addr_self, sizeof(addr_self)) < 0) {
        MITLog_DetErrPrintf("bind() failed");
    }

    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        MITLog_DetErrPrintf("event_base_new() failed");
        goto CLOSE_FD_TAG;
    }
    // register socket listen handle
    struct event socket_ev_r;
    int ret = event_assign(&socket_ev_r, ev_base, app_socket_fd, EV_READ|EV_PERSIST, socket_ev_r_cb, &socket_ev_r);
    if (ret < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "event_assign() failed");
        goto EVENT_BASE_FREE_TAG;
    }
    if (event_add(&socket_ev_r, NULL) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "couldn't add socket event");
        goto EVENT_BASE_FREE_TAG;
    }
    // add time event
    event_assign(&timeout, ev_base, -1, EV_PERSIST, timeout_cb, &timeout);
    evutil_timerclear(&tv);
    tv.tv_sec = feed_configure->feed_period;
    event_add(&timeout, &tv);

    event_base_dispatch(ev_base);

EVENT_BASE_FREE_TAG:
    event_base_free(ev_base);
CLOSE_FD_TAG:
    close(app_socket_fd);
THREAD_EXIT_TAG:
    pthread_exit(NULL);
}

MITFuncRetValue unregister_watchdog()
{
    MITLog_DetLogEnter
    MITFuncRetValue ret = MIT_RETV_SUCCESS;
    int wait_time = 0;
    while (wait_time++ < 10) {
        if(pthread_mutex_lock(&conf_mutex) != 0) {
            MITLog_DetErrPrintf("pthread_mutex_lock() failed");
        }
        if (wd_confirmed_unreg_flag == WD_CONF_UNREG_SELF_NOT_SNED) {
            if(pthread_mutex_unlock(&conf_mutex) != 0) {
                MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
            }
            struct sockaddr_in addr_self;
            socklen_t addr_len = sizeof(addr_self);
            if (getsockname(app_socket_fd, (struct sockaddr *)&addr_self, &addr_len) < 0) {
                MITLog_DetErrPrintf("getsockname() failed");
                ret = MIT_RETV_FAIL;
                goto RETURN_FUNC_TAG;
            }
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Port:%d", ntohs(addr_self.sin_port));
            int pg_len = 0;
            void *self_unreg_pg = wd_pg_return_new(&pg_len, WD_PG_CMD_SELF_UNREG, WD_PG_ERR_SUCCESS);
            if (self_unreg_pg == NULL) {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_return_new() failed");
                ret = MIT_RETV_FAIL;
                goto RETURN_FUNC_TAG;
            }
            ssize_t len = sendto(app_socket_fd, self_unreg_pg,
                                 (size_t)pg_len, 0,
                                 (struct sockaddr *)&addr_self,
                                 sizeof(addr_self));
            if (len < 0) {
                MITLog_DetErrPrintf("sendto() failed");
            }
            free(self_unreg_pg);
        } else if(wd_confirmed_unreg_flag == WD_CONF_UNREG_SER_NOT_CONF) {
            if(pthread_mutex_unlock(&conf_mutex) != 0) {
                MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
            }
            MITLog_DetPuts(MITLOG_LEVEL_COMMON, "Wait watchdog to confirm unregistering");
        } else if (wd_confirmed_unreg_flag == WD_CONF_UNREG_SER_HAS_CONF) {
            if(pthread_mutex_unlock(&conf_mutex) != 0) {
                MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
            }
            ret = MIT_RETV_SUCCESS;
            goto RETURN_FUNC_TAG;
        } else {
            if(pthread_mutex_unlock(&conf_mutex) != 0) {
                MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
            }
        }
        sleep(2);
    }

RETURN_FUNC_TAG:
    pthread_mutex_destroy(&conf_mutex);
    return ret;
}

MITFuncRetValue create_feed_thread(struct feed_thread_configure *feed_conf)
{
    if (feed_conf->monitored_pid == 0 ||
        feed_conf->feed_period == 0 ||
        strlen(feed_conf->cmd_line) == 0) {
        MITLog_DetPuts(MITLOG_LEVEL_ERROR, "paramaters error");
        return MIT_RETV_PARAM_ERROR;
    }
    feed_configure = feed_conf;
    wd_confirmed_register = WD_CONF_REG_NOT_CONF;
    miss_feedback_times = 0;
    if(pthread_mutex_init(&conf_mutex, NULL) != 0){
        MITLog_DetErrPrintf("pthread_mutex_init() failed");
    }
    if(pthread_mutex_lock(&conf_mutex) != 0) {
        MITLog_DetErrPrintf("pthread_mutex_lock() failed");
    }
    wd_confirmed_unreg_flag = WD_CONF_UNREG_SELF_NOT_SNED;
    if(pthread_mutex_unlock(&conf_mutex) != 0) {
        MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
    }

    int ret = pthread_create(&thread, NULL, start_libevent_udp_feed, NULL);
    if (ret != 0) {
        MITLog_DetErrPrintf("pthread_create() failed");
        return MIT_RETV_FAIL;
    }
    ret = pthread_detach(thread);
    if (ret != 0) {
        MITLog_DetErrPrintf("pthread_detach() failed");
        pthread_kill(thread, SIGKILL);
        return MIT_RETV_FAIL;
    }
    return MIT_RETV_SUCCESS;
}
