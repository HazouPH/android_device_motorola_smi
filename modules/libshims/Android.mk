LOCAL_PATH := $(call my-dir)

# Audio

include $(CLEAR_VARS)

LOCAL_SRC_FILES := icu51.c mixer.c
LOCAL_SHARED_LIBRARIES := libicuuc libicui18n 
LOCAL_MODULE := libshim_audio
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# Camera

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    gui/SensorManager.cpp \
    ui/GraphicBufferAllocator.cpp \
    ui/GraphicBuffer.cpp \
    ui/GraphicBufferMapper.cpp \
    atomic.c \
    #surface-control.cpp

LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware libui libgui libbinder libutils libsync libsensor
LOCAL_MODULE := libshim_camera
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)

# MMGR

include $(CLEAR_VARS)

LOCAL_SRC_FILES := icu51.c
LOCAL_SHARED_LIBRARIES := libicuuc libicui18n
LOCAL_MODULE := libshim_mmgr
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# Crypto

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
     crypto_malloc.c

LOCAL_MODULE := libshim_crypto
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

# atomic

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
     atomic.c

LOCAL_MODULE := libshim_atomic
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
