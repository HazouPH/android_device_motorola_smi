////////////////////////////////////////////////////////////////////////////
// te_xmm7260.cpp
//
// Copyright (C) Intel 2013.
//
//
// Description:
//    Overlay for the IMC 7260 modem
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "nd_structs.h"
#include "util.h"
#include "extract.h"
#include "rillog.h"
#include "te.h"
#include "te_xmm7260.h"
#include "init7260.h"
#include "usat_init_state_machine.h"
#include "rildmain.h"
#include "repository.h"


CTE_XMM7260::CTE_XMM7260(CTE& cte)
: CTE_XMM7160(cte),
  m_usatInitStateMachine(UsatInitStateMachine::GetStateMachine())
{
}

CTE_XMM7260::~CTE_XMM7260()
{
}

CInitializer* CTE_XMM7260::GetInitializer()
{
    RIL_LOG_VERBOSE("CTE_XMM7260::GetInitializer() - Enter\r\n");
    CInitializer* pRet = NULL;

    RIL_LOG_INFO("CTE_XMM7260::GetInitializer() - Creating CInit7260 initializer\r\n");
    m_pInitializer = new CInit7260();
    if (NULL == m_pInitializer)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::GetInitializer() - Failed to create a CInit7260 "
                "initializer!\r\n");
        goto Error;
    }

    pRet = m_pInitializer;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::GetInitializer() - Exit\r\n");
    return pRet;
}

void CTE_XMM7260::HandleSimState(const UINT32 uiSIMState, BOOL& bNotifySimStatusChange)
{
    CTE_XMM6260::HandleSimState(uiSIMState, bNotifySimStatusChange);

    switch (uiSIMState)
    {
        case 0: // SIM not present
        case 9: // SIM Removed
        case 14: // SIM powered off by modem
            m_usatInitStateMachine.SendEvent(UsatInitStateMachine::SIM_UNAVAILABLE);
            break;
        case 2: // PIN verification not needed - Ready
        case 3: // PIN verified - Ready
        case 7: // ready for attach (+COPS)
            m_usatInitStateMachine.SendEvent(UsatInitStateMachine::SIM_READY);
            break;
    }
}

//
// RIL_REQUEST_SETUP_DATA_CALL
//
RIL_RESULT_CODE CTE_XMM7260::CoreSetupDataCall(REQUEST_DATA& rReqData,
       void* pData, UINT32 uiDataSize, UINT32& uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CoreSetupDataCall() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char szIPV4V6[] = "IPV4V6";
    int nPapChap = 0; // no auth
    PdpData stPdpData;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    CChannel_Data* pChannelData = NULL;
    int dataProfile = -1;
    int nReqType = 0; // 0 => MT decides if new PDP or handover
    int emergencyFlag = 0; // 1: emergency pdn
    int requestPcscfFlag = 0; // 1: request pcscf address
    int imCnSignallingFlagInd = 0; // 1: IMS Only APN
    UINT32 uiDnsMode = 0;

    RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - uiDataSize=[%u]\r\n", uiDataSize);

    memset(&stPdpData, 0, sizeof(PdpData));

    // extract data
    stPdpData.szRadioTechnology = ((char**)pData)[0];  // not used
    stPdpData.szRILDataProfile  = ((char**)pData)[1];
    stPdpData.szApn             = ((char**)pData)[2];
    stPdpData.szUserName        = ((char**)pData)[3];
    stPdpData.szPassword        = ((char**)pData)[4];
    stPdpData.szPAPCHAP         = ((char**)pData)[5];

    pDataCallContextData =
            (S_SETUP_DATA_CALL_CONTEXT_DATA*)malloc(sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA));
    if (NULL == pDataCallContextData)
    {
        goto Error;
    }

    dataProfile = atoi(stPdpData.szRILDataProfile);
    pChannelData = CChannel_Data::GetFreeChnlsRilHsi(uiCID, dataProfile);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CoreSetupDataCall() - "
                "****** No free data channels available ******\r\n");
        goto Error;
    }

    pDataCallContextData->uiCID = uiCID;

    RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - stPdpData.szRadioTechnology=[%s]\r\n",
            stPdpData.szRadioTechnology);
    RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - stPdpData.szRILDataProfile=[%s]\r\n",
            stPdpData.szRILDataProfile);
    RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - stPdpData.szApn=[%s]\r\n", stPdpData.szApn);
    RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - stPdpData.szUserName=[%s]\r\n",
            stPdpData.szUserName);
    RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - stPdpData.szPassword=[%s]\r\n",
            stPdpData.szPassword);
    RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - stPdpData.szPAPCHAP=[%s]\r\n",
            stPdpData.szPAPCHAP);

    // if user name is empty, always use no authentication
    if (stPdpData.szUserName == NULL || strlen(stPdpData.szUserName) == 0)
    {
        nPapChap = 0;    // No authentication
    }
    else
    {
        // PAP/CHAP auth type 3 (PAP or CHAP) is not supported. In this case if a
        // a username is defined we will use PAP for authentication.
        // Note: due to an issue in the Android/Fw (missing check of the username
        // length), if the authentication is not defined, it's the value 3 (PAP or
        // CHAP) which is sent to RRIL by default.
        nPapChap = atoi(stPdpData.szPAPCHAP);
        if (nPapChap == 3)
        {
            nPapChap = 1;    // PAP authentication

            RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - New PAP/CHAP=[%d]\r\n", nPapChap);
        }
    }

    if (RIL_VERSION >= 4 && (uiDataSize >= (7 * sizeof(char*))))
    {
        stPdpData.szPDPType = ((char**)pData)[6];
        RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - stPdpData.szPDPType=[%s]\r\n",
                stPdpData.szPDPType);
    }


    if (IsImsEnabledApn(stPdpData.szApn))
    {
        requestPcscfFlag = 1;
        imCnSignallingFlagInd = 1;
    }
    if (IsEImsEnabledApn(stPdpData.szApn))
    {
        requestPcscfFlag = 1;
        imCnSignallingFlagInd = 1;
        emergencyFlag = 1;
    }
    if (IsRcsEnabledApn(stPdpData.szApn))
    {
        requestPcscfFlag = 1;
    }

    //
    //  IP type is passed in dynamically.
    if (NULL == stPdpData.szPDPType)
    {
        //  hard-code "IPV4V6" (this is the default)
        stPdpData.szPDPType = szIPV4V6;
    }

    //  IP type is passed in dynamically.
    if (NULL == stPdpData.szPDPType)
    {
        //  hard-code "IPV4V6" (this is the default)
        CopyStringNullTerminate(stPdpData.szPDPType, PDPTYPE_IPV4V6, sizeof(stPdpData.szPDPType));
    }

    if (nReqType != 1 && uiDataSize >= (8 * sizeof(char*)))
    {
        stPdpData.szHandover = ((char**)pData)[7];  // new 27.007 R12.
        RIL_LOG_INFO("CTE_XMM7260::CoreSetupDataCall() - stPdpData.szHandover=[%s]\r\n",
            stPdpData.szHandover);
        if (atoi(stPdpData.szHandover) != 0) {
            nReqType = 3; // 3 => PDP context is for handover
        } else {
            nReqType = 2; // 2 => new PDP context
        }
    }

    //  dynamic PDP type, need to set XDNS parameter depending on szPDPType.
    //  If not recognized, just use IPV4V6 as default.
    uiDnsMode = GetXDNSMode(stPdpData.szPDPType);
    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CGDCONT=%d,\"%s\",\"%s\",,0,0,,%d,%d,%d;+XGAUTH=%d,%u,\"%s\",\"%s\";+XDNS=%d,%u\r",
            uiCID, stPdpData.szPDPType, stPdpData.szApn, nReqType, requestPcscfFlag,
            imCnSignallingFlagInd, uiCID, nPapChap, stPdpData.szUserName, stPdpData.szPassword,
            uiCID, uiDnsMode))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CoreSetupDataCall() -"
                " cannot create CGDCONT command, stPdpData.szPDPType\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pDataCallContextData);
    }
    else
    {
        pChannelData->SetDataState(E_DATA_STATE_INITING);

        rReqData.pContextData = (void*)pDataCallContextData;
        rReqData.cbContextData = sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA);
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::CoreSetupDataCall() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
//
RIL_RESULT_CODE CTE_XMM7260::CoreReportStkServiceRunning(REQUEST_DATA& /*rReqData*/,
                                                           void* /*pData*/,
                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CoreReportStkServiceRunning() - Enter/Exit\r\n");

    m_usatInitStateMachine.SendEvent(UsatInitStateMachine::STK_SERVICE_RUNNING);

    // complete request without sending the AT command to modem.
    return RRIL_RESULT_OK_IMMEDIATE;
}

