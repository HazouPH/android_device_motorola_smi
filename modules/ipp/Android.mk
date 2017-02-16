LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE:=true
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=host/libsvml.a
LOCAL_MODULE:=libsvml
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippacmerged.a
LOCAL_MODULE:=libippacmerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippccmerged.a
LOCAL_MODULE:=libippccmerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippdcmerged.a
LOCAL_MODULE:=libippdcmerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippsmerged.a
LOCAL_MODULE:=libippsmerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippcc.a
LOCAL_MODULE:=libippcc
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippcv.a
LOCAL_MODULE:=libippcv
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippj.a
LOCAL_MODULE:=libippj
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_MODULE:=libippi
GEN := target/libippi.a
$(GEN): $(shell tar xvzf $(LOCAL_PATH)/target/libippi.a.tar.gz -C $(LOCAL_PATH)/target)
LOCAL_SRC_FILES := $(GEN)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippm.a
LOCAL_MODULE:=libippm
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippvm.a
LOCAL_MODULE:=libippvm
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libipps.a
LOCAL_MODULE:=libipps
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippcore.a
LOCAL_MODULE:=libippcore
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippscemerged.a
LOCAL_MODULE:=libippscemerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippscmerged.a
LOCAL_MODULE:=libippscmerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippsremerged.a
LOCAL_MODULE:=libippsremerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippsrmerged.a
LOCAL_MODULE:=libippsrmerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libippsemerged.a
LOCAL_MODULE:=libippsemerged
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_CLASS:=STATIC_LIBRARIES
LOCAL_MODULE_TAGS:=optional
LOCAL_SRC_FILES:=target/libiomp5.a
LOCAL_MODULE:=libiomp5
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS:=include/ippac.h include/ippcc.h include/ippch.h include/ippcore.h include/ippcv.h include/ippdc.h include/ippdefs.h include/ippdi.h include/ipp.h include/ippi.h include/ippj.h include/ippm.h include/ippr.h include/ipp_s8.h include/ippsc.h include/ipps.h include/ippsr.h include/ippvc.h include/ippversion.h include/ippvm.h
LOCAL_COPY_HEADERS_TO:=ipp
include $(BUILD_COPY_HEADERS)
