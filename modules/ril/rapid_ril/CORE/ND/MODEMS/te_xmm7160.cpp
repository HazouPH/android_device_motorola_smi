////////////////////////////////////////////////////////////////////////////
// te_xmm7160.cpp
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Overlay for the IMC 7160 modem
//
/////////////////////////////////////////////////////////////////////////////

#include <wchar.h>
#include <math.h>

//  This is for socket-related calls.
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/gsmmux.h>
#include <cutils/properties.h>

#include "types.h"
#include "nd_structs.h"
#include "util.h"
#include "extract.h"
#include "rillog.h"
#include "te.h"
#include "sync_ops.h"
#include "command.h"
#include "te_xmm7160.h"
#include "rildmain.h"
#include "callbacks.h"
#include "oemhookids.h"
#include "repository.h"
#include "reset.h"
#include "data_util.h"
#include "init7160.h"

CTE_XMM7160::CTE_XMM7160(CTE& cte)
: CTE_XMM6360(cte)
{
    m_bNeedGetInfoOnCellChange = false;
}

CTE_XMM7160::~CTE_XMM7160()
{
}

CInitializer* CTE_XMM7160::GetInitializer()
{
    RIL_LOG_VERBOSE("CTE_XMM7160::GetInitializer() - Enter\r\n");
    CInitializer* pRet = NULL;

    RIL_LOG_INFO("CTE_XMM7160::GetInitializer() - Creating CInit7160 initializer\r\n");
    m_pInitializer = new CInit7160();
    if (NULL == m_pInitializer)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::GetInitializer() - Failed to create a CInit7160 "
                "initializer!\r\n");
        goto Error;
    }

    pRet = m_pInitializer;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::GetInitializer() - Exit\r\n");
    return pRet;
}

char* CTE_XMM7160::GetBasicInitCommands(UINT32 uiChannelType)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::GetBasicInitCommands() - Enter\r\n");

    char szInitCmd[MAX_BUFFER_SIZE] = {'\0'};
    char* pInitCmd = NULL;

    pInitCmd = CTE_XMM6360::GetBasicInitCommands(uiChannelType);
    if (NULL == pInitCmd)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::GetBasicInitCommands() - Failed to get the "
                "basic init cmd string!\r\n");
        goto Done;
    }

    // Add 7160-specific init commands to ATCMD channel
    if (RIL_CHANNEL_ATCMD == uiChannelType)
    {
        UINT32 uiModeOfOperation = m_cte.IsVoiceCapable()
                ? MODE_CS_PS_VOICE_CENTRIC
                : MODE_CS_PS_DATA_CENTRIC;

        // Set mode of operation for when the modem is LTE camped
        // Note: no need to check for content of pInitCmd as init string code supports spurious "|"
        PrintStringNullTerminate(szInitCmd, sizeof(szInitCmd), "%s|+CEMODE=%u",
                pInitCmd, uiModeOfOperation);
        free(pInitCmd);
        pInitCmd = strdup(szInitCmd);
    }

Done:
    RIL_LOG_VERBOSE("CTE_XMM7160::GetBasicInitCommands() - Exit\r\n");
    return pInitCmd;
}

char* CTE_XMM7160::GetUnlockInitCommands(UINT32 uiChannelType)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::GetUnlockInitCommands() - Enter\r\n");

    char szInitCmd[MAX_BUFFER_SIZE] = {'\0'};
    char* pUnlockInitCmd = NULL;

    pUnlockInitCmd = CTE_XMM6360::GetUnlockInitCommands(uiChannelType);
    if (pUnlockInitCmd != NULL)
    {
        // copy base class init command
        CopyStringNullTerminate(szInitCmd, pUnlockInitCmd, sizeof(szInitCmd));
        free(pUnlockInitCmd);
    }

    if (RIL_CHANNEL_URC == uiChannelType)
    {
        char szConformanceProperty[PROPERTY_VALUE_MAX] = {'\0'};
        BOOL bConformance = FALSE;
        // read the conformance property
        property_get("persist.conformance", szConformanceProperty, NULL);
        bConformance =
                (0 == strncmp(szConformanceProperty, "true", PROPERTY_VALUE_MAX)) ? TRUE : FALSE;

        // read the property enabling ciphering
        CRepository repository;
        int uiEnableCipheringInd = 1;
        if (!repository.Read(g_szGroupModem, g_szEnableCipheringInd, uiEnableCipheringInd))
        {
            RIL_LOG_VERBOSE("CTE_XMM7160::GetUnlockInitCommands()- Repository read failed!\r\n");
        }

        ConcatenateStringNullTerminate(szInitCmd, sizeof(szInitCmd),
                (bConformance || (uiEnableCipheringInd != 0)) ? "|+XUCCI=1" : "|+XUCCI=0");
    }

    if (RIL_CHANNEL_DLC6 == uiChannelType)
    {
        // Enabling ETWS based on pws property
        char szEtwsSupport[PROPERTY_VALUE_MAX] = {'\0'};
        property_get("persist.pws_support", szEtwsSupport, "3");
        if (1 == strlen(szEtwsSupport)
                && (szEtwsSupport[0] == '3' || szEtwsSupport[0] == '2'))
        {
            if (!ConcatenateStringNullTerminate(szInitCmd, sizeof(szInitCmd),
                    GetEnablingEtwsString()))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::GetUnlockInitCommands()- Can't add ETWS string\r\n");
            }
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::GetUnlockInitCommands() - Exit\r\n");
    return strndup(szInitCmd, strlen(szInitCmd));
}

const char* CTE_XMM7160::GetRegistrationInitString()
{
    return "+CREG=3|+XREG=3|+CEREG=3";
}

const char* CTE_XMM7160::GetPsRegistrationReadString()
{
    return "AT+CEREG=3;+XREG=3;+CEREG?;+XREG?;+CEREG=0;+XREG=0;\r";
}

const char* CTE_XMM7160::GetLocationUpdateString(BOOL bIsLocationUpdateEnabled)
{
    if (bIsLocationUpdateEnabled)
    {
        return "AT+CREG=3;+CEREG=3\r";
    }
    else
    {
        return (SCREEN_STATE_ON == m_cte.GetScreenState())
                ? "AT+CREG=3;+CEREG=3\r"
                : "AT+CREG=1;+CEREG=1\r";
    }
}

const char* CTE_XMM7160::GetScreenOnString()
{
    if (m_cte.IsSignalStrengthReportEnabled())
    {
        return "AT+CREG=3;+CGREG=0;+XREG=3;+CEREG=3;+XCESQ=1\r";
    }
    return "AT+CREG=3;+CGREG=0;+XREG=3;+CEREG=3\r";
}

const char* CTE_XMM7160::GetScreenOffString()
{
    char szScreenOffString[MAX_BUFFER_SIZE] = {'\0'};

    if (m_cte.IsLocationUpdatesEnabled())
    {
        CopyStringNullTerminate(szScreenOffString,
                m_cte.IsSignalStrengthReportEnabled()
                ? "AT+CGREG=1;+CEREG=1;+XCESQ=0"
                : "AT+CGREG=1;+CEREG=1",
                sizeof(szScreenOffString));
    }
    else
    {
        CopyStringNullTerminate(szScreenOffString,
                m_cte.IsSignalStrengthReportEnabled()
                ? "AT+CREG=1;+CGREG=1;+CEREG=1;+XCESQ=0"
                : "AT+CREG=1;+CGREG=1;+CEREG=1",
                sizeof(szScreenOffString));
    }

    ConcatenateStringNullTerminate(szScreenOffString, sizeof(szScreenOffString),
            m_bRegStatusAndBandIndActivated ? "\r" : ";+XREG=0\r");

    return strndup(szScreenOffString, strlen(szScreenOffString));
}

const char* CTE_XMM7160::GetSignalStrengthReportingStringAlloc()
{
    return strdup("+XCESQ=1");
}

const char* CTE_XMM7160::GetEnablingEtwsString()
{
    return "|+XETWCFG=1,1,0,0";
}

const char* CTE_XMM7160::GetReadCellInfoString()
{
    return "AT+XMCI=\r";
}

