//
//  mit_data_define.h
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef MIT_DATA_DEFINE_H
#define MIT_DATA_DEFINE_H

#include <string.h>
#include <sys/types.h>

/**
 * The max UDP package size.
 * The appname and cmdline sum length mustn't larger than it.
 */
#define MAX_UDP_PG_SIZE                      512

/** If app not set feed period watchdog will use it. */
#define DEFAULT_FEED_PERIOD                  15

/**
 * If app doesn't feed for DEFAULT_MAX_MISSED_FEED_TIMES*FEED_PERIOD seconds,
 * watchdog will restart the app.
 */
#define DEFAULT_MAX_MISSED_FEED_TIMES        3

/**
 * If the app has sent MAX_MISS_FEEDBACK_TIMES feed packages without watchdog feeding back,
 * the app will start to send reigister packages.
 */
#define MAX_MISS_FEEDBACK_TIMES              (DEFAULT_MAX_MISSED_FEED_TIMES - 1)

/** The path must end with '/' */
/** Use to store watchdog's configure file. */
#define WD_FILE_PATH_APP                "/tmp/watchdog/"

/** Use to store watchdog's log files. */
#define WD_FILE_PATH_LOG                "/tmp/logs/watchdog"

/** The name of watchdog's configure file. */
#define WD_FILE_NAME_CONFIGURE          "watchdog.cfg"

/** The name of watchdog exported port file name. */
#define WD_FILE_NAME_PORT               "port"

/** The name of watchdog exported pid file name. */
#define WD_FILE_NAME_PID                "pid"

/**
 * The prefix of app update locking file name.
 * The locking file show create in WD_FILE_PATH_APP.
 * After the app update, the locking file should delete immediately.
 */
#define APP_UPDATE_FILE_PREFIX          "UpLock."

/**
 * The divide string between key and value
 * in watchdog configure file.
 */
#define CONF_KEY_VALUE_DIVIDE_STR       "="

/** The divide string between appname
 * and cmdline in watchdog configure file.
 */
#define APP_NAME_CMDLINE_DIVIDE_STR     ";"

/** The path of system procfs. */
#define SYS_PROC_PATH                   "/proc/"
/** The name of file which stores the app's comm */
#define SYS_APP_COMM_NAME               "comm"
/** The file path of file which stores the system max pid */
#define SYS_PROC_MAX_PID_FILE           "/proc/sys/kernel/pid_max"
/************* Watchdag Constants Definition ***************/
typedef enum MITFuncRetValue {
    MIT_RETV_SUCCESS              = 0,
    MIT_RETV_FAIL                 = -1,
    MIT_RETV_HAS_OPENED           = -2,
    MIT_RETV_PARAM_EMPTY          = -100,
    MIT_RETV_OPEN_FILE_FAIL       = -101,
    MIT_RETV_ALLOC_MEM_FAIL       = -102
} MITFuncRetValue;

/**
 *  WD_PG_CMD_REGISTER:     app <---register---> watchdog
 *  WD_PG_CMD_FEED:         app <-----feed-----> watchdog
 *  WD_PG_CMD_UNREGISTER:   app <--unregister--> watchdog
 *  WD_PG_CMD_SELF_UNREG:   app ---unregister--> app
 *                          app <--unregister--> watchdog
 *
 *  WD_PG_CMD_SELF_UNREG: main thread send a UDP to self to start unregister function.
 */
typedef enum MITWatchdogPgCmd {
    WD_PG_CMD_REGISTER      = 1,
    WD_PG_CMD_FEED          = 2,
    WD_PG_CMD_UNREGISTER    = 3,
    WD_PG_CMD_SELF_UNREG    = 4,
    WD_PG_CMD_UNKNOWN       = -1
} MITWatchdogPgCmd;

typedef enum MITWatchdogPgError {
    WD_PG_ERR_SUCCESS           = 0,
    WD_PG_ERR_INVAILD_CMD       = 1,
    WD_PG_ERR_REGISTER_FAIL     = 100,
    WD_PG_ERR_FEED_FAIL         = 200,
    WD_PG_ERR_UNREGISTER_FAIL   = 300
} MITWatchdogPgError;


/************* Watchdag Package Definition ***************/
struct feed_thread_configure {
    pid_t               monitored_pid;
    unsigned long int   feed_period;
    char *              app_name;
    char *              cmd_line;
};

struct wd_pg_register {
    short   cmd;
    short   period;
    int     pid;
    int     cmd_len;
    char *  cmd_line;
    int     name_len;
    char *  app_name;
};

struct wd_pg_return {
    short   cmd;
    short   error;
};

/**
 * The feed and unregister package both use this structure.
 * cmd: WD_PG_CMD_FEED/WD_PG_CMD_UNREGISTER
 */
struct wd_pg_action {
    short   cmd;
    short   reserved;
    int     pid;
};

/************* Watchdag Package Operation ***************/
/**
 * Return the package's cmd type.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
short wd_get_net_package_cmd(void *pg);

/**
 * Create a new watchdog register package.
 *
 * @param pg_len    : the length of the package;
 * @param feed_conf : the monitored app's configure info;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_register_new(int *pg_len, struct feed_thread_configure *feed_conf);

/**
 * Unpackage a new watchdog register package.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
struct wd_pg_register *wd_pg_register_unpg(void *pg, int pg_len);

/**
 * Create a new watchdog action package.
 *
 * @param pg_len    : the length of the package;
 * @param pid       : the register's process id;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_action_new(int *pg_len, MITWatchdogPgCmd cmd, int pid);

/**
 * Unpackage a new watchdog action package.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
struct wd_pg_action *wd_pg_action_unpg(void *pg, int pg_len);

/**
 * Create a new watchdog return package.
 * 
 * @param pg_len    : the length of the package;
 * @param error_num : refer  enum MITWatchdogPgError;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_return_new(int *pg_len, MITWatchdogPgCmd cmd, short error_num);

/**
 * Unpackage a new watchdog return package.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
struct wd_pg_return *wd_pg_return_unpg(void *pg, int pg_len);

/*********************** Tools Function ************************/
/**
 * Strip space from string both on head and tail
 * If there is space the memory will be realloc.
 *
 * @return the new string lenght will return.
 */
size_t strip_string_space(char **tar_str);

/**
 * Compare cmd_name is equal in two cmd_lines
 * @param f_cmdline: first cmd_line string
 * @param s_cmdline: second cmd_line string
 * @return  On equal 0 will be returned
 *          else -1 will be returned.
 */
int compare_two_cmd_line(const char *f_cmdline, const char *s_cmdline);

/**
 * Write content into file_path. 
 * The file_path file will be open with "w" flag.
 * @param file_path: the absolute file path, ex: /tmp/watchdog/watchdog.conf
 * @param content:   the content will be written into file
 * @param cont_len   the lenght of content
 */
MITFuncRetValue write_file(const char *file_path, const char *content, size_t cont_len);

/**
 * Get application's name whoes name is "comm", just app's name without arguments
 * @return On success the pid of the app will be returned.
 *         If the app isn't executing 0 will be returned.
 */
long long int get_pid_with_comm(const char *comm);

#endif










































