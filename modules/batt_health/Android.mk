LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_BUILD_VARIANT), user)
    LOCAL_CFLAGS += -DLOG_BHD_DISABLE
    LOCAL_CFLAGS += -DLOG_PS_ACTIVATE_LOG_ENABLE_CHECK
endif

LOCAL_SRC_FILES:= \
        dbg.c \
        main.c \
        nvm.c \
        sys.c \
        logger.c

LOCAL_STATIC_LIBRARIES := libcutils liblog

LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE:= batt_health

include $(BUILD_EXECUTABLE)
