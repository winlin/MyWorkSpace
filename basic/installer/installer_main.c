//
//  installer_daemon.c
//
//  Created by gtliu on 8/2/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

// -- Describe TBLSOFTWAREPACKAGELIST
// CREATE TABLE "tblsoftwarepackagelist" (
// 	 "guid" char(32,0) NOT NULL ON CONFLICT ROLLBACK DEFAULT (lower(hex(randomblob(16)))),
// 	 "packagename" text(255,0) NOT NULL ON CONFLICT ROLLBACK DEFAULT invalid COLLATE nocase,
// 	 "versionnumber" text(255,0) NOT NULL ON CONFLICT ROLLBACK DEFAULT invalid COLLATE nocase,
// 	 "packagesize" integer NOT NULL ON CONFLICT ROLLBACK DEFAULT 0,
// 	 "checksum" char(16,0) NOT NULL ON CONFLICT ROLLBACK DEFAULT 0,
// 	 "recordsinpackagetbl" integer NOT NULL ON CONFLICT ROLLBACK DEFAULT 0,
// 	 "upgradingdescription" text,
// 	 "preinstallscript" text,
// 	 "postinstallscript" TEXT,
// 	PRIMARY KEY("guid")
// )
// -- Describe TBLSOFTWAREPACKAGE
// CREATE TABLE "tblsoftwarepackage" (
// 	 "guid" char(32,0) NOT NULL ON CONFLICT ROLLBACK DEFAULT (lower(hex(randomblob(16)))) COLLATE nocase,
// 	 "packageid" char(32,0) NOT NULL ON CONFLICT ROLLBACK COLLATE nocase,
// 	 "serialno" integer NOT NULL ON CONFLICT ROLLBACK DEFAULT 0,
// 	 "content" varbinary(2048,0) NOT NULL ON CONFLICT ROLLBACK,
// 	PRIMARY KEY("guid"),
// 	CONSTRAINT "tblsoftwarepackage_foreign_key" FOREIGN KEY ("packageid") REFERENCES "tblsoftwarepackagelist" ("guid") ON DELETE CASCADE ON UPDATE CASCADE
// )

#include "../common/mit_log_module.h"
#include "../common/mit_data_define.h"
#include "../common/md5func.h"
#include "installer_module.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event-config.h>
#include <event2/util.h>


#define MD5_VALUE_STR_LEN               32
#define CHECK_UPDATE_SGINAL_SECONDS     3
#define RETRY_DB_OPTION_MSEC            200
#define RETRY_DB_OPTION_TIMES           3
#define MAX_SQL_STR_LEN                 512

static int app_socket_fd;
static int main_thread_period = 5;
static struct sockaddr_in addr_server;
static struct feed_thread_configure th_conf;
static WDConfirmedUnregFlag wd_confirmed_register;

static int max_missed_feed_time;
static unsigned int update_app_signal;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

MITFuncRetValue set_status_table(sqlite3 *bus_db, sqlite3_stmt *stmt)
{
    MITFuncRetValue func_ret = MIT_RETV_SUCCESS;
    char *package_name = (char *)sqlite3_column_text(stmt, 1);
    char *version_str  = (char *)sqlite3_column_text(stmt, 2);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get APP:%s Version:%s", package_name, version_str);
    char insert_str[MAX_SQL_STR_LEN] = {0};
    snprintf(insert_str, MAX_SQL_STR_LEN,
             "INSERT INTO tblglobalstatus(statusname, strvalue) VALUES('%s', '%s')",
             package_name,
             version_str);
    int f_ret = 0;
    int try_times = 0;
    char *err_msg = NULL;
    if ((f_ret=sqlite3_exec(bus_db, insert_str, NULL, NULL, &err_msg)) != SQLITE_OK &&
        try_times<RETRY_DB_OPTION_TIMES) {
        MITLog_DetErrPrintf("sqlite3_exec(%s) failed:%s", insert_str, sqlite3_errstr(f_ret));
        ++try_times;
    }
    if (f_ret != SQLITE_OK) {
        func_ret = MIT_RETV_FAIL;
        goto FUNC_RETURN_TAG;
    }
FUNC_RETURN_TAG:
    return func_ret;
}

