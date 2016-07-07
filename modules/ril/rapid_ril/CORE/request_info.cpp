////////////////////////////////////////////////////////////////////////////
// request_info.cpp
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Maps Android AT command requests to DLC channels.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rilchannels.h"
#include "request_info.h"

// Internal request info
REQ_INFO_INTERNAL g_ReqInternal[] =
{
     { { "SilentPinEntry", RIL_CHANNEL_DLC8, 0 }, E_REQ_ID_INTERNAL_SILENT_PIN_ENTRY },
     { { "QuerySimSmsStoreStatus", RIL_CHANNEL_OEM, 0 },
            E_REQ_ID_INTERNAL_QUERY_SIM_SMS_STORE_STATUS },
     { { "SetProfileDownloadForNextUiccStartup", RIL_CHANNEL_URC, 0 },
            E_REQ_ID_INTERNAL_SET_PROFILE_DOWNLOAD_FOR_NEXT_UICC_STARTUP },
     { { "ConfigureUsatProfileDownload", RIL_CHANNEL_URC, 0 },
            E_REQ_ID_INTERNAL_CONFIGURE_USAT_PROFILE_DOWNLOAD },
     { { "QueryUiccState", RIL_CHANNEL_DLC8, 0 }, E_REQ_ID_INTERNAL_QUERY_UICC_STATE },
     { { "ReadUsatProfiles", RIL_CHANNEL_DLC8, 0 }, E_REQ_ID_INTERNAL_READ_USAT_PROFILES },
     { { "WriteUsatProfile", RIL_CHANNEL_DLC8, 0 }, E_REQ_ID_INTERNAL_WRITE_USAT_PROFILE },
     { { "ResetUicc", RIL_CHANNEL_DLC8, 0 }, E_REQ_ID_INTERNAL_RESET_UICC },
     { { "EnableProfileHandlingFacility", RIL_CHANNEL_URC, 0 },
            E_REQ_ID_INTERNAL_ENABLE_PROFILE_FACILITY_HANDLING }
};

const int INTERNAL_REQ_ID_TOTAL = (sizeof(g_ReqInternal) / sizeof(REQ_INFO_INTERNAL));

REQ_INFO* g_pReqInfo;

