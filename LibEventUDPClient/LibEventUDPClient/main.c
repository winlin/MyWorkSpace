//
//  main.c
//  LibEventUDPClient
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "create_feed_thread.h"
#include <sys/types.h>
#include <unistd.h>

int main(int argc, const char * argv[])
{
    MITLogOpen("UDPClinet", "./logs");
    
    char dir[1024];
    getcwd(dir, sizeof(dir));
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "%s", dir);
    
    struct feed_thread_configure th_conf;
    th_conf.cmd_line = "./LibEventUDPClient";
    th_conf.feed_period = 5;
    th_conf.monitored_pid = getpid();
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Start the feed thread:cmd=%s", th_conf.cmd_line);
    pthread_t pthread;
    create_feed_thread(&pthread, &th_conf);
    
    while (1) {
        //MITLog_DetPuts(MITLOG_LEVEL_COMMON, "The main thread rotate one time per 3 seconds");
        sleep(3);
    }
    MITLogClose();
    return 0;
}

