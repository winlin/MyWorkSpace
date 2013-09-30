//
//  mit_data_define.c
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "mit_data_define.h"
#include "mit_log_module.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

short wd_get_net_package_cmd(void *pg)
{
    short tmp_short = 0;
    // cmd
    memcpy(&tmp_short, pg, sizeof(short));
    return ntohs(tmp_short);
}

void *wd_pg_register_new(int *pg_len,
                         short period,
                         int thread_id,
                         struct feed_thread_configure *feed_conf)
{
    if (pg_len == NULL || feed_conf == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "pg_len can't be NULL");
        return NULL;
    }
    char *app_name = strdup(feed_conf->app_name);
    if (app_name == NULL) {
        MITLog_DetErrPrintf("strdup() app_name failed");
        return NULL;
    }
    strip_string_space(&app_name);
    char *cmd_line = strdup(feed_conf->cmd_line);
    if (cmd_line == NULL) {
        MITLog_DetErrPrintf("strdup() cmd_line failed");
        free(app_name);
        return NULL;
    }
    strip_string_space(&cmd_line);
     // 2 is for the ';' between name & cmd line and the '\0'
    size_t cmd_name_len = strlen(app_name) + strlen(cmd_line) + 2;
    *pg_len = sizeof(short)*2 + sizeof(int)*3 + (int)cmd_name_len;
    void *pg = calloc(*pg_len, sizeof(char));
    if (pg == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        free(cmd_line);
        free(app_name);
        return NULL;
    }

    // cmd
    int offset = 0;
    short tmp_short = htons(WD_PG_CMD_REGISTER);
    memcpy(pg+offset, &tmp_short, sizeof(short));
    // period
    offset += sizeof(short);
    tmp_short = htons(period);
    memcpy(pg+offset, &tmp_short, sizeof(short));
    // pid
    offset += sizeof(short);
    int tmp_int = htonl(feed_conf->monitored_pid);
    memcpy(pg+offset, &tmp_int, sizeof(int));
    // thread_id
    offset += sizeof(int);
    tmp_int = htonl(thread_id);
    memcpy(pg+offset, &tmp_int, sizeof(int));
    // name_cmd_len
    offset += sizeof(int);
    tmp_int = htonl(cmd_name_len);
    memcpy(pg+offset, &tmp_int, sizeof(int));
    // name_cmd_line
    offset += sizeof(int);
    char *name_cmd_line = calloc(cmd_name_len + 1, sizeof(char));
    if (name_cmd_line == NULL) {
        MITLog_DetErrPrintf("calloc failed");
        free(cmd_line);
        free(app_name);
        free(pg);
        return NULL;
    }
    sprintf(name_cmd_line, "%s;%s", app_name, cmd_line);
    memcpy(pg+offset, name_cmd_line, cmd_name_len);
    free(name_cmd_line);
    free(cmd_line);
    free(app_name);
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
    int offset = 0;
    memcpy(&tmp_short, pg+offset, sizeof(short));
    pg_reg->cmd = ntohs(tmp_short);
    // period
    offset += sizeof(short);
    memcpy(&tmp_short, pg+offset, sizeof(short));
    pg_reg->period = ntohs(tmp_short);
    // pid
    offset += sizeof(short);
    int tmp_int = 0;
    memcpy(&tmp_int, pg+offset, sizeof(int));
    pg_reg->pid = ntohl(tmp_int);
    // thread_id
    offset += sizeof(int);
    memcpy(&tmp_int, pg+offset, sizeof(int));
    pg_reg->thread_id = ntohl(tmp_int);
    // ignore name_cmd_len
    offset += sizeof(int);
    // app_name and cmd_line
    offset += sizeof(int);
    char *p_cmd = pg + offset;
    char *tmpstr = NULL, *token = NULL;
    token = strtok_r(p_cmd, APP_NAME_CMDLINE_DIVIDE_STR, &tmpstr);
    if (token) {
        pg_reg->app_name = strdup(token);
        if (pg_reg->app_name == NULL) {
            MITLog_DetErrPrintf("Get strdup() appname failed");
            free(pg_reg);
            return NULL;
        }
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                         "Register package content error. Maybe lack a ';' between appname and cmdline");
        free(pg_reg);
        return NULL;
    }
    strip_string_space(&pg_reg->app_name);
    pg_reg->name_len = strlen(pg_reg->app_name);
    token = strtok_r(NULL, APP_NAME_CMDLINE_DIVIDE_STR, &tmpstr);
    if (token) {
        pg_reg->cmd_line = strdup(token);
        if (pg_reg->cmd_line == NULL) {
            MITLog_DetErrPrintf("Get cmd_line failed");
            free(pg_reg->app_name);
            free(pg_reg);
            return NULL;
        }
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                         "Register package content error. Maybe lack a ';' between appname and cmdline");
        free(pg_reg->app_name);
        free(pg_reg);
        return NULL;
    }
    strip_string_space(&pg_reg->cmd_line);
    pg_reg->cmd_len = strlen(pg_reg->cmd_line);
    
    return pg_reg;
}

