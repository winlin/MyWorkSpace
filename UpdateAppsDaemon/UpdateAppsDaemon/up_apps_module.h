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

/** 
 * The time interval to check the updated apps' list.
 * Unit is second.
 */
#define UP_APP_DAEMON_TIME_INTERVAL         3

/************* Update App Daemon Struct Definition ***************/
typedef enum  UPAppType{
    UPAPP_TYPE_C        = 1,
    UPAPP_TYPE_KMODULE  = 2,
    UPAPP_TYPE_JAVA     = 3
} UPAppType;

struct up_app_info {
    /** the updated app's type */
    UPAppType app_type;
    /** the updated app's name */
    char * app_name;
    /** the update app's absolutely path without file name */
    char * app_path;
    /** 
     * the path points to 'new' app
     * which version number maybe greater
     * than the current installed one.
     */
    char * new_app_path;
    /** the 'new' app's verson */
    char * new_version;
    /** the current installed app's verson */
    char * cur_version;
    /** the installed app's backup app used for rollback */
    char * backup_app_path;
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
