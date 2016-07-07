////////////////////////////////////////////////////////////////////////////
// te_xmm6260.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Overlay for the IMC 6260 modem
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_TE_XMM6260_H
#define RRIL_TE_XMM6260_H

#include <cutils/properties.h>

#include "te_base.h"
#include "nd_structs.h"
#include "channel_data.h"

class CEvent;
class CInitializer;

class CTE_XMM6260 : public CTEBase
{
public:
    enum { OEM_API_VERSION = 1 };

    CTE_XMM6260(CTE& cte);
    virtual ~CTE_XMM6260();

private:

    CTE_XMM6260();

    //  Prevent assignment: Declared but not implemented.
    CTE_XMM6260(const CTE_XMM6260& rhs);  // Copy Constructor
    CTE_XMM6260& operator=(const CTE_XMM6260& rhs);  //  Assignment operator

protected:
    int m_currentNetworkType;

public:
    // modem overrides
    virtual char* GetBasicInitCommands(UINT32 uiChannelType);
    virtual char* GetUnlockInitCommands(UINT32 uiChannelType);

    virtual CInitializer* GetInitializer();

    virtual BOOL IsRequestSupported(int requestId);

    // RIL_REQUEST_GET_SIM_STATUS
    virtual RIL_RESULT_CODE CoreGetSimStatus(REQUEST_DATA& rReqData, void* pData,
            UINT32 uiDataSize);

