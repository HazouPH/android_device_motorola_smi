LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := at-manager
LOCAL_COPY_HEADERS := \
    ATCommand.h \
    ATManager.h \
    ATCmdStatus.h \
    PeriodicATCommand.h \
    UnsollicitedATCommand.h \
    EventNotifier.h
include $(BUILD_COPY_HEADERS)
include $(CLEAR_VARS)


LOCAL_CFLAGS := \
        -DDEBUG

LOCAL_SRC_FILES := \
        ATManager.cpp \
        ATCommand.cpp \
        PeriodicATCommand.cpp \
        UnsollicitedATCommand.cpp

LOCAL_C_INCLUDES += \
        hardware/intel/rapid_ril/CORE \
        system/core/include/cutils

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../at-parser \
        $(LOCAL_PATH)/../tty-handler \
        $(TARGET_OUT_HEADERS)/event-listener \
        $(TARGET_OUT_HEADERS)/property

LOCAL_C_INCLUDES += \
        external/stlport/stlport/ \
        bionic/libstdc++ \
        bionic/

LOCAL_C_INCLUDES += \
        $(TARGET_OUT_HEADERS)/IFX-modem

LOCAL_SHARED_LIBRARIES := libstlport libcutils libtty-handler libat-parser libevent-listener libproperty

LOCAL_MODULE := libat-manager
LOCAL_MODULE_TAGS := optional

TARGET_ERROR_FLAGS += -Wno-non-virtual-dtor

include $(BUILD_SHARED_LIBRARY)
