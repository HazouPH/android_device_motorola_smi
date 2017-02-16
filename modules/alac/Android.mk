ifeq ($(USE_FEATURE_ALAC),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -O3 -msse3 -mfpmath=sse

LOCAL_SRC_FILES := \
        SoftALAC.cpp \
        ALACBitUtilities.c \
        ALACEngine.cpp \
        dp_dec.c \
        EndianPortable.c \
        matrix_dec.c \
        ag_dec.c

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
        frameworks/native/include/media/openmax \
        frameworks/av/media/libstagefright/omx \
        $(LOCAL_PATH) 

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils liblog

LOCAL_MODULE := libstagefright_soft_alacdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif
