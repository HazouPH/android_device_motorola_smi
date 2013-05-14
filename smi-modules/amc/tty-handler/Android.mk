LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -DDEBUG

LOCAL_SRC_FILES := \
        TtyHandler.cpp

LOCAL_C_INCLUDES += \
        system/core/include/cutils

LOCAL_SHARED_LIBRARIES := libcutils

LOCAL_MODULE := libtty-handler
LOCAL_MODULE_TAGS := optional

TARGET_ERROR_FLAGS += -Wno-non-virtual-dtor

include $(BUILD_SHARED_LIBRARY)

