//
//  mit_data_define.h
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef MIT_DATA_DEFINE_H
#define MIT_DATA_DEFINE_H

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define UDP_IP_SER         "127.0.0.1"
#define UDP_IP_CLIENT      "127.0.0.1"

/* The max absolutely path length. */
#define MAX_AB_PATH_LEN                      512
/* The max file name length. */
#define MAX_F_NAME_LEN                       256

/*
 * The max UDP package size.
 * The appname and cmdline sum length mustn't larger than it.
 */
#define MAX_UDP_PG_SIZE                      1024

/*
 * The max line lenght.
 * The appname and cmdline sum length mustn't larger than it.
 */
#define WD_CONFIG_FILE_LINE_MAX_LEN          1024

/* If app not set feed period watchdog will use it. */
#define DEFAULT_FEED_PERIOD                  15

/*
 * If app doesn't feed for DEFAULT_MAX_MISSED_FEED_TIMES*FEED_PERIOD seconds,
 * watchdog will restart the app.
 */
#define DEFAULT_MAX_MISSED_FEED_TIMES        5

/*
 * If the app has sent MAX_MISS_FEEDBACK_TIMES feed packages without watchdog feeding back,
 * the app will start to send reigister packages.
 */
#define MAX_MISS_FEEDBACK_TIMES              (DEFAULT_MAX_MISSED_FEED_TIMES - 2)

/*
 * The path must end with '/'.
 * Please make sure these path exist.
 */
/* The apps' common path */
#define APP_COMM_PATH                  "/sdcard/ipaloma/"
/* The path to store the apps */
#define EXEC_APPS_PATH                 "/data/ipaloma/bin/"
/* Use to store all apps' log files */
#define LOG_FILE_PATH                  APP_COMM_PATH"logs/"
/* Use to store all apps' configure files */
#define APP_CONF_PATH                  APP_COMM_PATH"configure/"
/* The default file name of app's info of pid */
#define F_NAME_COMM_PID                "pid"
/* The default file name of app's info of port */
#define F_NAME_COMM_PORT               "port"
/* The default file name of app's info of version */
#define F_NAME_COMM_VERSON             "version"
/* The default file name of app's configure */
#define F_NAME_COMM_CONF               "configure.cfg"
/* The default file name of app's update lock file */
#define F_NAME_COMM_UPLOCK             "update.lock"

/*
 * The app's log and configure path names
 * must be same with the app's name.
 * ex: app's name is "dev_watchdog"
 *     app's log file path is LOG_FILE_PATH"dev_watchdog"
 */
/* The device watchdog app name */
#define APP_NAME_WATCHDOG              "watchdogd"
/* Use to store watchdog's configure file. */
#define CONF_PATH_WATCHD               APP_CONF_PATH APP_NAME_WATCHDOG"/"
/* Use to store watchdog's log files. */
#define LOG_PATH_WATCHD                LOG_FILE_PATH APP_NAME_WATCHDOG"/"
/* Watchdog's version number */
#define VERSION_WD                     "v1.0.1"
/*
 * The interval of watchdog check apps's alive.
 * Unit is second.
 */
#define WD_CHECK_TIME_INTERVAL               1

/*
 * The divide string between key and value
 * in watchdog configure file.
 */
#define CONF_KEY_VALUE_DIVIDE_STR       "="

/* The divide string between appname
 * and cmdline in watchdog configure file.
 */
#define APP_NAME_CMDLINE_DIVIDE_STR     ";"

/* The path of system procfs. */
#define SYS_PROC_PATH                   "/proc/"
/* The name of file which stores the app's comm */
#define SYS_APP_COMM_NAME               "comm"
/* The file path of file which stores the system max pid */
#define SYS_PROC_MAX_PID_FILE           "/proc/sys/kernel/pid_max"


/************ Function Constants Definition ***************/
typedef enum MITFuncRetValue {
    MIT_RETV_SUCCESS              = 0,
    MIT_RETV_FAIL                 = -1,
    MIT_RETV_PARAM_ERROR          = -100,
    MIT_RETV_OPEN_FILE_FAIL       = -101,
    MIT_RETV_ALLOC_MEM_FAIL       = -102,
    MIT_RETV_TIMEOUT              = -103,
    MIT_RETV_MD5_FAIL             = -104
} MITFuncRetValue;

