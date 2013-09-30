
LOCAL_PATH := $(call my-dir)

# build installer for android device
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
    installer_module.c installer_main.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../libevent/include $(LOCAL_PATH)/../sqlite

LOCAL_LDLIBS := 

LOCAL_CFLAGS :=

LOCAL_STATIC_LIBRARIES := \
	libcommon_static libsqlite_static libevent_static libevent_pthread_static

LOCAL_MODULE := installerd

include $(BUILD_EXECUTABLE)

# build notify_installer for testing
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := notify_installer.c

LOCAL_STATIC_LIBRARIES := libcommon_static

LOCAL_MODULE := notify_installer

include $(BUILD_EXECUTABLE)