MITFuncRetValue del_from_package_table(sqlite3 *bus_db, sqlite3_stmt *stmt)
{
    MITFuncRetValue func_ret = MIT_RETV_SUCCESS;
    char *packageid = (char *)sqlite3_column_text(stmt, 0);
    int i = 0;
    for (; i < 2; ++i) {
        char del_str[MAX_SQL_STR_LEN] = {0};
        if (i == 0) {
            snprintf(del_str, MAX_SQL_STR_LEN, "DELETE FROM tblsoftwarepackage WHERE packageid='%s'",
             packageid);
        } else if (i == 1) {
            snprintf(del_str, MAX_SQL_STR_LEN,
                     "DELETE FROM tblsoftwarepackagelist WHERE guid='%s'",
                     packageid);
        }
        int f_ret = 0;
        int try_times = 0;
        char *err_msg = NULL;
        if ((f_ret=sqlite3_exec(bus_db, del_str, NULL, NULL, &err_msg)) != SQLITE_OK &&
            try_times<RETRY_DB_OPTION_TIMES) {
            MITLog_DetErrPrintf("sqlite3_exec(%s) failed:%s", del_str, err_msg);
            sqlite3_free(err_msg);
            ++try_times;
        }
        if (f_ret != SQLITE_OK) {
            func_ret = MIT_RETV_FAIL;
            goto FUNC_RETURN_TAG;
        }
    }
FUNC_RETURN_TAG:
    return func_ret;
}

