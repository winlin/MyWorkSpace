LOCAL_PATH := $(call my-dir)
 
include $(CLEAR_VARS)
 
LOCAL_MODULE    := native_utilities
LOCAL_SRC_FILES := native_utilities.c common/watchdog_comm.c common/mit_data_define.c common/mit_log_module.c 
LOCAL_LDLIBS := -llog

LOCAL_CFLAGS := -Wall -O2 -std=gnu99 

include $(BUILD_SHARED_LIBRARY)
