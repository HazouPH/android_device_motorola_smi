# Copyright (C) 2012 The Android Open Source Project
# Copyright (C) 2012 Hiemanshu Sharma <mail@theindiangeek.in>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# This file is the build configuration for a full Android
# build for Motorola smi hardware. This cleanly combines a set of
# device-specific aspects (drivers) with a device-agnostic
# product configuration (apps). Except for a few implementation
# details, it only fundamentally contains two inherit-product
# lines, full and maserati, hence its name.
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Inherit from smi device
$(call inherit-product, device/motorola/smi/device.mk)

PRODUCT_RUNTIMES := runtime_libart_default

# Boot animation
TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 480	#Real=540 (Fix bootanimation)

# Set those variables here to overwrite the inherited values.
PRODUCT_DEVICE := smi
PRODUCT_NAME := lineage_smi
PRODUCT_BRAND := Motorola
PRODUCT_MODEL := XT890
PRODUCT_RELEASE_NAME := Razr I
PRODUCT_MANUFACTURER := Motorola