BOOL CTE_XMM7260::ParseActivateUsatProfile(const char* pszResponse)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseActivateUsatProfile() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = pszResponse;
    UINT32 uiUiccState = 0;
    UINT32 uiCmeError = 0;
    UINT32 uiAdditionalProfileSupport = 0;

    // Parse "+CUSATA: "
    if (!FindAndSkipString(pszRsp, "+CUSATA: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseActivateUsatProfile() -"
                " Could not skip over \"+CUSATA: \".\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, uiUiccState, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseActivateUsatProfile() -"
                " Could not find UICC state argument.\r\n");
        goto Error;
    }
    else
    {
        RIL_LOG_INFO("CTE_XMM7260::ParseActivateUsatProfile() -"
                " uiUiccState= %u.\r\n", uiUiccState);
        m_usatInitStateMachine.SetUiccState(uiUiccState);
        if (uiUiccState == UsatInitStateMachine::UICC_ACTIVE)
        {
            m_usatInitStateMachine.SendEvent(UsatInitStateMachine::SIM_READY_TO_ACTIVATE);
        }
    }

    // Parse "," if additional Profile Support exists
    if (SkipString(pszRsp, ",", pszRsp))
    {
        if (ExtractUInt32(pszRsp, uiAdditionalProfileSupport, pszRsp))
        {
            RIL_LOG_INFO("CTE_XMM7260::ParseActivateUsatProfile() -"
                    " Found Additional Profile Support argument= %u.\r\n",
                    uiAdditionalProfileSupport);
        }
    }

    // Parse "<CR><LF>", to check if "CME error" or "OK" answer is present
    if (FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        if (!SkipString(pszRsp, m_szNewLine, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseActivateUsatProfile() -"
                    " Could not extract response prefix.\r\n");
            goto Error;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseActivateUsatProfile - "
                "Could not skip response postfix.\r\n");
        goto Error;
    }

    // Parse "+CME ERROR: " if present
    if (SkipString(pszRsp, "+CME ERROR: ", pszRsp))
    {
        if (!ExtractUInt32(pszRsp, uiCmeError, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseActivateUsatProfile() -"
                   " Could not find CME argument.\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseActivateUsatProfile() -"
                    " CME ERROR: %u \r\n", uiCmeError);
        }
    }

    res = RRIL_RESULT_OK;

Error:
    m_usatInitStateMachine.SetAddProfileSupport(uiAdditionalProfileSupport);
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseActivateUsatProfile() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7260::ParseReportStkServiceRunning(RESPONSE_DATA& /*rspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseReportStkServiceRunning() - Enter / Exit\r\n");

    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_STK_GET_PROFILE
//
RIL_RESULT_CODE CTE_XMM7260::CoreStkGetProfile(REQUEST_DATA& /*rReqData*/,
                                                           void* /*pData*/,
                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CoreStkGetProfile() - Enter/Exit\r\n");
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

//
// RIL_REQUEST_STK_SET_PROFILE
//
RIL_RESULT_CODE CTE_XMM7260::CoreStkSetProfile(REQUEST_DATA& /*rReqData*/,
                                                           void* /*pData*/,
                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CoreStkSetProfile() - Enter/Exit\r\n");
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

//
// RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
//
RIL_RESULT_CODE CTE_XMM7260::CoreStkSendTerminalResponse(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CoreStkSendTerminalResponse() - Enter\r\n");
    char* pszTermResponse = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    // AT+CUSATT=<terminal_response>
    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CoreStkSendTerminalResponse() -"
                " Data pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != sizeof(char*))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CoreStkSendTerminalResponse() - Invalid data size.\r\n");
        goto Error;
    }
    RIL_LOG_INFO("CTE_XMM7260::CoreStkSendTerminalResponse() - uiDataSize= %d\r\n", uiDataSize);

    pszTermResponse = (char*)pData;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CUSATT=\"%s\"\r", pszTermResponse))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CoreStkSendTerminalResponse() -"
                " Could not form string.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::CoreStkSendTerminalResponse() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7260::ParseStkSendTerminalResponse(RESPONSE_DATA& /*rspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseStkSendTerminalResponse() - Enter/Exit\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    return res;
}

//
// RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
//
RIL_RESULT_CODE CTE_XMM7260::CoreStkSendEnvelopeCommand(REQUEST_DATA& rReqData,
                                                                    void* pData,
                                                                    UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CoreStkSendEnvelopeCommand() - Enter\r\n");
    char* pszEnvCommand = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    /* AT+CUSATE=<envelope_command> */

    if (sizeof(char*) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CoreStkSendEnvelopeCommand() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CoreStkSendEnvelopeCommand() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    // extract data
    pszEnvCommand = (char*)pData;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CUSATE=\"%s\"\r", pszEnvCommand))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CoreStkSendEnvelopeCommand() -"
                " Could not form string.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::CoreStkSendEnvelopeCommand() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7260::ParseEnvelopCommandResponse(const char* pszResponse,
        char* pszEnvelopResp, UINT32* puiBusy, UINT32* puiSw1, UINT32* puiSw2)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseEnvelopCommandResponse() - Enter\r\n");
    const char* pszRsp = pszResponse;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    // +CUSATE:<envelope_response>[,<busy>][<CR><LF>+CUSATE2:<sw1>,<sw2>]

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseEnvelopCommandResponse() -"
                " Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "+CUSATE: "
    if (!FindAndSkipString(pszRsp, "+CUSATE: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseEnvelopCommandResponse() -"
                " Could not skip over \"+CUSATE: \".\r\n");
        goto Error;
    }

    // Parse "<envelope_response>"
    if (!ExtractQuotedString(pszRsp, pszEnvelopResp, MAX_BUFFER_SIZE, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseEnvelopCommandResponse() -"
                " Could not parse envelope response.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM7260::ParseEnvelopCommandResponse() - response data: \"%s\".\r\n",
            pszEnvelopResp);

    // Parse "," if optional <Busy> exists
    if (SkipString(pszRsp, ",", pszRsp))
    {
        if (!ExtractUInt32(pszRsp, *puiBusy, pszRsp))
        {
            RIL_LOG_INFO("CTE_XMM7260::ParseEnvelopCommandResponse() -"
                    " Could not extract Busy.\r\n");
            goto Error;
        }
    }

    // Parse "<CR><LF>", to check if +CUSATE2 error is present
    if (FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        if (!SkipString(pszRsp, m_szNewLine, pszRsp))
        {
            RIL_LOG_INFO("CTE_XMM7260::ParseEnvelopCommandResponse() -"
                    " Could not extract response prefix.\r\n");
            goto Error;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseEnvelopCommandResponse - "
                "Could not skip response postfix.\r\n");
        goto Error;
    }

    // Parse "+CUSATE2", added in 27.007 V11.8.0 (Rel.11)
    // Note: It might not be yet supported by the modem, but is mandatory
    // as fix for a defect of 27.007 spec Rel.10
    if (SkipString(pszRsp, "+CUSATE2: ", pszRsp))
    {
        // Parse "<sw1>"
        if (!ExtractUInt32(pszRsp, *puiSw1, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseEnvelopCommandResponse() -"
                    " Could not extract sw1.\r\n");
            goto Error;
        }

        // Parse ",<sw2>"
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, *puiSw2, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseEnvelopCommandResponse() -"
                    " Could not extract sw2.\r\n");
            goto Error;
        }

        if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseEnvelopCommandResponse - "
                    "Could not skip response postfix.\r\n");
            goto Error;
        }

        RIL_LOG_INFO("CTE_XMM7260::ParseEnvelopCommandResponse() - "
                " +CUSATE2: sw1=%u, sw2=%u\r\n", *puiSw1, *puiSw2);
    }
    else
    {
        RIL_LOG_INFO("CTE_XMM7260::ParseEnvelopCommandResponse() -"
                " Could not find \"+CUSATE2: \".\r\n");
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseEnvelopCommandResponse() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7260::ParseStkSendEnvelopeCommand(RESPONSE_DATA& rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseStkSendEnvelopeCommand() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiSw1 = 0;
    UINT32 uiSw2 = 0;
    UINT32 uiBusy = 0;
    const char* pszRsp = rspData.szResponse;
    char* pszEnvelopResp = NULL;

    pszEnvelopResp = (char*)malloc(MAX_BUFFER_SIZE);
    if (NULL == pszEnvelopResp)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseStkSendEnvelopeCommand() -"
                " Could not alloc mem for command.\r\n");
        goto Error;
    }
    memset(pszEnvelopResp, 0x00, MAX_BUFFER_SIZE);

    if (ParseEnvelopCommandResponse(pszRsp, pszEnvelopResp, &uiBusy, &uiSw1, &uiSw2) !=
            RRIL_RESULT_OK)
    {
        goto Error;
    }

    rspData.uiDataSize = strlen(pszEnvelopResp);
    if (rspData.uiDataSize == 0)
    {
        free(pszEnvelopResp);
        pszEnvelopResp = NULL;
    }
    rspData.pData = (void*)pszEnvelopResp;

    res = RRIL_RESULT_OK;