//
// RIL_REQUEST_SETUP_DATA_CALL
//
RIL_RESULT_CODE CTE_XMM7160::CoreSetupDataCall(REQUEST_DATA& rReqData,
       void* pData, UINT32 uiDataSize, UINT32& uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetupDataCall() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char szIPV4V6[] = "IPV4V6";
    int papChap = 0; // no auth
    PdpData stPdpData;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    CChannel_Data* pChannelData = NULL;
    int dataProfile = -1;
    int emergencyFlag = 0; // 1: emergency pdn
    int requestPcscfFlag = 0; // 1: request pcscf address
    int imCnSignallingFlagInd = 0; // 1: IMS Only APN
    UINT32 uiDnsMode = 0;

    RIL_LOG_INFO("CTE_XMM7160::CoreSetupDataCall() - uiDataSize=[%u]\r\n", uiDataSize);

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
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetupDataCall() - "
                "****** No free data channels available ******\r\n");
        goto Error;
    }

    pDataCallContextData->uiCID = uiCID;

    RIL_LOG_INFO("CTE_XMM7160::CoreSetupDataCall() - "
            "stPdpData.szRadioTechnology=[%s], stPdpData.szRILDataProfile=[%s], "
            "stPdpData.szApn=[%s], stPdpData.szUserName=[%s], stPdpData.szPassword=[%s], "
            "stPdpData.szPAPCHAP=[%s]\r\n",
            stPdpData.szRadioTechnology, stPdpData.szRILDataProfile,
            stPdpData.szApn, stPdpData.szUserName, stPdpData.szPassword,
            stPdpData.szPAPCHAP);

    // if user name is empty, always use no authentication
    if (stPdpData.szUserName == NULL || strlen(stPdpData.szUserName) == 0)
    {
        papChap = 0;    // No authentication
    }
    else
    {
        // PAP/CHAP auth type 3 (PAP or CHAP) is not supported. In this case if a
        // a username is defined we will use PAP for authentication.
        // Note: due to an issue in the Android/Fw (missing check of the username
        // length), if the authentication is not defined, it's the value 3 (PAP or
        // CHAP) which is sent to RRIL by default.
        papChap = atoi(stPdpData.szPAPCHAP);
        if (papChap == 3)
        {
            papChap = 1;    // PAP authentication

            RIL_LOG_INFO("CTE_XMM7160::CoreSetupDataCall() - New PAP/CHAP=[%d]\r\n", papChap);
        }
    }

    if (RIL_VERSION >= 4 && (uiDataSize >= (7 * sizeof(char*))))
    {
        stPdpData.szPDPType = ((char**)pData)[6]; // new in Android 2.3.4.
        RIL_LOG_INFO("CTE_XMM7160::CoreSetupDataCall() - stPdpData.szPDPType=[%s]\r\n",
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

    //  dynamic PDP type, need to set XDNS parameter depending on szPDPType.
    //  If not recognized, just use IPV4V6 as default.
    uiDnsMode = GetXDNSMode(stPdpData.szPDPType);

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CGDCONT=%d,\"%s\",\"%s\",,0,0,,%d,%d,%d;+XGAUTH=%d,%u,\"%s\",\"%s\";+XDNS=%d,%u\r",
            uiCID, stPdpData.szPDPType, stPdpData.szApn, emergencyFlag, requestPcscfFlag,
            imCnSignallingFlagInd, uiCID, papChap, stPdpData.szUserName, stPdpData.szPassword,
            uiCID, uiDnsMode))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSetupDataCall() -"
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
        pChannelData->SetApn(stPdpData.szApn);

        rReqData.pContextData = (void*)pDataCallContextData;
        rReqData.cbContextData = sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA);
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetupDataCall() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIGNAL_STRENGTH
//
RIL_RESULT_CODE CTE_XMM7160::CoreSignalStrength(REQUEST_DATA& rReqData,
        void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSignalStrength() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+XCESQ?\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSignalStrength() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseSignalStrength(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseSignalStrength() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    RIL_SignalStrength* pSigStrData = NULL;

    const char* pszRsp = rRspData.szResponse;

    pSigStrData = ParseXCESQ(pszRsp, FALSE);
    if (NULL == pSigStrData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseSignalStrength() -"
                " Could not allocate memory for RIL_SignalStrength.\r\n");
        goto Error;
    }

    rRspData.pData   = (void*)pSigStrData;
    rRspData.uiDataSize  = sizeof(RIL_SignalStrength);
    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseSignalStrength() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_DATA_REGISTRATION_STATE
//
RIL_RESULT_CODE CTE_XMM7160::ParseGPRSRegistrationState(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGPRSRegistrationState() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    const char* pszDummy;

    S_ND_GPRS_REG_STATUS psRegStatus;
    P_ND_GPRS_REG_STATUS pGPRSRegStatus = NULL;

    pGPRSRegStatus = (P_ND_GPRS_REG_STATUS)malloc(sizeof(S_ND_GPRS_REG_STATUS));
    if (NULL == pGPRSRegStatus)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseGPRSRegistrationState() -"
                " Could not allocate memory for a S_ND_GPRS_REG_STATUS struct.\r\n");
        goto Error;
    }
    memset(pGPRSRegStatus, 0, sizeof(S_ND_GPRS_REG_STATUS));

    if (FindAndSkipString(pszRsp, "+CEREG: ", pszDummy))
    {
        if (!m_cte.ParseCEREG(pszRsp, FALSE, psRegStatus))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGPRSRegistrationState() - "
                    "ERROR in parsing CEREG response.\r\n");
            goto Error;
        }

        if (strcmp(psRegStatus.szStat, UNREGISTERED_SEARCHING))
        {
            m_cte.StoreRegistrationInfo(&psRegStatus, E_REGISTRATION_TYPE_CEREG);
        }
    }

    if (FindAndSkipString(pszRsp, "+XREG: ", pszDummy))
    {
        if (!m_cte.ParseXREG(pszRsp, FALSE, psRegStatus))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGPRSRegistrationState() - "
                    "ERROR in parsing XREG response.\r\n");
            goto Error;
        }

        if (strcmp(psRegStatus.szStat, UNREGISTERED_SEARCHING))
        {
            m_cte.StoreRegistrationInfo(&psRegStatus, E_REGISTRATION_TYPE_XREG);
        }
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

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGPRSRegistrationState() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_DEACTIVATE_DATA_CALL
//
RIL_RESULT_CODE CTE_XMM7160::CoreDeactivateDataCall(REQUEST_DATA& rReqData,
                                                                void* pData,
                                                                UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreDeactivateDataCall() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszCid = NULL;
    UINT32 uiCID = 0;
    const LONG REASON_RADIO_OFF = 1;
    const LONG REASON_PDP_RESET = 2;
    LONG reason = 0;
    CChannel_Data* pChannelData = NULL;
    UINT32 uiDefaultPdnCid = m_cte.GetDefaultPDNCid();

    if (uiDataSize < (1 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreDeactivateDataCall() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreDeactivateDataCall() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM7160::CoreDeactivateDataCall() - uiDataSize=[%d]\r\n", uiDataSize);

    pszCid = ((char**)pData)[0];
    if (pszCid == NULL || '\0' == pszCid[0])
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreDeactivateDataCall() - pszCid was NULL\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM7160::CoreDeactivateDataCall() - pszCid=[%s]\r\n", pszCid);

    //  Get CID as UINT32.
    if (sscanf(pszCid, "%u", &uiCID) == EOF)
    {
        // Error
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreDeactivateDataCall() -  cannot convert %s to int\r\n",
                pszCid);
        goto Error;
    }

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_VERBOSE("CTE_XMM7160::CoreDeactivateDataCall() -  "
                "no channel found for CID:%u\r\n", uiCID);
        goto Error;
    }

    if ((RIL_VERSION >= 4) && (uiDataSize >= (2 * sizeof(char *))))
    {
        reason = GetDataDeactivateReason(((char**)pData)[1]);
        RIL_LOG_INFO("CTE_XMM7160::CoreDeactivateDataCall() - reason=[%ld]\r\n", reason);
    }

    if (reason == REASON_RADIO_OFF || RIL_APPSTATE_READY != GetSimAppState())
    {
        // complete the request without sending the AT command to modem.
        res = RRIL_RESULT_OK_IMMEDIATE;
        DataConfigDown(uiCID, TRUE);
    }
    else if (reason != REASON_PDP_RESET && m_cte.IsEPSRegistered() && uiCID == uiDefaultPdnCid)
    {
        // complete the request without sending the AT command to modem.
        res = RRIL_RESULT_OK_IMMEDIATE;
        DataConfigDown(uiCID, FALSE);
    }
    else
    {
        res = CTE_XMM6260::CoreDeactivateDataCall(rReqData, pData, uiDataSize);
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreDeactivateDataCall() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
//
RIL_RESULT_CODE CTE_XMM7160::CoreSetPreferredNetworkType(REQUEST_DATA& rReqData,
        void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetPreferredNetworkType() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_PreferredNetworkType networkType = PREF_NET_TYPE_LTE_GSM_WCDMA; //9

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetPreferredNetworkType() - Data "
                "pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != sizeof(RIL_PreferredNetworkType))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                "Invalid data size.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                 "Network type {%d} from framework.\r\n",
                 ((RIL_PreferredNetworkType*)pData)[0]);

    networkType = ((RIL_PreferredNetworkType*)pData)[0];

    // if network type already set, NO-OP this command
    if (m_currentNetworkType == networkType)
    {
        rReqData.szCmd1[0] = '\0';
        res = RRIL_RESULT_OK_IMMEDIATE;
        RIL_LOG_INFO("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                "Network type {%d} already set.\r\n", networkType);
        goto Error;
    }

    switch (networkType)
    {
        case PREF_NET_TYPE_GSM_WCDMA: // WCDMA Preferred
            RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                            "WCDMA pref:XACT=3,1) - Enter\r\n");
            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XACT=3,1\r",
                    sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::HandleNetworkType() - Can't "
                        "construct szCmd1 networkType=%d\r\n", networkType);
                break;
            }
            res = RRIL_RESULT_OK;
            break;

        case PREF_NET_TYPE_GSM_ONLY: // GSM Only
            RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetPreferredNetworkType() -"
                    "GSM only:XACT=0) - Enter\r\n");
            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XACT=0\r",
                    sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::HandleNetworkType() - Can't "
                        "construct szCmd1 networkType=%d\r\n", networkType);
                break;
            }
            res = RRIL_RESULT_OK;
            break;

        case PREF_NET_TYPE_WCDMA: // WCDMA Only
            RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                    "WCDMA only:XACT=1) - Enter\r\n");
            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XACT=1\r",
                    sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::HandleNetworkType() - Can't "
                        "construct szCmd1 networkType=%d\r\n", networkType);
                break;
            }
            res = RRIL_RESULT_OK;
            break;

        case PREF_NET_TYPE_LTE_ONLY: // LTE Only
            RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                    "LTE Only:XACT=2) - Enter\r\n");
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+XACT=2\r"))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                        "Can't construct szCmd1 networkType=%d\r\n", networkType);
                goto Error;
            }
            break;

        /*
         * PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO value is received as a result
         * of the recovery mechanism in the framework.
         */
        case PREF_NET_TYPE_LTE_GSM_WCDMA: // LTE Preferred
        case PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO:
            RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                    "LTE,GSM,WCDMA:XACT=6,2,1) - Enter\r\n");
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+XACT=6,2,1\r"))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                    "Can't construct szCmd1 networkType=%d\r\n", networkType);
                goto Error;
            }
            break;

        case PREF_NET_TYPE_LTE_WCDMA: // LTE Preferred
            RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                    "LTE,WCDMA:XACT=4,2) - Enter\r\n");
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+XACT=4,2\r"))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::HandleNetworkType() - Can't "
                        "construct szCmd1 networkType=%d\r\n", networkType);
                goto Error;
            }
            break;

        default:
            RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                    "Undefined rat code: %d\r\n", networkType);
            res = RIL_E_MODE_NOT_SUPPORTED;
            goto Error;
    }

    //  Set the context of this command to the network type we're attempting to set
    rReqData.pContextData = (void*)networkType;  // Store this as an int.

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetPreferredNetworkType() - "
                    "Exit:%d\r\n", res);
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseGetPreferredNetworkType(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGetPreferredNetworkType() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    UINT32 rat = 0;
    UINT32 pref = 0;

    int* pRat = (int*)malloc(sizeof(int));
    if (NULL == pRat)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetPreferredNetworkType() - Could "
                "not allocate memory for response.\r\n");
        goto Error;
    }

    // Skip "<prefix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetPreferredNetworkType() - Could "
                "not skip response prefix.\r\n");
        goto Error;
    }

    // Skip "+XACT: "
    if (!SkipString(pszRsp, "+XACT: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetPreferredNetworkType() - Could "
                "not skip \"+XACT: \".\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, rat, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetPreferredNetworkType() - Could "
                "not extract rat value.\r\n");
        goto Error;
    }

    if (FindAndSkipString(pszRsp, ",", pszRsp))
    {
        if (!ExtractUInt32(pszRsp, pref, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetPreferredNetworkType() - "
                    "Could not find and skip pref value even though it was expected.\r\n");
            goto Error;
        }
    }

    switch (rat)
    {
        case 0:     // GSM Only
        {
            pRat[0] = PREF_NET_TYPE_GSM_ONLY;
            m_currentNetworkType = PREF_NET_TYPE_GSM_ONLY;
            break;
        }

        case 1:     // WCDMA Only
        {
            pRat[0] = PREF_NET_TYPE_WCDMA;
            m_currentNetworkType = PREF_NET_TYPE_WCDMA;
            break;
        }

        case 2:     // LTE only
        {
            pRat[0] = PREF_NET_TYPE_LTE_ONLY;
            m_currentNetworkType = PREF_NET_TYPE_LTE_ONLY;
            break;
        }

        case 3:     // WCDMA preferred
        {
            pRat[0] = PREF_NET_TYPE_GSM_WCDMA;
            m_currentNetworkType = PREF_NET_TYPE_GSM_WCDMA;
            break;
        }

        case 4:     // LTE/WCDMA, LTE preferred
        {
            pRat[0] = PREF_NET_TYPE_LTE_WCDMA;
            m_currentNetworkType = PREF_NET_TYPE_LTE_WCDMA;
            break;
        }

        case 6:     // LTE/WCDMA/GSM, LTE preferred
        {
            pRat[0] = PREF_NET_TYPE_LTE_GSM_WCDMA;
            m_currentNetworkType = PREF_NET_TYPE_LTE_GSM_WCDMA;
            break;
        }

        default:
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetPreferredNetworkType() - "
                    "Unexpected rat found: %d. Failing out.\r\n", rat);
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

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGetPreferredNetworkType() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_NEIGHBORING_CELL_IDS
//
RIL_RESULT_CODE CTE_XMM7160::CoreGetNeighboringCellIDs(REQUEST_DATA& rReqData,
        void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGetNeighboringCellIDs() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+XMCI=\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGetNeighboringCellIDs() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseGetNeighboringCellIDs(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGetNeighboringCellIDs() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    P_ND_N_CELL_DATA pNeighboringCellData = NULL;
    P_ND_N_CELL_INFO_DATA_V2 pCellInfoData = NULL;
    int nCellInfos = 0;
    int nNeighboringCellInfos = 0;

    pCellInfoData = ParseXMCI(rRspData, nCellInfos);

    if (NULL != pCellInfoData && nCellInfos > 0)
    {
        pNeighboringCellData = (P_ND_N_CELL_DATA)malloc(sizeof(S_ND_N_CELL_DATA));
        if (NULL == pNeighboringCellData)
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetNeighboringCellIDs() -"
                    " Could not allocate memory for a S_ND_N_CELL_DATA struct.\r\n");
            goto Error;
        }
        memset(pNeighboringCellData, 0, sizeof(S_ND_N_CELL_DATA));

        for (int i = 0; i < nCellInfos; i++)
        {
            RIL_CellInfo_v2& info = pCellInfoData->aRilCellInfo[i];
            if (info.registered)
            {
                // Do not report serving cell
                continue;
            }

            int nCellInfoType = info.cellInfoType;
            switch(nCellInfoType)
            {
                case RIL_CELL_INFO_TYPE_GSM_V2:
                    pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos].cid
                            = pNeighboringCellData->aszCellCIDBuffers[nNeighboringCellInfos];

                    // cid = upper 16 bits (LAC), lower 16 bits (CID)
                    snprintf(pNeighboringCellData->aszCellCIDBuffers[nNeighboringCellInfos],
                            CELL_ID_ARRAY_LENGTH, "%04X%04X",
                            info.CellInfo.gsm.cellIdentityGsm.lac,
                            info.CellInfo.gsm.cellIdentityGsm.cid);

                    pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos].rssi
                            = info.CellInfo.gsm.signalStrengthGsm.signalStrength;

                    pNeighboringCellData->apRilNeighboringCell[nNeighboringCellInfos]
                            = &(pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos]);
                    nNeighboringCellInfos++;
                    break;

                case RIL_CELL_INFO_TYPE_WCDMA_V2:
                    pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos].cid
                            = pNeighboringCellData->aszCellCIDBuffers[nNeighboringCellInfos];

                    // cid = upper 16 bits (LAC), lower 16 bits (CID)
                    snprintf(pNeighboringCellData->aszCellCIDBuffers[nNeighboringCellInfos],
                            CELL_ID_ARRAY_LENGTH, "%04X%04X",
                            info.CellInfo.wcdma.cellIdentityWcdma.lac,
                            info.CellInfo.wcdma.cellIdentityWcdma.cid);

                    pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos].rssi
                            = info.CellInfo.wcdma.signalStrengthWcdma.signalStrength;

                    pNeighboringCellData->apRilNeighboringCell[nNeighboringCellInfos]
                            = &(pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos]);
                    nNeighboringCellInfos++;
                    break;

                case RIL_CELL_INFO_TYPE_LTE_V2:
                    pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos].cid
                            = pNeighboringCellData->aszCellCIDBuffers[nNeighboringCellInfos];

                    // cid = upper 16 bits (TAC), lower 16 bits (CID)
                    snprintf(pNeighboringCellData->aszCellCIDBuffers[nNeighboringCellInfos],
                            CELL_ID_ARRAY_LENGTH, "%04X%04X",
                            info.CellInfo.lte.cellIdentityLte.tac,
                            info.CellInfo.lte.cellIdentityLte.ci);

                    pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos].rssi = 0;

                    pNeighboringCellData->apRilNeighboringCell[nNeighboringCellInfos]
                            = &(pNeighboringCellData->aRilNeighboringCell[nNeighboringCellInfos]);
                    nNeighboringCellInfos++;
                    break;

                default:
                    break;
            }
        }
    }

    res = RRIL_RESULT_OK;
Error:
    if (nNeighboringCellInfos > 0 && RRIL_RESULT_OK == res)
    {
        rRspData.pData  = (void*)pNeighboringCellData;
        rRspData.uiDataSize = nNeighboringCellInfos * sizeof(RIL_NeighboringCell*);
    }
    else
    {
        rRspData.pData  = NULL;
        rRspData.uiDataSize = 0;
        free(pNeighboringCellData);
        pNeighboringCellData = NULL;
    }

    free(pCellInfoData);
    pCellInfoData = NULL;

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGetNeighboringCellIDs() - Exit\r\n");
    return res;
}

BOOL CTE_XMM7160::IMSRegister(REQUEST_DATA& rReqData, void* pData,
        UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::IMSRegister() - Enter\r\n");

    BOOL bRet = FALSE;

    if ((NULL == pData) || (sizeof(int) != uiDataSize))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::IMSRegister() - Invalid input data\r\n");
        return bRet;
    }

    int* pService = (int*)pData;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+XICFG=0,1,50,%d;+XIREG=%d\r", *pService, *pService))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::IMSRegister() - Can't construct szCmd1.\r\n");
        return bRet;
    }

    int temp = 0;
    const int DEFAULT_XIREG_TIMEOUT = 180000;
    CRepository repository;

    if (repository.Read(g_szGroupOtherTimeouts, g_szTimeoutWaitForXIREG, temp))
    {
        rReqData.uiTimeout = temp;
    }
    else
    {
        rReqData.uiTimeout = DEFAULT_XIREG_TIMEOUT;
    }

    bRet = TRUE;

    return bRet;
}

RIL_RESULT_CODE CTE_XMM7160::ParseIMSRegister(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseIMSRegister() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseIMSRegister() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::CreateIMSRegistrationReq(REQUEST_DATA& rReqData,
        const char** pszRequest,
        const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateIMSRegistrationReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (pszRequest == NULL || '\0' == pszRequest[1])
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSRegistrationReq() - pszRequest was empty\r\n");
        goto Error;
    }

    if (uiDataSize < (2 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSRegistrationReq() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    int service;
    if (sscanf(pszRequest[1], "%d", &service) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSRegistrationReq() -"
                " cannot convert %s to int\r\n", pszRequest);
        goto Error;
    }

    if ((service < 0) || (service > 1))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSRegistrationReq() -"
                " service %s out of boundaries\r\n", service);
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM7160::CreateIMSRegistrationReq() - service=[%d]\r\n", service);

    if (!IMSRegister(rReqData, &service, sizeof(int)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSRegistrationReq() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateIMSRegistrationReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::CreateIMSConfigReq(REQUEST_DATA& rReqData,
        const char** pszRequest,
        const int nNumStrings)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateIMSConfigReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char szXicfgCmd[MAX_BUFFER_SIZE] = {'\0'};
    int xicfgParams[XICFG_N_PARAMS] = {0};

    if (pszRequest == NULL)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - pszRequest was empty\r\n");
        return res;
    }

    // There should be XICFG_N_PARAMS parameters and the request ID
    if (nNumStrings < (XICFG_N_PARAMS + 1))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() :"
                " received_size < required_size\r\n");
        return res;
    }

    xicfgParams[0] = E_XICFG_IMS_APN;
    xicfgParams[1] = E_XICFG_PCSCF_ADDRESS;
    xicfgParams[2] = E_XICFG_PCSCF_PORT;
    xicfgParams[3] = E_XICFG_IMS_AUTH_MODE;
    xicfgParams[4] = E_XICFG_PHONE_CONTEXT;
    xicfgParams[5] = E_XICFG_LOCAL_BREAKOUT;
    xicfgParams[6] = E_XICFG_XCAP_APN;
    xicfgParams[7] = E_XICFG_XCAP_ROOT_URI;
    xicfgParams[8] = E_XICFG_XCAP_USER_NAME;
    xicfgParams[9] = E_XICFG_XCAP_USER_PASSWORD;

    char szTemp1Xicfg[MAX_BUFFER_SIZE] = {'\0'};
    char szTemp2Xicfg[MAX_BUFFER_SIZE] = {'\0'};
    int nParams = 0;

    for (int i = 1; i <= XICFG_N_PARAMS; i++)
    {
        if ((pszRequest[i] != NULL) && (0 != strncmp(pszRequest[i], "default", 7)))
        {   // The XICFG parameter is a numeric hence "" not required.
            if (xicfgParams[i - 1] == E_XICFG_PCSCF_PORT ||
                xicfgParams[i - 1] == E_XICFG_IMS_AUTH_MODE ||
                xicfgParams[i - 1] == E_XICFG_LOCAL_BREAKOUT)
            {
                if (!PrintStringNullTerminate(szTemp1Xicfg,
                                             MAX_BUFFER_SIZE,",%d,%s",
                                             xicfgParams[i - 1], pszRequest[i]))
                {
                    RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - Can't add %s.\r\n",
                                     pszRequest[i]);
                    goto Error;
                }
            }
            else
            {   // The XICFG parameter is a string hence "" required.
                if (!PrintStringNullTerminate(szTemp1Xicfg,
                                              MAX_BUFFER_SIZE,",%d,\"%s\"",
                                              xicfgParams[i - 1], pszRequest[i]))
                {
                    RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - Can't add %s.\r\n",
                                     pszRequest[i]);
                    goto Error;
                }
            }
            if (!ConcatenateStringNullTerminate(szTemp2Xicfg, sizeof(szTemp2Xicfg), szTemp1Xicfg))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - Can't add %s.\r\n",
                                 szTemp1Xicfg);
                goto Error;
            }
            nParams++;
        }
    }

    if (nParams == 0)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - nParams=0\r\n");
        goto Error;
    }

    if (!ConcatenateStringNullTerminate(szTemp2Xicfg, sizeof(szTemp2Xicfg), "\r"))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - Can't add %s.\r\n",
                         "\r");
        goto Error;
    }

    if (!PrintStringNullTerminate(szXicfgCmd,
            MAX_BUFFER_SIZE,"AT+XICFG=%d,%d", XICFG_SET, nParams))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    if (!ConcatenateStringNullTerminate(szXicfgCmd, sizeof(szXicfgCmd), szTemp2Xicfg))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM7160::CreateIMSConfigReq() - IMS_APN=[%s]\r\n",
                 szXicfgCmd);

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), szXicfgCmd))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateIMSConfigReq() - Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateIMSConfigReq() - Exit\r\n");
    return res;
}

