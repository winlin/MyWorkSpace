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

ifndef BUILD_ANDROID
export BUILD_ANDROID=1
endif

include $(LOCAL_PATH)/common.mk

# build static lib: libsqlite.a
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := sqlite3.c sqlite3_codec.c

LOCAL_CFLAGS += $(common_sqlite3_flags)

LOCAL_MODULE := libsqlite_static

include $(BUILD_STATIC_LIBRARY)


# build shared lib: libsqlite.so
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := sqlite3.c sqlite3_codec.c

LOCAL_LDLIBS := -ldl

LOCAL_CFLAGS += $(common_sqlite3_flags)

LOCAL_MODULE := libsqlite

include $(BUILD_SHARED_LIBRARY)


# build bin: sqlite3
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libsqlite_static

LOCAL_SRC_FILES := shell.c

LOCAL_CFLAGS += $(common_sqlite3_flags)

LOCAL_LDLIBS += -ldl

LOCAL_MODULE := sqlite3

include $(BUILD_EXECUTABLE)
