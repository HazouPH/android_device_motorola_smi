////////////////////////////////////////////////////////////////////////////
// te_xmm6260.cpp
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Overlay for the IMC 6260 modem
//
/////////////////////////////////////////////////////////////////////////////

#include <wchar.h>
#include <cutils/properties.h>

//  This is for socket-related calls.
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/gsmmux.h>

#include "types.h"
#include "nd_structs.h"
#include "util.h"
#include "extract.h"
#include "rillog.h"
#include "te.h"
#include "te_base.h"
#include "sync_ops.h"
#include "command.h"
#include "te_xmm6260.h"
#include "rildmain.h"
#include "callbacks.h"
#include "oemhookids.h"
#include "repository.h"
#include "reset.h"
#include "data_util.h"
#include "rril.h"
#include "callbacks.h"
#include "init6260.h"
#include "bertlv_util.h"
#include "hardwareconfig.h"

CTE_XMM6260::CTE_XMM6260(CTE& cte)
: CTEBase(cte),
  m_currentNetworkType(-1)
{
    m_bNeedGetInfoOnCellChange = true;
}

CTE_XMM6260::~CTE_XMM6260()
{
}

CInitializer* CTE_XMM6260::GetInitializer()
{
    RIL_LOG_VERBOSE("CTE_XMM6260::GetInitializer() - Enter\r\n");
    CInitializer* pRet = NULL;

    RIL_LOG_INFO("CTE_XMM6260::GetInitializer() - Creating CInit6260 initializer\r\n");
    m_pInitializer = new CInit6260();
    if (NULL == m_pInitializer)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetInitializer() - Failed to create a CInit6260 "
                "initializer!\r\n");
        goto Error;
    }

    pRet = m_pInitializer;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::GetInitializer() - Exit\r\n");
    return pRet;
}

char* CTE_XMM6260::GetBasicInitCommands(UINT32 uiChannelType)
{
    return (RIL_CHANNEL_URC == uiChannelType) ? strdup(GetRegistrationInitString()) : NULL;
}

char* CTE_XMM6260::GetUnlockInitCommands(UINT32 /*uiChannelType*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::GetUnlockInitCommands() - Enter / Exit\r\n");
    return NULL;
}

const char* CTE_XMM6260::GetRegistrationInitString()
{
    return "+CREG=2|+XREG=2";
}

const char* CTE_XMM6260::GetPsRegistrationReadString()
{
    return "AT+XREG=2;+XREG?;+XREG=0\r";
}

const char* CTE_XMM6260::GetScreenOnString()
{
    if (m_cte.IsSignalStrengthReportEnabled())
    {
        return "AT+CREG=2;+CGREG=0;+XREG=2;+XCSQ=1\r";
    }

    return "AT+CREG=2;+CGREG=0;+XREG=2\r";
}

const char* CTE_XMM6260::GetScreenOffString()
{
    char szScreenOffString[MAX_BUFFER_SIZE] = {'\0'};

    if (m_cte.IsLocationUpdatesEnabled())
    {
        CopyStringNullTerminate(szScreenOffString,
                m_cte.IsSignalStrengthReportEnabled()
                ? "AT+CGREG=1;+XCSQ=0"
                : "AT+CGREG=1",
                sizeof(szScreenOffString));
    }
    else
    {
        CopyStringNullTerminate(szScreenOffString,
                m_cte.IsSignalStrengthReportEnabled()
                ? "AT+CREG=1;+CGREG=1;+XCSQ=0"
                : "AT+CREG=1;+CGREG=1",
                sizeof(szScreenOffString));
    }

    ConcatenateStringNullTerminate(szScreenOffString, sizeof(szScreenOffString),
            m_bRegStatusAndBandIndActivated ? "\r" : ";+XREG=0\r");

    return strndup(szScreenOffString, strlen(szScreenOffString));
}

const char* CTE_XMM6260::GetSignalStrengthReportingStringAlloc()
{
    return strdup("+XCSQ=1");
}

LONG CTE_XMM6260::GetDataDeactivateReason(char* pszReason)
{
    return strtol(pszReason, NULL, 10);
}

const char* CTE_XMM6260::GetReadCellInfoString()
{
    return "AT+XCELLINFO?\r";
}

BOOL CTE_XMM6260::IsRequestSupported(int requestId)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::IsRequestSupported() - Enter\r\n");

    switch (requestId)
    {
        case RIL_REQUEST_GET_SIM_STATUS:
        case RIL_REQUEST_SIM_IO:
        case RIL_REQUEST_OEM_HOOK_RAW:
        case RIL_REQUEST_OEM_HOOK_STRINGS:
        case RIL_REQUEST_SET_BAND_MODE:
        case RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE:
        case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE:
        case RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE:
        case RIL_REQUEST_GET_NEIGHBORING_CELL_IDS:
        case RIL_REQUEST_SET_TTY_MODE:
        case RIL_REQUEST_QUERY_TTY_MODE:
        case RIL_REQUEST_REPORT_SMS_MEMORY_STATUS:
            return TRUE;
        case RIL_REQUEST_DIAL:
        case RIL_REQUEST_HANGUP:
        case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND:
        case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
        case RIL_REQUEST_SWITCH_HOLDING_AND_ACTIVE:
        case RIL_REQUEST_CONFERENCE:
        case RIL_REQUEST_UDUB:
        case RIL_REQUEST_LAST_CALL_FAIL_CAUSE:
        case RIL_REQUEST_DTMF:
        case RIL_REQUEST_SEND_USSD:
        case RIL_REQUEST_CANCEL_USSD:
        case RIL_REQUEST_GET_CLIR:
        case RIL_REQUEST_SET_CLIR:
        case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS:
        case RIL_REQUEST_SET_CALL_FORWARD:
        case RIL_REQUEST_QUERY_CALL_WAITING:
        case RIL_REQUEST_SET_CALL_WAITING:
        case RIL_REQUEST_ANSWER:
        case RIL_REQUEST_CHANGE_BARRING_PASSWORD:
        case RIL_REQUEST_DTMF_START:
        case RIL_REQUEST_DTMF_STOP:
        case RIL_REQUEST_SEPARATE_CONNECTION:
        case RIL_REQUEST_SET_MUTE:
        case RIL_REQUEST_GET_MUTE:
        case RIL_REQUEST_QUERY_CLIP:
        case RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION:
        case RIL_REQUEST_EXPLICIT_CALL_TRANSFER:
            return m_cte.IsVoiceCapable();

        case RIL_REQUEST_SEND_SMS:
        case RIL_REQUEST_SEND_SMS_EXPECT_MORE:
        case RIL_REQUEST_SMS_ACKNOWLEDGE:
        case RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU:
        case RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG:
        case RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG:
        case RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION:
            return (m_cte.IsSmsOverCSCapable() || m_cte.IsSmsOverPSCapable());

        case RIL_REQUEST_STK_GET_PROFILE:
        case RIL_REQUEST_STK_SET_PROFILE:
        case RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND:
        case RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE:
        case RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM:
        case RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING:
        case RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS:
            return m_cte.IsStkCapable();
        case RIL_REQUEST_SETUP_DATA_CALL:
        case RIL_REQUEST_SET_INITIAL_ATTACH_APN:
        case RIL_REQUEST_DATA_REGISTRATION_STATE:
        case RIL_REQUEST_DEACTIVATE_DATA_CALL:
            return m_cte.IsDataCapable();

        default:
            return CTEBase::IsRequestSupported(requestId);
    }
}

//
// RIL_REQUEST_GET_SIM_STATUS
//
RIL_RESULT_CODE CTE_XMM6260::CoreGetSimStatus(REQUEST_DATA& /*rReqData*/,
                                                          void* /*pData*/,
                                                          UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreGetSimStatus() - Enter / Exit\r\n");
    return RRIL_RESULT_OK;
}

RIL_RESULT_CODE CTE_XMM6260::ParseGPRSRegistrationState(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseGPRSRegistrationState() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    S_ND_GPRS_REG_STATUS psRegStatus;
    P_ND_GPRS_REG_STATUS pGPRSRegStatus = NULL;

    pGPRSRegStatus = (P_ND_GPRS_REG_STATUS)malloc(sizeof(S_ND_GPRS_REG_STATUS));
    if (NULL == pGPRSRegStatus)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseGPRSRegistrationState() -"
                " Could not allocate memory for a S_ND_GPRS_REG_STATUS struct.\r\n");
        goto Error;
    }
    memset(pGPRSRegStatus, 0, sizeof(S_ND_GPRS_REG_STATUS));

    if (!m_cte.ParseXREG(pszRsp, FALSE, psRegStatus))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseGPRSRegistrationState() - "
                "ERROR in parsing response.\r\n");
        goto Error;
    }

    if (strcmp(psRegStatus.szStat, UNREGISTERED_SEARCHING))
    {
        m_cte.StoreRegistrationInfo(&psRegStatus, E_REGISTRATION_TYPE_XREG);
    }

    m_cte.CopyCachedRegistrationInfo(pGPRSRegStatus, TRUE);

    rRspData.pData  = (void*)pGPRSRegStatus;
    rRspData.uiDataSize = sizeof(S_ND_GPRS_REG_STATUS_POINTERS);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pGPRSRegStatus);
        pGPRSRegStatus = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseGPRSRegistrationState() - Exit\r\n");
    return res;
}

// RIL_REQUEST_SETUP_DATA_CALL
RIL_RESULT_CODE CTE_XMM6260::CoreSetupDataCall(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize,
                                                           UINT32& uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSetupDataCall() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int nPapChap = 0; // no auth
    PdpData stPdpData;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    CChannel_Data* pChannelData = NULL;
    UINT32 uiDnsMode = 0;

    pChannelData = CChannel_Data::GetFreeChnl(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetupDataCall() - "
                "****** No free data channels available ******\r\n");
        goto Error;
    }

    pDataCallContextData =
            (S_SETUP_DATA_CALL_CONTEXT_DATA*)malloc(sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA));
    if (NULL == pDataCallContextData)
    {
        goto Error;
    }

    memset(&stPdpData, 0, sizeof(PdpData));
    pDataCallContextData->uiCID = uiCID;

    // extract data
    stPdpData.szRadioTechnology = ((char**)pData)[0];  // not used
    stPdpData.szRILDataProfile  = ((char**)pData)[1];
    stPdpData.szApn             = ((char**)pData)[2];
    stPdpData.szUserName        = ((char**)pData)[3];
    stPdpData.szPassword        = ((char**)pData)[4];
    stPdpData.szPAPCHAP         = ((char**)pData)[5];

    RIL_LOG_INFO("CTE_XMM6260::CoreSetupDataCall() - stPdpData.szRadioTechnology=[%s]\r\n",
            stPdpData.szRadioTechnology);
    RIL_LOG_INFO("CTE_XMM6260::CoreSetupDataCall() - stPdpData.szRILDataProfile=[%s]\r\n",
            stPdpData.szRILDataProfile);
    RIL_LOG_INFO("CTE_XMM6260::CoreSetupDataCall() - stPdpData.szApn=[%s]\r\n", stPdpData.szApn);
    RIL_LOG_INFO("CTE_XMM6260::CoreSetupDataCall() - stPdpData.szUserName=[%s]\r\n",
            stPdpData.szUserName);
    RIL_LOG_INFO("CTE_XMM6260::CoreSetupDataCall() - stPdpData.szPassword=[%s]\r\n",
            stPdpData.szPassword);
    RIL_LOG_INFO("CTE_XMM6260::CoreSetupDataCall() - stPdpData.szPAPCHAP=[%s]\r\n",
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

            RIL_LOG_INFO("CTE_XMM6260::CoreSetupDataCall() - New PAP/CHAP=[%d]\r\n", nPapChap);
        }
    }

    if (RIL_VERSION >= 4 && (uiDataSize >= (7 * sizeof(char*))))
    {
        stPdpData.szPDPType         = ((char**)pData)[6];  // new in Android 2.3.4.
        RIL_LOG_INFO("CTE_XMM6260::CoreSetupDataCall() - stPdpData.szPDPType=[%s]\r\n",
                stPdpData.szPDPType);
    }

    //
    //  IP type is passed in dynamically.
    if (NULL == stPdpData.szPDPType)
    {
        //  hard-code szIPV4V6 (this is the default)
        CopyStringNullTerminate(stPdpData.szPDPType, PDPTYPE_IPV4V6, sizeof(stPdpData.szPDPType));
    }

    //  dynamic PDP type, need to set XDNS parameter depending on szPDPType.
    //  If not recognized, just use IPV4V6 as default.
    uiDnsMode = GetXDNSMode(stPdpData.szPDPType);
    switch (uiDnsMode)
    {
        case 1:
        case 2:
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+CGDCONT=%d,\"%s\",\"%s\",,0,0;+XGAUTH=%d,%u,\"%s\",\"%s\";+XDNS=%d,%u\r",
                    uiCID, stPdpData.szPDPType, stPdpData.szApn, uiCID, nPapChap,
                    stPdpData.szUserName, stPdpData.szPassword, uiCID, uiDnsMode))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetupDataCall() -"
                        " cannot create CGDCONT command, stPdpData.szPDPType\r\n");
                goto Error;
            }
        break;

        case 3:
            // XDNS=3 is not supported by the modem so two commands +XDNS=1 and +XDNS=2
            // should be sent.
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+CGDCONT=%d,\"%s\",\"%s\",,0,0;+XGAUTH=%u,%d,\"%s\","
                    "\"%s\";+XDNS=%d,1;+XDNS=%d,2\r", uiCID, stPdpData.szPDPType, stPdpData.szApn,
                    uiCID, nPapChap, stPdpData.szUserName, stPdpData.szPassword, uiCID, uiCID))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetupDataCall() -"
                        " cannot create CGDCONT command, stPdpData.szPDPType\r\n");
                goto Error;
            }
        break;

        default:
            RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetupDataCall() -"
                    " cannot create CGDCONT command, Wrong PDP type\r\n");
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
        pChannelData->SetApn(stPdpData.szApn);

        rReqData.pContextData = (void*)pDataCallContextData;
        rReqData.cbContextData = sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA);
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSetupDataCall() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseSetupDataCall(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSetupDataCall() - Enter\r\n");

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSetupDataCall() - Exit\r\n");
    return RRIL_RESULT_OK;
}

BOOL CTE_XMM6260::PdpContextActivate(REQUEST_DATA& rReqData, void* pData,
                                                            UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::PdpContextActivate() - Enter\r\n");

    BOOL bRet = FALSE;
    UINT32 uiCID = 0;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;

    if (NULL == pData ||
                    sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::PdpContextActivate() - Invalid input data\r\n");
        goto Error;
    }

    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)pData;
    uiCID = pDataCallContextData->uiCID;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                                                    "AT+CGACT=1,%d\r", uiCID))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::PdpContextActivate() -  cannot create CGDATA command\r\n");
        goto Error;
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::PdpContextActivate() - Cannot create CEER command\r\n");
    }

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::PdpContextActivate() - Exit\r\n");
    return bRet;
}

RIL_RESULT_CODE CTE_XMM6260::ParsePdpContextActivate(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParsePdpContextActivate() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        RIL_LOG_INFO("CTE_XMM6260::ParsePdpContextActivate() - uiCause: %u\r\n", uiCause);

        S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
        UINT32 uiCID = 0;
        CChannel_Data* pChannelData = NULL;
        int failCause = PDP_FAIL_ERROR_UNSPECIFIED;

        if (NULL == rRspData.pContextData ||
                sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != rRspData.cbContextData)
        {
            RIL_LOG_INFO("CTE_XMM6260::ParsePdpContextActivate() - Invalid context data\r\n");
            goto Error;
        }

        pDataCallContextData =
                        (S_SETUP_DATA_CALL_CONTEXT_DATA*)rRspData.pContextData;
        uiCID = pDataCallContextData->uiCID;

        pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
        if (NULL == pChannelData)
        {
            RIL_LOG_INFO("CTE_XMM6260::ParsePdpContextActivate() -"
                    " No Data Channel for CID %u.\r\n", uiCID);
            goto Error;
        }

        failCause = MapErrorCodeToRilDataFailCause(uiCause);
        pChannelData->SetDataFailCause(failCause);
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParsePdpContextActivate() - Exit\r\n");
    return res;
}

BOOL CTE_XMM6260::QueryIpAndDns(REQUEST_DATA& rReqData, UINT32 uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::QueryIpAndDns() - Enter\r\n");
    BOOL bRet = FALSE;

    if (uiCID != 0)
    {
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                                            "AT+CGPADDR=%u;+XDNS?\r", uiCID))
        {
            bRet = TRUE;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::QueryIpAndDns() - Exit\r\n");
    return bRet;
}

RIL_RESULT_CODE CTE_XMM6260::ParseQueryIpAndDns(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryIpAndDns() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    const char* pszRsp = rRspData.szResponse;
    const char* szStrExtract = NULL;

    // Parse prefix
    if (FindAndSkipString(pszRsp, "+CGPADDR: ", pszRsp))
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseQueryIpAndDns() - parse \"+CGPADDR\" \r\n");
        res = ParseIpAddress(rRspData);
    }

    if (FindAndSkipString(pszRsp, "+XDNS: ", pszRsp))
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseQueryIpAndDns() - parse \"+XDNS\" \r\n");
        res = ParseDns(rRspData);
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryIpAndDns() - Exit\r\n");
    return res;
}

//
// Response to AT+CGPADDR=<CID>
//
RIL_RESULT_CODE CTE_XMM6260::ParseIpAddress(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseIpAddress() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;
    UINT32 uiCID;

    // Parse prefix
    if (!FindAndSkipString(szRsp, "+CGPADDR: ", szRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() -"
                " Unable to parse \"+CGPADDR\" prefix.!\r\n");
        goto Error;
    }

    // Parse <cid>
    if (!ExtractUInt32(szRsp, uiCID, szRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() - Unable to parse <cid>!\r\n");
        goto Error;
    }

    if (uiCID > 0)
    {
        //  The response could come back as:
        //  +CGPADDR: <cid>,<PDP_Addr1>,<PDP_Addr2>
        //  PDP_Addr1 could be in IPv4, or IPv6.  PDP_Addr2 is present only for IPv4v6
        //  in which case PDP_Addr1 is IPv4 and PDP_Addr2 is IPv6.
        //  a1.a2.a3.a4 (for IPv4)
        //  a1.a2.a3.a4.a5.a6.a7.a8.a9.a10.a11.a12.a13.a14.a15.a16 (for IPv6)

        //  The IPv6 format above is not IPv6 standard address string notation, as
        //  required by Android, so we need to convert it.

        //  Extract original string into szPdpAddr.
        //  Then converted address is in szIpAddr1.
        char szPdpAddr[MAX_IPADDR_SIZE] = {'\0'};
        char szIpAddr1[MAX_IPADDR_SIZE] = {'\0'};
        char szIpAddr2[MAX_IPADDR_SIZE] = {'\0'};
        int state;

        CChannel_Data* pChannelData =
                                    CChannel_Data::GetChnlFromContextID(uiCID);
        if (NULL == pChannelData)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() - No Data Channel for CID %u.\r\n",
                                                                                        uiCID);
            goto Error;
        }

        state = pChannelData->GetDataState();
        if (E_DATA_STATE_ACTIVE != state)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() - Wrong data state: %d\r\n",
                                                                                state);
            goto Error;
        }

        //  Extract ,<Pdp_Addr1>
        if (!SkipString(szRsp, ",", szRsp) ||
            !ExtractQuotedString(szRsp, szPdpAddr, MAX_IPADDR_SIZE, szRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() - Unable to parse <PDP_addr1>!\r\n");
            goto Error;
        }

        //  The AT+CGPADDR command doesn't return IPV4V6 format
        if (!ConvertIPAddressToAndroidReadable(szPdpAddr,
                                                szIpAddr1,
                                                MAX_IPADDR_SIZE,
                                                szIpAddr2,
                                                MAX_IPADDR_SIZE))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() -"
                    " ConvertIPAddressToAndroidReadable failed!\r\n");
            goto Error;
        }

        //  Extract ,<PDP_Addr2>
        //  Converted address is in szIpAddr2.
        if (SkipString(szRsp, ",", szRsp))
        {
            if (!ExtractQuotedString(szRsp, szPdpAddr, MAX_IPADDR_SIZE, szRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() -"
                        " Unable to parse <PDP_addr2>!\r\n");
                goto Error;
            }

            //  The AT+CGPADDR command doesn't return IPV4V6 format.
            if (!ConvertIPAddressToAndroidReadable(szPdpAddr,
                                                    szIpAddr1,
                                                    MAX_IPADDR_SIZE,
                                                    szIpAddr2,
                                                    MAX_IPADDR_SIZE))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() -"
                        " ConvertIPAddressToAndroidReadable failed! m_szIpAddr2\r\n");
                goto Error;
            }

            //  Extract ,<PDP_Addr2>
            //  Converted address is in pChannelData->m_szIpAddr2.
            if (SkipString(szRsp, ",", szRsp))
            {
                if (!ExtractQuotedString(szRsp, szPdpAddr, MAX_IPADDR_SIZE, szRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() -"
                            " Unable to parse <PDP_addr2>!\r\n");
                    goto Error;
                }

                //  The AT+CGPADDR command doesn't return IPV4V6 format
                //  Use a dummy IPV4 address as only an IPV6 address is expected here
                char szDummyIpAddr[MAX_IPADDR_SIZE];
                szDummyIpAddr[0] = '\0';

                if (!ConvertIPAddressToAndroidReadable(szPdpAddr,
                        szDummyIpAddr, MAX_IPADDR_SIZE, szIpAddr2, MAX_IPADDR_SIZE))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() -"
                            " ConvertIPAddressToAndroidReadable failed! m_szIpAddr2\r\n");
                    goto Error;
                }

            RIL_LOG_INFO("CTE_XMM6260::ParseIpAddress() - IPV4 address: %s\r\n", szIpAddr1);
            RIL_LOG_INFO("CTE_XMM6260::ParseIpAddress() - IPV6 address: %s\r\n", szIpAddr2);
            }
        }

        // First clear IP addresses
        pChannelData->DeleteAddressesString(pChannelData->ADDR_IP);
        // Now add IP addresses
        pChannelData->AddAddressString(pChannelData->ADDR_IP, szIpAddr1);
        pChannelData->AddAddressString(pChannelData->ADDR_IP, szIpAddr2);

        res = RRIL_RESULT_OK;
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseIpAddress() - uiCID=[%u] not valid!\r\n", uiCID);
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseIpAddress() - Exit\r\n");
    return res;
}

//
// Response to AT+XDNS?
//
RIL_RESULT_CODE CTE_XMM6260::ParseDns(RESPONSE_DATA & rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseDns() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;

    // Parse "+XDNS: "
    while (FindAndSkipString(szRsp, "+XDNS: ", szRsp))
    {
        UINT32 uiCID = 0;
        char szDNS[MAX_IPADDR_SIZE] = {'\0'};
        char szIpDNS1[MAX_IPADDR_SIZE] = {'\0'};
        char szIpDNS2[MAX_IPADDR_SIZE] = {'\0'};
        char szIpV6DNS1[MAX_IPADDR_SIZE] = {'\0'};
        char szIpV6DNS2[MAX_IPADDR_SIZE] = {'\0'};
        CChannel_Data* pChannelData = NULL;
        int state;

        // Parse <cid>
        if (!ExtractUInt32(szRsp, uiCID, szRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseDns() - Unable to parse <cid>!\r\n");
            continue;
        }

        pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
        if (NULL == pChannelData)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseDns() - No Data Channel for CID %u.\r\n",
                    uiCID);
            continue;
        }

        state = pChannelData->GetDataState();
        if (E_DATA_STATE_ACTIVE != state)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseDns() - Wrong data state: %d\r\n",
                    state);
            continue;
        }

        //  The response could come back as:
        //  +XDNS: <cid>,<Primary_DNS>,<Secondary_DNS>
        //  Also, Primary_DNS and Secondary_DNS could be in ipv4, ipv6, or ipv4v6 format.
        //  String is in dot-separated numeric (0-255) of the form:
        //  a1.a2.a3.a4 (for IPv4)
        //  a1.a2.a3.a4.a5.a6.a7.a8.a9.a10.a11.a12.a13.a14.a15.a16 (for IPv6)
        //  a1.a2.a3.a4.a5.a6.a7.a8.a9.a10.a11.a12.a13.a14.a15.a16.a17.a18.a19.a20 (for IPv4v6)

        //  The IPV6, and IPV4V6 format above is incompatible with Android, so we need to convert
        //  to an Android-readable IPV6, IPV4 address format.

        //  Extract original string into
        //  Then converted address is in szIpDNS1, szIpV6DNS1 (if IPv4v6).

        // Parse <primary DNS>
        // Converted address is in szIpDNS1, szIpV6DNS1 (if necessary)
        if (SkipString(szRsp, ",", szRsp))
        {
            if (!ExtractQuotedString(szRsp, szDNS, MAX_IPADDR_SIZE, szRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseDns() - Unable to extact szDNS 1!\r\n");
                continue;
            }

            //  Now convert to Android-readable format (and split IPv4v6 parts (if applicable)
            if (!ConvertIPAddressToAndroidReadable(szDNS,
                                                    szIpDNS1,
                                                    MAX_IPADDR_SIZE,
                                                    szIpV6DNS1,
                                                    MAX_IPADDR_SIZE))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseDns() -"
                        " ConvertIPAddressToAndroidReadable failed! m_szDNS1\r\n");
                continue;
            }

            RIL_LOG_INFO("CTE_XMM6260::ParseDns() - szIpDNS1: %s\r\n", szIpDNS1);

            if (strlen(szIpV6DNS1) > 0)
            {
                RIL_LOG_INFO("CTE_XMM6260::ParseDns() - szIpV6DNS1: %s\r\n", szIpV6DNS1);
            }
            else
            {
                RIL_LOG_INFO("CTE_XMM6260::ParseDns() - szIpV6DNS1: <NONE>\r\n");
            }
        }

        // Parse <secondary DNS>
        // Converted address is in szIpDNS2, szIpV6DNS2 (if necessary)
        if (SkipString(szRsp, ",", szRsp))
        {
            if (!ExtractQuotedString(szRsp, szDNS, MAX_IPADDR_SIZE, szRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseDns() - Unable to extact szDNS 2!\r\n");
                continue;
            }

            //  Now convert to Android-readable format (and split IPv4v6 parts (if applicable)
            if (!ConvertIPAddressToAndroidReadable(szDNS,
                                                    szIpDNS2,
                                                    MAX_IPADDR_SIZE,
                                                    szIpV6DNS2,
                                                    MAX_IPADDR_SIZE))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseDns() -"
                        " ConvertIPAddressToAndroidReadable failed! szIpDNS2\r\n");
                continue;
            }

            RIL_LOG_INFO("CTE_XMM6260::ParseDns() - szIpDNS2: %s\r\n", szIpDNS2);

            if (strlen(szIpV6DNS2) > 0)
            {
                RIL_LOG_INFO("CTE_XMM6260::ParseDns() - szIpV6DNS2: %s\r\n", szIpV6DNS2);
            }
            else
            {
                RIL_LOG_INFO("CTE_XMM6260::ParseDns() - szIpV6DNS2: <NONE>\r\n");
            }
        }

        // First clear DNS addresses
        pChannelData->DeleteAddressesString(pChannelData->ADDR_DNS);
        // Now add DNS addresses
        pChannelData->AddAddressString(pChannelData->ADDR_DNS, szIpDNS1);
        pChannelData->AddAddressString(pChannelData->ADDR_DNS, szIpDNS2);
        pChannelData->AddAddressString(pChannelData->ADDR_DNS, szIpV6DNS1);
        pChannelData->AddAddressString(pChannelData->ADDR_DNS, szIpV6DNS2);

        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseDns() - Exit\r\n");
    return res;
}