BOOL CTE_XMM7160::GetSetInitialAttachApnReqData(REQUEST_DATA& reqData)
{
    UINT32 uiMode = GetXDNSMode(m_InitialAttachApnParams.szPdpType);
    int requestPcscf = IsImsEnabledApn(m_InitialAttachApnParams.szApn) ? 1 : 0;

    if ('\0' == m_InitialAttachApnParams.szApn[0])
    {
        if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                "AT+CGDCONT=1,\"%s\",,,,,,,%d,%d;+XDNS=1,%u\r",
                m_InitialAttachApnParams.szPdpType, requestPcscf, requestPcscf, uiMode))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::GetSetInitialAttachApnReqData() - "
                    "Can't construct szCmd1.\r\n");
        }
    }
    else
    {
        if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                "AT+CGDCONT=1,\"%s\",\"%s\",,,,,,%d,%d;+XDNS=1,%u\r",
                m_InitialAttachApnParams.szPdpType, m_InitialAttachApnParams.szApn,
                requestPcscf, requestPcscf, uiMode))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::GetSetInitialAttachApnReqData() - "
                    "Can't construct szCmd1.\r\n");
        }
    }

    return TRUE;
}

BOOL CTE_XMM7160::QueryIpAndDns(REQUEST_DATA& rReqData, UINT32 uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::QueryIpAndDns() - Enter\r\n");
    BOOL bRet = FALSE;

    if (uiCID != 0)
    {
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CGCONTRDP=%u\r", uiCID))
        {
            bRet = TRUE;
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::QueryIpAndDns() - Exit\r\n");
    return bRet;
}

RIL_RESULT_CODE CTE_XMM7160::ParseQueryIpAndDns(RESPONSE_DATA& rRspData)
{
    return ParseReadContextParams(rRspData);
}

RIL_RESULT_CODE CTE_XMM7160::HandleSetupDefaultPDN(RIL_Token rilToken,
        CChannel_Data* pChannelData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::HandleSetupDefaultPDN() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char* szModemResourceName = {'\0'};
    int muxControlChannel = -1;
    int hsiChannel = pChannelData->GetHSIChannel();
    int ipcDataChannelMin = 0;
    UINT32 uiRilChannel = pChannelData->GetRilChannel();
    REQUEST_DATA reqData;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    CCommand* pCmd = NULL;

    pDataCallContextData =
            (S_SETUP_DATA_CALL_CONTEXT_DATA*)malloc(sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA));
    if (NULL == pDataCallContextData)
    {
        goto Error;
    }

    pDataCallContextData->uiCID = pChannelData->GetContextID();

    // Get the mux channel id corresponding to the control of the data channel
    switch (uiRilChannel)
    {
        case RIL_CHANNEL_DATA1:
            sscanf(g_szDataPort1, "/dev/gsmtty%d", &muxControlChannel);
            break;
        case RIL_CHANNEL_DATA2:
            sscanf(g_szDataPort2, "/dev/gsmtty%d", &muxControlChannel);
            break;
        case RIL_CHANNEL_DATA3:
            sscanf(g_szDataPort3, "/dev/gsmtty%d", &muxControlChannel);
            break;
        case RIL_CHANNEL_DATA4:
            sscanf(g_szDataPort4, "/dev/gsmtty%d", &muxControlChannel);
            break;
        case RIL_CHANNEL_DATA5:
            sscanf(g_szDataPort5, "/dev/gsmtty%d", &muxControlChannel);
            break;
        default:
            RIL_LOG_CRITICAL("CTE_XMM7160::HandleSetupDefaultPDN() - Unknown mux channel"
                    "for RIL Channel [%u] \r\n", uiRilChannel);
            goto Error;
    }

    szModemResourceName = pChannelData->GetModemResourceName();
    ipcDataChannelMin = pChannelData->GetIpcDataChannelMin();

    if (ipcDataChannelMin > hsiChannel || RIL_MAX_NUM_IPC_CHANNEL <= hsiChannel )
    {
       RIL_LOG_CRITICAL("CTE_XMM7160::HandleSetupDefaultPDN() - Unknown HSI Channel [%d] \r\n",
                hsiChannel);
       goto Error;
    }

    memset(&reqData, 0, sizeof(REQUEST_DATA));

    if (pChannelData->IsRoutingEnabled())
    {
        /*
         * Default PDN is already active and also routing is enabled. Don't send any command to
         * modem. Just bring up the interface.
         */
    }
    else
    {
        if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                "AT+XDATACHANNEL=1,1,\"/mux/%d\",\"/%s/%d\",0,%d;+CGDATA=\"M-RAW_IP\",%d\r",
                muxControlChannel, szModemResourceName, hsiChannel,
                pDataCallContextData->uiCID, pDataCallContextData->uiCID))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::HandleSetupDefaultPDN() - cannot create XDATACHANNEL"
                    "command\r\n");
            goto Error;
        }
    }

    pCmd = new CCommand(uiRilChannel, rilToken, REQ_ID_NONE, reqData,
            &CTE::ParseSetupDefaultPDN, &CTE::PostSetupDefaultPDN);

    if (pCmd)
    {
        pCmd->SetContextData(pDataCallContextData);
        pCmd->SetContextDataSize(sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA));

        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::HandleSetupDefaultPDN() - "
                    "Unable to add command to queue\r\n");
            delete pCmd;
            pCmd = NULL;
            goto Error;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::HandleSetupDefaultPDN() -"
                " Unable to allocate memory for command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pDataCallContextData);
        pDataCallContextData = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::HandleSetupDefaultPDN() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseSetupDefaultPDN(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseSetupDefaultPDN() - Enter\r\n");

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseSetupDefaultPDN() - Exit\r\n");
    return RRIL_RESULT_OK;
}

void CTE_XMM7160::PostSetupDefaultPDN(POST_CMD_HANDLER_DATA& rData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::PostSetupDefaultPDN() Enter\r\n");

    BOOL bSuccess = FALSE;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    UINT32 uiCID = 0;
    CChannel_Data* pChannelData = NULL;

    if (NULL == rData.pContextData ||
            sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != rData.uiContextDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::PostSetupDefaultPDN() - Invalid context data\r\n");
        goto Error;
    }

    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)rData.pContextData;
    uiCID = pDataCallContextData->uiCID;

    if (RIL_E_SUCCESS != rData.uiResultCode)
    {
        RIL_LOG_INFO("CTE_XMM7160::PostSetupDefaultPDN() - Failure\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM7160::PostSetupDefaultPDN() - CID=%d\r\n", uiCID);

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTE_XMM7160::PostSetupDefaultPDN() -"
                " No Data Channel for CID %u.\r\n", uiCID);
        goto Error;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::PostSetupDefaultPDN() set channel data\r\n");

    pChannelData->SetDataState(E_DATA_STATE_ACTIVE);
    pChannelData->SetRoutingEnabled(TRUE);

    if (!SetupInterface(uiCID))
    {
        RIL_LOG_INFO("CTE_XMM7160::PostSetupDefaultPDN() - SetupInterface failed\r\n");
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
}

//
//  Call this whenever data is disconnected
//
BOOL CTE_XMM7160::DataConfigDown(UINT32 uiCID, BOOL bForceCleanup)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::DataConfigDown() - Enter\r\n");

    //  First check to see if uiCID is valid
    if (uiCID == 0)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::DataConfigDown() - Invalid CID = [%u]\r\n", uiCID);
        return FALSE;
    }

    CChannel_Data* pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::DataConfigDown() -"
                " Invalid CID=[%u], no data channel found!\r\n", uiCID);
        return FALSE;
    }

    UINT32 uiDefaultPdnCid = m_cte.GetDefaultPDNCid();
    /*
     * Bring down the interface and bring up the interface again if the data down is for
     * default PDN(EPS registered) and not force cleanup.
     *
     * This is done to make sure packets are flushed out if the data is deactivated
     * in LTE.
     */
    BOOL bStopInterface = !m_cte.IsEPSRegistered() || uiCID != uiDefaultPdnCid || bForceCleanup;
    CMutex* pDataChannelRefCountMutex = m_cte.GetDataChannelRefCountMutex();
    CMutex::Lock(pDataChannelRefCountMutex);
    pChannelData->DecrementRefCount();
    if (pChannelData->GetRefCount() == 0)
    {
        pChannelData->RemoveInterface(!bStopInterface);
    }
    CMutex::Unlock(pDataChannelRefCountMutex);

    if (bStopInterface)
    {
        if (uiCID == uiDefaultPdnCid)
        {
            m_cte.SetDefaultPDNCid(CChannel_Data::GetFirstActiveDataConnectionCid());
        }
        pChannelData->ResetDataCallInfo();
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::DataConfigDown() EXIT\r\n");
    return TRUE;
}

//
// RIL_REQUEST_SET_BAND_MODE
//
RIL_RESULT_CODE CTE_XMM7160::CoreSetBandMode(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetBandMode() - Enter\r\n");
    // TODO: Change to +XACT usage when the modem is ready
    return CTE_XMM6260::CoreSetBandMode(rReqData, pData, uiDataSize);
}

RIL_RESULT_CODE CTE_XMM7160::ParseSetBandMode(RESPONSE_DATA & /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseSetBandMode() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseSetBandMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::CreateSetDefaultApnReq(REQUEST_DATA& rReqData,
        const char** pszRequest, const int numStrings)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateSetDefaultApnReq() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (numStrings != 3)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateSetDefaultApnReq() :"
                " received_size != required_size\r\n");
        return res;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CGDCONT=1,\"%s\",\"%s\"\r", pszRequest[2], pszRequest[1]))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateSetDefaultApnReq() - "
                "Can't construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateSetDefaultApnReq() - Exit\r\n");
    return res;
}

RIL_SignalStrength* CTE_XMM7160::ParseXCESQ(const char*& rszPointer, const BOOL bUnsolicited)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseXCESQ() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    int mode = 0;
    int rxlev = RSSI_UNKNOWN; // received signal strength level
    int ber = BER_UNKNOWN; // channel bit error rate
    int rscp = RSCP_UNKNOWN; // Received signal code power
    // ratio of the received energy per PN chip to the total received power spectral density
    int ecNo = ECNO_UNKNOWN;
    int rsrq = RSRQ_UNKNOWN; // Reference signal received quality
    int rsrp = RSRP_UNKNOWN; // Reference signal received power
    int rssnr = RSSNR_UNKNOWN; // Radio signal strength Noise Ratio value
    RIL_SignalStrength* pSigStrData = NULL;

    if (!bUnsolicited)
    {
        // Parse "<prefix>+XCESQ: <n>,<rxlev>,<ber>,<rscp>,<ecno>,<rsrq>,<rsrp>,<rssnr><postfix>"
        if (!SkipRspStart(rszPointer, m_szNewLine, rszPointer)
                || !SkipString(rszPointer, "+XCESQ: ", rszPointer))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseUnsolicitedSignalStrength() - "
                    "Could not find AT response.\r\n");
            goto Error;
        }

        if (!ExtractInt(rszPointer, mode, rszPointer))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseUnsolicitedSignalStrength() - "
                    "Could not extract <mode>\r\n");
            goto Error;
        }

        if (!SkipString(rszPointer, ",", rszPointer))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() - Could not extract , before <rxlev>\r\n");
            goto Error;
        }
    }

    if (!ExtractInt(rszPointer, rxlev, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() - Could not extract <rxlev>\r\n");
        goto Error;
    }

    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractInt(rszPointer, ber, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() - Could not extract <ber>\r\n");
        goto Error;
    }

    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractInt(rszPointer, rscp, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() - Could not extract <rscp>\r\n");
        goto Error;
    }

    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractInt(rszPointer, ecNo, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() - Could not extract <ecno>\r\n");
        goto Error;
    }

    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractInt(rszPointer, rsrq, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() - Could not extract <rsrq>\r\n");
        goto Error;
    }

    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractInt(rszPointer, rsrp, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() - Could not extract <rsrp>.\r\n");
        goto Error;
    }

    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractInt(rszPointer, rssnr, rszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() - "
                "Could not extract <rssnr>.\r\n");
        goto Error;
    }

    if (!bUnsolicited)
    {
        if (!FindAndSkipRspEnd(rszPointer, m_szNewLine, rszPointer))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() -"
                    " Could not extract the response end.\r\n");
            goto Error;
        }
    }

    pSigStrData = (RIL_SignalStrength*)malloc(sizeof(RIL_SignalStrength));
    if (NULL == pSigStrData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCESQ() -"
                " Could not allocate memory for RIL_SignalStrength.\r\n");
        goto Error;
    }

    // reset to default values
    pSigStrData->GW_SignalStrength.signalStrength = RSSI_UNKNOWN;
    pSigStrData->GW_SignalStrength.bitErrorRate = BER_UNKNOWN;
    pSigStrData->CDMA_SignalStrength.dbm = -1;
    pSigStrData->CDMA_SignalStrength.ecio = -1;
    pSigStrData->EVDO_SignalStrength.dbm = -1;
    pSigStrData->EVDO_SignalStrength.ecio = -1;
    pSigStrData->EVDO_SignalStrength.signalNoiseRatio = -1;
    pSigStrData->LTE_SignalStrength.signalStrength = RSSI_UNKNOWN;
    pSigStrData->LTE_SignalStrength.rsrp = INT_MAX;
    pSigStrData->LTE_SignalStrength.rsrq = INT_MAX;
    pSigStrData->LTE_SignalStrength.rssnr = INT_MAX;
    pSigStrData->LTE_SignalStrength.cqi = INT_MAX;