MITFuncRetValue check_install_app(sqlite3 *bus_db, sqlite3_stmt *stmt)
{
    // start a transaction for every updated app
    char *err_msg = NULL;
    if (sqlite3_exec(bus_db, "BEGIN TRANSACTION", NULL, NULL, &err_msg) != SQLITE_OK) {
        MITLog_DetErrPrintf("sqlite3_exec() BEGIN TRANSACTION failed:%s", err_msg);
        sqlite3_free(err_msg);
    }
    
    MITFuncRetValue func_ret = MIT_RETV_SUCCESS;
    char *packageid    = (char *)sqlite3_column_text(stmt, 0);
    char *package_name = (char *)sqlite3_column_text(stmt, 1);
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get update app:%s GUID:%s", package_name, packageid);
    
    // get the package from tblsoftwarepackage table
    char get_query[MAX_SQL_STR_LEN] = {0};
    snprintf(get_query, MAX_SQL_STR_LEN,
             "SELECT content FROM tblsoftwarepackage WHERE packageid='%s' ORDER BY serialno ASC",
             packageid);
    int f_ret = 0;
    int try_times = 0;
    sqlite3_stmt *get_stmt = NULL;
    while ((f_ret=sqlite3_prepare_v2(bus_db, get_query, -1, &get_stmt, NULL)) != SQLITE_OK &&
           try_times<RETRY_DB_OPTION_TIMES) {
        ++try_times;
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "sqlite3_prepare_v2(%s) failed:%s", get_query, sqlite3_errstr(f_ret));
    }
    if (f_ret != SQLITE_OK) {
        func_ret = MIT_RETV_FAIL;
        goto FUNC_RETURN_TAG;
    }
    try_times = 0;
    // create the package file
    char app_tmp_path[MAX_AB_PATH_LEN] = {0};
    snprintf(app_tmp_path, MAX_AB_PATH_LEN, "%s%s", UPDATE_APP_TEMP_PATH, package_name);
    if(write_file(app_tmp_path, "w", NULL, 0) != MIT_RETV_SUCCESS) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "write_file() create or empty the %s failed", app_tmp_path);
        func_ret = MIT_RETV_FAIL;
        goto FINALIZE_STMT_TAG;
    }
    while ((f_ret=sqlite3_step(get_stmt)) == SQLITE_ROW) {
        const void *content = sqlite3_column_blob(get_stmt, 0);
        int len = sqlite3_column_bytes(get_stmt, 0);
        if (len)
            if (write_file(app_tmp_path, "ab", content, len) != MIT_RETV_SUCCESS)
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "write_file() failed:%s", app_tmp_path);
    }
    // check the md5 value
    char *orign_md5 = (char *)sqlite3_column_text(stmt, 4);
    char tmp_app_md5[MD5_VALUE_STR_LEN+1] = {0};
    md5_file(app_tmp_path, tmp_app_md5);
    if (strncmp(orign_md5, tmp_app_md5, MD5_VALUE_STR_LEN) != 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Origin MD5: %s  Tmp MD5: %s", orign_md5, tmp_app_md5);
        func_ret = MIT_RETV_MD5_FAIL;
        goto FINALIZE_STMT_TAG;
    }
    
    // ecex preinstallscript shell
    const char *preinstall_script = (const char *)sqlite3_column_text(stmt, 7);
    if (strlen(preinstall_script)) {
        char preinstall_script_path[MAX_AB_PATH_LEN] = {0};
        snprintf(preinstall_script_path, MAX_AB_PATH_LEN, "%s%s", UPDATE_APP_TEMP_PATH, "preinstall_script.sh");
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Create preinstall_script path:%s", preinstall_script_path);
        if(write_file(preinstall_script_path, "w", preinstall_script, strlen(preinstall_script)) != MIT_RETV_SUCCESS) {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "write_file() create or empty the %s failed", preinstall_script_path);
            func_ret = MIT_RETV_FAIL;
            goto FINALIZE_STMT_TAG;
        }
        char cmd_line[MAX_AB_PATH_LEN] = {0};
        snprintf(cmd_line, MAX_AB_PATH_LEN, "%s %s", "sh", preinstall_script_path);
        if(posix_system(cmd_line) != 0) {
            MITLog_DetErrPrintf("call system(%s) faild:%s", cmd_line);
            func_ret = MIT_RETV_FAIL;
            goto FINALIZE_STMT_TAG;
        }
    }
    // save the postinstallscript shell
    char postinstall_script_path[MAX_AB_PATH_LEN] = {0};
    const char *postinstall_script = (const char *)sqlite3_column_text(stmt, 8);
    if (strlen(postinstall_script)) {
        snprintf(postinstall_script_path, MAX_AB_PATH_LEN, "%s%s", UPDATE_APP_TEMP_PATH, "postinstall_script.sh");
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Create postinstall_script path:%s", postinstall_script_path);
        if(write_file(postinstall_script_path, "w", postinstall_script, strlen(postinstall_script)) != MIT_RETV_SUCCESS) {
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "write_file() create or empty the %s failed", postinstall_script_path);
            func_ret = MIT_RETV_FAIL;
        }
    }
    
    // install app success so will insert into the tblglobalstatus table
    // and delete from tblsoftwarepackagelist table
    if((func_ret=set_status_table(bus_db, stmt)) != MIT_RETV_SUCCESS){
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "set_status_table() failed: tblglobalstatus");
        goto FINALIZE_STMT_TAG;
    }
    if ((func_ret=del_from_package_table(bus_db, stmt)) != MIT_RETV_SUCCESS) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "del_from_package_table() failed: tblsoftwarepackagelist");
        goto FINALIZE_STMT_TAG;
    }
    
    // commit the transaction
    if (sqlite3_exec(bus_db, "COMMIT TRANSACTION", NULL, NULL, &err_msg) != SQLITE_OK) {
        MITLog_DetErrPrintf("sqlite3_exec() COMMIT TRANSACTION failed:%s", err_msg);
        sqlite3_free(err_msg);
        func_ret = MIT_RETV_FAIL;
        goto FINALIZE_STMT_TAG;
    }
    
    // exec postinstallscript shell
    if (strlen(postinstall_script_path)) {
        char cmd_line[MAX_AB_PATH_LEN] = {0};
        snprintf(cmd_line, MAX_AB_PATH_LEN, "%s %s", "sh", postinstall_script_path);
        if(posix_system(cmd_line) != 0) {
            MITLog_DetErrPrintf("call system(%s) faild:%s", cmd_line);
            func_ret = MIT_RETV_FAIL;
        }
    }
    
    sqlite3_finalize(get_stmt);
    return func_ret;
    
FINALIZE_STMT_TAG:
    sqlite3_finalize(get_stmt);