void *wd_pg_action_new(int *pg_len,
                       short period,
                       int pid,
                       int thread_id,
                       MITWatchdogPgCmd cmd)
{
    if (pg_len == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "pg_len can't be NULL");
        return NULL;
    }
    *pg_len = sizeof(short)*2 + sizeof(int)*2;
    void *pg = calloc(*pg_len, sizeof(char));
    if (pg == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        return NULL;
    }

    // cmd
    int offset = 0;
    short tmp_short = htons(cmd);
    memcpy(pg+offset, &tmp_short, sizeof(short));
    // period or reserved
    offset += sizeof(short);
    tmp_short = htons(period);
    memcpy(pg+offset, &tmp_short, sizeof(short));
    // pid
    offset += sizeof(short);
    int tmp_int = htonl(pid);
    memcpy(pg+offset, &tmp_int, sizeof(int));
    // thread_id
    offset += sizeof(int);
    tmp_int = htonl(thread_id);
    memcpy(pg+offset, &tmp_int, sizeof(int));
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
    // cmd
    short tmp_short = 0;
    int offset = 0;
    memcpy(&tmp_short, pg+offset, sizeof(short));
    pg_action->cmd = ntohs(tmp_short);
    // period
    offset += sizeof(short);
    memcpy(&tmp_short, pg+offset, sizeof(short));
    pg_action->period = ntohs(tmp_short);
    // pid
    int tmp_int = 0;
    offset += sizeof(short);
    memcpy(&tmp_int, pg+offset, sizeof(int));
    pg_action->pid = ntohl(tmp_int);
    // thread_id
    offset += sizeof(int);
    memcpy(&tmp_int, pg+offset, sizeof(int));
    pg_action->thread_id = ntohl(tmp_int);

    return pg_action;
}

void *wd_pg_return_new(int *pg_len,
                       MITWatchdogPgCmd cmd,
                       short error_num)
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
    int offset = 0;
    short tmp_short = htons(cmd);
    memcpy(pg, &tmp_short, sizeof(short));
    // error number
    offset += sizeof(short);
    tmp_short = htons(error_num);
    memcpy(pg+offset, &tmp_short, sizeof(short));
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

    // cmd
    short tmp_short = 0;
    int offset = 0;
    memcpy(&tmp_short, pg+offset, sizeof(short));
    pg_ret_feed->cmd = ntohs(tmp_short);
    // error number
    offset += sizeof(short);
    memcpy(&tmp_short, pg+offset, sizeof(short));
    pg_ret_feed->error = ntohs(tmp_short);
    return pg_ret_feed;
}

