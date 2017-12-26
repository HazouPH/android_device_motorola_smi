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

LOCAL_PATH := device/motorola/smi

$(call inherit-product, $(LOCAL_PATH)/configs/phone-hdpi-1024-dalvik-heap.mk)

# xt890 specific overlay
DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# Device uses high-density artwork where available
PRODUCT_LOCALES := en_US
PRODUCT_LOCALES += hdpi
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := hdpi
PRODUCT_AAPT_PREBUILT_DPI := hdpi tvdpi mdpi ldpi

# Art
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.dex2oat-swap=false

# Audio
PRODUCT_PACKAGES += \
    audio.primary.smi \
    audio.a2dp.default \
    audio.r_submix.default \
    libaudioutils \
    libtinyalsa \
    libremote-processor \
    remote-process

# MM Compability
PRODUCT_PACKAGES += \
    libshim_audio \
    libshim_camera \
    libshim_mmgr \
    libshim_crypto

# Doze
PRODUCT_PACKAGES += \
   CMActions

# Radio
PRODUCT_PACKAGES += \
    Stk \
    librapid-ril-core \
    librapid-ril-util
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/etc/telephony/repository.txt:system/etc/telephony/repository.txt
PRODUCT_PROPERTY_OVERRIDES += \
    keyguard.no_require_sim=true \
    ro.ril.status.polling.enable=0 \
    ro.telephony.default_network=3 \
    ro.ril.telephony.mqanelements=5 \
    rild.libpath=/system/lib/librapid-ril-core.so

# Intel Display
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=240

# Ramdisk
PRODUCT_PACKAGES += \
    fstab.sc1 \
    init.avc.rc \
    init.bt.rc \
    init.bt.vendor.rc \
    init.common.rc \
    init.debug.rc \
    init.gps.rc \
    init.modem.rc \
    init.moto.usb.rc \
    init.nfc.rc \
    init.oom.rc \
    init.sc1.rc \
    init.watchdog.rc \
    init.wifi.rc \
    init.wifi.vendor.rc \
    init.xmm.rc \
    ueventd.sc1.rc \
    init.recovery.sc1.rc \
    init.zram.rc \
    init.ksm.rc

# PDS
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/bin/pdsbackup.sh:system/bin/pdsbackup.sh

# Camera
PRODUCT_PACKAGES += \
    camera.sc1 \
    libintelmetadatabuffer \
    Snap

# Newer camera API isn't supported.
PRODUCT_PROPERTY_OVERRIDES += \
    camera2.portability.force_api=1 \
    media.stagefright.legacyencoder=true \
    media.stagefright.less-secure=true

# Power
PRODUCT_PACKAGES += \
    power.smi

# Lights
PRODUCT_PACKAGES += \
    lights.smi

# psb video
PRODUCT_PACKAGES += \
    msvdx_bin \
    msvdx_fw_mfld_DE2.0.bin \
    topazsc_bin \
    topazsc_fw.bin\
    pvr_drv_video

# libva
PRODUCT_PACKAGES += \
    libva \
    libva-android \
    libva-tpi

# libstagefrighthw
PRODUCT_PACKAGES += \
    libstagefrighthw

# libmix
PRODUCT_PACKAGES += \
    libmixvbp_mpeg4 \
    libmixvbp_h264 \
    libmixvbp_vc1 \
    libmixvbp \
    libva_videodecoder \
    libva_videoencoder

# HW acceleration
PRODUCT_PACKAGES += \
    libwrs_omxil_common \
    libwrs_omxil_core_pvwrapped \
    libOMXVideoDecoderAVC \
    libOMXVideoDecoderH263 \
    libOMXVideoDecoderMPEG4 \
    libOMXVideoDecoderPAVC \
    libOMXVideoDecoderWMV \
    libOMXVideoEncoderAVC \
    libOMXVideoEncoderH263 \
    libOMXVideoEncoderMPEG4
#    libOMXVideoDecoderAVCSecure \

# libwsbm
PRODUCT_PACKAGES += \
    libwsbm

# libdrm
PRODUCT_PACKAGES += \
    libdrm

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/vendor/lib/mediadrm/libwvdrmengine.so:system/vendor/lib/mediadrm/libwvdrmengine.so

# pvrsrvctl
PRODUCT_PACKAGES += \
    pvrsrvctl

# Motorola AGPS
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/framework/com.motorola.android.location.jar:system/framework/com.motorola.android.location.jar
PRODUCT_PROPERTY_OVERRIDES += \
    persist.motoagps.enable=true

# Vibrator
PRODUCT_PACKAGES += \
    vibrator.sc1

# Misc
PRODUCT_PACKAGES += \
    curl \
    libbson \
    libcurl \
    tcpdump \
    libcorkscrew \
    imgdiff \
    com.android.future.usb.accessory

# Houdini (arm native bridge)
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,$(LOCAL_PATH)/houdini/system,system)

# Houdini (arm native bridge)
PRODUCT_PROPERTY_OVERRIDES += \
    ro.enable.native.bridge.exec=1

ADDITIONAL_DEFAULT_PROPERTIES += \
    ro.dalvik.vm.native.bridge=libhoudini.so

# Dalvik
PRODUCT_PROPERTY_OVERRIDES += \
    ro.dalvik.vm.isa.arm=x86 \
    dalvik.vm.implicit_checks=none

# Appends path to ARM libs for Houdini
PRODUCT_LIBRARY_PATH := $(PRODUCT_LIBRARY_PATH):/system/lib/arm:/system/lib/arm/nb