/*
 *  WD_PG_CMD_REGISTER:     app <---register---> watchdog
 *  WD_PG_CMD_FEED:         app <-----feed-----> watchdog
 *  WD_PG_CMD_UNREGISTER:   app <--unregister--> watchdog
 *  WD_PG_CMD_UPDATE_APP:   db_sync <-- notify --> installer
 *
 */
typedef enum MITWatchdogPgCmd {
    WD_PG_CMD_REGISTER      = 1,
    WD_PG_CMD_FEED          = 2,
    WD_PG_CMD_UNREGISTER    = 3,
    WD_PG_CMD_UPDATE_APP    = 4,
    WD_PG_CMD_UNKNOWN       = -1
} MITWatchdogPgCmd;

typedef enum MITWatchdogPgError {
    WD_PG_ERR_SUCCESS           = 0,
    WD_PG_ERR_INVAILD_CMD       = 1,
    WD_PG_ERR_REGISTER_FAIL     = 100,
    WD_PG_ERR_FEED_FAIL         = 200,
    WD_PG_ERR_UNREGISTER_FAIL   = 300
} MITWatchdogPgError;


/************ Watchdag Package Definition ***************/
struct feed_thread_configure {
    pid_t               monitored_pid;
    char *              app_name;
    char *              cmd_line;
};

struct wd_pg_register {
    short   cmd;
    short   period;
    int     pid;
    int     thread_id;
    int     cmd_len;
    char *  cmd_line;
    int     name_len;
    char *  app_name;
};

struct wd_pg_return {
    short   cmd;
    short   error;
};

/*
 * The feed and unregister package both use this structure.
 * cmd: WD_PG_CMD_FEED/WD_PG_CMD_UNREGISTER
 */
struct wd_pg_action {
    short   cmd;
    short   period;
    int     pid;
    int     thread_id;
};

/************ Watchdag Package Operation ***************/
/*
 * Return the package's cmd type.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
short wd_get_net_package_cmd(void *pg);

/*
 * Create a new watchdog register package.
 *
 * @param pg_len    : the length of the package;
 * @param period    : the period of feed timeout;
 * @param thread_id : the monitored thread id;
 * @param feed_conf : the monitored app's configure info;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_register_new(int *pg_len,
                         short period,
                         int thread_id,
                         struct feed_thread_configure *feed_conf);

/*
 * Unpackage a new watchdog register package.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
struct wd_pg_register *wd_pg_register_unpg(void *pg, int pg_len);

/*
 * Create a new watchdog action package.
 *
 * @param pg_len    : the length of the package;
 * @param period    : the period of feed timeout;
 * @param pid       : the register's process id;
 * @param thread_id : the monitored thread id;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_action_new(int *pg_len,
                       short period,
                       int pid,
                       int thread_id,
                       MITWatchdogPgCmd cmd);

/*
 * Unpackage a new watchdog action package.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
struct wd_pg_action *wd_pg_action_unpg(void *pg, int pg_len);

/*
 * Create a new watchdog return package.
 *
 * @param pg_len    : the length of the package;
 * @param error_num : refer  enum MITWatchdogPgError;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_return_new(int *pg_len,
                       MITWatchdogPgCmd cmd,
                       short error_num);

/*
 * Unpackage a new watchdog return package.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
struct wd_pg_return *wd_pg_return_unpg(void *pg, int pg_len);

/*
 * Monitored application register to watchdog.
 *
 * @param fd             : the socket of the monitored app
 *        period         : the feed period by seconds
 *        thread_id      : the feed thread identifier
 *        addr_server    : watchdog's address
 *        feed_configure : the monitored app's configuration
 */
void monapp_send_register_pg(int fd,
                         int period,
                         int thread_id,
                         struct sockaddr_in* addr_server,
                         struct feed_thread_configure *feed_configure);

/*
 * Monitored application send package to watchdog.
 *
 * @param fd             : the socket of the monitored app
 *        period         : the feed period by seconds
 *        pid            : the monitored app's pid
 *        thread_id      : the feed thread identifier
 *        cmd            : WD_PG_CMD_FEED/WD_PG_CMD_UNREGISTER
 *        tar_addr       : watchdog's address
 */
void monapp_send_action_pg(int fd,
                       short period,
                       int pid,
                       int thread_id,
                       MITWatchdogPgCmd cmd,
                       struct sockaddr_in *tar_addr);