BOOL CTE_XMM6260::EnterDataState(REQUEST_DATA& rReqData, UINT32 uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::EnterDataState() - Enter\r\n");

    BOOL bRet = FALSE;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                                        "AT+CGDATA=\"M-RAW_IP\",%d\r", uiCID))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::EnterDataState() -  cannot create CGDATA command\r\n");
        goto Error;
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::EnterDataState() - Cannot create CEER command\r\n");
    }

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::EnterDataState() - Exit\r\n");
    return bRet;
}

RIL_RESULT_CODE CTE_XMM6260::ParseEnterDataState(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseEnterDataState() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    CChannel_Data* pChannelData = NULL;
    UINT32 uiCause;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    UINT32 uiCID = 0;

    if (NULL == rRspData.pContextData ||
            sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != rRspData.cbContextData)
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseEnterDataState() - Invalid context data\r\n");
        goto Error;
    }

    if (ParseCEER(rRspData, uiCause))
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseEnterDataState() - uiCause: %u\r\n",
                                                                    uiCause);
        int failCause = PDP_FAIL_ERROR_UNSPECIFIED;

        pDataCallContextData =
                        (S_SETUP_DATA_CALL_CONTEXT_DATA*)rRspData.pContextData;
        uiCID = pDataCallContextData->uiCID;

        pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
        if (NULL == pChannelData)
        {
            RIL_LOG_INFO("CTE_XMM6260::ParseEnterDataState() - No Data Channel for CID %u.\r\n",
                    uiCID);
            goto Error;
        }

        failCause = MapErrorCodeToRilDataFailCause(uiCause);
        pChannelData->SetDataFailCause(failCause);
        goto Error;
    }

    if (!FindAndSkipString(pszRsp, "CONNECT", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseEnterDataState() -"
                "  Did not get \"CONNECT\" response.\r\n");
        goto Error;
    }

    pDataCallContextData =
                    (S_SETUP_DATA_CALL_CONTEXT_DATA*)rRspData.pContextData;
    uiCID = pDataCallContextData->uiCID;

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseEnterDataState() - No Data Channel for CID %u.\r\n",
                uiCID);
        goto Error;
    }
    else
    {
        // Block the read thread and then flush the tty and the channel
        // From now, any failure will lead to DataConfigDown
        pChannelData->BlockAndFlushChannel(BLOCK_CHANNEL_BLOCK_ALL, FLUSH_CHANNEL_NO_FLUSH);
        pChannelData->FlushAndUnblockChannel(UNBLOCK_CHANNEL_UNBLOCK_TTY, FLUSH_CHANNEL_FLUSH_ALL);
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseEnterDataState() - Exit\r\n");
    return res;
}


//  Extract USIM GET_RESPONSE info.
struct sUSIMGetResponse
{
    UINT32 dwRecordType;
    //UINT32 dwItemCount;
    UINT32 dwTotalSize;
    UINT32 dwRecordSize;
};

const int RIL_SIMRECORDTYPE_UNKNOWN = 0x00000000;
const int RIL_SIMRECORDTYPE_TRANSPARENT = 0x00000001;
const int RIL_SIMRECORDTYPE_CYCLIC = 0x00000002;
const int RIL_SIMRECORDTYPE_LINEAR = 0x00000003;
const int RIL_SIMRECORDTYPE_MASTER = 0x00000004;
const int RIL_SIMRECORDTYPE_DEDICATED = 0x00000005;


//
//  This function converts a USIM GET_RESPONSE response into a SIM GET_RESPONSE that Android
//  can understand.  We extract the record type, total size, and record size.
//
//  sResponse = [in] binary buffer of CRSM response
//  cbResponse = [in] length of binary buffer
//  sUSIM = [in,out] where the extracted data is stored
BOOL ParseUSIMRecordStatus(UINT8* sResponse, UINT32 cbResponse, struct sUSIMGetResponse* sUSIM)
{
    RIL_LOG_VERBOSE("ParseUSIMRecordStatus - ENTER\r\n");

    BOOL bRet = FALSE;

    const UINT8 FCP_TAG = 0x62;
    const UINT8 FILE_SIZE_TAG = 0x80;
    const UINT8 FILE_DESCRIPTOR_TAG = 0x82;
    const UINT8 FILE_ID_TAG = 0x83;
    const UINT32 MASTER_FILE_ID = 0x3F00;
    UINT32 dwRecordType = 0;
    UINT32 dwItemCount = 0;
    UINT32 dwRecordSize = 0;
    UINT32 dwTotalSize = 0;
    BerTlv tlvFileDescriptor;
    BerTlv tlvFcp;

    const UINT8* pbFcpData = NULL;
    UINT32 cbFcpDataSize = 0;
    UINT32 cbDataUsed = 0;
    const UINT8* pbFileDescData = NULL;
    UINT32 cbFileDescDataSize = 0;

    BOOL fIsDf = FALSE;
    UINT8 bEfStructure = 0;

    if (NULL == sResponse ||
        0 == cbResponse ||
        NULL == sUSIM)
    {
        RIL_LOG_CRITICAL("ParseUSIMRecordStatus - Input parameters\r\n");
        goto Error;
    }

    // Need at least 2 bytes for response data FCP (file control parameters)
    if (2 > cbResponse)
    {
        RIL_LOG_CRITICAL("ParseUSIMRecordStatus - Need at least 2 bytes for response data\r\n");
        goto Error;
    }

    // Validate this response is a 3GPP 102 221 SELECT response.
    tlvFcp.Parse(sResponse, cbResponse);
    if (FCP_TAG != tlvFcp.GetTag())
    {
        RIL_LOG_CRITICAL("ParseUSIMRecordStatus - First tag is not FCP.  Tag=[0x%02X]\r\n",
                tlvFcp.GetTag());
        goto Error;
    }

    if (cbResponse != tlvFcp.GetTotalSize())
    {
        RIL_LOG_CRITICAL("ParseUSIMRecordStatus -"
                " cbResponse=[%d] not equal to total size=[%d]\r\n",
                cbResponse, tlvFcp.GetTotalSize());
        goto Error;
    }
    pbFcpData = tlvFcp.GetValue();
    cbFcpDataSize = tlvFcp.GetLength();

    // Retrieve the File Descriptor data object
    tlvFileDescriptor.Parse(pbFcpData, cbFcpDataSize);
    if (FILE_DESCRIPTOR_TAG != tlvFileDescriptor.GetTag())
    {
        RIL_LOG_CRITICAL("ParseUSIMRecordStatus -"
                " File descriptor tag is not FCP.  Tag=[0x%02X]\r\n", tlvFileDescriptor.GetTag());
        goto Error;
    }

    cbDataUsed = tlvFileDescriptor.GetTotalSize();
    if (cbDataUsed > cbFcpDataSize)
    {
        RIL_LOG_CRITICAL("ParseUSIMRecordStatus -"
                " cbDataUsed=[%d] is greater than cbFcpDataSize=[%d]\r\n",
                cbDataUsed, cbFcpDataSize);
        goto Error;
    }

    pbFileDescData = tlvFileDescriptor.GetValue();
    cbFileDescDataSize = tlvFileDescriptor.GetLength();
    // File descriptors should only be 2 or 5 bytes long.
    if((2 != cbFileDescDataSize) && (5 != cbFileDescDataSize))
    {
        RIL_LOG_CRITICAL("ParseUSIMRecordStatus -"
                " File descriptor can only be 2 or 5 bytes.  cbFileDescDataSize=[%d]\r\n",
                cbFileDescDataSize);
        goto Error;

    }
    if (2 > cbFileDescDataSize)
    {
        RIL_LOG_CRITICAL("ParseUSIMRecordStatus -"
                " File descriptor less than 2 bytes.  cbFileDescDataSize=[%d]\r\n",
                cbFileDescDataSize);
        goto Error;
    }

    fIsDf = (0x38 == (0x38 & pbFileDescData[0]));
    bEfStructure = (0x07 & pbFileDescData[0]);

    if (fIsDf)
    {
        dwRecordType = RIL_SIMRECORDTYPE_DEDICATED;
    }
    // or it is an EF or MF.
    else
    {
        switch (bEfStructure)
        {
            // Transparent
            case 0x01:
                dwRecordType = RIL_SIMRECORDTYPE_TRANSPARENT;
                break;

            // Linear Fixed
            case 0x02:
                dwRecordType = RIL_SIMRECORDTYPE_LINEAR;
                break;

            // Cyclic
            case 0x06:
                dwRecordType = RIL_SIMRECORDTYPE_CYCLIC;
                break;

           default:
                break;
        }

        if (RIL_SIMRECORDTYPE_LINEAR == dwRecordType ||
            RIL_SIMRECORDTYPE_CYCLIC == dwRecordType)
        {
            // Need at least 5 bytes
            if (5 > cbFileDescDataSize)
            {
                RIL_LOG_CRITICAL("ParseUSIMRecordStatus -"
                        " File descriptor less than 5 bytes.  cbFileDescDataSize=[%d]\r\n",
                        cbFileDescDataSize);
                goto Error;
            }

            dwItemCount = pbFileDescData[4];
            dwRecordSize = (pbFileDescData[2] << 4) + (pbFileDescData[3]);

            // Skip checking of consistency with the File Size data object to
            // save time.
            dwTotalSize = dwItemCount * dwRecordSize;
        }
        else if(RIL_SIMRECORDTYPE_TRANSPARENT == dwRecordType)
        {
            // Retrieve the file size.
            BerTlv tlvCurrent;

            while (cbFcpDataSize > cbDataUsed)
            {
                if (!tlvCurrent.Parse(
                    pbFcpData + cbDataUsed,
                    cbFcpDataSize - cbDataUsed))
                {
                    RIL_LOG_CRITICAL("ParseUSIMRecordStatus - Couldn't parse TRANSPARENT\r\n");
                    goto Error;
                }

                cbDataUsed += tlvCurrent.GetTotalSize();

                if (FILE_SIZE_TAG == tlvCurrent.GetTag())
                {
                    const UINT8* pbFileSize = NULL;

                    if (2 > tlvCurrent.GetLength())
                    {
                        RIL_LOG_CRITICAL("ParseUSIMRecordStatus -"
                                " TRANSPARENT length must be at least 2\r\n");
                        goto Error;
                    }

                    pbFileSize = tlvCurrent.GetValue();

                    dwTotalSize = (pbFileSize[0] << 4) + pbFileSize[1];

                    // Size found. Leave loop
                    break;
                }
            }
        }
    }

    // if record type has not been resolved, check for Master file.
    if (RIL_SIMRECORDTYPE_UNKNOWN == dwRecordType)
    {
        const UINT8* pbFileId = NULL;
        UINT32 uFileId = 0;

        // Next data object should be File ID.
        BerTlv tlvFileId;
        tlvFileId.Parse(
            pbFcpData + cbFcpDataSize,
            cbFcpDataSize - cbDataUsed);

        if (FILE_ID_TAG != tlvFileId.GetTag())
        {
            RIL_LOG_CRITICAL("ParseUSIMRecordStatus - UNKNOWN tag not equal to FILE_ID_TAG\r\n");
            goto Error;
        }

        if (2 != tlvFileId.GetLength())
        {
            RIL_LOG_CRITICAL("ParseUSIMRecordStatus - UNKNOWN length not equal to 2\r\n");
            goto Error;
        }

        pbFileId = tlvFileId.GetValue();
        uFileId = (pbFileId[0] << 4) + pbFileId[1];

        if (MASTER_FILE_ID != uFileId)
        {
            RIL_LOG_CRITICAL("ParseUSIMRecordStatus -"
                    " UNKNOWN file ID not equal to MASTER_FILE_ID\r\n");
            goto Error;
        }

        dwRecordType = RIL_SIMRECORDTYPE_MASTER;
    }

    sUSIM->dwRecordType = dwRecordType;
    sUSIM->dwTotalSize = dwTotalSize;
    sUSIM->dwRecordSize = dwRecordSize;

    bRet = TRUE;
Error:

    RIL_LOG_VERBOSE("ParseUSIMRecordStatus - EXIT = [%d]\r\n", bRet);
    return bRet;
}

RIL_RESULT_CODE CTE_XMM6260::HandlePin2RelatedSIMIO(RIL_SIM_IO_v6* pSimIOArgs,
                                                        REQUEST_DATA& rReqData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::HandlePin2RelatedSIMIO() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char szSimIoCmd[MAX_BUFFER_SIZE] = {0};

    //  If PIN2 is required, send out AT+CPIN2 request
    if (NULL == pSimIOArgs->pin2 && SIM_COMMAND_UPDATE_RECORD == pSimIOArgs->command)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                " PIN 2 required but not provided!\r\n");
        res = RIL_E_SIM_PIN2;
        goto Error;
    }

    if (RIL_PINSTATE_ENABLED_BLOCKED == m_ePin2State)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                " RIL_PINSTATE_ENABLED_BLOCKED\r\n");
        res = RIL_E_SIM_PUK2;
        goto Error;
    }
    else if (RIL_PINSTATE_ENABLED_PERM_BLOCKED == m_ePin2State)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                " RIL_PINSTATE_ENABLED_PERM_BLOCKED\r\n");
        res = RIL_E_GENERIC_FAILURE;
        goto Error;
    }

    if (NULL == pSimIOArgs->data)
    {
        if(NULL == pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(szSimIoCmd, sizeof(szSimIoCmd),
                "+CRSM=%d,%d,%d,%d,%d\r",
                pSimIOArgs->command,
                pSimIOArgs->fileid,
                pSimIOArgs->p1,
                pSimIOArgs->p2,
                pSimIOArgs->p3))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                        " cannot create CRSM command!\r\n");
                goto Error;
            }
        }
        else // (NULL != pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(szSimIoCmd, sizeof(szSimIoCmd),
                          "+CRSM=%d,%d,%d,%d,%d,,\"%s\"\r",
                          pSimIOArgs->command,
                          pSimIOArgs->fileid,
                          pSimIOArgs->p1,
                          pSimIOArgs->p2,
                          pSimIOArgs->p3,
                          pSimIOArgs->path))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                        " cannot create CRSM command!\r\n");
                goto Error;
            }
        }
    }
    else // (NULL != pSimIOArgs->data)
    {
        if(NULL == pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(szSimIoCmd, sizeof(szSimIoCmd),
                "+CRSM=%d,%d,%d,%d,%d,\"%s\"\r",
                pSimIOArgs->command,
                pSimIOArgs->fileid,
                pSimIOArgs->p1,
                pSimIOArgs->p2,
                pSimIOArgs->p3,
                pSimIOArgs->data))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                        " cannot create CRSM command!\r\n");
                goto Error;
            }
        }
        else // (NULL != pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(szSimIoCmd, sizeof(szSimIoCmd),
                "+CRSM=%d,%d,%d,%d,%d,\"%s\",\"%s\"\r",
                pSimIOArgs->command,
                pSimIOArgs->fileid,
                pSimIOArgs->p1,
                pSimIOArgs->p2,
                pSimIOArgs->p3,
                pSimIOArgs->data,
                pSimIOArgs->path))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                        " cannot create CRSM command!\r\n");
                goto Error;
            }
        }
    }

    if (RIL_PINSTATE_ENABLED_NOT_VERIFIED == m_ePin2State ||
                    RIL_PINSTATE_UNKNOWN == m_ePin2State)
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1,
            sizeof(rReqData.szCmd1),
            "AT+CPIN2=\"%s\";%s\r",
            pSimIOArgs->pin2, szSimIoCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                    " cannot create CPIN2 command!\r\n");
            goto Error;
        }
    }
    else if (RIL_PINSTATE_ENABLED_VERIFIED == m_ePin2State)
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1,
            sizeof(rReqData.szCmd1),
            "AT+CPWD=\"P2\",\"%s\",\"%s\";%s\r",
            pSimIOArgs->pin2,
            pSimIOArgs->pin2, szSimIoCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::HandlePin2RelatedSIMIO() -"
                    " cannot create CPWD command!\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::HandlePin2RelatedSIMIO() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIM_IO
//
RIL_RESULT_CODE CTE_XMM6260::CoreSimIo(REQUEST_DATA & rReqData, void * pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSimIo() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_SIM_IO_v6*   pSimIOArgs = NULL;
    // substitute actual path instead of string "img"
    char szGraphicsPath[] = "3F007F105F50";
    char szImg[] = "img";
    char* pszPath = NULL;
    S_SIM_IO_CONTEXT_DATA* pContextData = NULL;
    char szAid[MAX_AID_SIZE] = {'\0'};

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSimIo() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if (sizeof(RIL_SIM_IO_v6) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSimIo() - Invalid data size. Given %d bytes\r\n",
                uiDataSize);
        goto Error;
    }

    // extract data
    pSimIOArgs = (RIL_SIM_IO_v6 *)pData;

#if defined(DEBUG)
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSimIo() -"
            " command=%d fileid=%04X path=\"%s\" p1=%d p2=%d p3=%d data=\"%s\""
            " pin2=\"%s\" aidPtr=\"%s\"\r\n",
            pSimIOArgs->command, pSimIOArgs->fileid, pSimIOArgs->path,
            pSimIOArgs->p1, pSimIOArgs->p2, pSimIOArgs->p3,
            pSimIOArgs->data, pSimIOArgs->pin2, pSimIOArgs->aidPtr);
#else
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSimIo() - command=%d fileid=%04X path=\"%s\""
            " p1=%d p2=%d p3=%d data=\"%s\" aidPtr=\"%s\"\r\n",
            pSimIOArgs->command, pSimIOArgs->fileid, pSimIOArgs->path,
            pSimIOArgs->p1, pSimIOArgs->p2, pSimIOArgs->p3,
            pSimIOArgs->data, pSimIOArgs->aidPtr);
#endif

    rReqData.pContextData = NULL;

    pContextData = (S_SIM_IO_CONTEXT_DATA*) malloc(sizeof(S_SIM_IO_CONTEXT_DATA));
    if (NULL != pContextData)
    {
        pContextData->fileId = pSimIOArgs->fileid;
        pContextData->command = pSimIOArgs->command;
        rReqData.pContextData = pContextData;
        rReqData.cbContextData = sizeof(S_SIM_IO_CONTEXT_DATA);
    }

    GetSimAppId(RIL_APPTYPE_ISIM, szAid, sizeof(szAid));

    if (RIL_APPSTATE_READY == GetIsimAppState() && NULL != pSimIOArgs->aidPtr
            && (0 == strcmp(pSimIOArgs->aidPtr, szAid)))
    {
        int sessionId = GetSessionId(RIL_APPTYPE_ISIM);
        POST_CMD_HANDLER_DATA data;
        memset(&data, 0, sizeof(data));
        data.uiChannel = g_pReqInfo[RIL_REQUEST_SIM_IO].uiChannel;
        data.requestId = RIL_REQUEST_SIM_IO;

        CEvent::Reset(m_pUiccOpenLogicalChannelEvent);

        if (0 >= sessionId && OpenLogicalChannel(data, szAid))
        {
            CEvent::Wait(m_pUiccOpenLogicalChannelEvent, WAIT_FOREVER);
        }

        sessionId = GetSessionId(RIL_APPTYPE_ISIM);
        if (0 >= sessionId)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CoreSimIo() - OpenLogicalChannel failed\r\n");
            goto Error;
        }
        else
        {
            return HandleSimIO(pSimIOArgs, rReqData, sessionId);
        }
    }

    switch (pSimIOArgs->fileid)
    {
        case EF_FDN:
        case EF_EXT2:
            if (EF_FDN == pSimIOArgs->fileid &&
                        SIM_COMMAND_UPDATE_RECORD == pSimIOArgs->command)
            {
                return HandlePin2RelatedSIMIO(pSimIOArgs, rReqData);
            }
        break;

        default:
        break;
    }

    //  Replace path of "img" with "3F007F105F50"
    if (pSimIOArgs->path)
    {
        if (0 == strcmp(pSimIOArgs->path, szImg))
        {
            //  We have a match for "img"
            RIL_LOG_INFO("CTE_XMM6260::CoreSimIo() -"
                    " Found match for path='img'.  Use GRAPHICS path\r\n");
            pszPath = szGraphicsPath;
        }
        else
        {
            //  Original path
            pszPath = pSimIOArgs->path;
        }
    }

    if (NULL == pSimIOArgs->data)
    {
        if(NULL == pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                sizeof(rReqData.szCmd1),
                "AT+CRSM=%d,%d,%d,%d,%d\r",
                pSimIOArgs->command,
                pSimIOArgs->fileid,
                pSimIOArgs->p1,
                pSimIOArgs->p2,
                pSimIOArgs->p3))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSimIo() - cannot create CRSM command\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                sizeof(rReqData.szCmd1),
                "AT+CRSM=%d,%d,%d,%d,%d,,\"%s\"\r",
                pSimIOArgs->command,
                pSimIOArgs->fileid,
                pSimIOArgs->p1,
                pSimIOArgs->p2,
                pSimIOArgs->p3,
                pszPath))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSimIo() - cannot create CRSM command\r\n");
                goto Error;
            }
        }
    }
    else
    {
        if(NULL == pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                sizeof(rReqData.szCmd1),
                "AT+CRSM=%d,%d,%d,%d,%d,\"%s\"\r",
                pSimIOArgs->command,
                pSimIOArgs->fileid,
                pSimIOArgs->p1,
                pSimIOArgs->p2,
                pSimIOArgs->p3,
                pSimIOArgs->data))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSimIo() - cannot create CRSM command\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                sizeof(rReqData.szCmd1),
                "AT+CRSM=%d,%d,%d,%d,%d,\"%s\",\"%s\"\r",
                pSimIOArgs->command,
                pSimIOArgs->fileid,
                pSimIOArgs->p1,
                pSimIOArgs->p2,
                pSimIOArgs->p3,
                pSimIOArgs->data,
                pszPath))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSimIo() - cannot create CRSM command\r\n");
                goto Error;
            }
        }
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSimIo() - Exit\r\n");
    return res;
}



RIL_RESULT_CODE CTE_XMM6260::ParseSimIo(RESPONSE_DATA & rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSimIo() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    UINT32  uiSW1 = 0;
    UINT32  uiSW2 = 0;
    char* szResponseString = NULL;
    UINT32  cbResponseString = 0;
    RIL_SIM_IO_Response* pResponse = NULL;

    if (NULL == rRspData.szResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() - Response String pointer is NULL.\r\n");
        goto Error;
    }

    // Parse "<prefix>+CRSM: <sw1>,<sw2>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() - Could not skip over response prefix.\r\n");
        goto Error;
    }

    if (!SkipString(pszRsp, "+CRSM: ", pszRsp)
            && !SkipString(pszRsp, "+CRLA: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() - Could not skip over \"+CRSM: \""
                "\"+CRLA: \"\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, uiSW1, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() - Could not extract SW1 value.\r\n");
        goto Error;
    }

    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiSW2, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() - Could not extract SW2 value.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::ParseSimIo() - Extracted SW1 = %u and SW2 = %u\r\n", uiSW1, uiSW2);

    // Parse ","
    if (SkipString(pszRsp, ",", pszRsp))
    {
        int nCRSMCommand = 0;
        S_SIM_IO_CONTEXT_DATA* pContextData = NULL;

        // Parse <response>
        // NOTE: we take ownership of allocated szResponseString
        if (!ExtractQuotedStringWithAllocatedMemory(pszRsp,
                                          szResponseString,
                                          cbResponseString,
                                          pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() - Could not extract data string.\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CTE_XMM6260::ParseSimIo() -"
                    " Extracted data string: \"%s\" (%u chars including NULL)\r\n",
                    szResponseString, cbResponseString);
        }

        if (0 != (cbResponseString - 1) % 2)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() : String was not a multiple of 2.\r\n");
            goto Error;
        }

        // Check for USIM GET_RESPONSE response
        if (NULL == rRspData.pContextData ||
                    rRspData.cbContextData != sizeof(S_SIM_IO_CONTEXT_DATA))
        {
            goto Error;
        }

        pContextData = (S_SIM_IO_CONTEXT_DATA*) rRspData.pContextData;
        nCRSMCommand = pContextData->command;

        if ((192 == nCRSMCommand) && ('6' == szResponseString[0]) && ('2' == szResponseString[1]))
        {
            //  USIM GET_RESPONSE response
            RIL_LOG_INFO("CTE_XMM6260::ParseSimIo() - USIM GET_RESPONSE\r\n");

            char szTemp[5] = {0};

            struct sUSIMGetResponse sUSIM;
            memset(&sUSIM, 0, sizeof(struct sUSIMGetResponse));

            //  Need to convert the response string from ascii to binary.
            BYTE* sNewString = NULL;
            UINT32 cbNewString = (cbResponseString-1)/2;
            sNewString = new BYTE[cbNewString];
            if (NULL == sNewString)
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() - Cannot create new string!\r\n");
                goto Error;
            }
            memset(sNewString, 0, cbNewString);

            UINT32 cbUsed = 0;
            if (!GSMHexToGSM(szResponseString, cbResponseString, sNewString, cbNewString, cbUsed))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() -"
                        " Cannot cconvert szResponseString to GSMHex.\r\n");
                delete[] sNewString;
                sNewString = NULL;
                cbNewString = 0;
                goto Error;
            }

            //  OK, now parse!
            if (!ParseUSIMRecordStatus((UINT8*)sNewString, cbNewString, &sUSIM))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() - Cannot parse USIM record status\r\n");
                delete[] sNewString;
                sNewString = NULL;
                cbNewString = 0;
                goto Error;
            }


            delete[] sNewString;
            sNewString = NULL;
            cbNewString = 0;

            RIL_LOG_VERBOSE("CTE_XMM6260::ParseSimIo() - sUSIM.dwRecordType=[0x%04X]\r\n",
                    sUSIM.dwRecordType);
            RIL_LOG_VERBOSE("CTE_XMM6260::ParseSimIo() - sUSIM.dwTotalSize=[0x%04X]\r\n",
                    sUSIM.dwTotalSize);
            RIL_LOG_VERBOSE("CTE_XMM6260::ParseSimIo() - sUSIM.dwRecordSize=[0x%04X]\r\n",
                    sUSIM.dwRecordSize);

            //  Delete old original response.  Create new "fake" response.
            delete []szResponseString;
            szResponseString = NULL;
            cbResponseString = 0;

            //  Java layers as of Froyo (Nov 1/2010) only use:
            //  Total size (0-based bytes 2 and 3), File type (0-based byte 6),
            //  Data structure (0-based byte 13), Data record length (0-based byte 14)
            cbResponseString = 31;  //  15 bytes + NULL
            szResponseString = new char[cbResponseString];
            if (NULL == szResponseString)
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() -"
                        " Cannot create new szResponsestring!\r\n");
                delete[] sNewString;
                sNewString = NULL;
                goto Error;
            }
            if (!CopyStringNullTerminate(szResponseString,
                    "000000000000000000000000000000", cbResponseString))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() -"
                        " Cannot CopyStringNullTerminate szResponsestring!\r\n");
                delete[] sNewString;
                sNewString = NULL;
                goto Error;
            }

            //  Extract info, put into new response string
            if (!PrintStringNullTerminate(szTemp, 5, "%04X", sUSIM.dwTotalSize))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() -"
                        " Cannot PrintStringNullTerminate sUSIM.dwTotalSize!\r\n");
                delete[] sNewString;
                sNewString = NULL;
                goto Error;
            }
            memcpy(&szResponseString[4], szTemp, 4);

            if (!PrintStringNullTerminate(szTemp, 3, "%02X", sUSIM.dwRecordSize))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() -"
                        " Cannot PrintStringNullTerminate sUSIM.dwRecordSize!\r\n");
                delete[] sNewString;
                sNewString = NULL;
                goto Error;
            }
            memcpy(&szResponseString[28], szTemp, 2);

            if (RIL_SIMRECORDTYPE_UNKNOWN == sUSIM.dwRecordType)
            {
                //  bad parse.
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() -"
                        " sUSIM.dwRecordType is unknown!\r\n");
                delete[] sNewString;
                sNewString = NULL;
                goto Error;
            }


            if (RIL_SIMRECORDTYPE_MASTER == sUSIM.dwRecordType)
            {
                szResponseString[13] = '1';
            }
            else if (RIL_SIMRECORDTYPE_DEDICATED == sUSIM.dwRecordType)
            {
                szResponseString[13] = '2';
            }
            else
            {
                //  elementary file
                szResponseString[13] = '4';

                if (RIL_SIMRECORDTYPE_TRANSPARENT == sUSIM.dwRecordType)
                {
                    szResponseString[27] = '0';
                }
                else if (RIL_SIMRECORDTYPE_LINEAR == sUSIM.dwRecordType)
                {
                    szResponseString[27] = '1';
                }
                else if (RIL_SIMRECORDTYPE_CYCLIC == sUSIM.dwRecordType)
                {
                    szResponseString[27] = '3';
                }
            }

            //  ok we're done.  print.
            RIL_LOG_INFO("CTE_XMM6260::ParseSimIo() - new USIM response=[%s]\r\n",
                    szResponseString);

        }

    }

    // Allocate memory for the response struct PLUS a buffer for the response string
    // The char* in the RIL_SIM_IO_Response will point to the buffer allocated directly after the
    // RIL_SIM_IO_Response, When the RIL_SIM_IO_Response is deleted, the corresponding response
    // string will be freed as well.
    pResponse = (RIL_SIM_IO_Response*)malloc(sizeof(RIL_SIM_IO_Response) + cbResponseString + 1);
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() -"
                " Could not allocate memory for a RIL_SIM_IO_Response struct.\r\n");
        goto Error;
    }
    memset(pResponse, 0, sizeof(RIL_SIM_IO_Response) + cbResponseString + 1);

    pResponse->sw1 = uiSW1;
    pResponse->sw2 = uiSW2;

    if (NULL == szResponseString)
    {
        pResponse->simResponse = NULL;
    }
    else
    {
        pResponse->simResponse = (char*)(((char*)pResponse) + sizeof(RIL_SIM_IO_Response));
        if (!CopyStringNullTerminate(pResponse->simResponse, szResponseString, cbResponseString))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimIo() -"
                    " Cannot CopyStringNullTerminate szResponseString\r\n");
            goto Error;
        }

        // Ensure NULL termination!
        pResponse->simResponse[cbResponseString] = '\0';
    }

    // Parse "<postfix>"
    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        goto Error;
    }

    rRspData.pData   = (void*)pResponse;
    rRspData.uiDataSize  = sizeof(RIL_SIM_IO_Response);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    delete[] szResponseString;
    szResponseString = NULL;

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSimIo() - Exit\r\n");
    return res;
}