FUNC_RETURN_TAG:
    // rollback the transaction
    if (sqlite3_exec(bus_db, "ROLLBACK TRANSACTION", NULL, NULL, &err_msg) != SQLITE_OK) {
        MITLog_DetErrPrintf("sqlite3_exec() BEGIN TRANSACTION failed:%s", err_msg);
        sqlite3_free(err_msg);
    }
    return func_ret;
}

// Update app thread function
void *update_pthread_func(void *data)
{
    while (1) {
        sleep(CHECK_UPDATE_SGINAL_SECONDS);
        // check the power status
        if (check_power_enough(LOW_POWER_LEVEL) != MIT_RETV_SUCCESS) {
            continue;
        }
        // check the update_app_signal
        int f_ret = 0;
        f_ret  = pthread_mutex_lock(&qlock);
        if (f_ret) {
            MITLog_DetErrPrintf("update_pthread_func() thread mutex lock failed");
            continue;
        }
        if (update_app_signal == 0) {
            f_ret = pthread_mutex_unlock(&qlock);
            if (f_ret) MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
            continue;
        } else {
            f_ret = pthread_mutex_unlock(&qlock);
            if (f_ret) MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
            
            // read databse and update the app
            sqlite3 *bus_db = NULL;
            if ((f_ret=sqlite3_open(SQLITE_DB_PATH_POS, &bus_db)) != SQLITE_OK) {
                MITLog_DetErrPrintf("sqlite3_open(%s) failed:%s", SQLITE_DB_PATH_POS, sqlite3_errstr(f_ret));
                continue;
            }
            // set the busy timeout
            if ((f_ret=sqlite3_busy_timeout(bus_db, RETRY_DB_OPTION_MSEC)) != SQLITE_OK) {
                MITLog_DetErrPrintf("sqlite3_busy_timeout() failed:%s", sqlite3_errstr(f_ret));
                goto CLOSE_DB_TAG;
            }
            // enable the foreign key
            char *err_msg = NULL;
            if (sqlite3_exec(bus_db, "PRAGMA foreign_keys = ON", NULL, NULL, &err_msg) != SQLITE_OK) {
                MITLog_DetErrPrintf("sqlite3_exec() set foreign_keys failed:%s", err_msg);
                sqlite3_free(err_msg);
                goto CLOSE_DB_TAG;
            }
            
            char *query_str = "SELECT * FROM vswpkgtoinstalllist";
            sqlite3_stmt *stmt = NULL;
            int try_times = 0;
            while ((f_ret=sqlite3_prepare_v2(bus_db, query_str, -1, &stmt, NULL)) != SQLITE_OK &&
                   try_times<RETRY_DB_OPTION_TIMES) {
                ++try_times;
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "sqlite3_prepare_v2() failed:%s", sqlite3_errstr(f_ret));
            }
            if (f_ret != SQLITE_OK) goto CLOSE_DB_TAG;
            
            MITFuncRetValue func_ret = MIT_RETV_FAIL;
            try_times = 0;
            while ((f_ret=sqlite3_step(stmt)) == SQLITE_ROW) {                
                if((func_ret=check_install_app(bus_db, stmt)) != MIT_RETV_SUCCESS) 
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "check_install_app() failed:%d will sleep for", func_ret);
            }
            if (f_ret == SQLITE_DONE) {
                // set update_app_signal = 0
                f_ret  = pthread_mutex_lock(&qlock);
                if (f_ret) 
                    MITLog_DetErrPrintf("update_pthread_func() thread mutex lock failed");
                update_app_signal =  0;
                f_ret = pthread_mutex_unlock(&qlock);
                if (f_ret) MITLog_DetErrPrintf("pthread_mutex_unlock() failed");
            }
            sqlite3_finalize(stmt);
        CLOSE_DB_TAG:
            sqlite3_close(bus_db);
        }
    }
}


