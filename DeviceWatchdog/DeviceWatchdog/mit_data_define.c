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
#include <ctype.h>
#include <sys/stat.h>

short wd_get_net_package_cmd(void *pg)
{
    short tmp_short = 0;
    // cmd
    memcpy(&tmp_short, pg, sizeof(short));
    return ntohs(tmp_short);
}

void *wd_pg_register_new(int *pg_len, struct feed_thread_configure *feed_conf)
{
    if (pg_len == NULL || feed_conf == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "pg_len can't be NULL");
        return NULL;
    }
    strip_string_space(&feed_conf->app_name);
    strip_string_space(&feed_conf->cmd_line);
    
    size_t cmd_name_len = strlen(feed_conf->app_name) + strlen(feed_conf->cmd_line) + 1;
    *pg_len = sizeof(short)*2 + sizeof(int)*2 + (int)cmd_name_len;
    void *pg = calloc(*pg_len, sizeof(char));
    if (pg == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        return NULL;
    }
    // cmd
    short tmp_short = htons(WD_PG_CMD_REGISTER);
    memcpy(pg, &tmp_short, sizeof(short));
    // period
    tmp_short = htons(feed_conf->feed_period);
    memcpy(pg+sizeof(short), &tmp_short, sizeof(short));
    // pid
    int tmp_int = htonl(feed_conf->monitored_pid);
    memcpy(pg+sizeof(short)*2, &tmp_int, sizeof(int));
    // cmd_len
    tmp_int = htonl(cmd_name_len);
    memcpy(pg+sizeof(short)*2+sizeof(int), &tmp_int, sizeof(int));
    // cmd_line
    char *cmd_line = calloc(cmd_name_len + 1, sizeof(char));
    if (cmd_line == NULL) {
        MITLog_DetErrPrintf("calloc failed");
        free(pg);
        return NULL;
    }
    sprintf(cmd_line, "%s;%s", feed_conf->app_name, feed_conf->cmd_line);
    memcpy(pg+sizeof(short)*2+sizeof(int)*2, cmd_line, cmd_name_len);
    free(cmd_line);
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
    // app_name
    char *p_cmd = pg+sizeof(short)*2+sizeof(int)*2;
    for (int i=0; i<pg_reg->cmd_len; ++i) {
        if (p_cmd[i] == ';') {
            break;
        }
        pg_reg->name_len++;
    }
    pg_reg->app_name = calloc(pg_reg->name_len+1, sizeof(char));
    if (pg_reg->app_name == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        free(pg_reg);
        return NULL;
    }
    strncpy(pg_reg->app_name, p_cmd, pg_reg->name_len);    
    // cmd_line
    pg_reg->cmd_len = pg_reg->cmd_len - pg_reg->name_len - 1;
    pg_reg->cmd_line = calloc(pg_reg->cmd_len+1, sizeof(char));
    if (pg_reg->cmd_line == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        free(pg_reg->app_name);
        free(pg_reg);
        return NULL;
    }
    strncpy(pg_reg->cmd_line, p_cmd+pg_reg->name_len+1, pg_reg->cmd_len);
    return pg_reg;
}

void *wd_pg_action_new(int *pg_len, MITWatchdogPgCmd cmd, int pid)
{
    if (pg_len == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "pg_len can't be NULL");
        return NULL;
    }
    *pg_len = sizeof(short)*2 + sizeof(int);
    void *pg = calloc(*pg_len, sizeof(char));
    if (pg == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        return NULL;
    }
    // cmd
    short tmp_short = htons(cmd);
    memcpy(pg, &tmp_short, sizeof(short));
    // pid
    int tmp_int = htonl(pid);
    memcpy(pg+sizeof(short)*2, &tmp_int, sizeof(int));
    
    return pg;
}
struct wd_pg_action *wd_pg_action_unpg(void *pg, int pg_len)
{
    if (pg == NULL || pg_len < 1) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramaters error");
        return NULL;
    }
    struct wd_pg_action *pg_action = calloc(1, sizeof(struct wd_pg_action));
    if (pg_action == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        return NULL;
    }
    
    short tmp_short = 0;
    // cmd
    memcpy(&tmp_short, pg, sizeof(short));
    pg_action->cmd = ntohs(tmp_short);
    // pid
    int tmp_int = 0;
    memcpy(&tmp_int, pg+sizeof(short)*2, sizeof(int));
    pg_action->pid = ntohl(tmp_int);
    return pg_action;
}

