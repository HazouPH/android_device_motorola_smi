LOCAL_PATH := $(call my-dir)
include $(call all-subdir-makefiles,$(LOCAL_PATH))

NVS_FILE = wl1271-nvs.bin
FAC_NVS_FILE = nvs_map_mac80211.bin
FAC_NVS_FILE_SYM = wl12xx-fac-nvs.bin

FACSYMLINKS := $(addprefix $(PRODUCT_OUT)/system/etc/firmware/ti-connectivity/,$(FAC_NVS_FILE_SYM))
$(FACSYMLINKS) : $(LOCAL_INSTALLED_MODULE) $(LOCAL_PATH)/Android.mk
	@echo "Symlink: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(addprefix /pds/wifi/,$(FAC_NVS_FILE)) $@

SYMLINKS := $(addprefix $(PRODUCT_OUT)/system/etc/firmware/ti-connectivity/,$(NVS_FILE))
$(SYMLINKS): $(LOCAL_INSTALLED_MODULE) $(LOCAL_PATH)/Android.mk
	@echo "Symlink: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(addprefix /data/misc/firmware/ti-connectivity/,$(NVS_FILE)) $@

ALL_DEFAULT_INSTALLED_MODULES += $(FACSYMLINKS) $(SYMLINKS)
ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
	$(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(FACSYMLINKS) $(SYMLINKS)