void socket_ev_r_cb(evutil_socket_t fd, short ev_type, void *data)
{
    // recieve from the watchdog and db_sync
    if (ev_type & EV_READ) {
        struct event_base *ev_base = ((struct event *)data)->ev_base;
        char msg[MAX_UDP_PG_SIZE] = {0};
        struct sockaddr_in src_addr;
        socklen_t addrlen = sizeof(src_addr);
        ssize_t len = recvfrom(fd, msg, sizeof(msg)-1, 0, (struct sockaddr *)&src_addr, &addrlen);
        if (len > 0) {
            MITWatchdogPgCmd cmd = wd_get_net_package_cmd(msg);
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Server CMD:%d", cmd);
            
            struct wd_pg_return *ret_pg = wd_pg_return_unpg(msg, (int)len);
            if (ret_pg) {
                if (cmd == WD_PG_CMD_REGISTER) {
                    if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send rigister package failed, so package will be sent again");
                        monapp_send_register_pg(fd,
                                                main_thread_period,
                                                fd,
                                                &addr_server,
                                                &th_conf);
                    } else {
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "great! rigister success");
                        wd_confirmed_register = WD_CONF_REG_HAS_CONF;
                    }
                } else if (cmd == WD_PG_CMD_FEED) {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get Server Feed Back");
                    if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR,
                                         "send feed package failed:%d and rigister package will resend",
                                         ret_pg->error);
                        wd_confirmed_register = WD_CONF_REG_NOT_CONF;
                    } else {
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "send feed package success");
                        max_missed_feed_time = 0;
                    }
                } else if (cmd == WD_PG_CMD_UNREGISTER) {
                    MITLog_DetPuts(MITLOG_LEVEL_COMMON, "Get Server Unregister Feed Back");
                    // handle unregister
                    if (ret_pg->error != WD_PG_ERR_SUCCESS) {
                        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "send unregister package failed:%d", ret_pg->error);
                        monapp_send_action_pg(fd,
                                              0,
                                              th_conf.monitored_pid,
                                              fd,
                                              WD_PG_CMD_UNREGISTER,
                                              &addr_server);
                    } else {
                        MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "send unregister package success");
                        if(event_base_loopbreak(ev_base) < 0) {
                            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "event_base_loopbreak() failed.");
                        }
                    }
                } else if(cmd == WD_PG_CMD_UPDATE_APP) {
                    MITLog_DetPrintf(MITLOG_LEVEL_COMMON, "Get DB_SYNC updated app cmd");
                    // add the update_app_signal to update the app
                    int lock_ret = pthread_mutex_lock(&qlock);
                    if (lock_ret != 0) {
                        MITLog_DetErrPrintf("pthread_mutex_lock() failed");
                    } else {
                        update_app_signal++;
                        lock_ret = pthread_mutex_unlock(&qlock);
                        if (lock_ret != 0)
                            MITLog_DetErrPrintf("pthread_mutex_lock() failed");
                    }
                } else {
                    MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "Unknown cmd recieved:%d", cmd);
                }
                free(ret_pg);
            } else {
                MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "wd_pg_return_unpg() failed");
            }
        }
    }
}

void timeout_cb(int fd, short ev_type, void* data)
{
    // register and feed to the watchdog
    if (wd_confirmed_register == WD_CONF_REG_NOT_CONF) {
        monapp_send_register_pg(app_socket_fd,
                                main_thread_period,
                                app_socket_fd,
                                &addr_server,
                                &th_conf);
    } else if (wd_confirmed_register == WD_CONF_REG_HAS_CONF) {
        if(max_missed_feed_time++ < MAX_MISS_FEEDBACK_TIMES) {
            monapp_send_action_pg(app_socket_fd,
                                  main_thread_period,
                                  th_conf.monitored_pid,
                                  app_socket_fd,
                                  WD_PG_CMD_FEED,
                                  &addr_server);
        } else {
            wd_confirmed_register = WD_CONF_REG_NOT_CONF;
            max_missed_feed_time = 0;
        }
    }
}

