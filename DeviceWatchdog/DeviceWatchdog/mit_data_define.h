//
//  mit_data_define.h
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef MIT_DATA_DEFINE_H
#define MIT_DATA_DEFINE_H


#define MAX_UDP_PG_SIZE    512

/************* Watchdag Constants Definition ***************/
typedef enum MITFuncRetValue {
    MIT_RETV_SUCCESS              = 0,
    MIT_RETV_FAIL                 = -1,
    MIT_RETV_HAS_OPENED           = -2,
    MIT_RETV_PARAM_EMPTY          = -100,
    MIT_RETV_OPEN_FILE_FAIL       = -101,
    MIT_RETV_ALLOC_MEM_FAIL       = -102
} MITFuncRetValue;

typedef enum MITWatchdogPgCmd {
    WD_PG_CMD_REGISTER      = 1,
    WD_PG_CMD_FEED          = 2,
    WD_PG_CMD_UNREGISTER    = 3,
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
struct wd_pg_register {
    short   cmd;
    short   period;
    int     pid;
    int     cmd_len;
    char *  cmd_line;
};

struct wd_pg_return {
    short   cmd;
    short   error;
};

struct wd_pg_feed {
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
 * @param pid       : the register's process id;
 * @param peroid    : the feed period; unit is second;
 * @param cmd_len   : the length of the command line;
 * @param cmd_line  : the command line;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_register_new(int *pg_len, int pid, short period, int cmd_len, char *cmd_line);

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
 * Create a new watchdog feed package.
 *
 * @param pg_len    : the length of the package;
 * @param pid       : the register's process id;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_feed_new(int *pg_len, int pid);

/**
 * Unpackage a new watchdog feed package.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
struct wd_pg_feed *wd_pg_feed_unpg(void *pg, int pg_len);

/**
 * Create a new watchdog return package.
 * 
 * @param pg_len    : the length of the package;
 * @param error_num : refer  enum MITWatchdogPgError;
 * @return on success the package point will be return,
 *         and the *pg_len will be set the length of the package;
 *         on error NULL will be returned
 */
void *wd_pg_return_new(int *pg_len, short cmd, short error_num);

/**
 * Unpackage a new watchdog return package.
 *
 * @param pg        : the pointer of the package;
 * @param pg_len    : the length of the package;
 * @return on success the package point will be return,
 *         on error NULL will be returned
 */
struct wd_pg_return *wd_pg_return_unpg(void *pg, int pg_len);



#endif










































