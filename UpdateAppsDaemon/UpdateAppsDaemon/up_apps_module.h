//
//  up_apps_module.h
//  UpdateAppsDaemon
//
//  Created by gtliu on 8/2/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef UpdateAppsDaemon_up_apps_module_h
#define UpdateAppsDaemon_up_apps_module_h

#include "include/mit_data_define.h"

/** The device update daemon app name */
#define APP_NAME_UPAPPSD               "update_apps_daemon"
/** The device update daemon app verson */
#define VERSION_UPAPPSD                "v1.0.1"

/** 
 * The time interval to check the updated apps' list.
 * Unit is second.
 */
#define UP_APP_DAEMON_TIME_INTERVAL         3
#define APP_BACKUP_SUFFIX                   ".BAK"

/************* Update App Daemon Struct Definition ***************/
typedef enum  UPAppType{
    UPAPP_TYPE_C        = 1,
    UPAPP_TYPE_KMODULE  = 2,
    UPAPP_TYPE_JAVA     = 3
} UPAppType;

/**
 * Attention: the NEW app's version number MAYBE greater
 * than the current installed one.
 */
struct up_app_info {
    /** the updated app's type */
    UPAppType app_type;
    /** the updated app's name */
    char * app_name;
    /** 
     * the installed app's absolutely path without file name 
     * ended with '/'
     */
    char * app_path;
    /** the path points to NEW app inclucde file name */
    char * new_app_path;
    /** the NEW app's verson */
    char * new_version;
};

struct up_app_info_node {
    struct up_app_info_node *next_node;
    struct up_app_info app_info;
};

/**
 * Start the app update function.
 */
MITFuncRetValue start_app_update_func(struct up_app_info_node **head);

/**
 * Free the update app info node's list memory.
 */
void free_up_app_list(struct up_app_info_node *head);

#endif
