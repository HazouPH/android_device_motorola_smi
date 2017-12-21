LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE:=
LOCAL_MODULE_CLASS:=SHARED_LIBRARIES
LOCAL_MODULE_TAGS:=optional
OVERRIDE_BUILT_MODULE_PATH:=$(PRODUCT_OUT)/obj/lib
LOCAL_UNINSTALLABLE_MODULE:=
LOCAL_SRC_FILES:=target/libsepdrm.so
LOCAL_BUILT_MODULE_STEM:=libsepdrm.so
LOCAL_STRIP_MODULE:=
LOCAL_MODULE:=libsepdrm
LOCAL_MODULE_STEM:=libsepdrm.so
LOCAL_CERTIFICATE:=
LOCAL_MODULE_PATH:=$(PRODUCT_OUT)/system/lib
LOCAL_REQUIRED_MODULES:=
LOCAL_SHARED_LIBRARIES:=libcutils libc libcrypto libc libstdc++ libm
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS:=include/sepdrm.h include/sepdrm-common.h include/drm_wv_mod_lib_error.h include/drm_wv_mod_lib.h
LOCAL_COPY_HEADERS_TO:=libsepdrm
include $(BUILD_COPY_HEADERS)