Error:
    if (res != RRIL_RESULT_OK)
    {
        free(pszEnvelopResp);
        pszEnvelopResp = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::ParseStkSendEnvelopeCommand() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
//
RIL_RESULT_CODE CTE_XMM7260::ParseStkSendEnvelopeWithStatus(RESPONSE_DATA& rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseStkSendEnvelopeWithStatus() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiSw1 = 0;
    UINT32 uiSw2 = 0;
    UINT32 uiBusy = 0;
    const char* pszRsp = rspData.szResponse;
    char* pszEnvelopResp = NULL;
    UINT32 cbEnvelopResp = 0;
    RIL_SIM_IO_Response* pResponse = NULL;

    pszEnvelopResp = (char*)malloc(MAX_BUFFER_SIZE);
    if (NULL == pszEnvelopResp)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseStkSendEnvelopeWithStatus() -"
                " Could not alloc mem for command.\r\n");
        goto Error;
    }

    memset(pszEnvelopResp, 0x00, MAX_BUFFER_SIZE);

    res = ParseEnvelopCommandResponse(pszRsp, pszEnvelopResp, &uiBusy, &uiSw1, &uiSw2);
    cbEnvelopResp = strlen(pszEnvelopResp);

    if (0 != cbEnvelopResp)
    {
        // Allocate memory for the RIL_SIM_IO_Response struct + sim response string.
        // The char* in the RIL_SIM_IO_Response will point to the buffer allocated
        // directly after the RIL_SIM_IO_Response.  When the RIL_SIM_IO_Response
        // is deleted, the corresponding response string will be freed as well.
        pResponse = (RIL_SIM_IO_Response*)malloc(sizeof(RIL_SIM_IO_Response)
                + cbEnvelopResp + 1);

        if (NULL == pResponse)
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseStkSendEnvelopeWithStatus() -"
                    " Could not allocate memory for a RIL_SIM_IO_Response struct.\r\n");
            goto Error;
        }
        memset(pResponse, 0, sizeof(RIL_SIM_IO_Response) + cbEnvelopResp + 1);

        // set location to copy sim response string just after RIL_SIM_IO_Response
        pResponse->simResponse = (char*)(((char*)pResponse) + sizeof(RIL_SIM_IO_Response));

        // Ensure NULL termination
        pResponse->simResponse[cbEnvelopResp] = '\0';

        if (!CopyStringNullTerminate(pResponse->simResponse, pszEnvelopResp, cbEnvelopResp+1))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseStkSendEnvelopeWithStatus() -"
                    " Cannot CopyStringNullTerminate szResponseString\r\n");
            goto Error;
        }
    }
    else
    {
        pResponse = (RIL_SIM_IO_Response*)malloc(sizeof(RIL_SIM_IO_Response));
        if (NULL == pResponse)
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseStkSendEnvelopeWithStatus() -"
                    " Could not allocate memory for a RIL_SIM_IO_Response struct.\r\n");
            goto Error;
        }
        pResponse->simResponse = NULL;
    }

    pResponse->sw1 = uiSw1;
    pResponse->sw2 = uiSw2;

    rspData.pData = (void*)pResponse;
    rspData.uiDataSize = sizeof(RIL_SIM_IO_Response);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    free(pszEnvelopResp);
    pszEnvelopResp = NULL;

    RIL_LOG_VERBOSE("CTE_XMM7260::ParseStkSendEnvelopeWithStatus() - Exit\r\n");
    return res;
}

// CUSATD, which determines which profile should be downloaded to the UICC automatically after
// start-up, is oneshot and needs to be resent atfer each reset
void CTE_XMM7260::SetProfileDownloadForNextUiccStartup(UINT32 uiDownload, UINT32 uiReporting)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::SetProfileDownloadForNextUiccStartup() - Enter\r\n");
    char szCommand[MAX_BUFFER_SIZE];
    CCommand* pCmd = NULL;

    if (!PrintStringNullTerminate(szCommand, MAX_BUFFER_SIZE, "AT+CUSATD=%u,%u\r", uiDownload,
            uiReporting))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::SetProfileDownloadForNextUiccStartup() - "
                "Could not form string. Exit\r\n");
        return;
    }

    pCmd = new CCommand(g_ReqInternal[E_REQ_IDX_SET_PROFILE_DOWNLOAD_FOR_NEXT_UICC_STARTUP]
            .reqInfo.uiChannel, NULL,
            g_ReqInternal[E_REQ_IDX_SET_PROFILE_DOWNLOAD_FOR_NEXT_UICC_STARTUP].reqId, szCommand);

    if (NULL != pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::SetProfileDownloadForNextUiccStartup() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::SetProfileDownloadForNextUiccStartup() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::SetProfileDownloadForNextUiccStartup() - Exit\r\n");
}

// Start the serie of AT command which starts with the configuration of profile download
void CTE_XMM7260::ConfigureUsatProfileDownload(UINT32 uiDownload, UINT32 uiReporting)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ConfigureUsatProfileDownload() - Enter\r\n");
    char szCommand[MAX_BUFFER_SIZE];
    CCommand* pCmd = NULL;

    if (!PrintStringNullTerminate(szCommand, MAX_BUFFER_SIZE, "AT+CUSATD=%u,%u\r", uiDownload,
            uiReporting))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ConfigureUsatProfileDownload() - "
                "Could not form string. Exit\r\n");
        return;
    }

    pCmd = new CCommand(g_ReqInternal[E_REQ_IDX_CONFIGURE_USAT_PROFILE_DOWNLOAD].reqInfo.uiChannel,
            NULL, g_ReqInternal[E_REQ_IDX_CONFIGURE_USAT_PROFILE_DOWNLOAD].reqId, szCommand, NULL,
            &CTE::PostConfigureUsatProfileDownloadHandler);

    if (NULL != pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ConfigureUsatProfileDownload() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ConfigureUsatProfileDownload() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::ConfigureUsatProfileDownload() - Exit\r\n");
}

void CTE_XMM7260::PostConfigureUsatProfileDownloadHandler(POST_CMD_HANDLER_DATA& data)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::PostConfigureUsatProfileDownloadHandler() - Enter\r\n");

    if (RIL_E_SUCCESS == data.uiResultCode)
    {
        QueryUiccState();
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::PostConfigureUsatProfileDownloadHandler() - Enter\r\n");
}

// By sending CUSATA, UICC state is retrieved and if the UICC supports the "Additional TERMINAL
// PROFILE after UICC activation" feature
void CTE_XMM7260::QueryUiccState()
{
    RIL_LOG_VERBOSE("CTE_XMM7260::QueryUiccState() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_ReqInternal[E_REQ_IDX_QUERY_UICC_STATE].reqInfo.uiChannel,
            NULL, g_ReqInternal[E_REQ_IDX_QUERY_UICC_STATE].reqId, "AT+CUSATA\r",
            &CTE::ParseQueryUiccState, &CTE::PostQueryUiccStateHandler);

    if (NULL != pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::QueryUiccState() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::QueryUiccState() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::QueryUiccState() - Exit\r\n");
}

RIL_RESULT_CODE CTE_XMM7260::ParseQueryUiccState(RESPONSE_DATA& rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseQueryUiccState() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rspData.szResponse;

    res = ParseActivateUsatProfile(pszRsp);

    RIL_LOG_VERBOSE("CTE_XMM7260::ParseQueryUiccState() - Exit\r\n");
    return res;
}

void CTE_XMM7260::PostQueryUiccStateHandler(POST_CMD_HANDLER_DATA& data)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::PostQueryUiccStateHandler() - Enter\r\n");

    if (RIL_E_SUCCESS == data.uiResultCode)
    {
        ReadUsatProfiles();
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::PostQueryUiccStateHandler() - Enter\r\n");
}

void CTE_XMM7260::ReadUsatProfiles()
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ReadUsatProfiles() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_ReqInternal[E_REQ_IDX_READ_USAT_PROFILES].reqInfo.uiChannel,
            NULL, g_ReqInternal[E_REQ_IDX_READ_USAT_PROFILES].reqId, "AT+CUSATR\r",
            &CTE::ParseReadUsatProfiles);

    if (NULL != pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ReadUsatProfiles() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ReadUsatProfiles() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::ReadProfiles() - Exit\r\n");
}

