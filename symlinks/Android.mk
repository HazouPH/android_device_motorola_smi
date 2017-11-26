LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := wl1271-nvs.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := FAKE

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): TARGET := /data/misc/wifi/$(LOCAL_MODULE)
$(LOCAL_BUILT_MODULE): SYMLINK := $(TARGET_OUT)/etc/firmware/ti-connectivity/$(LOCAL_MODULE)
$(LOCAL_BUILT_MODULE):
	$(hide) echo "Symlink: $(SYMLINK) -> $(TARGET)"
	$(hide) mkdir -p $(dir $@)
	$(hide) mkdir -p $(dir $(SYMLINK))
	$(hide) rm -rf $@
	$(hide) rm -rf $(SYMLINK)
	$(hide) ln -sf $(TARGET) $(SYMLINK)
	$(hide) touch $@
