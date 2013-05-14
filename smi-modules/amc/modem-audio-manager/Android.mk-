LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := modem-audio-manager
LOCAL_COPY_HEADERS := \
    ModemAudioManager.h \
    ModemStatusNotifier.h \
    ModemAudioManagerInstance.h \
    DualSimModemAudioManager.h
include $(BUILD_COPY_HEADERS)
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -DDEBUG

LOCAL_SRC_FILES := \
        ModemAudioManager.cpp \
        DualSimModemAudioManager.cpp \
        ModemAudioManagerInstance.cpp

LOCAL_C_INCLUDES += \
        hardware/intel/rapid_ril/CORE \
        system/core/include/cutils \
        $(TARGET_OUT_HEADERS)/IFX-modem

LOCAL_C_INCLUDES += \
        $(TARGET_OUT_HEADERS)/audio-at-manager \
        $(TARGET_OUT_HEADERS)/at-manager \
        $(TARGET_OUT_HEADERS)/event-listener \
        $(TARGET_OUT_HEADERS)/property

LOCAL_C_INCLUDES += \
        external/stlport/stlport/ \
        bionic/libstdc++ \
        bionic/

LOCAL_SHARED_LIBRARIES := libstlport libcutils libaudio-at-manager libat-manager libevent-listener libproperty

LOCAL_MODULE := libmodem-audio-manager
LOCAL_MODULE_TAGS := optional

TARGET_ERROR_FLAGS += -Wno-non-virtual-dtor

include $(BUILD_SHARED_LIBRARY)
