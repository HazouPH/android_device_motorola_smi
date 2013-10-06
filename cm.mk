# Inherit from smi device
$(call inherit-product, device/motorola/smi/full_smi.mk)

# Boot animation
TARGET_BOOTANIMATION_NAME := vertical-540x960
TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 540

# Specify phone tech before including full_phone
$(call inherit-product, vendor/cm/config/gsm.mk)

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit enhanced nfc config 
$(call inherit-product, vendor/cm/config/nfc_enhanced.mk)

DEVICE_PACKAGE_OVERLAYS += device/motorola/smi/overlay

#
# Setup device specific product configuration.
#
PRODUCT_NAME := cm_smi
PRODUCT_DEVICE := smi
PRODUCT_BRAND := Motorola
PRODUCT_MODEL := XT890
PRODUCT_MANUFACTURER := Motorola
PRODUCT_RELEASE_NAME := Razr i
