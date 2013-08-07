//
//  mit_data_define.c
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "../include/mit_data_define.h"
#include "../include/mit_log_module.h"
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

void *wd_pg_register_new(int *pg_len, struct feed_thread_configure *feed_conf)
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
    *pg_len = sizeof(short)*2 + sizeof(int)*2 + (int)cmd_name_len;
    void *pg = calloc(*pg_len, sizeof(char));
    if (pg == NULL) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "calloc() failed");
        free(cmd_line);
        free(app_name);
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
    char *name_cmd_line = calloc(cmd_name_len + 1, sizeof(char));
    if (name_cmd_line == NULL) {
        MITLog_DetErrPrintf("calloc failed");
        free(cmd_line);
        free(app_name);
        free(pg);
        return NULL;
    }
    sprintf(name_cmd_line, "%s;%s", app_name, cmd_line);
    memcpy(pg+sizeof(short)*2+sizeof(int)*2, name_cmd_line, cmd_name_len);
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
    memcpy(&tmp_short, pg, sizeof(short));
    pg_reg->cmd = ntohs(tmp_short);
    // period
    memcpy(&tmp_short, pg+sizeof(short), sizeof(short));
    pg_reg->period = ntohs(tmp_short);
    // pid
    int tmp_int = 0;
    memcpy(&tmp_int, pg+sizeof(short)*2, sizeof(int));
    pg_reg->pid = ntohl(tmp_int);
    // ignore name_cmd_len 
    // app_name
    char *p_cmd = pg+sizeof(short)*2+sizeof(int)*2;
    char *tmpstr, *token;
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
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "register unpkg appname:%s", pg_reg->app_name);
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
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "register unpkg cmd_line:%s", pg_reg->cmd_line);

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

long long int get_pid_with_comm(const char *comm)
{
    if (comm == NULL || strlen(comm) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramater can't be empty");
        return 0;
    }
    long long int app_pid = 0;
    // get the system max pid
    if ((access(SYS_PROC_MAX_PID_FILE, F_OK)) != 0) {
        MITLog_DetErrPrintf("access() %s failed", SYS_PROC_MAX_PID_FILE);
        return app_pid;
    }
    FILE *max_pid_file = fopen(SYS_PROC_MAX_PID_FILE, "r");
    long long int sys_max_pid = 0;
    if (max_pid_file == NULL) {
        MITLog_DetErrPrintf("fopen() %s falied", SYS_PROC_MAX_PID_FILE);
        return app_pid;
    }
    int scan_num = fscanf(max_pid_file, "%lld", &sys_max_pid);
    if (scan_num <= 0) {
        MITLog_DetErrPrintf("fscanf() failed");
        fclose(max_pid_file);
        return app_pid;
    }

    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get System Max PID:%lld", sys_max_pid);
    fclose(max_pid_file);
    for (long long int i=1; i <= sys_max_pid; ++i) {
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
        scan_num = fscanf(comm_fp, "%s", app_comm);
        if (scan_num <= 0) {
            MITLog_DetErrPrintf("fscanf() %s failed", app_comm_path);
        }
        fclose(comm_fp);
        if (strcmp(comm, app_comm) == 0) {
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Find the target app pid:%lld", i);
            return i;
        }
    }
    return 0;
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
        return MIT_RETV_PARAM_EMPTY;
    }
    /** create the configure path for the app */
    char file_path[MAX_AB_PATH_LEN] = {0};
    snprintf(file_path, MAX_AB_PATH_LEN, "%s%s", APP_CONF_PATH, app_name);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "file path:%s", file_path);
    if ((access(file_path, F_OK)) != 0) {
        int ret_t = mkdir(file_path, S_IRWXU|S_IRWXG|S_IRWXO);
            if (ret_t == -1 && errno != EEXIST) {
                MITLog_DetErrPrintf("mkdir() failed:%s", file_path);
                return MIT_RETV_OPEN_FILE_FAIL;
            }
    }
    /** save the content into file */
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

void get_app_version(const char *app_name, char *ver_str)
{
    if (strlen(app_name) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "paramaters can't be empty");
        return ;
    }
    /** create the configure path for the app */
    char file_path[MAX_AB_PATH_LEN] = {0};
    snprintf(file_path, MAX_AB_PATH_LEN, "%s%s/%s", APP_CONF_PATH, app_name, F_NAME_COMM_VERSON);
    if ((access(file_path, F_OK)) != 0) {
            MITLog_DetErrPrintf("%s dosen't exist", file_path);
            return;
    }
    /** read the content from file */
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "app conf file:%s", file_path);
    FILE *conf_fp = fopen(file_path, "w");
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
    /** check whether the target is updating */
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

MITFuncRetValue start_app_with_cmd_line(const char * cmd_line)
{
    char *exec_line = strdup(cmd_line);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "strdup() exec_line:%s", exec_line);
    if (exec_line == NULL) {
        MITLog_DetErrPrintf("strdup() failed");
        return MIT_RETV_FAIL;
    }
    int unit_alloc_size   = 4;
    int sum_alloc_size    = 0;
    char **cmd_argvs = (char **)calloc(unit_alloc_size, sizeof(char *));
    sum_alloc_size += unit_alloc_size;
    char **np        = NULL;
    if (cmd_argvs == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        free(exec_line);
        return MIT_RETV_FAIL;
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
                return MIT_RETV_FAIL;
            }
            memcpy(np, cmd_argvs, sum_alloc_size*sizeof(char *));
            sum_alloc_size += unit_alloc_size;
            free(cmd_argvs);
            cmd_argvs = np;
        }
        cmd_argvs[j] = token;
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "j:%d token:%s", j, cmd_argvs[j]);
        if (token == NULL) {
            break;
        }
    }
    int pid = vfork();
    MITFuncRetValue f_ret = MIT_RETV_SUCCESS;
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
        f_ret = MIT_RETV_FAIL;
    }
    free(cmd_argvs);
    free(exec_line);
    return f_ret;
}
