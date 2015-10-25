LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#MIXVBP_LOG_ENABLE := true

LOCAL_SRC_FILES :=			\
	vbp_h264_parser.c		\
	vbp_vc1_parser.c		\
	vbp_loader.c			\
	vbp_mp42_parser.c		\
	vbp_utils.c			\
	viddec_parse_sc.c		\
	viddec_pm_parser_ops.c		\
	viddec_pm_utils_bstream.c	\
	viddec_pm_utils_list.c

LOCAL_CFLAGS := -DVBP -DHOST_ONLY

LOCAL_C_INCLUDES +=			\
	$(LOCAL_PATH)/include		\
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/include		   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/include		   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/h264/include \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/mp2/include  \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/mp4/include  \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/vc1/include  \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/vc1/parser   \
	$(TARGET_OUT_HEADERS)/libva

LOCAL_COPY_HEADERS_TO := libmixvbp

LOCAL_COPY_HEADERS :=	\
	vbp_loader.h

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp

LOCAL_SHARED_LIBRARIES :=		\
	libdl				\
	libcutils

ifeq ($(strip $(MIXVBP_LOG_ENABLE)),true)
LOCAL_CFLAGS += -DVBP_TRACE
LOCAL_SHARED_LIBRARIES += liblog
endif

ifeq ($(USE_HW_VP8),true)
LOCAL_SRC_FILES += vbp_vp8_parser.c
LOCAL_C_INCLUDES += $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/vp8/include
LOCAL_CFLAGS += -DUSE_HW_VP8
endif

PLATFORM_SUPPORT_AVC_SHORT_FORMAT := \
    baytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_SUPPORT_AVC_SHORT_FORMAT)),)
LOCAL_CFLAGS += -DUSE_AVC_SHORT_FORMAT
LOCAL_SRC_FILES += vbp_h264secure_parser.c
endif
include $(BUILD_SHARED_LIBRARY)