//
// RIL_REQUEST_DEACTIVATE_DATA_CALL
//
RIL_RESULT_CODE CTE_XMM6260::CoreDeactivateDataCall(REQUEST_DATA& rReqData,
                                                                void* pData,
                                                                UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreDeactivateDataCall() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszCid = NULL;
    UINT32 uiCID = 0;
    const LONG REASON_RADIO_OFF = 1;
    LONG reason = 0;
    CChannel_Data* pChannelData = NULL;

    if (uiDataSize < (1 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreDeactivateDataCall() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreDeactivateDataCall() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CoreDeactivateDataCall() - uiDataSize=[%d]\r\n", uiDataSize);

    pszCid = ((char**)pData)[0];
    if (pszCid == NULL || '\0' == pszCid[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreDeactivateDataCall() - pszCid was NULL\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CoreDeactivateDataCall() - pszCid=[%s]\r\n", pszCid);

    //  Get CID as UINT32.
    if (sscanf(pszCid, "%u", &uiCID) == EOF)
    {
        // Error
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreDeactivateDataCall() -  cannot convert %s to int\r\n",
                pszCid);
        goto Error;
    }

    if ((RIL_VERSION >= 4) && (uiDataSize >= (2 * sizeof(char *))))
    {
        reason = GetDataDeactivateReason(((char**)pData)[1]);
        RIL_LOG_INFO("CTE_XMM6260::CoreDeactivateDataCall() - reason=[%ld]\r\n", reason);
    }

    if (reason == REASON_RADIO_OFF || RIL_APPSTATE_READY != GetSimAppState())
    {
        // complete the request without sending the AT command to modem.
        res = RRIL_RESULT_OK_IMMEDIATE;
        DataConfigDown(uiCID, TRUE);
        goto Error;
    }
    else
    {
        UINT32* pCID = (UINT32*)malloc(sizeof(UINT32));
        if (NULL == pCID)
        {
            goto Error;
        }

        *pCID = uiCID;

        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CGACT=0,%s\r", pszCid))
        {
            free(pCID);
            pCID = NULL;
            goto Error;
        }
        else
        {
            // Set the context of this command to the CID (for multiple context support).
            rReqData.pContextData = (void*)pCID;
            rReqData.cbContextData = sizeof(UINT32);
        }
    }

    res = RRIL_RESULT_OK;
    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL != pChannelData)
    {
        pChannelData->SetDataState(E_DATA_STATE_DEACTIVATING);
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreDeactivateDataCall() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseDeactivateDataCall(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseDeactivateDataCall() - Enter\r\n");

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseDeactivateDataCall() - Exit\r\n");
    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_OEM_HOOK_RAW
//
// [out] UINT32 uiRilChannel - Set this value to the RIL channel that the command will be sent on
//                             e.g. RIL_CHANNEL_DLC2  (as defined in rilchannels.h)
//                             Default value is RIL_CHANNEL_ATCMD
//
// Note: Use REQUEST_DATA's pContextData2 to pass custom data to the parse function.
//       RIL Framework uses pContextData, and is reserved in this function.
RIL_RESULT_CODE CTE_XMM6260::CoreHookRaw(REQUEST_DATA& rReqData,
                                                     void* pData,
                                                     UINT32 uiDataSize,
                                                     UINT32& /*uiRilChannel*/)
{
    RIL_LOG_INFO("CTE_XMM6260::CoreHookRaw() - Enter\r\n");

    BYTE* pDataBytes = (BYTE*)pData;

    RIL_LOG_INFO("CTE_XMM6260::CoreHookRaw() - uiDataSize=[%d]\r\n", uiDataSize);
    for (int i = 0; i < (int)uiDataSize; i++)
    {
        BYTE b = pDataBytes[i];
        RIL_LOG_INFO("CTE_XMM6260::CoreHookRaw() - pData[%d]=[0x%02X]\r\n", i, (unsigned char)b);
    }

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    int nCommand = 0;

    //  uiDataSize MUST be 4 or greater.
    if (4 > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookRaw() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookRaw() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    memcpy(&nCommand, &(pDataBytes[0]), sizeof(int));
    nCommand = ntohl(nCommand);  //  This is the command.
    rReqData.pContextData = (void*)(intptr_t)nCommand;

    switch(nCommand)
    {
        default:
            RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookRaw() - Received unknown command=[0x%08X]\r\n",
                    nCommand);
            goto Error;
            break;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_INFO("CTE_XMM6260::CoreHookRaw() - Exit\r\n");
    return res;
}


RIL_RESULT_CODE CTE_XMM6260::ParseHookRaw(RESPONSE_DATA & rRspData)
{
    RIL_LOG_INFO("CTE_XMM6260::ParseHookRaw() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;
    int nCommand = (intptr_t)rRspData.pContextData;

    //  Populate these below (if applicable)
    void* pData = NULL;
    UINT32 uiDataSize = 0;

    res = RRIL_RESULT_OK;

    rRspData.pData   = pData;
    rRspData.uiDataSize  = uiDataSize;

    RIL_LOG_INFO("CTE_XMM6260::ParseHookRaw() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_OEM_HOOK_STRINGS
//
// [out] UINT32 uiRilChannel - Set this value to the RIL channel that the command will be sent on
//                             e.g. RIL_CHANNEL_DLC2  (as defined in rilchannels.h)
//                             Default value is RIL_CHANNEL_ATCMD
//
// Note: Use REQUEST_DATA's pContextData2 to pass custom data to the parse function.
//       RIL Framework uses pContextData, and is reserved in this function.
RIL_RESULT_CODE CTE_XMM6260::CoreHookStrings(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize,
                                                         UINT32& uiRilChannel)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreHookStrings() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char** pszRequest = NULL;
    int command = 0;
    int nNumStrings = 0;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - Data pointer is NULL.\r\n");
        goto Error;
    }

    pszRequest = ((char**)pData);
    if (NULL == pszRequest || NULL == pszRequest[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - pszRequest was NULL\r\n");
        goto Error;
    }

    if ( (uiDataSize < (1 * sizeof(char *))) || (0 != (uiDataSize % sizeof(char*))) )
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    //  Loop through input strings and print them
    nNumStrings = uiDataSize / sizeof(char*);
    RIL_LOG_INFO("CTE_XMM6260::CoreHookStrings() - uiDataSize=[%d], numStrings=[%d]\r\n",
            uiDataSize, nNumStrings);
    for (int i=0; i<nNumStrings; i++)
    {
        RIL_LOG_INFO("CTE_XMM6260::CoreHookStrings() - pszRequest[%d]=[%s]\r\n",
                i, pszRequest[i]);
    }

    //  Get command as int.
    if (sscanf(pszRequest[0], "%d", &command) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - cannot convert %s to int\r\n",
                pszRequest);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CoreHookStrings(), command: %d", command);

    if (E_MMGR_EVENT_MODEM_UP != m_cte.GetLastModemEvent()
            && RIL_OEM_HOOK_STRING_NOTIFY_RELEASE_MODEM != command
            && RIL_OEM_HOOK_STRING_GET_OEM_VERSION != command
            && RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_IND != command)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings(), "
                "unable to process command in modem not up\r\n");
        goto Error;
    }

    switch(command)
    {
        case RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR");
            res = CreateGetThermalSensorValuesReq(rReqData, (const char**) pszRequest, uiDataSize);
            break;

        case RIL_OEM_HOOK_STRING_THERMAL_SET_THRESHOLD:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_THERMAL_SET_THRESHOLD\r\n");
            res = CreateActivateThermalSensorInd(rReqData, (const char**) pszRequest, uiDataSize);
            // Send this command on URC channel to get the notification in URC channel
            uiRilChannel = RIL_CHANNEL_URC;
            break;

        case RIL_OEM_HOOK_STRING_GET_ATR:
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XGATR\r"))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() -"
                        " ERROR: RIL_OEM_HOOK_STRINGS_GET_ATR - Can't construct szCmd1.\r\n");
                goto Error;
            }
            res = RRIL_RESULT_OK;
            break;

        case RIL_OEM_HOOK_STRING_SET_MODEM_AUTO_FAST_DORMANCY:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_MODEM_AUTO_FAST_DORMANCY");
            // Check if Fast Dormancy mode allows OEM Hook
            if (m_cte.GetFastDormancyMode() == E_FD_MODE_OEM_MANAGED)
            {
                res = CreateAutonomousFDReq(rReqData, (const char**) pszRequest, uiDataSize);
            }
            else
            {
                RIL_LOG_INFO("RIL_OEM_HOOK_STRING_SET_MODEM_AUTO_FAST_DORMANCY -"
                        " FD Mode is not OEM Managed.\r\n");
                goto Error;
            }
            break;

        case RIL_OEM_HOOK_STRING_GET_GPRS_CELL_ENV:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_GET_GPRS_CELL_ENV");
            //  Send this command on OEM channel.
            uiRilChannel = RIL_CHANNEL_OEM;
            res = CreateGetGprsCellEnvReq(rReqData);
            break;

        case RIL_OEM_HOOK_STRING_DEBUG_SCREEN_COMMAND:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_DEBUG_SCREEN_COMMAND");
            res = CreateDebugScreenReq(rReqData, (const char**) pszRequest, uiDataSize);
            //  Send this command on OEM channel.
            uiRilChannel = RIL_CHANNEL_OEM;
            break;

        case RIL_OEM_HOOK_STRING_RELEASE_ALL_CALLS:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_RELEASE_ALL_CALLS");
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CHLD=8\r"))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() -"
                        " RIL_OEM_HOOK_STRING_RELEASE_ALL_CALLS - Can't construct szCmd1.\r\n");
                goto Error;
            }
            res = RRIL_RESULT_OK;
            break;

        case RIL_OEM_HOOK_STRING_GET_SMS_TRANSPORT_MODE:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_GET_SMS_TRANSPORT_MODE");
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CGSMS?\r"))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - "
                        "RIL_OEM_HOOK_STRING_GET_SMS_TRANSPORT_MODE -"
                        " Can't construct szCmd1.\r\n");
                goto Error;
            }
            //  Send this command on SMS channel.
            uiRilChannel = RIL_CHANNEL_DLC6;
            res = RRIL_RESULT_OK;
            break;

        case RIL_OEM_HOOK_STRING_SET_SMS_TRANSPORT_MODE:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_SMS_TRANSPORT_MODE");
            //  Send this command on SMS channel.
            uiRilChannel = RIL_CHANNEL_DLC6;
            res = CreateSetSMSTransportModeReq(rReqData, (const char**) pszRequest, uiDataSize);
            break;

        case RIL_OEM_HOOK_STRING_GET_RF_POWER_CUTBACK_TABLE:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_GET_RF_POWER_CUTBACK_TABLE");
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                    sizeof(rReqData.szCmd1), "AT+XRFCBT?\r"))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - "
                        "RIL_OEM_HOOK_STRING_GET_RF_POWER_CUTBACK_TABLE - "
                        "Can't construct szCmd1.\r\n");
                goto Error;
            }
            //  Send this command on OEM channel.
            uiRilChannel = RIL_CHANNEL_OEM;
            res = RRIL_RESULT_OK;
            break;

        case RIL_OEM_HOOK_STRING_SET_RF_POWER_CUTBACK_TABLE:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_RF_POWER_CUTBACK_TABLE");
            //  Send this command on OEM channel.
            uiRilChannel = RIL_CHANNEL_OEM;
            res = CreateSetRFPowerCutbackTableReq(rReqData,
                    (const char**) pszRequest, uiDataSize);
            break;

        case RIL_OEM_HOOK_STRING_SWAP_PS:
            {
                if (CHardwareConfig::GetInstance().IsMultiSIM())
                {
                    RIL_LOG_INFO("Received Command: RIL_OEM_HOOK_STRING_SWAP_PS");
                    if (!PrintStringNullTerminate(rReqData.szCmd1,
                            sizeof(rReqData.szCmd1), "AT+XRAT=8\r"))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - "
                                "RIL_OEM_HOOK_STRING_SWAP_PS - Can't construct szCmd1.\r\n");
                        goto Error;
                    }
                    rReqData.uiTimeout = 450000;
                    res = RRIL_RESULT_OK;
                }
                break;
            }

        case RIL_OEM_HOOK_STRING_SIM_RESET:
            if (CHardwareConfig::GetInstance().IsMultiSIM())
            {
                RIL_LOG_INFO("Received Command: RIL_OEM_HOOK_STRING_SIM_RESET");
                if (!PrintStringNullTerminate(rReqData.szCmd1,
                        sizeof(rReqData.szCmd1), "AT+CFUN=22,1\r"))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - "
                            "RIL_OEM_HOOK_STRING_SIM_RESET - Can't construct szCmd1.\r\n");
                    goto Error;
                }
                //  Send this command on OEM channel.
                uiRilChannel = RIL_CHANNEL_OEM;
                res = RRIL_RESULT_OK;
            }
            break;

        case RIL_OEM_HOOK_STRING_GET_DVP_STATE:
            if (CHardwareConfig::GetInstance().IsMultiSIM())
            {
                RIL_LOG_INFO("Received Command: RIL_OEM_HOOK_STRING_GET_DVP_STATE");
                if (!PrintStringNullTerminate(rReqData.szCmd1,
                        sizeof(rReqData.szCmd1), "AT+XDVP?\r"))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - "
                            "RIL_OEM_HOOK_STRING_GET_DVP_STATE - Can't construct szCmd1.\r\n");
                    goto Error;
                }
                //  Send this command on OEM channel.
                uiRilChannel = RIL_CHANNEL_OEM;
                res = RRIL_RESULT_OK;
            }
            break;

        case RIL_OEM_HOOK_STRING_SET_DVP_ENABLED:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_DVP_ENABLED");
            //  Send this command on OEM channel.
            uiRilChannel = RIL_CHANNEL_OEM;
            res = CreateSetDVPEnabledReq(rReqData, (const char**) pszRequest, uiDataSize);
            break;

        case RIL_OEM_HOOK_STRING_IMS_REGISTRATION:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_IMS_REGISTRATION");
            // Send this command on DLC2 channel
            uiRilChannel = RIL_CHANNEL_DLC2;
            res = m_cte.CreateIMSRegistrationReq(rReqData,
                    (const char**) pszRequest, uiDataSize);
            break;

        case RIL_OEM_HOOK_STRING_IMS_CONFIG:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_IMS_CONFIG");
            // Send this command on DLC2 channel
            uiRilChannel = RIL_CHANNEL_DLC2;
            res = m_cte.CreateIMSConfigReq(rReqData,
                    (const char**) pszRequest, nNumStrings);
            break;

        case RIL_OEM_HOOK_STRING_IMS_CALL_STATUS:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_IMS_CALL_STATUS");
            res = SetCallImsAvailable(rReqData,
                    (const char**) pszRequest, nNumStrings);
            break;

        case RIL_OEM_HOOK_STRING_IMS_SMS_STATUS:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_IMS_SMS_STATUS");
            res = SetSmsImsAvailable(rReqData,
                    (const char**) pszRequest, nNumStrings);
            break;

        case RIL_OEM_HOOK_STRING_IMS_GET_PCSCF:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_IMS_GET_PCSCF");
            res = GetPcscf(rReqData,
                    (const char**) pszRequest, nNumStrings);
            break;

        case RIL_OEM_HOOK_STRING_IMS_SRVCC_PARAM:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_IMS_SRVCC_PARAM");
            res = SetSrvccParams(rReqData, (const char**) pszRequest);
            break;

        case RIL_OEM_HOOK_STRING_CSG_SET_AUTOMATIC_SELECTION:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_CSG_SET_AUTOMATIC_SELECTION");
            uiRilChannel = RIL_CHANNEL_URC;
            res = m_cte.SetCsgAutomaticSelection(rReqData);
            break;

        case RIL_OEM_HOOK_STRING_CSG_GET_CURRENT_CSG_STATE:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_CSG_GET_CURRENT_CSG_STATE");
            uiRilChannel = RIL_CHANNEL_DLC2;
            res = m_cte.GetCsgCurrentState(rReqData);
            break;

        case RIL_OEM_HOOK_STRING_SET_DEFAULT_APN:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_DEFAULT_APN");
            // Send this command on ATCMD channel
            uiRilChannel = RIL_CHANNEL_ATCMD;
            res = CreateSetDefaultApnReq(rReqData, (const char**) pszRequest, nNumStrings);
            break;

        case RIL_OEM_HOOK_STRING_NOTIFY_RELEASE_MODEM:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_NOTIFY_RELEASE_MODEM");
            res = HandleReleaseModemReq(rReqData, (const char**) pszRequest, uiDataSize);
        break;

        case RIL_OEM_HOOK_STRING_SEND_AT:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SEND_AT");
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                    sizeof(rReqData.szCmd1), pszRequest[1]))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() - "
                       "RIL_OEM_HOOK_STRING_SEND_AT - Can't construct szCmd1.\r\n");
                goto Error;
            }
            // Send this command on DLC23 dedicated to RF coexistence
            uiRilChannel = RIL_CHANNEL_DLC23;
            res = RRIL_RESULT_OK;
            break;
        case RIL_OEM_HOOK_STRING_GET_OEM_VERSION:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_GET_OEM_VERSION");
            res = GetOemVersion(rReqData);
            break;

        case RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR_V2:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR_V2");
            res = CreateGetThermalSensorValuesV2Req(rReqData, (const char**) pszRequest,
                    uiDataSize);
            break;

        case RIL_OEM_HOOK_STRING_THERMAL_SET_THRESHOLD_V2:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_THERMAL_SET_THRESHOLD_V2\r\n");
            res = CreateActivateThermalSensorV2Ind(rReqData, (const char**) pszRequest,
                    uiDataSize);
            // Send this command on URC channel to get the notification in URC channel
            uiRilChannel = RIL_CHANNEL_URC;
            break;

        case RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_IND:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_IND");
            res = CreateRegStatusAndBandInd(rReqData, (const char**) pszRequest,
                    uiDataSize);
            uiRilChannel = RIL_CHANNEL_URC;
            break;

        case RIL_OEM_HOOK_STRING_CNAP_GET_CURRENT_STATE:
            {
                RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_CNAP_GET_CURRENT_STATE");
                // Send this command on ATCMD channel to avoid collision with +CNAP URC
                uiRilChannel = RIL_CHANNEL_ATCMD;
                res = m_cte.GetCnapState(rReqData);
                break;
            }

        case RIL_OEM_HOOK_STRING_ADPCLK_ACTIVATE:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_ADPCLK_ACTIVATE");
            res = m_cte.CreateSetAdaptiveClockingReq(rReqData, (const char**) pszRequest,
                    uiDataSize);
            // Send this command on URC channel to get the notifications in URC channel
            uiRilChannel = RIL_CHANNEL_URC;
            break;

        case RIL_OEM_HOOK_STRING_ADPCLK_GET_FREQ_INFO:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_ADPCLK_GET_FREQ_INFO");
            res = m_cte.CreateGetAdaptiveClockingFreqInfo(rReqData, (const char**) pszRequest,
                    uiDataSize);
            //  Send this command on OEM channel.
            uiRilChannel = RIL_CHANNEL_OEM;
            break;

        case RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_REPORT:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_REPORT");
            res = m_cte.CreateSetRegStatusAndBandReport(rReqData, (const char**) pszRequest,
                    uiDataSize);
            uiRilChannel = RIL_CHANNEL_URC;
            break;

        case RIL_OEM_HOOK_STRING_SET_COEX_REPORT:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_COEX_REPORT");
            res = m_cte.CreateSetCoexReport(rReqData, (const char**) pszRequest,
                    uiDataSize);
            // Send this command on DLC23 dedicated to RF coexistence
            uiRilChannel = RIL_CHANNEL_DLC23;
            break;

        case RIL_OEM_HOOK_STRING_SET_COEX_WLAN_PARAMS:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_COEX_WLAN_PARAMS");
            res = m_cte.CreateSetCoexWlanParams(rReqData, (const char**) pszRequest,
                    uiDataSize);
            // Send this command on DLC23 dedicated to RF coexistence
            uiRilChannel = RIL_CHANNEL_DLC23;
            break;

        case RIL_OEM_HOOK_STRING_SET_COEX_BT_PARAMS:
            RIL_LOG_INFO("Received Commmand: RIL_OEM_HOOK_STRING_SET_COEX_BT_PARAMS");
            res = m_cte.CreateSetCoexBtParams(rReqData, (const char**) pszRequest,
                    uiDataSize);
            // Send this command on DLC23 dedicated to RF coexistence
            uiRilChannel = RIL_CHANNEL_DLC23;
            break;

        default:
            RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() -"
                    " ERROR: Received unknown command=[0x%x]\r\n", command);
            goto Error;
    }

    if (RRIL_RESULT_OK != res && RRIL_RESULT_OK_IMMEDIATE != res)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreHookStrings() :"
                " Can't create OEM HOOK String request command: 0x%x", command);
        goto Error;
    }

    rReqData.pContextData = (void*)(intptr_t)command;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreHookStrings() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseHookStrings(RESPONSE_DATA & rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseHookStrings() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    int command = (intptr_t)rRspData.pContextData;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseHookStrings() - Response string is NULL!\r\n");
        goto Error;
    }

    // Skip "<prefix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseHookStrings() - Could not skip response prefix.\r\n");
        goto Error;
    }

    switch(command)
    {
        case RIL_OEM_HOOK_STRING_GET_ATR:
            res = ParseXGATR(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR:
        case RIL_OEM_HOOK_STRING_THERMAL_SET_THRESHOLD:
            res = ParseXDRV(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_GET_GPRS_CELL_ENV:
            res = ParseCGED(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_DEBUG_SCREEN_COMMAND:
            res = ParseXCGEDPAGE(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_GET_SMS_TRANSPORT_MODE:
            res = ParseCGSMS(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_GET_RF_POWER_CUTBACK_TABLE:
            res = ParseXRFCBT(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_IMS_SRVCC_PARAM:
            res = ParseXISRVCC(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_SEND_AT:
            res = HandleSendAtResponse(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_GET_DVP_STATE:
            res = ParseXDVP(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_CSG_GET_CURRENT_CSG_STATE:
            res = m_cte.ParseXCSG(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_ADPCLK_GET_FREQ_INFO:
            res = m_cte.ParseGetAdaptiveClockingFreqInfo(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_SET_MODEM_AUTO_FAST_DORMANCY:
        case RIL_OEM_HOOK_STRING_RELEASE_ALL_CALLS:
        case RIL_OEM_HOOK_STRING_SET_SMS_TRANSPORT_MODE:
        case RIL_OEM_HOOK_STRING_SET_RF_POWER_CUTBACK_TABLE:
        case RIL_OEM_HOOK_STRING_IMS_REGISTRATION:
        case RIL_OEM_HOOK_STRING_IMS_CONFIG:
        case RIL_OEM_HOOK_STRING_IMS_CALL_STATUS:
        case RIL_OEM_HOOK_STRING_IMS_SMS_STATUS:
        case RIL_OEM_HOOK_STRING_CSG_SET_AUTOMATIC_SELECTION:
        case RIL_OEM_HOOK_STRING_SET_DEFAULT_APN:
        case RIL_OEM_HOOK_STRING_NOTIFY_RELEASE_MODEM:
        case RIL_OEM_HOOK_STRING_SWAP_PS:
        case RIL_OEM_HOOK_STRING_SIM_RESET:
        case RIL_OEM_HOOK_STRING_SET_DVP_ENABLED:
        case RIL_OEM_HOOK_STRING_ADPCLK_ACTIVATE:
        case RIL_OEM_HOOK_STRING_SET_COEX_REPORT:
        case RIL_OEM_HOOK_STRING_SET_COEX_WLAN_PARAMS:
        case RIL_OEM_HOOK_STRING_SET_COEX_BT_PARAMS:
            // no need for a parse function as this AT command only returns "OK"
            res = RRIL_RESULT_OK;
            break;
        case RIL_OEM_HOOK_STRING_CNAP_GET_CURRENT_STATE:
            res = m_cte.ParseQueryCnap(pszRsp, rRspData);
            break;

        case RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_IND:
        case RIL_OEM_HOOK_STRING_SET_REG_STATUS_AND_BAND_REPORT:
            res = ParseRegStatusAndBandInd(pszRsp, rRspData);
            break;

        default:
            RIL_LOG_INFO("CTE_XMM6260::ParseHookStrings() -"
                    " Parsing not implemented for command: %d\r\n", command);
            break;
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseHookStrings() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_BAND_MODE
//
RIL_RESULT_CODE CTE_XMM6260::CoreSetBandMode(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSetBandMode() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32* pnBandMode;

    if (sizeof(int*) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetBandMode() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetBandMode() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pnBandMode = (UINT32*)pData;

    switch(*pnBandMode)
    {
    case 0:
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XBANDSEL=0\r"))
        {
            res = RRIL_RESULT_OK;
        }
        break;

    case 1:
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XBANDSEL=900\r"))
        {
            res = RRIL_RESULT_OK;
        }
        break;

    case 2:
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+XBANDSEL=850,1900\r"))
        {
            res = RRIL_RESULT_OK;
        }
        break;

    case 3:
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetBandMode() - Japan region is not supported!\r\n");
        res = RIL_E_GENERIC_FAILURE;
        break;

    case 4:
    case 5:
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+XBANDSEL=850,900,1800\r"))
        {
            res = RRIL_RESULT_OK;
        }
        break;

    default:
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetBandMode() -"
                " Undefined region code: %d\r\n", *pnBandMode);
        res = RIL_E_GENERIC_FAILURE;
        break;
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSetBandMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseSetBandMode(RESPONSE_DATA & /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSetBandMode() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSetBandMode() - Exit\r\n");
    return res;
}


//
// RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE
//
RIL_RESULT_CODE CTE_XMM6260::CoreQueryAvailableBandMode(REQUEST_DATA& rReqData,
                                                                    void* /*pData*/,
                                                                    UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreQueryAvailableBandMode() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XBANDSEL=?\r"))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreQueryAvailableBandMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseQueryAvailableBandMode(RESPONSE_DATA & rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryAvailableBandMode() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    const char* szRsp = rRspData.szResponse;
    const char* szDummy = NULL;

    BOOL automatic = FALSE;
    BOOL euro = FALSE;
    BOOL usa = FALSE;
    BOOL japan = FALSE;
    BOOL aus = FALSE;
    BOOL aus2 = FALSE;
    UINT32 count = 0, count2 = 0;

    int* pModes = NULL;

    // Skip "+XBANDSEL: "
    if (!FindAndSkipString(szRsp, "+XBANDSEL: ", szRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryAvailableBandMode() -"
                " Could not skip \"+XBANDSEL: \".\r\n");
        goto Error;
    }

    // Check for automatic
    if (FindAndSkipString(szRsp, "(0", szDummy) ||
        FindAndSkipString(szRsp, ",0,", szDummy) ||
        FindAndSkipString(szRsp, ",0)", szDummy))
    {
        automatic = TRUE;
    }

    // Check for 800 Mhz
    if (FindAndSkipString(szRsp, "(800", szDummy) ||
        FindAndSkipString(szRsp, ",800,", szDummy) ||
        FindAndSkipString(szRsp, ",800)", szDummy))
    {
        japan = TRUE;
    }

    // Check for 850 Mhz
    if (FindAndSkipString(szRsp, "(850", szDummy) ||
        FindAndSkipString(szRsp, ",850,", szDummy) ||
        FindAndSkipString(szRsp, ",850)", szDummy))
    {
        usa = TRUE;
        aus = TRUE;
        aus2 = TRUE;
    }

    // Check for 900 Mhz
    if (FindAndSkipString(szRsp, "(900", szDummy) ||
        FindAndSkipString(szRsp, ",900,", szDummy) ||
        FindAndSkipString(szRsp, ",900)", szDummy))
    {
        euro = TRUE;
        aus = TRUE;
        aus2 = TRUE;
    }

    // Check for 1800 Mhz
    if (FindAndSkipString(szRsp, "(1800", szDummy) ||
        FindAndSkipString(szRsp, ",1800,", szDummy) ||
        FindAndSkipString(szRsp, ",1800)", szDummy))
    {
        euro = TRUE;
        aus = TRUE;
        aus2 = TRUE;
    }

    // Check for 1900 Mhz
    if (FindAndSkipString(szRsp, "(1900", szDummy) ||
        FindAndSkipString(szRsp, ",1900,", szDummy) ||
        FindAndSkipString(szRsp, ",1900)", szDummy))
    {
        usa = TRUE;
    }

    // Check for 2000 Mhz
    if (FindAndSkipString(szRsp, "(2000", szDummy) ||
        FindAndSkipString(szRsp, ",2000,", szDummy) ||
        FindAndSkipString(szRsp, ",2000)", szDummy))
    {
        japan = TRUE;
        euro = TRUE;
        aus = TRUE;
    }

    // pModes is an array of ints where the first is the number of ints to follow
    // then an int representing each available band mode where:
    /* 0 for "unspecified" (selected by baseband automatically)
    *  1 for "EURO band" (GSM-900 / DCS-1800 / WCDMA-IMT-2000)
    *  2 for "US band" (GSM-850 / PCS-1900 / WCDMA-850 / WCDMA-PCS-1900)
    *  3 for "JPN band" (WCDMA-800 / WCDMA-IMT-2000)
    *  4 for "AUS band" (GSM-900 / DCS-1800 / WCDMA-850 / WCDMA-IMT-2000)
    *  5 for "AUS band 2" (GSM-900 / DCS-1800 / WCDMA-850)
    */

    //  Get total count
    if (automatic)
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseQueryAvailableBandMode() - automatic\r\n");
        count++;
    }

    if (euro)
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseQueryAvailableBandMode() - euro\r\n");
        count++;
    }

    if (usa)
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseQueryAvailableBandMode() - usa\r\n");
        count++;
    }

    if (japan)
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseQueryAvailableBandMode() - japan\r\n");
        count++;
    }

    if (aus)
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseQueryAvailableBandMode() - aus\r\n");
        count++;
    }

    if (aus2)
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseQueryAvailableBandMode() - aus2\r\n");
        count++;
    }

    RIL_LOG_INFO("CTE_XMM6260::ParseQueryAvailableBandMode() - count=%d\r\n", count);

    if (0 == count)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryAvailableBandMode() -"
                " Cannot have a count of zero.\r\n");
        goto Error;
    }

    //  Alloc array and don't forget about the first entry for size.
    pModes = (int*)malloc( (1 + count) * sizeof(int));
    if (NULL == pModes)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryAvailableBandMode() -"
                " Could not allocate memory for response.\r\n");
        goto Error;
    }
    memset(pModes, 0, (1 + count) * sizeof(int));

    pModes[0] = count + 1;

    //  Go assign bands in array.
    count2 = 1;
    if (automatic)
    {
        pModes[count2] = 0;
        count2++;
    }

    if (euro)
    {
        pModes[count2] = 1;
        count2++;
    }

    if (usa)
    {
        pModes[count2] = 2;
        count2++;
    }

    if (japan)
    {
        pModes[count2] = 3;
        count2++;
    }

    if (aus)
    {
        pModes[count2] = 4;
        count2++;
    }

    if (aus2)
    {
        pModes[count2] = 5;
        count2++;
    }


    res = RRIL_RESULT_OK;

    rRspData.pData   = (void*)pModes;
    rRspData.uiDataSize  = sizeof(int) * (count + 1);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pModes);
        pModes = NULL;
    }


    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryAvailableBandMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_GET_PROFILE
//
RIL_RESULT_CODE CTE_XMM6260::CoreStkGetProfile(REQUEST_DATA& rReqData,
                                                           void* /*pData*/,
                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_INFO("CTE_XMM6260::CoreStkGetProfile() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+STKPROF?\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_INFO("CTE_XMM6260::CoreStkGetProfile() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseStkGetProfile(RESPONSE_DATA & rRspData)
{
    RIL_LOG_INFO("CTE_XMM6260::ParseStkGetProfile() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    char* pszTermProfile = NULL;
    UINT32 uiLength = 0;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkGetProfile() - Response string is NULL!\r\n");
        goto Error;
    }

    pszTermProfile = (char*)malloc(MAX_BUFFER_SIZE);
    if (NULL == pszTermProfile)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkGetProfile() -"
                " Could not allocate memory for a %u-char string.\r\n", MAX_BUFFER_SIZE);
        goto Error;
    }

    memset(pszTermProfile, 0x00, MAX_BUFFER_SIZE);

    // Parse "<prefix>+STKPROF: <length>,<data><postfix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkGetProfile() -"
                " Could not skip response prefix.\r\n");
        goto Error;
    }

    if (!SkipString(pszRsp, "+STKPROF: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkGetProfile() -"
                " Could not skip \"+STKPROF: \".\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, uiLength, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkGetProfile() -"
                " Could not extract length value.\r\n");
        goto Error;
    }

    if (SkipString(pszRsp, ",", pszRsp))
    {
        if (!ExtractQuotedString(pszRsp, pszTermProfile, MAX_BUFFER_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkGetProfile() -"
                    " Could not extract the terminal profile.\r\n");
            goto Error;
        }
    }

    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkGetProfile() -"
                " Could not extract the response end.\r\n");
        goto Error;
    }

    rRspData.pData   = (void*)pszTermProfile;
    rRspData.uiDataSize  = uiLength;

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pszTermProfile);
        pszTermProfile = NULL;
    }

    RIL_LOG_INFO("CTE_XMM6260::ParseStkGetProfile() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SET_PROFILE
//
RIL_RESULT_CODE CTE_XMM6260::CoreStkSetProfile(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize)
{
    RIL_LOG_INFO("CTE_XMM6260::CoreStkSetProfile() - Enter\r\n");
    char* pszTermProfile = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (0 == uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSetProfile() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSetProfile() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    // extract data
    pszTermProfile = (char*)pData;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+STKPROF=%u,\"%s\"\r", uiDataSize, pszTermProfile))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSetProfile() - Could not form string.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_INFO("CTE_XMM6260::CoreStkSetProfile() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseStkSetProfile(RESPONSE_DATA & /*rRspData*/)
{
    RIL_LOG_INFO("CTE_XMM6260::ParseStkSetProfile() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_INFO("CTE_XMM6260::ParseStkSetProfile() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
//
RIL_RESULT_CODE CTE_XMM6260::CoreStkSendEnvelopeCommand(REQUEST_DATA& rReqData,
                                                                    void* pData,
                                                                    UINT32 uiDataSize)
{
    RIL_LOG_INFO("CTE_XMM6260::CoreStkSendEnvelopeCommand() - Enter\r\n");
    char* pszEnvCommand = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (sizeof(char*) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSendEnvelopeCommand() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSendEnvelopeCommand() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    // extract data
    pszEnvCommand = (char*)pData;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+SATE=\"%s\"\r", pszEnvCommand))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSendEnvelopeCommand() -"
                " Could not form string.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_INFO("CTE_XMM6260::CoreStkSendEnvelopeCommand() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseStkSendEnvelopeCommand(RESPONSE_DATA & rRspData)
{
    RIL_LOG_INFO("CTE_XMM6260::ParseStkSendEnvelopeCommand() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiSw1 = 0, uiSw2 = 0;
    UINT32 uiEventType = 0;
    UINT32 uiEnvelopeType = 0;
    char* pszRespData = NULL;

    const char* pszRsp = rRspData.szResponse;
    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                " Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "<prefix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                " Could not skip over response prefix.\r\n");
        goto Error;
    }

    // Parse "+SATE: "
    if (!SkipString(pszRsp, "+SATE: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                " Could not skip over \"+SATE: \".\r\n");
        goto Error;
    }

    // Parse "<sw1>"
    if (!ExtractUInt32(pszRsp, uiSw1, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                " Could not extract sw1.\r\n");
        goto Error;
    }

    // Parse ",<sw2>"
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiSw2, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                " Could not extract sw2.\r\n");
        goto Error;
    }

    // Parse ",<event_type>"
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiEventType, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                " Could not extract event type.\r\n");
        goto Error;
    }

    // Parse ",<envelope_type>"
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiEnvelopeType, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                " Could not extract envelope type.\r\n");
        goto Error;
    }

    RIL_LOG_INFO(" sw1: %u, sw2: %u\r\n", uiSw1, uiSw2);
    RIL_LOG_INFO(" event type: %u\r\n", uiEventType);
    RIL_LOG_INFO(" envelope type: %u\r\n", uiEnvelopeType);

    // Parse "," if response data exists
    if (SkipString(pszRsp, ",", pszRsp))
    {
        pszRespData = (char*)malloc(MAX_BUFFER_SIZE);
        if (NULL == pszRespData)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                    " Could not alloc mem for command.\r\n");
            goto Error;
        }

        memset(pszRespData, 0x00, MAX_BUFFER_SIZE);

        // Parse ",<response_data>"
        if (!ExtractUnquotedString(pszRsp, m_cTerminator, pszRespData, MAX_BUFFER_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                    " Could not parse response data.\r\n");
            goto Error;
        }

        RIL_LOG_INFO("CTE_XMM6260::ParseStkSendEnvelopeCommand() - response data: \"%s\".\r\n",
                pszRespData);
    }
    else
    {
       pszRespData = NULL;
    }

    // Parse "<postfix>"
    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeCommand() -"
                " Could not extract the response end.\r\n");
        goto Error;
    }

    rRspData.pData   = (void*)pszRespData;
    rRspData.uiDataSize  = sizeof(char*);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pszRespData);
        pszRespData = NULL;
    }

    RIL_LOG_INFO("CTE_XMM6260::ParseStkSendEnvelopeCommand() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
//
RIL_RESULT_CODE CTE_XMM6260::CoreStkSendTerminalResponse(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize)
{
    RIL_LOG_INFO("CTE_XMM6260::CoreStkSendTerminalResponse() - Enter\r\n");
    char* pszTermResponse = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSendTerminalResponse() -"
                " Data pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != sizeof(char *))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSendTerminalResponse() - Invalid data size.\r\n");
        goto Error;
    }

    RIL_LOG_INFO(" uiDataSize= %d\r\n", uiDataSize);

    pszTermResponse = (char *)pData;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+SATR=\"%s\"\r", pszTermResponse))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkSendTerminalResponse() -"
                " Could not form string.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_INFO("CTE_XMM6260::CoreStkSendTerminalResponse() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseStkSendTerminalResponse(RESPONSE_DATA & /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseStkSendTerminalResponse() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseStkSendTerminalResponse() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM
//
RIL_RESULT_CODE CTE_XMM6260::CoreStkHandleCallSetupRequestedFromSim(REQUEST_DATA& rReqData,
                                                                               void* pData,
                                                                               UINT32 uiDataSize)
{
    RIL_LOG_INFO("CTE_XMM6260::CoreStkHandleCallSetupRequestedFromSim() - Enter\r\n");
    int nConfirmation = 0;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkHandleCallSetupRequestedFromSim() -"
                " Data pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != sizeof(int *))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkHandleCallSetupRequestedFromSim() -"
                " Invalid data size.\r\n");
        goto Error;
    }

    nConfirmation = ((int*)pData)[0];
    if (0 == nConfirmation)
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+SATD=0\r"))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkHandleCallSetupRequestedFromSim() -"
                    " Could not form string.\r\n");
            goto Error;
        }
    }
    else
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+SATD=1\r"))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CoreStkHandleCallSetupRequestedFromSim() -"
                    " Could not form string.\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_INFO("CTE_XMM6260::CoreStkHandleCallSetupRequestedFromSim() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseStkHandleCallSetupRequestedFromSim(RESPONSE_DATA & /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseStkHandleCallSetupRequestedFromSim() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseStkHandleCallSetupRequestedFromSim() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
//
RIL_RESULT_CODE CTE_XMM6260::CoreSetPreferredNetworkType(REQUEST_DATA& rReqData,
                                                                     void* pData,
                                                                     UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSetPreferredNetworkType() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_PreferredNetworkType networkType = PREF_NET_TYPE_GSM_WCDMA; // 0

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetPreferredNetworkType() -"
                " Data pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != sizeof(RIL_PreferredNetworkType*))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetPreferredNetworkType() -"
                " Invalid data size.\r\n");
        goto Error;
    }

    networkType = ((RIL_PreferredNetworkType*)pData)[0];

    // if network type already set, NO-OP this command
    if (m_currentNetworkType == networkType)
    {
        rReqData.szCmd1[0] = '\0';
        res = RRIL_RESULT_OK_IMMEDIATE;
        RIL_LOG_INFO("CTE_XMM6260::CoreSetPreferredNetworkType() - Network type {%d} "
                "already set.\r\n", networkType);
        goto Error;
    }

    switch (networkType)
    {
        case PREF_NET_TYPE_GSM_WCDMA: // WCDMA Preferred

            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XRAT=1,2\r", sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetPreferredNetworkType() - Can't "
                    "construct szCmd1 networkType=%d\r\n", networkType);
                goto Error;
            }

            break;

        case PREF_NET_TYPE_GSM_ONLY: // GSM Only

            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XRAT=0\r", sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetPreferredNetworkType() - Can't "
                    "construct szCmd1 networkType=%d\r\n", networkType);
                goto Error;
            }

            break;

        case PREF_NET_TYPE_WCDMA: // WCDMA Only

            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XRAT=2\r", sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetPreferredNetworkType() - Can't "
                    "construct szCmd1 networkType=%d\r\n", networkType);
                goto Error;
            }

            break;

        // This value is received as a result of the recovery mechanism in the framework even
        // though not supported by modem.  In this case, set to supported default value of
        // PREF_NET_TYPE_GSM_WCDMA.
        case PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO:

            RIL_LOG_INFO("CTE_XMM6260::CoreSetPreferredNetworkType() - Unsupported rat type "
                "%d, changing to %d\r\n", networkType, PREF_NET_TYPE_GSM_WCDMA);

            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XRAT=1,2\r", sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetPreferredNetworkType() - Can't "
                        "construct szCmd1 networkType=%d\r\n", networkType);
                goto Error;
            }

            break;

        default:
            RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetPreferredNetworkType() - Undefined rat "
                    "code: %d\r\n", networkType);
            res = RIL_E_MODE_NOT_SUPPORTED;
            goto Error;
            break;
    }

    //  Set the context of this command to the network type we're attempting to set
    rReqData.pContextData = (void*)networkType;  // Store this as an int.

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreSetPreferredNetworkType() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseSetPreferredNetworkType(RESPONSE_DATA & rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSetPreferredNetworkType() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_PreferredNetworkType networkType =
            (RIL_PreferredNetworkType)((intptr_t)rRspData.pContextData);
    m_currentNetworkType = networkType;

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSetPreferredNetworkType() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
//
RIL_RESULT_CODE CTE_XMM6260::CoreGetPreferredNetworkType(REQUEST_DATA& rReqData,
                                                                     void* /*pData*/,
                                                                     UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreGetPreferredNetworkType() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+XRAT?\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::CoreGetPreferredNetworkType() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseGetPreferredNetworkType(RESPONSE_DATA & rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseGetPreferredNetworkType() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    UINT32 rat = 0;
    UINT32 pref = 0;

    int* pRat = (int*)malloc(sizeof(int));
    if (NULL == pRat)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseGetPreferredNetworkType() -"
                " Could not allocate memory for response.\r\n");
        goto Error;
    }

    // Skip "<prefix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseGetPreferredNetworkType() -"
                " Could not skip response prefix.\r\n");
        goto Error;
    }

    // Skip "+XRAT: "
    if (!SkipString(pszRsp, "+XRAT: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseGetPreferredNetworkType() -"
                " Could not skip \"+XRAT: \".\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, rat, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseGetPreferredNetworkType() -"
                " Could not extract rat value.\r\n");
        goto Error;
    }

    if (FindAndSkipString(pszRsp, ",", pszRsp))
    {
        if (!ExtractUInt32(pszRsp, pref, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseGetPreferredNetworkType() -"
                    " Could not find and skip pref value even though it was expected.\r\n");
            goto Error;
        }
    }

    switch(rat)
    {
        case 0:     // GSM Only
        {
            pRat[0] = PREF_NET_TYPE_GSM_ONLY;
            m_currentNetworkType = PREF_NET_TYPE_GSM_ONLY;
            break;
        }

        case 1:     // WCDMA Preferred
        {
            pRat[0] = PREF_NET_TYPE_GSM_WCDMA;
            m_currentNetworkType = PREF_NET_TYPE_GSM_WCDMA;
            break;
        }

        case 2:     // WCDMA only
        {
            pRat[0] = PREF_NET_TYPE_WCDMA;
            m_currentNetworkType = PREF_NET_TYPE_WCDMA;
            break;
        }

        default:
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseGetPreferredNetworkType() -"
                    " Unexpected rat found: %d. Failing out.\r\n", rat);
            goto Error;
        }
    }

    rRspData.pData  = (void*)pRat;
    rRspData.uiDataSize = sizeof(int*);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pRat);
        pRat = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseGetPreferredNetworkType() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_NEIGHBORING_CELL_IDS