void *wd_pg_return_new(int *pg_len, MITWatchdogPgCmd cmd, short error_num)
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
struct wd_pg_return *wd_pg_return_unpg(void *pg, int pg_len)
{
    if (pg == NULL || pg_len < 1) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramaters error");
        return NULL;
    }
    struct wd_pg_return *pg_ret_feed = calloc(1, sizeof(struct wd_pg_return));
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

size_t strip_string_space(char **tar_str)
{
    char *src_str = *tar_str;
    size_t src_len = strlen(src_str);
    int h_space = 0, t_space = 0;
    while (h_space<src_len && isspace(*(src_str+h_space))) {
        ++h_space;
    }
    while (t_space<(src_len-h_space) && isspace(*(src_str+src_len-t_space-1))) {
        ++t_space;
    }
    if (t_space || h_space) {
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "head space num:%d  tail space num:%d", h_space, t_space);
        size_t len = src_len-h_space-t_space + 1;
        if (len > 1) {
            *tar_str = calloc(len, sizeof(char));
            if (tar_str == NULL) {
                MITLog_DetErrPrintf("calloc() failed");
            } else {
                strncpy(*tar_str, src_str + h_space, len-1);
                free(src_str);
                return (len-1);
            }
        } else {
            free(src_str);
            *tar_str = NULL;
            return 0;
        }
    }
    return src_len;
}

int compare_two_cmd_line(const char *f_cmdline, const char *s_cmdline)
{
    if (f_cmdline == NULL ||
        s_cmdline == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramaters error");
        return -1;
    }
    size_t min_len = strlen(f_cmdline)>strlen(s_cmdline) ? strlen(f_cmdline) : strlen(s_cmdline);
    for (int i=0; i<min_len; ++i) {
        if (isspace(f_cmdline[i])) {
            break;
        }
        if (f_cmdline[i] != s_cmdline[i]) {
            return -1;
        }
    }
    return 0;
}

MITFuncRetValue write_file(const char *file_path, const char *content, size_t cont_len)
{
    int ret = MIT_RETV_SUCCESS;
    FILE *fp = fopen(file_path, "w");
    if (fp == NULL) {
        if (errno == ENOENT) {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "%s directory will be created", file_path);
            int path_len = (int)strlen(file_path);
            char *path = calloc(path_len, sizeof(char));
            if (path == NULL) {
                MITLog_DetErrPrintf("calloc() failed");
                ret = MIT_RETV_ALLOC_MEM_FAIL;
                goto FUNC_RETU_TAG;
            }
            while (file_path[--path_len] != '/') {}
            strncpy(path, file_path, path_len);
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "file path:%s", path);
            int ret_t = mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);
            if (ret_t == -1 && errno != EEXIST) {
                MITLog_DetErrPrintf("mkdir() failed");
                free(path);
                ret = MIT_RETV_OPEN_FILE_FAIL;
                goto FUNC_RETU_TAG;
            }
            /** write info into configure file */
            fp = fopen(file_path, "w");
            if (fp == NULL) {
                MITLog_DetErrPrintf("fopen() %s failed", file_path);
                free(path);
                ret = MIT_RETV_OPEN_FILE_FAIL;
                goto FUNC_RETU_TAG;
            }
        } else {
            MITLog_DetErrPrintf("fopen() failed");
            ret = MIT_RETV_OPEN_FILE_FAIL;
            goto FUNC_RETU_TAG;
        }
    }
    size_t w_len_sum = 0, w_len = 0;
    while ((w_len = fwrite(content+w_len_sum, sizeof(char), cont_len-w_len_sum, fp)) > 0) {
        w_len_sum += w_len;
    }
    fclose(fp);
FUNC_RETU_TAG:
    return ret;
}







