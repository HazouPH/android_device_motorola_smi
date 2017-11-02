# Inherit some common Lineage stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

PRODUCT_RUNTIMES := runtime_libart_default

# Boot animation
TARGET_SCREEN_HEIGHT := 960
TARGET_SCREEN_WIDTH := 480	#Real=540 (Fix bootanimation)

# Release name
PRODUCT_RELEASE_NAME := Razr I
PRODUCT_NAME := lineage_smi

# Inherit from smi device
$(call inherit-product, device/motorola/smi/full_smi.mk)

PRODUCT_BUILD_PROP_OVERRIDES += \
    PRODUCT_BRAND=motorola \
    PRODUCT_NAME=XT890 \
    BUILD_PRODUCT=smi


