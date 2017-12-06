# Copyright (c) 2017 The LineageOS Project
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

# Audio HAL
include $(CLEAR_VARS)

LOCAL_MODULE := audio.primary.$(TARGET_BOOTLOADER_BOARD_NAME)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := audiowrapper.c

LOCAL_C_INCLUDES += \
    system/media/audio_utils/include \
    system/media/audio_effects/include
LOCAL_SHARED_LIBRARIES := liblog libcutils libaudioutils libdl
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