RIL_RESULT_CODE CTE_XMM7260::ParseReadUsatProfiles(RESPONSE_DATA& rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseReadUsatProfiles() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rspData.szResponse;
    UINT32 uiProfileStorage;
    char* pszProfile = NULL;
    UINT32 uiProfileLen = 0;
    // array of read profiles
    char** ppszProfiles
            = (char**) malloc(UsatInitStateMachine::PROFILE_READ_NUMBER * sizeof(char*));

    if (NULL == ppszProfiles)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                " Could not allocate memory for array of profiles.\r\n");
        goto Error;
    }

    memset(ppszProfiles, 0, UsatInitStateMachine::PROFILE_READ_NUMBER * sizeof(char*));

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() - Response string is NULL!\r\n");
        goto Error;
    }

    // Skip over the <cr><lf> prefix
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                " Could not skip over response prefix.\r\n");
        goto Error;
    }

    do
    {
        // Parse "+CUSATR: "
        if (!SkipString(pszRsp, "+CUSATR: ", pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                    " Could not skip over \"+CUSATR: \".\r\n");
            goto Error;
        }

        if (!ExtractUInt32(pszRsp, uiProfileStorage, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                    " Could not find profile storage argument.\r\n");
            goto Error;
        }

        // Parse "," if additional profile exists
        if (!SkipString(pszRsp, ",", pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                    " Missing profile argument\r\n");
            goto Error;
        }

        if (!ExtractQuotedStringWithAllocatedMemory(pszRsp, pszProfile, uiProfileLen, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                    " Could not parse profile.\r\n");
            goto Error;
        }

        if (uiProfileStorage < UsatInitStateMachine::PROFILE_READ_NUMBER)
        {
            // truncate larger profile to known size (byte represented in 2 hex char)
            ppszProfiles[uiProfileStorage] = strndup(pszProfile,
                    UsatInitStateMachine::MAX_SIZE_PROFILE * 2);

            if (NULL != ppszProfiles[uiProfileStorage])
            {
                RIL_LOG_VERBOSE("CTE_XMM7260::ParseReadUsatProfiles() - ProfileStorage=%u, "
                        "TermProfile=%s", uiProfileStorage, ppszProfiles[uiProfileStorage]);
            }
        }
        else
        {
            RIL_LOG_WARNING("CTE_XMM7260::ParseReadUsatProfiles() - ProfileStorage=%u, "
                    "TermProfile=%s has been ignored", uiProfileStorage, pszProfile);
        }
        delete[] pszProfile;
        pszProfile = NULL;

        // Skip over the <cr><lf> postfix
        if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                    " Could not skip over response postfix.\r\n");
            goto Error;
        }

        // Skip over the <cr><lf> prefix
        if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                    " Could not skip over response prefix.\r\n");
            goto Error;
        }

    } while (!SkipString(pszRsp, "OK", pszRsp));

    // Skip over the <cr><lf> postfix
    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseReadUsatProfiles() -"
                " Could not skip over response postfix.\r\n");
        goto Error;
    }

    m_usatInitStateMachine.SetProfileArray(ppszProfiles);
    m_usatInitStateMachine.SendEvent(UsatInitStateMachine::PROFILE_READ);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        if (ppszProfiles)
        {
            for (UINT32 i = 0; i < UsatInitStateMachine::PROFILE_READ_NUMBER; i++)
            {
                free(ppszProfiles[i]);
            }
            free(ppszProfiles);
        }
    }
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseReadUsatProfiles() - Exit\r\n");
    return res;
}

void CTE_XMM7260::WriteUsatProfiles(const char* pszTeProfile, const BOOL isTeWriteNeeded,
            const char* pszMtProfile, const BOOL isMtWriteNeeded)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::WriteUsatProfiles() - Enter\r\n");

    m_usatInitStateMachine.SetTeWriteNeeded(isTeWriteNeeded);
    m_usatInitStateMachine.SetTeProfileToWrite(pszTeProfile);

    if (isTeWriteNeeded && !isMtWriteNeeded)
    {
        m_usatInitStateMachine.SetTeWriteNeeded(FALSE);
        WriteUsatProfile(0, pszTeProfile);
    }
    else
    {
        WriteUsatProfile(1, pszMtProfile);
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::WriteUsatProfiles() - Exit\r\n");
}


void CTE_XMM7260::WriteUsatProfile(const UINT32 uiProfileStorage, const char* pszProfile)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::WriteUsatProfile() - Enter\r\n");
    char szCommand[MAX_BUFFER_SIZE];
    CCommand* pCmd = NULL;

    if (!PrintStringNullTerminate(szCommand, MAX_BUFFER_SIZE, "AT+CUSATW=%u,\"%s\"\r",
            uiProfileStorage, pszProfile))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::WriteUsatProfile() - Could not form string. Exit\r\n");
        return;
    }

    pCmd = new CCommand(g_ReqInternal[E_REQ_IDX_WRITE_USAT_PROFILE].reqInfo.uiChannel,
            NULL, g_ReqInternal[E_REQ_IDX_WRITE_USAT_PROFILE].reqId, szCommand,
            &CTE::ParseWriteUsatProfile, &CTE::PostWriteUsatProfileHandler);

    if (NULL != pCmd)
    {
        // Method SetAlwaysParse() is called to parse the intermediate response when error is
        // received.
        pCmd->SetAlwaysParse();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::WriteUsatProfile() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::WriteUsatProfile() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::WriteUsatProfile() - Exit\r\n");
}

RIL_RESULT_CODE CTE_XMM7260::ParseWriteUsatProfile(RESPONSE_DATA& rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseWriteUsatProfile() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rspData.szResponse;
    UINT32 uiProfileStorage = 0;
    char* pszConflictProfile = NULL;
    UINT32 uiConflictProfileLen = 0;

    // As method SetAlwaysParse() was set, all answers from Modem needs to be handled
    // For AT+CUSATW (with/out parameters) we can received from Modem the following answers :
    // Basic reponses: "OK" or "CME ERROR: "x" "
    // +CUSATW: <profile_storage>,<conflict_profile> , [<CME ERROR>: "x"]

    // Skip over the <cr><lf> prefix
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseWriteUsatProfile() -"
                " Could not skip over response prefix.\r\n");
        goto Exit;
    }

    if (SkipString(pszRsp, "OK", pszRsp))
    {
        RIL_LOG_INFO("CTE_XMM7260::ParseWriteUsatProfile() - OK found \r\n");
        res = RRIL_RESULT_OK;
        goto Exit;
    }

    if (ParseCmeError(pszRsp, pszRsp))
    {
        goto Exit;
    }

    /* +CUSATW: <profile_storage>,<conflict_profile> */

    // Parse "+CUSATW: "
    if (!SkipString(pszRsp, "+CUSATW: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseWriteUsatProfile() -"
                " Could not skip over \"+CUSATW: \".\r\n");
        goto Exit;
    }

    if (!ExtractUInt32(pszRsp, uiProfileStorage, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseWriteUsatProfile() -"
                " Could not find profile storage argument.\r\n");
        goto Exit;
    }
    else
    {
        RIL_LOG_INFO("CTE_XMM7260::ParseWriteUsatProfile() -"
                " uiProfileStorage= %u.\r\n", uiProfileStorage);
    }

    // Parse "," if conflict profile exists
    if (!SkipString(pszRsp, ",", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseWriteUsatProfile() -"
                " Missing profile argument\r\n");
        goto Exit;
    }

    if (!ExtractQuotedStringWithAllocatedMemory(pszRsp, pszConflictProfile, uiConflictProfileLen,
            pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseWriteUsatProfile() -"
                " Could not parse conflict profile.\r\n");
        goto Exit;
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseWriteUsatProfile() -"
                " A conflict happened while writing profile");
    }

    // Parse "<CR><LF>", to check if CME error is present
    if (SkipString(pszRsp, m_szNewLine, pszRsp))
    {
        if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseWriteUsatProfile() -"
                    " Could not extract response prefix.\r\n");
            goto Exit;
        }
    }

    if (!ParseCmeError(pszRsp, pszRsp))
    {
        goto Exit;
    }

Exit:
    delete[] pszConflictProfile;
    pszConflictProfile = NULL;

    RIL_LOG_VERBOSE("CTE_XMM7260::ParseWriteUsatProfile() - Exit\r\n");
    return res;
}

void CTE_XMM7260::PostWriteUsatProfileHandler(POST_CMD_HANDLER_DATA& data)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::PostWriteUsatProfileHandler() - Enter\r\n");

    if (RIL_E_SUCCESS == data.uiResultCode)
    {
        if (m_usatInitStateMachine.GetTeWriteNeeded())
        {
            m_usatInitStateMachine.SetTeWriteNeeded(FALSE);
            WriteUsatProfile(0, m_usatInitStateMachine.GetTeProfileToWrite());
        }
        else
        {
            m_usatInitStateMachine
                    .SendEvent(UsatInitStateMachine::SIM_READY_FOR_RESET);
        }
    }
    RIL_LOG_VERBOSE("CTE_XMM7260::PostWriteUsatProfileHandler() - Exit\r\n");
}

void CTE_XMM7260::ResetUicc()
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ResetUicc() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_ReqInternal[E_REQ_IDX_RESET_UICC].reqInfo.uiChannel,
            NULL, g_ReqInternal[E_REQ_IDX_RESET_UICC].reqId, "AT+CFUN=27,1\r");

    if (NULL != pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ResetUicc() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ResetUicc() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::ResetUicc() - Exit\r\n");
}

