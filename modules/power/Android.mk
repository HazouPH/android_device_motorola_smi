# Copyright (C) 2011 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := power.$(TARGET_DEVICE)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SRC_FILES := power_intel.c

ifeq ($(POWERHAL_CLV), true)
    LOCAL_CFLAGS += -DPOWERHAL_CLV
endif
ifeq ($(POWERHAL_MFLD), true)
    LOCAL_CFLAGS += -DPOWERHAL_MFLD
endif
ifeq ($(POWERHAL_GI), true)
    LOCAL_CFLAGS += -DPOWERHAL_GI
endif
ifeq ($(POWERHAL_MRFLD), true)
    LOCAL_CFLAGS += -DPOWERHAL_MRFLD
endif
ifeq ($(POWERHAL_BYT), true)
    LOCAL_CFLAGS += -DPOWERHAL_BYT
endif

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

