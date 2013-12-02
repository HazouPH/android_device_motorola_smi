LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_BOARD_PLATFORM),sc1)
include $(call all-makefiles-under,$(LOCAL_PATH))
include $(call all-subdir-makefiles,$(LOCAL_PATH))
# include x86 encoder (apache-harmony (intel))
include $(TOP)/dalvik/vm/compiler/codegen/x86/libenc/Android.mk
endif

# create symlink compiled source directory in root folder
$(shell cd ../../../)
$(shell ln -s out/target/product/smi/ compiled-source)