void CTE_XMM7260::NotifyUiccReady()
{
    RIL_LOG_INFO("CTE_XMM7260::NotifyUiccReady()\r\n");

    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
}

// This will activate the TE profile handling facility and +CUSATP: <proactive_command> and
// +CUSATEND will be received
void CTE_XMM7260::EnableProfileFacilityHandling()
{
    RIL_LOG_VERBOSE("CTE_XMM7260::EnableProfileFacilityHandling() - Enter\r\n");

    CCommand* pCmd = new CCommand(
            g_ReqInternal[E_REQ_IDX_ENABLE_PROFILE_FACILITY_HANDLING].reqInfo.uiChannel,
            NULL, g_ReqInternal[E_REQ_IDX_ENABLE_PROFILE_FACILITY_HANDLING].reqId, "AT+CUSATA=1\r");

    if (NULL != pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::EnableProfileFacilityHandling() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::EnableProfileFacilityHandling() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::EnableProfileFacilityHandling() - Exit\r\n");
}

void CTE_XMM7260::SendModemDownToUsatSM()
{
    m_usatInitStateMachine.SendEvent(UsatInitStateMachine::SIM_UNAVAILABLE);
}

BOOL CTE_XMM7260::ParseCmeError(const char* pszStart, const char*& pszEnd)
{
    UINT32 uiCmeError = 0;
    const char* pszWalk = pszStart;
    BOOL bRet = FALSE;

    if (SkipString(pszWalk, "+CME ERROR: ", pszWalk))
    {
        bRet = TRUE;
        if (!ExtractUInt32(pszWalk, uiCmeError, pszWalk))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseCmeError() -"
                    " Could not find CME argument.\r\n");
            goto Out;
        }
        else
        {
            RIL_LOG_INFO("CTE_XMM7260::ParseCmeError() -"
                    " CME ERROR: %u \r\n", uiCmeError);
        }

        if (!SkipRspEnd(pszWalk, m_szNewLine, pszWalk))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseCmeError() -"
                    " Could not skip over response postfix.\r\n");
        }
    }

Out:
    pszEnd = (bRet) ? pszWalk : pszStart;

    return bRet;
}

RIL_RESULT_CODE CTE_XMM7260::CreateGetGprsCellEnvReq(REQUEST_DATA& /*reqData*/)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTE_XMM7260::CreateDebugScreenReq(REQUEST_DATA& /*reqData*/,
        const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

const char* CTE_XMM7260::GetEnableFetchingString()
{
    return NULL;
}

const char* CTE_XMM7260::GetSiloVoiceURCInitString()
{
    const char* pszInit = NULL;
    if (m_cte.IsVoiceCapable())
    {
        if (m_cte.IsDataCapable())
        {
            pszInit = "|+XCALLSTAT=1|+CSSN=1,1|+CNAP=1";
        }
        else
        {
            pszInit = "|+XCALLSTAT=1|+XCGCLASS=\"CC\"|+CSSN=1,1|+CNAP=1";
        }
    }
    else
    {
        pszInit = "|+XCSFB=3|+XCGCLASS=\"CG\"|+XCONFIG=3,0";
    }
    return pszInit;
}

RIL_RESULT_CODE CTE_XMM7260::QueryPinRetryCount(REQUEST_DATA& reqData,
                                                        void* /*pData*/,
                                                        UINT32 /*uiDataSize*/)
{
    RIL_RESULT_CODE res = RIL_E_GENERIC_FAILURE;

    const char* pszSelCode = NULL;

    //  Extract request id from context
    int nRequestId = *((int*)reqData.pContextData);

    if (0 == nRequestId)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::QueryPinRetryCount() - nRequestId is 0\r\n");
        goto Error;
    }

    switch (nRequestId)
    {
        case RIL_REQUEST_ENTER_SIM_PIN:
        case RIL_REQUEST_CHANGE_SIM_PIN:
            pszSelCode = "SIM PIN";
            break;
        case RIL_REQUEST_ENTER_SIM_PIN2:
        case RIL_REQUEST_CHANGE_SIM_PIN2:
            pszSelCode = "SIM PIN2";
            break;
        case RIL_REQUEST_ENTER_SIM_PUK:
            pszSelCode = "SIM PUK";
            break;
        case RIL_REQUEST_ENTER_SIM_PUK2:
            pszSelCode = "SIM PUK2";
            break;
        default:
            pszSelCode = "SIM*"; // means status for all
            break;
    }

    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1), "AT+CPINR=\"%s\"\r",
            pszSelCode))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::QueryPinRetryCount() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::QueryPinRetryCount() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7260::ParseSimPinRetryCount(RESPONSE_DATA& rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseSimPinRetryCount() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    const char* pszRsp = rspData.szResponse;

    UINT32 uiPinPuk = 0;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseSimPinRetryCount() -"
                "Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "<prefix>+CPINR: <cod>,<retries>,<default_retries>
    // Loop on +CPINR until no more entries are found
    while (FindAndSkipString(pszRsp, "+CPINR: ", pszRsp))
    {
        // Skip <cod>
        if (SkipString(pszRsp, "SIM PIN,", pszRsp))
        {
            if (!ExtractUInt32(pszRsp, uiPinPuk, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM7260::ParseSimPinRetryCount() -"
                        "Cannot parse SIM PIN\r\n");
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
                // Set pin retries to -1 (unknown)
                for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
                {
                    m_CardStatusCache.applications[i].pin1_num_retries = -1;
                }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED
            }
            else
            {
                RIL_LOG_INFO("CTE_XMM7260::ParseSimPinRetryCount() -"
                        " retry pin1:%d\r\n",uiPinPuk);
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
                for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
                {
                    m_CardStatusCache.applications[i].pin1_num_retries = uiPinPuk;
                }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED

                m_PinRetryCount.pin = uiPinPuk;
            }
        }
        // Skip <cod>
        else if (SkipString(pszRsp, "SIM PUK,", pszRsp))
        {
            if (!ExtractUInt32(pszRsp, uiPinPuk, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM7260::ParseSimPinRetryCount() -"
                        "Cannot parse SIM PUK\r\n");
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
                // Set pin retries to -1 (unknown)
                for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
                {
                    m_CardStatusCache.applications[i].puk1_num_retries = -1;
                }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED
            }
            else
            {
                RIL_LOG_INFO("CTE_XMM7260::ParseSimPinRetryCount() -"
                        " retry puk1:%d\r\n",uiPinPuk);
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
                for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
                {
                    m_CardStatusCache.applications[i].puk1_num_retries = uiPinPuk;
                }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED

                m_PinRetryCount.puk = uiPinPuk;
            }
        }
        // Skip <cod>
        else if (SkipString(pszRsp, "SIM PIN2,", pszRsp))
        {
            if (!ExtractUInt32(pszRsp, uiPinPuk, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM7260::ParseSimPinRetryCount() -"
                        "Cannot parse SIM PIN2\r\n");
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
                // Set pin retries to -1 (unknown)
                for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
                {
                    m_CardStatusCache.applications[i].pin2_num_retries = -1;
                }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED
            }
            else
            {
                RIL_LOG_INFO("CTE_XMM7260::ParseSimPinRetryCount() -"
                        " retry pin2:%d\r\n",uiPinPuk);
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
                for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
                {
                    m_CardStatusCache.applications[i].pin2_num_retries = uiPinPuk;
                }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED

                m_PinRetryCount.pin2 = uiPinPuk;
            }
        }
        // Skip <cod>
        else if (SkipString(pszRsp, "SIM PUK2,", pszRsp))
        {
            if (!ExtractUInt32(pszRsp, uiPinPuk, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM7260::ParseSimPinRetryCount() -"
                        "Cannot parse SIM PUK2\r\n");
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
                // Set pin retries to -1 (unknown)
                for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
                {
                    m_CardStatusCache.applications[i].puk2_num_retries = -1;
                }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED
            }
            else
            {
                RIL_LOG_INFO("CTE_XMM7260::ParseSimPinRetryCount() -"
                        " retry puk2:%d\r\n",uiPinPuk);
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
                for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
                {
                    m_CardStatusCache.applications[i].puk2_num_retries = uiPinPuk;
                }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED

                m_PinRetryCount.puk2 = uiPinPuk;
            }
        }
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::ParseSimPinRetryCount() - Exit\r\n");
    return res;
}

void CTE_XMM7260::QueryPinRetryCount()
{
    RIL_LOG_VERBOSE("CTE_XMM7260::QueryPinRetryCount() - Enter\r\n");

    char szCmd[17] = {'\0'};
    const char* pszRetryCode = "SIM*";

    if (!PrintStringNullTerminate(szCmd, sizeof(szCmd), "AT+CPINR=\"%s\"\r", pszRetryCode))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::QueryPinRetryCount() - Can't construct szCmd.\r\n");
    }

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_GET_SIM_STATUS].uiChannel,
            NULL, RIL_REQUEST_GET_SIM_STATUS, szCmd, &CTE::ParseSimPinRetryCount);

    if (NULL == pCmd)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::QueryPinRetryCount() - "
                "Unable to allocate memory for new command!\r\n");
    }
    else
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::QueryPinRetryCount() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM7260::QueryPinRetryCount() - Exit\r\n");
}

