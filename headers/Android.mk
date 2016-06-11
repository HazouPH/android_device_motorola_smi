LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS := \
    hal_public.h
LOCAL_COPY_HEADERS_TO := pvr/hal
include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS := \
    ../modules/pcg/libpcg.h
LOCAL_COPY_HEADERS_TO := pcg
include $(BUILD_COPY_HEADERS)
