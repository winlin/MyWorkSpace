
LOCAL_PATH := $(call my-dir)

# build watchdog jni
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	md5func.c \
	mit_log_module.c \
	mit_data_define.c \
	watchdog_comm.c \

LOCAL_CFLAGS :=  

LOCAL_MODULE := libcommon_static

include $(BUILD_STATIC_LIBRARY)

# pre-build libevent.a

#TODO ifeq ($(wildcard ../libevent/.lib),)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../libevent/.libs/libevent.a

LOCAL_MODULE := libevent_static

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../libevent/.libs/libevent_pthreads.a

LOCAL_MODULE := libevent_pthread_static

include $(PREBUILT_STATIC_LIBRARY)

