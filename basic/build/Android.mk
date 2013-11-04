
MY_LOCAL_PATH := $(call my-dir)

# build sqlite
include $(MY_LOCAL_PATH)/../sqlite/Android.mk  

# build sqlite extension
include $(MY_LOCAL_PATH)/../sqlite-ext/Android.mk

# build sqlite4java JNI wrapper
include $(MY_LOCAL_PATH)/../sqlite4java/Android.mk

# build background daemon
include $(MY_LOCAL_PATH)/../common/Android.mk
include $(MY_LOCAL_PATH)/../watchdog/Android.mk
include $(MY_LOCAL_PATH)/../installer/Android.mk

# build watchdog4java JNI wrapper
include $(MY_LOCAL_PATH)/../watchdog4java/Android.mk

