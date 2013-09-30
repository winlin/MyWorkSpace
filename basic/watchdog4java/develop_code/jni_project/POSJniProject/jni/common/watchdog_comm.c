#include "watchdog_comm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#define UDP_IP_SER                  "127.0.0.1"
#define UDP_REC_TIMEOUT_SEC         1
#define UDP_SND_TIMEOUT_SEC         UDP_REC_TIMEOUT_SEC

static struct sockaddr_in addr_server;
static struct feed_thread_configure gl_feed_configure;

void recvfrom_watchdog_callback(int cmd, int error_code)
{
    // TODO: call JAVA callback function
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get watchdog return: [cmd:%d  error_code:%d]", cmd, error_code);
}

struct feed_thread_configure *new_thread_configure(const struct feed_thread_configure *source)
{
    struct feed_thread_configure *tmp_feed = calloc(1, sizeof(struct feed_thread_configure));
    if(tmp_feed == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        goto RETURN_FUNC_TAG;
    }
    tmp_feed->monitored_pid = gl_feed_configure.monitored_pid;
    tmp_feed->app_name      = strdup(gl_feed_configure.app_name);
    if(tmp_feed->app_name == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        goto TMP_FEED_TAG;
    }
    tmp_feed->cmd_line      = strdup(gl_feed_configure.cmd_line);
    if(tmp_feed->cmd_line == NULL) {
        MITLog_DetErrPrintf("calloc() failed");
        goto APP_NAME_TAG;
    }

    return tmp_feed;

APP_NAME_TAG:
    free(tmp_feed->app_name);
    tmp_feed->app_name = NULL;
TMP_FEED_TAG:
    free(tmp_feed);
    tmp_feed = NULL;
RETURN_FUNC_TAG:
    return tmp_feed;
}

void free_thread_configure(struct feed_thread_configure *feed_conf)
{
    if(feed_conf == NULL) {
        MITLog_DetErrPrintf("paramater can't be empty");
        return;
    }
    free(feed_conf->app_name);
    feed_conf->app_name = NULL;
    free(feed_conf->cmd_line);
    feed_conf->cmd_line = NULL;
    free(feed_conf);
    feed_conf           = NULL;
}

MITFuncRetValue wd_send_register_pg(int socket_id,
                                    short period,
                                    int thread_id,
                                    struct feed_thread_configure *feed_conf)
{
    // read from watchdog port file to get port
    FILE *fp = fopen(CONF_PATH_WATCHD F_NAME_COMM_PORT, "r");
    if (fp == NULL) {
        return MIT_RETV_FAIL;
    }
    int wd_port = 0;
    int scan_num = fscanf(fp, "%d", &wd_port);
    fclose(fp);
    if (scan_num <= 0) {
        MITLog_DetErrPrintf("fscanf() failed. Get watchdog port failed:");
        return MIT_RETV_FAIL;
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Watchdog port:%d", wd_port);
    if (wd_port > 0) {
        memset(&addr_server, 0, sizeof(addr_server));
        addr_server.sin_family      = AF_INET;
        addr_server.sin_port        = htons(wd_port);
        addr_server.sin_addr.s_addr = inet_addr(UDP_IP_SER);

        int pg_len = 0;
        void *pg_reg = wd_pg_register_new(&pg_len, period, thread_id, feed_conf);
        if (pg_reg > 0) {
            ssize_t ret = sendto(socket_id, pg_reg,
                                 (size_t)pg_len, 0,
                                 (struct sockaddr *)&addr_server,
                                 sizeof(struct sockaddr_in));
            free(pg_reg);
            if (ret < 0) {
                MITLog_DetErrPrintf("sendto() failed");
                return MIT_RETV_FAIL;
            }
            return MIT_RETV_SUCCESS;
        }
        else {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_register_new() failed");
            return MIT_RETV_FAIL;
        }
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Get Watchdog port failed");
        return MIT_RETV_FAIL;
    }
}

MITFuncRetValue wd_send_action_pg(int socket_id,
                                  int period,
                                  int thread_id,
                                  MITWatchdogPgCmd cmd,
                                  struct sockaddr_in *tar_addr)
{
    if(cmd != WD_PG_CMD_FEED &&
       cmd != WD_PG_CMD_UNREGISTER) {
           MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "params illegal");
           return MIT_RETV_PARAM_ERROR;
    }
    int pg_len = 0;
    void *action_pg = wd_pg_action_new(&pg_len, period, gl_feed_configure.monitored_pid, thread_id, cmd);
    if (action_pg) {
        ssize_t ret = sendto(socket_id, action_pg,
                             (size_t)pg_len, 0,
                             (struct sockaddr *)tar_addr,
                             sizeof(*tar_addr));
        free(action_pg);
        if (ret < 0) {
            MITLog_DetErrPrintf("sendto() failed");
            return MIT_RETV_FAIL;
        }
        return MIT_RETV_SUCCESS;
    } else {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_action_new() failed");
        return MIT_RETV_FAIL;
    }
}

