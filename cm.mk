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

# For userdebug builds
ADDITIONAL_DEFAULT_PROPERTIES += \
    	keyguard.no_require_sim=false \
    	ro.sf.lcd_density=240 \
   	panel.physicalWidthmm=52 \
    	panel.physicalHeightmm=89 \
    	ro.opengles.version=131072 \
    	gsm.net.interface=rmnet0 \
    	persist.system.at-proxy.mode=0
    	ro.secure=0 \
    	ro.allow.mock.location=0 \
    	ro.debuggable=1 \
    	persist.sys.usb.config=adb \
    	persist.ril-daemon.disable=0

ADDITIONAL_DEFAULT_PROPERTIES  += wifi.interface=wlan0:0

#
# Setup device specific product configuration.
#
PRODUCT_NAME := cm_smi
PRODUCT_DEVICE := smi
PRODUCT_BRAND := Motorola
PRODUCT_MODEL := XT890
PRODUCT_MANUFACTURER := Motorola
PRODUCT_RELEASE_NAME := Razr i
