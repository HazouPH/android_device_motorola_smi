////////////////////////////////////////////////////////////////////////////
// te_xmm7160.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Overlay for the IMC 7160 modem
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_TE_XMM7160_H
#define RRIL_TE_XMM7160_H

#include "te_xmm6360.h"
#include "nd_structs.h"
#include "channel_data.h"
#include "cbs_info.h"

class CEvent;
class CInitializer;

class CTE_XMM7160 : public CTE_XMM6360
{
public:

    enum { OEM_API_VERSION = 2 };

    CTE_XMM7160(CTE& cte);
    virtual ~CTE_XMM7160();

private:

    CTE_XMM7160();

    //  Prevent assignment: Declared but not implemented.
    CTE_XMM7160(const CTE_XMM7160& rhs);  // Copy Constructor
    CTE_XMM7160& operator=(const CTE_XMM7160& rhs);  //  Assignment operator

public:
    // modem overrides

    virtual CInitializer* GetInitializer();

    virtual RIL_RESULT_CODE CreateIMSRegistrationReq(REQUEST_DATA& rReqData,
                                            const char** pszRequest,
                                            const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateIMSConfigReq(REQUEST_DATA& rReqData,
                                       const char** pszRequest,
                                       const int nNumStrings);

    virtual RIL_RESULT_CODE CreateSetDefaultApnReq(REQUEST_DATA& rReqData,
            const char** pszRequest, const int nNumStrings);

    // RIL_REQUEST_SETUP_DATA_CALL
    virtual RIL_RESULT_CODE CoreSetupDataCall(REQUEST_DATA& rReqData,
            void* pData, UINT32 uiDataSize, UINT32& uiCID);

    // RIL_REQUEST_SET_BAND_MODE
    virtual RIL_RESULT_CODE CoreSetBandMode(REQUEST_DATA& rReqData,
                                                       void* pData,
                                                       UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSetBandMode(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIGNAL_STRENGTH
    virtual RIL_RESULT_CODE CoreSignalStrength(REQUEST_DATA& rReqData,
            void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseSignalStrength(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DATA_REGISTRATION_STATE
    virtual RIL_RESULT_CODE ParseGPRSRegistrationState(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_DEACTIVATE_DATA_CALL
    virtual RIL_RESULT_CODE CoreDeactivateDataCall(REQUEST_DATA& rReqData,
                                                                void* pData,
                                                                UINT32 uiDataSize);

    // RIL_REQUEST_OEM_HOOK_STRINGS
    virtual RIL_RESULT_CODE ParseHookStrings(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
    virtual RIL_RESULT_CODE CoreSetPreferredNetworkType(REQUEST_DATA& rReqData,
            void* pData, UINT32 uiDataSize);

    virtual RIL_RESULT_CODE ParseGetPreferredNetworkType(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_NEIGHBORING_CELL_IDS
    virtual RIL_RESULT_CODE CoreGetNeighboringCellIDs(REQUEST_DATA& rReqData,
        void* pData, UINT32 uiDataSize);
    virtual  RIL_RESULT_CODE ParseGetNeighboringCellIDs(RESPONSE_DATA& rRspData);

    // RIL_UNSOL_SIGNAL_STRENGTH
    virtual RIL_RESULT_CODE ParseUnsolicitedSignalStrength(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_ISIM_AUTHETICATE
    virtual RIL_RESULT_CODE CoreISimAuthenticate(REQUEST_DATA& rReqData,
             void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseISimAuthenticate(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_GET_CELL_INFO_LIST
    virtual RIL_RESULT_CODE CoreGetCellInfoList(REQUEST_DATA& rReqData, void* pData,
            UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseCellInfoList(RESPONSE_DATA& rRspData, BOOL isUnsol = FALSE);

    virtual BOOL IMSRegister(REQUEST_DATA& rReqData, void* pData,
                                                    UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseIMSRegister(RESPONSE_DATA & rRspData);

    virtual BOOL QueryIpAndDns(REQUEST_DATA& rReqData, UINT32 uiCID);
    virtual RIL_RESULT_CODE ParseQueryIpAndDns(RESPONSE_DATA& rRspData);

    /*
     * AT commands to configure the channel for default PDN and also to put the
     * channel in data mode is added to the command queue.
     */
    virtual RIL_RESULT_CODE HandleSetupDefaultPDN(RIL_Token rilToken,
            CChannel_Data* pChannelData);

    // Parser function for Setup default PDN.
    virtual RIL_RESULT_CODE ParseSetupDefaultPDN(RESPONSE_DATA& rRspData);

    /*
     * Post Command handler function for the setting up of default PDN.
     *
     * Upon success, data state of default PDN will be set to E_DATA_STATE_ACTIVE
     * and interface is setup and the RIL_REEQUEST_SETUP_DATA_CALL request will
     * be completed.
     *
     * Upon failure, RIL_REEQUEST_SETUP_DATA_CALL request will be completed.
     */
    virtual void PostSetupDefaultPDN(POST_CMD_HANDLER_DATA& rData);

    virtual BOOL DataConfigDown(UINT32 uiCID, BOOL bForceCleanup = FALSE);

    virtual char* GetBasicInitCommands(UINT32 uiChannelType);

    virtual char* GetUnlockInitCommands(UINT32 uiChannelType);

    virtual const char* GetSignalStrengthReportingStringAlloc();

    virtual RIL_SignalStrength* ParseXCESQ(const char*& rszPointer, const BOOL bUnsolicited);

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

    virtual RIL_RESULT_CODE ParseXCSG(const char* pszRsp, RESPONSE_DATA& rspData);

    virtual RIL_RESULT_CODE ParseGetAdaptiveClockingFreqInfo(const char* pszRsp,
            RESPONSE_DATA& rspData);

    virtual const char* GetSiloVoiceURCInitString();
    virtual const char* GetReadCellInfoString();
    virtual BOOL GetSetInitialAttachApnReqData(REQUEST_DATA& reqData);

    // RIL_REQUEST_SET_INITIAL_ATTACH_APN
    virtual RIL_RESULT_CODE CoreSetInitialAttachApn(REQUEST_DATA& reqData,
            void* pData, UINT32 uiDataSize);

    virtual RIL_RESULT_CODE RestoreSavedNetworkSelectionMode(RIL_Token rilToken, UINT32 uiChannel,
            PFN_TE_PARSE pParseFcn = NULL, PFN_TE_POSTCMDHANDLER pHandlerFcn = NULL);

    void HandleSetupDataCallSuccess(UINT32 uiCID, void* pRilToken);
    void HandleSetupDataCallFailure(UINT32 uiCID, void* pRilToken, UINT32 uiResultCode);

protected:

    virtual const char* GetRegistrationInitString();
    virtual const char* GetPsRegistrationReadString();

    virtual const char* GetLocationUpdateString(BOOL bIsLocationUpdateEnabled);

    virtual const char* GetScreenOnString();
    virtual const char* GetScreenOffString();

    virtual void QuerySignalStrength();

    virtual int GetOemVersion() { return OEM_API_VERSION; }
    virtual RIL_RESULT_CODE CreateGetThermalSensorValuesReq(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateActivateThermalSensorInd(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateGetThermalSensorValuesV2Req(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateActivateThermalSensorV2Ind(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);

    virtual RIL_RESULT_CODE SetCsgAutomaticSelection(REQUEST_DATA& reqData);
    virtual RIL_RESULT_CODE GetCsgCurrentState(REQUEST_DATA& reqData);

    virtual const char* GetEnablingEtwsString();
    virtual P_ND_N_CELL_INFO_DATA_V2 ParseXMCI(RESPONSE_DATA& rspData, int& nCellInfos);
    int MapRxlevToSignalStrengh(int rxlev);
    int MapToAndroidRsrp(int rsrp);
    int MapToAndroidRsrq(int rsrq);
    int MapToAndroidRssnr(int rssnr);

    bool IsImsEnabledApn(const char* pszApn);
    bool IsEImsEnabledApn(const char* pszApn);
    bool IsRcsEnabledApn(const char* pszApn);

    virtual RIL_RESULT_CODE CreateSetAdaptiveClockingReq(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);
    virtual RIL_RESULT_CODE CreateGetAdaptiveClockingFreqInfo(REQUEST_DATA& reqData,
            const char** ppszRequest, const UINT32 uiDataSize);

    virtual RIL_RESULT_CODE SetInitialAttachApn(RIL_Token rilToken, UINT32 uiChannel,
            PFN_TE_PARSE pParseFcn, PFN_TE_POSTCMDHANDLER pHandlerFcn, int nextState);

private:

    void ConvertCellInfoForVanillaAOSP(P_ND_N_CELL_INFO_DATA_V2 pOldData,
            P_ND_N_CELL_INFO_DATA_V12 pNewData, int nCellInfos);
    RIL_RESULT_CODE ParseXTAMR(const char* pszRsp, RESPONSE_DATA& rspData);
    RIL_RESULT_CODE ParseXTSM(const char* pszRsp, RESPONSE_DATA& rspData);

    RIL_RESULT_CODE CreateEtwSmsRequest(char*& rszRequest, UINT32 uiReqSize);
    RIL_RESULT_CODE CreateSmsCbRequest(char*& rszRequest, UINT32 uiReqSize);
    RIL_RESULT_CODE FilterSmsCbFromConfig(RIL_GSM_BroadcastSmsConfigInfo** ppConfigInfo,
            const UINT32 uiDataSize,
            android::Vector<RIL_GSM_BroadcastSmsConfigInfo>& vBroadcastSmsConfigInfo,
            CCbsInfo::CbmIds* pConfigIds);

    // Dedicated class about Cell Broadcast messages (SMS)
    CCbsInfo m_CbsInfo;
};

#endif
