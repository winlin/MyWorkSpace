//
//  main.c
//  LibEventDemo
//
//  Created by gtliu on 7/15/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "MITLogModule.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <event2/event.h>
#include <event2/event_struct.h>

#define LOCAL_SOCKET_PATH       "/tmp/domain_socket_one"

#define FUNC_FAIL_NUM           -1
#define FUNC_SUCCESS_NUM        0

void ev_rw_handle(evutil_socket_t fd, short ev_type, void *data)
{

    if (ev_type & EV_READ) {
        MITLog_DetPuts(MITLOG_LEVEL_COMMON, "it is a read event");
        
        char msg[1024] = {0};   
        ssize_t len = 0;
        
        struct sockaddr_un client_un;
        socklen_t size = sizeof(client_un);
        memset(&client_un, 0, sizeof(client_un));
        len = recvfrom(fd, msg, sizeof(msg)-1, 0, (struct sockaddr *)&client_un, &size);
        if (len > 0) {
            MITLog_DetPuts(MITLOG_LEVEL_COMMON, msg);
        }
    }
    if (ev_type & EV_WRITE) {
        MITLog_DetPuts(MITLOG_LEVEL_COMMON, "it is a write event");
    }
}

int main(int argc, const char * argv[])
{
    MITLogOpen("LibEventDemo");
    
    int socket_fd = 0;
    struct sockaddr_un server_un;
    memset(&server_un, 0, sizeof(server_un));
    server_un.sun_family = AF_UNIX;
    strncpy(server_un.sun_path, LOCAL_SOCKET_PATH, sizeof(server_un.sun_path)-1);
    
    if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "socket() fail:%s", strerror(errno));
        return FUNC_FAIL_NUM;
    }
    int value = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "setsockopt() failed:%s", strerror(errno));
        close(socket_fd);
        return FUNC_FAIL_NUM;
    }
    /* in case socket file has existed */
    unlink(LOCAL_SOCKET_PATH);
    
    if (bind(socket_fd, (struct sockaddr *)&server_un, sizeof(server_un)) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "bind() failed:%s", strerror(errno));
        close(socket_fd);
        return FUNC_FAIL_NUM;
    }
    
    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "event_base_new() fail:%s", strerror(errno));
        close(socket_fd);
        return FUNC_FAIL_NUM;
    }
    
    struct event ev_rw;
    event_assign(&ev_rw, ev_base, socket_fd, EV_READ|EV_PERSIST, ev_rw_handle, &ev_rw);
    
    if (event_add(&ev_rw, NULL) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "couldn't create/add an event");
        event_base_free(ev_base);
        close(socket_fd);
        return FUNC_FAIL_NUM;
    }
    
    event_base_dispatch(ev_base);
    event_free(&ev_rw);
    event_base_free(ev_base);
    
    MITLogClose();
    return 0;
}




































