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
#############################################################################
LOCAL_PATH := $(call my-dir)

#############################################################################
# build uuid static lib
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	libs/uuid/clear.c \
	libs/uuid/compare.c \
	libs/uuid/copy.c \
	libs/uuid/gen_uuid.c \
	libs/uuid/isnull.c \
	libs/uuid/pack.c \
	libs/uuid/parse.c \
	libs/uuid/tst_uuid.c \
	libs/uuid/unpack.c \
	libs/uuid/unparse.c \
	libs/uuid/uuid_time.c 


LOCAL_MODULE := libuuid_static

include $(BUILD_STATIC_LIBRARY)

#############################################################################
#  default flags for sqlite extension
common_sqlite3_ext_flags := -DNDEBUG=1

common_src_files := \
	src/sqlite3_ext_main.c \
	src/utils.c \
	src/exec_sp.c \
	src/exec_sql.c \
	src/table_info.c \
	src/mprintf_value.c \
    src/tool_funcs.c \

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := $(common_src_files)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/libs/uuid $(LOCAL_PATH)/../sqlite

LOCAL_CFLAGS += $(common_sqlite3_ext_flags)

LOCAL_STATIC_LIBRARIES := libuuid_static 

LOCAL_MODULE := libsqliteext

include $(BUILD_SHARED_LIBRARY)
