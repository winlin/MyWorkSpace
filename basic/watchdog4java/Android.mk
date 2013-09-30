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

# build sqlite4java JNI wrapper: sqlite4java-android-armv7.so
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libcommon_static

LOCAL_SRC_FILES := wrapper/native_utilities.c

LOCAL_LDLIBS := -llog

LOCAL_CFLAGS := 

LOCAL_MODULE := libnative_utilities

include $(BUILD_SHARED_LIBRARY)

