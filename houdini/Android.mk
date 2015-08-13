# file: vendor/intel/PREBUILT/SG/Android.mk
#
# This makefile is to generate the final version of the source
# ISA libraries. It only does so if the prebuilt ARM libraries
# were already copied to the libs_prebuilt folder. Otherwise it
# does nothing.
#

LOCAL_PATH := $(call my-dir)

# Houdini hook libraries for different module
include $(CLEAR_VARS)
LOCAL_MODULE := libhoudini_hook
LOCAL_SRC_FILES := hooks/libhoudini_hook.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_C_INCLUDES :=
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := houdini_hook
LOCAL_SRC_FILES := hooks/houdini_hook.c
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_C_INCLUDES :=
include $(BUILD_STATIC_LIBRARY)
