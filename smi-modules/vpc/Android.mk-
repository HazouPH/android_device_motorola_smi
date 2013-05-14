ifeq ($(BOARD_USES_ALSA_AUDIO),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_COPY_HEADERS_TO := hw

LOCAL_COPY_HEADERS := \
    vpc_hardware.h

LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar

ifeq ($(CUSTOM_BOARD),mfld_pr2)
	LOCAL_CFLAGS += \
		-DHAL_VPC_PLUS_6DB_MODEM_UL
endif

ifeq ($(TARGET_DEVICE),smi)
	LOCAL_CFLAGS += \
		-DHAL_VPC_PLUS_6DB_MODEM_UL
endif

#enable TTY hadnling for LATAM products only (ADR && Claro-PR)
ifneq (,$(filter $(TARGET_PRODUCT),XT890_amxpr XT890_adrpr XT890_adrbr))
       LOCAL_CFLAGS += \
               -DENABLE_TTY_PROFILE
endif

LOCAL_SRC_FILES:= \
     ctl_vpc.cpp \
     bt.cpp \
     msic.cpp \
     volume_keys.cpp

LOCAL_SHARED_LIBRARIES := \
     libasound \
     liblog \
     libcutils \
     libamc \
     libbinder \
     libutils \
     libbluetooth-audio

ifeq ($(BOARD_HAVE_BLUETOOTH),false)
LOCAL_CFLAGS += -DBTDISABLED
endif

ifeq ($(BOARD_HAVE_AUDIENCE),true)
    LOCAL_CFLAGS += -DCUSTOM_BOARD_WITH_AUDIENCE
    LOCAL_SRC_FILES += acoustic.cpp
    LOCAL_C_INCLUDES += \
        $(TARGET_OUT_HEADERS)/property \
        external/stlport/stlport/ \
        bionic/libstdc++ \
        bionic
    LOCAL_SHARED_LIBRARIES += \
        libstlport \
        libproperty
endif

ifeq ($(CUSTOM_BOARD),ctp_pr0)
     LOCAL_CFLAGS += -DCUSTOM_BOARD_WITH_VOICE_CODEC_SLAVE
endif

ifeq ($(CUSTOM_BOARD),ctp_pr1)
     LOCAL_CFLAGS += -DCUSTOM_BOARD_WITH_VOICE_CODEC_SLAVE
endif

ifeq ($(CUSTOM_BOARD),ctp_pr2)
     LOCAL_CFLAGS += -DCUSTOM_BOARD_WITH_VOICE_CODEC_SLAVE
endif

ifneq ($(ALSA_DEFAULT_SAMPLE_RATE),)
    LOCAL_CFLAGS += -DALSA_DEFAULT_SAMPLE_RATE=$(ALSA_DEFAULT_SAMPLE_RATE)
endif

LOCAL_C_INCLUDES += \
     external/alsa-lib/include \
     hardware/intel/include \
     system/core/include/cutils \
     $(TARGET_OUT_HEADERS)/vpc \
     $(TARGET_OUT_HEADERS)/libamc \
     $(TARGET_OUT_HEADERS)/at-manager \
     $(TARGET_OUT_HEADERS)/IFX-modem \
     hardware/intel/mfld_cdk

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
LOCAL_C_INCLUDES += \
     external/bluetooth/bluez/lib/bluetooth \
     $(TARGET_OUT_HEADERS)/libbluetooth_vs \
     hardware/intel/mfld_cdk/MediaBTService
endif


LOCAL_MODULE:= vpc.$(TARGET_DEVICE)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


endif

