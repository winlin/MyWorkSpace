#include "installer_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define APP_NAME_NOTIFY_INSTALLER       "notify_installer"

int main(int argc, const char * argv[])
{
    MITLogOpen(APP_NAME_NOTIFY_INSTALLER, LOG_FILE_PATH APP_NAME_NOTIFY_INSTALLER, _IOLBF);
    int ret = 0;
    // get installer port
    FILE *fp = fopen(APP_CONF_PATH APP_NAME_INSTALLERD"/"F_NAME_COMM_PORT, "r");
    if (fp == NULL) {
        return MIT_RETV_FAIL;
    }
    int wd_port = 0;
    int scan_num = fscanf(fp, "%d", &wd_port);
    fclose(fp);
    if (scan_num <= 0) {
        MITLog_DetErrPrintf("fscanf() failed. Get installer port failed:");
        return MIT_RETV_FAIL;
    }
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get installer port:%d", wd_port);
    int app_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (app_socket_fd < 0) {
        MITLog_DetErrPrintf("socket() failed");
        ret = -1;
        goto FUNC_TETURN_TAG;
    }
    
    int set_value = 1;
    if(setsockopt(app_socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&set_value, sizeof(set_value)) < 0) {
        MITLog_DetErrPrintf("setsockopt() failed");
    }
    if (fcntl(app_socket_fd, F_SETFD, FD_CLOEXEC) < 0) {
        MITLog_DetErrPrintf("fcntl() failed");
    }
    
    struct sockaddr_in addr_self;
    memset(&addr_self, 0, sizeof(addr_self));
    addr_self.sin_family        = AF_INET;
    addr_self.sin_port          = 0;
    // any local network card is ok
    addr_self.sin_addr.s_addr   = INADDR_ANY;
    
    if (bind(app_socket_fd, (struct sockaddr*)&addr_self, sizeof(addr_self)) < 0) {
        MITLog_DetErrPrintf("bind() failed");
        ret = -1;
        goto FUNC_TETURN_TAG;
    }
    struct sockaddr_in addr_installre;
    if (wd_port > 0) {
        memset(&addr_installre, 0, sizeof(addr_installre));
        addr_installre.sin_family      = AF_INET;
        addr_installre.sin_port        = htons(wd_port);
        addr_installre.sin_addr.s_addr = inet_addr(UDP_IP_SER);
    }
    
    monapp_send_action_pg(app_socket_fd, 0, getpid(), app_socket_fd, WD_PG_CMD_UPDATE_APP, &addr_installre);
    
    MITLogClose();
    
FUNC_TETURN_TAG:
    return ret;
}

