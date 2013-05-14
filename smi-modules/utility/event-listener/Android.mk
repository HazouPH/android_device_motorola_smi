LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := event-listener
LOCAL_COPY_HEADERS := \
    EventListener.h \
    EventThread.h
include $(BUILD_COPY_HEADERS)
include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := event-listener
LOCAL_COPY_HEADERS := \
    EventListener.h \
    EventThread.h
include $(BUILD_COPY_HEADERS)
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -DDEBUG

LOCAL_SRC_FILES := \
        EventThread.cpp

LOCAL_C_INCLUDES += \
        system/core/include/cutils

LOCAL_C_INCLUDES += \
        external/stlport/stlport/ \
        bionic/libstdc++ \
        bionic/

LOCAL_COPY_HEADERS_TO := event-listener

LOCAL_COPY_HEADERS := \
        EventListener.h

LOCAL_SHARED_LIBRARIES := libstlport libcutils

LOCAL_MODULE := libevent-listener
LOCAL_MODULE_TAGS := optional

TARGET_ERROR_FLAGS += -Wno-non-virtual-dtor

include $(BUILD_SHARED_LIBRARY)