#if defined (USE_PATCHED_AOSP)
    pSigStrData->WCDMA_SignalStrength.rscp = RSCP_UNKNOWN;
    pSigStrData->WCDMA_SignalStrength.ecNo = ECNO_UNKNOWN;
    pSigStrData->GSM_SignalStrength.rxlev = RSSI_UNKNOWN;
#endif

    /*
     * If the current serving cell is GERAN cell, then <rxlev> and <ber> are set to
     * valid values.
     * For <rxlev>, valid values are 0 to 63.
     * For <ber>, valid values are 0 to 7.
     * If the current service cell is not GERAN cell, then <rxlev> and <ber> are set
     * to value 99.
     *
     * If the current serving cell is UTRA cell, then <rscp> is set to valid value.
     * For <rscp>, valid values are 0 to 96.
     * If the current service cell is not UTRA cell, then <rscp> is set to value 255.
     *
     * If the current serving cell is E-UTRA cell, then <rsrq> and <rsrp> are set to
     * valid values.
     * For <rsrq>, valid values are 0 to 34.
     * For <rsrp>, valid values are 0 to 97.
     * If the current service cell is not E-UTRA cell, then <rsrq> and <rsrp> are set
     * to value 255.
     */
    if (RSSI_UNKNOWN != rxlev)
    {
        pSigStrData->GW_SignalStrength.bitErrorRate = ber;
        // Note: no mapping here as GSM_SignalStrength structure uses range from TS 27.007 8.69
#if defined (USE_PATCHED_AOSP)
        pSigStrData->GSM_SignalStrength.rxlev = rxlev;
#else
        pSigStrData->GW_SignalStrength.signalStrength = MapRxlevToSignalStrengh(rxlev);
#endif
    }
    else if (RSCP_UNKNOWN != rscp)
    {
        pSigStrData->GW_SignalStrength.signalStrength = MapRscpToRssi(rscp);
#if defined (USE_PATCHED_AOSP)
        pSigStrData->WCDMA_SignalStrength.rscp = rscp;
        pSigStrData->WCDMA_SignalStrength.ecNo = ecNo;
#endif
    }
    else if (RSRQ_UNKNOWN != rsrq && RSRP_UNKNOWN != rsrp)
    {
        pSigStrData->LTE_SignalStrength.rsrp = MapToAndroidRsrp(rsrp);
        pSigStrData->LTE_SignalStrength.rsrq = MapToAndroidRsrq(rsrq);
        pSigStrData->LTE_SignalStrength.rssnr = MapToAndroidRssnr(rssnr);
    }
    else
    {
        RIL_LOG_INFO("CTE_XMM7160::ParseXCESQ - "
                "pSigStrData set to default values\r\n");
    }

    res = RRIL_RESULT_OK;
Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pSigStrData);
        pSigStrData = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseXCESQ - Exit()\r\n");
    return pSigStrData;
}

void CTE_XMM7160::QuerySignalStrength()
{
    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_SIGNAL_STRENGTH].uiChannel, NULL,
            RIL_REQUEST_SIGNAL_STRENGTH, "AT+XCESQ?\r", &CTE::ParseUnsolicitedSignalStrength);

    if (pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::QuerySignalStrength() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::QuerySignalStrength() - "
                "Unable to allocate memory for new command!\r\n");
    }
}

