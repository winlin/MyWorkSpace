//
//  up_apps_module.h
//  UpdateAppsDaemon
//
//  Created by gtliu on 8/2/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef UpdateAppsDaemon_up_apps_module_h
#define UpdateAppsDaemon_up_apps_module_h

#include "../include/mit_data_define.h"

/* The device update daemon app name */
#define APP_NAME_UPAPPSD               "update_apps_daemon"
/* The device update daemon app verson */
#define VERSION_UPAPPSD                "v1.0.1"

/*
 * The time interval to check the updated apps' list.
 * Unit is second.
 */
#define UP_APP_DAEMON_TIME_INTERVAL    3

/*
 * The suffix of java application's name
 */
#define JAVA_APP_SUFFIX                ".apk"

#define KMODULE_LIB_SUFFIX             ".ko"

/************ Update App Daemon Struct Definition ***************/
typedef enum  UPAppType{
    UPAPP_TYPE_C        = 1,
    UPAPP_TYPE_KMODULE  = 2,
    UPAPP_TYPE_JAVA     = 3
} UPAppType;

/*
 * Attention: the NEW app's version number MAYBE greater
 * than the current installed one.
 */
struct up_app_info {
    /* the updated app's type */
    UPAppType app_type;
    /*
     * the updated app's name.
     * for C app, it MUST be same with watchdog register app name;
     * for JAVA app, it MUST be standar name: com.ipaloma.posjniproject.
     */
    char *app_name;
    /* the path points to NEW app inclucde file name */
    char *new_app_path;
    /* the NEW app's verson */
    char *new_version;

    struct up_app_info *next_node;
};

/*
 * Start the app update function.
 */
MITFuncRetValue start_app_update_func(struct up_app_info **head);

/*
 * Free the update app info node's list memory.
 */
void free_up_app_list(struct up_app_info *head);

#endif