void monapp_send_register_pg(int fd,
                         int period,
                         int thread_id,
                         struct sockaddr_in* addr_server,
                         struct feed_thread_configure *feed_configure)
{
    // read from watchdog port file to get port
    FILE *fp = fopen(CONF_PATH_WATCHD F_NAME_COMM_PORT, "r");
    if (fp == NULL) {
        return;
    }
    int wd_port = 0;
    int scan_num = fscanf(fp, "%d", &wd_port);
    if (scan_num != 1) {
        MITLog_DetErrPrintf("fscanf() failed");
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Watchdog port:%d", wd_port);
    if (wd_port <= 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Get Watchdog port failed");
        return;
    }
    if (wd_port > 0) {
        memset(addr_server, 0, sizeof(struct sockaddr_in));
        addr_server->sin_family      = AF_INET;
        addr_server->sin_port        = htons(wd_port);
        addr_server->sin_addr.s_addr = inet_addr(UDP_IP_SER);
        
        int pg_len = 0;
        void *pg_reg = wd_pg_register_new(&pg_len, period, thread_id, feed_configure);
        if (pg_len <= 0) {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_register_new() failed");
            return;
        }
        ssize_t ret = sendto(fd, pg_reg,
                             (size_t)pg_len, 0,
                             (struct sockaddr *)addr_server,
                             sizeof(struct sockaddr_in));
        if (ret < 0) {
            MITLog_DetErrPrintf("sendto() failed");
        }
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "register pg len:%d", pg_len);
        free(pg_reg);
    }
}

void monapp_send_action_pg(int fd,
                       short period,
                       int pid,
                       int thread_id,
                       MITWatchdogPgCmd cmd,
                       struct sockaddr_in *tar_addr)
{
    int pg_len = 0;
    void *action_pg = wd_pg_action_new(&pg_len, period, pid, thread_id, cmd);
    if (action_pg) {
        ssize_t ret = sendto(fd, action_pg,
                             (size_t)pg_len, 0,
                             (struct sockaddr *)tar_addr,
                             sizeof(struct sockaddr_in));
        if (ret < 0) {
            MITLog_DetErrPrintf("sendto() failed");
        }
        free(action_pg);
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_action_new() failed");
    }
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
    int i;
    for (i=0; i<min_len; ++i) {
        if (isspace(f_cmdline[i])) {
            break;
        }
        if (f_cmdline[i] != s_cmdline[i]) {
            return -1;
        }
    }
    return 0;
}

MITFuncRetValue write_file(const char *file_path, const char* mode, const void *content, size_t cont_len)
{
    int ret = MIT_RETV_SUCCESS;
    FILE *fp = fopen(file_path, mode);
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
            if(create_directory(path) != 0) {
                MITLog_DetErrPrintf("create_directory() failed");
                free(path);
                ret = MIT_RETV_OPEN_FILE_FAIL;
                goto FUNC_RETU_TAG;
            }
            free(path);
            /* write info into configure file */
            fp = fopen(file_path, "w");
            if (fp == NULL) {
                MITLog_DetErrPrintf("fopen() %s failed", file_path);
                ret = MIT_RETV_OPEN_FILE_FAIL;
                goto FUNC_RETU_TAG;
            }
        } else {
            MITLog_DetErrPrintf("fopen() failed");
            ret = MIT_RETV_OPEN_FILE_FAIL;
            goto FUNC_RETU_TAG;
        }
    }
    if (cont_len) {
        size_t w_len_sum = 0, w_len = 0;
        while ((w_len = fwrite(content+w_len_sum, 1, cont_len-w_len_sum, fp)) > 0) {
            w_len_sum += w_len;
        }
    }
    fclose(fp);
FUNC_RETU_TAG:
    return ret;
}

