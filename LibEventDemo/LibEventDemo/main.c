//
//  main.c
//  LibEventDemo
//
//  Created by gtliu on 7/15/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <event2/event.h>

int main(int argc, const char * argv[])
{
    int socket_fd = 0, size = 0;
    struct sockaddr_un server_un;
    server_un.sun_family = AF_UNIX;
    
    return 0;
}