// Request info array - Maps a request id to request names and channels
// Access request info using request ids defined in ril.h
const REQ_INFO g_ReqInfoDefault[] =
{
    // reserved/not used
    { "", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_GET_SIM_STATUS
    { "GetSimStatus", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_ENTER_SIM_PIN
    { "EnterSimPin", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_ENTER_SIM_PUK
    { "EnterSimPuk", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_ENTER_SIM_PIN2
    { "EnterSimPin2", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_ENTER_SIM_PUK2
    { "EnterSimPuk2", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_CHANGE_SIM_PIN
    { "ChangeSimPin", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_CHANGE_SIM_PIN2
    { "ChangeSimPin2", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION
    { "EnterNetworkDepersonalization", RIL_CHANNEL_DLC2, 0},
    // RIL_REQUEST_GET_CURRENT_CALLS
    { "GetCurrentCalls", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_DIAL
    { "Dial", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_GET_IMSI
    { "GetIMSI", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_HANGUP
    { "Hangup", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND
    { "HangupWaitingOrBackground", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND
    { "HangupForegroundResumeBackground", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE
    { "SwitchHoldingAndActive", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_CONFERENCE
    { "Conference", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_UDUB
    { "UDUB", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_LAST_CALL_FAIL_CAUSE
    { "LastCallFailCause", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_SIGNAL_STRENGTH
    { "SignalStrength", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_VOICE_REGISTRATION_STATE
    { "RegistrationState", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_DATA_REGISTRATION_STATE
    { "GprsRegistrationState", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_OPERATOR
    { "Operator", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_RADIO_POWER
    { "RadioPower", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_DTMF
    { "Dtmf", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_SEND_SMS
    { "SendSms", RIL_CHANNEL_SMS, 0 },
    // RIL_REQUEST_SEND_SMS_EXPECT_MORE
    { "SendSmsExpectMore", RIL_CHANNEL_SMS, 0 },
    // RIL_REQUEST_SETUP_DATA_CALL
    { "SetupDefaultPDP", RIL_CHANNEL_DATA1, 0 },
    // RIL_REQUEST_SIM_IO
    { "SimIO", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_SEND_USSD
    { "SendUSSD", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_CANCEL_USSD
    { "CancelUSSD", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_GET_CLIR
    { "GetCLIR", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_SET_CLIR
    { "SetCLIR", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
    { "QueryCallForwardStatus", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_SET_CALL_FORWARD
    { "SetCallForward", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_QUERY_CALL_WAITING
    { "QueryCallWaiting", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_SET_CALL_WAITING
    { "SetCallWaiting", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_SMS_ACKNOWLEDGE
    { "SmsAcknowledge", RIL_CHANNEL_SMS, 0 },
    // RIL_REQUEST_GET_IMEI
    { "GetIMEI", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_GET_IMEISV
    { "GetIMEISV", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_ANSWER
    { "Answer", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_DEACTIVATE_DATA_CALL
    { "DeactivateDataCall", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_QUERY_FACILITY_LOCK
    { "QueryFacilityLock", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_SET_FACILITY_LOCK
    { "SetFacilityLock", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_CHANGE_BARRING_PASSWORD
    { "ChangeBarringPassword", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE
    { "QueryNetworkSelectionMode", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
    { "SetNetworkSelectionAutomatic", RIL_CHANNEL_DLC22, 0 },
    // RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL
    { "SetNetworkSelectionManual", RIL_CHANNEL_DLC22, 0 },
    // RIL_REQUEST_QUERY_AVAILABLE_NETWORKS
    { "QueryAvailableNetworks", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_DTMF_START
    { "RequestDtmfStart", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_DTMF_STOP
    { "RequestDtmfStop", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_BASEBAND_VERSION
    { "BasebandVersion", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_SEPARATE_CONNECTION
    { "SeperateConnection", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_SET_MUTE
    { "SetMute", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_GET_MUTE
    { "GetMute", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_QUERY_CLIP
    { "QueryCLIP", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE
    { "LastPdpFailCause", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_DATA_CALL_LIST
    { "PdpContextList", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_RESET_RADIO
    { "ResetRadio", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_OEM_HOOK_RAW
    { "OemHookRaw", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_OEM_HOOK_STRINGS
    { "OemHookStrings", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_SCREEN_STATE
    { "ScreenState", RIL_CHANNEL_URC, 0 },
    // RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION
    { "SetSuppSvcNotification", RIL_CHANNEL_DLC6, 0 },
    // RIL_REQUEST_WRITE_SMS_TO_SIM
    { "WriteSmsToSim", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_DELETE_SMS_ON_SIM
    { "DeleteSmsOnSim", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_SET_BAND_MODE
    { "SetBandMode", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE
    { "QueryAvailableBandMode", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_STK_GET_PROFILE
    { "StkGetProfile", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_STK_SET_PROFILE
    { "StkSetProfile", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
    { "StkSendEnvelopeCommand", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
    { "StkSendTerminalResponse", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM
    { "StkHandleCallSetupRequestedFromSim", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_EXPLICIT_CALL_TRANSFER
    { "ExplicitCallTransfer", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
    { "SetPreferredNetworkType", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
    { "GetPreferredNetworkType", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_GET_NEIGHBORING_CELL_IDS
    { "GetNeighboringCellIDs", RIL_CHANNEL_OEM, 0 },
    // RIL_REQUEST_SET_LOCATION_UPDATES
    { "SetLocationUpdates", RIL_CHANNEL_URC, 0 },
    // RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE
    { "CdmaSetSubscription", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE
    { "CdmaSetRoamingPreference", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE
    { "CdmaQueryRoamingPreference", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_SET_TTY_MODE
    { "SetTtyMode", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_QUERY_TTY_MODE
    { "QueryTtyMode", RIL_CHANNEL_ATCMD, 0 },
    // RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE
    { "CdmaSetPreferredVoicePrivacyMode", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE
    { "CdmaQueryPreferredVoicePrivacyMode", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_FLASH
    { "CdmaFlash", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_BURST_DTMF
    { "CdmaBurstDtmf", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY
    { "CdmaValidateKey", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_SEND_SMS
    { "CdmaSendSms", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE
    { "CdmaSmsAcknowledge", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG
    { "GetBroadcastSmsConfig", RIL_CHANNEL_SMS, 0 },
    // RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG
    { "SetBroadcastSmsConfig", RIL_CHANNEL_SMS, 0 },
    // RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION
    { "SmsBroadcastActivation", RIL_CHANNEL_SMS, 0 },
    // RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG
    { "CdmaGetBroadcastSmsConfig", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG
    { "CdmaSetBroadcastSmsConfig", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION
    { "CdmaSmsBroadcastActivation", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_SUBSCRIPTION
    { "CdmaSubscription", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM
    { "CdmaWriteSmsToRuim", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM
    { "CdmaDeleteSmsOnRuim", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_DEVICE_IDENTITY
    { "DeviceIdentity", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE
    { "ExitEmergencyCallBackMode", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_GET_SMSC_ADDRESS
    { "GetSmscAddress", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_SET_SMSC_ADDRESS
    { "SetSmscAddress", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_REPORT_SMS_MEMORY_STATUS
    { "ReportSmsMemoryStatus", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
    { "ReportStkServiceIsRunning", RIL_CHANNEL_URC, 0 },
    // RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE
    { "GetSubscriptionSource", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_ISIM_AUTHENTICATION
    { "IsimAuthentication", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU
    { "AckIncomingSmsWithPdu", RIL_CHANNEL_SMS, 0 },
    // RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
    { "StkSendEnvelopeWithStatus", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_VOICE_RADIO_TECH
    { "VoiceRadioTech", RIL_CHANNEL_DLC2, 0 },
    // RIL_REQUEST_GET_CELL_INFO_LIST
    { "GetCellInfoList", RIL_CHANNEL_OEM, 0 },
    // RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE
    { "SetCellInfoListRate", RIL_CHANNEL_OEM, 0 },
    // RIL_REQUEST_SET_INITIAL_ATTACH_APN
    { "SetInitialAttachApn", RIL_CHANNEL_DLC22, 0},
    // RIL_REQUEST_IMS_REGISTRATION_STATE
    { "GetImsRegistrationState", RIL_CHANNEL_RESERVED, 0},
    // RIL_REQUEST_IMS_SEND_SMS
    { "SendImsSms", RIL_CHANNEL_RESERVED, 0},
    // RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC
    { "SimTransmitBasic", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_SIM_OPEN_CHANNEL
    { "SimOpenChannel", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_SIM_CLOSE_CHANNEL
    { "SimCloseChannel", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL
    { "SimTransmitChannel", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_NV_READ_ITEM
    { "NVReadItem", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_NV_WRITE_ITEM
    { "NVWriteItem", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_NV_WRITE_CDMA_PRL
    { "NVWriteCdmaPRL", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_NV_RESET_CONFIG
    { "NVResetConfig", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_SET_UICC_SUBSCRIPTION
    { "SetUiccSubscription", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_ALLOW_DATA
    { "AllowData", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_GET_HARDWARE_CONFIG
    { "GetHardwareConfig", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_SIM_AUTHENTICATION
    { "SimAuthentication", RIL_CHANNEL_DLC8, 0 },
    // RIL_REQUEST_GET_DC_RT_INFO
    { "GetDcRtInfo", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_SET_DC_RT_INFO_RATE
    { "SetDcRtInfo", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_SET_DATA_PROFILE
    { "SetDataProfile", RIL_CHANNEL_RESERVED, 0 },
    // RIL_REQUEST_SHUTDOWN
    { "Shutdown", RIL_CHANNEL_ATCMD, 0 },
};

const int REQ_ID_TOTAL = (sizeof(g_ReqInfoDefault) / sizeof(REQ_INFO));
