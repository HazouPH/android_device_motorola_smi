ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        pcm_hook_ctl_voice.c \
        pcm_voice.c

LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/lib/alsa-lib

LOCAL_CFLAGS += -DPIC -UNDEBUG -DDEBUG=1 -DLOG_NDEBUG=1

LOCAL_C_INCLUDES:= \
        external/alsa-lib/include

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libasound

LOCAL_MODULE := libasound_module_pcm_voice
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        ctl_voice.c

LOCAL_MODULE_PATH := $(TARGET_OUT)/usr/lib/alsa-lib

#enable log
LOCAL_CFLAGS += -DPIC -UNDEBUG -DDEBUG=1 -DLOG_NDEBUG=1

LOCAL_C_INCLUDES:= \
        external/alsa-lib/include \
        $(TARGET_OUT_HEADERS)/hw

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libhardware \
        libasound

LOCAL_MODULE := libasound_module_ctl_voice
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
endif