P_ND_N_CELL_INFO_DATA_V2 CTE_XMM7260::ParseXMCI(RESPONSE_DATA& rspData, int& nCellInfos)
{
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int index = 0;
    int mcc = INT_MAX;
    int mnc = INT_MAX;
    int lac = INT_MAX;
    unsigned int uCi = INT_MAX;
    const char* pszRsp = rspData.szResponse;
    P_ND_N_CELL_INFO_DATA_V2 pCellData = NULL;
    const int MCC_UNKNOWN = 0;
    const int MNC_UNKNOWN = 0;
    const int LAC_UNKNOWN = 0xFFFE;
    const unsigned int CI_UNKNOWN = 0xFFFFFFFF;
    const int BSIC_UNKNOWN = 0xFF;
    const int PSC_UNKNOWN = 0xFFFF;
    const int PCI_UNKNOWN = 0xFFFF;
    const int ARFCN_UNKNOWN = 0xFFFF;
    const unsigned int DL_UARFCN_UNKNOWN = 0xFFFFFFFF;
    const unsigned int UL_UARFCN_UNKNOWN = 0xFFFFFFFF;
    const int PATHLOSS_UNKNOWN = -1;
    const int CQI_UNKNOWN = 0;
    const int TA_INVALID = 0x7FFFFFFF;
    uint64_t timestamp = ril_nano_time();

    const unsigned int PATHLOSS_LTE_UNKNOWN = 0xFFFFFFFF;
    const int TA_LTE_INVALID = 0x0000FFFF;
    const int LAC_UNKNOWN_BIS = 0x0000;

    pCellData = (P_ND_N_CELL_INFO_DATA_V2) malloc(sizeof(S_ND_N_CELL_INFO_DATA_V2));
    if (NULL == pCellData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseXMCI() -"
                " Could not allocate memory for a S_ND_N_CELL_INFO_DATA_V2 struct.\r\n");
        goto Error;
    }
    memset(pCellData, 0, sizeof(S_ND_N_CELL_INFO_DATA_V2));

    /*
     * GSM serving and neighboring cell:
     *
     * +XMCI: 0,<MCC>,<MNC>,<LAC>,<CI>,<BSIC>,<RXLEV>,<BER>,<ARFCN>,<taReliability>,<ta>
     * +XMCI: 1,<MCC>,<MNC>,<LAC>,<CI>,<BSIC>,<RXLEV>,<BER>,<ARFCN>,<taReliability>,<ta>
     *
     * UMTS serving and neighboring cell:
     *
     * +XMCI: 2,<MCC>,<MNC>,<LAC>,<CI>,<PSC>,<DLUARFCN>,<ULUARFCN>,<PATHLOSS>,<RSSI>,<RSCP>,<ECNO>
     * +XMCI: 3,<MCC>,<MNC>,<LAC>,<CI>,<PSC>,<DLUARFCN>,<ULUARFCN>,<PATHLOSS>,<RSSI>,<RSCP>,<ECNO>
     *
     * LTE serving and neighboring cell:
     *
     * +XMCI: 4,<MCC>,<MNC>,<TAC>,<CI>,<PCI>,<DLUARFCN>,<ULUARFCN>,<PATHLOSS>,<RSRP>,<RSRQ>,
     *          <RSSNR>,<TA>,<CQI>
     * +XMCI: 5,<MCC>,<MNC>,<TAC>,<CI>,<PCI>,<DLUARFCN>,<ULUARFCN>,<PATHLOSS>,<RSRP>,<RSRQ>,
     *          <RSSNR>,<TA>,<CQI>
     */

    // Loop on +XMCI until no more entries are found
    while (FindAndSkipString(pszRsp, "+XMCI: ", pszRsp))
    {
        int type = 0;

        if (RRIL_MAX_CELL_ID_COUNT == index)
        {
            //  We're full.
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseXMCI() -"
                    " Exceeded max count = %d\r\n", RRIL_MAX_CELL_ID_COUNT);
            break;
        }

        // Read <TYPE>
        if (!ExtractInt(pszRsp, type, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::ParseXMCI() -"
                    " could not extract <TYPE> value\r\n");
            continue;
        }

        // Read <MCC>
        if ((!SkipString(pszRsp, ",", pszRsp))
                || (!ExtractInt(pszRsp, mcc, pszRsp)))
        {
            RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                    " could not extract <MCC> value\r\n");
            continue;
        }
        mcc = (MCC_UNKNOWN == mcc) ? INT_MAX : mcc;

        // Read <MNC>
        if ((!SkipString(pszRsp, ",", pszRsp))
                || (!ExtractInt(pszRsp, mnc, pszRsp)))
        {
            RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                    " could not extract <MNC> value\r\n");
            continue;
        }
        mnc = (MCC_UNKNOWN == mnc) ? INT_MAX : mnc;

        // Read <LAC> or <TAC>
        if ((!SkipString(pszRsp, ",", pszRsp))
                || (!ExtractQuotedHexInt(pszRsp, lac, pszRsp)))
        {
            RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                    " could not extract <lac> or <tac> value\r\n");
            continue;
        }
        lac = ((LAC_UNKNOWN == lac) || (LAC_UNKNOWN_BIS == lac)) ? INT_MAX : lac;

        // Read <CI>
        if ((!SkipString(pszRsp, ",", pszRsp))
                || (!ExtractQuotedHexUnsignedInt(pszRsp, uCi, pszRsp)))
        {
            RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                    " could not extract <ci> value\r\n");
            continue;
        }
        uCi = ((uCi > INT_MAX) || (CI_UNKNOWN == uCi)) ? INT_MAX : uCi;

        switch (type)
        {
            case 0: // GSM serving cell
            case 1: // GSM neighboring cell
            {
                int basestationId = INT_MAX;
                int rxlev = RXLEV_UNKNOWN;
                int ber = BER_UNKNOWN;
                int arfcn = INT_MAX;
                int taReliability = INT_MAX;
                int timingAdvance = INT_MAX;

                // Read <BSIC>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, basestationId, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <BSIC> value\r\n");
                    continue;
                }
                basestationId = (BSIC_UNKNOWN == basestationId) ? INT_MAX : basestationId;

                // Read <RXLEV>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, rxlev, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <RXLEV> value\r\n");
                    continue;
                }

                // Read <BER>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, ber, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <BER> value\r\n");
                    continue;
                }

                // Read <arfcn>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexInt(pszRsp, arfcn, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <arfcn> value\r\n");
                    continue;
                }
                arfcn = (ARFCN_UNKNOWN == arfcn) ? INT_MAX : arfcn;

                // Read <taReliability>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, taReliability, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <taReliability> value\r\n");
                    continue;
                }

                // Read <ta>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexInt(pszRsp, timingAdvance, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <timingAdvance> value\r\n");
                    continue;
                }

                RIL_CellInfo_v2& info = pCellData->aRilCellInfo[index];
                info.registered = (0 == type) ? SERVING_CELL : NEIGHBOURING_CELL;
                info.cellInfoType = (RIL_CellInfoType) RIL_CELL_INFO_TYPE_GSM_V2;
                info.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
                info.timeStamp = timestamp;
                info.CellInfo.gsm.signalStrengthGsm.signalStrength =
                        MapRxlevToSignalStrengh(rxlev);
                info.CellInfo.gsm.signalStrengthGsm.bitErrorRate = ber;
                info.CellInfo.gsm.signalStrengthGsm.rxLev = rxlev;
                info.CellInfo.gsm.signalStrengthGsm.timingAdvance =
                        (1 == taReliability) ? timingAdvance : INT_MAX;
                info.CellInfo.gsm.cellIdentityGsm.lac = lac;
                info.CellInfo.gsm.cellIdentityGsm.cid = (int) uCi;
                info.CellInfo.gsm.cellIdentityGsm.mnc = mnc;
                info.CellInfo.gsm.cellIdentityGsm.mcc = mcc;
                info.CellInfo.gsm.cellIdentityGsm.basestationId = basestationId;
                info.CellInfo.gsm.cellIdentityGsm.arfcn = arfcn;
                RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() - GSM  index=[%d] cid[%d] "
                        "lac[%d] mcc[%d] mnc[%d] bsic[%d] arfcn[%d] ta[%d]\r\n",
                        index, info.CellInfo.gsm.cellIdentityGsm.cid,
                        info.CellInfo.gsm.cellIdentityGsm.lac,
                        info.CellInfo.gsm.cellIdentityGsm.mcc,
                        info.CellInfo.gsm.cellIdentityGsm.mnc,
                        info.CellInfo.gsm.cellIdentityGsm.basestationId,
                        info.CellInfo.gsm.cellIdentityGsm.arfcn,
                        info.CellInfo.gsm.signalStrengthGsm.timingAdvance);
                index++;
            }
            break;

            case 2: // UMTS serving cell
            case 3: // UMTS neighboring cell
            {
                int rssi = RSSI_UNKNOWN;
                int rscp = RSCP_UNKNOWN;
                int ecNo = ECNO_UNKNOWN;
                int psc = INT_MAX;
                unsigned int uDluarfcn = INT_MAX;
                unsigned int uUluarfcn = INT_MAX;
                int pathloss = INT_MAX;

                // Read <psc>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexInt(pszRsp, psc, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <psc> value\r\n");
                    continue;
                }
                psc = (PSC_UNKNOWN == psc) ? INT_MAX : psc;

                // Read <dluarfcn>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexUnsignedInt(pszRsp, uDluarfcn, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <dluarfcn> value\r\n");
                    continue;
                }
                uDluarfcn = ((uDluarfcn > INT_MAX) || (uDluarfcn == 0xFFFFFFFF)) ?
                        INT_MAX : uDluarfcn;

                // Read <uluarfcn>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexUnsignedInt(pszRsp, uUluarfcn, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <uluarfcn> value\r\n");
                    continue;
                }
                uUluarfcn = ((uUluarfcn > INT_MAX) || (uUluarfcn == 0xFFFFFFFF)) ?
                        INT_MAX : uUluarfcn;

                // Read <pathloss>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexInt(pszRsp, pathloss, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <pathloss> value\r\n");
                    continue;
                }
                pathloss = (PATHLOSS_UNKNOWN == pathloss) ? INT_MAX : pathloss;

                // Read <rssi>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, rssi, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <rssi>\r\n");
                    continue;
                }

                // Read <RSCP>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, rscp, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <RSCP>\r\n");
                    continue;
                }

                // Read <ECNO>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, ecNo, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <ECNO>\r\n");
                    continue;
                }

                RIL_CellInfo_v2& info = pCellData->aRilCellInfo[index];
                info.registered = (2 == type) ? SERVING_CELL : NEIGHBOURING_CELL;
                info.cellInfoType = (RIL_CellInfoType) RIL_CELL_INFO_TYPE_WCDMA_V2;
                info.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
                info.timeStamp = timestamp;
                info.CellInfo.wcdma.signalStrengthWcdma.signalStrength = MapRscpToRssi(rscp);
                info.CellInfo.wcdma.signalStrengthWcdma.bitErrorRate = BER_UNKNOWN;
                info.CellInfo.wcdma.signalStrengthWcdma.rscp = rscp;
                info.CellInfo.wcdma.signalStrengthWcdma.ecNo = ecNo;
                info.CellInfo.wcdma.cellIdentityWcdma.lac = lac;
                info.CellInfo.wcdma.cellIdentityWcdma.cid = (int) uCi;
                info.CellInfo.wcdma.cellIdentityWcdma.psc = psc;
                info.CellInfo.wcdma.cellIdentityWcdma.mnc = mnc;
                info.CellInfo.wcdma.cellIdentityWcdma.mcc = mcc;
                info.CellInfo.wcdma.cellIdentityWcdma.dluarfcn = (int) uDluarfcn;
                info.CellInfo.wcdma.cellIdentityWcdma.uluarfcn = (int) uUluarfcn;
                info.CellInfo.wcdma.cellIdentityWcdma.pathloss = pathloss;

                RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() - UMTS index=[%d] cid[%d] "
                        "lac[%d] mcc[%d] mnc[%d] scrCode[%d] "
                        "dluarfcn[%d] uluarfcn[%d] pathloss[%d]\r\n",
                        index, info.CellInfo.wcdma.cellIdentityWcdma.cid,
                        info.CellInfo.wcdma.cellIdentityWcdma.lac,
                        info.CellInfo.wcdma.cellIdentityWcdma.mcc,
                        info.CellInfo.wcdma.cellIdentityWcdma.mnc,
                        info.CellInfo.wcdma.cellIdentityWcdma.psc,
                        info.CellInfo.wcdma.cellIdentityWcdma.dluarfcn,
                        info.CellInfo.wcdma.cellIdentityWcdma.uluarfcn,
                        info.CellInfo.wcdma.cellIdentityWcdma.pathloss);
                index++;
            }
            break;

            case 4: // LTE serving cell
            case 5: // LTE neighboring cell
            {
                int pci = PCI_UNKNOWN;
                int rsrp = RSRP_UNKNOWN;
                int rsrq = RSRQ_UNKNOWN;
                int rssnr = RSSNR_UNKNOWN;
                unsigned int uDlearfcn = INT_MAX;
                unsigned int uUlearfcn = INT_MAX;
                unsigned int uPathloss = INT_MAX;
                int timingAdvance = INT_MAX;
                int cqi = INT_MAX;

                // Read <pci>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexInt(pszRsp, pci, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <pci> value\r\n");
                    continue;
                }
                pci == (PCI_UNKNOWN == pci) ? INT_MAX : pci;

                // Read <dlearfcn>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexUnsignedInt(pszRsp, uDlearfcn, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <dlearfcn> value\r\n");
                    continue;
                }
                uDlearfcn = ((uDlearfcn > INT_MAX) || (uDlearfcn == 0xFFFFFFFF)) ?
                        INT_MAX : uDlearfcn;

                // Read <ulearfcn>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexUnsignedInt(pszRsp, uUlearfcn, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <ulearfcn> value\r\n");
                    continue;
                }
                uUlearfcn = ((uUlearfcn > INT_MAX) || (uUlearfcn == 0xFFFFFFFF)) ?
                        INT_MAX : uUlearfcn;

                // Read <pathloss>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexUnsignedInt(pszRsp, uPathloss, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <pathloss> value\r\n");
                    continue;
                }
                uPathloss = ((uPathloss > INT_MAX) || (PATHLOSS_LTE_UNKNOWN == uPathloss)) ?
                        INT_MAX : uPathloss;

                // Read <RSRP>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, rsrp, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <RSRP>\r\n");
                    continue;
                }

                // Read <RSRQ>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, rsrq, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <RSRQ>\r\n");
                    continue;
                }

                // Read <RSSNR>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractInt(pszRsp, rssnr, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <RSSNR>\r\n");
                    continue;
                }

                // Read <ta>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexInt(pszRsp, timingAdvance, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <timingAdvance> value\r\n");
                    continue;
                }
                timingAdvance = ((TA_INVALID == timingAdvance)
                        || (TA_LTE_INVALID == timingAdvance)) ?
                        INT_MAX : timingAdvance;

                // Read <cqi>
                if ((!SkipString(pszRsp, ",", pszRsp))
                        || (!ExtractQuotedHexInt(pszRsp, cqi, pszRsp)))
                {
                    RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                            " could not extract <cqi> value\r\n");
                    continue;
                }
                cqi = (CQI_UNKNOWN == cqi) ? INT_MAX : cqi;

                RIL_CellInfo_v2& info = pCellData->aRilCellInfo[index];
                info.registered = (4 == type) ? SERVING_CELL : NEIGHBOURING_CELL;
                info.cellInfoType = (RIL_CellInfoType) RIL_CELL_INFO_TYPE_LTE_V2;
                info.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
                info.timeStamp = timestamp;
                info.CellInfo.lte.signalStrengthLte.signalStrength = RSSI_UNKNOWN;
                info.CellInfo.lte.signalStrengthLte.rsrp = MapToAndroidRsrp(rsrp);
                info.CellInfo.lte.signalStrengthLte.rsrq = MapToAndroidRsrq(rsrq);
                info.CellInfo.lte.signalStrengthLte.rssnr = MapToAndroidRssnr(rssnr);
                info.CellInfo.lte.signalStrengthLte.cqi = cqi;
                info.CellInfo.lte.signalStrengthLte.timingAdvance = timingAdvance;
                info.CellInfo.lte.cellIdentityLte.tac = lac;
                info.CellInfo.lte.cellIdentityLte.ci = (int) uCi;
                info.CellInfo.lte.cellIdentityLte.pci = pci;
                info.CellInfo.lte.cellIdentityLte.mnc = mnc;
                info.CellInfo.lte.cellIdentityLte.mcc = mcc;
                info.CellInfo.lte.cellIdentityLte.dlearfcn = (int) uDlearfcn;
                info.CellInfo.lte.cellIdentityLte.ulearfcn = (int) uUlearfcn;
                info.CellInfo.lte.cellIdentityLte.pathloss = (int) uPathloss;
                RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() - LTE  index=[%d] cid[%d] "
                        "tac[%d] mcc[%d] mnc[%d] rsrp[%d] rsrq[%d] "
                        "ta[%d] rssnr[%d] Phyci[%d] dlearfcn[%d] ulearfcn[%d] pathloss[%d]\r\n",
                        index, info.CellInfo.lte.cellIdentityLte.ci,
                        info.CellInfo.lte.cellIdentityLte.tac,
                        info.CellInfo.lte.cellIdentityLte.mcc,
                        info.CellInfo.lte.cellIdentityLte.mnc,
                        info.CellInfo.lte.signalStrengthLte.rsrp,
                        info.CellInfo.lte.signalStrengthLte.rsrq,
                        info.CellInfo.lte.signalStrengthLte.timingAdvance,
                        info.CellInfo.lte.signalStrengthLte.rssnr,
                        info.CellInfo.lte.cellIdentityLte.pci,
                        info.CellInfo.lte.cellIdentityLte.dlearfcn,
                        info.CellInfo.lte.cellIdentityLte.ulearfcn,
                        info.CellInfo.lte.cellIdentityLte.pathloss);
                index++;
            }
            break;

            default:
            {
                RIL_LOG_INFO("CTE_XMM7260::ParseXMCI() -"
                        " Invalid type=[%d]\r\n", type);
                continue;
            }
            break;
        }
    }

    nCellInfos = index;
    res = RRIL_RESULT_OK;
