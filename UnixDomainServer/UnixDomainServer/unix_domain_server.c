//
//  unix_domain_server.c
//  UnixDomainServer
//
//  Created by gtliu on 7/11/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "MITLogModule.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define FUNC_FAIL                           -1
#define FUNC_SUCCESS                        0
#define SOCKADDR_UN_SUN_PATH_MAX_LEN        103  // 104 - 1
#define SER_ACCEPT_CON_NUM                  1

/**
 * Create a server endpoint of a connection.
 *
 * @param  scok_path: the unix domain socket path
 * @return return the file descirption if all ok, <0 on err
 */
int create_serv_listen(const char *sock_path)
{
    size_t path_len = strlen(sock_path);
    if (path_len == 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "socket path can't be empty");
        return FUNC_FAIL;
    } else if (path_len > SOCKADDR_UN_SUN_PATH_MAX_LEN) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "socket path length must less than%d", SOCKADDR_UN_SUN_PATH_MAX_LEN);
        return FUNC_FAIL;
    }
    
    int fd = 0, size = 0;
    struct sockaddr_un server_un;
    server_un.sun_family = AF_UNIX;
    strcpy(server_un.sun_path, sock_path);
    
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "socket() faild:%s", strerror(errno));
        return FUNC_FAIL;
    }
    /* in case it already exists */
    unlink(sock_path);
    
    size = offsetof(struct sockaddr_un, sun_path) + strlen(server_un.sun_path);
    if (bind(fd, (struct sockaddr *)&server_un, size) < 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "bind() failed:%s", strerror(errno));
        close(fd);
        return FUNC_FAIL;
    }
    
    if (listen(fd, SER_ACCEPT_CON_NUM) < 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "listen() failed:%s", strerror(errno));
        close(fd);
        return FUNC_FAIL;
    }
    
    return fd;
}

/**
 * Accept a client's connection and return the relative file descriptor.
 *
 * @param  listen_fd: the server socket file descriptor
 * @return return the file descirption if all ok, <0 on err
 */
int serv_accept(int listen_fd, struct sockaddr_un* income_un)
{
    int child_fd = 0, len = 0;
    
    len = sizeof(income_un);
    if ((child_fd = accept(listen_fd, (struct sockaddr *)&income_un, &len)) < 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "accept() failed:%s", strerror(errno));
        /* often errno=EINTR, if signal caught */
        return FUNC_FAIL;
    }
    
    return child_fd;
}


int main(int argc, const char * argv[])
{

    MITLogOpen("unix_domain_server");
    
    MITLogWrite(MITLOG_LEVEL_COMMON, "Starting the server...");
    int listen_fd = create_serv_listen("~/local_socket_one");
    if (listen_fd == FUNC_FAIL) {
        return FUNC_FAIL;
    }
    
    MITLogWrite(MITLOG_LEVEL_COMMON, "Wait for the client...");
    struct sockaddr_un income_un;
    int child_fd = serv_accept(listen_fd, &income_un);
    if (child_fd == FUNC_FAIL) {
        close(listen_fd);
        return FUNC_FAIL;
    }
    
    char recv_buffer[1024] = {0};
    while (1) {
        MITLogWrite(MITLOG_LEVEL_COMMON, "Wait for the message...");
        if(read(child_fd, recv_buffer, sizeof(recv_buffer) - 1) > 0) {
            printf("Recieve message:%s\n", recv_buffer);
        }
    } 
    
    MITLogClose();
    return 0;
}