//
RIL_RESULT_CODE CTE_XMM6260::CoreGetNeighboringCellIDs(REQUEST_DATA& rReqData,
                                                                   void* /*pData*/,
                                                                   UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreGetNeighboringCellIDs() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+XCELLINFO?\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::CoreGetNeighboringCellIDs() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseNeighboringCellInfo(P_ND_N_CELL_DATA pCellData,
                                                    const char* pszRsp,
                                                    UINT32 uiIndex,
                                                    UINT32 uiMode)
{
    RIL_RESULT_CODE res = RIL_E_GENERIC_FAILURE;
    UINT32 uiLAC = 0, uiCI = 0, uiRSSI = 0, uiScramblingCode = 0;
    const char* pszStart = pszRsp;

    //  GSM cells:
    //  +XCELLINFO: 0,<MCC>,<MNC>,<LAC>,<CI>,<RxLev>,<BSIC>,<BCCH_Car>,<true_freq>,<t_advance>
    //  +XCELLINFO: 1,<LAC>,<CI>,<RxLev>,<BSIC>,<BCCH_Car>
    //  one row for each neighboring cell [0..6]
    //  For GSM cells, according to ril.h, must return (LAC/CID , received RSSI)
    //
    //  UMTS FDD cells:
    //  +XCELLINFO: 2,<MCC>,<MNC>,<LAC>,<UCI>,<scrambling_code>,<dl_frequency>,<ul_frequency>
    //  +XCELLINFO: 2,<scrambling_code>,<dl_frequency>,<UTRA_rssi>,<rscp>,<ecno>,<pathloss>
    // If UMTS has any ACTIVE SET neighboring cell
    //  +XCELLINFO: 3,<scrambling_code>,<dl_frequency>,<UTRA_rssi>,<rscp>,<ecno>,<pathloss>
    // One row
    //                          // for each intra-frequency neighboring cell [1..32] for each
    //                          // frequency [0..8] in BA list
    //  For UMTS cells, according to ril.h, must return (Primary scrambling code ,
    //  received signal code power)
    //  NOTE that for first UMTS format above, there is no <rcsp> parameter.
    //
    //  A <type> of 0 or 1 = GSM.  A <type> of 2,3 = UMTS.

    switch(uiMode)
    {
        case 0: // GSM  get (LAC/CI , RxLev)
        {
            //  <LAC> and <CI> are parameters 4 and 5
            if (!FindAndSkipString(pszRsp, ",", pszRsp) ||
                    !FindAndSkipString(pszRsp, ",", pszRsp) ||
                    !FindAndSkipString(pszRsp, ",", pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 0, cannot skip to LAC and CI\r\n");
                goto Error;
            }

            //  Read <LAC> and <CI>
            if (!ExtractUInt32(pszRsp, uiLAC, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 0, could not extract LAC\r\n");
                goto Error;
            }
            //  Read <CI>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiCI, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 0, could not extract CI value\r\n");
                goto Error;
            }
            //  Read <RxLev>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiRSSI, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 0, could not extract RSSI value\r\n");
                goto Error;
            }

            //  We now have what we want, copy to main structure.
            pCellData->aRilNeighboringCell[uiIndex].cid = pCellData->aszCellCIDBuffers[uiIndex];

            //  cid = upper 16 bits (LAC), lower 16 bits (CID)
            snprintf(pCellData->aszCellCIDBuffers[uiIndex], CELL_ID_ARRAY_LENGTH,
                    "%04X%04X", uiLAC, uiCI);
            RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                    " mode 0 GSM LAC,CID index=[%d]  cid=[%s]\r\n",
                    uiIndex, pCellData->aszCellCIDBuffers[uiIndex]);

            //  rssi = <RxLev>

            //  Convert RxLev to asu (0 to 31).
            //  For GSM, it is in "asu" ranging from 0 to 31 (dBm = -113 + 2*asu)
            //  0 means "-113 dBm or less" and 31 means "-51 dBm or greater"
            //  Divide nRSSI by 2 since rxLev = [0-63] and assume ril.h wants 0-31
            //  like AT+CSQ response.
            pCellData->aRilNeighboringCell[uiIndex].rssi = (int)(uiRSSI / 2);
            RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                    " mode 0 GSM rxlev index=[%d]  rssi=[%d]\r\n",
                    uiIndex, pCellData->aRilNeighboringCell[uiIndex].rssi);
            res = RRIL_RESULT_OK;
        }
        break;

        case 1: // GSM  get (LAC/CI , RxLev)
        {
            //  <LAC> and <CI> are parameters 2 and 3
            //  Read <LAC> and <CI>
            if (!SkipString(pszRsp, ",", pszRsp) ||
                    !ExtractUInt32(pszRsp, uiLAC, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 1, could not extract LAC\r\n");
                goto Error;
            }
            //  Read <CI>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiCI, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 1, could not extract CI value\r\n");
                goto Error;
            }
            //  Read <RxLev>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiRSSI, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 1, could not extract RSSI value\r\n");
                goto Error;
            }
            //  We now have what we want, copy to main structure.
            pCellData->aRilNeighboringCell[uiIndex].cid = pCellData->aszCellCIDBuffers[uiIndex];
            //  cid = upper 16 bits (LAC), lower 16 bits (CID)
            snprintf(pCellData->aszCellCIDBuffers[uiIndex], CELL_ID_ARRAY_LENGTH,
                    "%04X%04X", uiLAC, uiCI);
            RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                    " mode 1 GSM LAC,CID index=[%d]  cid=[%s]\r\n",
                    uiIndex, pCellData->aszCellCIDBuffers[uiIndex]);
            //  rssi = <RxLev>

            //  May have to convert RxLev to asu (0 to 31).
            //  For GSM, it is in "asu" ranging from 0 to 31 (dBm = -113 + 2*asu)
            //  0 means "-113 dBm or less" and 31 means "-51 dBm or greater"
            //  Divide nRSSI by 2 since rxLev = [0-63] and assume ril.h wants 0-31
            //  like AT+CSQ response.
            pCellData->aRilNeighboringCell[uiIndex].rssi = (int)(uiRSSI / 2);
            RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                    " mode 1 GSM rxlev index=[%d]  rssi=[%d]\r\n",
                    uiIndex, pCellData->aRilNeighboringCell[uiIndex].rssi);
            res = RRIL_RESULT_OK;
        }
        break;

        case 2: // UMTS  get (scrambling_code , rscp)
        {
            //  This can be either first case or second case.
            //  Loop and count number of commas
            char szBuf[MAX_BUFFER_SIZE] = {0};
            const char* szDummy = pszRsp;
            UINT32 uiCommaCount = 0;
            if (!ExtractUnquotedString(pszRsp, m_cTerminator, szBuf, MAX_BUFFER_SIZE, szDummy))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 2, could not extract temp buf\r\n");
                goto Error;
            }

            for (UINT32 n=0; n < strlen(szBuf); n++)
            {
                if (szBuf[n] == ',')
                    uiCommaCount++;
            }
            RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                    " mode 2, found %d commas\r\n", uiCommaCount);

            if (6 != uiCommaCount)
            {
                //  Handle first case here
                //  <scrambling_code> is parameter 6
                if (!FindAndSkipString(pszRsp, ",", pszRsp) ||
                        !FindAndSkipString(pszRsp, ",", pszRsp) ||
                        !FindAndSkipString(pszRsp, ",", pszRsp) ||
                        !FindAndSkipString(pszRsp, ",", pszRsp) ||
                        !FindAndSkipString(pszRsp, ",", pszRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                            " mode 2, could not skip to scrambling code\r\n");
                    goto Error;
                }
                if (!ExtractUInt32(pszRsp, uiScramblingCode, pszRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                            " mode 2, could not extract scrambling code\r\n");
                    goto Error;
                }
                //  Cannot get <rscp> as it does not exist.
                //  We now have what we want, copy to main structure.
                //  cid = <scrambling code> as char *
                pCellData->aRilNeighboringCell[uiIndex].cid = pCellData->aszCellCIDBuffers[uiIndex];
                snprintf(pCellData->aszCellCIDBuffers[uiIndex], CELL_ID_ARRAY_LENGTH, "%08x",
                        uiScramblingCode);

                RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 2 UMTS scramblingcode index=[%d]  cid=[%s]\r\n",
                        uiIndex, pCellData->aszCellCIDBuffers[uiIndex]);

                //  rssi = <rscp>
                //  Note that <rscp> value does not exist with this response.
                //  Set to 0 for now.
                pCellData->aRilNeighboringCell[uiIndex].rssi = 0;
                RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode 2 UMTS rscp index=[%d]  rssi=[%d]\r\n",
                        uiIndex, pCellData->aRilNeighboringCell[uiIndex].rssi);
                res = RRIL_RESULT_OK;
                break;
            }
            else
            {
                //  fall through to case 3 as it is parsed the same.
                RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " comma count = 6, drop to case 3\r\n");
            }
        }

        case 3: // UMTS  get (scrambling_code , rscp)
        {
            //  scrabling_code is parameter 2
            //  Read <scrambling_code>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiScramblingCode, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode %d, could not extract scrambling code\r\n", uiMode);
                goto Error;
            }
            //  <rscp> is parameter 5
            if (!FindAndSkipString(pszRsp, ",", pszRsp) ||
                    !FindAndSkipString(pszRsp, ",", pszRsp) ||
                    !FindAndSkipString(pszRsp, ",", pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                       " mode %d, could not skip to rscp\r\n", uiMode);
                goto Error;
            }
            //  read <rscp>
            if (!ExtractUInt32(pszRsp, uiRSSI, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                        " mode %d, could not extract rscp\r\n", uiMode);
                goto Error;
            }

            //  We now have what we want, copy to main structure.
            //  cid = <scrambling code> as char *
            pCellData->aRilNeighboringCell[uiIndex].cid = pCellData->aszCellCIDBuffers[uiIndex];
            snprintf(pCellData->aszCellCIDBuffers[uiIndex], CELL_ID_ARRAY_LENGTH, "%08x",
                    uiScramblingCode);

            RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                    " mode %d UMTS scramblingcode index=[%d]  cid=[%s]\r\n",
                    uiMode, uiIndex, pCellData->aszCellCIDBuffers[uiIndex]);

            //  rssi = <rscp>
            //  Assume that rssi value is same as <rscp> value and no conversion needs to
            //  be done.
            pCellData->aRilNeighboringCell[uiIndex].rssi = (int)uiRSSI;
            RIL_LOG_INFO("CTE_XMM6260::ParseNeighboringCellInfo() -"
                    " mode %d UMTS rscp index=[%d]  rssi=[%d]\r\n",
                    uiMode, uiIndex, pCellData->aRilNeighboringCell[uiIndex].rssi);
            res = RRIL_RESULT_OK;
        }
        break;

        default:
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseNeighboringCellInfo() -"
                    " Invalid nMode=[%d]\r\n", uiMode);
            goto Error;
        }
        break;
    }

