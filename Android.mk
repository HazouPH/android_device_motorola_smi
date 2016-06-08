LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_DEVICE),smi)
include $(call all-makefiles-under,$(LOCAL_PATH))

# create symlink compiled source directory in root folder
$(shell rm compiled-source)
$(shell ln -sf out/target/product/$(PRODUCT_DEVICE)/ compiled-source)
endif
