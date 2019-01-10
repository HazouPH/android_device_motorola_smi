#
# Copyright 2010 Intrinsyc Software International, Inc.  All rights reserved.
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    ND/te_base.cpp \
    ND/te.cpp \
    ND/cellInfo_cache.cpp \
    ND/systemmanager.cpp \
    ND/radio_state.cpp \
    silo.cpp \
    ND/silo_voice.cpp \
    ND/silo_network.cpp \
    ND/silo_sim.cpp \
    ND/silo_data.cpp \
    ND/silo_phonebook.cpp \
    ND/silo_sms.cpp \
    ND/silo_misc.cpp \
    ND/silo_ims.cpp \
    ND/silo_rfcoexistence.cpp \
    ND/silo_common.cpp \
    ND/channel_nd.cpp \
    channelbase.cpp \
    channel_atcmd.cpp \
    channel_data.cpp \
    channel_DLC2.cpp \
    channel_DLC6.cpp \
    channel_DLC8.cpp \
    channel_DLC22.cpp \
    channel_DLC23.cpp \
    channel_Sms.cpp \
    channel_URC.cpp \
    channel_OEM.cpp \
    port.cpp \
    ND/callbacks.cpp \
    ND/file_ops.cpp \
    ND/reset.cpp \
    ND/rildmain.cpp \
    ND/sync_ops.cpp \
    ND/thread_ops.cpp \
    cmdcontext.cpp \
    command.cpp \
    request_info.cpp \
    response.cpp \
    request_info_table.cpp \
    thread_manager.cpp \
    ND/MODEMS/initializer.cpp \
    ND/MODEMS/init6260.cpp \
    ND/MODEMS/init6360.cpp \
    ND/MODEMS/init7160.cpp \
    ND/MODEMS/init7260.cpp \
    ND/MODEMS/init2230.cpp \
    ND/MODEMS/data_util.cpp \
    ND/MODEMS/te_xmm6260.cpp \
    ND/MODEMS/te_xmm6360.cpp \
    ND/MODEMS/te_xmm7160.cpp \
    ND/MODEMS/te_xmm7260.cpp \
    ND/MODEMS/te_xmm2230.cpp \
    ND/bertlv_util.cpp \
    ND/ccatprofile.cpp \
    usat_state.cpp \
    usat_init_state_machine.cpp \
    cbs_info.cpp \
    ND/hardwareconfig.cpp

LOCAL_SHARED_LIBRARIES := libcutils libutils libmmgrcli librilutils liblog

# Activating this macro enables PIN retry count feature
ifeq ($(strip $(M2_PIN_RETRIES_FEATURE_ENABLED)),true)
LOCAL_CFLAGS += -DM2_PIN_RETRIES_FEATURE_ENABLED
endif

# Activating this macro enables PIN caching (for modem cold reboot)
LOCAL_CFLAGS += -DM2_PIN_CACHING_FEATURE_ENABLED

LOCAL_C_INCLUDES :=  \
    $(LOCAL_PATH)/ND  \
    $(LOCAL_PATH)/ND/MODEMS  \
    $(LOCAL_PATH)/MODEMS  \
    $(LOCAL_PATH)/../INC \
    $(LOCAL_PATH)/../UTIL/ND \
    $(TARGET_OUT_HEADERS)/IFX-modem \

#build shared library
LOCAL_PRELINK_MODULE := false
LOCAL_STRIP_MODULE := true
LOCAL_SHARED_LIBRARIES += librapid-ril-util
LOCAL_CFLAGS += -DRIL_SHLIB -Os
LOCAL_MODULE:= librapid-ril-core
LOCAL_MODULE_OWNER := intel
LOCAL_MODULE_TAGS:= optional
include $(BUILD_SHARED_LIBRARY)