Error:
    return res;
}

//
// RIL_REQUEST_SET_TTY_MODE
//
RIL_RESULT_CODE CTE_XMM6260::CoreSetTtyMode(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 uiDataSize)
{
    RIL_LOG_INFO("CTE_XMM6260::CoreSetTtyMode() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int nTtyMode = 0;

    if (sizeof(int) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetTtyMode() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetTtyMode() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    // extract the data
    nTtyMode = ((int*)pData)[0];

    RIL_LOG_INFO(" Set TTY mode: %d\r\n", nTtyMode);

    // check for invalid value
    if ((nTtyMode < 0) || (nTtyMode > 3))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetTtyMode() -"
                " Undefined CTM/TTY mode: %d\r\n", nTtyMode);
        res = RIL_E_GENERIC_FAILURE;
        goto Error;
    }

    //  Need to switch 2 and 3 for this modem.
    if (2 == nTtyMode)
        nTtyMode = 3;
    else if (3 == nTtyMode)
        nTtyMode = 2;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XCTMS=%d\r",
            nTtyMode))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreSetTtyMode() - Could not form string.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_INFO("CTE_XMM6260::CoreSetTtyMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseSetTtyMode(RESPONSE_DATA & /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSetTtyMode() - Enter\r\n");
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSetTtyMode() - Exit\r\n");

    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_QUERY_TTY_MODE
//
RIL_RESULT_CODE CTE_XMM6260::CoreQueryTtyMode(REQUEST_DATA& rReqData,
                                                          void* /*pData*/,
                                                          UINT32 /*uiDataSize*/)
{
    RIL_LOG_INFO("CTE_XMM6260::CoreQueryTtyMode() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+XCTMS?\r"))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_INFO("CTE_XMM6260::CoreQueryTtyMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseQueryTtyMode(RESPONSE_DATA & rRspData)
{
    RIL_LOG_INFO("CTE_XMM6260::ParseQueryTtyMode() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;
    UINT32 uiTtyMode = 0;

    int* pnMode = (int*)malloc(sizeof(int));
    if (NULL == pnMode)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryTtyMode() -"
                " Could not allocate memory for response.\r\n");
        goto Error;
    }

    // Parse prefix
    if (!FindAndSkipString(szRsp, "+XCTMS: ", szRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryTtyMode() -"
                " Unable to parse \"CTM/TTY mode: \" prefix.!\r\n");
        goto Error;
    }

    // Parse <mode>
    if (!ExtractUInt32(szRsp, uiTtyMode, szRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryTtyMode() - Unable to parse <mode>!\r\n");
        goto Error;
    }

    //  Need to switch 2 and 3 for this modem.
    if (2 == uiTtyMode)
        uiTtyMode = 3;
    else if (3 == uiTtyMode)
        uiTtyMode = 2;

    pnMode[0] = (int)uiTtyMode;

    RIL_LOG_INFO(" Current TTY mode: %d\r\n", uiTtyMode);

    rRspData.pData  = (void*)pnMode;
    rRspData.uiDataSize = sizeof(int*);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnMode);
        pnMode = NULL;
    }
    RIL_LOG_INFO("CTE_XMM6260::ParseQueryTtyMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_REPORT_SMS_MEMORY_STATUS
//
RIL_RESULT_CODE CTE_XMM6260::CoreReportSmsMemoryStatus(REQUEST_DATA& rReqData,
                                                                   void* pData,
                                                                   UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreReportSmsMemoryStatus - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    int nSmsMemoryStatus = 0;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CoreReportSmsMemoryStatus() - Data pointer is NULL\r\n");
        goto Error;
    }

    nSmsMemoryStatus = ((int *)pData)[0];

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            ((nSmsMemoryStatus == 1) ? "AT+XTESM=0\r" : "AT+XTESM=1\r")) )
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreReportSmsMemoryStatus() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseReportSmsMemoryStatus(RESPONSE_DATA & /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseReportSmsMemoryStatus() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseReportSmsMemoryStatus() - Exit\r\n");
    return res;
}


//
// RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
//
RIL_RESULT_CODE CTE_XMM6260::CoreReportStkServiceRunning(REQUEST_DATA& rReqData,
                                                                     void* /*pData*/,
                                                                     UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreReportStkServiceRunning - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XSATK=1,1\r"))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::CoreReportStkServiceRunning() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseReportStkServiceRunning(RESPONSE_DATA & rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseReportStkServiceRunning() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    UINT32 uiCmeError = 0;

    if (pszRsp == NULL)
     {
         RIL_LOG_CRITICAL("CTE_XMM6260::ParseReportStkServiceRunning() -"
                 " ERROR:NO Response string...\r\n");
         goto Error;
     }

    // Parse <prefix>
    if (SkipString(pszRsp, m_szNewLine, pszRsp))
    {
        // Search for "+CME ERROR: " after prefix
        if (SkipString(pszRsp, "+CME ERROR: ", pszRsp))
        {
            if (!ExtractUInt32(pszRsp, uiCmeError, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseReportStkServiceRunning() -"
                        " Could not find CME  argument.\r\n");
                goto Error;
            }
            else
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseReportStkServiceRunning() -"
                        " CME ERROR: %u \r\n", uiCmeError);
                goto Error;
            }
        }
    }

    if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseReportStkServiceRunning - "
                "Could not skip response postfix.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseReportStkServiceRunning() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
//
RIL_RESULT_CODE CTE_XMM6260::ParseStkSendEnvelopeWithStatus(RESPONSE_DATA & rRspData)
{
    RIL_LOG_INFO("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiSw1 = 0, uiSw2 = 0;
    UINT32 uiEventType = 0;
    UINT32 uiEnvelopeType = 0;
    char* pszRespData = NULL;
    UINT32 cbRespData = 0;
    RIL_SIM_IO_Response* pResponse = NULL;

    const char* pszRsp = rRspData.szResponse;
    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                "Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "<prefix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                "Could not skip over response prefix.\r\n");
        goto Error;
    }

    // Parse "+SATE: "
    if (!SkipString(pszRsp, "+SATE: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                "Could not skip over \"+SATE: \".\r\n");
        goto Error;
    }

    // Parse "<sw1>"
    if (!ExtractUInt32(pszRsp, uiSw1, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                "Could not extract sw1.\r\n");
        goto Error;
    }

    // Parse ",<sw2>"
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiSw2, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                "Could not extract sw2.\r\n");
        goto Error;
    }

    // Parse ",<event_type>"
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiEventType, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                "Could not extract event type.\r\n");
        goto Error;
    }

    // Parse ",<envelope_type>"
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiEnvelopeType, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                "Could not extract envelope type.\r\n");
        goto Error;
    }

    RIL_LOG_INFO(" sw1: %u, sw2: %u, event type: %u, envelope type: %u\r\n",
            uiSw1, uiSw2, uiEventType, uiEnvelopeType);

    // Parse "," if response data exists
    if (SkipString(pszRsp, ",", pszRsp))
    {
        // Parse ",<response_data>"
        // NOTE: we take ownership of allocated pszRespData
        if (!ExtractUnquotedStringWithAllocatedMemory(pszRsp, m_cTerminator, pszRespData,
                cbRespData, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                    "Could not parse response data.\r\n");
            goto Error;
        }

        RIL_LOG_INFO("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() - response data: \"%s\".\r\n",
                pszRespData);

        // Allocate memory for the RIL_SIM_IO_Response struct + sim response string.
        // The char* in the RIL_SIM_IO_Response will point to the buffer allocated
        // directly after the RIL_SIM_IO_Response.  When the RIL_SIM_IO_Response
        // is deleted, the corresponding response string will be freed as well.
        pResponse = (RIL_SIM_IO_Response*)malloc(sizeof(RIL_SIM_IO_Response) + cbRespData + 1);
        if (NULL == pResponse)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                    "Could not allocate memory for a RIL_SIM_IO_Response struct.\r\n");
            goto Error;
        }
        memset(pResponse, 0, sizeof(RIL_SIM_IO_Response) + cbRespData + 1);

        if (NULL == pszRespData)
        {
            pResponse->simResponse = NULL;
        }
        else
        {
            // set location to copy sim response string just after RIL_SIM_IO_Response
            pResponse->simResponse = (char*)(((char*)pResponse) + sizeof(RIL_SIM_IO_Response));

            if (!CopyStringNullTerminate(pResponse->simResponse, pszRespData, cbRespData))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                        "Cannot CopyStringNullTerminate szResponseString\r\n");
                goto Error;
            }

            // Ensure NULL termination!
            pResponse->simResponse[cbRespData] = '\0';
        }
    }
    else
    {
        pResponse = (RIL_SIM_IO_Response*)malloc(sizeof(RIL_SIM_IO_Response));
        if (NULL == pResponse)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                    "Could not allocate memory for a RIL_SIM_IO_Response struct.\r\n");
            goto Error;
        }
        pResponse->simResponse = NULL;
    }

    pResponse->sw1 = uiSw1;
    pResponse->sw2 = uiSw2;

    // Parse "<postfix>"
    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() -"
                "Could not extract the response end.\r\n");
        goto Error;
    }

    rRspData.pData   = (void*)pResponse;
    rRspData.uiDataSize  = sizeof(RIL_SIM_IO_Response);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    delete[] pszRespData;
    pszRespData = NULL;

    RIL_LOG_INFO("CTE_XMM6260::ParseStkSendEnvelopeWithStatus() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::CreateGetThermalSensorValuesReq(REQUEST_DATA& reqData,
                                                             const char** ppszRequest,
                                                             const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateGetThermalSensorValuesReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int sensorId;

    if (uiDataSize < (2 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateGetThermalSensorValuesReq() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    if (sscanf(ppszRequest[1], "%d", &sensorId) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateGetThermalSensorReq() -"
                " cannot convert %s to int\r\n", ppszRequest[1]);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CreateGetThermalSensorValuesReq() - sensorId=[%d]\r\n", sensorId);

    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
            "AT+XDRV=5,9,%d\r", sensorId))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateGetThermalSensorValuesReq() -"
                " Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateGetThermalSensorValuesReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::CreateActivateThermalSensorInd(REQUEST_DATA& reqData,
                                                        const char** ppszRequest,
                                                        const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateActivateThermalSensorInd() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int sensorId = 0;
    int minThersholdTemp = 0;
    int maxThersholdTemp = 0;
    char szActivate[10] = {0};
    int noOfVariablesFilled = 0;
    const int MAX_NUM_OF_INPUT_DATA = 4;

    if (uiDataSize < (2 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateActivateThermalSensorInd() :"
                " received data size is not enough to process the request\r\n");
        goto Error;
    }

    noOfVariablesFilled = sscanf(ppszRequest[1], "%s %d %d %d", szActivate, &sensorId,
            &minThersholdTemp, &maxThersholdTemp);
    if (noOfVariablesFilled < MAX_NUM_OF_INPUT_DATA || noOfVariablesFilled == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateActivateThermalSensorInd() -"
                " Issue with received input data: %d\r\n", noOfVariablesFilled);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CreateActivateThermalSensorInd() - szActivate=[%s],"
                 " sensor Id=[%d], Low Threshold=[%d], Max Threshold=[%d]\r\n",
                 szActivate, sensorId, minThersholdTemp, maxThersholdTemp);

    /*
     * For activating the thermal sensor threshold reached indication, threshold
     * temperatures(minimum,maximum) needs to be sent as part of the set command.
     */
    if (strcmp(szActivate, "true") == 0)
    {
        if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                "AT+XDRV=5,14,%d,%d,%d\r",
                sensorId, minThersholdTemp, maxThersholdTemp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateActivateThermalSensorInd() -"
                   " Can't construct szCmd1.\r\n");
            goto Error;
        }
    }
    else
    {
        if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                "AT+XDRV=5,14,%d\r", sensorId))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateActivateThermalSensorInd() -"
                    " Can't construct szCmd1.\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateActivateThermalSensorInd() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::CreateAutonomousFDReq(REQUEST_DATA& rReqData,
                                                        const char** pszRequest,
                                                        const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateAutonomousFDReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int noOfVariablesFilled = 0;
    const int MAX_NUM_OF_INPUT_DATA = 3;
    char szFDEnable[10] = {0};
    char szDelayTimer[3] = {0};
    char szSCRITimer[3] = {0};

    if (pszRequest == NULL || '\0' == pszRequest[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateAutonomousFDReq() - pszRequest was NULL\r\n");
        goto Error;
    }

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateAutonomousFDReq() : received data size is not enough"
                " to process the request\r\n");
        goto Error;
    }

    noOfVariablesFilled = sscanf(pszRequest[1], "%s %s %s", szFDEnable, szDelayTimer, szSCRITimer);
    if (noOfVariablesFilled < MAX_NUM_OF_INPUT_DATA || noOfVariablesFilled == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateAutonomousFDReq() -"
                " Issue with received input data: %d\r\n", noOfVariablesFilled);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CreateAutonomousFDReq() - Activate=[%s], Delay Timer = [%s],"
            "SCRI Timer = [%s]\r\n", szFDEnable, szDelayTimer, szSCRITimer);

    if (strcmp(szDelayTimer, "0") == 0) strcpy(szDelayTimer, "");
    if (strcmp(szSCRITimer, "0") == 0) strcpy(szSCRITimer, "");

    if (strcmp(szFDEnable, "true") == 0)
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+XFDOR=2,%s,%s\r", szDelayTimer, szSCRITimer))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateAutonomousFDReq -"
                    " Can't construct szCmd1. 2\r\n");
            goto Error;
        }

    }
    else
    {
        if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XFDOR=3\r", sizeof(rReqData.szCmd1)))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateAutonomousFDReq -"
                    " Can't construct szCmd1. 3\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateAutonomousFDReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::CreateDebugScreenReq(REQUEST_DATA& rReqData,
                                                    const char** pszRequest,
                                                    const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateDebugScreenReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const int MAX_NUM_OF_RESPONSE_PAGES = 12;
    int mode = E_MODE_ONE_SHOT_DUMP;
    int page_nr = 1; // Number of response pages

    if (pszRequest == NULL || '\0' == pszRequest[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateDebugScreenReq() - pszRequest was NULL\r\n");
        goto Error;
    }

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_INFO("CTE_XMM6260::CreateDebugScreenReq() - mode and  page_nr not provided, "
                "default to mode = 0 and page_nr = 1\r\n");
    }
    else if (uiDataSize < (3 * sizeof(char*)))
    {
        RIL_LOG_INFO("CTE_XMM6260::CreateDebugScreenReq() - page_nr is not provided, "
                "default to page_nr = 1\r\n");

        // Get mode
        if (sscanf(pszRequest[1], "%d", &mode) == EOF)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateDebugScreenReq() - cannot convert %s to int\r\n",
                    pszRequest[1]);
            goto Error;
        }
    }
    else
    {
        // Get mode and page_nr
        if (sscanf(pszRequest[1], "%d", &mode) == EOF)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateDebugScreenReq() - cannot convert %s to int\r\n",
                    pszRequest[1]);
            goto Error;
        }

        if (E_MODE_ONE_SHOT_DUMP != mode
                && E_MODE_RESET_STATISTICS != mode
                && E_MODE_STOP_EM != mode)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateDebugScreenReq() - mode: %d not allowed\r\n",
                    mode);
            goto Error;
        }

        if (E_MODE_ONE_SHOT_DUMP == mode) // page_nr valid only for mode 0.
        {
            if (sscanf(pszRequest[2], "%d", &page_nr) == EOF)
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CreateDebugScreenReq() - "
                        "cannot convert %s to int\r\n", pszRequest[2]);
                goto Error;
            }

            if (MAX_NUM_OF_RESPONSE_PAGES < page_nr)
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CreateDebugScreenReq() - invalid page_nr: %d\r\n",
                        page_nr);
                goto Error;
            }
        }
    }

    RIL_LOG_INFO("CTE_XMM6260::CreateDebugScreenReq() - mode=[%d] - page_nr=[%d]\r\n",
            mode, page_nr);

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+XCGEDPAGE=%d,%d\r", mode, page_nr))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateDebugScreenReq() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateDebugScreenReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::CreateSetSMSTransportModeReq(REQUEST_DATA& rReqData,
                                                          const char** pszRequest,
                                                          const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateSetSMSTransportModeReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int service;

    if (pszRequest == NULL || '\0' == pszRequest[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetSMSTransportModeReq() - pszRequest was NULL\r\n");
        goto Error;
    }

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetSMSTransportModeReq() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    if (sscanf(pszRequest[1], "%d", &service) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetSMSTransportModeReq() -"
                " cannot convert %s to int\r\n", pszRequest);
        goto Error;
    }

    if ((service < 0) || (service > 3))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetSMSTransportModeReq() -"
                " service %s out of boundaries\r\n", service);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CreateSetSMSTransportModeReq() - service=[%d]\r\n", service);

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CGSMS=%d\r", service))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetSMSTransportModeReq() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateSetSMSTransportModeReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::CreateSetRFPowerCutbackTableReq(REQUEST_DATA& rReqData,
        const char** pszRequest, const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateSetRFPowerCutbackTableReq() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int powerTableOffset;

    if (pszRequest == NULL || '\0' == pszRequest[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM260::CreateSetRFPowerCutbackTableReq()"
                " - invalid input parameter pszRequest \r\n");
        goto Error;
    }

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetRFPowerCutbackTableReq()"
            ": received_size < required_size\r\n");
        goto Error;
    }

    if (sscanf(pszRequest[1], "%d", &powerTableOffset) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetRFPowerCutbackTableReq()"
            " - cannot convert %s to int\r\n", pszRequest);
        goto Error;
    }

    if (powerTableOffset < 0 || powerTableOffset > 3)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetRFPowerCutbackTableReq()"
            " - service %s out of boundaries\r\n", powerTableOffset);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CreateSetRFPowerCutbackTableReq() - powerTableOffset=[%d]\r\n",
        powerTableOffset);

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XRFCBT=%d\r",
            powerTableOffset))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetRFPowerCutbackTableReq()"
            " - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateSetRFPowerCutbackTableReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::SetCallImsAvailable(REQUEST_DATA& rReqData,
        const char** pszRequest, const int nNumStrings)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::SetCallImsAvailable() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int callStatus;

    if (pszRequest == NULL || '\0' == pszRequest[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM260::SetCallImsAvailable()"
                " - invalid input parameter pszRequest \r\n");
        goto Error;
    }

    if (nNumStrings < 2)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetCallImsAvailable()"
            ": received_size < required_size\r\n");
        goto Error;
    }

    if (sscanf(pszRequest[1], "%d", &callStatus) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetCallImsAvailable()"
            " - cannot convert %s to int\r\n", pszRequest);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::SetCallImsAvailable() - callStatus=[%d]\r\n",
        callStatus);

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CAVIMS=%d\r",
            callStatus))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetCallImsAvailable()"
            " - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::SetCallImsAvailable() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::SetSmsImsAvailable(REQUEST_DATA& rReqData,
        const char** pszRequest, const int nNumStrings)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::SetSmsImsAvailable() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int smsStatus;

    if (pszRequest == NULL || '\0' == pszRequest[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM260::SetSmsImsAvailable()"
                " - invalid input parameter pszRequest \r\n");
        goto Error;
    }

    if (nNumStrings < 2)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetSmsImsAvailable()"
            ": received_size < required_size\r\n");
        goto Error;
    }

    if (sscanf(pszRequest[1], "%d", &smsStatus) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetSmsImsAvailable()"
            " - cannot convert %s to int\r\n", pszRequest);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::SetSmsImsAvailable() - smsStatus=[%d]\r\n",
        smsStatus);

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CASIMS=%d\r",
            smsStatus))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetSmsImsAvailable()"
            " - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::SetSmsImsAvailable() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::GetPcscf(REQUEST_DATA& rReqData,
        const char** pszRequest, const UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::GetPcscf() - Enter\r\n");
    CChannel_Data *pChannelData = NULL;
    P_ND_GET_PCSCF_RESPONSE pResponse = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char szPcscfes[MAX_BUFFER_SIZE] = {'\0'};
    UINT32 uiCID = 0;
    size_t responseLen = 0;

    if (pszRequest[1] == NULL)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetPcscf()"
            " - invalid ifname\r\n");
        goto Error;
    }

    pChannelData = CChannel_Data::GetChnlFromIfName(pszRequest[1]);

    if (pChannelData == NULL)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetPcscf()"
            " - channel data not found for %s\r\n", pszRequest[1]);
        goto Error;
    }

    uiCID = pChannelData->GetContextID();
    if (uiCID >= 100)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetPcscf() - Invalid uiCID value");
        goto Error;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::GetPcscf() - cid=[%d]\r\n", uiCID);

    // fill the PCSCF response: CID + PCSCF addresses
    pChannelData->GetAddressString(szPcscfes, pChannelData->ADDR_PCSCF, sizeof(szPcscfes));

    pResponse = (P_ND_GET_PCSCF_RESPONSE) malloc(sizeof(S_ND_GET_PCSCF_RESPONSE));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetPcscf() - Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_GET_PCSCF_RESPONSE));

    //  create the full response
    if (!PrintStringNullTerminate(pResponse->szPcscf, sizeof(pResponse->szPcscf),
            "%d %s", uiCID, szPcscfes))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetPcscf() - Could not create response");
        goto Error;
    }
    RIL_LOG_INFO("CTE_XMM6260::GetPcscf() - response string: %s", pResponse->szPcscf);
    pResponse->sResponsePointer.pszPcscf = pResponse->szPcscf;

    // Response data are passed in pContextData2 and len in cbContextData2
    // when response is immediate.
    rReqData.pContextData2 = (void*)pResponse;
    rReqData.cbContextData2 = sizeof(S_ND_GET_PCSCF_RESPONSE_PTR);

    res = RRIL_RESULT_OK_IMMEDIATE;
Error:
    if (RRIL_RESULT_OK_IMMEDIATE != res)
    {
        free(pResponse);
        pResponse = NULL;
    }
    RIL_LOG_VERBOSE("CTE_XMM6260::GetPcscf() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::SetSrvccParams(REQUEST_DATA& rReqData, const char** pszRequest)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::SetSrvccParams() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    // pszRequest and pszRequest[0] are already checked, before calling this function
    if (pszRequest[1] == NULL)
    {
        RIL_LOG_CRITICAL("CTE_XMM260::SetSrvccParams()"
                " - invalid input parameter pszRequest \r\n");
        goto Error;
    }

    // Using, as it is, the string passed from the high-layer, without any parsing.
    // The value of the param "mode" should be 2 in this case
    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+XISRVCC=2, %s\r", pszRequest[1]))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetSrvccParams() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::SetSrvccParams() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::CreateSetDVPEnabledReq(REQUEST_DATA& rReqData,
                                                    const char** ppszRequest,
                                                    const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateSetDVPEnabledReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int dvpConfig;

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetDVPEnabledReq() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    if (ppszRequest == NULL || ppszRequest[1] == NULL || '\0' == ppszRequest[1][0])
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetDVPEnabledReq() - ppszRequest was NULL\r\n");
        goto Error;
    }

    if (sscanf(ppszRequest[1], "%d", &dvpConfig) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetDVPEnabledReq() -"
                " cannot convert %s to int\r\n", ppszRequest);
        goto Error;
    }

    if ((dvpConfig < 0) || (dvpConfig > 3))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetDVPEnabledReq() -"
                " dvpConfig %s out of boundaries\r\n", dvpConfig);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::CreateSetDVPEnabledReq() - dvpConfig=[%d]\r\n", dvpConfig);

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+XDVP=%d\r", dvpConfig))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateSetDVPEnabledReq() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateSetDVPEnabledReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseXGATR(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXGATR() - Enter\r\n");

    P_ND_GET_ATR pResponse = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    pResponse = (P_ND_GET_ATR) malloc(sizeof(S_ND_GET_ATR));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXGATR() - Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_GET_ATR));

    // Skip "+XGATR: "
    if (!SkipString(pszRsp, "+XGATR: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXGATR() - Could not skip \"+XGATR: \".\r\n");
        goto Error;
    }

    if (!ExtractQuotedString(pszRsp, pResponse->szATR, sizeof( pResponse->szATR), pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXGATR() - Could not extract ATR value.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6260::ParseXGATR() - szATR: %s\r\n", pResponse->szATR);

    // Parse "<postfix>"
    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        goto Error;
    }

    pResponse->sResponsePointer.pszATR = pResponse->szATR;

    rRspData.pData   = (void*)pResponse;
    rRspData.uiDataSize  = sizeof(S_ND_GET_ATR_POINTER);
    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXGATR() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseXDRV(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXDRV() - Enter\r\n");
    UINT32 uiIpcChrGrp;
    UINT32 uiIpcChrTempGet;
    UINT32 uiXdrvResult;
    UINT32 uiTempSensorId;
    UINT32 uiFilteredTemp;
    UINT32 uiRawTemp;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const UINT32 GET_THERMAL_SENSOR = 9;
    P_ND_THERMAL_SENSOR_VALUE pResponse = NULL;

    // Parse prefix
    if (!FindAndSkipString(pszRsp, "+XDRV: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDRV() - Unable to parse \"+XDRV\" prefix.!\r\n");
        goto Error;
    }

    // Parse <IPC_CHR_GRP>
    if (!ExtractUInt32(pszRsp, uiIpcChrGrp, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDRV() - Unable to parse <IPC_CHR_GRP>!\r\n");
        goto Error;
    }

    // Parse <IPC_CHR_TEMP_GET>
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiIpcChrTempGet, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDRV() - Unable to parse <IPC_CHR_TEMP_GET>!\r\n");
        goto Error;
    }

    // Parse <xdrv_result>
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiXdrvResult, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDRV() - Unable to parse <result>!\r\n");
        goto Error;
    }

    // XDRV Result should be XDRV_RESULT_OK (0) otherwise this is an error
    if (uiXdrvResult)
    {
        rRspData.uiResultCode = RIL_E_GENERIC_FAILURE;
        return RRIL_RESULT_OK;
    }

    if (GET_THERMAL_SENSOR == uiIpcChrTempGet)
    {
        // Parse <temp_sensor_id>
        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiTempSensorId, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDRV() - Unable to parse <temp_sensor_id>!\r\n");
            goto Error;
        }

        // Parse <filtered_temp>
        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiFilteredTemp, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDRV() - Unable to parse <filtered_temp>!\r\n");
            goto Error;
        }

        // Parse <raw_temp>
        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiRawTemp, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDRV() - Unable to parse <raw_temp>!\r\n");
            goto Error;
        }

        pResponse = (P_ND_THERMAL_SENSOR_VALUE) malloc(sizeof(S_ND_THERMAL_SENSOR_VALUE));
        if (NULL == pResponse)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDRV() - Could not allocate memory for response");
            goto Error;
        }
        memset(pResponse, 0, sizeof(S_ND_THERMAL_SENSOR_VALUE));

        RIL_LOG_INFO("IPC_CHR_GRP: %u, IPC_CHR_TEMP_GET: %u, xdrv_result: %u, temp_sensor_id: %u "
                    "filtered_temp: %u, raw_temp: %u\r\n", uiIpcChrGrp, uiIpcChrTempGet,
                    uiXdrvResult, uiTempSensorId, uiFilteredTemp, uiRawTemp);

        snprintf(pResponse->pszTemperature, sizeof(pResponse->pszTemperature),
                 "%u %u", uiFilteredTemp, uiRawTemp);

        pResponse->sResponsePointer.pszTemperature = pResponse->pszTemperature;

        rRspData.pData   = (void*)pResponse;
        rRspData.uiDataSize  = sizeof(S_ND_THERMAL_SENSOR_VALUE_PTR);
    }

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXDRV() - Exit\r\n");
    return res;
}


