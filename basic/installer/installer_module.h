//
//  installer_module.h
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#ifndef INSTALLER_DAEMON_H
#define INSTALLER_DAEMON_H

#include "../common/mit_log_module.h"
#include "../common/mit_data_define.h"
#include <sys/types.h>
#define APP_NAME_INSTALLERD         "installerd"
#define VERSION_INSTALLERD          "v1.0.1"

#define SQLITE_DB_PATH_POS          "/sdcard/ipaloma/db_sync.sqlite"
#define UPDATE_APP_TEMP_PATH        "/data/local/tmp/"
#define PACKAGE_INSTALL_SCRIPT      "install.sh"

typedef enum WDConfirmedUnregFlag {
    WD_CONF_REG_NOT_CONF        = 0,
    WD_CONF_REG_HAS_CONF        = 1
} WDConfirmedUnregFlag;

/*
 * Check whethe the power is enough 
 * compared with the low_level to install updates.
 *
 * @return greater than low_level return MIT_RETV_SUCCESS
 *         else return MIT_RETV_FAIL
 */
MITFuncRetValue check_power_enough(int low_level);

#endif
