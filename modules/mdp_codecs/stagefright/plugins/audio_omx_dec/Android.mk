LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE:=
LOCAL_MODULE_CLASS:=SHARED_LIBRARIES
LOCAL_MODULE_TAGS:=optional
OVERRIDE_BUILT_MODULE_PATH:=$(PRODUCT_OUT)/obj/lib
LOCAL_UNINSTALLABLE_MODULE:=
LOCAL_SRC_FILES:=target/libstagefright_soft_aacdec_mdp.so
LOCAL_BUILT_MODULE_STEM:=libstagefright_soft_aacdec_mdp.so
LOCAL_STRIP_MODULE:=
LOCAL_MODULE:=libstagefright_soft_aacdec_mdp
LOCAL_MODULE_STEM:=libstagefright_soft_aacdec_mdp.so
LOCAL_CERTIFICATE:=
LOCAL_MODULE_PATH:=$(PRODUCT_OUT)/system/lib
LOCAL_REQUIRED_MODULES:=
LOCAL_SHARED_LIBRARIES:=libstagefright libstagefright_omx libstagefright_foundation libutils liblog libc libstdc++ libm
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE:=
LOCAL_MODULE_CLASS:=SHARED_LIBRARIES
LOCAL_MODULE_TAGS:=optional
OVERRIDE_BUILT_MODULE_PATH:=$(PRODUCT_OUT)/obj/lib
LOCAL_UNINSTALLABLE_MODULE:=
LOCAL_SRC_FILES:=target/libstagefright_soft_mp3dec_mdp.so
LOCAL_BUILT_MODULE_STEM:=libstagefright_soft_mp3dec_mdp.so
LOCAL_STRIP_MODULE:=
LOCAL_MODULE:=libstagefright_soft_mp3dec_mdp
LOCAL_MODULE_STEM:=libstagefright_soft_mp3dec_mdp.so
LOCAL_CERTIFICATE:=
LOCAL_MODULE_PATH:=$(PRODUCT_OUT)/system/lib
LOCAL_REQUIRED_MODULES:=
LOCAL_SHARED_LIBRARIES:=libstagefright libstagefright_omx libstagefright_foundation libutils liblog libc libstdc++ libm
include $(BUILD_PREBUILT)
