////////////////////////////////////////////////////////////////////////////
// te_nd.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CTE class which handles all overrides to requests and
//    basic behavior for responses for a specific modem
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_TE_H
#define RRIL_TE_H

#include "nd_structs.h"
#include "rril.h"
#include "rril_OEM.h"
#include "radio_state.h"
#include "types.h"
#include "command.h"
#include "initializer.h"
#include "cellInfo_cache.h"
#include "constants.h"

class CTEBase;

class CTE
{
private:
    CTE();
    CTE(UINT32 modemType);
    ~CTE();

    //  Prevent assignment: Declared but not implemented.
    CTE(const CTE& rhs);  // Copy Constructor
    CTE& operator=(const CTE& rhs);  //  Assignment operator

    CTEBase* CreateModemTE(CTE* pTEInstance);

    static CTE* m_pTEInstance;
    CTEBase* m_pTEBaseInstance;

public:
    static void CreateTE(UINT32 modemType);
    static CTE& GetTE();
    static void DeleteTEObject();
    UINT32 GetModemType() { return m_uiModemType; };

    CInitializer* GetInitializer();

    // Accessor functions for configuring data connections
    BOOL DataConfigDown(UINT32 uiCID, BOOL bForceCleanup);
    void CleanupAllDataConnections();

    BOOL ParseCREG(const char*& rszPointer,
                          const BOOL bUnSolicited,
                          S_ND_REG_STATUS& pCSRegStruct);
    BOOL ParseCGREG(const char*& rszPointer,
                           const BOOL bUnSolicited,
                           S_ND_GPRS_REG_STATUS& pPSRegStruct);
    BOOL ParseXREG(const char*& rszPointer,
                          const BOOL bUnSolicited,
                          S_ND_GPRS_REG_STATUS& pPSRegStruct);
    BOOL ParseCEREG(const char*& rszPointer, const BOOL bUnSolicited,
                         S_ND_GPRS_REG_STATUS& rPSRegStatusInfo);

    RIL_RadioTechnology MapAccessTechnology(UINT32 uiStdAct, int regType);

    char* GetBasicInitCommands(UINT32 uiChannelType);
    char* GetUnlockInitCommands(UINT32 uiChannelType);

    BOOL IsRequestSupported(int requestId);
    void HandleRequest(int requestID, void* pData, size_t datalen, RIL_Token hRilToken);
    RIL_Errno HandleRequestWhenNoModem(int requestID, RIL_Token hRilToken);
    RIL_Errno HandleRequestInRadioOff(int requestID, RIL_Token hRilToken);
    RIL_Errno HandleRequestWhenNotRegistered(int requestID, RIL_Token hRilToken);

    BOOL IsRequestAllowedInSpoofState(int requestId);
    BOOL IsRequestAllowedInRadioOff(int requestId);
    BOOL IsRequestAllowedInSimNotReady(int requestId);
    BOOL IsRequestAllowedWhenNotRegistered(int requestId);
    BOOL IsInternalRequestsAllowedInRadioOff(int requestId);
    BOOL IsRequestAllowed(int requestId, RIL_Token rilToken, UINT32 uiChannelId,
            BOOL bIsInitCommand, int callId = 0);
    BOOL IsOemHookPossible(int requestId, void* pData, size_t uiDataSize);

    // Calls FindIdenticalRequestsAndSendResponses on the given channel
    static int CompleteIdenticalRequests(UINT32 uiChannelId, int reqID, UINT32 uiResultCode,
            void* pResponse, size_t responseLen, int callId = -1);

