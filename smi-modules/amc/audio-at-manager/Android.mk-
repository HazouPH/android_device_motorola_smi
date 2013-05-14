LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := audio-at-manager
LOCAL_COPY_HEADERS := \
    AudioATManager.h \
    ModemAudioEvent.h \
    CallStatUnsollicitedATCommand.h \
    ProgressUnsollicitedATCommand.h \
    XDRVIUnsollicitedATCommand.h \
    AudioATModemTypes.h \
    Tokenizer.h
include $(BUILD_COPY_HEADERS)
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -DDEBUG

LOCAL_SRC_FILES := \
        AudioATManager.cpp \
        Tokenizer.cpp \
        CallStatUnsollicitedATCommand.cpp \
        ProgressUnsollicitedATCommand.cpp \
        XDRVIUnsollicitedATCommand.cpp

LOCAL_C_INCLUDES += \
        $(TARGET_OUT_HEADERS)/event-listener \
        $(TARGET_OUT_HEADERS)/at-manager \
        system/core/include/cutils \
        $(TARGET_OUT_HEADERS)/IFX-modem

LOCAL_C_INCLUDES += \
        external/stlport/stlport/ \
        bionic/libstdc++ \
        bionic/

LOCAL_SHARED_LIBRARIES := libstlport libcutils libat-manager

LOCAL_MODULE := libaudio-at-manager
LOCAL_MODULE_TAGS := optional

TARGET_ERROR_FLAGS += -Wno-non-virtual-dtor

include $(BUILD_SHARED_LIBRARY)