//
// RIL_UNSOL_SIGNAL_STRENGTH
//
RIL_RESULT_CODE CTE_XMM7160::ParseUnsolicitedSignalStrength(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseUnsolicitedSignalStrength() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_SignalStrength* pSigStrData = NULL;
    const char* pszRsp = rRspData.szResponse;

    pSigStrData = ParseXCESQ(pszRsp, FALSE);
    if (NULL == pSigStrData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseUnsolicitedSignalStrength() -"
                " parsing failed\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

    RIL_onUnsolicitedResponse(RIL_UNSOL_SIGNAL_STRENGTH, (void*)pSigStrData,
            sizeof(RIL_SignalStrength));

Error:
    free(pSigStrData);
    pSigStrData = NULL;

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseUnsolicitedSignalStrength() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_ISIM_AUTHENTICATE
//
RIL_RESULT_CODE CTE_XMM7160::CoreISimAuthenticate(REQUEST_DATA& rReqData,
                                                                void* pData,
                                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreISimAuthenticate() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char szAutn[AUTN_LENGTH+1]; //32 bytes + null terminated
    char szRand[RAND_LENGTH+1]; //32 bytes + null terminated
    char* pszInput = (char*) pData;
    int nChannelId;
    int nContext;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreISimAuthenticate() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    nChannelId = GetSessionId(RIL_APPTYPE_ISIM);

    if (nChannelId == -1) {
        // If we do not find ISIM, we use default USIM channel, and security context 3G
        nChannelId = 0; // Use default channel ( USIM ) for now. may need to use the one
                        // for ISIM.
        nContext = 1;   // 1 is USIM security context. ( see CAT Spec )
    } else {
        // Found ISIM channel
        nContext = 6; // Security Context for IMS is 6.
    }

    CopyStringNullTerminate(szAutn, pszInput, sizeof(szAutn));
    CopyStringNullTerminate(szRand, pszInput+AUTN_LENGTH, sizeof(szRand));

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+XAUTH=%u,%u,\"%s\",\"%s\"\r", nChannelId, nContext, szRand, szAutn))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreISimAuthenticate() - Cannot create XAUTH command -"
                " szRand=%s, szAutn=%s\r\n",szRand,szAutn);
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreISimAuthenticate() - Exit\r\n");

    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseISimAuthenticate(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseISimAuthenticate() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    int reslen = 0;
    char * pszResult = NULL;
    UINT32 uiStatus;
    char szRes[MAX_BUFFER_SIZE];
    char szCk[MAX_BUFFER_SIZE];
    char szIk[MAX_BUFFER_SIZE];
    char szKc[MAX_BUFFER_SIZE];

    memset(szRes, '\0', sizeof(szRes));
    memset(szCk, '\0', sizeof(szCk));
    memset(szIk, '\0', sizeof(szIk));
    memset(szKc, '\0', sizeof(szKc));

    if (NULL == rRspData.szResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                " Response String pointer is NULL.\r\n");
        goto Error;
    }

    if (FindAndSkipString(pszRsp, "+XAUTH: ", pszRsp))
    {
        if (!ExtractUInt32(pszRsp, uiStatus, pszRsp)) {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                    " Error parsing status.\r\n");
            goto Error;
        }

        if ((uiStatus == 0)||(uiStatus == 1))
        {
            // Success, need to parse the extra parameters...
            if (!SkipString(pszRsp, ",", pszRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                                 " Error parsing status.\r\n");
                goto Error;
            }

            if (!ExtractQuotedString(pszRsp, szRes, sizeof(szRes), pszRsp)) {
                RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                                 " Error parsing Res.\r\n");
                goto Error;
            }

            if (uiStatus == 0)
            {
                // This is success, so we need to get CK, IK, KC
                if (!SkipString(pszRsp, ",", pszRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                                     " Error parsing Res.\r\n");
                    goto Error;
                }

                if (!ExtractQuotedString(pszRsp, szCk, sizeof(szCk), pszRsp)) {
                    RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                                     " Error parsing CK.\r\n");
                    goto Error;
                }
                if (!SkipString(pszRsp, ",", pszRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                                     " Error parsing CK.\r\n");
                    goto Error;
                }

                if (!ExtractQuotedString(pszRsp, szIk, sizeof(szIk), pszRsp)) {
                    RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                                     " Error parsing IK.\r\n");
                    goto Error;
                }
                if (!SkipString(pszRsp, ",", pszRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                                     " Error parsing IK.\r\n");
                    goto Error;
                }

                if (!ExtractQuotedString(pszRsp, szKc, sizeof(szKc), pszRsp)) {
                    RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                                     " Error parsing Kc.\r\n");
                    goto Error;
                }
            }

            // Log the result for debug
            RIL_LOG_VERBOSE("CTE_XMM7160::ParseISimAuthenticate - Res/Auts -"
                            " =%s\r\n", szRes);
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                " Error searching +XAUTH:.\r\n");
        goto Error;
    }

    if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                " Could not extract the response end.\r\n");
        goto Error;
    }

    // reslen = 3 bytes for the status (int), 4 bytes for the :, and the rest + 1 for
    // the carriage return...
    reslen = 3 + 4 + strlen(szRes) + strlen(szCk) + strlen(szIk) + strlen(szKc) + 1;
    pszResult = (char *) malloc(reslen);
    if (!pszResult)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                " Could not allocate memory for result string.\r\n");
        goto Error;
    }
    if (!PrintStringNullTerminate(pszResult, reslen,
                                  "%d:%s:%s:%s:%s", uiStatus, szRes, szCk, szIk, szKc))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseISimAuthenticate() -"
                " Error creating response string.\r\n");
        goto Error;
    }
    rRspData.pData = (void *) pszResult;
    rRspData.uiDataSize = reslen;

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseISimAuthenticate() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::CreateGetThermalSensorValuesReq(REQUEST_DATA& /*reqData*/,
        const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTE_XMM7160::CreateActivateThermalSensorInd(REQUEST_DATA& /*reqData*/,
        const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTE_XMM7160::CreateGetThermalSensorValuesV2Req(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateGetThermalSensorValuesV2Req() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int sensorId;
    char szSensorId[MAX_SENSOR_ID_SIZE] = {0};
    char szFormat[32];

    if (uiDataSize < (2 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateGetThermalSensorValuesV2Req() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    if (NULL == ppszRequest || NULL == ppszRequest[0] || '\0' == ppszRequest[0][0])
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateGetThermalSensorValuesV2Req() -"
                " ppszRequest was NULL\r\n");
        goto Error;
    }

    snprintf(szFormat, sizeof(szFormat), "%%%ds", sizeof(szSensorId) - 1);

    if (sscanf(ppszRequest[1], szFormat, szSensorId) == EOF)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateGetThermalSensorValuesV2Req() -"
                " cannot extract sensor Id\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM7160::CreateGetThermalSensorValuesV2Req() - sensorId=[%s]\r\n",
            szSensorId);

    if (szSensorId[0] != '\0')
    {
        if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                "AT+XTAMR=\"%s\"\r", szSensorId))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CreateGetThermalSensorValuesV2Req() -"
                    " Can't construct szCmd1.\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateGetThermalSensorValuesV2Req() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::CreateActivateThermalSensorV2Ind(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateActivateThermalSensorV2Ind() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int alarmId = 0;
    int tripPointNumber = 0;
    int hysteresis = 0;
    int nVar = 0;
    char szSensorId[MAX_SENSOR_ID_SIZE] = {0};
    char szFormat[32];

    if (uiDataSize < (2 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateActivateThermalSensorV2Ind() :"
                " received data size is not enough to process the request\r\n");
        goto Error;
    }

    snprintf(szFormat, sizeof(szFormat), "%%%ds %%d %%d %%d", sizeof(szSensorId) - 1);

    nVar = sscanf(ppszRequest[1], szFormat, szSensorId, &alarmId, &tripPointNumber, &hysteresis);

    RIL_LOG_INFO("CTE_XMM7160::CreateActivateThermalSensorV2Ind()"
            " sensor Id=[%s], alarmId=[%d], TripPointNumber=[%d], Hysteresis=[%d]\r\n",
            szSensorId, alarmId, tripPointNumber, hysteresis);

    if (hysteresis < 0)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateActivateThermalSensorV2Ind() -"
                " Invalid input\r\n");
        goto Error;
    }

    /*
     * For activating the thermal sensor threshold reached indication, threshold
     * temperatures(trippointnumber,hystereris) needs to be sent as part of the set command.
     *
     * AT+XTSM = <temp_sensor_id>[,AlarmID>,[<TripPointNumber>,<Hysteresis>],[<sampling_period>]]
     */
    switch (nVar)
    {
        case 1:
            if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                    "AT+XTSM=\"%s\"\r", szSensorId))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CreateActivateThermalSensorV2Ind() -"
                        " Can't construct szCmd1.\r\n");
                goto Error;
            }
            break;
        case 4:
            if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                    "AT+XTSM=\"%s\",%d,%d,%d\r", szSensorId, alarmId, tripPointNumber, hysteresis))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CreateActivateThermalSensorV2Ind() -"
                       " Can't construct szCmd1.\r\n");
                goto Error;
            }
            break;
        default:
            goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateActivateThermalSensorV2Ind() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseHookStrings(RESPONSE_DATA & rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseHookStrings() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rspData.szResponse;
    UINT32 uiCommand = (intptr_t)rspData.pContextData;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseHookStrings() - Response string is NULL!\r\n");
        goto Error;
    }

    switch (uiCommand)
    {
        case RIL_OEM_HOOK_STRING_THERMAL_GET_SENSOR_V2:
            res = ParseXTAMR(pszRsp, rspData);
            break;

        case RIL_OEM_HOOK_STRING_THERMAL_SET_THRESHOLD_V2:
            res = ParseXTSM(pszRsp, rspData);
            break;

        default:
            res = CTE_XMM6260::ParseHookStrings(rspData);
            break;
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseHookStrings() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseXTAMR(const char* pszRsp, RESPONSE_DATA& rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseXTAMR() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    P_ND_THERMAL_SENSOR_VALUE pResponse = NULL;
    int temp = 0;
    char szSensorId[MAX_SENSOR_ID_SIZE] = {0};

    // Parse prefix
    if (!FindAndSkipString(pszRsp, "+XTAMR: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXTAMR() - Unable to parse \"+XTSM\" prefix.!\r\n");
        goto Error;
    }

    // Parse <temp_sensor_id>
    if (!ExtractUnquotedString(pszRsp, ",", szSensorId, sizeof(szSensorId), pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXTAMR() - Unable to parse <temp_sensor_id>!\r\n");
        goto Error;
    }

    // Parse <temp>
    if (!SkipString(pszRsp, ",", pszRsp)
            || !ExtractInt(pszRsp, temp, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXTAMR() - Unable to parse <temp>!\r\n");
        goto Error;
    }

    pResponse = (P_ND_THERMAL_SENSOR_VALUE) malloc(sizeof(S_ND_THERMAL_SENSOR_VALUE));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXTAMR() - Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_THERMAL_SENSOR_VALUE));

    PrintStringNullTerminate(pResponse->pszTemperature, sizeof(pResponse->pszTemperature),
            "%d", temp);

    pResponse->sResponsePointer.pszTemperature = pResponse->pszTemperature;

    rspData.pData = (void*)pResponse;
    rspData.uiDataSize = sizeof(S_ND_THERMAL_SENSOR_VALUE_PTR);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseXTAMR() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseXTSM(const char* /*pszRsp*/, RESPONSE_DATA& /*rspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseXTSM() - Enter/Exit\r\n");
        return RRIL_RESULT_OK;
}

RIL_RESULT_CODE CTE_XMM7160::SetCsgAutomaticSelection(REQUEST_DATA& reqData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::SetCsgAutomaticSelection() - Enter\r\n");
    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1), "AT+XCSG=1,0\r"))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::SetCsgAutomaticSelection() -"
                "Cannot construct szCmd1.\r\n");
        return RRIL_RESULT_ERROR;
    }
    RIL_LOG_VERBOSE("CTE_XMM7160::SetCsgAutomaticSelection() - Exit\r\n");
    return RRIL_RESULT_OK;
}

RIL_RESULT_CODE CTE_XMM7160::GetCsgCurrentState(REQUEST_DATA& reqData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::GetCsgCurrentState() - Enter\r\n");
    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1), "AT+XCSG?\r"))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::GetCsgCurrentState() -"
                "Cannot construct szCmd1.\r\n");
        return RRIL_RESULT_ERROR;
    }
    RIL_LOG_VERBOSE("CTE_XMM7160::GetCsgCurrentState() - Exit\r\n");
    return RRIL_RESULT_OK;
}

RIL_RESULT_CODE CTE_XMM7160::ParseXCSG(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseXCSG() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    P_ND_CSG_CURRENT_STATE pResponse
            = (P_ND_CSG_CURRENT_STATE) malloc(sizeof(S_ND_CSG_CURRENT_STATE));

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXCSG() - Could not allocate memory for response.\r\n");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_CSG_CURRENT_STATE));

    CopyStringNullTerminate(pResponse->szCsgCurrentState, pszRsp, MAX_CSG_STATE_SIZE);

    pResponse->sResponsePointer.pszCsgCurrentState = pResponse->szCsgCurrentState;

    rRspData.pData = (void*)pResponse;
    rRspData.uiDataSize = sizeof(S_ND_CSG_CURRENT_STATE_PTR);

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseXCSG() - Exit\r\n");
    return res;
}

//
// Creates 2 vector lists, one for Etws msg Ids and one for Cmas msgIds
//
RIL_RESULT_CODE CTE_XMM7160::FilterSmsCbFromConfig(RIL_GSM_BroadcastSmsConfigInfo** ppConfigInfo,
        const UINT32 uiDataSize,
        android::Vector<RIL_GSM_BroadcastSmsConfigInfo>& vBroadcastSmsConfigInfo,
        CCbsInfo::CbmIds* pConfigIds)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::FilterSmsCbFromConfig() - Enter \r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    int nConfigInfos = 0;
    RIL_GSM_BroadcastSmsConfigInfo selectedConfigInfo;

    nConfigInfos = uiDataSize / sizeof(RIL_GSM_BroadcastSmsConfigInfo**);

    RIL_LOG_INFO("CTE_XMM7160::FilterSmsCbFromConfig() - "
            "nConfigInfos = %d.\r\n", nConfigInfos);

    pConfigIds->cmasIds = 0;
    pConfigIds->etwsIds = 0;

    // Builds a list of SMS CB ranges and a list of Etws ranges
    for (int i = 0; i < nConfigInfos; i++)
    {
        RIL_LOG_VERBOSE("CTE_XMM7160::FilterSmsCbFromConfig() - "
                "CHECK msg Ids Range[%d-%d]\r\n",
                ppConfigInfo[i]->fromServiceId, ppConfigInfo[i]->toServiceId);

        if (ppConfigInfo[i]->selected == false)
        {
            RIL_LOG_WARNING("CTE_XMM7160::FilterSmsCbFromConfig() - "
                    "Range not selected skipping...\r\n");
            continue;
        }

        // Check that given range intersects with ETWS dedicated range [ETWS_FIRST, ETWS_LAST]
        if ((CCbsInfo::ETWS_LAST >= ppConfigInfo[i]->fromServiceId) &&
                (CCbsInfo::ETWS_FIRST <= ppConfigInfo[i]->toServiceId))
        {
            // There are some ETWS ids in the given range
            RIL_LOG_VERBOSE("CTE_XMM7160::FilterSmsCbFromConfig() - "
                    "Contains ETWS Ids\r\n");

            //===<from>===<ETWS_FIRST>====<to>=======<ETWS_LAST>====
            //===<from>===<ETWS_FIRST>====<ETWS_LAST>===<to>========
            //===<ETWS_FIRST>===<from>====<ETWS_LAST>===<to>========
            //===<ETWS_FIRST>===<from>====<to>=========<ETWS_LAST>==
            if (ppConfigInfo[i]->fromServiceId < CCbsInfo::ETWS_FIRST)
            {
                // Need to create a SMS CB range [<from>,<ETWS_FIRST>]
                selectedConfigInfo = *ppConfigInfo[i];
                selectedConfigInfo.toServiceId = CCbsInfo::ETWS_FIRST-1;
                pConfigIds->cmasIds += m_CbsInfo.GetNumberOfIdsFromRange(
                        selectedConfigInfo.fromServiceId,
                        selectedConfigInfo.toServiceId);
                vBroadcastSmsConfigInfo.push_back(selectedConfigInfo);
                if (ppConfigInfo[i]->toServiceId <= CCbsInfo::ETWS_LAST)
                {
                    // Need to create an ETWS range [<ETWS_FIRST>,<to>]
                    selectedConfigInfo = *ppConfigInfo[i];
                    selectedConfigInfo.fromServiceId = CCbsInfo::ETWS_FIRST;
                    pConfigIds->etwsIds += m_CbsInfo.GetNumberOfIdsFromRange(
                            selectedConfigInfo.fromServiceId,
                            selectedConfigInfo.toServiceId);
                    m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.push_back(selectedConfigInfo);
                }
                else // ppConfigInfo[i]->toServiceId > CCbsInfo::ETWS_LAST
                {
                    // Need to create one ETWS range [<ETWS_FIRST>,<ETWS_LAST>]
                    selectedConfigInfo = *ppConfigInfo[i];
                    selectedConfigInfo.fromServiceId = CCbsInfo::ETWS_FIRST;
                    selectedConfigInfo.toServiceId = CCbsInfo::ETWS_LAST;
                    pConfigIds->etwsIds += m_CbsInfo.GetNumberOfIdsFromRange(
                            selectedConfigInfo.fromServiceId,
                            selectedConfigInfo.toServiceId);
                    m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.push_back(selectedConfigInfo);
                    // Need to create one SMS CB range [<ETWS_LAST>+1,<to>]
                    selectedConfigInfo = *ppConfigInfo[i];
                    selectedConfigInfo.fromServiceId = CCbsInfo::ETWS_LAST + 1;
                    pConfigIds->cmasIds += m_CbsInfo.GetNumberOfIdsFromRange(
                            selectedConfigInfo.fromServiceId,
                            selectedConfigInfo.toServiceId);
                    vBroadcastSmsConfigInfo.push_back(selectedConfigInfo);
                }
            }
            else if (ppConfigInfo[i]->toServiceId <= CCbsInfo::ETWS_LAST)
            {
                // Need to create one ETWS range [<from>,<to>]
                pConfigIds->etwsIds += m_CbsInfo.GetNumberOfIdsFromRange(
                        ppConfigInfo[i]->fromServiceId,
                        ppConfigInfo[i]->toServiceId);
                m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.push_back(*ppConfigInfo[i]);
            }
            else // ppConfigInfo[i]->toServiceId > ETWS_LAST
            {
                // Need to create one ETWS range [<from>,<ETWS_LAST>]
                selectedConfigInfo = *ppConfigInfo[i];
                selectedConfigInfo.toServiceId = CCbsInfo::ETWS_LAST;
                pConfigIds->etwsIds += m_CbsInfo.GetNumberOfIdsFromRange(
                        selectedConfigInfo.fromServiceId,
                        selectedConfigInfo.toServiceId);
                m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.push_back(selectedConfigInfo);
                // Need to create an SMS CB range [<ETWS_LAST>+1,<to>]
                selectedConfigInfo = *ppConfigInfo[i];
                selectedConfigInfo.fromServiceId = CCbsInfo::ETWS_LAST + 1;
                pConfigIds->cmasIds += m_CbsInfo.GetNumberOfIdsFromRange(
                        selectedConfigInfo.fromServiceId,
                        selectedConfigInfo.toServiceId);
                vBroadcastSmsConfigInfo.push_back(selectedConfigInfo);
            }
        }
        else
        {
            RIL_LOG_VERBOSE("CTE_XMM7160::FilterSmsCbFromConfig() - "
                                "Contains SMS CB Range, Current:%d\r\n", pConfigIds->cmasIds);

            pConfigIds->cmasIds += m_CbsInfo.GetNumberOfIdsFromRange(ppConfigInfo[i]->fromServiceId,
                    ppConfigInfo[i]->toServiceId);

            vBroadcastSmsConfigInfo.push_back(*(ppConfigInfo[i]));
        }
    }

    return res;
}

//
// RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG
//
RIL_RESULT_CODE CTE_XMM7160::CoreGsmSetBroadcastSmsConfig(REQUEST_DATA& /*reqData*/,
                                                                 void* pData,
                                                                 UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - Enter \r\n");

    int nConfigInfos = 0;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    android::Vector<CCbsInfo::IdRange>::const_iterator smsRange;
    CCbsInfo::CbmIds configIds = {0,0};
    android::Vector<RIL_GSM_BroadcastSmsConfigInfo> vBroadcastSmsConfigInfo;
    android::Vector<RIL_GSM_BroadcastSmsConfigInfo>::const_iterator selectedConfigInfo;
    bool bSkip = false;
    RIL_GSM_BroadcastSmsConfigInfo** ppConfigInfo = (RIL_GSM_BroadcastSmsConfigInfo**)pData;

    m_vBroadcastSmsConfigInfo.clear();
    m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.clear();

    if ((uiDataSize != 0) && ((uiDataSize % sizeof(RIL_GSM_BroadcastSmsConfigInfo**)) != 0 ))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (uiDataSize != 0 && pData == NULL)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    vBroadcastSmsConfigInfo.clear();
    // Filters Etws from Cmas msg ranges, number of config Ids is also calculated (configIds)
    FilterSmsCbFromConfig(ppConfigInfo, uiDataSize, vBroadcastSmsConfigInfo, &configIds);

    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
            "Configuring %d SMS CB (Activated:%d) and %d ETWS message Ids\r\n",
            configIds.cmasIds, m_CbsInfo.m_activatedIds.cmasIds, configIds.etwsIds);

    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
                "SmsCb/Cmas Ranges in config list:%d\r\n",
                vBroadcastSmsConfigInfo.size());

    // If number of activated and newly configured ids are different,
    // then a new config needs to be setup.
    bSkip = (m_CbsInfo.m_activatedIds.cmasIds != configIds.cmasIds);

    // Post process to check if new SMS CB/Cmas ranges intersect with activated ones
    for (selectedConfigInfo = vBroadcastSmsConfigInfo.begin();
            selectedConfigInfo != vBroadcastSmsConfigInfo.end();
            selectedConfigInfo++)
    {
        int msgIds = 0;

        RIL_LOG_INFO("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
                "Analysing Range [%d,%d]\r\n",
                selectedConfigInfo->fromServiceId, selectedConfigInfo->toServiceId);

        // Prepare new range list
        m_vBroadcastSmsConfigInfo.push_back(*selectedConfigInfo);

        // A range has not been found
        if (bSkip)
        {
            // At least one difference causes to activate all configured Ids.
            continue;
        }

        // Checking each current range Id against activated ones
        for (msgIds = selectedConfigInfo->fromServiceId;
                msgIds <= selectedConfigInfo->toServiceId; msgIds++)
        {
            // Verify if an activated range contains the current msg Id
            bool bFound = false;
            for (smsRange = m_CbsInfo.m_vActivatedBroadcastSms.begin();
                    smsRange != m_CbsInfo.m_vActivatedBroadcastSms.end();
                    smsRange++)
            {
                if ((msgIds >= smsRange->minVal) &&
                        (msgIds <= smsRange->maxVal))
                {
                    // Current msg Id is already activated
                    bFound = true;
                    break;
                }
            }

            // Check if msgId was found in activated ranges
            if (bFound == false)
            {
                // Not found then msg id needs to be activated
                break;
            }
        }

        // Check if at least one msd ID was not found
        if (msgIds <= selectedConfigInfo->toServiceId)
        {
            RIL_LOG_INFO("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
                    "A MSG ID NOT FOUND in [%d-%d]\r\n",
                    selectedConfigInfo->fromServiceId, selectedConfigInfo->toServiceId);
            bSkip = true;
        }
        else
        {
            RIL_LOG_INFO("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
                    "All MSG IDs FOUND in [%d-%d]\r\n",
                    selectedConfigInfo->fromServiceId, selectedConfigInfo->toServiceId);
        }
    }

    // All given ranges were found so no need to activate them
    if (bSkip == false)
    {
        RIL_LOG_INFO("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
                "All SMS CB/Cmas ranges already activated.\r\n");
        m_vBroadcastSmsConfigInfo.clear();
    }

    // Only ETWS Test Id can be enabled or not through Android UI,
    // So just checking number of ETWS activated Ids should be enough
    if (configIds.etwsIds == m_CbsInfo.m_activatedIds.etwsIds)
    {
        RIL_LOG_INFO("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
                "All ETWS ranges already activated.\r\n");
        m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.clear();
    }
    else
    {
        RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
                "New list of ETWS ranges Length:%u, %d Ids\r\n",
                m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.size(), configIds.etwsIds);
        // Reset activated ETWS info
        m_CbsInfo.m_activatedIds.etwsIds = 0;
        m_CbsInfo.m_bEtwSmsTestActivated = false;
    }

    if (m_vBroadcastSmsConfigInfo.empty())
    {
        RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() -"
                " m_vBroadcastSmsConfigInfo empty.\r\n");
    }
    else
    {
        RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - "
                "New list of Sms CB ranges Length:%u, %d Ids\r\n",
                m_vBroadcastSmsConfigInfo.size(), configIds.cmasIds);
        // Reset activated Sms Cb info
        m_CbsInfo.m_vActivatedBroadcastSms.clear();
        m_CbsInfo.m_activatedIds.cmasIds = 0;
    }

    res = RRIL_RESULT_OK_IMMEDIATE;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSetBroadcastSmsConfig() - Exit - res=%d\r\n", res);
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseGsmSetBroadcastSmsConfig(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGsmSetBroadcastSmsConfig() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGsmSetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION
//
RIL_RESULT_CODE CTE_XMM7160::CoreGsmSmsBroadcastActivation(REQUEST_DATA& reqData,
                                                                  void* pData,
                                                                  UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int fBcActivate = 0;
    char szTmpReqData[MAX_BUFFER_SIZE] = {0};
    size_t uiEtwsReqLen = 0;
    char* pszEtwsRequest = NULL;
    char* pszSmsCbRequest = NULL;

    if (sizeof(int) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSmsBroadcastActivation() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSmsBroadcastActivation() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    fBcActivate = *((int*)pData);
    RIL_LOG_INFO("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - fBcActivate=[%d]\r\n",
            fBcActivate);

    if (fBcActivate > 0)
    {
        // DEACTIVATE
        // This command deactivates all channels with all code schemes.
        RIL_LOG_INFO("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                "DEACTIVATE SMS-CB and TWS WARNINGS\r\n");
        if (CopyStringNullTerminate(reqData.szCmd1, "AT+CSCB=0;+XETWNTFYSTOP=255\r",
                sizeof(reqData.szCmd1)))
        {
            // Reset activated info
            m_CbsInfo.Clean();
            res = RRIL_RESULT_OK;
        }
    }
    else
    {
        pszEtwsRequest = szTmpReqData;
        // Check ETWS activation request
        if (CreateEtwSmsRequest(pszEtwsRequest, sizeof(szTmpReqData)) != RRIL_RESULT_OK)
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                    "Error creating ETWS request\r\n");
            goto Error;
        }

        uiEtwsReqLen = strlen(szTmpReqData);

        RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                "ETWS activation request Len:%u\r\n", uiEtwsReqLen);

        if (uiEtwsReqLen > 0)
        {
            RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                    "ETWS activation request:%s\r\n", szTmpReqData);

            if (!ConcatenateStringNullTerminate(reqData.szCmd1,
                    sizeof(reqData.szCmd1), szTmpReqData))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                        "Cannot append ETWS activation request to global request\r\n");
                goto Error;
            }
        }

        szTmpReqData[0] = '\0';

        pszSmsCbRequest = szTmpReqData;
        // Check SMS Cell Broadcast activation request
        if (CreateSmsCbRequest(pszSmsCbRequest, sizeof(szTmpReqData)) != RRIL_RESULT_OK)
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                    "Error creating SMS CB request\r\n");
            goto Error;
        }

        if (strlen(szTmpReqData) > 0)
        {
            // Some Sms Cb/Cmas msg ids need to be activated
            RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                    "Sms Cb/Cmas activation request:%s\r\n", szTmpReqData);

            if (uiEtwsReqLen > 0)
            {
                // Append the +CSCB to the command string, separator is ';'
                if (!PrintStringNullTerminate(reqData.szCmd1 + uiEtwsReqLen,
                        sizeof(reqData.szCmd1) - uiEtwsReqLen,
                        ";%s", szTmpReqData))
                {
                    RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                            "Cannot append SMS CB request\r\n");
                    goto Error;
                }
            }
            else
            {
                // Create the +CSCB command string, prefix is 'AT'
                if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
                        "AT%s", szTmpReqData))
                {
                    RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                            "Cannot copy SMS CB request\r\n");
                    goto Error;
                }
            }
        }

        // Add multiple commands termination character: <CR>
        if (strlen(reqData.szCmd1) > 0)
        {
            if (!ConcatenateStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1), "\r"))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - "
                        "Cannot add <cr> after command\r\n");
                goto Error;
            }
        }

        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGsmSmsBroadcastActivation() - Exit - res:%d\r\n", res);
    return res;
}

