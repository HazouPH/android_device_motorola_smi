# Copyright (C) 2017, The LineageOS Project
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

ifeq ($(WITH_TWRP), true)
     LOCAL_PATH := $(call my-dir)

     # Copy vendor files needed for touchscreen
     $(shell mkdir -p "$(TARGET_RECOVERY_ROOT_OUT)/vendor/firmware";)
     $(shell cp -rf "$(LOCAL_PATH)/../prebuilt/usr/idc/atmxt-i2c.idc" "$(TARGET_RECOVERY_ROOT_OUT)/vendor/firmware/";)
     $(shell cp -rf "$(LOCAL_PATH)/../prebuilt/usr/idc/atmxt-r2.tdat" "$(TARGET_RECOVERY_ROOT_OUT)/vendor/firmware/";)

endif
