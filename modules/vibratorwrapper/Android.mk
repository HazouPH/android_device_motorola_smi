LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    VibratorWrapper.cpp

LOCAL_SHARED_LIBRARIES := \
    libhardware liblog libutils

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := vibrator.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    vibratest.c

LOCAL_SHARED_LIBRARIES := \
    libhardware liblog libutils

LOCAL_MODULE := vibratest
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