//
// Builds an ETWS activation request.
// An +XETWNTFYSTOP command is called to reset configuration prior to +XETWNTFYSTART activation
// command.
//
RIL_RESULT_CODE CTE_XMM7160::CreateEtwSmsRequest(char*& reqData, UINT32 uiReqSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateETWSRequest() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiRanges = 0;
    android::Vector<RIL_GSM_BroadcastSmsConfigInfo>::const_iterator selectedConfigInfo;
    char szTwsWarnings[MAX_BUFFER_SIZE] = {0};
    char szReqETWSTest[MAX_BUFFER_SIZE] = {0};
    int testRequest = 0;

    // Loop on ETWS configured ranges
    for (selectedConfigInfo = m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.begin();
            selectedConfigInfo != m_CbsInfo.m_vBroadcastEtwSmsConfigInfo.end();
            selectedConfigInfo++)
    {
        RIL_LOG_VERBOSE("CTE_XMM7160::CreateETWSRequest() - "
                "Checking ETWS Range [%d,%d]\r\n",
                selectedConfigInfo->fromServiceId, selectedConfigInfo->toServiceId);

        // Builds 'regular' ETWS range
        if (!PrintStringNullTerminate(szTwsWarnings + strlen(szTwsWarnings),
                sizeof(szTwsWarnings) - strlen(szTwsWarnings), ",%u,%u",
                selectedConfigInfo->fromServiceId, selectedConfigInfo->toServiceId))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CreateETWSRequest() - "
                    "Unable to create string with TWS [%d,%d] ids range\r\n",
                    selectedConfigInfo->fromServiceId, selectedConfigInfo->toServiceId);
            goto Error;
        }

        RIL_LOG_VERBOSE("CTE_XMM7160::CreateETWSRequest() - "
                "Adding ETWS range [%d,%d]\r\n",
                selectedConfigInfo->fromServiceId, selectedConfigInfo->toServiceId);

        uiRanges++;
        m_CbsInfo.m_activatedIds.etwsIds +=
                m_CbsInfo.GetNumberOfIdsFromRange(selectedConfigInfo->fromServiceId,
                selectedConfigInfo->toServiceId);

    }

    RIL_LOG_VERBOSE("CTE_XMM7160::CreateETWSRequest() - "
                        "Added %u ETWS Ranges and %d Test range\r\n",
                        uiRanges, testRequest);

    // First, buid ETWS stop request if needed
    if ((testRequest > 0) || (uiRanges > 0))
    {
        // ETWS ranges will be activated, so the stop request is prepared
        if (!CopyStringNullTerminate(reqData, "AT+XETWNTFYSTOP=255", uiReqSize))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CreateETWSRequest() - "
                    "Cannot create AT+XETWNTFYSTOP request\r\n");
            goto Error;
        }
    }

    // Then, check and append ETWS test range to request
    if (testRequest > 0)
    {
        if (!ConcatenateStringNullTerminate(reqData, uiReqSize, szReqETWSTest))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CreateETWSRequest() - "
                            "Cannot create ETWS TEST request\r\n");
            goto Error;
        }
        m_CbsInfo.m_bEtwSmsTestActivated = true;
    }

    // Finally, check ETWS ranges
    if (uiRanges > 0)
    {
        size_t uiRequestLen = strlen(reqData);
        // Prepare ETWS activation request
        if (!PrintStringNullTerminate(reqData + uiRequestLen,
                uiReqSize - uiRequestLen, ";+XETWNTFYSTART=5,%u", uiRanges))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CreateETWSRequest() - "
                    "Unable to prepare ETWS activation request with %u ranges\r\n",
                    uiRanges);
            goto Error;
        }

        // Append ETWS ranges to ETWS activation request
        if (!ConcatenateStringNullTerminate(reqData, uiReqSize, szTwsWarnings))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CreateETWSRequest() - "
                            "Cannot create ETWS request\r\n");
            goto Error;
        }
    }
    res = RRIL_RESULT_OK;

Error:
    if (res != RRIL_RESULT_OK)
    {
        m_CbsInfo.m_activatedIds.etwsIds = 0;
        m_CbsInfo.m_bEtwSmsTestActivated = false;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::CreateETWSRequest() - Exit - res:%d\r\n", res);
    return res;
}

//
// Builds an activation request for Sms Cb/Cmas msg ids.
//
RIL_RESULT_CODE CTE_XMM7160::CreateSmsCbRequest(char*& reqData, UINT32 uiReqSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateSMSCBRequest() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiRanges = 0;
    android::Vector<RIL_GSM_BroadcastSmsConfigInfo>::const_iterator selectedConfigInfo;
    char szChannels[MAX_BUFFER_SIZE] = {0};
    size_t uiChannelsLen = 0;
    char szLangs[MAX_BUFFER_SIZE] = {0};
    size_t uiLangsLen = 0;
    CCbsInfo::IdRange smsRange = {0,0};

    // Loop on Sms Cb/Cmas configured ranges
    for (selectedConfigInfo = m_vBroadcastSmsConfigInfo.begin();
            selectedConfigInfo != m_vBroadcastSmsConfigInfo.end();
            selectedConfigInfo++)
    {
        uiChannelsLen = strlen(szChannels);

        if (selectedConfigInfo->fromServiceId == selectedConfigInfo->toServiceId)
        {
            if (!PrintStringNullTerminate(szChannels + uiChannelsLen,
                    MAX_BUFFER_SIZE - uiChannelsLen, "%d,",
                    selectedConfigInfo->fromServiceId))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CreateSMSCBRequest() -"
                        "Cannot format szChannels string\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(szChannels + uiChannelsLen,
                    MAX_BUFFER_SIZE - uiChannelsLen, "%d-%d,",
                    selectedConfigInfo->fromServiceId, selectedConfigInfo->toServiceId))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CreateSMSCBRequest() -"
                        "Cannot format szChannels string\r\n");
                goto Error;
            }
        }
        uiLangsLen = strlen(szLangs);
        if (selectedConfigInfo->fromCodeScheme == selectedConfigInfo->toCodeScheme)
        {
            if (!PrintStringNullTerminate(szLangs + uiLangsLen,
                    MAX_BUFFER_SIZE - uiLangsLen, "%d,",
                    selectedConfigInfo->fromCodeScheme))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CreateSMSCBRequest() -"
                        "Cannot format szLangs string\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(szLangs + uiLangsLen,
                    MAX_BUFFER_SIZE - uiLangsLen, "%d-%d,",
                    selectedConfigInfo->fromCodeScheme, selectedConfigInfo->toCodeScheme))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CreateSMSCBRequest() -"
                        "Cannot format szLangs string\r\n");
                goto Error;
            }
        }

        // Update activated Ids
        smsRange.minVal = selectedConfigInfo->fromServiceId;
        smsRange.maxVal = selectedConfigInfo->toServiceId;
        m_CbsInfo.m_activatedIds.cmasIds +=
                m_CbsInfo.GetNumberOfIdsFromRange(selectedConfigInfo->fromServiceId,
                selectedConfigInfo->toServiceId);
        m_CbsInfo.m_vActivatedBroadcastSms.push_back(smsRange);

        // Counts ranges to be activated
        uiRanges++;
    }

    if (uiRanges > 0)
    {
        uiChannelsLen = strlen(szChannels);
        uiLangsLen = strlen(szLangs);
        if ((uiChannelsLen == 0) || (uiLangsLen == 0))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::CreateSMSCBRequest() - "
                    "Error no Id or Lang ranges configured\r\n");
            goto Error;
        }
        else
        {
            // Trick to remove the last comma
            szChannels[strlen(szChannels) - 1] = '\0';
            szLangs[strlen(szLangs) - 1] = '\0';
            if (!PrintStringNullTerminate(reqData,
                    uiReqSize, "+CSCB=0,\"%s\",\"%s\"",
                    szChannels, szLangs))
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::CreateSMSCBRequest() - "
                        "Unable to create SMS CB activation request\r\n");
                goto Error;
            }
        }
    }

    res = RRIL_RESULT_OK;
Error:
    if (res != RRIL_RESULT_OK)
    {
        m_CbsInfo.m_activatedIds.cmasIds = 0;
        m_CbsInfo.m_vActivatedBroadcastSms.clear();
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::CreateSMSCBRequest() - Exit - res:%d\r\n", res);
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseGsmSmsBroadcastActivation(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGsmSmsBroadcastActivation() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGsmSmsBroadcastActivation() - Exit\r\n");
    return res;
}

const char* CTE_XMM7160::GetSiloVoiceURCInitString()
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
        pszInit = "|+XCSFB=3|+XCGCLASS=\"CG\"|+XCONFIG=3,0";
    }
    return pszInit;
}

//
// RIL_REQUEST_GET_CELL_INFO_LIST
//
RIL_RESULT_CODE CTE_XMM7160::CoreGetCellInfoList(REQUEST_DATA& rReqData,
        void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGetCellInfoList() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+XMCI=\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::CoreGetCellInfoList() - Exit\r\n");
    return res;
}

void CTE_XMM7160::ConvertCellInfoForVanillaAOSP(P_ND_N_CELL_INFO_DATA_V2 pOldData,
        P_ND_N_CELL_INFO_DATA pNewData, int nCellInfos)
{

    for (int i=0; i < nCellInfos; i++)
    {
        RIL_CellInfo_v2& oldInfo = pOldData->aRilCellInfo[i];
        RIL_CellInfo& newInfo = pNewData->aRilCellInfo[i];
        newInfo.registered = oldInfo.registered;
        newInfo.timeStampType = oldInfo.timeStampType;
        newInfo.timeStamp = oldInfo.timeStamp;

        int cellInfoType = oldInfo.cellInfoType;
        switch(cellInfoType)
        {
            case RIL_CELL_INFO_TYPE_GSM_V2:
                newInfo.cellInfoType = RIL_CELL_INFO_TYPE_GSM;
                newInfo.CellInfo.gsm.signalStrengthGsm
                        = *((RIL_GW_SignalStrength*) &oldInfo.CellInfo.gsm.signalStrengthGsm);
                newInfo.CellInfo.gsm.cellIdentityGsm
                        = *((RIL_CellIdentityGsm*) &oldInfo.CellInfo.gsm.cellIdentityGsm);
            break;
            case RIL_CELL_INFO_TYPE_WCDMA_V2:
                newInfo.cellInfoType = RIL_CELL_INFO_TYPE_WCDMA;
                newInfo.CellInfo.wcdma.signalStrengthWcdma
                        = *((RIL_SignalStrengthWcdma*) &oldInfo.CellInfo.wcdma.signalStrengthWcdma);
                newInfo.CellInfo.wcdma.cellIdentityWcdma
                        = *((RIL_CellIdentityWcdma*) &oldInfo.CellInfo.wcdma.cellIdentityWcdma);
            break;
            case RIL_CELL_INFO_TYPE_LTE_V2:
                newInfo.cellInfoType = RIL_CELL_INFO_TYPE_LTE;
                newInfo.CellInfo.lte.signalStrengthLte
                        = *((RIL_LTE_SignalStrength_v8*) &oldInfo.CellInfo.lte.signalStrengthLte);
                newInfo.CellInfo.lte.cellIdentityLte
                        = *((RIL_CellIdentityLte*) &oldInfo.CellInfo.lte.cellIdentityLte);
            break;
        }
    }
}