RIL_RESULT_CODE CTE_XMM6260::ParseCGED(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseCGED() - Enter\r\n");

    P_ND_GPRS_CELL_ENV pResponse = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    pResponse = (P_ND_GPRS_CELL_ENV) malloc(sizeof(S_ND_GPRS_CELL_ENV));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseCGED() - Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_GPRS_CELL_ENV));

    //  Copy entire response verbatim to response.
    strncpy(pResponse->pszGprsCellEnv, pszRsp, (MAX_BUFFER_SIZE*2)-1);
    pResponse->pszGprsCellEnv[(MAX_BUFFER_SIZE*2)-1] = '\0';

    pResponse->sResponsePointer.pszGprsCellEnv = pResponse->pszGprsCellEnv;

    rRspData.pData   = (void*)pResponse;
    rRspData.uiDataSize  = sizeof(S_ND_GPRS_CELL_ENV_PTR);
    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseCGED() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseXCGEDPAGE(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXCGEDPAGE() - Enter\r\n");

    P_ND_DEBUG_SCREEN pResponse = NULL;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    pResponse = (P_ND_DEBUG_SCREEN) malloc(sizeof(S_ND_DEBUG_SCREEN));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXCGEDPAGE() -"
                " Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_DEBUG_SCREEN));

    //  Copy entire response verbatim to response.
    strncpy(pResponse->pszDebugScreen, pszRsp, (MAX_BUFFER_SIZE*2)-1);
    pResponse->pszDebugScreen[(MAX_BUFFER_SIZE*2)-1] = '\0';

    pResponse->sResponsePointer.pszDebugScreen = pResponse->pszDebugScreen;

    rRspData.pData   = (void*)pResponse;
    rRspData.uiDataSize  = sizeof(S_ND_DEBUG_SCREEN_PTR);
    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXCGEDPAGE() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseCGSMS(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseCGSMS() - Enter\r\n");
    UINT32 uiService;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    P_ND_SMS_TRANSPORT_MODE pResponse = NULL;

    // Parse prefix
    if (!FindAndSkipString(pszRsp, "+CGSMS: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseCGSMS() - Unable to parse \"+CGSMS\" prefix.!\r\n");
        goto Error;
    }

    // Parse <service>
    if (!ExtractUInt32(pszRsp, uiService, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseCGSMS() - Unable to parse <service>!\r\n");
        goto Error;
    }

    // Check the upper boundary of the service
    if (uiService > 3) goto Error;

    pResponse = (P_ND_SMS_TRANSPORT_MODE) malloc(sizeof(S_ND_SMS_TRANSPORT_MODE));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseCGSMS() - Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_SMS_TRANSPORT_MODE));

    RIL_LOG_INFO("SMS Transport Mode: %u\r\n", uiService);

    snprintf(pResponse->szService, sizeof(pResponse->szService), "%u", uiService);

    pResponse->sResponsePointer.pszService = pResponse->szService;

    rRspData.pData   = (void*)pResponse;
    rRspData.uiDataSize  = sizeof(S_ND_SMS_TRANSPORT_MODE_PTR);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseCGSMS() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::HandleSendAtResponse(const char* pszRsp,
RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::HandleSendAtResponse() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    P_ND_SEND_AT_RESPONSE pResponse = NULL;

    pResponse = (P_ND_SEND_AT_RESPONSE) malloc(sizeof(S_ND_SEND_AT_RESPONSE));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::HandleSendAtResponse() -"
                "Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_SEND_AT_RESPONSE));
    strncpy(pResponse->szResponse, pszRsp, sizeof(pResponse->szResponse) - 1);

    pResponse->sResponsePointer.pszResponse = pResponse->szResponse;
    rRspData.pData = (void*)pResponse;
    rRspData.uiDataSize = sizeof(S_ND_SEND_AT_RESPONSE_PTR);

    res = RRIL_RESULT_OK;
Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }
    RIL_LOG_VERBOSE("CTE_XMM6260::HandleSendAtResponse() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseXRFCBT(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXRFCBT() - Enter\r\n");

    UINT32 uiIsEnabled;
    UINT32 uiOffsetTableIndex;
    UINT32 uiIsApplied;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    P_ND_RF_POWER_CUTBACK_TABLE pResponse = NULL;

    // Parse prefix
    if (!FindAndSkipString(pszRsp, "+XRFCBT: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXRFCBT() - Unable to parse \"+XRFCBT\" prefix\r\n");
        goto Error;
    }

    // Parse <Is Enabled>
    if (!ExtractUInt32(pszRsp, uiIsEnabled, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXRFCBT() - Unable to parse <Is Enabled>\r\n");
        goto Error;
    }

    // Parse <Offset Table Index>
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiOffsetTableIndex, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXRFCBT() - Unable to parse <Offset Table Index>\r\n");
        goto Error;
    }

    // Parse <Is Applied>
    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiIsApplied, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXRFCBT() - Unable to parse <Is Applied>\r\n");
        goto Error;
    }

    // Check the upper boundary of the parameters
    if (uiIsEnabled > 1 || uiOffsetTableIndex > 3 || uiIsApplied > 1)
    {
        goto Error;
    }

    pResponse = (P_ND_RF_POWER_CUTBACK_TABLE) malloc(sizeof(S_ND_RF_POWER_CUTBACK_TABLE));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXRFCBT() - Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_RF_POWER_CUTBACK_TABLE));

    RIL_LOG_INFO("Is Enabled: %u, Offset Table Index: %u, Is Applied: %u\r\n",
            uiIsEnabled, uiOffsetTableIndex, uiIsApplied);

    snprintf(pResponse->szRFPowerCutbackTable,
            sizeof(pResponse->szRFPowerCutbackTable),
            "%u %u %u", uiIsEnabled, uiOffsetTableIndex, uiIsApplied);

    pResponse->sResponsePointer.pszRFPowerCutbackTable = pResponse->szRFPowerCutbackTable;

    rRspData.pData = (void*)pResponse;
    rRspData.uiDataSize = sizeof(S_ND_RF_POWER_CUTBACK_TABLE_PTR);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXRFCBT() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseRegStatusAndBandInd(const char* pszRsp,
                                                              RESPONSE_DATA& /*rspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseRegStatusAndBandInd() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int dummy;
    int regStatus;
    char szBand[MAX_BAND_SIZE] = {0};

    if (!SkipRspStart(pszRsp, "+XREG: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseRegStatusAndBandInd() - "
                "Could not skip \"+XREG: \".\r\n");
        goto Error;
    }

    // Extract <n> and throw away
    if (!ExtractInt(pszRsp, dummy, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseRegStatusAndBandInd() - Could not extract <n>.\r\n");
        goto Error;
    }

    // "<stat>"
    if (!SkipString(pszRsp, ",", pszRsp)
            || !ExtractInt(pszRsp, regStatus, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseRegStatusAndBandInd() - Could not extract <stat>.\r\n");
        goto Error;
    }

    // Parse <AcT> and throw away
    if (!SkipString(pszRsp, ",", pszRsp)
            || !ExtractInt(pszRsp, dummy, pszRsp))
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseRegStatusAndBandInd() - Could not extract <AcT>\r\n");
    }

    // Extract <Band>
    if (!SkipString(pszRsp, ",", pszRsp)
            || !ExtractUnquotedString(pszRsp, ",", szBand, sizeof(szBand), pszRsp))
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseRegStatusAndBandInd() - Could not extract <Band>\r\n");
    }

    // Skip "<postfix>"
    if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseRegStatusAndBandInd() - "
                "Could not skip response postfix.\r\n");
    }

    if (m_bRegStatusAndBandIndActivated)
    {
        sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND info;
        info.commandId = RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND;
        info.regStatus = m_cte.IsRegistered(regStatus);
        CopyStringNullTerminate(info.szBand, szBand, sizeof(info.szBand));
        info.bandLength = strlen(info.szBand);
        SetRegStatusAndBandInfo(info);

        RIL_onUnsolicitedResponse(RIL_UNSOL_OEM_HOOK_RAW,
                (void*)&info, sizeof(sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND));
    }

    if (m_bCoexRegStatusAndBandIndActivated)
    {
        sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_REPORT info;
        info.commandId = RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_REPORT;
        info.regStatus = m_cte.IsRegistered(regStatus);
        CopyStringNullTerminate(info.szBand, szBand, sizeof(info.szBand));
        info.bandLength = strlen(info.szBand);

        RIL_onUnsolicitedResponse(RIL_UNSOL_OEM_HOOK_RAW,
                &info, sizeof(sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_REPORT));
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseRegStatusAndBandInd() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseXISRVCC(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXISRVCC() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    P_ND_SRVCC_RESPONSE_VALUE pResponse = NULL;
    BOOL isFirstIteration = TRUE;
    UINT32 nCallId = 0;
    UINT32 nTransferResult = 0;

    pResponse = (P_ND_SRVCC_RESPONSE_VALUE) malloc(sizeof(S_ND_SRVCC_RESPONSE_VALUE));

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXISRVCC() -"
                "Could not allocate memory for response");
        goto Error;
    }

    memset(pResponse, 0, sizeof(S_ND_SRVCC_RESPONSE_VALUE));

    while (FindAndSkipString(pszRsp, "+XISRVCC: ", pszRsp))
    {
        // Parse "<call_id> and <transfer_result>"
        if (!ExtractUInt32(pszRsp, nCallId, pszRsp) ||
            !SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, nTransferResult, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXISRVCC() -"
                    " Unable to extract UINTS, skip to next entry\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CTE_XMM6260::ParseXISRVCC() - INFO: CallId= %d    "
                    "TransferResult=%d\r\n", nCallId, nTransferResult);

            // It Contains the current pair (CallId, TransferResult)
            char szCurrentPair[MAX_SRVCC_RSP_SIZE] = {'\0'};

            // The comma should be added at the beginning of the string if not first iteration
            snprintf(szCurrentPair, sizeof(szCurrentPair),
                        (isFirstIteration) ? "%u, %u" : ", %u, %u", nCallId, nTransferResult);

            isFirstIteration = FALSE;

            // Appending the current pair to the response
            if (!ConcatenateStringNullTerminate(pResponse->szSrvccPairs,
                     sizeof(pResponse->szSrvccPairs), szCurrentPair))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseXISRVCC() - Can't add %s.\r\n",
                            szCurrentPair);
                goto Error;
            }
        }

        // Find "<postfix>"
        if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXISRVCC() - Unable to find response end\r\n");
            goto Error;
        }
    }

    RIL_LOG_INFO("CTE_XMM6260::ParseXISRVCC() - Parsed string %s.\r\n",
            pResponse->szSrvccPairs);

    pResponse->sResponsePointer.pszSrvccPairs = pResponse->szSrvccPairs;
    rRspData.pData = (void*)pResponse;
    rRspData.uiDataSize = sizeof(S_ND_SRVCC_RESPONSE_PTR);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXISRVCC() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseXDVP(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXDVP() - Enter\r\n");
    const UINT32 DVP_CONFIG_MAX = 3;
    UINT32 uiDVPConfig;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    P_ND_GET_DVP_RESPONSE pResponse = NULL;

    // Parse prefix
    if (!FindAndSkipString(pszRsp, "+XDVP: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDVP() - Unable to parse \"+XDVP\" prefix.!\r\n");
        goto Error;
    }

    // Parse <dvpConfig>
    if (!ExtractUInt32(pszRsp, uiDVPConfig, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDVP() - Unable to parse <dvpConfig>!\r\n");
        goto Error;
    }

    // Check the upper boundary of the DVP config
    if (uiDVPConfig > DVP_CONFIG_MAX) goto Error;

    pResponse = (P_ND_GET_DVP_RESPONSE) malloc(sizeof(S_ND_GET_DVP_RESPONSE));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXDVP() - Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_GET_DVP_RESPONSE));

    RIL_LOG_INFO("DVP Config: %u\r\n", uiDVPConfig);

    snprintf(pResponse->szDVPConfig, sizeof(pResponse->szDVPConfig), "%u", uiDVPConfig);

    pResponse->sResponsePointer.pszDVPConfig = pResponse->szDVPConfig;

    rRspData.pData   = (void*)pResponse;
    rRspData.uiDataSize  = sizeof(S_ND_GET_DVP_RESPONSE_PTR);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXDVP() - Exit\r\n");
    return res;
}

BOOL CTE_XMM6260::HandleSilentPINEntry(void* pRilToken, void* /*pContextData*/, int /*size*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::HandleSilentPINEntry() - Enter\r\n");

    char szPIN[MAX_PIN_SIZE] = {0};
    BOOL bPINCodeIsOk = FALSE;
    BOOL bRet = FALSE;

    ePCache_Code ret = PCache_Get_PIN(m_szUICCID, szPIN);
    if (PIN_OK == ret)
    {
        char szCmd[MAX_BUFFER_SIZE] = {0};
        CCommand* pCmd = NULL;

        bPINCodeIsOk = TRUE;

        //  Queue AT+CPIN=<PIN> command
        if (!PrintStringNullTerminate(szCmd, MAX_BUFFER_SIZE, "AT+CPIN=\"%s\"\r", szPIN))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::HandleSilentPINEntry() -"
                    " cannot create silent AT+CPIN=<PIN> command\r\n");
            goto Error;
        }

        pCmd = new CCommand(g_ReqInternal[E_REQ_IDX_SILENT_PIN_ENTRY].reqInfo.uiChannel,
                                pRilToken, g_ReqInternal[E_REQ_IDX_SILENT_PIN_ENTRY].reqId, szCmd,
                                &CTE::ParseSilentPinEntry, &CTE::PostSilentPinRetryCmdHandler);
        if (pCmd)
        {
            if (!CCommand::AddCmdToQueue(pCmd, TRUE))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::HandleSilentPINEntry() -"
                        " Unable to queue AT+CPIN command!\r\n");
                delete pCmd;
                pCmd = NULL;
                goto Error;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::HandleSilentPINEntry() -"
                    " Unable to allocate memory for new silent AT+CPIN=<PIN> command!\r\n");
            goto Error;
        }

        bRet = TRUE;
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::HandleSilentPINEntry() - Exit\r\n");
    return bRet;
}

//
//  Response to Silent PIN Entry
//
RIL_RESULT_CODE CTE_XMM6260::ParseSilentPinEntry(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSilentPinEntry() - Enter\r\n");

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSilentPinEntry() - Exit\r\n");
    return RRIL_RESULT_OK;
}

RIL_RESULT_CODE CTE_XMM6260::QueryPinRetryCount(REQUEST_DATA& rReqData,
                                                        void* /*pData*/,
                                                        UINT32 /*uiDataSize*/)
{
    RIL_RESULT_CODE res = RIL_E_GENERIC_FAILURE;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XPINCNT\r"))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::QueryPinRetryCount() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::QueryPinRetryCount() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseSimPinRetryCount(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSimPinRetryCount() - Enter / Exit\r\n");

    const char* pszRsp = rRspData.szResponse;

    return ParseXPINCNT(pszRsp);
}

void CTE_XMM6260::PostSetupDataCallCmdHandler(POST_CMD_HANDLER_DATA& rData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::PostSetupDataCallCmdHandler() Enter\r\n");

    BOOL bSuccess = FALSE;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    UINT32 uiCID = 0;
    CChannel_Data* pChannelData = NULL;

    if (NULL == rData.pContextData ||
            sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != rData.uiContextDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::PostSetupDataCallCmdHandler() - Invalid context data\r\n");
        goto Error;
    }

    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)rData.pContextData;
    uiCID = pDataCallContextData->uiCID;

    if (RIL_E_SUCCESS != rData.uiResultCode)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostSetupDataCallCmdHandler() - Failure\r\n");
        goto Error;
    }

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostSetupDataCallCmdHandler() -"
                " No Data Channel for CID %u.\r\n", uiCID);
        goto Error;
    }

    pChannelData->SetDataState(E_DATA_STATE_ACTIVATING);

    if (!CreatePdpContextActivateReq(rData.uiChannel, rData.pRilToken,
                                    rData.requestId, rData.pContextData,
                                    rData.uiContextDataSize,
                                    &CTE::ParsePdpContextActivate,
                                    &CTE::PostPdpContextActivateCmdHandler))
    {
        RIL_LOG_INFO("CTE_XMM6260::PostSetupDataCallCmdHandler() -"
                " CreatePdpContextActivateReq failed\r\n");
        goto Error;
    }

    bSuccess = TRUE;
Error:
    if (!bSuccess)
    {
        free(rData.pContextData);
        rData.pContextData = NULL;

        HandleSetupDataCallFailure(uiCID, rData.pRilToken, rData.uiResultCode);
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::PostSetupDataCallCmdHandler() Exit\r\n");
}

void CTE_XMM6260::PostPdpContextActivateCmdHandler(POST_CMD_HANDLER_DATA& rData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::PostPdpContextActivateCmdHandler() Enter\r\n");

    CChannel_Data* pChannelData = NULL;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    UINT32 uiCID = 0;
    int state;
    BOOL bSuccess = FALSE;

    if (NULL == rData.pContextData ||
            sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != rData.uiContextDataSize)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostPdpContextActivateCmdHandler() -"
                " Invalid context data\r\n");
        goto Error;
    }

    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)rData.pContextData;
    uiCID = pDataCallContextData->uiCID;

    if (RIL_E_SUCCESS != rData.uiResultCode)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostPdpContextActivateCmdHandler() - Failure\r\n");
        goto Error;
    }

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostPdpContextActivateCmdHandler() -"
                " No Data Channel for CID %u.\r\n", uiCID);
        goto Error;
    }

    state = pChannelData->GetDataState();
    if (E_DATA_STATE_ACTIVATING != state)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostPdpContextActivateCmdHandler() -"
                " Wrong data state: %d\r\n", state);
        goto Error;
    }

    pChannelData->SetDataState(E_DATA_STATE_ACTIVE);
    pChannelData->SetRoutingEnabled(TRUE);

    if (!CreateQueryIpAndDnsReq(rData.uiChannel, rData.pRilToken,
                                    rData.requestId, rData.pContextData,
                                    rData.uiContextDataSize,
                                    &CTE::ParseQueryIpAndDns,
                                    &CTE::PostQueryIpAndDnsCmdHandler))
    {
        RIL_LOG_INFO("CTE_XMM6260::PostPdpContextActivateCmdHandler() -"
                " CreateQueryIpAndDnsReq failed\r\n");
        goto Error;
    }

    bSuccess = TRUE;
Error:
    if (!bSuccess)
    {
        free(rData.pContextData);
        rData.pContextData = NULL;

        HandleSetupDataCallFailure(uiCID, rData.pRilToken, rData.uiResultCode);
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::PostPdpContextActivateCmdHandler() Exit\r\n");
}

void CTE_XMM6260::PostQueryIpAndDnsCmdHandler(POST_CMD_HANDLER_DATA& rData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::PostQueryIpAndDnsCmdHandler() Enter\r\n");

    UINT32 uiCID = 0;
    CChannel_Data* pChannelData = NULL;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;;
    int state;
    BOOL bSuccess = FALSE;

    if (NULL == rData.pContextData ||
            sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != rData.uiContextDataSize)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostQueryIpAndDnsCmdHandler() - Invalid context data\r\n");
        goto Error;
    }

    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)rData.pContextData;
    uiCID = pDataCallContextData->uiCID;

    if (RIL_E_SUCCESS != rData.uiResultCode)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostQueryIpAndDnsCmdHandler() - Failure\r\n");
        goto Error;
    }

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostQueryIpAndDnsCmdHandler() -"
                " No Data Channel for CID %u.\r\n", uiCID);
        goto Error;
    }

    state = pChannelData->GetDataState();
    if (E_DATA_STATE_ACTIVE != state)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostQueryIpAndDnsCmdHandler() - Wrong data state: %d\r\n",
                state);
        goto Error;
    }

    if (!CreateEnterDataStateReq(rData.uiChannel, rData.pRilToken,
                                    rData.requestId, rData.pContextData,
                                    rData.uiContextDataSize,
                                    &CTE::ParseEnterDataState,
                                    &CTE::PostEnterDataStateCmdHandler))
    {
        RIL_LOG_INFO("CTE_XMM6260::PostQueryIpAndDnsCmdHandler() -"
                " CreateEnterDataStateReq failed\r\n");
        goto Error;
    }

    bSuccess = TRUE;

Error:
    if (!bSuccess)
    {
        free(rData.pContextData);
        rData.pContextData = NULL;

        HandleSetupDataCallFailure(uiCID, rData.pRilToken, rData.uiResultCode);
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::PostQueryIpAndDnsCmdHandler() Exit\r\n");
}

void CTE_XMM6260::PostEnterDataStateCmdHandler(POST_CMD_HANDLER_DATA& rData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::PostEnterDataStateCmdHandler() Enter\r\n");

    UINT32 uiCID = 0;
    CChannel_Data* pChannelData = NULL;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;;
    int state;
    BOOL bSuccess = FALSE;

    if (NULL == rData.pContextData ||
            sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != rData.uiContextDataSize)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostEnterDataStateCmdHandler() - Invalid context data\r\n");
        goto Error;
    }

    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)rData.pContextData;
    uiCID = pDataCallContextData->uiCID;

    if (RIL_E_SUCCESS != rData.uiResultCode)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostEnterDataStateCmdHandler() - Failure\r\n");
        goto Error;
    }

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostEnterDataStateCmdHandler() -"
                " No Data Channel for CID %u.\r\n", uiCID);
        goto Error;
    }

    state = pChannelData->GetDataState();
    if (E_DATA_STATE_ACTIVE != state)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostEnterDataStateCmdHandler() - Wrong data state: %d\r\n",
                state);
        goto Error;
    }

    if (!SetupInterface(uiCID))
    {
        RIL_LOG_INFO("CTE_XMM6260::PostEnterDataStateCmdHandler() - SetupInterface failed\r\n");
        goto Error;
    }

    bSuccess = TRUE;

Error:
    free(rData.pContextData);
    rData.pContextData = NULL;

    if (!bSuccess)
    {
        HandleSetupDataCallFailure(uiCID, rData.pRilToken, rData.uiResultCode);
    }
    else
    {
        pChannelData->IncrementRefCount();
        HandleSetupDataCallSuccess(uiCID, rData.pRilToken);
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::PostEnterDataStateCmdHandler() Exit\r\n");
}

void CTE_XMM6260::PostDeactivateDataCallCmdHandler(POST_CMD_HANDLER_DATA& rData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::PostDeactivateDataCallCmdHandler() Enter\r\n");

    UINT32 uiCID;

    if (RIL_E_SUCCESS != rData.uiResultCode)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostDeactivateDataCallCmdHandler() request failure\r\n");
        goto Error;
    }

    if (NULL == rData.pContextData ||
            sizeof(UINT32) != rData.uiContextDataSize)
    {
        RIL_LOG_INFO("CTE_XMM6260::PostDeactivateDataCallCmdHandler() -"
                " Invalid context data\r\n");
        goto Error;
    }

    uiCID = *((UINT32*)rData.pContextData);

    DataConfigDown(uiCID);

Error:
    free(rData.pContextData);
    rData.pContextData = NULL;

    if (NULL != rData.pRilToken)
    {
        RIL_onRequestComplete(rData.pRilToken, RIL_E_SUCCESS, NULL, 0);
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::PostDeactivateDataCallCmdHandler() Exit\r\n");
}

BOOL CTE_XMM6260::CreatePdpContextActivateReq(UINT32 uiChannel,
                                            RIL_Token rilToken,
                                            int reqId, void* pData,
                                            UINT32 uiDataSize,
                                            PFN_TE_PARSE pParseFcn,
                                            PFN_TE_POSTCMDHANDLER pPostCmdHandlerFcn)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreatePdpContextActivateReq() - Enter\r\n");

    BOOL bRet = FALSE;
    REQUEST_DATA reqData;

    memset(&reqData, 0, sizeof(REQUEST_DATA));

    if (!PdpContextActivate(reqData, pData, uiDataSize))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreatePdpContextActivateReq() -"
                " Unable to create AT command data\r\n");
        goto Error;
    }
    else
    {
        CCommand* pCmd = new CCommand(uiChannel, rilToken, reqId, reqData,
                                                pParseFcn, pPostCmdHandlerFcn);
        if (pCmd)
        {
            pCmd->SetContextData(pData);
            pCmd->SetContextDataSize(uiDataSize);
            pCmd->SetHighPriority();
            if (!CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CreatePdpContextActivateReq() -"
                        " Unable to add command to queue\r\n");
                delete pCmd;
                pCmd = NULL;
                goto Error;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreatePdpContextActivateReq() -"
                    " Unable to allocate memory for command\r\n");
            goto Error;
        }
    }

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreatePdpContextActivateReq() - Exit\r\n");
    return bRet;
}

BOOL CTE_XMM6260::CreateQueryIpAndDnsReq(UINT32 uiChannel, RIL_Token rilToken,
                                            int reqId, void* pData,
                                            UINT32 uiDataSize,
                                            PFN_TE_PARSE pParseFcn,
                                            PFN_TE_POSTCMDHANDLER pPostCmdHandlerFcn)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateQueryIpAndDnsReq() - Enter\r\n");

    BOOL bRet = FALSE;
    REQUEST_DATA reqData;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;

    if (NULL == pData || sizeof(*pDataCallContextData) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateQueryIpAndDnsReq() - Invalid context data\r\n");
        goto Error;
    }

    memset(&reqData, 0, sizeof(REQUEST_DATA));
    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)pData;

    if (!QueryIpAndDns(reqData, pDataCallContextData->uiCID))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateQueryIpAndDnsReq() -"
                " Unable to create AT command data\r\n");
        goto Error;
    }
    else
    {
        CCommand* pCmd = new CCommand(uiChannel, rilToken, reqId, reqData,
                                                pParseFcn, pPostCmdHandlerFcn);
        if (pCmd)
        {
            pCmd->SetContextData(pData);
            pCmd->SetContextDataSize(uiDataSize);
            pCmd->SetHighPriority();
            if (!CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CreateQueryIpAndDnsReq() -"
                        " Unable to add command to queue\r\n");
                delete pCmd;
                pCmd = NULL;
                goto Error;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateQueryIpAndDnsReq() -"
                    " Unable to allocate memory for command\r\n");
            goto Error;
        }
    }

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateQueryIpAndDnsReq() - Exit\r\n");
    return bRet;
}

