//
//  main.c
//  UnixDomainClient
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
#include <sys/stat.h>

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
int client_connection(const char *sock_path)
{
    size_t path_len = strlen(sock_path);
    if (path_len == 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "socket path can't be empty");
        return FUNC_FAIL;
    } else if (path_len > SOCKADDR_UN_SUN_PATH_MAX_LEN) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "socket path length must less than%d", SOCKADDR_UN_SUN_PATH_MAX_LEN);
        return FUNC_FAIL;
    }
    
    int fd = 0, len = 0;
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "socket() faild:%s", strerror(errno));
        return FUNC_FAIL;
    }
    
    struct sockaddr_un client_un;
    client_un.sun_family = AF_UNIX;
    strcpy(client_un.sun_path, sock_path);
    
    /* in case it already exists */
    unlink(sock_path);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(client_un.sun_path);
    if (bind(fd, (struct sockaddr *)&client_un, len) < 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "bind() failed:%s", strerror(errno));
        close(fd);
        return FUNC_FAIL;
    }
    if (chmod(client_un.sun_path, S_IRWXU) < 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "chmod() failed:%s", strerror(errno));
        close(fd);
        return FUNC_FAIL;
    }
    if (connect(fd, (struct sockaddr *)&client_un, len) < 0) {
        MITLogWrite(MITLOG_LEVEL_ERROR, "connect() failed:%s", strerror(errno));
        close(fd);
        return FUNC_FAIL;
    }
    
    return fd;
}

int main(int argc, const char * argv[])
{
    
    MITLogOpen("unix_domain_client");
    
    MITLogWrite(MITLOG_LEVEL_COMMON, "Starting the client...");
    int client_fd = client_connection("~/local_socket_one");
    if (client_fd == FUNC_FAIL) {
        return FUNC_FAIL;
    }
        
    for (int i=1; i < 100; ++i) {
        MITLogWrite(MITLOG_LEVEL_COMMON, "Send message to server...");
        char msg[256] = {0};
        sprintf(msg, "client message :%d", i);
        if(write(client_fd, msg, strlen(msg)) > 0) {
            MITLogWrite(MITLOG_LEVEL_COMMON, "Send message :%d success", i);
        }
        sleep(2);
    }
    
    MITLogClose();
    return 0;
}
    