MITFuncRetValue save_appinfo_config(pid_t monitored_pid,
                                    const char *app_name,
                                    const char *cmd_line,
                                    const char *version_str)
{
    if(strlen(app_name) == 0||
            strlen(cmd_line) == 0||
            strlen(version_str) == 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "app_name/cmd_line/version_str paramater cannot be null");
        return MIT_RETV_PARAM_ERROR;
    }

    gl_feed_configure.monitored_pid = monitored_pid;
    /*
     * if pointed to string, free first,
     * because this function maybe called many times
     */
    if(gl_feed_configure.app_name) {
        free(gl_feed_configure.app_name);
        gl_feed_configure.app_name = NULL;
    }
    gl_feed_configure.app_name = strdup(app_name);
    if(gl_feed_configure.app_name == NULL) {
        MITLog_DetErrPrintf("strdup() failed");
        return MIT_RETV_ALLOC_MEM_FAIL;
    }
    if(gl_feed_configure.cmd_line) {
        free(gl_feed_configure.cmd_line);
        gl_feed_configure.cmd_line = NULL;
    }
    gl_feed_configure.cmd_line = strdup(cmd_line);
    if(gl_feed_configure.cmd_line == NULL) {
        free(gl_feed_configure.app_name);
        gl_feed_configure.app_name  = NULL;
        MITLog_DetErrPrintf("strdup() failed");
        return MIT_RETV_ALLOC_MEM_FAIL;
    }
    /* save pid and version info */
    char tmp_str[16] = {0};
    sprintf(tmp_str, "%d", monitored_pid);
    if(save_app_pid_ver_info(app_name, monitored_pid, version_str) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_pid_ver_info() %s failed", app_name);
        return MIT_RETV_FAIL;
    }
    return MIT_RETV_SUCCESS;
}

void free_appinfo_config(void)
{
    free(gl_feed_configure.app_name);
    free(gl_feed_configure.cmd_line);
}

int init_udp_socket(void)
{
    int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        MITLog_DetErrPrintf("socket() failed");
        return -1;
    }
    int set_value = 1;
    if(setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, (void *)&set_value, sizeof(set_value)) < 0) {
        MITLog_DetErrPrintf("setsockopt() failed");
        close(socket_id);
        return -1;
    }
    if (fcntl(socket_id, F_SETFD, FD_CLOEXEC) < 0) {
        MITLog_DetErrPrintf("fcntl() failed");
        close(socket_id);
        return -1;
    }
    struct timeval socket_tv;
    socket_tv.tv_sec = UDP_REC_TIMEOUT_SEC;
    socket_tv.tv_usec = 0;
    if(setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, (void *)&socket_tv, sizeof(socket_tv)) < 0) {
        MITLog_DetErrPrintf("setsockopt() recieve timeout failed");
        close(socket_id);
        return -1;
    }
    socket_tv.tv_sec = UDP_SND_TIMEOUT_SEC;
    if(setsockopt(socket_id, SOL_SOCKET, SO_SNDTIMEO, (void *)&socket_tv, sizeof(socket_tv)) < 0) {
        MITLog_DetErrPrintf("setsockopt() send timeout failed");
        close(socket_id);
        return -1;
    }

    //to get the app's local port so need to bind first
    struct sockaddr_in addr_self;
    memset(&addr_self, 0, sizeof(addr_self));
    addr_self.sin_family      = AF_INET;
    addr_self.sin_port        = 0;
    addr_self.sin_addr.s_addr = INADDR_ANY;
    if (bind(socket_id, (struct sockaddr *)&addr_self, sizeof(addr_self)) < 0) {
        MITLog_DetErrPrintf("bind() failed");
        close(socket_id);
        return -1;
    }

