LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := property
LOCAL_COPY_HEADERS := \
    PropertyBase.h \
    Property.h
include $(BUILD_COPY_HEADERS)
include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := property
LOCAL_COPY_HEADERS := \
    Property.h \
    PropertyBase.h
include $(BUILD_COPY_HEADERS)
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -DDEBUG

LOCAL_SRC_FILES := \
        PropertyBase.cpp \
        Property.cpp

LOCAL_C_INCLUDES += \
        system/core/include/cutils

LOCAL_C_INCLUDES += \
        external/stlport/stlport/ \
        bionic/libstdc++ \
        bionic/

LOCAL_SHARED_LIBRARIES := libstlport libcutils

LOCAL_MODULE := libproperty
LOCAL_MODULE_TAGS := optional

TARGET_ERROR_FLAGS += -Wno-non-virtual-dtor

include $(BUILD_SHARED_LIBRARY)
