LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_BOARD_PLATFORM),sc1)
include $(call all-makefiles-under,$(LOCAL_PATH))
include $(call all-subdir-makefiles,$(LOCAL_PATH))
# include x86 encoder (apache-harmony (intel))
include dalvik/vm/compiler/codegen/x86/libenc/Android.mk
ifeq ($(wildcard ../../../patches/Android.mk),)
$(info Motorola Razr I patch directory exist, hopefully applied)
include patches/Android.mk
else
$(warning Patch directory for Motorola razr I does not exist)
endif
endif

# create symlink compiled source directory in root folder
$(shell ln -sf out/target/product/smi/ compiled-source)