/*
    // save port info
    socklen_t addr_len = sizeof(addr_self);
    memset(&addr_self, 0, sizeof(addr_self));
    if (getsockname(socket_id, (struct sockaddr *)&addr_self, &addr_len) < 0) {
        MITLog_DetErrPrintf("getsockname() failed");
        close(socket_id);
        return MIT_RETV_FAIL;
    }
    long long int self_port = ntohs(addr_self.sin_port);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Port:%lld", self_port);
    if(self_port == 0) {
        MITLog_DetErrPrintf("Get port failed port=%lld", self_port);
        close(socket_id);
        return MIT_RETV_FAIL;
    }
    char port_str[16] = {0};
    sprintf(port_str, "%lld", self_port);
    if(save_app_conf_info(gl_feed_configure.app_name, F_NAME_COMM_PORT, port_str) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_conf_info() %s/%s failed", gl_feed_configure.app_name, F_NAME_COMM_PORT);
        close(socket_id);
        return MIT_RETV_FAIL;
    }
*/
    return socket_id;
}

MITFuncRetValue recvfrom_watchdog(int fd)
{
    char msg[MAX_UDP_PG_SIZE] = {0};
    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);
    ssize_t len = recvfrom(fd, msg, sizeof(msg)-1, MSG_WAITALL, (struct sockaddr *)&src_addr, &addrlen);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "recieve back package len=%d", len);
    if (len > 0) {
        MITWatchdogPgCmd cmd = wd_get_net_package_cmd(msg);
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Server CMD:%d", cmd);
        struct wd_pg_return *ret_pg = wd_pg_return_unpg(msg, (int)len);
        if (ret_pg) {
            if (cmd == WD_PG_CMD_REGISTER) {
                if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send rigister package failed, so package should be sent again");
                }
            } else if (cmd == WD_PG_CMD_FEED) {
                if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send feed package failed:%d and rigister package will resend", ret_pg->error);
                }
            } else if (cmd == WD_PG_CMD_UNREGISTER) {
                /* handle unregister */
                if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send unregister package failed:%d", ret_pg->error);
                }
            }
            /* call the action interface for next action */
            // recvfrom_watchdog_callback(cmd, ret_pg->error);
            if(ret_pg->error == WD_PG_ERR_SUCCESS) {
            	free(ret_pg);
            	return MIT_RETV_SUCCESS;
            } else {
            	free(ret_pg);
            	return MIT_RETV_FAIL;
            }
        } else {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_return_unpg() failed");
            return MIT_RETV_FAIL;
        }
    } else {
        if(errno == EAGAIN ||
           errno == EWOULDBLOCK) {
               MITLog_DetErrPrintf("recvfrom() failed");
               return MIT_RETV_TIMEOUT;
        }
    }
    return MIT_RETV_FAIL;
}

MITFuncRetValue send_wd_register_package(int socket_id,
                                         short period,
                                         int thread_id)
{
    MITFuncRetValue func_ret = wd_send_register_pg(socket_id, period, thread_id, &gl_feed_configure);
    if (func_ret == MIT_RETV_SUCCESS) {
        /* wait for the watchdog return package */
        func_ret = recvfrom_watchdog(socket_id);
        if(func_ret != MIT_RETV_SUCCESS) {
        	MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "recvfrom_watchdog() failed:%d", func_ret);
        }
    } else {
    	MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_send_register_pg() failed:%d", func_ret);
    }
    return func_ret;
}

MITFuncRetValue send_wd_feed_package(int socket_id, short period, int thread_id)
{
    MITFuncRetValue func_ret = wd_send_action_pg(socket_id, period, thread_id, WD_PG_CMD_FEED, &addr_server);
        if (func_ret == MIT_RETV_SUCCESS) {
        /* wait for the watchdog return package */
        func_ret = recvfrom_watchdog(socket_id);
    }
    return func_ret;
}

MITFuncRetValue send_wd_unregister_package(int socket_id, int thread_id)
{
    MITFuncRetValue func_ret = wd_send_action_pg(socket_id, 0, thread_id, WD_PG_CMD_UNREGISTER, &addr_server);
        if (func_ret == MIT_RETV_SUCCESS) {
        /* wait for the watchdog return package */
        func_ret = recvfrom_watchdog(socket_id);
    }
    return func_ret;
}

MITFuncRetValue close_udp_socket(int socket_id)
{
    if(close(socket_id)) {
        MITLog_DetErrPrintf("close() failed");
        return MIT_RETV_FAIL;
    }
    return MIT_RETV_SUCCESS;
}





















