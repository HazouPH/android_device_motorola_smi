////////////////////////////////////////////////////////////////////////////
// te_base.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CTEBase class which handles all requests and
//    basic behavior for responses
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_TE_BASE_H
#define RRIL_TE_BASE_H

#include "types.h"
#include "rril_OEM.h"
#include "rril.h"
#include "radio_state.h"
#include <utils/Vector.h>
#include "oemhookids.h"

class CChannel_Data;
class CTE;
class CInitializer;

class CTEBase
{
protected:
    CTE& m_cte;
    char m_cTerminator;
    char m_szNewLine[3];
    CInitializer* m_pInitializer;
    char m_szNetworkInterfaceNamePrefix[MAX_BUFFER_SIZE];
    char m_szPIN[MAX_PIN_SIZE];
    android::Vector<RIL_GSM_BroadcastSmsConfigInfo> m_vBroadcastSmsConfigInfo;
    // This tracks the radio state and handles notifications
    CRadioState m_RadioState;

    S_PIN_RETRY_COUNT m_PinRetryCount;
    RIL_PinState m_ePin2State;
    char m_szUICCID[PROPERTY_VALUE_MAX];
    RIL_CardStatus_v6 m_CardStatusCache;
    S_ND_SIM_APP_LIST_DATA m_SimAppListData;

    CEvent* m_pDtmfStopReqEvent;
    BOOL m_bReadyForAttach;
    BOOL m_bRefreshWithUSIMInitOn;

    CEvent* m_pUiccOpenLogicalChannelEvent;
    S_NETWORK_SELECTION_MODE_PARAMS m_NetworkSelectionModeParams;
    S_INITIAL_ATTACH_APN_PARAMS m_InitialAttachApnParams;

    bool m_bRegStatusAndBandIndActivated;
    sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND m_sRegStatusAndBandInfo;

    bool m_bNeedGetInfoOnCellChange;

    android::Vector<S_DATA_PROFILE_INFO> m_vDataProfileInfos;

    static const char* PDPTYPE_IPV4V6;
    static const char* PDPTYPE_IPV6;
    static const char* PDPTYPE_IP;
    static const char* UNREGISTERED_SEARCHING;

    bool m_bCoexRegStatusAndBandIndActivated;
    bool m_bCoexReportActivated;

public:
    CTEBase(CTE& cte);
    virtual ~CTEBase();

private:
    CTEBase();

public:
    virtual CInitializer* GetInitializer() = 0;

    virtual char* GetBasicInitCommands(UINT32 uiChannelType);
    virtual char* GetUnlockInitCommands(UINT32 uiChannelType);

    virtual BOOL IsRequestSupported(int requestId);

