
LOCAL_PATH := $(call my-dir)

# build watchdog for android device
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := wd_configure.c watchdog_main.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../libevent/include

LOCAL_LDLIBS := 

LOCAL_CFLAGS :=  

LOCAL_STATIC_LIBRARIES := \
	libcommon_static libevent_static libevent_pthread_static

LOCAL_MODULE := watchdogd

include $(BUILD_EXECUTABLE)

# build monitor_app for testing watchdogd
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := monitor_main.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../libevent/include

LOCAL_LDLIBS := 

LOCAL_CFLAGS := 

LOCAL_STATIC_LIBRARIES := \
	libcommon_static libevent_static libevent_pthread_static

LOCAL_MODULE := monitored_app

include $(BUILD_EXECUTABLE)