/*
 * Save app's configure info into APP_CONF_PATH
 * The file will be open by flag "w".
 */
MITFuncRetValue save_app_conf_info(const char *app_name, const char *file_name, const char *content);

/*
 * Save app's configure info into APP_CONF_PATH
 * The file will be open by flag "w".
 * @param  app_name: the application's name
 *         pid     : the pid of the current application
 *                   if the pid<0, the pid won't be saved
 *         version : the version str of the application
 *                   if the version==NULL, the version won't be saved
 */
MITFuncRetValue save_app_pid_ver_info(const char *app_name, pid_t pid, const char *version);
/*
 * Get app's verson info from APP_CONF_PATH/app_name/version.
 */
void get_app_version(const char *app_name, char *ver_str);

/*
 * Check whethe app's update lock file exist.
 * The path is APP_CONF_PATH/app_name/update.lock.
 *
 * @return if exist return 0 else return -1
 */
int check_update_lock_file(const char *app_name);

/*
 * Create the update lock file for the special application
 * in the path of APP_CONF_PATH/app_name.
 *
 * @return 0 on success else -1 will be returned.
 */
int create_update_lock_file(const char *app_name);

/*
 * Remove the update lock file
 *
 * @return 0 on success else -1 will be returned.
 */
int remove_update_lock_file(const char *app_name);

/*
 * Replace the application with an other version
 *
 * @return 0 on success else -1 will be returned.
 */
int replace_the_application(const char *app_name, const char *new_app_path);

/*
 * Backup the application
 *
 * @return 0 on success else -1 will be returned.
 */
int backup_application(const char *app_file_path);


/********************** Tools Function ************************/
/*
 * Strip space from string both on head and tail
 * If there is space the memory will be realloc.
 *
 * @return the new string lenght will return.
 */
size_t strip_string_space(char **tar_str);

/*
 * Compare cmd_name is equal in two cmd_lines.
 *
 * @param f_cmdline: first cmd_line string
 * @param s_cmdline: second cmd_line string
 * @return  On equal 0 will be returned
 *          else -1 will be returned.
 */
int compare_two_cmd_line(const char *f_cmdline, const char *s_cmdline);

/*
 * Write content into file_path.
 * The file_path file will be open with "w" flag.
 *
 * @param file_path: the absolute file path, ex: /tmp/watchdog/watchdog.conf
 * @param mode:      "rw"/"w"/"a"
 * @param content:   the content will be written into file
 * @param cont_len:  the lenght of content
 */
MITFuncRetValue write_file(const char *file_path, const char* mode, const void *content, size_t cont_len);

/*
 * Get application's pid whoes name is "comm", just app's name without arguments.
 * @return On success the pid of the app will be returned.
 *         If the app isn't executing 0 will be returned.
 */
long long int get_pid_with_comm(const char *comm, long long int sys_max_pid);

/*
 * Get the max process id from linux proc file system
 *
 */
long long int get_sys_max_pid(void);

/*
 * Check whether one is another one's substring.
 *
 * @return 0 will be returned on success else -1 will be returned.
 */
int check_substring(const char *str_one, const char *str_two);

/*
 * Get application's comm whoes pid is "pid".
 *
 * @return On success the comm of the pid will be set into app_comm.
 */
void get_comm_with_pid(long long int pid, char* app_comm);

/*
 * Call system() function to exec a shell cmd
 *
 * @return the return value of system() call
 *         0 on success else will be <0
 */
int posix_system(const char *cmd_line);

/*
 * Start a new process to execute cmd line.
 */
pid_t start_app_with_cmd_line(const char * cmd_line);

/*
 * Check whether the target directory existed,
 * if not the directory will be created.
 *
 * @param path_prefix: the prefix of the target directory
 *        dir_name   : the name of the target directory
 * @return 0 on success else -1 will be returned.
 */
int create_directory(const char *dir_path);




/*
 * Get the Android device's electric quantity by BATTERY_CAPACITY_PATH.
 * 
 * @param sys_power_path: always use BATTERY_CAPACITY_PATH
 * @return: the percent of current battery capacity, -1 will be return on error
 */
#define BATTERY_CAPACITY_PATH   "/sys/class/power_supply/battery/capacity"
#define LOW_POWER_LEVEL         25
int get_android_power_capacity(const char *sys_power_path);

#endif










