BOOL CTE_XMM6260::CreateEnterDataStateReq(UINT32 uiChannel, RIL_Token rilToken,
                                            int reqId, void* pData,
                                            UINT32 uiDataSize,
                                            PFN_TE_PARSE pParseFcn,
                                            PFN_TE_POSTCMDHANDLER pPostCmdHandlerFcn)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateEnterDataStateReq() - Enter\r\n");

    BOOL bRet = FALSE;
    REQUEST_DATA reqData;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;

    if (NULL == pData || sizeof(*pDataCallContextData) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateEnterDataStateReq() - Invalid context data\r\n");
        goto Error;
    }

    memset(&reqData, 0, sizeof(REQUEST_DATA));
    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)pData;

    if (!EnterDataState(reqData, pDataCallContextData->uiCID))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateEnterDataStateReq() -"
                " Unable to create AT command data\r\n");
        goto Error;
    }
    else
    {
        CCommand* pCmd = new CCommand(uiChannel, rilToken, reqId, reqData,
                                                pParseFcn, pPostCmdHandlerFcn);
        if (pCmd)
        {
            pCmd->SetContextData(pData);
            pCmd->SetContextDataSize(uiDataSize);
            pCmd->SetHighPriority();
            pCmd->SetAlwaysParse();
            if (!CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::CreateEnterDataStateReq() -"
                        " Unable to add command to queue\r\n");
                delete pCmd;
                pCmd = NULL;
                goto Error;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateEnterDataStateReq() -"
                    " Unable to allocate memory for command\r\n");
            goto Error;
        }
    }

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateEnterDataStateReq() - Exit\r\n");
    return bRet;
}

BOOL CTE_XMM6260::SetupInterface(UINT32 uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::SetupInterface() - Enter\r\n");

    BOOL bRet = FALSE;
    char szNetworkInterfaceName[IFNAMSIZ];
    struct gsm_netconfig netconfig;
    int fd = -1;
    int ret = 0;
    CChannel_Data* pChannelData = NULL;
    PDP_TYPE eDataConnectionType = PDP_TYPE_IPV4;  //  dummy for now, set to IPv4.
    UINT32 uiChannel = 0;
    int state = 0;

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetupInterface() - No Data Channel for CID %u.\r\n",
                                                                    uiCID);
        goto Error;
    }

    state = pChannelData->GetDataState();
    if (E_DATA_STATE_ACTIVE != state)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetupInterface() - Invalid data state %d.\r\n",
                                                                    state);
        goto Error;
    }

    if (!PrintStringNullTerminate(szNetworkInterfaceName, IFNAMSIZ,
                            "%s%u", m_szNetworkInterfaceNamePrefix, uiCID-1))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetupInterface() - Cannot set network interface name\r\n");
        goto Error;
    }
    else
    {
        RIL_LOG_INFO("CTE_XMM6260::SetupInterface() - szNetworkInterfaceName=[%s], CID=[%u]\r\n",
                                                szNetworkInterfaceName, uiCID);
    }

    pChannelData->SetInterfaceName(szNetworkInterfaceName);
    uiChannel = pChannelData->GetRilChannel();

    // N_GSM related code
    netconfig.adaption = 3; /// @TODO: Use meaningful name
    netconfig.protocol = htons(ETH_P_IP);
    strncpy(netconfig.if_name, szNetworkInterfaceName, IFNAMSIZ - 1);
    netconfig.if_name[IFNAMSIZ - 1] = '\0';

    // Add IF NAME
    fd = pChannelData->GetFD();
    if (fd >= 0)
    {
        RIL_LOG_INFO("CTE_XMM6260::SetupInterface() -"
                " ***** PUTTING channel=[%u] in DATA MODE *****\r\n", uiChannel);
        ret = ioctl(fd, GSMIOC_ENABLE_NET, &netconfig); // Enable data channel
        if (ret < 0)
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::SetupInterface() -"
                    " Unable to create interface %s : %s \r\n", netconfig.if_name,strerror(errno));
            goto Error;
        }
    }
    else
    {
        //  No FD.
        RIL_LOG_CRITICAL("CTE_XMM6260::SetupInterface() - "
                "Could not get Data Channel chnl=[%u] fd=[%d].\r\n", uiChannel, fd);
        goto Error;
    }

    pChannelData->GetDataConnectionType(eDataConnectionType);

    // set interface address(es) and bring up interface
    if (!DataConfigUp(szNetworkInterfaceName, pChannelData, eDataConnectionType))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::SetupInterface() -"
                " Unable to bringup interface ifconfig\r\n");
        goto Error;
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::SetupInterface() Exit\r\n");
    return bRet;
}

//
//  Call this whenever data is disconnected
//
BOOL CTE_XMM6260::DataConfigDown(UINT32 uiCID, BOOL /*bForceCleanup*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::DataConfigDown() - Enter\r\n");

    //  First check to see if uiCID is valid
    if (uiCID == 0)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::DataConfigDown() - Invalid CID = [%u]\r\n", uiCID);
        return FALSE;
    }

    CChannel_Data* pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::DataConfigDown() -"
                " Invalid CID=[%u], no data channel found!\r\n", uiCID);
        return FALSE;
    }

    pChannelData->RemoveInterface();
    pChannelData->ResetDataCallInfo();

    RIL_LOG_VERBOSE("CTE_XMM6260::DataConfigDown() EXIT\r\n");
    return TRUE;
}

RIL_RESULT_CODE CTE_XMM6260::HandleScreenStateReq(int screenState)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::HandleScreenStateReq() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    REQUEST_DATA reqData;
    CCommand* pCmd = NULL;
    char szConformanceProperty[PROPERTY_VALUE_MAX] = {'\0'};
    CRepository repository;
    const char* pszScreenOffString = NULL;

    // Data for fast dormancy
    static char s_szFDDelayTimer[MAX_FDTIMER_SIZE] = {'\0'};
    static char s_szSCRITimer[MAX_FDTIMER_SIZE] = {'\0'};
    static BOOL s_bFDParamReady = FALSE;

    memset(&reqData, 0, sizeof(REQUEST_DATA));

    //  Store setting in context.
    reqData.pContextData = (void*)(intptr_t)screenState;

    switch (screenState)
    {
        case SCREEN_STATE_ON:
            if (!CopyStringNullTerminate(reqData.szCmd1, GetScreenOnString(),
                    sizeof(reqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::HandleScreenStateReq() - Cannot create command\r\n");
                goto Error;
            }
            break;

        case SCREEN_STATE_OFF:
            pszScreenOffString = GetScreenOffString();
            if (!CopyStringNullTerminate(reqData.szCmd1, pszScreenOffString,
                    sizeof(reqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::HandleScreenStateReq() - "
                        "Cannot create command\r\n");
                goto Error;
            }
            break;

        default:
            goto Error;
    }

    // Read the "conformance" property and disable FD if it is set to "true"
    property_get("persist.conformance", szConformanceProperty, NULL);

    // if Modem Fast Dormancy mode is "Display Driven"
    if (E_FD_MODE_DISPLAY_DRIVEN == m_cte.GetFastDormancyMode()
            && strncmp(szConformanceProperty, "true", PROPERTY_VALUE_MAX))
    {
        // disable MAFD when "Screen On", enable MAFD when "Screen Off"
        //      XFDOR=2: switch ON MAFD
        //      XFDOR=3: switch OFF MAFD
        // Read Fast Dormancy Timers from repository if screen is off
        if (SCREEN_STATE_OFF == screenState && s_bFDParamReady == FALSE)
        {
            repository.ReadFDParam(g_szGroupModem, g_szFDDelayTimer, s_szFDDelayTimer,
                    MAX_FDTIMER_SIZE, XFDOR_MIN_FDDELAY_TIMER, XFDOR_MAX_FDDELAY_TIMER);
            repository.ReadFDParam(g_szGroupModem, g_szSCRITimer, s_szSCRITimer,
                    MAX_FDTIMER_SIZE, XFDOR_MIN_SCRI_TIMER, XFDOR_MAX_SCRI_TIMER);
            s_bFDParamReady = TRUE;
        }

        if (!PrintStringNullTerminate(reqData.szCmd2, sizeof(reqData.szCmd2),
                (SCREEN_STATE_ON == screenState) ? "AT+XFDOR=3\r" : "AT+XFDOR=2,%s,%s\r",
                s_szFDDelayTimer, s_szSCRITimer))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::HandleScreenStateReq() - "
                    "Cannot create XFDOR command\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;

    pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_SCREEN_STATE].uiChannel,
                            NULL, RIL_REQUEST_SCREEN_STATE, reqData, &CTE::ParseScreenState);

    if (pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::HandleScreenStateReq() - "
                    "Unable to add command to queue\r\n");
            res = RIL_E_GENERIC_FAILURE;
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::HandleScreenStateReq() -"
                " Unable to allocate memory for command\r\n");
        res = RIL_E_GENERIC_FAILURE;
    }

Error:
    free((char*)pszScreenOffString);
    RIL_LOG_VERBOSE("CTE_XMM6260::HandleScreenStateReq() - Exit\r\n");
    return res;
}

BOOL CTE_XMM6260::GetRadioPowerCommand(BOOL bTurnRadioOn, int radioOffReason,
        char* pCmdBuffer, int cmdBufferLen)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::GetRadioPowerCommand() - Enter\r\n");

    BOOL bRet = FALSE;
    char szCmd[MAX_BUFFER_SIZE] = {'\0'};

    if (NULL == pCmdBuffer || cmdBufferLen <= 0)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetRadioPowerCommand() - Invalid buffer\r\n");

        return bRet;
    }

    if (!CHardwareConfig::GetInstance().IsMultiSIM()
            || CHardwareConfig::GetInstance().IsMultiModem())
    {
        if (bTurnRadioOn)
        {
            strcpy(szCmd, "AT+CFUN=1\r");
        }
        else
        {
            if (E_RADIO_OFF_REASON_SHUTDOWN == radioOffReason)
            {
                char szConformanceProperty[PROPERTY_VALUE_MAX] = {'\0'};

                property_get("persist.conformance", szConformanceProperty, NULL);
                if (0 == strncmp(szConformanceProperty, "true", PROPERTY_VALUE_MAX))
                {
                    /*
                    * Since sending of CFUN=4 results in few conformance test cases failing,
                    * don't send any command to modem if conformance property is set.
                    */
                }
                else
                {
                    strcpy(szCmd, "AT+CFUN=4\r");
                }
            }
            else if (E_RADIO_OFF_REASON_AIRPLANE_MODE == radioOffReason
                    || E_RADIO_OFF_REASON_NONE == radioOffReason)
            {
                strcpy(szCmd, "AT+CFUN=4\r");
            }
        }
    }
    else
    {
        // set the SIM power off parameter
        UINT32 uiSimPoweredOff = bTurnRadioOn ? 0 : 1;

        // Power On (20) or flight mode (21)
        UINT32 uiFunMode = bTurnRadioOn ? 20 : 21;

        if (!PrintStringNullTerminate(szCmd, sizeof(szCmd),
                "AT+CFUN=%u,%u\r", uiFunMode, uiSimPoweredOff))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::GetRadioPowerCommand() - Cannot create command\r\n");
            goto Error;
        }
    }

    if (!CopyStringNullTerminate(pCmdBuffer, szCmd, cmdBufferLen))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetRadioPowerCommand() - "
                "Cannot copy command to output buffer\r\n");

        goto Error;
    }

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::GetRadioPowerCommand() - Exit\r\n");

    return bRet;
}

void CTE_XMM6260::HandleInternalDtmfStopReq()
{
    RIL_LOG_VERBOSE("CTE_XMM6260::HandleInternalDtmfStopReq() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_DTMF_STOP].uiChannel,
                                    NULL, RIL_REQUEST_DTMF_STOP, "AT+XVTS\r",
                                    NULL, &CTE::PostInternalDtmfStopReq);

    if (pCmd)
    {
        pCmd->SetCallId(GetCurrentCallId());
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::HandleInternalDtmfStopReq() -"
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::HandleInternalDtmfStopReq() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::HandleInternalDtmfStopReq() - Exit\r\n");
}

void CTE_XMM6260::HandleChannelsBasicInitComplete()
{
    RIL_LOG_VERBOSE("CTE_XMM6260::HandleChannelsBasicInitComplete() - Enter\r\n");

    if (!m_cte.IsRadioRequestPending()
            && RADIO_STATE_UNAVAILABLE == GetRadioState())
    {
        /*
         * Needed as RIL_REQUEST_RADIO_POWER request is not received
         * after modem core dump, warm reset.
         */
        SetRadioStateAndNotify(RRIL_RADIO_STATE_OFF);
    }

    QueryUiccInfo();

    RIL_LOG_VERBOSE("CTE_XMM6260::HandleChannelsBasicInitComplete() - Exit\r\n");
}

void CTE_XMM6260::QueryUiccInfo()
{
    // For the following requests, same channel needs to be used
    QueryActiveApplicationType();

    QueryAvailableApplications();

    QueryIccId();

    QueryPinRetryCount();

    QuerySimState();
}

void CTE_XMM6260::QueryPinRetryCount()
{
    RIL_LOG_VERBOSE("CTE_XMM6260::QueryPinRetryCount() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_GET_SIM_STATUS].uiChannel,
            NULL, RIL_REQUEST_GET_SIM_STATUS, "AT+XPINCNT\r", &CTE::ParseSimPinRetryCount);


    if (NULL == pCmd)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::QueryPinRetryCount() - "
                "Unable to allocate memory for new command!\r\n");
    }
    else
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::QueryPinRetryCount() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::QueryPinRetryCount() - Exit\r\n");
}

void CTE_XMM6260::QuerySimState()
{
    RIL_LOG_VERBOSE("CTE_XMM6260::QuerySimState() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_GET_SIM_STATUS].uiChannel,
            NULL, RIL_REQUEST_GET_SIM_STATUS, "AT+XSIMSTATE?\r", &CTE::ParseSimStateQuery,
            &CTE::PostSimStateQuery);

    if (NULL == pCmd)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::QuerySimState() - "
                "Unable to allocate memory for new command!\r\n");
    }
    else
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::QuerySimState() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::QuerySimState() - Exit\r\n");
}

RIL_RESULT_CODE CTE_XMM6260::ParseSimStateQuery(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseSimStateQuery() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    if (FindAndSkipString(pszRsp, "+XSIMSTATE: ", pszRsp))
    {
        ParseXSIMSTATE(pszRsp);
    }

    if (FindAndSkipString(pszRsp, "+XLOCK: ", pszRsp))
    {
        ParseXLOCK(pszRsp);
    }

    // Skip "<postfix>"
    if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseSimStateQuery - "
                "Could not skip response postfix.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    return res;
}

void CTE_XMM6260::HandleSimState(const UINT32 uiSIMState, BOOL& bNotifySimStatusChange)
{
    int appState = RIL_APPSTATE_UNKNOWN;
    int pinState = RIL_PINSTATE_UNKNOWN;
    int cardState = RIL_CARDSTATE_PRESENT;

    // Based on sim state, override bNotifySimStatusChange to FALSE if sim status change should not
    // be reported to framework.
    bNotifySimStatusChange = TRUE;

    switch (uiSIMState)
    {
        case 0: // SIM not present
            RIL_LOG_INFO("CTE_XMM6260::HandleSimState() - SIM NOT PRESENT\r\n");
            m_bReadyForAttach = FALSE;
            m_bRefreshWithUSIMInitOn = FALSE;
            cardState = RIL_CARDSTATE_ABSENT;
            ResetCardStatus(TRUE);
            break;

        case 9: // SIM Removed
            RIL_LOG_INFO("CTE_XMM6260::HandleSimState() - SIM REMOVED\r\n");
            m_bReadyForAttach = FALSE;
            m_bRefreshWithUSIMInitOn = FALSE;
            cardState = RIL_CARDSTATE_ABSENT;
            PCache_Clear();
            ResetCardStatus(TRUE);
            ResetInitialAttachApn();
            break;

        case 1: // PIN verification needed
            appState = RIL_APPSTATE_PIN;
            pinState = RIL_PINSTATE_ENABLED_NOT_VERIFIED;
            break;

        /*
         * XSIM: 2 means PIN verification not needed but not ready for attach.
         * SIM_READY should be triggered only when the modem is ready
         * to attach.(XSIM: 7 or XSIM: 3(in some specific case))
         */
        case 2:
            // The SIM is initialized, but modem is still in the process of it.
            // we can inform Android that SIM is still not ready.
            RIL_LOG_INFO("CTE_XMM6260::HandleSimState() - SIM NOT READY\r\n");
            appState = RIL_APPSTATE_DETECTED;
            break;

        /*
         * XSIM: 3 will be received upon PIN related operations.
         *
         * For PIN related operation occuring after the SIM initialisation,
         * no XSIM: 7 will be sent by modem. So, trigger the radio state
         * change with SIM ready upon XSIM: 3.
         *
         * For PIN related operation occuring during the boot or before
         * the SIM initialisation, then XSIM: 7 will be sent by modem. In this
         * case, radio state  change with SIM ready will be sent upon the
         * receival of XSIM: 7 event.
         */
        case 3: // PIN verified - Ready
            if (m_bReadyForAttach)
            {
                RIL_LOG_INFO("CTE_XMM6260::HandleSimState() - READY FOR ATTACH\r\n");
                appState = RIL_APPSTATE_READY;
            }
            else
            {
                appState = RIL_APPSTATE_DETECTED;
            }
            pinState = RIL_PINSTATE_ENABLED_VERIFIED;
            break;

        case 4: // PUK verification needed
            appState = RIL_APPSTATE_PUK;
            pinState = RIL_PINSTATE_ENABLED_BLOCKED;
            break;

        case 5: // SIM permanently blocked
            appState = RIL_APPSTATE_PUK;
            pinState = RIL_PINSTATE_ENABLED_PERM_BLOCKED;
            break;

        case 6: // SIM Error
            RIL_LOG_INFO("CTE_XMM6260::HandleSimState() - SIM ERROR\r\n");
            cardState = RIL_CARDSTATE_ERROR;
            break;

        case 7: // ready for attach (+COPS)
            RIL_LOG_INFO("CTE_XMM6260::HandleSimState() - READY FOR ATTACH\r\n");
            m_bReadyForAttach = TRUE;
            appState = RIL_APPSTATE_READY;
            CSystemManager::GetInstance().TriggerSimUnlockedEvent();
            break;

        case 8: // SIM application error
        {
            RIL_LOG_INFO("CTE_XMM6260::HandleSimState() - SIM TECHNICAL PROBLEM\r\n");
            // +XSIM: 8 is for 6FXX error type
            const char error[] = "6FXX";
            triggerSIMAppError(error);
            break;
        }

        /*
         * XSIM: 10 and XSIM: 11 will be received when the SIM driver has
         * lost contact of SIM and re-established the contact respectively.
         * After XSIM: 10, either XSIM: 9 or XSIM: 11 will be received.
         * So, no need to trigger SIM_NOT_READY on XSIM: 10. Incase of
         * XSIM: 11, network registration will be restored by the modem
         * itself.
         */
        case 10: // SIM Reactivating
            break;

        case 11: // SIM Reactivated
            appState = RIL_APPSTATE_READY;
            break;

        case 12: // SIM SMS caching completed
            RIL_LOG_INFO("[RIL STATE] SIM SMS CACHING COMPLETED\r\n");
            bNotifySimStatusChange = FALSE;
            appState = GetSimAppState();
            break;

        case 16: // SIM IMEI locked
            RIL_LOG_INFO("[RIL STATE] SIM IMEI check failure (16)\r\n");
            appState = RIL_APPSTATE_IMEI;
            break;

        case 14: // SIM powered off by modem
        case 99: // SIM state unknown
        default:
            m_bReadyForAttach = FALSE;
            break;
    }

    if (m_bRefreshWithUSIMInitOn)
    {
        if (m_bReadyForAttach)
        {
            RIL_SimRefreshResponse_v7 simRefreshResp;

            simRefreshResp.result = SIM_INIT;
            simRefreshResp.ef_id = 0;
            simRefreshResp.aid = NULL;

            m_bRefreshWithUSIMInitOn = FALSE;

            // Send out SIM_REFRESH notification
            RIL_onUnsolicitedResponse(RIL_UNSOL_SIM_REFRESH, (void*)&simRefreshResp,
                    sizeof(RIL_SimRefreshResponse_v7));
        }
        else
        {
            /*
             * Incase of REFRESH with USIM INIT, once the refresh is done on the modem side,
             * then XSIM: 2 and XSIM: 7 will be sent by modem. Notifying framework of SIM status
             * change on XSIM: 2 will result in framework querying the SIM status. It is highly
             * possible that XSIM: 7 will be received even before the SIM status change is queried
             * from modem. So, if REFRESH with USIM init is active, it is better not to notify
             * the SIM status change on XSIM: 2 to avoid unnecessary requests between AP and modem.
             * Even if the SIM related requests are sent by framework on SIM not ready, rapid ril's
             * internal sim state will complete the SIM requests with error.
             */
            bNotifySimStatusChange = FALSE;
        }
    }

    SetSimState(cardState, appState, pinState);
}

BOOL CTE_XMM6260::ParseXSIMSTATE(const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXSIMSTATE() - Enter\r\n");

    UINT32 uiMode = 0;
    UINT32 uiSimState = 0;
    UINT32 uiTemp = 0;
    BOOL bRet = FALSE;
    BOOL bNotifySimStatusChange = FALSE;
    const UINT32 SIM_PHONEBOOK_CACHING_COMPLETE = 1;

    // Extract "<mode>"
    if (!ExtractUInt32(rszPointer, uiMode, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXSIMSTATE() - Could not parse <mode>.\r\n");
        goto Error;
    }

    // Extract ",<SIM state>"
    if (!SkipString(rszPointer, ",", rszPointer) ||
            !ExtractUInt32(rszPointer, uiSimState, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXSIMSTATE() - Could not parse <SIM state>.\r\n");
        goto Error;
    }

    HandleSimState(uiSimState, bNotifySimStatusChange);

    // Extract ",<pbready>"
    if (!SkipString(rszPointer, ",", rszPointer) ||
            !ExtractUInt32(rszPointer, uiTemp, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXSIMSTATE() - Could not parse <SIM PB Ready>.\r\n");
        goto Error;
    }

    if (uiTemp == SIM_PHONEBOOK_CACHING_COMPLETE)
    {
        QuerySimSmsStoreStatus();
    }

    // Extract ",<SMS Ready>"
    if (SkipString(rszPointer, ",", rszPointer))
    {
        if (!ExtractUInt32(rszPointer, uiTemp, rszPointer))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXSIMSTATE() - Could not parse "
                    "<SIM SMS Ready>.\r\n");
            goto Error;
        }
    }

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXSIMSTATE() - Exit\r\n");
    return bRet;
}

BOOL CTE_XMM6260::ParseXLOCK(const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXLOCK() - Enter\r\n");

    /*
     * If the MT is waiting for any of the following passwords
     * PH-NET PIN, PH-NETSUB PIN, PH-SP PIN then only XLOCK URC will be
     * received.
     */

    BOOL bRet = FALSE;
    int i = 0;
    BOOL bIsDataValid = FALSE;
    int perso_substate = RIL_PERSOSUBSTATE_UNKNOWN;

    //  The number of locks returned by +XLOCK URC.
    const int nMAX_LOCK_INFO = 5;

    typedef struct
    {
        char fac[3];
        UINT32 lock_state;
        UINT32 lock_result;
    } S_LOCK_INFO;

    S_LOCK_INFO lock_info[nMAX_LOCK_INFO];

    memset(lock_info, 0, sizeof(lock_info));

    if (RIL_APPSTATE_IMEI == GetSimAppState())
    {
        RIL_LOG_INFO("CTE_XMM6260::ParseXLOCK() - ignore XLOCK when UICC is locked to a"
                " device\r\n");
        return TRUE;
    }

    // Change the number to the number of facility locks supported via XLOCK URC.
    while (i < nMAX_LOCK_INFO)
    {
        memset(lock_info[i].fac, '\0', sizeof(lock_info[i].fac));

        // Extract "<fac>"
        if (ExtractQuotedString(rszPointer, lock_info[i].fac, sizeof(lock_info[i].fac),
                rszPointer))
        {
            // Extract ",<Lock state>"
            if (!SkipString(rszPointer, ",", rszPointer) ||
                !ExtractUInt32(rszPointer, lock_info[i].lock_state, rszPointer))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseXLOCK() - Could not parse <lock state>.\r\n");
                goto Error;
            }

            // Extract ",<Lock result>"
            if (!SkipString(rszPointer, ",", rszPointer) ||
                !ExtractUInt32(rszPointer, lock_info[i].lock_result, rszPointer))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseXLOCK() - Could not parse <lock result>.\r\n");
                goto Error;
            }

            bIsDataValid = TRUE;
        }
        else
        {
            RIL_LOG_INFO("CTE_XMM6260::ParseXLOCK() - Unable to find <fac>!\r\n");
        }

        SkipString(rszPointer, ",", rszPointer);

        i++;
    }

    if (bIsDataValid)
    {
        i = 0;

        // notify Android if SIM lock state has changed
        while (i < nMAX_LOCK_INFO)
        {
            RIL_LOG_INFO("lock:%s state:%d result:%d", lock_info[i].fac, lock_info[i].lock_state,
                    lock_info[i].lock_result);

            if (0 == strncmp(lock_info[i].fac, "PN", 2))
            {
                if (lock_info[i].lock_state == 1 && lock_info[i].lock_result == 1)
                {
                    perso_substate =  RIL_PERSOSUBSTATE_SIM_NETWORK;
                }
                else if (lock_info[i].lock_state == 3 && lock_info[i].lock_result == 2)
                {
                    perso_substate = RIL_PERSOSUBSTATE_SIM_NETWORK_PUK;
                }

                if (RIL_PERSOSUBSTATE_UNKNOWN != perso_substate)
                {
                    SetPersonalisationSubState(perso_substate);
                    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
                }
                break;
            }
            else
            {
                if ((lock_info[i].lock_state == 1 && lock_info[i].lock_result == 1) ||
                        (lock_info[i].lock_state == 3 && lock_info[i].lock_result == 2))
                {
                    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
                    break;
                }
            }

            i++;
        }
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXLOCK() - Exit\r\n");
    return bRet;
}