    // RIL_REQUEST_GET_SIM_STATUS
    virtual RIL_RESULT_CODE CoreGetSimStatus(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseGetSimStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_SIM_PIN
    virtual RIL_RESULT_CODE CoreEnterSimPin(REQUEST_DATA& rReqData,
                                                       void* pData,
                                                       UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseEnterSimPin(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_SIM_PUK
    virtual RIL_RESULT_CODE CoreEnterSimPuk(REQUEST_DATA& rReqData,
                                                       void* pData,
                                                       UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseEnterSimPuk(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_SIM_PIN2
    virtual RIL_RESULT_CODE CoreEnterSimPin2(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseEnterSimPin2(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_SIM_PUK2
    virtual RIL_RESULT_CODE CoreEnterSimPuk2(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseEnterSimPuk2(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CHANGE_SIM_PIN
    virtual RIL_RESULT_CODE CoreChangeSimPin(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseChangeSimPin(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CHANGE_SIM_PIN2
    virtual RIL_RESULT_CODE CoreChangeSimPin2(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseChangeSimPin2(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION
    virtual RIL_RESULT_CODE CoreEnterNetworkDepersonalization(REQUEST_DATA& rReqData,
                                                                         void* pData,
                                                                         UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseEnterNetworkDepersonalization(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_CURRENT_CALLS
    virtual RIL_RESULT_CODE CoreGetCurrentCalls(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseGetCurrentCalls(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DIAL
    virtual RIL_RESULT_CODE CoreDial(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseDial(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_IMSI
    virtual RIL_RESULT_CODE CoreGetImsi(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetImsi(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_HANGUP
    virtual RIL_RESULT_CODE CoreHangup(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseHangup(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND
    virtual RIL_RESULT_CODE CoreHangupWaitingOrBackground(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseHangupWaitingOrBackground(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND
    virtual RIL_RESULT_CODE CoreHangupForegroundResumeBackground(REQUEST_DATA& rReqData,
                                                                            void* pData,
                                                                            UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseHangupForegroundResumeBackground(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE
    // RIL_REQUEST_SWITCH_HOLDING_AND_ACTIVE
    virtual RIL_RESULT_CODE CoreSwitchHoldingAndActive(REQUEST_DATA& rReqData,
                                                                  void* pData,
                                                                  UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSwitchHoldingAndActive(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CONFERENCE
    virtual RIL_RESULT_CODE CoreConference(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseConference(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_UDUB
    virtual RIL_RESULT_CODE CoreUdub(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseUdub(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_LAST_CALL_FAIL_CAUSE
    virtual RIL_RESULT_CODE CoreLastCallFailCause(REQUEST_DATA& rReqData,
                                                             void* pData,
                                                             UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseLastCallFailCause(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIGNAL_STRENGTH
    virtual RIL_RESULT_CODE CoreSignalStrength(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSignalStrength(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_VOICE_REGISTRATION_STATE
    virtual RIL_RESULT_CODE CoreRegistrationState(REQUEST_DATA& rReqData,
                                                             void* pData,
                                                             UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseRegistrationState(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DATA_REGISTRATION_STATE
    virtual RIL_RESULT_CODE CoreGPRSRegistrationState(REQUEST_DATA& rReqData,
                                                                 void* pData,
                                                                 UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseGPRSRegistrationState(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_OPERATOR
    virtual RIL_RESULT_CODE CoreOperator(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseOperator(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_RADIO_POWER
    virtual RIL_RESULT_CODE CoreRadioPower(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseRadioPower(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DTMF
    virtual RIL_RESULT_CODE CoreDtmf(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseDtmf(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SEND_SMS
    virtual RIL_RESULT_CODE CoreSendSms(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSendSms(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SEND_SMS_EXPECT_MORE
    virtual RIL_RESULT_CODE CoreSendSmsExpectMore(REQUEST_DATA& rReqData,
                                                             void* pData,
                                                             UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSendSmsExpectMore(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SETUP_DATA_CALL
    virtual RIL_RESULT_CODE CoreSetupDataCall(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize,
                                                         UINT32& uiCID);

    virtual RIL_RESULT_CODE ParseSetupDataCall(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_IO
    virtual RIL_RESULT_CODE CoreSimIo(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimIo(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SEND_USSD
    virtual RIL_RESULT_CODE CoreSendUssd(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSendUssd(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CANCEL_USSD
    virtual RIL_RESULT_CODE CoreCancelUssd(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCancelUssd(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_CLIR
    virtual RIL_RESULT_CODE CoreGetClir(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetClir(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_CLIR
    virtual RIL_RESULT_CODE CoreSetClir(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetClir(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
    virtual RIL_RESULT_CODE CoreQueryCallForwardStatus(REQUEST_DATA& rReqData,
                                                                  void* pData,
                                                                  UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseQueryCallForwardStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_CALL_FORWARD
    virtual RIL_RESULT_CODE CoreSetCallForward(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSetCallForward(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_CALL_WAITING
    virtual RIL_RESULT_CODE CoreQueryCallWaiting(REQUEST_DATA& rReqData,
                                                            void* pData,
                                                            UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseQueryCallWaiting(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_CALL_WAITING
    virtual RIL_RESULT_CODE CoreSetCallWaiting(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSetCallWaiting(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SMS_ACKNOWLEDGE
    virtual RIL_RESULT_CODE CoreSmsAcknowledge(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSmsAcknowledge(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_IMEI
    virtual RIL_RESULT_CODE CoreGetImei(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetImei(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_IMEISV
    virtual RIL_RESULT_CODE CoreGetImeisv(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetImeisv(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ANSWER
    virtual RIL_RESULT_CODE CoreAnswer(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseAnswer(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DEACTIVATE_DATA_CALL
    virtual RIL_RESULT_CODE CoreDeactivateDataCall(REQUEST_DATA& rReqData,
                                                              void* pData,
                                                              UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseDeactivateDataCall(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_FACILITY_LOCK
    virtual RIL_RESULT_CODE CoreQueryFacilityLock(REQUEST_DATA& rReqData,
                                                             void* pData,
                                                             UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseQueryFacilityLock(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_FACILITY_LOCK
    virtual RIL_RESULT_CODE CoreSetFacilityLock(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSetFacilityLock(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CHANGE_BARRING_PASSWORD
    virtual RIL_RESULT_CODE CoreChangeBarringPassword(REQUEST_DATA& rReqData,
                                                                 void* pData,
                                                                 UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseChangeBarringPassword(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE
    virtual RIL_RESULT_CODE CoreQueryNetworkSelectionMode(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseQueryNetworkSelectionMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
    virtual RIL_RESULT_CODE CoreSetNetworkSelectionAutomatic(REQUEST_DATA& rReqData,
                                                                        void* pData,
                                                                        UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSetNetworkSelectionAutomatic(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL
    virtual RIL_RESULT_CODE CoreSetNetworkSelectionManual(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSetNetworkSelectionManual(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_AVAILABLE_NETWORKS
    virtual RIL_RESULT_CODE CoreQueryAvailableNetworks(REQUEST_DATA& rReqData,
                                                                  void* pData,
                                                                  UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseQueryAvailableNetworks(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DTMF_START
    virtual RIL_RESULT_CODE CoreDtmfStart(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseDtmfStart(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DTMF_STOP
    virtual RIL_RESULT_CODE CoreDtmfStop(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseDtmfStop(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_BASEBAND_VERSION
    virtual RIL_RESULT_CODE CoreBasebandVersion(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseBasebandVersion(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SEPARATE_CONNECTION
    virtual RIL_RESULT_CODE CoreSeparateConnection(REQUEST_DATA& rReqData,
                                                              void* pData,
                                                              UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSeparateConnection(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_MUTE
    virtual RIL_RESULT_CODE CoreSetMute(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetMute(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_MUTE
    virtual RIL_RESULT_CODE CoreGetMute(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetMute(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_CLIP
    virtual RIL_RESULT_CODE CoreQueryClip(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseQueryClip(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE
    virtual RIL_RESULT_CODE CoreLastDataCallFailCause(REQUEST_DATA& rReqData,
                                                                 void* pData,
                                                                 UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseLastDataCallFailCause(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_RESET_RADIO
    virtual RIL_RESULT_CODE CoreResetRadio(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseResetRadio(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_OEM_HOOK_RAW
    virtual RIL_RESULT_CODE CoreHookRaw(REQUEST_DATA& rReqData,
                                                   void* pData,
                                                   UINT32 uiDataSize,
                                                   UINT32& uiRilChannel);

    virtual RIL_RESULT_CODE ParseHookRaw(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_OEM_HOOK_STRINGS
    virtual RIL_RESULT_CODE CoreHookStrings(REQUEST_DATA& rReqData,
                                                       void* pData,
                                                       UINT32 uiDataSize,
                                                       UINT32& uiRilChannel);

    virtual RIL_RESULT_CODE ParseHookStrings(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SCREEN_STATE
    virtual RIL_RESULT_CODE CoreScreenState(REQUEST_DATA& rReqData,
                                                       void* pData,
                                                       UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseScreenState(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION
    virtual RIL_RESULT_CODE CoreSetSuppSvcNotification(REQUEST_DATA& rReqData,
                                                                  void* pData,
                                                                  UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSetSuppSvcNotification(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_WRITE_SMS_TO_SIM
    virtual RIL_RESULT_CODE CoreWriteSmsToSim(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseWriteSmsToSim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DELETE_SMS_ON_SIM
    virtual RIL_RESULT_CODE CoreDeleteSmsOnSim(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseDeleteSmsOnSim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_BAND_MODE
    virtual RIL_RESULT_CODE CoreSetBandMode(REQUEST_DATA& rReqData,
                                                       void* pData,
                                                       UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseSetBandMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE
    virtual RIL_RESULT_CODE CoreQueryAvailableBandMode(REQUEST_DATA& rReqData,
                                                                  void* pData,
                                                                  UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseQueryAvailableBandMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_GET_PROFILE
    virtual RIL_RESULT_CODE CoreStkGetProfile(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseStkGetProfile(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SET_PROFILE
    virtual RIL_RESULT_CODE CoreStkSetProfile(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseStkSetProfile(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
    virtual RIL_RESULT_CODE CoreStkSendEnvelopeCommand(REQUEST_DATA& rReqData,
                                                                  void* pData,
                                                                  UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseStkSendEnvelopeCommand(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
    virtual RIL_RESULT_CODE CoreStkSendTerminalResponse(REQUEST_DATA& rReqData,
                                                                   void* pData,
                                                                   UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseStkSendTerminalResponse(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM
    virtual RIL_RESULT_CODE CoreStkHandleCallSetupRequestedFromSim(REQUEST_DATA& rReqData,
                                                                              void* pData,
                                                                              UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseStkHandleCallSetupRequestedFromSim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_EXPLICIT_CALL_TRANSFER
    virtual RIL_RESULT_CODE CoreExplicitCallTransfer(REQUEST_DATA& rReqData,
                                                                void* pData,
                                                                UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseExplicitCallTransfer(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
    virtual RIL_RESULT_CODE CoreSetPreferredNetworkType(REQUEST_DATA& rReqData,
                                                                   void* pData,
                                                                   UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetPreferredNetworkType(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
    virtual RIL_RESULT_CODE CoreGetPreferredNetworkType(REQUEST_DATA& rReqData,
                                                                   void* pData,
                                                                   UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetPreferredNetworkType(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_NEIGHBORING_CELL_IDS
    virtual RIL_RESULT_CODE CoreGetNeighboringCellIDs(REQUEST_DATA& rReqData,
                                                                 void* pData,
                                                                 UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetNeighboringCellIDs(RESPONSE_DATA& rRspData);

    virtual RIL_RESULT_CODE ParseNeighboringCellInfo(P_ND_N_CELL_DATA pCellData,
                                                            const char* pszRsp,
                                                            UINT32 uiIndex,
                                                            UINT32 uiMode);
    // RIL_REQUEST_SET_LOCATION_UPDATES
    virtual RIL_RESULT_CODE CoreSetLocationUpdates(REQUEST_DATA& rReqData,
                                                void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetLocationUpdates(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SET_SUBSCRIPTION
    virtual RIL_RESULT_CODE CoreCdmaSetSubscription(REQUEST_DATA& rReqData,
                                                               void* pData,
                                                               UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaSetSubscription(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE
    virtual RIL_RESULT_CODE CoreCdmaSetRoamingPreference(REQUEST_DATA& rReqData,
                                                                    void* pData,
                                                                    UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaSetRoamingPreference(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE
    virtual RIL_RESULT_CODE CoreCdmaQueryRoamingPreference(REQUEST_DATA& rReqData,
                                                                      void* pData,
                                                                      UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaQueryRoamingPreference(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_TTY_MODE
    virtual RIL_RESULT_CODE CoreSetTtyMode(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetTtyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_TTY_MODE
    virtual RIL_RESULT_CODE CoreQueryTtyMode(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseQueryTtyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE
    virtual RIL_RESULT_CODE CoreCdmaSetPreferredVoicePrivacyMode(REQUEST_DATA& rReqData,
                                                                            void* pData,
                                                                            UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaSetPreferredVoicePrivacyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE
    virtual RIL_RESULT_CODE CoreCdmaQueryPreferredVoicePrivacyMode(REQUEST_DATA& rReqData,
                                                                              void* pData,
                                                                              UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaQueryPreferredVoicePrivacyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_FLASH
    virtual RIL_RESULT_CODE CoreCdmaFlash(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaFlash(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_BURST_DTMF
    virtual RIL_RESULT_CODE CoreCdmaBurstDtmf(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaBurstDtmf(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY
    virtual RIL_RESULT_CODE CoreCdmaValidateAndWriteAkey(REQUEST_DATA& rReqData,
                                                                    void* pData,
                                                                    UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaValidateAndWriteAkey(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SEND_SMS
    virtual RIL_RESULT_CODE CoreCdmaSendSms(REQUEST_DATA& rReqData,
                                                       void* pData,
                                                       UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaSendSms(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE
    virtual RIL_RESULT_CODE CoreCdmaSmsAcknowledge(REQUEST_DATA& rReqData,
                                                              void* pData,
                                                              UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaSmsAcknowledge(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG
    virtual RIL_RESULT_CODE CoreGsmGetBroadcastSmsConfig(REQUEST_DATA& rReqData,
                                                                    void* pData,
                                                                    UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGsmGetBroadcastSmsConfig(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG
    virtual RIL_RESULT_CODE CoreGsmSetBroadcastSmsConfig(REQUEST_DATA& rReqData,
                                                                    void* pData,
                                                                    UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGsmSetBroadcastSmsConfig(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION
    virtual RIL_RESULT_CODE CoreGsmSmsBroadcastActivation(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGsmSmsBroadcastActivation(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG
    virtual RIL_RESULT_CODE CoreCdmaGetBroadcastSmsConfig(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaGetBroadcastSmsConfig(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG
    virtual RIL_RESULT_CODE CoreCdmaSetBroadcastSmsConfig(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaSetBroadcastSmsConfig(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION
    virtual RIL_RESULT_CODE CoreCdmaSmsBroadcastActivation(REQUEST_DATA& rReqData,
                                                                      void* pData,
                                                                      UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaSmsBroadcastActivation(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_SUBSCRIPTION
    virtual RIL_RESULT_CODE CoreCdmaSubscription(REQUEST_DATA& rReqData,
                                                            void* pData,
                                                            UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaSubscription(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM
    virtual RIL_RESULT_CODE CoreCdmaWriteSmsToRuim(REQUEST_DATA& rReqData,
                                                              void* pData,
                                                              UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaWriteSmsToRuim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM
    virtual RIL_RESULT_CODE CoreCdmaDeleteSmsOnRuim(REQUEST_DATA& rReqData,
                                                               void* pData,
                                                               UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCdmaDeleteSmsOnRuim(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DEVICE_IDENTITY
    virtual RIL_RESULT_CODE CoreDeviceIdentity(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseDeviceIdentity(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE
    virtual RIL_RESULT_CODE CoreExitEmergencyCallbackMode(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseExitEmergencyCallbackMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_SMSC_ADDRESS
    virtual RIL_RESULT_CODE CoreGetSmscAddress(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetSmscAddress(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_SMSC_ADDRESS
    virtual RIL_RESULT_CODE CoreSetSmscAddress(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetSmscAddress(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_REPORT_SMS_MEMORY_STATUS
    virtual RIL_RESULT_CODE CoreReportSmsMemoryStatus(REQUEST_DATA& rReqData,
                                                                 void* pData,
                                                                 UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseReportSmsMemoryStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
    virtual RIL_RESULT_CODE CoreReportStkServiceRunning(REQUEST_DATA& rReqData,
                                                                   void* pData,
                                                                   UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseReportStkServiceRunning(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ISIM_AUTHENTICATE
    virtual RIL_RESULT_CODE CoreISimAuthenticate(REQUEST_DATA& rReqData,
                                                                    void* pData,
                                                                    UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseISimAuthenticate(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU
    virtual RIL_RESULT_CODE CoreAckIncomingGsmSmsWithPdu(REQUEST_DATA& rReqData,
                                                                    void* pData,
                                                                    UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseAckIncomingGsmSmsWithPdu(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
    virtual RIL_RESULT_CODE ParseStkSendEnvelopeWithStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_CELL_INFO_LIST
    virtual RIL_RESULT_CODE CoreGetCellInfoList(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCellInfoList(RESPONSE_DATA& rRspData, BOOL isUnsol = FALSE);

    virtual RIL_RESULT_CODE ParseCellInfo(P_ND_N_CELL_INFO_DATA_V12 pCellData,
                                                           const char* pszRsp,
                                                           UINT32 uiIndex,
                                                           UINT32 uiMode);

    // RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE
    RIL_RESULT_CODE CoreSetCellInfoListRate(REQUEST_DATA& rReqData,
                                                                 void* pData,
                                                                 UINT32 uiDataSize);
    RIL_RESULT_CODE ParseSetCellInfoListRate(RESPONSE_DATA& rRspData);

    RIL_RESULT_CODE ParseUnsolCellInfoListRate(RESPONSE_DATA& rRspData);

    void RestartUnsolCellInfoListTimer(int newRate);

    // RIL_REQUEST_SET_INITIAL_ATTACH_APN
    virtual RIL_RESULT_CODE CoreSetInitialAttachApn(REQUEST_DATA& reqData,
            void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetInitialAttachApn(RESPONSE_DATA& rspData);

    // RIL_REQUEST_IMS_REGISTRATION_STATE
    // RIL_REQUEST_IMS_SEND_SMS
    // TODO

    // RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC
    virtual RIL_RESULT_CODE CoreSimTransmitApduBasic(REQUEST_DATA& rReqData,
                                                            void* pData,
                                                            UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimTransmitApduBasic(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_OPEN_CHANNEL
    virtual RIL_RESULT_CODE CoreSimOpenChannel(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimOpenChannel(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_CLOSE_CHANNEL
    virtual RIL_RESULT_CODE CoreSimCloseChannel(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimCloseChannel(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL
    virtual RIL_RESULT_CODE CoreSimTransmitApduChannel(REQUEST_DATA& rReqData,
                                                              void* pData,
                                                              UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimTransmitApduChannel(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_DATA_PROFILE
    virtual void CoreSetDataProfile(void* pData, size_t datalen);

    // RIL_REQUEST_SHUTDOWN
    virtual RIL_RESULT_CODE CoreShutdown(REQUEST_DATA& reqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseShutdown(RESPONSE_DATA& rspData);

    // RIL_REQUEST_SIM_AUTHENTICATION
    virtual RIL_RESULT_CODE CoreSimAuthentication(REQUEST_DATA& reqData, void* pData,
            UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimAuthentication(RESPONSE_DATA& rspData);
    virtual void PostSimAuthentication(POST_CMD_HANDLER_DATA& data);

    // RIL_UNSOL_SIGNAL_STRENGTH
    virtual RIL_RESULT_CODE ParseUnsolicitedSignalStrength(RESPONSE_DATA& rRspData);

    // QUERY SIM SMS STORE STATUS
    virtual RIL_RESULT_CODE ParseQuerySimSmsStoreStatus(RESPONSE_DATA& rRspData);

    virtual RIL_RESULT_CODE ParsePdpContextActivate(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseQueryIpAndDns(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseEnterDataState(RESPONSE_DATA& rRspData);

    virtual void SetIncomingCallStatus(UINT32 uiCallId, UINT32 uiStatus);
    virtual UINT32 GetIncomingCallId();

    // Get Extended Error Report (called internally)
    virtual BOOL CreateQueryCEER(REQUEST_DATA& rReqData);
    virtual BOOL ParseCEER(RESPONSE_DATA& rRspData, UINT32& rUICause);
    virtual RIL_RESULT_CODE ParseQueryCEER(RESPONSE_DATA& rRspData);

    // Get Extended Error Report for Location Update Reject
    // during CS registration (called internally)
    virtual BOOL CreateQueryNEER(REQUEST_DATA& rReqData);
    virtual RIL_RESULT_CODE ParseQueryNEER(RESPONSE_DATA& rRspData);
    virtual BOOL ParseNEER(RESPONSE_DATA& rRspData, UINT32& uiCause);

    // Getter functions for SIM and Radio states
    virtual RIL_RadioState GetRadioState();
    virtual int GetSimCardState();
    virtual int GetSimAppState();
    virtual int GetSimPinState();
    virtual void GetSimAppIdAndLabel(const int app_type, char* pszAppId[], char* pszAppLabel[]);
    virtual void GetSimAppId(const int app_type, char* pszAppId, const int maxAppIdLength);
    virtual int GetIsimAppState();
    virtual int GetSessionId(const int appType);

    // Setter functions for SIM and Radio states
    virtual void SetRadioState(const RRIL_Radio_State eRadioState);
    virtual void SetRadioStateAndNotify(const RRIL_Radio_State eRadioState);
    virtual void SetSimState(const int cardState, const int appState, const int pinState);
    virtual void SetSimAppState(const int appState);
    virtual void SetPersonalisationSubState(const int perso_substate);
    virtual void UpdateIsimAppState();
    virtual void SetSessionId(const int appType, const int sessionId);

    // Returns true on PIN entry required
    virtual BOOL IsPinEnabled();

    // Silent Pin Entry request and response handler
    virtual BOOL HandleSilentPINEntry(void* pRilToken, void* pContextData, int dataSize);
    virtual RIL_RESULT_CODE ParseSilentPinEntry(RESPONSE_DATA& rRspData);

    virtual RIL_RESULT_CODE ParseReadContextParams(RESPONSE_DATA& rRspData);

    virtual RIL_RESULT_CODE ParseReadBearerTFTParams(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseReadBearerQOSParams(RESPONSE_DATA& rRspData);

    // PIN retry count request and response handler
    virtual RIL_RESULT_CODE QueryPinRetryCount(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimPinRetryCount(RESPONSE_DATA& rRspData);

    // Post command handlers for RIL_REQUEST_SETUP_DATA_CALL request
    virtual void PostSetupDataCallCmdHandler(POST_CMD_HANDLER_DATA& rData);
    virtual void PostPdpContextActivateCmdHandler(POST_CMD_HANDLER_DATA& rData);
    virtual void PostQueryIpAndDnsCmdHandler(POST_CMD_HANDLER_DATA& rData);
    virtual void PostEnterDataStateCmdHandler(POST_CMD_HANDLER_DATA& rData);

    // Post command handler for RIL_REQUEST_DEACTIVATE_DATA_CALL request
    virtual void PostDeactivateDataCallCmdHandler(POST_CMD_HANDLER_DATA& rData);

    // Get functions returning number of retry counts left for respective locks
    virtual int GetPinRetryCount() { return m_PinRetryCount.pin; };
    virtual int GetPin2RetryCount() { return m_PinRetryCount.pin2; };
    virtual int GetPukRetryCount() { return m_PinRetryCount.puk; };
    virtual int GetPuk2RetryCount() { return m_PinRetryCount.puk2; };

    // Setter and getter functions for PIN2 state
    virtual void SetPin2State(RIL_PinState ePin2State) { m_ePin2State = ePin2State; };
    virtual RIL_PinState GetPin2State() { return m_ePin2State; };

    virtual void HandleSetupDataCallSuccess(UINT32 uiCID, void* pRilToken);
    virtual void HandleSetupDataCallFailure(UINT32 uiCID, void* pRilToken, UINT32 uiResultCode);

    // Functions for configuring data connections
    virtual BOOL DataConfigUp(char* pszNetworkInterfaceName, CChannel_Data* pChannelData,
                                                PDP_TYPE eDataConnectionType);

    /*
     * Removes the interface, puts the channels into AT command mode and
     * resets the data call information of the channel matching the CID.
     * If bForceCleanup is TRUE, then the data call information is reset
     * even if the provided context ID is default PDN context ID.
     */
    virtual BOOL DataConfigDown(UINT32 uiCID, BOOL bForceCleanup = FALSE) = 0;

    virtual void CleanupAllDataConnections();
    virtual BOOL DataConfigUpIpV4(char* pszNetworkInterfaceName, CChannel_Data* pChannelData);
    virtual BOOL DataConfigUpIpV6(char* pszNetworkInterfaceName, CChannel_Data* pChannelData);
    virtual BOOL DataConfigUpIpV4V6(char* pszNetworkInterfaceName, CChannel_Data* pChannelData);

    virtual RIL_RadioTechnology MapAccessTechnology(UINT32 uiStdAct, int regType);

    /*
     * AT commands which will disable detailed registration status reporting,
     * signal strength, fast dormancy etc are added to the command queue.
     */
    virtual RIL_RESULT_CODE HandleScreenStateReq(int screenState);

    virtual int GetCurrentCallId();

    BOOL IsDtmfAllowed(int callId);
    void SetDtmfAllowed(int callId, BOOL bDtmfAllowed);

    /*
     * Get AT commands to power on/off the radio
     */
    virtual BOOL GetRadioPowerCommand(BOOL bTurnRadioOn, int radioOffReason,
            /*INOUT*/ char* pCmdBuffer, int cmdBufferLen);

    virtual RIL_RESULT_CODE CreateIMSRegistrationReq(REQUEST_DATA& rReqData,
            const char** pszRequest,
            const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateIMSConfigReq(REQUEST_DATA& rReqData,
            const char** pszRequest,
            const int nNumStrings);

    virtual RIL_RESULT_CODE CreateSetDefaultApnReq(REQUEST_DATA& rReqData,
            const char** pszRequest, const int nNumStrings);

    /*
     * Upon response of CGACT or CGATT, all data connections will be
     * cleanup and also data call list change will be notified to
     * framework.
     */
    virtual RIL_RESULT_CODE ParseDeactivateAllDataCalls(RESPONSE_DATA& rRspData);

    /*
     * This function should be called only for setting up the default PDN which is
     * already active.
     */
    virtual RIL_RESULT_CODE HandleSetupDefaultPDN(RIL_Token rilToken,
            CChannel_Data* pChannelData);
    virtual RIL_RESULT_CODE ParseSetupDefaultPDN(RESPONSE_DATA& rRspData);
    virtual void PostSetupDefaultPDN(POST_CMD_HANDLER_DATA& rData);

    virtual BOOL SetupInterface(UINT32 uiCID);

    virtual void HandleChannelsBasicInitComplete();
    virtual void HandleChannelsUnlockInitComplete();

    virtual RIL_RESULT_CODE ParseSimStateQuery(RESPONSE_DATA& rRspData);
    virtual void QuerySimSmsStoreStatus();

    virtual BOOL IsInCall();

    // Post handler for internal DTMF stop request
    virtual void PostInternalDtmfStopReq(POST_CMD_HANDLER_DATA& rData);

    virtual const char* GetSignalStrengthReportingStringAlloc();

    virtual RIL_SignalStrength* ParseXCESQ(const char*& rszPointer, const BOOL bUnsolicited);

    virtual void ResetCardStatus(BOOL bForceReset);
    virtual void QueryUiccInfo();
    virtual RIL_RESULT_CODE ParseQueryActiveApplicationType(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseQueryAvailableApplications(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseQueryIccId(RESPONSE_DATA& rRspData);

    virtual void CopyCardStatus(RIL_CardStatus_v6& cardStatus);
    virtual void HandleSimState(const UINT32 uiSIMState, BOOL& bNotifySimStatusChange);
    virtual void SetRefreshWithUsimInitOn(BOOL bOn)
    {
        m_bRefreshWithUSIMInitOn = bOn;
    }

    virtual BOOL OpenLogicalChannel(POST_CMD_HANDLER_DATA& rData, const char* pszAid);

    virtual RIL_RESULT_CODE HandleSimIO(RIL_SIM_IO_v6* pSimIOArgs, REQUEST_DATA& rReqData,
            int sessionId);

    virtual void TriggerUiccOpenLogicalChannelEvent()
    {
        CEvent::Signal(m_pUiccOpenLogicalChannelEvent);
    }

    void ResetInitialAttachApn();

    virtual RIL_RESULT_CODE RestoreSavedNetworkSelectionMode(RIL_Token rilToken, UINT32 uiChannel,
            PFN_TE_PARSE pParseFcn = NULL, PFN_TE_POSTCMDHANDLER pHandlerFcn = NULL);
    int GetNetworkSelectionMode() { return m_NetworkSelectionModeParams.mode; }

    virtual void SetProfileDownloadForNextUiccStartup(UINT32 uiDownload, UINT32 uiReporting);
    virtual void ConfigureUsatProfileDownload(UINT32 uiDownload, UINT32 uiReporting);
    virtual void PostConfigureUsatProfileDownloadHandler(POST_CMD_HANDLER_DATA& data);

    virtual RIL_RESULT_CODE ParseQueryUiccState(RESPONSE_DATA& rspData);
    virtual void PostQueryUiccStateHandler(POST_CMD_HANDLER_DATA& data);

    virtual RIL_RESULT_CODE ParseReadUsatProfiles(RESPONSE_DATA& rspData);

    virtual void WriteUsatProfiles(const char* pszTeProfile, const BOOL isTeWriteNeeded,
            const char* pszMtProfile, const BOOL isMtWriteNeeded);
    virtual void WriteUsatProfile(const UINT32 uiProfileStorage, const char* pszProfile);
    virtual RIL_RESULT_CODE ParseWriteUsatProfile(RESPONSE_DATA& rspData);
    virtual void PostWriteUsatProfileHandler(POST_CMD_HANDLER_DATA& data);

    virtual void ResetUicc();
    virtual void NotifyUiccReady();

    virtual void EnableProfileFacilityHandling();

    virtual void SendModemDownToUsatSM();

    bool IsRegStatusAndBandIndActivated() { return m_bRegStatusAndBandIndActivated; }
    void GetRegStatusAndBandInfo(
            sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND& regStatusAndBandInfo);
    void SetRegStatusAndBandInfo(
            sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND regStatusAndBandInfo);

    virtual RIL_RESULT_CODE SetCsgAutomaticSelection(REQUEST_DATA& reqData);
    virtual RIL_RESULT_CODE GetCsgCurrentState(REQUEST_DATA& reqData);
    virtual RIL_RESULT_CODE ParseXCSG(const char* pszRsp, RESPONSE_DATA& rspData);

    void ResetNetworkSelectionMode();

    virtual RIL_RESULT_CODE SetInitialAttachApn(RIL_Token rilToken, UINT32 uiChannel,
            PFN_TE_PARSE pParseFcn, PFN_TE_POSTCMDHANDLER pHandlerFcn, int nextState);
    RIL_RESULT_CODE CoreQueryCnap(REQUEST_DATA& rReqData);
    RIL_RESULT_CODE ParseQueryCnap(const char* pszRsp, RESPONSE_DATA& rRspData);

    virtual const char* GetSiloVoiceURCInitString();

    virtual const char* GetEnableFetchingString();

    virtual const char* GetReadCellInfoString() { return NULL; }

    bool NeedGetCellInfoOnCellChange() { return m_bNeedGetInfoOnCellChange; }

    virtual RIL_RESULT_CODE CreateSetAdaptiveClockingReq(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateGetAdaptiveClockingFreqInfo(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseGetAdaptiveClockingFreqInfo(const char* pszRsp,
            RESPONSE_DATA& rspData);

    virtual RIL_RESULT_CODE CreateSetRegStatusAndBandReport(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateSetCoexReport(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateSetCoexWlanParams(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateSetCoexBtParams(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);

    bool IsCoexReportActivated() { return m_bCoexReportActivated; }

    bool IsApnEqual(const char* pszApn1, const char* pszApn2);
    bool IsPdpTypeCompatible(const char* pszPdpType1, const char* pszPdpType2);
    void RequestDetachOnIAChange();
    void RequestAttachOnIAChange(const UINT32 uiChannel, const int requestId);

protected:
    RIL_RESULT_CODE ParseSimPin(const char*& pszRsp);

    virtual const char* GetRegistrationInitString();
    virtual const char* GetCsRegistrationReadString();
    virtual const char* GetPsRegistrationReadString();
    virtual const char* GetLocationUpdateString(BOOL bIsLocationUpdateEnabled);
    virtual const char* GetScreenOnString();
    virtual const char* GetScreenOffString();

    virtual void HandleInternalDtmfStopReq();
    virtual void NotifyNetworkApnInfo();

    /*
     * Based on the radio and modem state, modem will be either released or powered off.
     */
    virtual RIL_RESULT_CODE HandleReleaseModemReq(REQUEST_DATA& reqData, const char** ppszRequest,
            const UINT32 uiDataSize);

    /*
     * Deactivates all data calls. This function is called before sending manual network scan.
     */
    virtual void DeactivateAllDataCalls();

    virtual void QuerySignalStrength();

    virtual BOOL ParseEFdir(const char* pszResponseString, const UINT32 uiResponseStringLen);

    virtual BOOL GetSetInitialAttachApnReqData(REQUEST_DATA& rReqData);

private:
    RIL_SignalStrength_v8* ParseQuerySignalStrength(RESPONSE_DATA& rRspData);

    /*
     * Wait for the modem power off event which will be triggered on MODEM_DOWN
     * event from MMGR.
     */
    void WaitForModemPowerOffEvent();
    void HandleShutdownReq(int requestId);

    typedef struct
    {
        UINT32 callId;
        UINT32 status;
        BOOL isAnswerReqSent;
    } INCOMING_CALL_INFO;
    INCOMING_CALL_INFO m_IncomingCallInfo;

    S_VOICECALL_STATE_INFO m_VoiceCallInfo[RRIL_MAX_CALL_ID_COUNT];

    // helper function to check if IMEI is in blacklist of default IMEIs
    void CheckImeiBlacklist(char* szImei);

    CMutex* m_pCardStatusUpdateLock;
};

#endif
