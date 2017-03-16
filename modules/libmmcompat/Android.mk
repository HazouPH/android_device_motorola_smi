LOCAL_PATH := $(call my-dir)

# sensors

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    gui/SensorManager.cpp \
    ui/GraphicBufferAllocator.cpp \
    ui/GraphicBuffer.cpp \
    ui/GraphicBufferMapper.cpp \
    surface-control.cpp \
    crypto_malloc.c \
    icu51.c

LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware libui libgui libbinder libutils libicuuc libicui18n libsync
LOCAL_MODULE := libmmcompat
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
