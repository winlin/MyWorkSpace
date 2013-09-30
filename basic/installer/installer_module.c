//
//  create_feed_thread.c
//
//  Created by gtliu on 7/19/13.
//  Copyright (c) 2013 GT. All rights reserved.
//

#include "installer_module.h"
#include "../common/mit_data_define.h"
#include "../common/mit_log_module.h"

MITFuncRetValue check_power_enough(int low_level)
{
    low_level = low_level>0 ? low_level : LOW_POWER_LEVEL;
    int capacity = get_android_power_capacity(BATTERY_CAPACITY_PATH);
    if (capacity > low_level) {
        return MIT_RETV_SUCCESS;
    } else {
        return MIT_RETV_FAIL;
    }
}