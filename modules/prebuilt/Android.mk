LOCAL_PATH := $(call my-dir)

PREBUILT_DIR := vendor/motorola/smi/proprietary

include $(CLEAR_VARS)
LOCAL_MODULE := libtinyalsa
LOCAL_SRC_FILES := $(PREBUILT_DIR)/lib/libtinyalsa.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libintelmetadatabuffer
LOCAL_SRC_FILES := $(PREBUILT_DIR)/lib/libintelmetadatabuffer.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