# stlport required for our LP blobs
PRODUCT_PACKAGES += \
    libstlport

# Wifi
PRODUCT_PACKAGES += \
    lib_driver_cmd_wl12xx \
    calibrator \
    libwpa_client \
    hostapd \
    wpa_supplicant \
    hostapd.conf \
    p2p_supplicant.conf \
    wpa_supplicant.conf \
    wl128x-fw-4-sr.bin \
    wl128x-fw-4-mr.bin \
    wl128x-fw-4-plt.bin \
    wl1271-nvs_128x.bin \
    wl1271-nvs.bin
PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0 \
    softap.interface=wlan0 \
    wifi.ap.interface=wlan0 \
    persist.wlan.ti.calibrated=0 \
    ro.disableWifiApFirmwareReload=true
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/bin/wifical.sh:system/bin/wifical.sh \
    $(LOCAL_PATH)/prebuilt/bin/wificalcheck.sh:system/bin/wificalcheck.sh \
    $(LOCAL_PATH)/prebuilt/etc/wifi/p2p_supplicant_overlay.conf:system/etc/wifi/p2p_supplicant_overlay.conf \
    $(LOCAL_PATH)/prebuilt/etc/wifi/wpa_supplicant_overlay.conf:system/etc/wifi/wpa_supplicant_overlay.conf \
    $(LOCAL_PATH)/prebuilt/etc/wifi/TQS.ini:system/etc/wifi/TQS.ini

# Misc
ADDITIONAL_DEFAULT_PROPERTIES += \
    panel.physicalWidthmm=52 \
    panel.physicalHeightmm=89 \
    ro.opengles.version=131072 \
    gsm.net.interface=rmnet0 \
    ro.secure=0 \
    ro.adb.secure=0 \
    ro.debuggable=1

# Storage
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sys.sdcardfs=true

# Fix SELinux execmod denials
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/bin/execmod-wrapper.sh:system/bin/akmd8963.sh \
    $(LOCAL_PATH)/prebuilt/bin/execmod-wrapper.sh:system/bin/batt_health.sh \
    $(LOCAL_PATH)/prebuilt/bin/execmod-wrapper.sh:system/bin/bd_prov.sh \
    $(LOCAL_PATH)/prebuilt/bin/execmod-wrapper.sh:system/bin/pvrsrvctl.sh \
    $(LOCAL_PATH)/prebuilt/bin/execmod-wrapper.sh:system/bin/gps_driver.sh \
    $(LOCAL_PATH)/prebuilt/bin/execmod-wrapper.sh:system/bin/mmgr.sh

# FS config
PRODUCT_PACKAGES += \
    fs_config_files

# SGX540 is slower with the scissor optimization enabled
# Avoids retrying for an EGL config w/o EGL_SWAP_BEHAVIOR_PRESERVED
# We don't support eglSwapBuffersWithDamageKHR; this avoids some unnecessary code.
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hwui.disable_scissor_opt=true \
    debug.hwui.render_dirty_regions=false \
    debug.hwui.swap_with_damage=false \
    debug.hwui.use_buffer_age=false

# Disable Atlas
PRODUCT_PROPERTY_OVERRIDES += \
    config.disable_atlas=true

# IDC
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/usr/idc/atmxt-i2c.idc:system/usr/idc/atmxt-i2c.idc \
    $(LOCAL_PATH)/prebuilt/usr/idc/mxt224_touchscreen_0.idc:system/usr/idc/mxt224_touchscreen_0.idc

# Keylayout (mapping)
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/usr/keylayout/medfield_audio_Intel_R__MID_Audio_Jack.kl:system/usr/keylayout/medfield_audio_Intel_R__MID_Audio_Jack.kl

# MediaProfile and Audio
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/etc/audio_policy.conf:system/etc/audio_policy.conf \
    $(LOCAL_PATH)/prebuilt/etc/media_profiles.xml:system/etc/media_profiles.xml \
    $(LOCAL_PATH)/prebuilt/etc/media_codecs.xml:system/etc/media_codecs.xml \
    $(LOCAL_PATH)/prebuilt/etc/wrs_omxil_components.list:system/etc/wrs_omxil_components.list \
    $(LOCAL_PATH)/prebuilt/etc/parameter-framework/Settings/Audio/AudioConfigurableDomains.xml:system/etc/parameter-framework/Settings/Audio/AudioConfigurableDomains.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:system/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:system/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video.xml:system/etc/media_codecs_google_video.xml

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml

# Device specific permissions
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/etc/permissions/com.motorola.android.drmcommonconfig.xml:system/etc/permissions/com.motorola.android.drmcommonconfig.xml

PRODUCT_GMS_CLIENTID_BASE ?= android-motorola

$(call inherit-product, vendor/motorola/smi/smi-vendor.mk)

# FM Radio
#PRODUCT_PACKAGES += \
    kfmapp \
    FmRxApp \
    FmTxApp \
    FmService \
    libfmradio \
    fmradioif \
    com.ti.fm.fmradioif.xml

# RazrIChargeCurrent (Author: NerdyProjects/PosixCompatible)
PRODUCT_PACKAGES += \
    RazrIChargeCurrent

# Nfc
$(call inherit-product, $(LOCAL_PATH)/modules/nfc/nfc.mk)

# Shared Transport (BLUETOOTH,FM,GPS)
#$(call inherit-product-if-exists, hardware/ti/wpan/ti-wpan-products.mk)
