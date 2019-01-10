#
# Copyright 2010 Intrinsyc Software International, Inc.  All rights reserved.
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    rillog.cpp \
    extract.cpp \
    util.cpp \
    repository.cpp \
    base64.c

LOCAL_SHARED_LIBRARIES := \
    libutils libcutils libmmgrcli liblog

LOCAL_C_INCLUDES :=  \
    $(LOCAL_PATH)/../../INC \
    $(LOCAL_PATH)/../../CORE \
    $(LOCAL_PATH)/../../CORE/ND \
    $(LOCAL_PATH)/../../OEM/ND \
    $(TARGET_OUT_HEADERS)/IFX-modem \

#build shared library
LOCAL_PRELINK_MODULE := false
LOCAL_STRIP_MODULE := true
LOCAL_CFLAGS += -DRIL_SHLIB -Os
LOCAL_MODULE:= librapid-ril-util
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS:= optional
include $(BUILD_SHARED_LIBRARY)

