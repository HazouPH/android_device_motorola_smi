ifeq ($(TARGET_PROVIDES_CAMERA_HAL),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    system/media/camera/include \
    system/core/base/include

LOCAL_HEADER_LIBRARIES := \
    libnativebase_headers

LOCAL_SRC_FILES := \
    CameraWrapper.cpp

LOCAL_STATIC_LIBRARIES := \
    libarect

LOCAL_SHARED_LIBRARIES := \
    libhardware liblog libcamera_client libutils \
    android.hidl.token@1.0-utils \
    android.hardware.graphics.bufferqueue@1.0

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif
