# This is a custom bootimage target file to be used on CM
# Set these variables on your BoardConfig.mk
#
# DEVICE_BASE_BOOT_IMAGE := path/to/stuff/existingboot.img
# DEVICE_BASE_RECOVERY_IMAGE := path/to/stuff/existingrecovery.img
# BOARD_CUSTOM_BOOTIMG_MK := path/to/intel-boot-tools/boot.mk
#

INTEL_PACK := $(HOST_OUT_EXECUTABLES)/pack_intel
BASE_BOOT_IMAGE := $(DEVICE_BASE_BOOT_IMAGE)
BASE_RECOVERY_IMAGE := $(DEVICE_BASE_RECOVERY_IMAGE)
CMDLINE_FILE := $(INSTALLED_BOOTIMAGE_TARGET)_cmdline
LZMA_BIN := $(shell which lzma)

$(INSTALLED_RECOVERYIMAGE_TARGET): $(recovery_ramdisk) \
		$(recovery_kernel) \
		$(INTEL_PACK)
ifeq ($(WITH_TWRP),true)
	@echo ----- Compressing recovery ramdisk with lzma ------
	rm -f $(recovery_uncompressed_ramdisk).lzma
	$(LZMA_BIN) $(recovery_uncompressed_ramdisk)
	$(hide) cp $(recovery_uncompressed_ramdisk).lzma $(recovery_ramdisk)
endif
	$(call pretty,"Target recovery image: $@")
	$(shell echo $(BOARD_KERNEL_CMDLINE) > $(CMDLINE_FILE))
	$(INTEL_PACK) $(BASE_RECOVERY_IMAGE) $(INSTALLED_KERNEL_TARGET) $(recovery_ramdisk)  $(CMDLINE_FILE) $@
	@echo -e ${CL_CYN}"Made recovery image: $@"${CL_RST}
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)

$(INSTALLED_BOOTIMAGE_TARGET): $(INSTALLED_RAMDISK_TARGET) \
		$(INSTALLED_KERNEL_TARGET) \
		$(INTEL_PACK)
	$(call pretty,"Target boot image: $@")
	$(shell echo $(BOARD_KERNEL_CMDLINE) > $(CMDLINE_FILE))
	$(INTEL_PACK) $(BASE_BOOT_IMAGE) $(INSTALLED_KERNEL_TARGET) $(INSTALLED_RAMDISK_TARGET) $(CMDLINE_FILE) $@
	@echo -e ${CL_CYN}"Made boot image: $@"${CL_RST}
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