RIL_RESULT_CODE CTE_XMM7160::ParseCellInfoList(RESPONSE_DATA& rRspData, BOOL isUnsol)
{
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int nCellInfos = 0;
    void* pCellData = NULL;

    pCellData = ParseXMCI(rRspData, nCellInfos);

    if (!isUnsol)
    {
        if (nCellInfos > 0 && NULL != pCellData)
        {
#if !defined(USE_PATCHED_AOSP)
            P_ND_N_CELL_INFO_DATA pNewCellData
                    = (P_ND_N_CELL_INFO_DATA) malloc(sizeof(S_ND_N_CELL_INFO_DATA));
            if (NULL == pNewCellData)
            {
                RIL_LOG_CRITICAL("CTE_XMM7160::ParseCellInfoList() -"
                        " Could not allocate memory for a S_ND_N_CELL_INFO_DATA struct.\r\n");
                goto Error;
            }
            memset(pNewCellData, 0, sizeof(S_ND_N_CELL_INFO_DATA));
            ConvertCellInfoForVanillaAOSP((P_ND_N_CELL_INFO_DATA_V2)pCellData,
                    pNewCellData, nCellInfos);
            free(pCellData);
            pCellData = pNewCellData;
            pNewCellData = NULL;
            rRspData.uiDataSize = nCellInfos * sizeof(RIL_CellInfo);

#else
            rRspData.uiDataSize = nCellInfos * sizeof(RIL_CellInfo_v2);
#endif
            rRspData.pData = (void*)((P_ND_N_CELL_INFO_DATA)pCellData)->aRilCellInfo;
        }
        else
        {
            nCellInfos = 0;
            rRspData.pData  = NULL;
            rRspData.uiDataSize = 0;
            free(pCellData);
            pCellData = NULL;
        }
    }
    else
    {
        // Unsolicited CELL INFO LIST
        // update the list only if there is a change in the information
        // compare the read values with the cellinfo cache and if the values
        // are different, report RIL_UNSOL_CELL_INFO_LIST
        if (nCellInfos > 0)
        {
            int requestedRate = (intptr_t)rRspData.pContextData;
            if (m_cte.updateCellInfoCache((P_ND_N_CELL_INFO_DATA_V2)pCellData, nCellInfos)
                    && -1 != requestedRate && INT_MAX != requestedRate)
            {

#if !defined(USE_PATCHED_AOSP)
                P_ND_N_CELL_INFO_DATA pNewCellData
                        = (P_ND_N_CELL_INFO_DATA) malloc(sizeof(S_ND_N_CELL_INFO_DATA));
                if (NULL == pNewCellData)
                {
                    RIL_LOG_CRITICAL("CTE_XMM7160::ParseCellInfoList() -"
                            " Could not allocate memory for a S_ND_N_CELL_INFO_DATA struct.\r\n");
                    goto Error;
                }
                memset(pNewCellData, 0, sizeof(S_ND_N_CELL_INFO_DATA));
                ConvertCellInfoForVanillaAOSP((P_ND_N_CELL_INFO_DATA_V2)pCellData,
                        pNewCellData, nCellInfos);
                free(pCellData);
                pCellData = (void*)pNewCellData;
                pNewCellData = NULL;
                RIL_onUnsolicitedResponse(RIL_UNSOL_CELL_INFO_LIST,
                        (void*)((P_ND_N_CELL_INFO_DATA)pCellData)->aRilCellInfo,
                        sizeof(RIL_CellInfo) * nCellInfos);

#else
                RIL_onUnsolicitedResponse(RIL_UNSOL_CELL_INFO_LIST,
                        (void*)((P_ND_N_CELL_INFO_DATA_V2)pCellData)->aRilCellInfo,
                        sizeof(RIL_CellInfo_v2) * nCellInfos);
#endif
            }
        }

        // restart the timer now with the latest rate setting.
        if (!m_cte.IsCellInfoTimerRunning())
        {
            int newRate = m_cte.GetCellInfoListRate();
            RestartUnsolCellInfoListTimer(newRate);
        }
    }

    res = RRIL_RESULT_OK;
Error:

    if (RRIL_RESULT_OK != res || isUnsol)
    {
        free(pCellData);
        pCellData = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseCellInfoList() - Exit\r\n");
    return res;
}

P_ND_N_CELL_INFO_DATA_V2 CTE_XMM7160::ParseXMCI(RESPONSE_DATA& rspData, int& nCellInfos)
{
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int index = 0;
    int mcc = INT_MAX;
    int mnc = INT_MAX;
    int lac = INT_MAX;
    int ci = INT_MAX;
    const char* pszRsp = rspData.szResponse;
    P_ND_N_CELL_INFO_DATA_V2 pCellData = NULL;
    const int MCC_UNKNOWN = 0xFFFF;
    const int MNC_UNKNOWN = 0xFFFF;
    const int LAC_UNKNOWN = 0;
    const int CI_UNKNOWN = 0;
    const int PCI_UNKNOWN = 0xFFFF;
    const int CQI_UNKNOWN = 0;
    const int BSIC_UNKNOWN = 0xFF;
    uint64_t timestamp = ril_nano_time();

    pCellData = (P_ND_N_CELL_INFO_DATA_V2) malloc(sizeof(S_ND_N_CELL_INFO_DATA_V2));
    if (NULL == pCellData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXMCI() -"
                " Could not allocate memory for a S_ND_N_CELL_INFO_DATA_V2 struct.\r\n");
        goto Error;
    }
    memset(pCellData, 0, sizeof(S_ND_N_CELL_INFO_DATA_V2));

    /*
     * GSM serving and neighboring cell:
     *
     * +XMCI: 0,<MCC>,<MNC>,<LAC>,<CI>,<BSIC>,<RXLEV>,<BER>
     * +XMCI: 1,<MCC>,<MNC>,<LAC>,<CI>,<BSIC>,<RXLEV>,<BER>
     *
     * UMTS serving and neighboring cell:
     *
     * +XMCI: 2,<MCC>,<MNC>,<LAC>,<CI>,<PSC>,<RSCP>,<ECNO>
     * +XMCI: 3,<MCC>,<MNC>,<LAC>,<CI>,<PSC>,<RSCP>,<ECNO>
     *
     * LTE serving and neighboring cell:
     *
     * +XMCI: 4,<MCC>,<MNC>,<TAC>,<CI>,<PCI>,<RSRP>,<RSRQ>,<RSSNR>,<TA>,<CQI>
     * +XMCI: 5,<MCC>,<MNC>,<TAC>,<CI>,<PCI>,<RSRP>,<RSRQ>,<RSSNR>,<TA>,<CQI>
     */

    // Loop on +XMCI until no more entries are found
    while (FindAndSkipString(pszRsp, "+XMCI: ", pszRsp))
    {
        int type = 0;

        if (RRIL_MAX_CELL_ID_COUNT == index)
        {
            //  We're full.
            RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                    " Exceeded max count = %d\r\n", RRIL_MAX_CELL_ID_COUNT);
            break;
        }

        // Read <TYPE>
        if (!ExtractInt(pszRsp, type, pszRsp))
        {
            RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                    " could not extract <TYPE> value\r\n");
            continue;
        }

        // Read <MCC>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractInt(pszRsp, mcc, pszRsp))
        {
            RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                    " could not extract <MCC> value\r\n");
            continue;
        }
        mcc = (MCC_UNKNOWN == mcc) ? INT_MAX : mcc;

        // Read <MNC>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractInt(pszRsp, mnc, pszRsp))
        {
            RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                    " could not extract <MNC> value\r\n");
            continue;
        }
        mnc = (MNC_UNKNOWN == mnc) ? INT_MAX : mnc;

        // Read <LAC> or <TAC>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractQuotedHexInt(pszRsp, lac, pszRsp))
        {
            RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                    " could not extract <LAC> or <TAC>\r\n");
            continue;
        }
        lac = (LAC_UNKNOWN == lac) ? INT_MAX : lac;

        // Read <CI>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractQuotedHexInt(pszRsp, ci, pszRsp))
        {
            RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                    " could not extract <CI> value\r\n");
            continue;
        }
        ci = (CI_UNKNOWN == ci) ? INT_MAX : ci;

        switch (type)
        {
            case 0: // GSM serving cell
            case 1: // GSM neighboring cell
            {
                int rxlev = RXLEV_UNKNOWN;
                int ber = BER_UNKNOWN;
                int basestationId = INT_MAX;

                // Read <BSIC>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, basestationId, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <BSIC> value\r\n");
                    continue;
                }
                basestationId = (BSIC_UNKNOWN == basestationId) ? INT_MAX : basestationId;

                // Read <RXLEV>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, rxlev, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <RXLEV> value\r\n");
                    continue;
                }

                // Read <BER>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, ber, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <BER> value\r\n");
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
                info.CellInfo.gsm.signalStrengthGsm.timingAdvance = INT_MAX;
                info.CellInfo.gsm.cellIdentityGsm.lac = lac;
                info.CellInfo.gsm.cellIdentityGsm.cid = ci;
                info.CellInfo.gsm.cellIdentityGsm.mnc = mnc;
                info.CellInfo.gsm.cellIdentityGsm.mcc = mcc;
                info.CellInfo.gsm.cellIdentityGsm.basestationId = basestationId;
                info.CellInfo.gsm.cellIdentityGsm.arfcn = INT_MAX;
                RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() - "
                        "GSM LAC,CID,MCC,MNC index=[%d] cid=[%d] lac[%d] mcc[%d] mnc[%d]\r\n",
                        index, info.CellInfo.gsm.cellIdentityGsm.cid,
                        info.CellInfo.gsm.cellIdentityGsm.lac,
                        info.CellInfo.gsm.cellIdentityGsm.mcc,
                        info.CellInfo.gsm.cellIdentityGsm.mnc);
                index++;
            }
            break;

            case 2: // UMTS serving cell
            case 3: // UMTS neighboring cell
            {
                int rscp;
                int psc;
                int ecNo;

                // Read <PSC>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, psc, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <PSC>\r\n");
                    continue;
                }
                psc = (psc < 0 || psc > 511) ? INT_MAX : psc;

                // Read <RSCP>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, rscp, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <RSCP>\r\n");
                    continue;
                }

                // Read <ECNO>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, ecNo, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
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
                info.CellInfo.wcdma.cellIdentityWcdma.cid = ci;
                info.CellInfo.wcdma.cellIdentityWcdma.psc = psc;
                info.CellInfo.wcdma.cellIdentityWcdma.mnc = mnc;
                info.CellInfo.wcdma.cellIdentityWcdma.mcc = mcc;
                info.CellInfo.wcdma.cellIdentityWcdma.dluarfcn = INT_MAX;
                info.CellInfo.wcdma.cellIdentityWcdma.uluarfcn = INT_MAX;
                info.CellInfo.wcdma.cellIdentityWcdma.pathloss = INT_MAX;
                RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() - "
                        "UMTS LAC,CID,MCC,MNC,ScrCode "
                        "index=[%d]  cid=[%d] lac[%d] mcc[%d] mnc[%d] scrCode[%d]\r\n",
                        index, info.CellInfo.wcdma.cellIdentityWcdma.cid,
                        info.CellInfo.wcdma.cellIdentityWcdma.lac,
                        info.CellInfo.wcdma.cellIdentityWcdma.mcc,
                        info.CellInfo.wcdma.cellIdentityWcdma.mnc,
                        info.CellInfo.wcdma.cellIdentityWcdma.psc);
                index++;
            }
            break;

            case 4: // LTE serving cell
            case 5: // LTE neighboring cell
            {
                int pci;
                int rsrp;
                int rsrq;
                int rssnr;
                int ta;
                int cqi;

                // Read <PCI>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, pci, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <PCI>\r\n");
                    continue;
                }

                // Read <RSRP>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, rsrp, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <RSRP>\r\n");
                    continue;
                }

                // Read <RSRQ>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, rsrq, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <RSRQ>\r\n");
                    continue;
                }

                // Read <RSSNR>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, rssnr, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <RSSNR>\r\n");
                    continue;
                }

                // Read <TA>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, ta, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <TA>\r\n");
                    continue;
                }
                if (type == 5)
                {
                    // In case of type 5, TA field is invalid but modem reports 0
                    ta = INT_MAX;
                }

                // Read <CQI>
                if (!SkipString(pszRsp, ",", pszRsp)
                        || !ExtractInt(pszRsp, cqi, pszRsp))
                {
                    RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                            " could not extract <CQI>\r\n");
                    continue;
                }
                if (type == 5)
                {
                    // In case of type 5, CQI field is invalid but modem reports 0
                    cqi = INT_MAX;
                }

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
                info.CellInfo.lte.signalStrengthLte.timingAdvance = ta;
                info.CellInfo.lte.cellIdentityLte.tac = lac;
                info.CellInfo.lte.cellIdentityLte.ci = ci;
                info.CellInfo.lte.cellIdentityLte.pci = pci;
                info.CellInfo.lte.cellIdentityLte.mnc = mnc;
                info.CellInfo.lte.cellIdentityLte.mcc = mcc;
                info.CellInfo.lte.cellIdentityLte.dlearfcn = INT_MAX;
                info.CellInfo.lte.cellIdentityLte.ulearfcn = INT_MAX;
                info.CellInfo.lte.cellIdentityLte.pathloss = INT_MAX;
                RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
                        "LTE TAC,CID,MCC,MNC,RSRP,RSRQ,TA,RSSNR,PhyCellId "
                        "index=[%d] cid=[%d] tac[%d] mcc[%d] mnc[%d] rsrp[%d] rsrq[%d] "
                        "ta[%d] rssnr[%d] Phyci[%d] \r\n",
                        index, info.CellInfo.lte.cellIdentityLte.ci,
                        info.CellInfo.lte.cellIdentityLte.tac,
                        info.CellInfo.lte.cellIdentityLte.mcc,
                        info.CellInfo.lte.cellIdentityLte.mnc,
                        info.CellInfo.lte.signalStrengthLte.rsrp,
                        info.CellInfo.lte.signalStrengthLte.rsrq,
                        info.CellInfo.lte.signalStrengthLte.timingAdvance,
                        info.CellInfo.lte.signalStrengthLte.rssnr,
                        info.CellInfo.lte.cellIdentityLte.pci);
                index++;
            }
            break;

            default:
            {
                RIL_LOG_INFO("CTE_XMM7160::ParseXMCI() -"
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
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseXMCI - Could not skip response postfix.\r\n");
    }

    if (RRIL_RESULT_OK != res)
    {
        nCellInfos = 0;
        free(pCellData);
        pCellData = NULL;
    }

    return pCellData;
}

int CTE_XMM7160::MapRxlevToSignalStrengh(int rxlev)
{
    // In case of invalid rxlev values, return RSSI_UNKNOWN.
    if (rxlev > 57 || rxlev < 0)
        return RSSI_UNKNOWN;
    else if (rxlev <= 57)
        return (rxlev / 2) + 2;
    else
        return 31;
}

int CTE_XMM7160::MapToAndroidRsrp(int rsrp)
{
    /*
     * In case of invalid rsrp values, return INT_MAX.
     * If modem returns 0  then rsrp = -140 dBm.
     * If modem returns 1  then rsrp = -140 dBm.
     * If modem returns 2  then rsrp = -139 dBm.
     * ...
     * If modem returns 97 then rsrp =  -44 dBm.
     *
     * As Android expects a positive value, rapid ril needs to send (141 - rsrp) to
     * framework.
     */
    if (rsrp > 97 || rsrp < 0)
        return INT_MAX;
    else if (rsrp <= 1)
        return 140;
    else
        return 141 - rsrp;
}

int CTE_XMM7160::MapToAndroidRsrq(int rsrq)
{
    /*
     * In case of invalid rsrq values, return INT_MAX.
     * If modem return 0 then rsrq = -20 dBm.
     * If modem return 1 then rsrq = -20 dBm.
     *
     * Note: -19.5dBm is rounded to -20dbm
     *
     * As Android expects a positive value, rapid ril needs to send (20 - rsrq / 2) to
     * framework.
     */
    if (rsrq > 34 || rsrq < 0)
        return INT_MAX;
    else
        return 20 - rsrq / 2;
}

int CTE_XMM7160::MapToAndroidRssnr(int rssnr)
{
    /*
     * If modem returns 0 then rssnr = 0 dBm
     * If modem returns 1 then rssnr = 0.5 dBm
     * As Android has granularity of 0.1 dB units, rapid ril needs to send
     * (rssnr/2)*10 => rssnr * 5 to framework.
     *
     * As per ril documentation, valid rssnr range is -200 to +300(-200 = -20.0dB, +300 = 30dB).
     * Return INT_MAX if the value is not within this range.
     */
    int androidRssnr = rssnr * 5;
    if (androidRssnr > 300 || androidRssnr < -200)
        return INT_MAX;
    else
        return androidRssnr;
}

