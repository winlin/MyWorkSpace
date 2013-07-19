//
//  mit_data_define.c
//  LibEventUDPClient
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "mit_data_define.h"
#include "MITLogModule.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

MITWatchdogPgCmd wd_get_net_package_cmd(void *pg)
{
    short tmp_short = 0;
    // cmd
    memcpy(&tmp_short, pg, sizeof(short));
    return ntohs(tmp_short);
}

void *wd_pg_register_new(int *pg_len, int pid, short period, int cmd_len, char *cmd_line)
{
    if (pg_len == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "pg_len can't be NULL");
        return NULL;
    }
    *pg_len = sizeof(short)*2 + sizeof(int)*2 + cmd_len;
    void *pg = calloc(*pg_len, sizeof(char));
    if (pg == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        return NULL;
    }
    // cmd
    short tmp_short = htons(WD_PG_CMD_REGISTER);
    memcpy(pg, &tmp_short, sizeof(short));
    // period
    tmp_short = htons(period);
    memcpy(pg+sizeof(short), &tmp_short, sizeof(short));
    // pid
    int tmp_int = htonl(pid);
    memcpy(pg+sizeof(short)*2, &tmp_int, sizeof(int));
    // cmd_len
    tmp_int = htonl(cmd_len);
    memcpy(pg+sizeof(short)*2+sizeof(int), &tmp_int, sizeof(int));
    // cmd_line
    memcpy(pg+sizeof(short)*2+sizeof(int)*2, cmd_line, cmd_len);
    
    return pg;
}
struct wd_pg_register *wd_pg_register_unpg(void *pg, int pg_len)
{
    if (pg == NULL || pg_len < 1) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramaters error");
        return NULL;
    }
    struct wd_pg_register *pg_reg = calloc(1, sizeof(struct wd_pg_register));
    if (pg_reg == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        return NULL;
    }
    
    short tmp_short = 0;
    // cmd
    memcpy(&tmp_short, pg, sizeof(short));
    pg_reg->cmd = ntohs(tmp_short);
    // period
    memcpy(&tmp_short, pg+sizeof(short), sizeof(short));
    pg_reg->period = ntohs(tmp_short);
    // pid
    int tmp_int = 0;
    memcpy(&tmp_int, pg+sizeof(short)*2, sizeof(int));
    pg_reg->pid = ntohl(tmp_int);
    // cmd_len
    memcpy( &tmp_int, pg+sizeof(short)*2+sizeof(int), sizeof(int));
    pg_reg->cmd_len = ntohl(tmp_int);
    // cmd_line
    pg_reg->cmd_line = calloc(pg_reg->cmd_len+1, sizeof(char));
    if (pg_reg->cmd_line == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        free(pg_reg);
        return NULL;
    }
    memcpy(pg_reg->cmd_line, pg+sizeof(short)*2+sizeof(int)*2, pg_reg->cmd_len);
    return pg_reg;
}

void *wd_pg_return_new(int *pg_len, short cmd, short error_num)
{
    if (pg_len == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "pg_len can't be NULL");
        return NULL;
    }
    *pg_len = sizeof(short)*2;
    void *pg = calloc(*pg_len, sizeof(char));
    if (pg == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        return NULL;
    }
    // cmd
    short tmp_short = htons(cmd);
    memcpy(pg, &tmp_short, sizeof(short));
    // error number
    tmp_short = htons(error_num);
    memcpy(pg+sizeof(short), &tmp_short, sizeof(short));
    return pg;
}
struct wd_pg_return_feed *wd_pg_return_feed_unpg(void *pg, int pg_len)
{
    if (pg == NULL || pg_len < 1) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramaters error");
        return NULL;
    }
    struct wd_pg_return_feed *pg_ret_feed = calloc(1, sizeof(struct wd_pg_return_feed));
    if (pg_ret_feed == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        return NULL;
    }
    
    short tmp_short = 0;
    // cmd
    memcpy(&tmp_short, pg, sizeof(short));
    pg_ret_feed->cmd = ntohs(tmp_short);
    // error number
    memcpy(&tmp_short, pg+sizeof(short), sizeof(short));
    pg_ret_feed->error = ntohs(tmp_short);
    return pg_ret_feed;
}