    // RIL_REQUEST_DATA_REGISTRATION_STATE
    virtual RIL_RESULT_CODE ParseGPRSRegistrationState(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SETUP_DATA_CALL
    virtual RIL_RESULT_CODE CoreSetupDataCall(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize,
                                                         UINT32& uiCID);
    virtual RIL_RESULT_CODE ParseSetupDataCall(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_IO
    virtual RIL_RESULT_CODE CoreSimIo(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimIo(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DEACTIVATE_DATA_CALL
    virtual RIL_RESULT_CODE CoreDeactivateDataCall(REQUEST_DATA& rReqData,
                                                              void* pData,
                                                              UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseDeactivateDataCall(RESPONSE_DATA& rRspData);

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
    virtual RIL_RESULT_CODE ParseHookStrings(RESPONSE_DATA& rspData);

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
    virtual RIL_RESULT_CODE ParseNeighboringCellInfo(P_ND_N_CELL_DATA pCellData,
                                                            const char* pszRsp,
                                                            UINT32 uiIndex,
                                                            UINT32 uiMode);

    // RIL_REQUEST_SET_TTY_MODE
    virtual RIL_RESULT_CODE CoreSetTtyMode(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetTtyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_QUERY_TTY_MODE
    virtual RIL_RESULT_CODE CoreQueryTtyMode(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseQueryTtyMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SMS_MEMORY_STATUS
    virtual RIL_RESULT_CODE CoreReportSmsMemoryStatus(REQUEST_DATA& rReqData,
                                                                 void* pData,
                                                                 UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseReportSmsMemoryStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
    virtual RIL_RESULT_CODE CoreReportStkServiceRunning(REQUEST_DATA& rReqData,
                                                                   void* pData,
                                                                   UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseReportStkServiceRunning(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
    virtual RIL_RESULT_CODE ParseStkSendEnvelopeWithStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_CELL_INFO_LIST
    virtual RIL_RESULT_CODE CoreGetCellInfoList(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseCellInfo(P_ND_N_CELL_INFO_DATA pCellData,
                                                           const char* pszRsp,
                                                           UINT32 uiIndex,
                                                           UINT32 uiMode);

    // internal response handlers
    virtual RIL_RESULT_CODE ParsePdpContextActivate(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseQueryIpAndDns(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseEnterDataState(RESPONSE_DATA& rRspData);

    // Silent Pin Entry request and response handler
    virtual BOOL HandleSilentPINEntry(void* pRilToken, void* pContextData, int dataSize);
    virtual RIL_RESULT_CODE ParseSilentPinEntry(RESPONSE_DATA& rRspData);

    // PIN retry count request and response handler
    virtual RIL_RESULT_CODE QueryPinRetryCount(REQUEST_DATA& rReqData,
                                                            void* pData,
                                                            UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimPinRetryCount(RESPONSE_DATA& rRspData);

    // Handles the PIN2 provided SIM IO requests
    RIL_RESULT_CODE HandlePin2RelatedSIMIO(RIL_SIM_IO_v6* pSimIOArgs,
                                           REQUEST_DATA& rReqData);

    virtual BOOL CreatePdpContextActivateReq(UINT32 uiChannel,
                                           RIL_Token rilToken,
                                           int reqId,
                                           void* pData,
                                           UINT32 uiDataSize,
                                           PFN_TE_PARSE pParseFcn,
                                           PFN_TE_POSTCMDHANDLER pPostCmdHandlerFcn);
    virtual BOOL CreateQueryIpAndDnsReq(UINT32 uiChannel,
                                      RIL_Token rilToken,
                                      int reqId,
                                      void* pData,
                                      UINT32 uiDataSize,
                                      PFN_TE_PARSE pParseFcn,
                                      PFN_TE_POSTCMDHANDLER pPostCmdHandlerFcn);
    virtual BOOL CreateEnterDataStateReq(UINT32 uiChannel,
                                       RIL_Token rilToken,
                                       int reqId,
                                       void* pData,
                                       UINT32 uiDataSize,
                                       PFN_TE_PARSE pParseFcn,
                                       PFN_TE_POSTCMDHANDLER pPostCmdHandlerFcn);
    virtual BOOL PdpContextActivate(REQUEST_DATA& rReqData,
                                               void* pData,
                                               UINT32 uiDataSize);
    virtual BOOL QueryIpAndDns(REQUEST_DATA& rReqData, UINT32 uiCID);
    virtual BOOL EnterDataState(REQUEST_DATA& rReqData, UINT32 uiCID);

    virtual void PostSetupDataCallCmdHandler(POST_CMD_HANDLER_DATA& rData);
    virtual void PostPdpContextActivateCmdHandler(POST_CMD_HANDLER_DATA& rData);
    virtual void PostQueryIpAndDnsCmdHandler(POST_CMD_HANDLER_DATA& rData);
    virtual void PostEnterDataStateCmdHandler(POST_CMD_HANDLER_DATA& rData);

    virtual void PostDeactivateDataCallCmdHandler(POST_CMD_HANDLER_DATA& rData);

    virtual BOOL SetupInterface(UINT32 uiCID);

    virtual BOOL DataConfigDown(UINT32 uiCID, BOOL bForceCleanup = FALSE);

    virtual RIL_RESULT_CODE HandleScreenStateReq(int screenState);

    virtual BOOL GetRadioPowerCommand(BOOL bTurnRadioOn, int radioOffReason,
            char* pCmdBuffer, int cmdBufferLen);

    virtual void HandleChannelsBasicInitComplete();

    virtual RIL_RESULT_CODE ParseSimStateQuery(RESPONSE_DATA& rRspData);

    virtual const char* GetSignalStrengthReportingStringAlloc();

    virtual void QueryUiccInfo();

    virtual RIL_RESULT_CODE ParseQueryActiveApplicationType(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseQueryAvailableApplications(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseQueryIccId(RESPONSE_DATA& rRspData);

    virtual void HandleSimState(const UINT32 uiSIMState, BOOL& bNotifySimStatusChange);

    RIL_RESULT_CODE CreateRegStatusAndBandInd(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    RIL_RESULT_CODE ParseRegStatusAndBandInd(const char* pszRsp, RESPONSE_DATA& rspData);

    virtual const char* GetEnableFetchingString();

    virtual const char* GetSiloVoiceURCInitString();

    virtual const char* GetReadCellInfoString();

protected:
    virtual RIL_RESULT_CODE ParseIpAddress(RESPONSE_DATA& rRspData);
    virtual RIL_RESULT_CODE ParseDns(RESPONSE_DATA& rRspData);

    virtual RIL_RESULT_CODE ParseXUICC(const char*& pszRsp);
    virtual RIL_RESULT_CODE ParseCUAD(const char*& pszRsp);
    virtual RIL_RESULT_CODE ParseCCID(const char*& pszRsp);
    virtual RIL_RESULT_CODE ParseXPINCNT(const char*& pszRsp);

    virtual const char* GetRegistrationInitString();
    virtual const char* GetPsRegistrationReadString();

    virtual const char* GetScreenOnString();
    virtual const char* GetScreenOffString();

    virtual LONG GetDataDeactivateReason(char* pszReason);

    virtual void HandleInternalDtmfStopReq();

    virtual void QueryActiveApplicationType();
    virtual void QueryAvailableApplications();
    virtual void QueryIccId();
    virtual void QueryPinRetryCount();
    virtual void QuerySimState();

    virtual UINT32 GetXDNSMode(const char* pszPdpType);

    virtual int GetOemVersion() { return OEM_API_VERSION; }
    virtual RIL_RESULT_CODE GetOemVersion(REQUEST_DATA& reqData);
    virtual RIL_RESULT_CODE CreateGetThermalSensorValuesReq(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateActivateThermalSensorInd(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateGetThermalSensorValuesV2Req(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateActivateThermalSensorV2Ind(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    // Maps the rscp values(0..96, 255) to rssi(0..31, 99) values
    int MapRscpToRssi(int rscp);

    virtual RIL_RESULT_CODE CreateGetGprsCellEnvReq(REQUEST_DATA& reqData);
    virtual RIL_RESULT_CODE CreateDebugScreenReq(REQUEST_DATA& rReqData,
              const char** ppszRequest, const UINT32 uiDataSize);

private:
    RIL_RESULT_CODE CreateAutonomousFDReq(REQUEST_DATA& rReqData,
                                          const char** pszRequest,
                                          const UINT32 uiDataSize);
    RIL_RESULT_CODE CreateSetSMSTransportModeReq(REQUEST_DATA& rReqData,
                                                 const char** pszRequest,
                                                 const UINT32 uiDataSize);
    RIL_RESULT_CODE CreateSetRFPowerCutbackTableReq(REQUEST_DATA& rReqData,
                                                    const char** pszRequest,
                                                    const UINT32 uiDataSize);
    RIL_RESULT_CODE SetCallImsAvailable(REQUEST_DATA& rReqData,
                                        const char** pszRequest,
                                        const int nNumStrings);
    RIL_RESULT_CODE SetSmsImsAvailable(REQUEST_DATA& rReqData,
                                        const char** pszRequest,
                                        const int nNumStrings);
    RIL_RESULT_CODE GetPcscf(REQUEST_DATA& rReqData,
                             const char** pszRequest,
                             const UINT32 uiDataSize);
    RIL_RESULT_CODE SetSrvccParams(REQUEST_DATA& rReqData,
                                   const char** pszRequest);
    RIL_RESULT_CODE CreateSetDVPEnabledReq(REQUEST_DATA& rReqData,
                                           const char** pszRequest,
                                           const UINT32 uiDataSize);
    RIL_RESULT_CODE ParseXGATR(const char* pszRsp, RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseXDRV(const char* pszRsp, RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseCGED(const char* pszRsp, RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseXCGEDPAGE(const char* pszRsp, RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseCGSMS(const char* pszRsp, RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseXRFCBT(const char* pszRsp, RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseXISRVCC(const char* pszRsp, RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE HandleSendAtResponse(const char* pszRsp, RESPONSE_DATA& rRspData);
    RIL_RESULT_CODE ParseXDVP(const char* pszRsp, RESPONSE_DATA& rRspData);
    BOOL ParseXSIMSTATE(const char*& rszPointer);
    BOOL ParseXLOCK(const char*& rszPointer);
};

#endif