RIL_RESULT_CODE CTE_XMM7160::CreateSetAdaptiveClockingReq(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    /*
     * Activate or deactivate the unsolicited response for adaptive clocking by sending
     *  - AT+XADPCLKFREQINFO=1 to activate
     *  - AT+XADPCLKFREQINFO=0 to deactivate
     */
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateSetAdaptiveClockingReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int enableUnsol;

    if (uiDataSize < (2 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateSetAdaptiveClockingReq() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    // The value received should be '0' or '1'.
    if (!ExtractInt(ppszRequest[1], enableUnsol, ppszRequest[1])
            || (enableUnsol < 0 || enableUnsol > 1))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateSetAdaptiveClockingReq() :"
                " wrong value received\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
            "AT+XADPCLKFREQINFO=%d\r", enableUnsol))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateSetAdaptiveClockingReq() -"
                " Cannot construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateSetAdaptiveClockingReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::CreateGetAdaptiveClockingFreqInfo(REQUEST_DATA& reqData,
        const char** ppszRequest, const UINT32 uiDataSize)
{
    /*
     * Retrieves frequency information from the modem.
     */
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateGetAdaptiveClockingFreqInfo() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
            "AT+XADPCLKFREQINFO=?\r"))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CreateGetAdaptiveClockingFreqInfo() -"
                " Cannot construct szCmd1.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CreateGetAdaptiveClockingFreqInfo() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo(const char* pszRsp,
        RESPONSE_DATA& rspData)
{
    /*
     * Parse +XADPCLKFREQINFO: , response to the test command "AT+XADPCLKFREQINFO=?".
     * If the response is received here, means that is not on URC channel,
     * and the answer have multiple triplets of <centFreq>, <freqSpread> and <noisePower>.
     */
    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() - Enter\r\n");

    int nParams = 1;
    long long centFreq;
    int freqSpread;
    int noisePower;
    bool bLastTriplet = false;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    P_ND_ADPCLK_FREQ_INFO pResponse
            = (P_ND_ADPCLK_FREQ_INFO) malloc(sizeof(S_ND_ADPCLK_FREQ_INFO));
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() -"
                "Could not allocate memory for response");
        goto Error;
    }
    memset(pResponse, 0, sizeof(S_ND_ADPCLK_FREQ_INFO));

    // test command response,
    // the answer can have multiple triplets of <centFreq>, <freqSpread> and <noisePower>
    // +XADPCLKFREQINFO: <centFreq>, <freqSpread>, <noisePower>[,<centFreq>,
    //                                                           <freqSpread>, <noisePower>[,...]]
    if (!FindAndSkipString(pszRsp, "+XADPCLKFREQINFO: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() -"
                " Unable to parse \"+XADPCLKFREQINFO\" prefix\r\n");
        goto Error;
    }

    // start parsing the response, triplet by triplet
    while (!bLastTriplet)
    {
        // Parse <centFreq>
        if (!ExtractLongLong(pszRsp, centFreq, pszRsp)
                || !SkipString(pszRsp, ",", pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() -"
                    " Unable to parse <centFreq>\r\n");
            goto Error;
        }

        // Parse <freqSpread>
        if (!ExtractInt(pszRsp, freqSpread, pszRsp)
                || !SkipString(pszRsp, ",", pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() -"
                    " Unable to parse <freqSpread>\r\n");
            goto Error;
        }

        // Parse <noisePower>
        if (!ExtractInt(pszRsp, noisePower, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() -"
                    " Unable to parse <noisePower>\r\n");
            goto Error;
        }

        // Look for ",", if found, means that we have new triplet to parse
        bLastTriplet = !SkipString(pszRsp, ",", pszRsp);

        // It Contains the current pair (centFreq, freqSpread, noisePower)
        char szCurrentPair[MAX_BUFFER_SIZE] = {'\0'};

        // The comma should be added at the end of the string if not the last triplet
        snprintf(szCurrentPair, sizeof(szCurrentPair),
                "%lld, %d, %d%s",
                centFreq, freqSpread, noisePower,
                (bLastTriplet) ? "" : ", ");

        // Appending the current pair to the response
        if (!ConcatenateStringNullTerminate(pResponse->szAdaptiveClockFrequencyInfo,
                 sizeof(pResponse->szAdaptiveClockFrequencyInfo), szCurrentPair))
        {
            RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() - "
                    " Cannot add %s.\r\n", szCurrentPair);
            goto Error;
        }
    }

    // Find "<postfix>"
    if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() - "
                " Unable to find response end\r\n");
        goto Error;
    }

    pResponse->sResponsePointer.pszAdaptiveClockFrequencyInfo =
            pResponse->szAdaptiveClockFrequencyInfo;
    rspData.pData = pResponse;
    rspData.uiDataSize = sizeof(S_ND_ADPCLK_FREQ_INFO_PTR);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM7160::ParseGetAdaptiveClockingFreqInfo() - Exit\r\n");
    return res;
}

bool CTE_XMM7160::IsImsEnabledApn(const char* pszApn)
{
    if (m_cte.IsIMSApCentric())
    {
        S_DATA_PROFILE_INFO info;
        for (size_t i = 0; i < m_vDataProfileInfos.size(); i++)
        {
            info = m_vDataProfileInfos[i];
            // DATA_PROFILE_IMS is AOSP constant, not a bit field so we must mask OEM bits
            if (DATA_PROFILE_IMS == (DATA_PROFILE_AOSP_MASK & info.profileId)
                    && !strcmp(info.szApn, pszApn))
            {
                return true;
            }
        }
    }
    return false;
}

bool CTE_XMM7160::IsEImsEnabledApn(const char* pszApn)
{
    if (m_cte.IsIMSApCentric())
    {
        S_DATA_PROFILE_INFO info;
        for (size_t i = 0; i < m_vDataProfileInfos.size(); i++)
        {
            info = m_vDataProfileInfos[i];
            if (DATA_PROFILE_OEM_EIMS == (DATA_PROFILE_OEM_EIMS & info.profileId)
                    && !strcmp(info.szApn, pszApn))
            {
                return true;
            }
        }
    }
    return false;
}

bool CTE_XMM7160::IsRcsEnabledApn(const char* pszApn)
{
    if (m_cte.IsIMSApCentric())
    {
        S_DATA_PROFILE_INFO info;
        for (size_t i = 0; i < m_vDataProfileInfos.size(); i++)
        {
            info = m_vDataProfileInfos[i];
            if (DATA_PROFILE_OEM_RCS == (DATA_PROFILE_OEM_RCS & info.profileId)
                    && !strcmp(info.szApn, pszApn))
            {
                return true;
            }
        }
    }
    return false;
}

// RIL_REQUEST_SET_INITIAL_ATTACH_APN
RIL_RESULT_CODE CTE_XMM7160::CoreSetInitialAttachApn(REQUEST_DATA& reqData,
       void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetInitialAttachApn() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_InitialAttachApn* pTemp = NULL;

    UINT32 uiMode = 0;
    char szPdpType[MAX_PDP_TYPE_SIZE] = {'\0'};
    char szApn[MAX_APN_SIZE] = {'\0'};
    bool bInitialAttachApnChanged = false;

    if (pData == NULL)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetInitialAttachApn() - "
                "pData is NULL \r\n");
        goto Error;
    }

    if (sizeof(RIL_InitialAttachApn) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetInitialAttachApn() - "
                "pData size if wrong\r\n");
        goto Error;
    }

    if (RIL_APPSTATE_READY != GetSimAppState())
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::CoreSetInitialAttachApn() - SIM not ready\r\n");
        ResetInitialAttachApn();
        res = RRIL_RESULT_OK_IMMEDIATE;
        goto Error;
    }

    pTemp = (RIL_InitialAttachApn*) pData;
    if (NULL == pTemp->protocol || pTemp->protocol[0] == '\0')
    {
        CopyStringNullTerminate(szPdpType, PDPTYPE_IPV4V6, sizeof(szPdpType));
    }
    else
    {
        CopyStringNullTerminate(szPdpType, pTemp->protocol, sizeof(szPdpType));
    }

    if (NULL != pTemp->apn)
    {
        CopyStringNullTerminate(szApn, pTemp->apn, sizeof(szApn));
    }

    /*
     * If the stored pdp type is not empty and if there is no change in initial attach apn
     * parameters, then complete the request without sending any command to modem.
     * Note: This is specifically done to make sure initial attach apn is not set and
     * network selection is not restored again on SIM REFRESH proactive command.
     */
    if (m_InitialAttachApnParams.szPdpType[0] != '\0'
            && (0 == strcmp(m_InitialAttachApnParams.szPdpType, szPdpType))
            && (0 == strcmp(m_InitialAttachApnParams.szApn, szApn)))
    {
        res = RRIL_RESULT_OK_IMMEDIATE;
        goto Error;
    }

    /*
     * Initial attach parameters are considered as changed:
     *     - If the stored pdp type is not empty and doesn't match with the pdp type provided
     *       as part of RIL_REQUEST_SET_INITIAL_ATTACH_APN request (or)
     *     - If the stored apn is not empty and doesn't match with the apn provided as part of
     *       RIL_REQUEST_SET_INITIAL_ATTACH_APN request.
     */
    if ((m_InitialAttachApnParams.szPdpType[0] != '\0'
            && strcmp(m_InitialAttachApnParams.szPdpType, szPdpType) != 0)
            || (m_InitialAttachApnParams.szApn[0] != '\0'
            && strcmp(m_InitialAttachApnParams.szApn, szApn) != 0))
    {
        bInitialAttachApnChanged = true;
    }

    ResetInitialAttachApn();

    CopyStringNullTerminate(m_InitialAttachApnParams.szApn,
            szApn, sizeof(m_InitialAttachApnParams.szApn));
    CopyStringNullTerminate(m_InitialAttachApnParams.szPdpType,
            szPdpType, sizeof(m_InitialAttachApnParams.szPdpType));

    /*
     * Case 1: Initial attach APN is not yet set. RIL_REQUEST_SET_INITIAL_ATTACH_APN received
     * during boot after sim records are loaded
     *
     * If there is no initial attach apn set, device is also not yet registered.
     * In this case, RIL_REQUEST_SET_INITIAL_ATTACH_APN will result in commands
     * AT+CGDCONT=<cid>,<PDP_type>[,<APN] and AT+COPS=0 sent to modem.
     *
     * Case 2: Initial attach APN is already set. Initial attach APN received again with different
     * initial attach parameters.
     *
     * Upon APN change by user or sim refresh, framework will send initial attach apn request,
     * deactivation for all active connections and then SETUP_DATA_CALL for default type.
     * As actions will be taken before completing SETUP_DATA_CALL request, ril request
     * RIL_REQUEST_SET_INITIAL_ATTACH_APN request is completed immediately without sending any
     * commands to modem that is only if there is no change in initial attach apn parameters.
     */
    if (bInitialAttachApnChanged)
    {
        res = RRIL_RESULT_OK_IMMEDIATE;
        goto Error;
    }
    else
    {
        if (!GetSetInitialAttachApnReqData(reqData))
        {
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM7160::CoreSetInitialAttachApn() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM7160::RestoreSavedNetworkSelectionMode(RIL_Token rilToken, UINT32 uiChannel,
        PFN_TE_PARSE pParseFcn, PFN_TE_POSTCMDHANDLER pHandlerFcn)
{
    RIL_LOG_VERBOSE("CTEBase::RestoreSavedNetworkSelectionMode() - Enter\r\n");

    if (m_cte.IsDataCapable() && m_InitialAttachApnParams.szPdpType[0] == '\0')
    {
        /*
         * Initial attach apn can't be set as android telephony framework hasn't provided
         * initial attach apn parameters. Network selection mode will be restored once
         * the initial attach apn is set.
         */
        RIL_LOG_INFO("CTEBase::RestoreSavedNetworkSelectionMode() - "
                "initial attach apn not set\r\n");
        return RRIL_RESULT_OK_IMMEDIATE;
    }

    return CTEBase::RestoreSavedNetworkSelectionMode(rilToken, uiChannel, pParseFcn, pHandlerFcn);
}

void CTE_XMM7160::HandleSetupDataCallSuccess(UINT32 uiCID, void* pRilToken)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::HandleSetupDataCallSuccess() - Enter\r\n");

    CChannel_Data* pChannelData = NULL;
    char szPdpType[MAX_PDP_TYPE_SIZE] = {'\0'};
    char szApn[MAX_APN_SIZE] = {'\0'};
    UINT32 uiDefaultPdnCid = m_cte.GetDefaultPDNCid();

    m_cte.SetupDataCallOngoing(FALSE);

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTE_XMM7160::HandleSetupDataCallSuccess() -"
                " No Data Channel for CID %u.\r\n", uiCID);
        goto Complete;
    }

    pChannelData->GetPdpType(szPdpType, sizeof(szPdpType));
    pChannelData->GetApn(szApn, sizeof(szApn));

    /*
     * If established connection is compatible with initial attach apn parameters,
     * make this connection as default and deactivate the current default PDN if there
     * are no clients using it.
     */
    if (uiCID != uiDefaultPdnCid
            && IsApnEqual(m_InitialAttachApnParams.szApn, szApn)
            && IsPdpTypeCompatible(m_InitialAttachApnParams.szPdpType, szPdpType))
    {
        m_cte.SetDefaultPDNCid(uiCID);

        if (uiDefaultPdnCid != 0)
        {
            /*
             * If there is no client using the default PDN, deactivate the old default PDN.
             */
            CChannel_Data* pDefaultPdnChannel = CChannel_Data::GetChnlFromContextID(
                    uiDefaultPdnCid);
            if (pDefaultPdnChannel != NULL && 0 >= pDefaultPdnChannel->GetRefCount())
            {
                pDefaultPdnChannel->SetDataState(E_DATA_STATE_DEACTIVATING);
                RIL_requestTimedCallback(triggerDeactivateDataCall,
                        (void*)(intptr_t)uiDefaultPdnCid, 0, 0);
            }
        }
    }

Complete:
    CTEBase::HandleSetupDataCallSuccess(uiCID, pRilToken);
    RIL_LOG_VERBOSE("CTE_XMM7160::HandleSetupDataCallSuccess() - Exit\r\n");
}

void CTE_XMM7160::HandleSetupDataCallFailure(UINT32 uiCID, void* pRilToken, UINT32 uiResultCode)
{
    RIL_LOG_VERBOSE("CTE_XMM7160::HandleSetupDataCallFailure() - Enter\r\n");

    char szApn[MAX_APN_SIZE] = {'\0'};
    char szPdpType[MAX_PDP_TYPE_SIZE] = {'\0'};
    const UINT32 uiDefaultPDNCid = m_cte.GetDefaultPDNCid();
    CChannel_Data* pDefaultPdnChannel = (uiDefaultPDNCid != 0)
            ? CChannel_Data::GetChnlFromContextID(uiDefaultPDNCid)
            : NULL;

    m_cte.SetupDataCallOngoing(FALSE);

    CChannel_Data* pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTE_XMM7160::HandleSetupDataCallFailure() -"
                " No data channel for CID: %u\r\n", uiCID);
        goto Complete;
    }

    pChannelData->GetApn(szApn, sizeof(szApn));
    pChannelData->GetPdpType(szPdpType, sizeof(szPdpType));

    /*
     * When user changes the default or initial attach apn, no action is taken on
     * RIL_REQUEST_SET_INITIAL_ATTACH_APN but action is taken on subsequent SETUP_DATA_CALL request
     * If the SETUP_DATA_CALL request fails and if the requested apn, pdptype matches the initial
     * attach apn, then it is possible that network rejects the SETUP_DATA_CALL request because of
     * active default PDN. In this case, detach from the network, set initial attach apn and request
     * for attach.
     */
    if (pDefaultPdnChannel != NULL && E_DATA_STATE_ACTIVE == pDefaultPdnChannel->GetDataState()
            && 0 >= pDefaultPdnChannel->GetRefCount()
            && IsApnEqual(m_InitialAttachApnParams.szApn, szApn)
            && IsPdpTypeCompatible(m_InitialAttachApnParams.szPdpType, szPdpType))
    {
        RequestDetachOnIAChange();
    }

Complete:
    CTEBase::HandleSetupDataCallFailure(uiCID, pRilToken, uiResultCode);

    RIL_LOG_VERBOSE("CTE_XMM7160::HandleSetupDataCallFailure() - Exit\r\n");
}

RIL_RESULT_CODE CTE_XMM7160::SetInitialAttachApn(RIL_Token rilToken, UINT32 uiChannel,
        PFN_TE_PARSE pParseFcn, PFN_TE_POSTCMDHANDLER pHandlerFcn, int nextState)
{
    if (m_InitialAttachApnParams.szPdpType[0] == '\0')
    {
        /*
         * Initial attach apn cannot be set as android telephony framework has not provided
         * initial attach apn parameters. Network selection mode will be restored once
         * the initial attach apn is set.
         */
        RIL_LOG_INFO("CTE_XMM7160::SetInitialAttachApn() - initial attach apn not set\r\n");
        return RRIL_RESULT_OK_IMMEDIATE;
    }

    return CTEBase::SetInitialAttachApn(rilToken, uiChannel, pParseFcn, pHandlerFcn, nextState);
}
