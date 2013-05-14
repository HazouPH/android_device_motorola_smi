LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)

LOCAL_COPY_HEADERS := \
        BluetoothAudio.h

LOCAL_SRC_FILES:= \
        BluetoothAudio.cpp

LOCAL_MODULE:= libbluetooth-audio

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := liblog

include $(BUILD_SHARED_LIBRARY)