long long int get_pid_with_comm(const char *comm, long long int sys_max_pid)
{
    if (comm == NULL || strlen(comm) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramater can't be empty");
        return 0;
    }
    
    long long int i;
    for (i=1; i <= sys_max_pid; ++i) {
        char app_comm_path[60] = {0};
        sprintf(app_comm_path, "%s%lld/%s", SYS_PROC_PATH, i, SYS_APP_COMM_NAME);
        if ((access(app_comm_path, F_OK)) != 0) {
            continue;
        }
        FILE *comm_fp = fopen(app_comm_path, "r");
        if (comm_fp == NULL) {
            if (errno != ENOENT) {
                MITLog_DetErrPrintf("fopen() %s failed", app_comm_path);
            } else {
                continue;
            }
        }
        char app_comm[60] = {0};
        int scan_num = fscanf(comm_fp, "%s", app_comm);
        if (scan_num <= 0) {
            MITLog_DetErrPrintf("fscanf() %s failed", app_comm_path);
        }
        fclose(comm_fp);
        if (check_substring(comm, app_comm) == 0) {
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Find the %s pid:%lld", comm, i);
            return i;
        }
    }
    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Can't find the %s pid", comm);
    return 0;
}

long long int get_sys_max_pid(void)
{
    long long int sys_max_pid = 0;
    // get the system max pid
    if ((access(SYS_PROC_MAX_PID_FILE, F_OK)) != 0) {
        MITLog_DetErrPrintf("access() %s failed", SYS_PROC_MAX_PID_FILE);
        return sys_max_pid;
    }
    FILE *max_pid_file = fopen(SYS_PROC_MAX_PID_FILE, "r");
    if (max_pid_file == NULL) {
        MITLog_DetErrPrintf("fopen() %s falied", SYS_PROC_MAX_PID_FILE);
        return sys_max_pid;
    }
    int scan_num = fscanf(max_pid_file, "%lld", &sys_max_pid);
    if (scan_num <= 0) {
        MITLog_DetErrPrintf("fscanf() failed");
        fclose(max_pid_file);
        return sys_max_pid;
    }

    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get System Max PID:%lld", sys_max_pid);
    fclose(max_pid_file);

    return sys_max_pid;
}

int check_substring(const char *str_one, const char *str_two)
{
    const char *shorter_str = str_one;
    const char *longer_str  = str_two;
    if(strlen(str_one) > strlen(str_two)) {
        shorter_str = str_two;
        longer_str  = str_one;
    }
    if(strstr(longer_str, shorter_str)) {
        return 0;
    }
    return -1;
}

void get_comm_with_pid(long long int pid, char* app_comm)
{
    if (pid < 2) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "pid can't be samller than 2");
        return;
    }
    char app_comm_path[60] = {0};
    sprintf(app_comm_path, "%s%lld/%s", SYS_PROC_PATH, pid, SYS_APP_COMM_NAME);
    if ((access(app_comm_path, F_OK)) != 0) {
        return;
    }
    FILE *comm_fp = fopen(app_comm_path, "r");
    if (comm_fp == NULL) {
        MITLog_DetErrPrintf("fopen() %s failed", app_comm_path);
        return;
    }
    int scan_num = fscanf(comm_fp, "%s", app_comm);
    if (scan_num <= 0) {
        MITLog_DetErrPrintf("fscanf() %s failed", app_comm_path);
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                         "get pid=%lld app's comm:%s",
                         pid,
                         app_comm);
    }
    fclose(comm_fp);
}

MITFuncRetValue save_app_conf_info(const char *app_name, const char *file_name, const char *content)
{
    if (strlen(app_name) == 0 ||
        strlen(file_name) == 0 ||
        strlen(content) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramaters can't be empty");
        return MIT_RETV_PARAM_ERROR;
    }
    /* create the configure path for the app */
    char file_path[MAX_AB_PATH_LEN] = {0};
    snprintf(file_path, MAX_AB_PATH_LEN, "%s%s", APP_CONF_PATH, app_name);
    if (create_directory(file_path) != 0) {
        MITLog_DetErrPrintf("mkdir() failed:%s", file_path);
        return MIT_RETV_OPEN_FILE_FAIL;
    }
    /* save the content into file */
    char tar_file[MAX_AB_PATH_LEN] = {0};
    snprintf(tar_file, MAX_AB_PATH_LEN, "%s/%s", file_path, file_name);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "app conf file:%s", tar_file);

    FILE *conf_fp = fopen(tar_file, "w");
    if (conf_fp == NULL) {
        MITLog_DetErrPrintf("call fopen() failed:%s", tar_file);
        return MIT_RETV_OPEN_FILE_FAIL;
    }

    size_t w_len_sum = 0, w_len = 0, cont_len = strlen(content);
    while ((w_len = fwrite(content+w_len_sum, sizeof(char), cont_len-w_len_sum, conf_fp)) > 0) {
        w_len_sum += w_len;
    }

    fclose(conf_fp);
    return MIT_RETV_SUCCESS;
}

