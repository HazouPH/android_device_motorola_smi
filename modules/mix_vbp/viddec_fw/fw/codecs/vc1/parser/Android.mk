LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    mix_vbp_vc1_stubs.c \
    vc1parse_bpic_adv.c \
    vc1parse_huffman.c \
    vc1parse_mv_com.c \
    vc1parse_ppic_adv.c \
    viddec_vc1_parse.c \
    vc1parse_bpic.c \
    vc1parse_common_tables.c \
    vc1parse_ipic_adv.c \
    vc1parse_pic_com_adv.c \
    vc1parse_ppic.c \
    vc1parse_bitplane.c \
    vc1parse.c \
    vc1parse_ipic.c \
    vc1parse_pic_com.c \
    vc1parse_vopdq.c

LOCAL_CFLAGS := -DVBP -DHOST_ONLY

LOCAL_C_INCLUDES := \
    $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/include   \
    $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/include   \
    $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/parser/include   \
    $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/vc1/include

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp_vc1

LOCAL_SHARED_LIBRARIES := \
    libmixvbp

include $(BUILD_SHARED_LIBRARY)