    // RIL_REQUEST_GET_SIM_STATUS
    RIL_RESULT_CODE RequestGetSimStatus(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetSimStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_SIM_PIN
    RIL_RESULT_CODE RequestEnterSimPin(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseEnterSimPin(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_SIM_PUK
    RIL_RESULT_CODE RequestEnterSimPuk(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseEnterSimPuk(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_SIM_PIN2
    RIL_RESULT_CODE RequestEnterSimPin2(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseEnterSimPin2(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_SIM_PUK2
    RIL_RESULT_CODE RequestEnterSimPuk2(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseEnterSimPuk2(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CHANGE_SIM_PIN
    RIL_RESULT_CODE RequestChangeSimPin(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseChangeSimPin(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CHANGE_SIM_PIN2
    RIL_RESULT_CODE RequestChangeSimPin2(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseChangeSimPin2(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION
    RIL_RESULT_CODE RequestEnterNetworkDepersonalization(RIL_Token rilToken,
                                                               void* pData,
                                                               size_t datalen);
    RIL_RESULT_CODE ParseEnterNetworkDepersonalization(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_CURRENT_CALLS
    RIL_RESULT_CODE RequestGetCurrentCalls(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetCurrentCalls(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DIAL
    RIL_RESULT_CODE RequestDial(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseDial(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_IMSI
    RIL_RESULT_CODE RequestGetImsi(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetImsi(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_HANGUP
    RIL_RESULT_CODE RequestHangup(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseHangup(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND
    RIL_RESULT_CODE RequestHangupWaitingOrBackground(RIL_Token rilToken,
                                                            void* pData,
                                                            size_t datalen);
    RIL_RESULT_CODE ParseHangupWaitingOrBackground(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND
    RIL_RESULT_CODE RequestHangupForegroundResumeBackground(RIL_Token rilToken,
                                                                   void* pData,
                                                                   size_t datalen);
    RIL_RESULT_CODE ParseHangupForegroundResumeBackground(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE
    // RIL_REQUEST_SWITCH_HOLDING_AND_ACTIVE
    RIL_RESULT_CODE RequestSwitchHoldingAndActive(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSwitchHoldingAndActive(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CONFERENCE
    RIL_RESULT_CODE RequestConference(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseConference(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_UDUB
    RIL_RESULT_CODE RequestUdub(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseUdub(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_LAST_CALL_FAIL_CAUSE
    RIL_RESULT_CODE RequestLastCallFailCause(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseLastCallFailCause(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIGNAL_STRENGTH
    RIL_RESULT_CODE RequestSignalStrength(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSignalStrength(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_VOICE_REGISTRATION_STATE
    RIL_RESULT_CODE RequestRegistrationState(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseRegistrationState(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DATA_REGISTRATION_STATE
    RIL_RESULT_CODE RequestGPRSRegistrationState(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGPRSRegistrationState(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_OPERATOR
    RIL_RESULT_CODE RequestOperator(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseOperator(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_RADIO_POWER
    RIL_RESULT_CODE RequestRadioPower(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseRadioPower(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DTMF
    RIL_RESULT_CODE RequestDtmf(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseDtmf(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SEND_SMS
    RIL_RESULT_CODE RequestSendSms(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSendSms(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SEND_SMS_EXPECT_MORE
    RIL_RESULT_CODE RequestSendSmsExpectMore(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSendSmsExpectMore(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SETUP_DATA_CALL
    RIL_RESULT_CODE RequestSetupDataCall(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetupDataCall(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_IO
    RIL_RESULT_CODE RequestSimIo(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSimIo(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SEND_USSD
    RIL_RESULT_CODE RequestSendUssd(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSendUssd(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CANCEL_USSD
    RIL_RESULT_CODE RequestCancelUssd(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseCancelUssd(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_CLIR
    RIL_RESULT_CODE RequestGetClir(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetClir(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_CLIR
    RIL_RESULT_CODE RequestSetClir(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetClir(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
    RIL_RESULT_CODE RequestQueryCallForwardStatus(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseQueryCallForwardStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_CALL_FORWARD
    RIL_RESULT_CODE RequestSetCallForward(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetCallForward(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_CALL_WAITING
    RIL_RESULT_CODE RequestQueryCallWaiting(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseQueryCallWaiting(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_CALL_WAITING
    RIL_RESULT_CODE RequestSetCallWaiting(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetCallWaiting(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SMS_ACKNOWLEDGE
    RIL_RESULT_CODE RequestSmsAcknowledge(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSmsAcknowledge(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_IMEI
    RIL_RESULT_CODE RequestGetImei(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetImei(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_IMEISV
    RIL_RESULT_CODE RequestGetImeisv(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetImeisv(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ANSWER
    RIL_RESULT_CODE RequestAnswer(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseAnswer(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DEACTIVATE_DATA_CALL
    RIL_RESULT_CODE RequestDeactivateDataCall(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseDeactivateDataCall(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_FACILITY_LOCK
    RIL_RESULT_CODE RequestQueryFacilityLock(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseQueryFacilityLock(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_FACILITY_LOCK
    RIL_RESULT_CODE RequestSetFacilityLock(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetFacilityLock(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CHANGE_BARRING_PASSWORD
    RIL_RESULT_CODE RequestChangeBarringPassword(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseChangeBarringPassword(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE
    RIL_RESULT_CODE RequestQueryNetworkSelectionMode(RIL_Token rilToken,
                                                            void* pData,
                                                            size_t datalen);
    RIL_RESULT_CODE ParseQueryNetworkSelectionMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
    RIL_RESULT_CODE RequestSetNetworkSelectionAutomatic(RIL_Token rilToken,
                                                               void* pData,
                                                               size_t datalen);
    RIL_RESULT_CODE ParseSetNetworkSelectionAutomatic(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL
    RIL_RESULT_CODE RequestSetNetworkSelectionManual(RIL_Token rilToken,
                                                            void* pData,
                                                            size_t datalen);
    RIL_RESULT_CODE ParseSetNetworkSelectionManual(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_AVAILABLE_NETWORKS
    RIL_RESULT_CODE RequestQueryAvailableNetworks(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseQueryAvailableNetworks(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DTMF_START
    RIL_RESULT_CODE RequestDtmfStart(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseDtmfStart(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DTMF_STOP
    RIL_RESULT_CODE RequestDtmfStop(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseDtmfStop(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_BASEBAND_VERSION
    RIL_RESULT_CODE RequestBasebandVersion(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseBasebandVersion(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SEPARATE_CONNECTION
    RIL_RESULT_CODE RequestSeparateConnection(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSeparateConnection(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_MUTE
    RIL_RESULT_CODE RequestSetMute(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetMute(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_MUTE
    RIL_RESULT_CODE RequestGetMute(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetMute(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_CLIP
    RIL_RESULT_CODE RequestQueryClip(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseQueryClip(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE
    RIL_RESULT_CODE RequestLastDataCallFailCause(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseLastDataCallFailCause(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DATA_CALL_LIST
    RIL_RESULT_CODE RequestDataCallList(RIL_Token rilToken, void* pData, size_t datalen);

    // RIL_REQUEST_RESET_RADIO
    RIL_RESULT_CODE RequestResetRadio(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseResetRadio(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_OEM_HOOK_RAW
    RIL_RESULT_CODE RequestHookRaw(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseHookRaw(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_OEM_HOOK_STRINGS
    RIL_RESULT_CODE RequestHookStrings(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseHookStrings(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseGetVersion(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseGetRxGain(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseSetRxGain(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SCREEN_STATE
    RIL_RESULT_CODE RequestScreenState(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseScreenState(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION
    RIL_RESULT_CODE RequestSetSuppSvcNotification(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetSuppSvcNotification(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_WRITE_SMS_TO_SIM
    RIL_RESULT_CODE RequestWriteSmsToSim(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseWriteSmsToSim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DELETE_SMS_ON_SIM
    RIL_RESULT_CODE RequestDeleteSmsOnSim(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseDeleteSmsOnSim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_BAND_MODE
    RIL_RESULT_CODE RequestSetBandMode(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetBandMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE
    RIL_RESULT_CODE RequestQueryAvailableBandMode(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseQueryAvailableBandMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_GET_PROFILE
    RIL_RESULT_CODE RequestStkGetProfile(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseStkGetProfile(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SET_PROFILE
    RIL_RESULT_CODE RequestStkSetProfile(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseStkSetProfile(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
    RIL_RESULT_CODE RequestStkSendEnvelopeCommand(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseStkSendEnvelopeCommand(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
    RIL_RESULT_CODE RequestStkSendTerminalResponse(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseStkSendTerminalResponse(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM
    RIL_RESULT_CODE RequestStkHandleCallSetupRequestedFromSim(RIL_Token rilToken,
                                                                     void* pData,
                                                                     size_t datalen);
    RIL_RESULT_CODE ParseStkHandleCallSetupRequestedFromSim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_EXPLICIT_CALL_TRANSFER
    RIL_RESULT_CODE RequestExplicitCallTransfer(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseExplicitCallTransfer(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
    RIL_RESULT_CODE RequestSetPreferredNetworkType(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetPreferredNetworkType(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
    RIL_RESULT_CODE RequestGetPreferredNetworkType(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetPreferredNetworkType(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_NEIGHBORING_CELL_IDS
    RIL_RESULT_CODE RequestGetNeighboringCellIDs(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetNeighboringCellIDs(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_LOCATION_UPDATES
    RIL_RESULT_CODE RequestSetLocationUpdates(RIL_Token rilToken, void* pData,
            size_t datalen);
    RIL_RESULT_CODE ParseSetLocationUpdates(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE
    RIL_RESULT_CODE RequestCdmaSetSubscription(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseCdmaSetSubscription(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE
    RIL_RESULT_CODE RequestCdmaSetRoamingPreference(RIL_Token rilToken,
                                                           void* pData,
                                                           size_t datalen);
    RIL_RESULT_CODE ParseCdmaSetRoamingPreference(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE
    RIL_RESULT_CODE RequestCdmaQueryRoamingPreference(RIL_Token rilToken,
                                                             void* pData,
                                                             size_t datalen);
    RIL_RESULT_CODE ParseCdmaQueryRoamingPreference(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_TTY_MODE
    RIL_RESULT_CODE RequestSetTtyMode(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetTtyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_TTY_MODE
    RIL_RESULT_CODE RequestQueryTtyMode(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseQueryTtyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE
    RIL_RESULT_CODE RequestCdmaSetPreferredVoicePrivacyMode(RIL_Token rilToken,
                                                                   void* pData,
                                                                   size_t datalen);
    RIL_RESULT_CODE ParseCdmaSetPreferredVoicePrivacyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE
    RIL_RESULT_CODE RequestCdmaQueryPreferredVoicePrivacyMode(RIL_Token rilToken,
                                                                     void* pData,
                                                                     size_t datalen);
    RIL_RESULT_CODE ParseCdmaQueryPreferredVoicePrivacyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_FLASH
    RIL_RESULT_CODE RequestCdmaFlash(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseCdmaFlash(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_BURST_DTMF
    RIL_RESULT_CODE RequestCdmaBurstDtmf(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseCdmaBurstDtmf(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_VALIDATE_AKEY
    RIL_RESULT_CODE RequestCdmaValidateAndWriteAkey(RIL_Token rilToken,
                                                           void* pData,
                                                           size_t datalen);
    RIL_RESULT_CODE ParseCdmaValidateAndWriteAkey(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SEND_SMS
    RIL_RESULT_CODE RequestCdmaSendSms(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseCdmaSendSms(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE
    RIL_RESULT_CODE RequestCdmaSmsAcknowledge(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseCdmaSmsAcknowledge(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG
    RIL_RESULT_CODE RequestGsmGetBroadcastSmsConfig(RIL_Token rilToken,
                                                           void* pData,
                                                           size_t datalen);
    RIL_RESULT_CODE ParseGsmGetBroadcastSmsConfig(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG
    RIL_RESULT_CODE RequestGsmSetBroadcastSmsConfig(RIL_Token rilToken,
                                                           void* pData,
                                                           size_t datalen);
    RIL_RESULT_CODE ParseGsmSetBroadcastSmsConfig(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION
    RIL_RESULT_CODE RequestGsmSmsBroadcastActivation(RIL_Token rilToken,
                                                            void* pData,
                                                            size_t datalen);
    RIL_RESULT_CODE ParseGsmSmsBroadcastActivation(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG
    RIL_RESULT_CODE RequestCdmaGetBroadcastSmsConfig(RIL_Token rilToken,
                                                            void* pData,
                                                            size_t datalen);
    RIL_RESULT_CODE ParseCdmaGetBroadcastSmsConfig(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG
    RIL_RESULT_CODE RequestCdmaSetBroadcastSmsConfig(RIL_Token rilToken,
                                                            void* pData,
                                                            size_t datalen);
    RIL_RESULT_CODE ParseCdmaSetBroadcastSmsConfig(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION
    RIL_RESULT_CODE RequestCdmaSmsBroadcastActivation(RIL_Token rilToken,
                                                             void* pData,
                                                             size_t datalen);
    RIL_RESULT_CODE ParseCdmaSmsBroadcastActivation(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SUBSCRIPTION
    RIL_RESULT_CODE RequestCdmaSubscription(RIL_Token rilToken,
                                                   void* pData,
                                                   size_t datalen);
    RIL_RESULT_CODE ParseCdmaSubscription(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM
    RIL_RESULT_CODE RequestCdmaWriteSmsToRuim(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseCdmaWriteSmsToRuim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM
    RIL_RESULT_CODE RequestCdmaDeleteSmsOnRuim(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseCdmaDeleteSmsOnRuim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DEVICE_IDENTITY
    RIL_RESULT_CODE RequestDeviceIdentity(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseDeviceIdentity(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE
    RIL_RESULT_CODE RequestExitEmergencyCallbackMode(RIL_Token rilToken,
                                                            void* pData,
                                                            size_t datalen);
    RIL_RESULT_CODE ParseExitEmergencyCallbackMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_SMSC_ADDRESS
    RIL_RESULT_CODE RequestGetSmscAddress(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetSmscAddress(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_SMSC_ADDRESS
    RIL_RESULT_CODE RequestSetSmscAddress(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetSmscAddress(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_REPORT_SMS_MEMORY_STATUS
    RIL_RESULT_CODE RequestReportSmsMemoryStatus(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseReportSmsMemoryStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
    RIL_RESULT_CODE RequestReportStkServiceRunning(RIL_Token rilToken,
                                                          void* pData,
                                                          size_t datalen);
    RIL_RESULT_CODE ParseReportStkServiceRunning(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ISIM_AUTHENTICATION
    RIL_RESULT_CODE RequestISimAuthenticate(RIL_Token rilToken,
                                                           void* pData,
                                                           size_t datalen);
    RIL_RESULT_CODE ParseISimAuthenticate(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU
    RIL_RESULT_CODE RequestAckIncomingGsmSmsWithPdu(RIL_Token rilToken,
                                                           void* pData,
                                                           size_t datalen);
    RIL_RESULT_CODE ParseAckIncomingGsmSmsWithPdu(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
    RIL_RESULT_CODE RequestStkSendEnvelopeWithStatus(RIL_Token rilToken,
                                                            void* pData,
                                                            size_t datalen);
    RIL_RESULT_CODE ParseStkSendEnvelopeWithStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_VOICE_RADIO_TECH
    RIL_RESULT_CODE RequestVoiceRadioTech(RIL_Token rilToken, void* pData, size_t datalen);

    // RIL_REQUEST_GET_CELL_INFO_LIST
    RIL_RESULT_CODE RequestGetCellInfoList(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseGetCellInfoList(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE
    RIL_RESULT_CODE RequestSetCellInfoListRate(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseUnsolCellInfoListRate(RESPONSE_DATA& rRspData);
    void PostUnsolCellInfoListRate(POST_CMD_HANDLER_DATA& rData);

    // RIL_REQUEST_SET_INITIAL_ATTACH_APN
    RIL_RESULT_CODE RequestSetInitialAttachApn(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSetInitialAttachApn(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_IMS_REGISTRATION_STATE:
    // RIL_REQUEST_IMS_SEND_SMS:
    // TODO

    // RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC
    RIL_RESULT_CODE RequestSimTransmitApduBasic(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSimTransmitApduBasic(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_OPEN_CHANNEL
    RIL_RESULT_CODE RequestSimOpenChannel(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSimOpenChannel(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_CLOSE_CHANNEL
    RIL_RESULT_CODE RequestSimCloseChannel(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSimCloseChannel(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL
    RIL_RESULT_CODE RequestSimTransmitApduChannel(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSimTransmitApduChannel(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_DATA_PROFILE
    RIL_RESULT_CODE RequestSetDataProfile(RIL_Token rilToken, void* pData, size_t datalen);

    // RIL_REQUEST_SHUTDOWN
    RIL_RESULT_CODE RequestShutdown(RIL_Token rilToken, void* pData, size_t datalen);
    RIL_RESULT_CODE ParseShutdown(RESPONSE_DATA& rspData);
    void PostShutdown(POST_CMD_HANDLER_DATA& data);

    // RIL_REQUEST_SIM_AUTHENTICATION
    RIL_RESULT_CODE RequestSimAuthentication(RIL_Token rilToken,
            void* pData, size_t datalen);
    RIL_RESULT_CODE ParseSimAuthentication(RESPONSE_DATA& rspData);
    void PostSimAuthentication(POST_CMD_HANDLER_DATA& data);

    // RIL_UNSOL_SIGNAL_STRENGTH
    RIL_RESULT_CODE ParseUnsolicitedSignalStrength(RESPONSE_DATA& rRspData);

    // Parser functions for data call related requests
    RIL_RESULT_CODE ParsePdpContextActivate(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseQueryIpAndDns(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseEnterDataState(RESPONSE_DATA& rRspData);

    RIL_RESULT_CODE ParseSilentPinEntry(RESPONSE_DATA& rRspData);

    void StoreRegistrationInfo(void* pRegStruct, int regType);
    RIL_RESULT_CODE ParseReadContextParams(RESPONSE_DATA& rRspData);

    RIL_RESULT_CODE ParseReadBearerTFTParams(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseReadBearerQOSParams(RESPONSE_DATA& rRspData);

    void CopyCachedRegistrationInfo(void* pRegStruct, BOOL bPSStatus);
    void ResetRegistrationCache();

    // REQ_ID_QUERY_SIM_SMS_STORE_STATUS
    RIL_RESULT_CODE ParseQuerySimSmsStoreStatus(RESPONSE_DATA& rRspData);

    void SetIncomingCallStatus(UINT32 uiCallId, UINT32 uiStatus);
    UINT32 GetIncomingCallId();

    void SetupDataCallOngoing(BOOL bStatus);
    BOOL IsSetupDataCallOnGoing();

    BOOL IsLocationUpdatesEnabled();

    RIL_RadioState GetRadioState();
    int GetSimCardState();
    int GetSimAppState();
    int GetSimPinState();
    void SetRadioState(const RRIL_Radio_State eRadioState);
    void SetRadioStateAndNotify(const RRIL_Radio_State eRadioState);
    void SetSimState(int cardState, int appState, int pinState);
    void SetSimAppState(int appState);
    void SetPersonalisationSubState(int perso_substate);

    void SetSpoofCommandsStatus(BOOL bStatus) { m_bSpoofCommandsStatus = bStatus; };
    BOOL GetSpoofCommandsStatus() { return m_bSpoofCommandsStatus; };
    BOOL TestAndSetSpoofCommandsStatus(BOOL bStatus);

    void SetLastModemEvent(int value) { m_LastModemEvent = value; }
    int GetLastModemEvent() { return m_LastModemEvent; }

    void SetModemOffInFlightModeState(BOOL bValue) { m_bModemOffInFlightMode = bValue; };
    BOOL GetModemOffInFlightModeState() { return m_bModemOffInFlightMode; };

    void SetManualNetworkSearchOn(BOOL bIsManualSearchOn)
    {
        m_bIsManualNetworkSearchOn = bIsManualSearchOn;
    }
    BOOL IsManualNetworkSearchOn() { return m_bIsManualNetworkSearchOn; };

    void SetDataSuspended(BOOL bIsSuspended) { m_bIsDataSuspended = bIsSuspended; };
    BOOL IsDataSuspended() { return m_bIsDataSuspended; };

    void SetClearPendingCHLDs(BOOL bIsClearPendingCHLDs)
    {
        m_bIsClearPendingCHLD = bIsClearPendingCHLDs;
    }
    BOOL IsClearPendingCHLD() { return m_bIsClearPendingCHLD; };

    void SetFastDormancyMode(int fastDormancyMode) { m_FastDormancyMode = fastDormancyMode; };
    int GetFastDormancyMode() { return m_FastDormancyMode; };

    void SetMTU(UINT32 uiMTU) { m_uiMTU = uiMTU; };
    UINT32 GetMTU() { return m_uiMTU; };

    void SetVoiceCapable(BOOL bIsVoiceCapable) { m_bVoiceCapable =  bIsVoiceCapable; }
    BOOL IsVoiceCapable() { return m_bVoiceCapable; }

    void SetDataCapable(BOOL bIsDataCapable) { m_bDataCapable =  bIsDataCapable; }
    BOOL IsDataCapable() { return m_bDataCapable; }

    void SetSmsOverCSCapable(BOOL bIsSmsOverCSCapable)
    {
        m_bSmsOverCSCapable =  bIsSmsOverCSCapable;
    }
    BOOL IsSmsOverCSCapable() { return m_bSmsOverCSCapable; }

    void SetSmsOverPSCapable(BOOL bIsSmsOverPSCapable)
    {
        m_bSmsOverPSCapable =  bIsSmsOverPSCapable;
    }
    BOOL IsSmsOverPSCapable() { return m_bSmsOverPSCapable; }

    void SetSmsCapable(BOOL bIsSmsCapable) { m_bSmsCapable = bIsSmsCapable; }
    BOOL IsSmsCapable() { return m_bSmsCapable; }

    void SetStkCapable(BOOL bIsStkCapable) { m_bStkCapable =  bIsStkCapable; }
    BOOL IsStkCapable() { return m_bStkCapable; }

    void SetRestrictedMode(BOOL bIsRetrictedMode) { m_bRestrictedMode = bIsRetrictedMode; }
    BOOL IsRestrictedMode() { return m_bRestrictedMode; }

    void SetXDATASTATReporting(BOOL bEnable) { m_bXDATASTATEnabled =  bEnable; }
    BOOL IsXDATASTATReportingEnabled() { return m_bXDATASTATEnabled; }

    void SetIMSCapable(BOOL bEnable) { m_bIMSCapable = bEnable; }
    BOOL IsIMSCapable() { return m_bIMSCapable; }

    void SetSMSOverIPCapable(BOOL bSMSOverIPCapable) { m_bSMSOverIPCapable = bSMSOverIPCapable; }
    BOOL IsSMSOverIPCapable() { return m_bSMSOverIPCapable; }

    void SetIMSApCentric(BOOL bEnable) { m_bIMSApCentric = bEnable; }
    BOOL IsIMSApCentric() { return m_bIMSApCentric; }

    void SetSupportCGPIAF(BOOL bSupportCGPIAF) { m_bSupportCGPIAF =  bSupportCGPIAF; }
    BOOL IsSupportCGPIAF() { return m_bSupportCGPIAF; }

    void SetSignalStrengthReporting(BOOL bEnable) { m_bSignalStrengthReporting = bEnable; }
    BOOL IsSignalStrengthReportEnabled() { return m_bSignalStrengthReporting; }

    void SetCellInfoEnabled(BOOL bCellInfoEnabled) { m_bCellInfoEnabled = bCellInfoEnabled; }
    BOOL IsCellInfoEnabled() { return m_bCellInfoEnabled; }

    void SaveCEER(const char* pszData);
    const char* GetLastCEER() { return m_szLastCEER; }

    void SaveNetworkData(LAST_NETWORK_DATA_ID id, const char* pszData);
    const char* GetNetworkData(LAST_NETWORK_DATA_ID id) { return m_szLastNetworkData[id]; }

    void SetTimeoutCmdInit(UINT32 uiCmdInit) { m_uiTimeoutCmdInit = uiCmdInit; };
    UINT32 GetTimeoutCmdInit()     { return m_uiTimeoutCmdInit; };
    void SetTimeoutAPIDefault(UINT32 uiAPIDefault) { m_uiTimeoutAPIDefault = uiAPIDefault; };
    UINT32 GetTimeoutAPIDefault()  { return m_uiTimeoutAPIDefault; };
    void SetTimeoutWaitForInit(UINT32 uiWaitForInit ) { m_uiTimeoutWaitForInit = uiWaitForInit; };
    UINT32 GetTimeoutWaitForInit() { return m_uiTimeoutWaitForInit; };
    void SetTimeoutThresholdForRetry(UINT32 uiThresholdForRetry)
    {
         m_uiTimeoutThresholdForRetry = uiThresholdForRetry;
    }
    UINT32 GetTimeoutThresholdForRetry() { return m_uiTimeoutThresholdForRetry; };

    BOOL IsSetupDataCallAllowed(int& retryTime);

    void SetDtmfState(UINT32 uiDtmfState);
    UINT32 TestAndSetDtmfState(UINT32 uiDtmfState);
    UINT32 GetDtmfState();

    UINT32 GetCellInfoListRate() { return m_nCellInfoListRate; };
    void SetCellInfoListRate(UINT32 uiRate) { m_nCellInfoListRate = uiRate; };
    BOOL IsCellInfoTimerRunning() { return m_bIsCellInfoTimerRunning; };
    void SetCellInfoTimerRunning(BOOL aValue) { m_bIsCellInfoTimerRunning = aValue; };
    BOOL updateCellInfoCache(P_ND_N_CELL_INFO_DATA pData, const int itemCount)
    {
        return m_CellInfoCache.updateCache(pData, itemCount);
    }
    BOOL updateCellInfoCache(P_ND_N_CELL_INFO_DATA_V2 pData, const int itemCount)
    {
        return m_CellInfoCache.updateCache(pData, itemCount);
    }
    BOOL getCellInfo(P_ND_N_CELL_INFO_DATA pRetData, int& itemCount)
    {
       return m_CellInfoCache.getCellInfo(pRetData, itemCount);
    }
    BOOL getCellInfo(P_ND_N_CELL_INFO_DATA_V2 pRetData, int& itemCount)
    {
       return m_CellInfoCache.getCellInfo(pRetData, itemCount);
    }
    bool IsCellInfoCacheEmpty() { return m_CellInfoCache.IsCellInfoCacheEmpty(); }

    void SetTempOoSNotificationReporting(BOOL bEnable) { m_bTempOoSNotifReporting = bEnable; }
    BOOL IsTempOoSNotificationEnabled() { return m_bTempOoSNotifReporting; }

    BOOL TestAndSetDataCleanupStatus(BOOL bCleanupStatus);

    // This function will return true if sys.shutdown.requested is set to 0 or 1
    BOOL IsPlatformShutDownRequested();

    BOOL IsRadioRequestPending() { return m_bRadioRequestPending; }

    int GetRequestedRadioPower() { return m_RequestedRadioPower; }
    void SetRequestedRadioPower(int power) { m_RequestedRadioPower = power; }

    int GetRadioOffReason() { return m_RadioOffReason; }
    void SetRadioOffReason(int reason) { m_RadioOffReason = reason; }

    CEvent* GetRadioStateChangedEvent() { return m_pRadioStateChangedEvent; }

    int GetCnapCniValidity() { return static_cast<int>(m_uiCnapCniValidity); }
    void SetCnapCniValidity(UINT32 uiValidity) { m_uiCnapCniValidity = uiValidity; }
    int GetNumberCliValidity() { return static_cast<int>(m_uiNumberCliValidity); }
    void SetNumberCliValidity(UINT32 uiValidity) { m_uiNumberCliValidity = uiValidity; }

    char* GetCnapName() { return m_szCnapName; }
    void SetCnapName(const char* pszName);
    char* GetNumber() { return m_szNumber; }
    void SetNumber(const char* pszNumber);

    void ResetCnapParameters();
    void ResetNumberParameters();

    // Resets all the internal states to default values
    void ResetInternalStates();

    RIL_RESULT_CODE RequestSimPinRetryCount(RIL_Token rilToken, void* pData, size_t datalen,
                                            int reqId = 0,
                                            PFN_TE_POSTCMDHANDLER pPostCmdHandlerFcn = NULL);
    RIL_RESULT_CODE ParseSimPinRetryCount(RESPONSE_DATA& rRspData);

    /*
     * Create request for Extended Error Report for Location Update Reject
     * during CS registration (called internally)
     */
    BOOL RequestQueryNEER(UINT32 uiChannel, RIL_Token rilToken, int reqId);
    RIL_RESULT_CODE ParseQueryNEER(RESPONSE_DATA& rRspData);

    /*
     * Post Command handler function for valid ril requests and also for internal requests.
     * This function only completes the ril requests upon valid ril token. No cleanup done
     * for any specific request.
     */
    void PostCmdHandlerCompleteRequest(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for Get SIM status request.
     *
     * Upon Success, request is completed
     *
     * Upon Failure, RIL_CardStatus_v6 filled with values based
     * on the error codes. Also sets the result code to SUCCESS as
     * expected by android telephony framework.
     *
     */
    void PostGetSimStatusCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for the SIM PIN/PIN2/PUK/PUK2/Facility
     * lock requests.
     *
     * Upon success or failure, pin retry count is requested from modem
     * Upon failure, error codes are mapped to the RIL error codes
     */
    void PostSimPinCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for the PIN2/PUK2 lock requests.
     *
     * Upon success, completes the request
     * Upon failure, error codes are mapped to the RIL error codes and completes the request.
     */
    void PostSimPin2CmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION
     *
     * Upon success, completes the request and also triggers the SIM UNLOCKED event.
     * Upon failure, error codes are mapped to the RIL error codes and completes the request.
     */
    void PostNtwkPersonalizationCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_GET_CURRENT_CALLS
     *
     * Upon success/failure, completes the request. If the failure is due to
     * SIM LOCKED OR ABSENT, then completes the request as SUCCESS
     */
    void PostGetCurrentCallsCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_DIAL and RIL_REQUEST_DIAL_VT
     *
     * Upon success/failure, completes the request and also completes
     * RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED.
     */
    void PostDialCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_HANGUP,
     * RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND, RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND,
     * and RIL_REQUEST_HANGUP_VT.
     *
     * Upon success/failure, completes pending DTMF requests with RIL_E_GENERIC_FAILURE error.
     */
    void PostHangupCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE,
     * RIL_REQUEST_SWITCH_HOLDING_AND_ACTIVE.
     *
     * Upon Success, completes the request and also completes pending DTMF requests with
     * RIL_E_GENERIC_FAILURE error.
     * Upon Failure, completes the request and also completes pending switch, DTMF requests with
     * RIL_E_GENERIC_FAILURE error.
     */
    void PostSwitchHoldingAndActiveCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_CONFERENCE
     *
     * Upon success/failure, completes pending DTMF requests with RIL_E_GENERIC_FAILURE error.
     */
    void PostConferenceCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_VOICE_REGISTRATION_STATE,
     * RIL_REQUEST_DATA_REGISTRATION_STATE, RIL_REQUEST_OPERATOR and
     * RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE request.
     *
     * Upon success/failure, completes the request and also completes identical requests
     * in the command queue.
     */
    void PostNetworkInfoCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_OPERATOR.
     *
     * Upon success, completes the request
     * Upon failure, error codes are mapped to the RIL error codes and completes the request.
     */
    void PostOperator(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_RADIO_POWER.
     *
     * Upon success, completes the request
     * Upon failure,
     *      If request state is airplane mode, then always complete success.
     *
     * Upon success/failure, reset all internal states and cleanup all data connections.
     */
    void PostRadioPower(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_SEND_SMS and
     * RIL_REQUEST_SEND_SMS_EXPECT_MORE request.
     *
     * Upon success, completes the request
     * Upon failure, error codes are mapped to the RIL error codes and completes the request.
     */
    void PostSendSmsCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler functions for the RIL_REQUEST_SETUP_DATA_CALL
     * ril request. Post processing is done at modem level.
     */
    void PostSetupDataCallCmdHandler(POST_CMD_HANDLER_DATA& rData);
    void PostPdpContextActivateCmdHandler(POST_CMD_HANDLER_DATA& rData);
    void PostQueryIpAndDnsCmdHandler(POST_CMD_HANDLER_DATA& rData);
    void PostEnterDataStateCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_SIM_IO
     *
     * Upon success, completes the request
     * Upon failure, error codes are mapped to the RIL error codes and completes the request.
     */
    void PostSimIOCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler functions for the RIL_REQUEST_DEACTIVATE_DATA_CALL
     * ril request. Post processing is done at modem level.
     */
    void PostDeactivateDataCallCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_SET_FACILITY_LOCK
     *
     * Upon success or failure, pin retry count is requested from modem for SC and FD locks
     * Upon failure, error codes are mapped to the RIL error codes.
     */
    void PostSetFacilityLockCmdHandler(POST_CMD_HANDLER_DATA& rData);

    // Sets the g_bIsManualNetworkSearchOngoing to false
    void PostQueryAvailableNetworksCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_DTMF_START
     *
     * Upon success, completes the request
     * Upon failure, completes the current request and pending requests.
     *               Also, DTMF state is updated.
     */
    void PostDtmfStart(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for RIL_REQUEST_DTMF_STOP
     *
     * Upon success, completes the request
     * Upon failure, completes the current request and pending requests.
     *               Also, DTMF state is updated.
     */
    void PostDtmfStop(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for the RIL_REQUEST_OEM_HOOK_STRINGS request.
     *
     * If the request is to power off the modem, then modem shutdown
     * request will be sent to MMGR and will wait for modem powered off event
     * to complete the request.
     *
     * For all other requests, request is completed.
     */
    void PostHookStrings(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for the RIL_REQUEST_WRITE_SMS_TO_SIM request.
     *
     * Upon success/failure, completes the request
     * If the failure is due to memory full, RIL_UNSOL_SIM_SMS_STORAGE_FULL is completed
     */
    void PostWriteSmsToSimCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for the RIL_REQUEST_GET_NEIGHBORING_CELL_IDS request.
     *
     * Upon success/failure, request and also any pending requestes are completed.
     */
    void PostGetNeighboringCellIDs(POST_CMD_HANDLER_DATA& rData);

    /*
     *
     * Post Command handler function for the RIL_REQUEST_SET_LOCATION_UPDATES request.
     *
     * Upon success/failure, completes the request
     */
    void PostSetLocationUpdates(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for the Silent PIN Entry request.
     *
     * Clears the cached pin on error, usecachedpin flag and
     * also notifies SIM status change to framework by completing
     * RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED.
     */
    void PostSilentPinRetryCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for the requests which require
     * lock retry count as response.
     *
     * Upon success, corresponding request is completed with the number of retries.
     * Upon failure, request is completed with number of retries set to -1
     */
    void PostSimPinRetryCount(POST_CMD_HANDLER_DATA& rData);

    /*
     * Post Command handler function for the RIL_REQUEST_SET_FACILITY_LOCK request.
     *
     * Upon success, request is completed with the number of retries set to the fetched
     *               retry count for SC and FD locks. For rest of the locks, number of
     *               retries set to -1
     * Upon failure, request is completed with number of retries set to -1
     */
    void PostFacilityLockRetryCount(POST_CMD_HANDLER_DATA& rData);

    void PostGetCellInfoList(POST_CMD_HANDLER_DATA& rData);

    /*
     *
     * Post Command handler function for the RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
     * and RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL requests.
     *
     * Upon success/failure, completes the request
     */
    void PostSetNetworkSelectionCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     *
     * Post Command handler function for the RIL_REQUEST_SET_INITIAL_ATTACH_APN request.
     *
     * Upon success/failure, completes the request and also restores the saved network
     * selection mode.
     */
    void PostSetInitialAttachApnCmdHandler(POST_CMD_HANDLER_DATA& rData);

    /*
     * Gets the list of active data calls.
     *
     * pPDPListData - Contains the list of active data calls
     * Number of active data calls is returned.
     */
    int GetActiveDataCallInfoList(P_ND_PDP_CONTEXT_DATA pPDPListData);

    /*
     * Completes RIL_UNSOL_DATA_CALL_LIST_CHANGED with the list
     * of active data call information.
     */
    void CompleteDataCallListChanged();

    void SetCallDropReportingState(BOOL bValue) { m_bCallDropReporting = bValue; }
    BOOL GetCallDropReportingState() { return m_bCallDropReporting; }

    RIL_RESULT_CODE ParseDeactivateAllDataCalls(RESPONSE_DATA& rRspData);

    RIL_RESULT_CODE CreateIMSRegistrationReq(REQUEST_DATA& rReqData,
            const char** pszRequest,
            const UINT32 uiDataSize);

    RIL_RESULT_CODE CreateIMSConfigReq(REQUEST_DATA& rReqData,
            const char** pszRequest,
            const int nNumStrings);

    BOOL IsEPSRegistered();

    void SetDefaultPDNCid(UINT32 uiCid) { m_uiDefaultPDNCid = uiCid; }
    UINT32 GetDefaultPDNCid() { return m_uiDefaultPDNCid; }

    /*
     * Post Command handler function for the read default PDN
     * context parameters request.
     *
     * Upon success, data state of default PDN will be set to E_DATA_STATE_ACTIVATING.
     * Upon failure, no actions taken
     */
    void PostReadDefaultPDNContextParams(POST_CMD_HANDLER_DATA& rData);

    // Parser function for setting up of default PDN.
    RIL_RESULT_CODE ParseSetupDefaultPDN(RESPONSE_DATA& rRspData);

    // Post command handler for setting up of default PDN.
    void PostSetupDefaultPDN(POST_CMD_HANDLER_DATA& rData);

    /*
     * This function will be called on basic initialisation completion of
     * all the channels. Trigger commands which return responses other than
     * OK/CME ERROR.
     */
    void HandleChannelsBasicInitComplete();

    /*
     * This function will be called on unlock initialisation completion of
     * all the channels. Trigger commands which return responses other than
     * OK/CME ERROR.
     */
    void HandleChannelsUnlockInitComplete();

    // Sends AT+CPMS? command to the modem
    void TriggerQuerySimSmsStoreStatus();

    // Parser function for sim status query.
    RIL_RESULT_CODE ParseSimStateQuery(RESPONSE_DATA& rRspData);

    UINT32 GetPinCacheMode() { return m_uiPinCacheMode; }
    void SetPinCacheMode(UINT32 uiPinCacheMode) { m_uiPinCacheMode = uiPinCacheMode; }
    // store for ciphering indication.
    // 0x00000001 = CS ciphering ON
    // 0x00000002 = PS ciphering ON
    UINT32 GetCurrentCipheringStatus() { return m_CurrentCipheringStatus; }
    void SetCurrentCipheringStatus(UINT32 status) { m_CurrentCipheringStatus = status; }

    void HandleCellBroadcastActivation();

    // Post handler for internal DTMF stop request
    void PostInternalDtmfStopReq(POST_CMD_HANDLER_DATA& rData);

    /*
     * If the device is LTE registered, then network initiated contexts will be accepted.
     * If the device is not LTE registered; then network initiated contexts will be rejected.
     */
    void AcceptOrRejectNwInitiatedContext();

    // Returns the signal strength reporting string used to enable signal strength URC
    const char* GetSignalStrengthReportingStringAlloc();

    RIL_SignalStrength* ParseXCESQ(const char*& rszPointer, const BOOL bUnsolicited);

    // Resets the sim card status cache
    void ResetCardStatus(BOOL bForceReset);

    void QueryUiccInfo();
    RIL_RESULT_CODE ParseQueryActiveApplicationType(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseQueryAvailableApplications(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseQueryIccId(RESPONSE_DATA& rRspData);

    void PostSimStateQuery(POST_CMD_HANDLER_DATA& rData);
    void HandleSimState(const UINT32 uiSIMState, BOOL& bNotifySimStatusChange);
    void SetRefreshWithUsimInitOn(BOOL bOngoing);

    void PostInternalOpenLogicalChannel(POST_CMD_HANDLER_DATA& rData);

    RIL_RESULT_CODE ParseDeregister(RESPONSE_DATA& rRspData);
    void PostDeregisterCmdHandler(POST_CMD_HANDLER_DATA& rData);

    CMutex* GetDataChannelRefCountMutex() { return m_pDataChannelRefCountMutex; }
    void GetPreviousGprsRegInfo(S_REG_INFO& previousRegInfo);

    // following functions are only for modem Rel.10+ with the new 3GPP USAT interface
    void SetProfileDownloadForNextUiccStartup(UINT32 uiDownload, UINT32 uiReporting);
    void ConfigureUsatProfileDownload(UINT32 uiDownload, UINT32 uiReporting);
    void PostConfigureUsatProfileDownloadHandler(POST_CMD_HANDLER_DATA& rData);

    RIL_RESULT_CODE ParseQueryUiccState(RESPONSE_DATA& rRspData);
    void PostQueryUiccStateHandler(POST_CMD_HANDLER_DATA& rData);

    RIL_RESULT_CODE ParseReadUsatProfiles(RESPONSE_DATA& rRspData);

    void WriteUsatProfiles(const char* pszTeProfile, const BOOL isTeWriteNeeded,
            const char* pszMtProfile, const BOOL isMtWriteNeeded);
    void WriteUsatProfile(const UINT32 uiProfileStorage, const char* pszProfile);
    RIL_RESULT_CODE ParseWriteUsatProfile(RESPONSE_DATA& rRspData);
    void PostWriteUsatProfileHandler(POST_CMD_HANDLER_DATA& rData);

    void ResetUicc();
    void NotifyUiccReady();

    void EnableProfileFacilityHandling();

    void SendModemDownToUsatSM();

    int GetScreenState() { return m_ScreenState; }

    RIL_RESULT_CODE SetCsgAutomaticSelection(REQUEST_DATA& reqData);
    RIL_RESULT_CODE GetCsgCurrentState(REQUEST_DATA& reqData);
    RIL_RESULT_CODE ParseXCSG(const char* pszRsp, RESPONSE_DATA& rspData);

    void SetImsRegistrationStatus(UINT32 uiRegStatus) { m_uiImsRegStatus = uiRegStatus; }
    bool IsImsRegistered() { return (m_uiImsRegStatus == IMS_REGISTERED); }

    const char* GetEnableFetchingString();
    const char* GetSiloVoiceURCInitString();

    RIL_RESULT_CODE ParseAtSecStateInfoRequest(RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE GetCnapState(REQUEST_DATA& reqData);
    RIL_RESULT_CODE ParseQueryCnap(const char* pszRsp, RESPONSE_DATA& rspData);

    BOOL ParseXREGNetworkInfo(const char*& pszPointer, const BOOL isUnSolicited);

    const char* GetReadCellInfoString();
    bool IsRegistered();
    bool IsRegistered(int status);
    bool IsRegisteredBasedOnRegType(int regType);

    bool TestAndSetNetworkStateChangeTimerRunning(bool bTimerRunning);

    RIL_RESULT_CODE CreateSetAdaptiveClockingReq(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    RIL_RESULT_CODE CreateGetAdaptiveClockingFreqInfo(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    RIL_RESULT_CODE ParseGetAdaptiveClockingFreqInfo(const char* pszRsp, RESPONSE_DATA& rspData);

    RIL_RESULT_CODE CreateSetRegStatusAndBandReport(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    RIL_RESULT_CODE CreateSetCoexReport(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    RIL_RESULT_CODE CreateSetCoexWlanParams(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    RIL_RESULT_CODE CreateSetCoexBtParams(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);

    bool IsCoexReportActivated();

private:
    UINT32 m_uiModemType;

    BOOL m_bCSStatusCached;
    BOOL m_bPSStatusCached;
    S_ND_GPRS_REG_STATUS m_sPSStatus;
    S_ND_REG_STATUS m_sCSStatus;
    S_ND_GPRS_REG_STATUS m_sEPSStatus;
    CellInfoCache m_CellInfoCache;

    // Flag used to store setup data call status
    BOOL m_bIsSetupDataCallOngoing;

    // Flag used to store spoof commands status
    BOOL m_bSpoofCommandsStatus;

    /*
     * Flag is used to know the current modem status.
     */
    int m_LastModemEvent;

    /*
     * Flag is used to know if modem shutdown is available during flightmode.
     * If TRUE, RIL will ask MMgr to power off the mode when flightmode is
     * enabled.
     */
    BOOL m_bModemOffInFlightMode;

    // Flag used to store the location update requested status
    int m_enableLocationUpdates;

    /*
     * Flag to indicate whether the Rapid ril is in restricted mode.
     * Restricted mode means no telephony functionalities possible due to
     * communication issue with MMGR.
     */
    BOOL m_bRestrictedMode;

    // Set to true if the radio on/off request is pending
    BOOL m_bRadioRequestPending;

    /*
     * Flag is used to store the manual network search status
     * If TRUE, RIL_REQUEST_SETUP_DATA_CALL will be rejected with
     * suggestedRetryTime set to 10seconds.
     */
    BOOL m_bIsManualNetworkSearchOn;

    /*
     * Flag is used to store the Data SUSPENDED/RESUME status.
     * Upon incoming WAP PUSH SMS, MMS applications requests for MMS
     * connection establishment. Since the data is in suspended
     * state on modem side due to SMS acknowledgment, RIL_REQUEST_SETUP_DATA_CALL
     * for MMS connection is not processed. So, when there is a
     * RIL_REQUEST_SETUP_DATA_CALL on data SUSPENDED state, reject
     * RIL_REQUEST_SETUP_DATA_CALL with suggestedRetryTime set to 3seconds.
     */
    BOOL m_bIsDataSuspended;

    /*
     * Flag is used to cancel the pending CHLD requests in ril when
     * the call is disconnected.
     */
    BOOL m_bIsClearPendingCHLD;

    const char* PrintRegistrationInfo(char* szRegInfo) const;
    const char* PrintGPRSRegistrationInfo(char* szGPRSInfo) const;
    const char* PrintRAT(char* szRAT) const;

    // Function to determine whether the error code falls under SMS retry case
    BOOL isRetryPossible(UINT32 uiErrorCode);

    // Function to cleanup the request data
    void CleanRequestData(REQUEST_DATA& rReqData);

    // Function to determine whether the SIMIO request is for FDN related SIM files
    BOOL isFDNRequest(int fileId);

    LONG GetCsRegistrationState(char* pCsRegState);
    LONG GetPsRegistrationState(char* pPsRegState);
    LONG GetCurrentAct();

    // initial value of Modem Autonomous Fast Dormancy (MAFD) mode
    static const int FAST_DORMANCY_MODE_DEFAULT = 2;
    int m_FastDormancyMode;

    // MTU size of 1358 is the recommended value in 3GPP 23.060 to support
    // IPV4 and IPV6 traffic.
    static const UINT32 MTU_SIZE = 1358;
    UINT32 m_uiMTU;

    BOOL m_bVoiceCapable;
    BOOL m_bDataCapable;
    BOOL m_bSmsOverCSCapable;
    BOOL m_bSmsOverPSCapable;
    BOOL m_bSmsCapable;
    BOOL m_bStkCapable;
    BOOL m_bXDATASTATEnabled;
    BOOL m_bIMSCapable;
    BOOL m_bSMSOverIPCapable;
    BOOL m_bIMSApCentric;
    BOOL m_bSupportCGPIAF;  // support CGPIAF in IMC IPV6 AT cmds
    BOOL m_bSignalStrengthReporting;
    BOOL m_bCellInfoEnabled;

    // Timeouts (in milliseconds)
    static const UINT32 TIMEOUT_INITIALIZATION_COMMAND = 5000;
    static const UINT32 TIMEOUT_API_DEFAULT            = 10000;
    static const UINT32 TIMEOUT_WAITFORINIT            = 10000;
    static const UINT32 TIMEOUT_THRESHOLDFORRETRY      = 10000;
    UINT32 m_uiTimeoutCmdInit;
    UINT32 m_uiTimeoutAPIDefault;
    UINT32 m_uiTimeoutWaitForInit;
    UINT32 m_uiTimeoutThresholdForRetry;

    UINT32 m_uiDtmfState;
    CMutex* m_pDtmfStateAccess;

    // SCREEN_STATE_UNKNOWN(-1), SCREEN_STATE_OFF(0), SCREEN_STATE_ON(1)
    int m_ScreenState;

    PREF_NET_TYPE_REQ_INFO* m_pPrefNetTypeReqInfo;

    // Delay Set Preferred Network Type request when Radio state is Off until On
    RIL_RESULT_CODE DelaySetPrefNetTypeRequest(void* pData, size_t datalen, RIL_Token hRilToken);
    void SendSetPrefNetTypeRequest();

    // RADIO_POWER_UNKNOWN(-1), RADIO_POWER_OFF(0), RADIO_POWER_OFF(1)
    int m_RequestedRadioPower;

    // RADIO_OFF_REASON_UNSPECIFIED(0),RADIO_OFF_REASON_SHUTDOWN(1) and
    // RADIO_OFF_REASON_AIRPLANE_MODE(2)
    int m_RadioOffReason;

    CEvent* m_pRadioStateChangedEvent;

    // For telephony crashtool report
    BOOL m_bCallDropReporting;

    // Default PDN context id
    UINT32 m_uiDefaultPDNCid;

    char m_szCachedLac[REG_STATUS_LENGTH];
    char m_szCachedCid[REG_STATUS_LENGTH];

    char m_szNewLine[3];

    char m_szLastNetworkData[LAST_NETWORK_DATA_COUNT][MAX_NETWORK_DATA_SIZE];
    char m_szLastCEER[255];
    char m_cTerminator;

    BOOL m_bDataCleanupStatus;
    CMutex* m_pDataCleanupStatusLock;

    UINT32 m_nCellInfoListRate;
    BOOL m_bIsCellInfoTimerRunning;
    UINT32 m_CurrentCipheringStatus;

    UINT32 m_uiPinCacheMode;

    BOOL m_bCbsActivationTimerRunning;
    int m_CbsActivate;

    BOOL m_bTempOoSNotifReporting;
    CMutex* m_pDataChannelRefCountMutex;

    UINT32 m_uiCnapCniValidity;
    char m_szCnapName[MAX_CNAP_NAME_SIZE];

    UINT32 m_uiNumberCliValidity;
    char m_szNumber[MAX_BUFFER_SIZE];

    UINT32 m_uiImsRegStatus;

    S_NETWORK_REG_STATE_INFO m_sNetworkRegStateInfo;
    bool m_bNetworkStateChangeTimerRunning;
    CMutex* m_pNetworkStateChangeTimerStatusLock;

    int m_ProductConfig;

    void CompleteGetSimStatusRequest(RIL_Token hRilToken);
    void FreeCardStatusPointers(RIL_CardStatus_v6& cardStatus);
    void MapCsRegistrationState(int& regState);
    void MapRegistrationRejectCause(int causeType, int& rejectCause);

    void SendAtSecStateInfoRequest();
    const char* GetPrintString(int definitionId);
    BOOL IsBuildTypeEngUserDebug();
    void TriggerRestrictedModeEvent();
    bool NeedGetCellInfoOnCellChange();
};

#endif