MITFuncRetValue save_app_pid_ver_info(const char *app_name, pid_t pid, const char *version)
{
    if(pid > 0) {
        /* save pid info */
        char tmp_str[16] = {0};
        sprintf(tmp_str, "%d", pid);
        if(save_app_conf_info(app_name, F_NAME_COMM_PID, tmp_str) != MIT_RETV_SUCCESS) {
            MITLog_DetErrPrintf("save_app_conf_info() %s/%s failed", app_name, F_NAME_COMM_PID);
            return MIT_RETV_FAIL;
        }
    }
    if(version) {
        /* save version info */
        if(save_app_conf_info(app_name, F_NAME_COMM_VERSON, version) != MIT_RETV_SUCCESS) {
            MITLog_DetErrPrintf("save_app_conf_info() %s/%s failed", app_name, F_NAME_COMM_VERSON);
            return MIT_RETV_FAIL;
        }
    }

    return MIT_RETV_SUCCESS;
}

void get_app_version(const char *app_name, char *ver_str)
{
    if (strlen(app_name) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramaters can't be empty");
        return ;
    }
    /* create the configure path for the app */
    char file_path[MAX_AB_PATH_LEN] = {0};
    snprintf(file_path, MAX_AB_PATH_LEN, "%s%s/%s", APP_CONF_PATH, app_name, F_NAME_COMM_VERSON);
    if ((access(file_path, F_OK)) != 0) {
            MITLog_DetErrPrintf("%s dosen't exist", file_path);
            return;
    }
    /* read the content from file */
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "app conf file:%s", file_path);
    FILE *conf_fp = fopen(file_path, "r");
    if (conf_fp == NULL) {
        MITLog_DetErrPrintf("call fopen() failed:%s", file_path);
        return ;
    }

    int scan_num = fscanf(conf_fp, "%s", ver_str);
    if (scan_num <= 0) {
        MITLog_DetErrPrintf("fscanf() %s failed", file_path);
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON,
                         "get app:%s verson number:%s",
                         app_name,
                         ver_str);
    }
    fclose(conf_fp);
}

int check_update_lock_file(const char *app_name)
{
    /* check whether the target is updating */
    char *update_lock_file = calloc(
                                    strlen(APP_CONF_PATH) +
                                    strlen(app_name) +
                                    strlen(F_NAME_COMM_UPLOCK) + 2,
                                    sizeof(char));
    if (update_lock_file == NULL) {
        MITLog_DetErrPrintf("calloc() falied");
        return -1;
    }
    sprintf(update_lock_file, "%s%s/%s", APP_CONF_PATH, app_name, F_NAME_COMM_UPLOCK);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "check update_lock_file:%s", update_lock_file);
    int exist_flag = 0;
    if ((exist_flag = access(update_lock_file, F_OK)) < 0) {
        exist_flag = -1;
    }
    free(update_lock_file);
    return exist_flag;
}

int create_update_lock_file(const char *app_name)
{
    char path_one[MAX_AB_PATH_LEN]  = {0};
    char cmd_str[MAX_AB_PATH_LEN*3] = {0};
    /* create the update lock file */
    snprintf(path_one, MAX_AB_PATH_LEN, "%s%s/%s", APP_CONF_PATH, app_name, F_NAME_COMM_UPLOCK);
    snprintf(cmd_str, MAX_AB_PATH_LEN*3, "touch %s", path_one);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "create update lock file cmd:%s", cmd_str);

    if (posix_system(cmd_str) == -1) {
        MITLog_DetErrPrintf("system():%s failed", cmd_str);
        return -1;
    }
    return 0;
}

