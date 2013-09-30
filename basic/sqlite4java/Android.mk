# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/../sqlite/common.mk 

# build sqlite4java JNI wrapper: sqlite4java-android-armv7.so
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libsqlite_static

# try to use sqlite_wrap.c which is built automatically when building jar
ifeq ($(wildcard jar/swig/sqlite_wrap.c),)
	sqlite_wrap := wrapper/sqlite_wrap.c
else
	sqlite_wrap := ../jar/swig/sqlite_wrap.c
endif

LOCAL_SRC_FILES := \
		$(sqlite_wrap) \
		wrapper/sqlite3_wrap_manual.c \
		wrapper/intarray.c \

#$(warning $(LOCAL_SRC_FILES))

LOCAL_LDLIBS := -ldl

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../sqlite

LOCAL_CFLAGS := \
	-O2 -static-libgcc \
	-fno-strict-aliasing \
	-fno-omit-frame-pointer \
	-Dfdatasync=fsync \

LOCAL_CFLAGS += $(common_sqlite3_flags)

LOCAL_MODULE := sqlite4java-android-armv7

include $(BUILD_SHARED_LIBRARY)

