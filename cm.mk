# Specify phone tech before including full_phone
$(call inherit-product, vendor/cm/config/gsm.mk)

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit enhanced nfc config 
$(call inherit-product, vendor/cm/config/nfc_enhanced.mk)

# Boot animation
TARGET_BOOTANIMATION_NAME := vertical-540x960
TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 540

# Release name
PRODUCT_RELEASE_NAME := Razr I
PRODUCT_NAME := cm_smi

# Inherit from smi device
$(call inherit-product, device/motorola/smi/full_smi.mk)

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_BRAND=motorola \
    PRODUCT_NAME=XT890 \
    BUILD_PRODUCT=smi \
    BUILD_FINGERPRINT=motorola/XT890/smi:4.1.2/9.8.1Q-66/28:user/release-keys \