Error:
    while (FindAndSkipString(pszRsp, "+XMCI: ", pszRsp));

    // Skip "<postfix>"
    if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::ParseXMCI - Could not skip response postfix.\r\n");
    }

    if (RRIL_RESULT_OK != res)
    {
        nCellInfos = 0;
        free(pCellData);
        pCellData = NULL;
    }

    return pCellData;
}

void CTE_XMM7260::CopyCardStatus(RIL_CardStatus_v6& cardStatus)
{
    cardStatus = m_CardStatusCache;

    // Do not report APPSTATE as being ready as long as SIM reset is possible in USAT state machine
    if (m_usatInitStateMachine.IsSimResetPossible() && GetSimAppState() == RIL_APPSTATE_READY)
    {
        for (int i = 0; i < m_CardStatusCache.num_applications; i++)
        {
            cardStatus.applications[i].app_state = RIL_APPSTATE_DETECTED;
        }
    }

    for (int i = 0; i < cardStatus.num_applications; i++)
    {
        GetSimAppIdAndLabel(cardStatus.applications[i].app_type,
                &cardStatus.applications[i].aid_ptr,
                &cardStatus.applications[i].app_label_ptr);
    }
}

const char* CTE_XMM7260::GetSignalStrengthReportingStringAlloc()
{
    RIL_LOG_VERBOSE("CTE_XMM7260::GetSignalStrengthReportingStringAlloc() - Enter\r\n");

    char sz2GParameters[MAX_BUFFER_SIZE] = {'\0'};
    char sz3GParameters[MAX_BUFFER_SIZE] = {'\0'};
    char szLteParameters[MAX_BUFFER_SIZE] = {'\0'};
    char szSignalStrengthReporting[MAX_BUFFER_SIZE] = {'\0'};
    const char* psz2GPrefix = "";
    const char* psz2GParams = "";
    const char* psz3GPrefix = "";
    const char* psz3GParams = "";
    const char* pszLtePrefix = "";
    const char* pszLteParams = "";
    CRepository repository;

    if (repository.Read(g_szGroupModem, g_sz2GParameters, sz2GParameters, MAX_BUFFER_SIZE))
    {
        psz2GPrefix = "|+XCESQRC=0,1,";
        psz2GParams = sz2GParameters;
    }

    if (repository.Read(g_szGroupModem, g_sz3GParameters, sz3GParameters, MAX_BUFFER_SIZE))
    {
        psz3GPrefix = "|+XCESQRC=1,1,";
        psz3GParams = sz3GParameters;
    }

    if (repository.Read(g_szGroupModem, g_szLteParameters, szLteParameters, MAX_BUFFER_SIZE))
    {
        pszLtePrefix = "|+XCESQRC=2,1,";
        pszLteParams = szLteParameters;
    }

    PrintStringNullTerminate(szSignalStrengthReporting, MAX_BUFFER_SIZE, "+XCESQ=1%s%s%s%s%s%s",
            psz2GPrefix, psz2GParams, psz3GPrefix, psz3GParams, pszLtePrefix, pszLteParams);

    RIL_LOG_VERBOSE("CTE_XMM7260::GetSignalStrengthReportingStringAlloc() - Exit\r\n");

    return strdup(szSignalStrengthReporting);
}