//
// RIL_REQUEST_GET_CELL_INFO_LIST
//
RIL_RESULT_CODE CTE_XMM6260::CoreGetCellInfoList(REQUEST_DATA& rReqData,
                                                            void* /*pData*/,
                                                            UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CoreGetCellInfoList() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (SCREEN_STATE_OFF != m_cte.GetScreenState() && !m_cte.IsCellInfoCacheEmpty())
    {
        res = RRIL_RESULT_OK_IMMEDIATE;
    }
    else
    {
        if (CopyStringNullTerminate(rReqData.szCmd1, "AT+XCELLINFO?\r", sizeof(rReqData.szCmd1)))
        {
            res = RRIL_RESULT_OK;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::CoreGetCellInfoList() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseCellInfo(P_ND_N_CELL_INFO_DATA pCellData,
                                                    const char* pszRsp,
                                                    UINT32 uiIndex,
                                                    UINT32 uiMode)
{
    RIL_RESULT_CODE res = RIL_E_GENERIC_FAILURE;
    UINT32 uiLAC = 0, uiCI = 0, uiRSSI = 0, uiScramblingCode = 0, uiMcc = 0, uiMnc = 0;
    const char* pszStart = pszRsp;

    //  GSM cells:
    //  +XCELLINFO: 0,<MCC>,<MNC>,<LAC>,<CI>,<RxLev>,<BSIC>,<BCCH_Car>,<true_freq>,<t_advance>
    //  +XCELLINFO: 1,<LAC>,<CI>,<RxLev>,<BSIC>,<BCCH_Car>
    //  one row for each neighboring cell [0..6]
    //  For GSM cells, according to ril.h, must return (LAC/CID , received RSSI)
    //
    //  UMTS FDD cells:
    //  +XCELLINFO: 2,<MCC>,<MNC>,<LAC>,<UCI>,<scrambling_code>,<dl_frequency>,<ul_frequency>
    //  +XCELLINFO: 2,<scrambling_code>,<dl_frequency>,<UTRA_rssi>,<rscp>,<ecno>,<pathloss>
    // If UMTS has any ACTIVE SET neighboring cell
    //  +XCELLINFO: 3,<scrambling_code>,<dl_frequency>,<UTRA_rssi>,<rscp>,<ecno>,<pathloss>
    // One row
    //                          // for each intra-frequency neighboring cell [1..32] for each
    //                          // frequency [0..8] in BA list
    //  For UMTS cells, according to ril.h, must return (Primary scrambling code ,
    //  received signal code power)
    //  NOTE that for first UMTS format above, there is no <rcsp> parameter.
    //
    //  A <type> of 0 or 1 = GSM.  A <type> of 2,3 = UMTS.

    switch (uiMode)
    {
        case 0: // GSM  get (MCC, MNC LAC/CI , RxLev)
        {

            //  Read <MCC>
            if ((!FindAndSkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiMcc, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 0, could not extract MCC value\r\n");
                goto Error;
            }

            //  Read <MNC>
            if ((!FindAndSkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiMnc, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 0, could not extract MNC value\r\n");
                goto Error;
            }

            //  Read <LAC>
            if ((!FindAndSkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiLAC, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 0, could not extract LAC\r\n");
                goto Error;
            }
            //  Read <CI>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiCI, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 0, could not extract CI value\r\n");
                goto Error;
            }
            //  Read <RxLev>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiRSSI, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 0, could not extract RSSI value\r\n");
                goto Error;
            }
            RIL_CellInfo& info = pCellData->aRilCellInfo[uiIndex];
            info.registered = 1;
            info.cellInfoType = RIL_CELL_INFO_TYPE_GSM;
            info.timeStampType = RIL_TIMESTAMP_TYPE_JAVA_RIL;
            info.timeStamp = ril_nano_time();
            info.CellInfo.gsm.signalStrengthGsm.signalStrength = (int)(uiRSSI / 2);
            info.CellInfo.gsm.signalStrengthGsm.bitErrorRate = 0;
            info.CellInfo.gsm.cellIdentityGsm.lac = uiLAC;
            info.CellInfo.gsm.cellIdentityGsm.cid = uiCI;
            info.CellInfo.gsm.cellIdentityGsm.mnc = uiMnc;
            info.CellInfo.gsm.cellIdentityGsm.mcc = uiMcc;
            RIL_LOG_INFO("CTE_XMM6260::ParseCellInfo() -"
                    " mode 0 GSM LAC,CID MNC MCC index=[%d] cid=[%d] lac[%d] mnc[%d] mcc[%d]\r\n",
                    uiIndex, info.CellInfo.gsm.cellIdentityGsm.cid,
                    info.CellInfo.gsm.cellIdentityGsm.lac,
                    info.CellInfo.gsm.cellIdentityGsm.mnc,
                    info.CellInfo.gsm.cellIdentityGsm.mcc);
            res = RRIL_RESULT_OK;
        }
        break;

        case 1: // GSM  get (LAC/CI , RxLev)
        {
            //  <LAC> and <CI> are parameters 2 and 3
            //  Read <LAC> and <CI>
            if (!SkipString(pszRsp, ",", pszRsp) ||
                    !ExtractUInt32(pszRsp, uiLAC, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 1, could not extract LAC\r\n");
                goto Error;
            }
            //  Read <CI>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiCI, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 1, could not extract CI value\r\n");
                goto Error;
            }
            //  Read <RxLev>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiRSSI, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 1, could not extract RSSI value\r\n");
                goto Error;
            }

            RIL_CellInfo& info = pCellData->aRilCellInfo[uiIndex];
            info.registered = 0;
            info.cellInfoType = RIL_CELL_INFO_TYPE_GSM;
            info.CellInfo.gsm.signalStrengthGsm.signalStrength = (int)(uiRSSI / 2);
            info.CellInfo.gsm.signalStrengthGsm.bitErrorRate = 0;
            info.timeStampType = RIL_TIMESTAMP_TYPE_JAVA_RIL;
            info.timeStamp = ril_nano_time();
            info.CellInfo.gsm.cellIdentityGsm.lac = uiLAC;
            info.CellInfo.gsm.cellIdentityGsm.cid = uiCI;
            info.CellInfo.gsm.cellIdentityGsm.mnc = INT_MAX;
            info.CellInfo.gsm.cellIdentityGsm.mcc = INT_MAX;
            RIL_LOG_INFO("CTE_XMM6260::ParseCellInfo() -"
                    " mode 0 GSM LAC,CID MCC MNC index=[%d] cid=[%d] lac[%d] mnc[%d] mcc[%d]\r\n",
                    uiIndex, info.CellInfo.gsm.cellIdentityGsm.cid,
                    info.CellInfo.gsm.cellIdentityGsm.lac,
                    info.CellInfo.gsm.cellIdentityGsm.mnc,
                    info.CellInfo.gsm.cellIdentityGsm.mcc);
            res = RRIL_RESULT_OK;
        }
        break;

        case 2: // UMTS  get (MCC, MNC, scrambling_code , rscp)
        {
            //  This can be either first case or second case.
            //  Loop and count number of commas
            char szBuf[MAX_BUFFER_SIZE] = {0};
            const char* szDummy = pszRsp;
            UINT32 uiCommaCount = 0;
            if (!ExtractUnquotedString(pszRsp, m_cTerminator, szBuf, MAX_BUFFER_SIZE, szDummy))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode 2, could not extract temp buf\r\n");
                goto Error;
            }

            for (UINT32 n=0; n < strlen(szBuf); n++)
            {
                if (szBuf[n] == ',')
                    uiCommaCount++;
            }
            RIL_LOG_INFO("CTE_XMM6260::ParseCellInfo() -"
                    " mode 2, found %d commas\r\n", uiCommaCount);

            if (6 != uiCommaCount)
            {
                //  Handle first case here
                //  +XCELLINFO:
                //      2,<MCC>,<MNC>,<LAC>,<UCI>,<scrambling_code>,<dl_frequency>,<ul_frequency>
                //  +XCELLINFO:
                //       2,<scrambling_code>,<dl_frequency>,<UTRA_rssi>,<rscp>,<ecno>,<pathloss>
                // If UMTS has any ACTIVE SET neighboring cell
                //  +XCELLINFO:
                //       3,<scrambling_code>,<dl_frequency>,<UTRA_rssi>,<rscp>,<ecno>,<pathloss>

                //  Read <MCC>
                if ((!FindAndSkipString(pszRsp, ",", pszRsp)) ||
                        (!ExtractUInt32(pszRsp, uiMcc, pszRsp)))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                            " mode 0, could not extract Mcc value\r\n");
                    goto Error;
                }

                //  Read <MNC>
                if ((!FindAndSkipString(pszRsp, ",", pszRsp)) ||
                        (!ExtractUInt32(pszRsp, uiMnc, pszRsp)))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                            " mode 0, could not extract MNC value\r\n");
                    goto Error;
                }

                //  Read <LAC>
                if ((!FindAndSkipString(pszRsp, ",", pszRsp)) ||
                        (!ExtractUInt32(pszRsp, uiLAC, pszRsp)))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                            " mode 0, could not extract LAC\r\n");
                    goto Error;
                }

                //  Read <CI>
                if ((!SkipString(pszRsp, ",", pszRsp)) ||
                        (!ExtractUInt32(pszRsp, uiCI, pszRsp)))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                            " mode 1, could not extract CI value\r\n");
                    goto Error;
                }

                if ((!FindAndSkipString(pszRsp, ",", pszRsp)) &&
                        (!ExtractUInt32(pszRsp, uiScramblingCode, pszRsp)))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                            " mode 2, could not extract scrambling code\r\n");
                    goto Error;
                }

                RIL_CellInfo& info = pCellData->aRilCellInfo[uiIndex];
                info.registered = 1;
                info.cellInfoType = RIL_CELL_INFO_TYPE_WCDMA;
                info.timeStampType = RIL_TIMESTAMP_TYPE_JAVA_RIL;
                info.timeStamp = ril_nano_time();

                //  rssi = <rscp>
                //  Note that <rscp> value does not exist with this response.
                info.CellInfo.wcdma.signalStrengthWcdma.signalStrength = RSSI_UNKNOWN;
                info.CellInfo.wcdma.signalStrengthWcdma.bitErrorRate = BER_UNKNOWN;
                info.CellInfo.wcdma.cellIdentityWcdma.lac = uiLAC;
                info.CellInfo.wcdma.cellIdentityWcdma.cid = uiCI;
                info.CellInfo.wcdma.cellIdentityWcdma.psc = uiScramblingCode;
                info.CellInfo.wcdma.cellIdentityWcdma.mnc = uiMnc;
                info.CellInfo.wcdma.cellIdentityWcdma.mcc = uiMcc;
                RIL_LOG_INFO("CTE_XMM6260::ParseCellInfo() -"
                        " mode 2 UMTS LAC,CID MCC MNC, ScrCode"
                        " index=[%d]  cid=[%d] lac[%d] mnc[%d] mcc[%d] scrCode[%d]\r\n",
                        uiIndex, info.CellInfo.wcdma.cellIdentityWcdma.cid,
                        info.CellInfo.wcdma.cellIdentityWcdma.lac,
                        info.CellInfo.wcdma.cellIdentityWcdma.mnc,
                        info.CellInfo.wcdma.cellIdentityWcdma.mcc,
                        info.CellInfo.wcdma.cellIdentityWcdma.psc);

                res = RRIL_RESULT_OK;
                break;
            }
            else
            {
                //  fall through to case 3 as it is parsed the same.
                RIL_LOG_INFO("CTE_XMM6260::ParseCellInfo() -"
                        " comma count = 6, drop to case 3\r\n");
                pCellData->aRilCellInfo[uiIndex].registered = 1;
            }
        }


        case 3: // UMTS  get (scrambling_code , rscp)
        {
            //  +XCELLINFO: 2,<scrambling_code>,<dl_frequency>,<UTRA_rssi>,<rscp>,<ecno>,<pathloss>
            // If UMTS has any ACTIVE SET neighboring cell
            //  +XCELLINFO: 3,<scrambling_code>,<dl_frequency>,<UTRA_rssi>,<rscp>,<ecno>,<pathloss>

            int rscp = RSSI_UNKNOWN; // value mapped to rssi
            int dummy = BER_UNKNOWN; // value mapped to ecNo

            //  scrambling_code is parameter 2
            //  Read <scrambling_code>
            if ((!SkipString(pszRsp, ",", pszRsp)) ||
                    (!ExtractUInt32(pszRsp, uiScramblingCode, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode %d, could not extract scrambling code\r\n", uiMode);
                goto Error;
            }
            //  <rscp> is parameter 5
            if (!FindAndSkipString(pszRsp, ",", pszRsp) ||
                    !FindAndSkipString(pszRsp, ",", pszRsp) ||
                    !FindAndSkipString(pszRsp, ",", pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                       " mode %d, could not skip to rscp\r\n", uiMode);
                goto Error;
            }

            //  read <rscp>
            if (!ExtractInt(pszRsp, rscp, pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode %d, could not extract rscp\r\n", uiMode);
                goto Error;
            }

            //  read <ecno> and ignore
            if ((!SkipString(pszRsp, ",", pszRsp)) || (!ExtractInt(pszRsp, dummy, pszRsp)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6260::ParseCellInfo() -"
                        " mode %d, could not extract ecNo\r\n", uiMode);
                goto Error;
            }

            RIL_CellInfo& info = pCellData->aRilCellInfo[uiIndex];
            info.cellInfoType = RIL_CELL_INFO_TYPE_WCDMA;
            info.registered = 0;
            info.timeStampType = RIL_TIMESTAMP_TYPE_JAVA_RIL;
            info.timeStamp = ril_nano_time();
            info.CellInfo.wcdma.signalStrengthWcdma.signalStrength = MapRscpToRssi(rscp);
            info.CellInfo.wcdma.signalStrengthWcdma.bitErrorRate = BER_UNKNOWN;
            info.CellInfo.wcdma.cellIdentityWcdma.lac = INT_MAX;
            info.CellInfo.wcdma.cellIdentityWcdma.cid = INT_MAX;
            info.CellInfo.wcdma.cellIdentityWcdma.psc = uiScramblingCode;
            info.CellInfo.wcdma.cellIdentityWcdma.mnc = INT_MAX;
            info.CellInfo.wcdma.cellIdentityWcdma.mcc = INT_MAX;
            RIL_LOG_INFO("CTE_XMM6260::ParseCellInfo() -"
                    " mode 2/3 UMTS LAC,CID MCC MNC, ScrCode"
                    " index=[%d]  cid=[%d] lac[%d] mnc[%d] mcc[%d] scrCode[%d]\r\n",
                    uiIndex, info.CellInfo.wcdma.cellIdentityWcdma.cid,
                    info.CellInfo.wcdma.cellIdentityWcdma.lac,
                    info.CellInfo.wcdma.cellIdentityWcdma.mnc,
                    info.CellInfo.wcdma.cellIdentityWcdma.mcc,
                    info.CellInfo.wcdma.cellIdentityWcdma.psc);
            res = RRIL_RESULT_OK;
        }
        break;

        default:
        {
            RIL_LOG_INFO("CTE_XMM6260::ParseCellInfo() -"
                    " Invalid nMode=[%d]\r\n", uiMode);
            goto Error;
        }
        break;
    }
Error:
    return res;

}

RIL_RESULT_CODE CTE_XMM6260::ParseXUICC(const char*& pszRsp)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXUICC() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiValue = 0;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXUICC() - Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "<prefix>+XUICC: <state><postfix>"
    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    if (SkipString(pszRsp, "+XUICC: ", pszRsp))
    {
        if (!ExtractUpperBoundedUInt32(pszRsp, 2, uiValue, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXUICC() - Invalid SIM type.\r\n");
        }
        else
        {
            if (1 == uiValue)
            {
                // Set to USIM
                m_CardStatusCache.gsm_umts_subscription_app_index = SIM_USIM_APP_INDEX;
                m_CardStatusCache.applications[SIM_USIM_APP_INDEX].app_type = RIL_APPTYPE_USIM;
            }
            else if (0 == uiValue)
            {
                // Set to SIM
                m_CardStatusCache.gsm_umts_subscription_app_index = SIM_USIM_APP_INDEX;
                m_CardStatusCache.applications[SIM_USIM_APP_INDEX].app_type = RIL_APPTYPE_SIM;
            }
        }

        SkipRspEnd(pszRsp, m_szNewLine, pszRsp);
    }

    res = RRIL_RESULT_OK;
Error:
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseXPINCNT(const char*& pszRsp)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseXPINCNT() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXPINCNT() - Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "<prefix>+XPINCNT: <PIN attempts>, <PIN2 attempts>,
    // <PUK attempts>, <PUK2 attempts><postfix>"
    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    if (SkipString(pszRsp, "+XPINCNT: ", pszRsp))
    {
        UINT32 uiPin1 = 0, uiPin2 = 0, uiPuk1 = 0, uiPuk2 = 0;

        if (!ExtractUInt32(pszRsp, uiPin1, pszRsp) ||
            !SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiPin2, pszRsp) ||
            !SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiPuk1, pszRsp) ||
            !SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiPuk2, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseXPINCNT() - Cannot parse XPINCNT\r\n");
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
            // Set pin retries to -1 (unknown)
            for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
            {
                m_CardStatusCache.applications[i].pin1_num_retries = -1;
                m_CardStatusCache.applications[i].puk1_num_retries = -1;
                m_CardStatusCache.applications[i].pin2_num_retries = -1;
                m_CardStatusCache.applications[i].puk2_num_retries = -1;
            }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED
        }
        else
        {
            RIL_LOG_INFO("CTE_XMM6260::ParseXPINCNT() -"
                    " retries pin1:%d pin2:%d puk1:%d puk2:%d\r\n",
                    uiPin1, uiPin2, uiPuk1, uiPuk2);

#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
            for (int i = 0 ; i < m_CardStatusCache.num_applications; i++)
            {
                m_CardStatusCache.applications[i].pin1_num_retries = uiPin1;
                m_CardStatusCache.applications[i].puk1_num_retries = uiPuk1;
                m_CardStatusCache.applications[i].pin2_num_retries = uiPin2;
                m_CardStatusCache.applications[i].puk2_num_retries = uiPuk2;
            }
#endif // M2_PIN_RETRIES_FEATURE_ENABLED

            m_PinRetryCount.pin = uiPin1;
            m_PinRetryCount.pin2 = uiPin2;
            m_PinRetryCount.puk = uiPuk1;
            m_PinRetryCount.puk2 = uiPuk2;
        }
        SkipRspEnd(pszRsp, m_szNewLine, pszRsp);
    }

    res = RRIL_RESULT_OK;
Error:
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseCCID(const char*& pszRsp)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseCCID() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseCCID() - Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "<prefix>+CCID: <ICCID><postfix>"
    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    if (SkipString(pszRsp, "+CCID: ", pszRsp))
    {
        if (!ExtractUnquotedString(pszRsp, m_cTerminator, m_szUICCID, PROPERTY_VALUE_MAX, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseCCID() - Cannot parse UICC ID\r\n");
            m_szUICCID[0] = '\0';
        }

        SkipRspEnd(pszRsp, m_szNewLine, pszRsp);
    }

    res = RRIL_RESULT_OK;
Error:
    return res;
}

RIL_RESULT_CODE CTE_XMM6260::ParseCUAD(const char*& pszRsp)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseCUAD() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char* pszResponseString = NULL;
    UINT32 uiResponseStringLen = 0;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseCUAD() - Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "<prefix>+CUAD: <response><postfix>"
    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    if (SkipString(pszRsp, "+CUAD: ", pszRsp))
    {
        if (!ExtractQuotedStringWithAllocatedMemory(pszRsp, pszResponseString,
                uiResponseStringLen, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::ParseCUAD() - Could not parse CUAD response\r\n");
            goto Error;
        }

        if (NULL == pszResponseString || 0 >= uiResponseStringLen
                || (0 != (uiResponseStringLen - 1) % 2))
        {
            RIL_LOG_INFO("CTE_XMM6260::ParseCUAD() - Invalid response\r\n");
            goto Error;
        }

        // Reduce by 1 for response string length
        uiResponseStringLen--;

        if (!ParseEFdir(pszResponseString, uiResponseStringLen))
        {
            RIL_LOG_INFO("CTE_XMM6260::ParseCUAD() - ParseEFdir failed\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;
Error:
    if (NULL != pszRsp)
    {
        FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp);
    }

    delete[] pszResponseString;
    pszResponseString = NULL;

    RIL_LOG_VERBOSE("CTE_XMM6260::ParseCUAD() - Exit\r\n");
    return res;
}

void CTE_XMM6260::QueryActiveApplicationType()
{
    RIL_LOG_VERBOSE("CTE_XMM6260::QueryActiveApplicationType() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_GET_SIM_STATUS].uiChannel,
            NULL, RIL_REQUEST_GET_SIM_STATUS, "AT+XUICC?\r",
            &CTE::ParseQueryActiveApplicationType);

    if (NULL == pCmd)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::QueryActiveApplicationType() - "
                "Unable to allocate memory for new command!\r\n");
    }
    else
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::QueryActiveApplicationType() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::QueryActiveApplicationType() - Exit\r\n");
}

RIL_RESULT_CODE CTE_XMM6260::ParseQueryActiveApplicationType(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryActiveApplicationType() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    res = ParseXUICC(pszRsp);
    if (RRIL_RESULT_OK != res)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryActiveApplicationType() - "
                "Could not parse XUICC\r\n");
        goto Error;
    }

    if (m_CardStatusCache.gsm_umts_subscription_app_index == SIM_USIM_APP_INDEX)
    {
        m_CardStatusCache.num_applications = 1;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryActiveApplicationType() - Exit\r\n");
    return res;
}

void CTE_XMM6260::QueryAvailableApplications()
{
    RIL_LOG_VERBOSE("CTE_XMM6260::QueryAvailableApplications() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_GET_SIM_STATUS].uiChannel,
            NULL, RIL_REQUEST_GET_SIM_STATUS, "AT+CUAD\r",
            &CTE::ParseQueryAvailableApplications);

    if (NULL == pCmd)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::QueryAvailableApplications() - "
                "Unable to allocate memory for new command!\r\n");
    }
    else
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::QueryAvailableApplications() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::QueryAvailableApplications() - Exit\r\n");
}

RIL_RESULT_CODE CTE_XMM6260::ParseQueryAvailableApplications(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryAvailableApplications() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    res = ParseCUAD(pszRsp);
    if (RRIL_RESULT_OK != res)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryAvailableApplications() - "
                "Could not parse CUAD\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryAvailableApplications() - Exit\r\n");
    return res;
}

void CTE_XMM6260::QueryIccId()
{
    RIL_LOG_VERBOSE("CTE_XMM6260::QueryIccId() - Enter\r\n");

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_GET_SIM_STATUS].uiChannel,
            NULL, RIL_REQUEST_GET_SIM_STATUS, "AT+CCID\r", &CTE::ParseQueryIccId);

    if (NULL == pCmd)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::QueryIccId() - "
                "Unable to allocate memory for new command!\r\n");
    }
    else
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::QueryIccId() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM6260::QueryIccId() - Exit\r\n");
}

RIL_RESULT_CODE CTE_XMM6260::ParseQueryIccId(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryIccId() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    res = ParseCCID(pszRsp);
    if (RRIL_RESULT_OK != res)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseQueryIccId() - Could not parse CCID\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6260::ParseQueryIccId() - Exit\r\n");
    return res;
}

UINT32 CTE_XMM6260::GetXDNSMode(const char* pszPdpType)
{
    UINT32 uiMode = 3;

    if (NULL == pszPdpType || '\0' == pszPdpType[0] || (0 == strcmp(pszPdpType, PDPTYPE_IPV4V6)))
    {
        uiMode = 3;
    }
    else if (0 == strcmp(pszPdpType, PDPTYPE_IP))
    {
        uiMode = 1;
    }
    else if (0 == strcmp(pszPdpType, PDPTYPE_IPV6))
    {
        uiMode = 2;
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetXDNSMode() - PdpType: %s not supported\r\n", pszPdpType);
    }

    return uiMode;
}

RIL_RESULT_CODE CTE_XMM6260::GetOemVersion(REQUEST_DATA& reqData)
{
    P_ND_OEM_VERSION pResponse = NULL;

    pResponse = (P_ND_OEM_VERSION) malloc(sizeof(S_ND_OEM_VERSION));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::GetOemVersion() - "
                "Could not allocate memory for response");
        return RRIL_RESULT_ERROR;
    }

    memset(pResponse, 0, sizeof(S_ND_OEM_VERSION));
    PrintStringNullTerminate(pResponse->szVersion, sizeof(pResponse->szVersion),
            "%d", GetOemVersion());

    pResponse->sResponsePointer.pszVersion = pResponse->szVersion;

    reqData.pContextData2  = (void*)pResponse;
    reqData.cbContextData2 = sizeof(S_ND_OEM_VERSION_PTR);

    return RRIL_RESULT_OK_IMMEDIATE;
}

RIL_RESULT_CODE CTE_XMM6260::CreateGetThermalSensorValuesV2Req(REQUEST_DATA& /*reqData*/,
        const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTE_XMM6260::CreateActivateThermalSensorV2Ind(REQUEST_DATA& /*reqData*/,
        const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTE_XMM6260::CreateRegStatusAndBandInd(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateRegStatusAndBandInd() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (uiDataSize < (2 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateRegStatusAndBandInd() :"
                " received data size is not enough to process the request\r\n");
        goto Error;
    }

    if (strcmp(ppszRequest[1], "true") == 0)
    {
        m_bRegStatusAndBandIndActivated = TRUE;
    }
    else if (strcmp(ppszRequest[1], "false") == 0)
    {
        m_bRegStatusAndBandIndActivated = FALSE;
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateRegStatusAndBandInd() :"
                " invalid data received(expected - true or false)\r\n");
        goto Error;
    }

    if (E_MMGR_EVENT_MODEM_UP != m_cte.GetLastModemEvent())
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateRegStatusAndBandInd() - Modem not ready\r\n");
        goto Error;
    }

    if (m_bRegStatusAndBandIndActivated)
    {
        if (!CopyStringNullTerminate(reqData.szCmd1, "AT+XREG=3;+XREG?\r",
                sizeof(reqData.szCmd1)))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateRegStatusAndBandInd() -"
                   " Cannot construct szCmd1.\r\n");
            goto Error;
        }
    }
    else if (SCREEN_STATE_OFF == m_cte.GetScreenState())
    {
        /*
         * Currently, XREG URC is also used for registration status reporting. So, disable
         * XREG URC only when the screen is off.
         */
        if (!CopyStringNullTerminate(reqData.szCmd1, "AT+XREG=0\r",
                sizeof(reqData.szCmd1)))
        {
            RIL_LOG_CRITICAL("CTE_XMM6260::CreateRegStatusAndBandInd() -"
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
    RIL_LOG_VERBOSE("CTE_XMM6260::CreateRegStatusAndBandInd() - Exit\r\n");
    return res;
}

int CTE_XMM6260::MapRscpToRssi(int rscp)
{
    int rssi = 31;

    // Maps the rscp values(0..96, 255) to rssi(0..31, 99) values
    if (rscp == 255)
    {
        rssi = RSSI_UNKNOWN;
    }
    else if (rscp <= 7)
    {
        rssi = 0;
    }
    else if (rscp <= 67)
    {
        rssi = (rscp - 6) / 2;
    }

    return rssi;
}

RIL_RESULT_CODE CTE_XMM6260::CreateGetGprsCellEnvReq(REQUEST_DATA& reqData)
{
    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    if (!CopyStringNullTerminate(reqData.szCmd1, "AT+CGED=0\r", sizeof(reqData.szCmd1)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::CreateGetGprsCellEnvReq() - Can't construct szCmd1.\r\n");
        res = RRIL_RESULT_ERROR;
    }

    return res;
}

const char* CTE_XMM6260::GetEnableFetchingString()
{
    /*
     * STK is disabled by sending all bytes of terminal profile set to 0.
     * This is already taken care as part of nvm configuration file. In order
     * to get the EAP-SIM authentication working, rapid ril needs to enable
     * proactive fetching. By sending AT+XSATK=1,0, proactive fetching is enabled
     * but the STK URCs are disabled.
     */

     const char* pszEnableFetching = NULL;
     if (!m_cte.IsStkCapable())
     {
        pszEnableFetching = "|+XSATK=1,0";
     }

    return pszEnableFetching;
}

const char* CTE_XMM6260::GetSiloVoiceURCInitString()
{
    const char* pszInit = NULL;
    if (m_cte.IsVoiceCapable())
    {
        if (m_cte.IsDataCapable())
        {
            pszInit = "|+XCALLSTAT=1|+CSSN=1,1|+CNAP=1|+CLIP=1";
        }
        else
        {
            pszInit = "|+XCALLSTAT=1|+XCGCLASS=\"CC\"|+CSSN=1,1|+CNAP=1|+CLIP=1";
        }
    }
    else
    {
        pszInit = "|+XCGCLASS=\"CG\"|+XCONFIG=3,0";
    }
    return pszInit;
}

