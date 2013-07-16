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

#define LOCAL_SOCKET_PATH       "/tmp/domain_socket_one"

#define FUNC_FAIL_NUM           -1
#define FUNC_SUCCESS_NUM        0

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
    
    char msg[1024] = {0};
    strcpy(msg, "HELLO LIBEVENT");
    ssize_t ret = sendto(socket_fd, msg, strlen(msg), 0, (struct sockaddr *)&server_un, sizeof(server_un));
    if (ret < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "sendto() fail:%s", strerror(errno));
        close(socket_fd);
        return FUNC_FAIL_NUM;
    }
    close(socket_fd);   
    MITLogClose();
    return 0;
}




































