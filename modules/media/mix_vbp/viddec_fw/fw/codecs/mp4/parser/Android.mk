LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=                           \
	viddec_mp4_visualobject.c            \
	viddec_mp4_decodevideoobjectplane.c  \
	viddec_mp4_parse.c                   \
	viddec_fw_mp4_workload.c             \
	viddec_mp4_videoobjectplane.c        \
	viddec_parse_sc_mp4.c                \
	viddec_mp4_shortheader.c             \
	viddec_mp4_videoobjectlayer.c

LOCAL_CFLAGS := -DVBP -DHOST_ONLY

LOCAL_C_INCLUDES :=							   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/include		   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/include		   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/parser/include	   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/mp4/include

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp_mpeg4

LOCAL_SHARED_LIBRARIES :=		\
	libmixvbp

include $(BUILD_SHARED_LIBRARY)