MITFuncRetValue start_listen_and_feed(int main_th_period)
{
    wd_confirmed_register = WD_CONF_REG_NOT_CONF;
    max_missed_feed_time = update_app_signal = 0;
    
    MITFuncRetValue func_ret = MIT_RETV_SUCCESS;
    app_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (app_socket_fd < 0) {
        MITLog_DetErrPrintf("socket() failed");
        func_ret = MIT_RETV_OPEN_FILE_FAIL;
        goto FUNC_EXIT_TAG;
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
        func_ret = MIT_RETV_FAIL;
        goto CLOSE_FD_TAG;
    }
    socklen_t addr_len = sizeof(addr_self);
    if (getsockname(app_socket_fd, (struct sockaddr *)&addr_self, &addr_len) < 0) {
        MITLog_DetErrPrintf("getsockname() failed");
        func_ret = MIT_RETV_FAIL;
        goto CLOSE_FD_TAG;
    }
    // save port info
    char port_str[16] = {0};
    sprintf(port_str, "%d", ntohs(addr_self.sin_port));
    if(save_app_conf_info(APP_NAME_INSTALLERD, F_NAME_COMM_PORT, port_str) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_conf_info() %s/%s failed", APP_NAME_INSTALLERD, F_NAME_COMM_PORT);
        func_ret = MIT_RETV_FAIL;
        goto CLOSE_FD_TAG;
    }
    struct event_base *ev_base = event_base_new();
    if (!ev_base) {
        MITLog_DetErrPrintf("event_base_new() failed");
        func_ret = MIT_RETV_FAIL;
        goto CLOSE_FD_TAG;
    }
    
    // Add UDP server event
    struct event socket_ev_r;
    event_assign(&socket_ev_r, ev_base, app_socket_fd, EV_READ|EV_PERSIST, socket_ev_r_cb, &socket_ev_r);
    if (event_add(&socket_ev_r, NULL) < 0) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "couldn't add an event");
        func_ret = MIT_RETV_FAIL;
        goto EVENT_BASE_FREE_TAG;
    }
    // Add timer event
    struct event timeout;
    struct timeval tv;
    event_assign(&timeout, ev_base, -1, EV_PERSIST, timeout_cb, &timeout);
    evutil_timerclear(&tv);
    tv.tv_sec = main_th_period - 1;
    event_add(&timeout, &tv);
    
    // start the update app thread
    pthread_t upapp_thread;
    int err = pthread_create(&upapp_thread, NULL, update_pthread_func, NULL);
    if (err) {
        MITLog_DetErrPrintf("pthread_create() start the update app thread failed");
        func_ret = MIT_RETV_FAIL;
        goto EVENT_BASE_FREE_TAG;
    }
    
    // start the event dispatch
    event_base_dispatch(ev_base);
    
EVENT_BASE_FREE_TAG:
    event_base_free(ev_base);
CLOSE_FD_TAG:
    close(app_socket_fd);
FUNC_EXIT_TAG:
    MITLog_DetLogExit
    return func_ret;
}

int main(int argc, const char * argv[])
{
    /*
     * Convert self into a daemon
     * Don't change process's working directory to '/'
     * Don't redirect stdin/stdou/stderr
     */
    int ret = 0;
#ifndef MITLOG_DEBUG_ENABLE
	ret = daemon(1, 1);
	if(ret == -1) {
		perror("call daemon() failed!");
	}
#endif
    MITLogOpen(APP_NAME_INSTALLERD, LOG_FILE_PATH APP_NAME_INSTALLERD, _IOLBF);
    
    // save pid and version info
    char tmp_str[16] = {0};
    sprintf(tmp_str, "%d", getpid());
    if(save_app_pid_ver_info(APP_NAME_INSTALLERD, getpid(), VERSION_INSTALLERD) != MIT_RETV_SUCCESS) {
        MITLog_DetErrPrintf("save_app_pid_ver_info() %s failed", APP_NAME_INSTALLERD);
        ret = -1;
        goto CLOSE_LOG_TAG;
    }
    
    th_conf.cmd_line = EXEC_APPS_PATH APP_NAME_INSTALLERD;
    th_conf.app_name = APP_NAME_INSTALLERD;
    th_conf.monitored_pid = getpid();
    
    MITFuncRetValue f_ret = start_listen_and_feed(main_thread_period);
    if(f_ret != MIT_RETV_SUCCESS) {
        MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "start_listen_and_feed() failed");
    }
    
CLOSE_LOG_TAG:
    MITLogClose();
    return ret;
}

