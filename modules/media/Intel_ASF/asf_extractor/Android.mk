ifeq ($(strip $(USE_INTEL_ASF_EXTRACTOR)),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES := \
    AsfExtractor.cpp \
    MediaBufferPool.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(TARGET_OUT_HEADERS)/libmix_asfparser \
    $(call include-path-for, libstagefright) \
    $(call include-path-for, frameworks-native)/media/openmax \
    external/stlport/stlport \
    bionic

LOCAL_COPY_HEADERS_TO := libmix_asf_extractor

LOCAL_COPY_HEADERS := \
    AsfExtractor.h \
    MetaDataExt.h \
    MediaBufferPool.h

LOCAL_CPPFLAGS += -DUSE_INTEL_ASF_EXTRACTOR
LOCAL_MODULE := libasfextractor
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)


endif
