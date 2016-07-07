////////////////////////////////////////////////////////////////////////////
// te_xmm7260.h
//
// Copyright (C) Intel 2013.
//
//
// Description:
//    Overlay for the IMC 7260 modem
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_TE_XMM7260_H
#define RRIL_TE_XMM7260_H

#include "te_xmm7160.h"

class CInitializer;
class UsatInitStateMachine;

class CTE_XMM7260 : public CTE_XMM7160
{
public:

    CTE_XMM7260(CTE& cte);
    virtual ~CTE_XMM7260();

private:

    CTE_XMM7260();

    //  Prevent assignment: Declared but not implemented.
    CTE_XMM7260(const CTE_XMM7260& rhs);  // Copy Constructor
    CTE_XMM7260& operator=(const CTE_XMM7260& rhs);  //  Assignment operator

public:
    // modem overrides

    virtual CInitializer* GetInitializer();

    virtual void HandleSimState(const UINT32 uiSIMState, BOOL& bNotifySimStatusChange);

    // RIL_REQUEST_SETUP_DATA_CALL
    virtual RIL_RESULT_CODE CoreSetupDataCall(REQUEST_DATA& rReqData,
            void* pData, UINT32 uiDataSize, UINT32& uiCID);

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

        // RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
    virtual RIL_RESULT_CODE CoreReportStkServiceRunning(REQUEST_DATA& rReqData,
                                                                   void* pData,
                                                                   UINT32 uiDataSize);

    virtual BOOL ParseActivateUsatProfile(const char* szResponse);
    virtual RIL_RESULT_CODE ParseReportStkServiceRunning(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
    virtual RIL_RESULT_CODE ParseStkSendEnvelopeWithStatus(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_STK_GET_PROFILE
    virtual RIL_RESULT_CODE CoreStkGetProfile(REQUEST_DATA& rReqData,
                                                void* pData,
                                                UINT32 uiDataSize);
    // RIL_REQUEST_STK_SET_PROFILE
    virtual RIL_RESULT_CODE CoreStkSetProfile(REQUEST_DATA& rReqData,
                                                void* pData,
                                                UINT32 uiDataSize);

    virtual void SetProfileDownloadForNextUiccStartup(UINT32 uiDownload, UINT32 uiReporting);
    virtual void ConfigureUsatProfileDownload(UINT32 uiDownload, UINT32 uiReporting);
    virtual void PostConfigureUsatProfileDownloadHandler(POST_CMD_HANDLER_DATA& data);

    virtual RIL_RESULT_CODE ParseQueryUiccState(RESPONSE_DATA& rRspData);
    virtual void PostQueryUiccStateHandler(POST_CMD_HANDLER_DATA& data);

    virtual void ReadUsatProfiles();
    virtual RIL_RESULT_CODE ParseReadUsatProfiles(RESPONSE_DATA& rRspData);

    virtual void WriteUsatProfiles(const char* pszTeProfile, const BOOL isTeWriteNeeded,
            const char* pszMtProfile, const BOOL isMtWriteNeeded);
    virtual RIL_RESULT_CODE ParseWriteUsatProfile(RESPONSE_DATA& rspData);
    virtual void PostWriteUsatProfileHandler(POST_CMD_HANDLER_DATA& data);

    virtual void ResetUicc();
    virtual void NotifyUiccReady();

    virtual void EnableProfileFacilityHandling();

    virtual void SendModemDownToUsatSM();

    virtual const char* GetEnableFetchingString();

    // PIN retry count request and response handler
    virtual RIL_RESULT_CODE QueryPinRetryCount(REQUEST_DATA& rReqData,
                                                            void* pData,
                                                            UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSimPinRetryCount(RESPONSE_DATA& rRspData);

    virtual const char* GetSignalStrengthReportingStringAlloc();

protected:
    virtual RIL_RESULT_CODE CreateGetGprsCellEnvReq(REQUEST_DATA& reqData);
    virtual RIL_RESULT_CODE CreateDebugScreenReq(REQUEST_DATA& reqData,
              const char** ppszRequest, const UINT32 uiDataSize);

    virtual const char* GetSiloVoiceURCInitString();
    virtual void QueryPinRetryCount();

    virtual void CopyCardStatus(RIL_CardStatus_v6& cardStatus);

    virtual P_ND_N_CELL_INFO_DATA_V2 ParseXMCI(RESPONSE_DATA& rspData, int& nCellInfos);

    virtual RIL_RESULT_CODE CreateSetRegStatusAndBandReport(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateSetCoexReport(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateSetCoexWlanParams(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateSetCoexBtParams(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);

private:

    RIL_RESULT_CODE ParseEnvelopCommandResponse(const char* pszResponse, char* pszEnvelopResp,
            UINT32* puiBusy, UINT32* puiSw1, UINT32* puiSw2);
    void QueryUiccState();
    void WriteUsatProfile(const UINT32 uiProfileStorage, const char* pszProfile);
    BOOL ParseCmeError(const char* szStart, const char*& rszEnd);

    UsatInitStateMachine& m_usatInitStateMachine;
};

#endif