/*
 * Activates or deactivates the registration state and band information reporting
 * using intent RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_REPORT
 */
RIL_RESULT_CODE CTE_XMM7260::CreateSetRegStatusAndBandReport(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CreateSetRegStatusAndBandReport() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int reportOn = -1;

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetRegStatusAndBandReport() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    // Request format received : <commandId>, <data>
    // The value received for data should be '0' or '1'.
    if (!ExtractInt(ppszRequest[1], reportOn, ppszRequest[1])
            || (reportOn < 0 && reportOn > 1))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetRegStatusAndBandReport() :"
                " invalid data received\r\n");
        goto Error;
    }

    m_bCoexRegStatusAndBandIndActivated = (reportOn == 1);

    if (m_bCoexRegStatusAndBandIndActivated)
    {
        if (!CopyStringNullTerminate(reqData.szCmd1, "AT+XREG=3;+XREG?\r",
                sizeof(reqData.szCmd1)))
        {
            RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetRegStatusAndBandReport() -"
                   " Cannot construct szCmd1.\r\n");
            goto Error;
        }
    }
    else
    {
        res = RRIL_RESULT_OK_IMMEDIATE;
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::CreateSetRegStatusAndBandReport() - Exit\r\n");
    return res;
}

/*
 * Activates or deactivates the coexistence intents reporting
 * - Enable Coex Reporting "AT+XNRTCWS=1"
 * - Disable Coex Reporting "AT+XNRTCWS=0"
 */
RIL_RESULT_CODE CTE_XMM7260::CreateSetCoexReport(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CreateSetCoexReport() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int reportOn = -1;

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexReport() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    // Request format received : <commandId>, <data>
    // The value received for data should be '0' or '1'.
    if (!ExtractInt(ppszRequest[1], reportOn, ppszRequest[1])
            || (reportOn < 0 && reportOn > 1))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexReport() :"
                " invalid data received\r\n");
        goto Error;
    }

    m_bCoexReportActivated = (reportOn == 1);

    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
            "AT+XNRTCWS=%d\r", reportOn))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexReport() -"
                " Cannot construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::CreateSetCoexReport() - Exit\r\n");
    return res;
}

/*
 * Set the Wlan parameters to the modem IDC part.
 */
RIL_RESULT_CODE CTE_XMM7260::CreateSetCoexWlanParams(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CreateSetCoexWlanParams() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int wlanStatus = -1;
    int wlanBandwidth = -1;

    if (uiDataSize < (3 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexWlanParams() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    // Request format received : <commandId>, <Wifi status>, <Wlan Bandwidth>
    // The value received for Wifi status should be '0' or '1'.
    if (!ExtractInt(ppszRequest[1], wlanStatus, ppszRequest[1])
            || (wlanStatus < 0 && wlanStatus > 1))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexWlanParams() :"
                " invalid wlanStatus received\r\n");
        goto Error;
    }

    // The value received for Wlan Bandwidth should be : 0/1/2
    if (!ExtractInt(ppszRequest[2], wlanBandwidth, ppszRequest[2])
            || (wlanBandwidth < 0 && wlanBandwidth > 2))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexWlanParams() :"
                " invalid wlanBandwidth received\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
            "AT+XNRTCWS=2,%d,%d\r", wlanStatus, wlanBandwidth))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexWlanParams() -"
                " Cannot construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::CreateSetCoexWlanParams() - Exit\r\n");
    return res;
}

/*
 * Set the Bt parameters to the modem IDC part.
 */
RIL_RESULT_CODE CTE_XMM7260::CreateSetCoexBtParams(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7260::CreateSetCoexBtParams() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int btStatus = -1;

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexBtParams() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    // Request format received : <commandId>, <data>
    // The value received for data should be '0' or '1'.
    if (!ExtractInt(ppszRequest[1], btStatus, ppszRequest[1])
            || (btStatus < 0 && btStatus > 1))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexBtParams() :"
                " invalid btStatus received\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
            "AT+XNRTCWS=2,,,%d\r", btStatus))
    {
        RIL_LOG_CRITICAL("CTE_XMM7260::CreateSetCoexBtParams() -"
                " Cannot construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7260::CreateSetCoexBtParams() - Exit\r\n");
    return res;
}
