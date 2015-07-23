LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES :=              \
    AsfStreamParser.cpp         \
    AsfDataParser.cpp           \
    AsfHeaderParser.cpp         \
    AsfIndexParser.cpp          \
    AsfGuids.cpp


LOCAL_C_INCLUDES :=             \
    $(LOCAL_PATH)               \
    bionic                      \
    $(call include-path-for, stlport) \


LOCAL_SHARED_LIBRARIES :=       \
     libutils  libcutils liblog libstlport

LOCAL_COPY_HEADERS_TO  := libmix_asfparser

LOCAL_COPY_HEADERS := \
    AsfParserDefs.h \
    AsfStreamParser.h \
    AsfObjects.h \
    AsfGuids.h  \

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libasfparser

include $(BUILD_SHARED_LIBRARY)
