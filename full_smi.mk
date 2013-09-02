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

PRODUCT_PACKAGES := \
    Camera \
    Gallery2

PRODUCT_COPY_FILES += \
    device/sample/etc/apns-full-conf.xml:system/etc/apns-conf.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# For userdebug builds
ADDITIONAL_DEFAULT_PROPERTIES += \
    	keyguard.no_require_sim=false \
    	ro.sf.lcd_density=240 \
   	panel.physicalWidthmm=52 \
    	panel.physicalHeightmm=89 \
    	ro.opengles.version=131072 \
    	gsm.net.interface=rmnet0 \
    	persist.system.at-proxy.mode=0
    	ro.secure=0 \
    	ro.allow.mock.location=0 \
    	ro.debuggable=1 \
    	persist.sys.usb.config=adb \
    	persist.ril-daemon.disable=0

ADDITIONAL_DEFAULT_PROPERTIES  += wifi.interface=wlan0:0

# Inherit from smi device
$(call inherit-product, device/motorola/smi/device.mk)

# Set those variables here to overwrite the inherited values.
PRODUCT_NAME := smi
PRODUCT_DEVICE := smi
PRODUCT_BRAND := Motorola
PRODUCT_MODEL := XT890
PRODUCT_MANUFACTURER := Motorola
PRODUCT_RELEASE_NAME := Razr i