int remove_update_lock_file(const char *app_name)
{
    char path_one[MAX_AB_PATH_LEN]  = {0};
    char cmd_str[MAX_AB_PATH_LEN*3] = {0};
    snprintf(path_one, MAX_AB_PATH_LEN, "%s%s/%s", APP_CONF_PATH, app_name, F_NAME_COMM_UPLOCK);
    snprintf(cmd_str ,MAX_AB_PATH_LEN*3, "rm -f %s", path_one);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "remove update lock file cmd:%s", cmd_str);

    if (posix_system(cmd_str) == -1) {
        MITLog_DetErrPrintf("system():%s failed", cmd_str);
        return -1;
    }
    return 0;
}

typedef void (*sighandler_t)(int);
int posix_system(const char *cmd_line)
{
	int ret = 0;
	sighandler_t old_handler;
	old_handler = signal(SIGCHLD, SIG_DFL);
	ret = system(cmd_line);
	signal(SIGCHLD, old_handler);

	return ret;
}

pid_t start_app_with_cmd_line(const char * cmd_line)
{
    char *exec_line = strdup(cmd_line);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "strdup() exec_line:%s", exec_line);
    if (exec_line == NULL) {
        MITLog_DetErrPrintf("strdup() failed");
        return 0;
    }
    int unit_alloc_size   = 4;
    int sum_alloc_size    = 0;
    char **cmd_argvs = (char **)calloc(unit_alloc_size, sizeof(char *));
    sum_alloc_size += unit_alloc_size;
    char **np        = NULL;
    if (cmd_argvs == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        free(exec_line);
        return 0;
    }
    char *str, *save_ptr, *token;
    int j=0;
    for (str=exec_line; ; ++j, str=NULL) {
        token = strtok_r(str, " ", &save_ptr);  // cmd and param divide by " "
        if (j >= sum_alloc_size) {
            np = (char **)calloc(sum_alloc_size+unit_alloc_size, sizeof(char *));
            if (np == NULL) {
                MITLog_DetErrPrintf("calloc() failed");
                free(exec_line);
                free(cmd_argvs);
                return 0;
            }
            memcpy(np, cmd_argvs, sum_alloc_size*sizeof(char *));
            sum_alloc_size += unit_alloc_size;
            free(cmd_argvs);
            cmd_argvs = np;
        }
        cmd_argvs[j] = token;
        if (token == NULL) break;
    }
    int pid = vfork();
    if (pid == 0) {
        int ret = execvp(cmd_argvs[0], cmd_argvs);
        if (ret < 0) {
            MITLog_DetErrPrintf("execvp() failed");
        }
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                         "child process:%d will execvp(%s)",
                         pid,
                         exec_line);
    } else {
        MITLog_DetErrPrintf("vfork() failed");
    }
    free(cmd_argvs);
    free(exec_line);
    return pid;
}

int create_directory(const char *dir_path)
{
    if((access(dir_path, F_OK)) == 0) {
        return 0;
    }
    int ret_t = mkdir(dir_path, S_IRWXU|S_IRWXG|S_IRWXO);
    if (ret_t == -1 && errno != EEXIST) {
        MITLog_DetErrPrintf("mkdir() failed:%s", dir_path);
        return -1;
    }
    return 0;
}

int get_android_power_capacity(const char *sys_power_path)
{
    if (strlen(sys_power_path) < 1) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramater can't be empty");
        return -1;
    }
    int capacity = -1;
    FILE *file = fopen(sys_power_path, "r");
    if (NULL == file) {
        MITLog_DetErrPrintf("fopen(%s) failed", sys_power_path);
        return capacity;
    }
    if (fscanf(file, "%d", &capacity) != 1) {
        MITLog_DetErrPrintf("fscanf() faild");
    }
    
    fclose(file);
    return capacity;
}


