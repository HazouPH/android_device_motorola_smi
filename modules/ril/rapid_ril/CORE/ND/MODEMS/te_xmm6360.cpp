////////////////////////////////////////////////////////////////////////////
// te_xmm6360.cpp
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Overlay for the IMC 6360 modem
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
#include "sync_ops.h"
#include "command.h"
#include "te_xmm6360.h"
#include "rildmain.h"
#include "callbacks.h"
#include "oemhookids.h"
#include "repository.h"
#include "reset.h"
#include "data_util.h"
#include "init6360.h"
#include "base64.h"


CTE_XMM6360::CTE_XMM6360(CTE& cte)
: CTE_XMM6260(cte)
{
}

CTE_XMM6360::~CTE_XMM6360()
{
}

CInitializer* CTE_XMM6360::GetInitializer()
{
    RIL_LOG_VERBOSE("CTE_XMM6360::GetInitializer() - Enter\r\n");
    CInitializer* pRet = NULL;

    RIL_LOG_INFO("CTE_XMM6360::GetInitializer() - Creating CInit6360 initializer\r\n");
    m_pInitializer = new CInit6360();
    if (NULL == m_pInitializer)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::GetInitializer() - Failed to create a CInit6360 "
                "initializer!\r\n");
        goto Error;
    }

    pRet = m_pInitializer;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6360::GetInitializer() - Exit\r\n");
    return pRet;
}

char* CTE_XMM6360::GetBasicInitCommands(UINT32 uiChannelType)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::GetBasicInitCommands() - Enter\r\n");

    char szInitCmd[MAX_BUFFER_SIZE] = {'\0'};
    char* pInitCmd = NULL;

    pInitCmd = CTE_XMM6260::GetBasicInitCommands(uiChannelType);
    if (NULL == pInitCmd)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::GetBasicInitCommands() - Failed to get the "
                "basic init cmd string!\r\n");
        goto Done;
    }

    // Add any XMM6360-specific init strings
    //
    if (RIL_CHANNEL_ATCMD == uiChannelType)
    {
        if (m_cte.IsSupportCGPIAF())
        {
            // Set IPV6 address format: Use IPv6 colon-notation, subnet prefix CIDR notation,
            //  leading zeros omitted, zero compression
            CopyStringNullTerminate(szInitCmd, "|+CGPIAF=1,1,0,1", sizeof(szInitCmd));
        }
    }
    else if (RIL_CHANNEL_URC == uiChannelType)
    {
        CopyStringNullTerminate(szInitCmd, pInitCmd, sizeof(szInitCmd));
    }

Done:
    free(pInitCmd);
    RIL_LOG_VERBOSE("CTE_XMM6360::GetBasicInitCommands() - Exit\r\n");
    return strndup(szInitCmd, strlen(szInitCmd));
}

char* CTE_XMM6360::GetUnlockInitCommands(UINT32 uiChannelType)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::GetUnlockInitCommands() - Enter\r\n");

    char szInitCmd[MAX_BUFFER_SIZE] = {'\0'};

    if (RIL_CHANNEL_URC == uiChannelType)
    {
        ConcatenateStringNullTerminate(szInitCmd, sizeof(szInitCmd), "|+CGAUTO=0|+CRC=1");
    }

    RIL_LOG_VERBOSE("CTE_XMM6360::GetUnlockInitCommands() - Exit\r\n");
    return strndup(szInitCmd, strlen(szInitCmd));
}

const char* CTE_XMM6360::GetRegistrationInitString()
{
    return "+CREG=3|+XREG=3";
}

const char* CTE_XMM6360::GetCsRegistrationReadString()
{
    return "AT+CREG=3;+CREG?;+CREG=0\r";
}

const char* CTE_XMM6360::GetPsRegistrationReadString()
{
    return "AT+XREG=3;+XREG?;+XREG=0\r";
}

const char* CTE_XMM6360::GetLocationUpdateString(BOOL bIsLocationUpdateEnabled)
{
    if (bIsLocationUpdateEnabled)
    {
        return "AT+CREG=3\r";
    }
    else
    {
        return (SCREEN_STATE_ON == m_cte.GetScreenState()) ? "AT+CREG=3\r" : "AT+CREG=1\r";
    }
}

const char* CTE_XMM6360::GetScreenOnString()
{
    return m_cte.IsSignalStrengthReportEnabled()
            ? "AT+CREG=3;+CGREG=0;+XREG=3;+XCSQ=1\r" : "AT+CREG=3;+CGREG=0;+XREG=3\r";
}

// RIL_REQUEST_SETUP_DATA_CALL
RIL_RESULT_CODE CTE_XMM6360::CoreSetupDataCall(REQUEST_DATA& rReqData,
       void* pData, UINT32 uiDataSize, UINT32& uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::CoreSetupDataCall() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int nPapChap = 0; // no auth
    PdpData stPdpData;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    CChannel_Data* pChannelData = NULL;
    int dataProfile = -1;
    UINT32 uiDnsMode = 0;

    RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - uiDataSize=[%u]\r\n", uiDataSize);

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
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSetupDataCall() - "
                "****** No free data channels available ******\r\n");
        goto Error;
    }

    pDataCallContextData->uiCID = uiCID;

    RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - stPdpData.szRadioTechnology=[%s]\r\n",
            stPdpData.szRadioTechnology);
    RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - stPdpData.szRILDataProfile=[%s]\r\n",
            stPdpData.szRILDataProfile);
    RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - stPdpData.szApn=[%s]\r\n", stPdpData.szApn);
    RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - stPdpData.szUserName=[%s]\r\n",
            stPdpData.szUserName);
    RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - stPdpData.szPassword=[%s]\r\n",
            stPdpData.szPassword);
    RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - stPdpData.szPAPCHAP=[%s]\r\n",
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

            RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - New PAP/CHAP=[%d]\r\n", nPapChap);
        }
    }

    if (RIL_VERSION >= 4 && (uiDataSize >= (7 * sizeof(char*))))
    {
        stPdpData.szPDPType = ((char**)pData)[6]; // new in Android 2.3.4.
        RIL_LOG_INFO("CTE_XMM6360::CoreSetupDataCall() - stPdpData.szPDPType=[%s]\r\n",
                stPdpData.szPDPType);
    }

    //
    //  IP type is passed in dynamically.
    if (NULL == stPdpData.szPDPType)
    {
        //  hard-code "IPV4V6" (this is the default)
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
                RIL_LOG_CRITICAL("CTE_XMM6360::CoreSetupDataCall() -"
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
                RIL_LOG_CRITICAL("CTE_XMM6360::CoreSetupDataCall() -"
                        " cannot create CGDCONT command, stPdpData.szPDPType\r\n");
                goto Error;
            }
        break;

        default:
            RIL_LOG_CRITICAL("CTE_XMM6360::CoreSetupDataCall() -"
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

        rReqData.pContextData = (void*)pDataCallContextData;
        rReqData.cbContextData = sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA);
    }

    RIL_LOG_VERBOSE("CTE_XMM6360::CoreSetupDataCall() - Exit\r\n");
    return res;
}

BOOL CTE_XMM6360::PdpContextActivate(REQUEST_DATA& rReqData, void* pData,
                                                            UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::PdpContextActivate() - Enter\r\n");

    BOOL bRet = FALSE;
    UINT32 uiCID = 0;
    int muxControlChannel = -1;
    int muxDataChannel = -1;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    CChannel_Data* pChannelData = NULL;
    BOOL bIsHSIDirect = FALSE;
    int hsiChannel = -1;
    UINT32 uiRilChannel = 0;
    int ipcDataChannelMin = 0;
    char* szModemResourceName = {'\0'};

    if (NULL == pData ||
                    sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::PdpContextActivate() - Invalid input data\r\n");
        goto Error;
    }

    pDataCallContextData = (S_SETUP_DATA_CALL_CONTEXT_DATA*)pData;
    uiCID = pDataCallContextData->uiCID;

    // Get Channel Data according to CID
    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::PdpContextActivate() -"
                " No Data Channel found for CID %u.\r\n",
                uiCID);
        goto Error;
    }

    bIsHSIDirect = pChannelData->IsHSIDirect();
    hsiChannel =  pChannelData->GetHSIChannel();
    uiRilChannel = pChannelData->GetRilChannel();

    muxControlChannel = pChannelData->GetMuxControlChannel();
    if (-1 == muxControlChannel)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::PdpContextActivate() - Unknown mux channel\r\n");
        goto Error;
    }

    if (!bIsHSIDirect)
    {
        muxDataChannel = muxControlChannel;

        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CGACT=1,%d;+XDATACHANNEL=1,1,\"/mux/%d\",\"/mux/%d\",0\r",
                uiCID, muxControlChannel, muxDataChannel))
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::PdpContextActivate() -"
                    "  cannot create CGACT command\r\n");
            goto Error;
        }
    }
    else
    {
        if (hsiChannel < 0)
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::PdpContextActivate() - No free HSI Channel \r\n");
            goto Error;
        }

       // Get the hsi channel id
        int hsiNetworkPath = -1;

        ipcDataChannelMin = pChannelData->GetIpcDataChannelMin();

        if (ipcDataChannelMin <= hsiChannel && RIL_MAX_NUM_IPC_CHANNEL > hsiChannel)
        {
           hsiNetworkPath = hsiChannel;
        }
        else
        {
           RIL_LOG_CRITICAL("CTE_XMM6360::PdpContextActivate() - Unknown HSI Channel [%d] \r\n",
                            hsiChannel);
           goto Error;
        }

        szModemResourceName = pChannelData->GetModemResourceName();

        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CGACT=1,%d;+XDATACHANNEL=1,1,\"/mux/%d\",\"/%s/%d\",0\r",
                uiCID, muxControlChannel,
                szModemResourceName, hsiNetworkPath))
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::PdpContextActivate() -"
                    "  cannot create CGACT command\r\n");
            goto Error;
        }
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r",
                                                    sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::PdpContextActivate() - Cannot create CEER command\r\n");
    }

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CTE_XMM6360::PdpContextActivate() - Exit\r\n");
    return bRet;
}

RIL_RESULT_CODE CTE_XMM6360::ParseEnterDataState(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseEnterDataState() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    UINT32 uiCause;
    BOOL bIsHSIDirect = FALSE;
    CChannel_Data* pChannelData = NULL;
    S_SETUP_DATA_CALL_CONTEXT_DATA* pDataCallContextData = NULL;
    UINT32 uiCID = 0;

    if (NULL == rRspData.pContextData ||
            sizeof(S_SETUP_DATA_CALL_CONTEXT_DATA) != rRspData.cbContextData)
    {
        RIL_LOG_INFO("CTE_XMM6360::ParseEnterDataState() - Invalid context data\r\n");
        goto Error;
    }

    pDataCallContextData =
                    (S_SETUP_DATA_CALL_CONTEXT_DATA*)rRspData.pContextData;
    uiCID = pDataCallContextData->uiCID;

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTE_XMM6360::ParseEnterDataState() - No Data Channel for CID %u.\r\n",
                                                                    uiCID);
        goto Error;
    }

    if (ParseCEER(rRspData, uiCause))
    {
        RIL_LOG_INFO("CTE_XMM6360::ParseEnterDataState() - uiCause: %u\r\n",
                                                                    uiCause);
        int failCause = PDP_FAIL_ERROR_UNSPECIFIED;

        failCause = MapErrorCodeToRilDataFailCause(uiCause);
        pChannelData->SetDataFailCause(failCause);
        goto Error;
    }

    bIsHSIDirect = pChannelData->IsHSIDirect();

    if (!bIsHSIDirect)
    {
        // Block the read thread and then flush the tty and the channel
        // From now, any failure will lead to DataConfigDown
        pChannelData->BlockAndFlushChannel(BLOCK_CHANNEL_BLOCK_ALL, FLUSH_CHANNEL_NO_FLUSH);
        pChannelData->FlushAndUnblockChannel(UNBLOCK_CHANNEL_UNBLOCK_TTY, FLUSH_CHANNEL_FLUSH_ALL);
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseEnterDataState() - Exit\r\n");
    return res;
}

BOOL CTE_XMM6360::SetupInterface(UINT32 uiCID)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::SetupInterface() - Enter\r\n");

    BOOL bRet = FALSE;

    char szNetworkInterfaceName[IFNAMSIZ];
    struct gsm_netconfig netconfig;
    int fd = -1;
    int ret = 0;
    UINT32 uiChannel = 0;
    int state = 0;
    int networkInterfaceID = -1;
    CChannel_Data* pChannelData = NULL;
    PDP_TYPE eDataConnectionType = PDP_TYPE_IPV4;  //  dummy for now, set to IPv4.
    int dataProfile = -1;
    BOOL bIsHSIDirect = FALSE;
    int hsiChannel = -1;
    int ipcDataChannelMin = 0;
    UINT32 nw_if_pdp_mux_offset = 0;

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() - No Data Channel for CID %u.\r\n",
                                                                    uiCID);
        goto Error;
    }

    ipcDataChannelMin = pChannelData->GetIpcDataChannelMin();

    if (ipcDataChannelMin > RIL_MAX_NUM_IPC_CHANNEL)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() - Invalid data channel range (%u).\r\n",
                                                                    ipcDataChannelMin);
        goto Error;
    }

    nw_if_pdp_mux_offset = (RIL_MAX_NUM_IPC_CHANNEL - ipcDataChannelMin);

    state = pChannelData->GetDataState();
    if (E_DATA_STATE_ACTIVE != state)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() - Invalid data state %d.\r\n",
                                                                    state);
        goto Error;
    }

    bIsHSIDirect = pChannelData->IsHSIDirect();
    dataProfile =  pChannelData->GetDataProfile();
    hsiChannel =  pChannelData->GetHSIChannel();

    if (!bIsHSIDirect)
    {
        networkInterfaceID = nw_if_pdp_mux_offset + uiCID - 1;
    }
    else
    {
        if (ipcDataChannelMin <= hsiChannel && RIL_MAX_NUM_IPC_CHANNEL > hsiChannel)
        {
            networkInterfaceID = (hsiChannel - ipcDataChannelMin);
            if (networkInterfaceID < 0)
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() - Invalid network"
                        " interface ID (%d) \r\n", networkInterfaceID);
                goto Error;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() - Unknown his channel [%d] \r\n",
                                                    hsiChannel);
            goto Error;
        }
    }

    if (!PrintStringNullTerminate(szNetworkInterfaceName, sizeof(szNetworkInterfaceName),
            "%s%d", m_szNetworkInterfaceNamePrefix, networkInterfaceID))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() - Cannot set network interface name\r\n");
        goto Error;
    }
    else
    {
        RIL_LOG_INFO("CTE_XMM6360::SetupInterface() - szNetworkInterfaceName=[%s], CID=[%u]\r\n",
                szNetworkInterfaceName, uiCID);
    }

    pChannelData->SetInterfaceName(szNetworkInterfaceName);

    if (!bIsHSIDirect)
    {
        UINT32 uiChannel = pChannelData->GetRilChannel();

        // N_GSM related code
        netconfig.adaption = 3;
        netconfig.protocol = htons(ETH_P_IP);
        strncpy(netconfig.if_name, szNetworkInterfaceName, IFNAMSIZ - 1);
        netconfig.if_name[IFNAMSIZ - 1] = '\0';

        // Add IF NAME
        fd = pChannelData->GetFD();
        if (fd >= 0)
        {
            RIL_LOG_INFO("CTE_XMM6360::SetupInterface() -"
                    " ***** PUTTING channel=[%u] in DATA MODE *****\r\n", uiChannel);
            ret = ioctl( fd, GSMIOC_ENABLE_NET, &netconfig );       // Enable data channel
            if (ret < 0)
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() -"
                        " Unable to create interface %s : %s \r\n",
                        netconfig.if_name, strerror(errno));
                goto Error;
            }
        }
        else
        {
            //  No FD.
            RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() -"
                    " Could not get Data Channel chnl=[%u] fd=[%d].\r\n", uiChannel, fd);
            goto Error;
        }
    }

    pChannelData->GetDataConnectionType(eDataConnectionType);

    // set interface address(es) and bring up interface
    if (!DataConfigUp(szNetworkInterfaceName, pChannelData, eDataConnectionType))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::SetupInterface() -"
                " Unable to bringup interface ifconfig\r\n");
        goto Error;
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6360::SetupInterface() Exit\r\n");
    return bRet;
}

//
// RIL_REQUEST_BASEBAND_VERSION
//
RIL_RESULT_CODE CTE_XMM6360::CoreBasebandVersion(REQUEST_DATA& rReqData,
        void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::CoreBasebandVersion() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "at+xgendata\r",
            sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM6360::CoreBasebandVersion() - Exit\r\n");
    return res;
}

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

RIL_RESULT_CODE CTE_XMM6360::ParseBasebandVersion(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseBasebandVersion() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    /* As baseband version is stored in a system property, it cannot be longer than
     * PROPERTY_VALUE_MAX-1 characters long (PROPERTY_VALUE_MAX counts the zero termination).
     *
     * + 1 is needed here as 'sscanf' will read PROPERTY_VALUE_MAX characters AND will add the
     * zero termination (cf comment above sscanf code).
     */
    char* pszBasebandVersion = (char*) malloc(PROPERTY_VALUE_MAX + 1);
    if (NULL == pszBasebandVersion)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseBasebandVersion() - Could not allocate memory"
                "for pszBasebandVersion\r\n");
        goto Error;
    }

    memset(pszBasebandVersion, 0, PROPERTY_VALUE_MAX + 1);

    /* Modem version is what is reported between '*' in the +XGENDATA reply:
     *    +XGENDATA: "    XMM6360_REV_2.0 2013-Jul-31 13:42:17
     *    *CLV_6360_MODEM_01.1332.A*
     *    OK
     *
     * This is retrieved using 'sscanf':
     *   = %*[^*]  : consumes (without storing) everything that is not '*'
     *   = %*c     : consumes (without storing) the '*' character
     *   = %XXX[^*]: stores up to XXX chars or up to next '*' character in pszBasebandVersion
     *               XXX's numerical value is constructed by converting the PROPERTY_VALUE_MAX
     *               define to a string using the TO_STRING macro
     */
    if (!sscanf(pszRsp, "%*[^*]%*c%" TO_STRING(PROPERTY_VALUE_MAX) "[^*]", pszBasebandVersion))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseBasebandVersion() - Could not "
                "extract the baseband version string.\r\n");
        goto Error;
    }
    if (pszBasebandVersion[PROPERTY_VALUE_MAX - 1] != '\0')
    {
        RIL_LOG_WARNING("CTE_XMM6360::ParseBasebandVersion() - "
                "Modem version too long, reporting truncated version.\r\n");
        pszBasebandVersion[PROPERTY_VALUE_MAX - 1] = '\0';
    }

    if (pszBasebandVersion[0] == '\0')
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseBasebandVersion() - "
                "Invalid baseband version string.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6360::ParseBasebandVersion() - "
            "pszBasebandVersion=[%s]\r\n", pszBasebandVersion);

    rRspData.pData   = (void*)pszBasebandVersion;
    rRspData.uiDataSize  = sizeof(char*);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pszBasebandVersion);
        pszBasebandVersion = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6360::ParseBasebandVersion() - Exit\r\n");
    return res;
}

RIL_RadioTechnology CTE_XMM6360::MapAccessTechnology(UINT32 uiStdAct, int regType)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::MapAccessTechnology() ENTER  uiStdAct=[%u]\r\n", uiStdAct);

    /*
     * 20111103: There is no 3GPP standard value defined for GPRS and HSPA+
     * access technology. So, values 1 and 8 are used in relation with the
     * IMC proprietary +XREG: <Act> parameter.
     *
     * Note: GSM Compact is not supported by IMC modem.
     */
    RIL_RadioTechnology rtAct = RADIO_TECH_UNKNOWN;

    //  Check state and set global variable for network technology
    switch (uiStdAct)
    {
        /* 20130202:
         * case 9 is added for HSPA dual carrier
         */
        case 9: // Proprietary value introduced for HSPA+ DC-FSPA+
        rtAct = RADIO_TECH_HSPAP; // 15
        break;

        default:
        rtAct = CTEBase::MapAccessTechnology(uiStdAct, regType);
        break;
    }
    RIL_LOG_VERBOSE("CTE_XMM6360::MapAccessTechnology() EXIT  rtAct=[%u]\r\n", (UINT32)rtAct);
    return rtAct;
}

//
// RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
//
RIL_RESULT_CODE CTE_XMM6360::CoreSetPreferredNetworkType(REQUEST_DATA& rReqData,
        void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::CoreSetPreferredNetworkType() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_PreferredNetworkType networkType = PREF_NET_TYPE_GSM_WCDMA; // 0

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSetPreferredNetworkType() - Data "
                "pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != sizeof(int))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSetPreferredNetworkType() - "
                "Invalid data size.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM6360::CoreSetPreferredNetworkType() - "
            "Network type {%d} from framework.\r\n", ((RIL_PreferredNetworkType*)pData)[0]);

    networkType = (RIL_PreferredNetworkType) ((int*)pData)[0];

    // if network type already set, NO-OP this command
    if (m_currentNetworkType == networkType)
    {
        rReqData.szCmd1[0] = '\0';
        res = RRIL_RESULT_OK_IMMEDIATE;
        RIL_LOG_INFO("CTE_XMM6360::CoreSetPreferredNetworkType() - "
                "Network type {%d} already set.\r\n", networkType);
        goto Error;
    }

    switch (networkType)
    {
        case PREF_NET_TYPE_GSM_WCDMA: // WCDMA Preferred
        // This value is received as a result of the recovery mechanism in the framework even
        // though not supported by modem.  In this case, set to supported default value of
        // PREF_NET_TYPE_GSM_WCDMA.
        case PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO:
            networkType = PREF_NET_TYPE_GSM_WCDMA;
            RIL_LOG_VERBOSE("CTE_XMM6360::CoreSetPreferredNetworkType() - "
                    "WCDMA pref:XACT=3,1) - Enter\r\n");
            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XACT=3,1\r",
                    sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::HandleNetworkType() - Can't "
                        "construct szCmd1 networkType=%d\r\n", networkType);
                break;
            }
            res = RRIL_RESULT_OK;
            break;

        case PREF_NET_TYPE_GSM_ONLY: // GSM Only
            RIL_LOG_VERBOSE("CTE_XMM6360::CoreSetPreferredNetworkType() -"
                    "GSM only:XACT=0) - Enter\r\n");
            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XACT=0\r",
                    sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::HandleNetworkType() - Can't "
                        "construct szCmd1 networkType=%d\r\n", networkType);
                break;
            }
            res = RRIL_RESULT_OK;
            break;

        case PREF_NET_TYPE_WCDMA: // WCDMA Only
            RIL_LOG_VERBOSE("CTE_XMM6360::CoreSetPreferredNetworkType() - "
                    "WCDMA only:XACT=1) - Enter\r\n");
            if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+XACT=1\r",
                    sizeof(rReqData.szCmd1)))
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::HandleNetworkType() - Can't "
                        "construct szCmd1 networkType=%d\r\n", networkType);
                break;
            }
            res = RRIL_RESULT_OK;
            break;

        default:
            RIL_LOG_CRITICAL("CTE_XMM6360::CoreSetPreferredNetworkType() - "
                    "Undefined rat code: %d\r\n", networkType);
            res = RIL_E_MODE_NOT_SUPPORTED;
            goto Error;
    }

    //  Set the context of this command to the network type we're attempting to set
    rReqData.pContextData = (void*)networkType;  // Store this as an int.

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTE_XMM6360::CoreSetPreferredNetworkType() - Exit:%d\r\n", res);
    return res;
}

// RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
//
RIL_RESULT_CODE CTE_XMM6360::CoreGetPreferredNetworkType(REQUEST_DATA& rReqData,
        void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::CoreGetPreferredNetworkType() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+XACT?\r",
            sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM6360::CoreGetPreferredNetworkType() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTE_XMM6360::ParseGetPreferredNetworkType(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseGetPreferredNetworkType() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    UINT32 rat = 0;
    UINT32 pref = 0;

    int* pRat = (int*)malloc(sizeof(int));
    if (NULL == pRat)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseGetPreferredNetworkType() - Could "
                "not allocate memory for response.\r\n");
        goto Error;
    }

    // Skip "<prefix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseGetPreferredNetworkType() - Could "
                "not skip response prefix.\r\n");
        goto Error;
    }

    // Skip "+XACT: "
    if (!SkipString(pszRsp, "+XACT: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseGetPreferredNetworkType() - Could "
                "not skip \"+XACT: \".\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, rat, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseGetPreferredNetworkType() - Could "
                "not extract rat value.\r\n");
        goto Error;
    }

    if (FindAndSkipString(pszRsp, ",", pszRsp))
    {
        if (!ExtractUInt32(pszRsp, pref, pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseGetPreferredNetworkType() - "
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

        case 3:     // WCDMA preferred
        {
            pRat[0] = PREF_NET_TYPE_GSM_WCDMA;
            m_currentNetworkType = PREF_NET_TYPE_GSM_WCDMA;
            break;
        }

        default:
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseGetPreferredNetworkType() - "
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

    RIL_LOG_VERBOSE("CTE_XMM6360::ParseGetPreferredNetworkType() - Exit\r\n");
    return res;
}

//
// Response to AT+CGPADDR=<CID>
//
RIL_RESULT_CODE CTE_XMM6360::ParseIpAddress(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseIpAddress() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;
    UINT32 uiCID;

    // Parse prefix
    if (!FindAndSkipString(szRsp, "+CGPADDR: ", szRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() -"
                " Unable to parse \"+CGPADDR\" prefix.!\r\n");
        goto Error;
    }

    // Parse <cid>
    if (!ExtractUInt32(szRsp, uiCID, szRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() - Unable to parse <cid>!\r\n");
        goto Error;
    }

    if (uiCID > 0)
    {
        //  The response could come back as:
        //      +CGPADDR: <cid>,<PDP_Addr1>,<PDP_Addr2>
        //  PDP_Addr1 could be in IPv4, or IPv6.  PDP_Addr2 is present only for IPv4v6
        //  in which case PDP_Addr1 is IPv4 and PDP_Addr2 is IPv6.
        //
        //  When +CGPIAF is set (CGPIAF=1,1,0,1), the returned address format
        //  is the expected format by Android.
        //    IPV4: a1.a2.a3.a4
        //    IPV6: a1a2:a3a4:a5a6:a7a8:a9a10:a11a12:a13a14:a15a16

        char szPdpAddr[MAX_IPADDR_SIZE] = {'\0'};
        char szIpAddr1[MAX_IPADDR_SIZE] = {'\0'}; // IPV4
        char szIpAddr2[MAX_IPADDR_SIZE] = {'\0'}; // IPV6
        int state = E_DATA_STATE_IDLE;

        CChannel_Data* pChannelData =
                CChannel_Data::GetChnlFromContextID(uiCID);
        if (NULL == pChannelData)
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() - No Data Channel for CID %u.\r\n",
                    uiCID);
            goto Error;
        }

        state = pChannelData->GetDataState();
        if (E_DATA_STATE_ACTIVE != state)
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() - Wrong data state: %d\r\n",
                    state);
            goto Error;
        }

        // Allow reverting to old implementation for IMC IPV6 AT commands when using CGPIAF
        if (m_cte.IsSupportCGPIAF())
        {
            //  Extract ,<Pdp_Addr1>
            if (!SkipString(szRsp, ",", szRsp) ||
                    !ExtractQuotedString(szRsp, szIpAddr1, MAX_IPADDR_SIZE, szRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() -"
                        " Unable to parse <PDP_addr1>!\r\n");
                goto Error;
            }

            //  Extract ,<PDP_Addr2>
            if (SkipString(szRsp, ",", szRsp))
            {
                if (!ExtractQuotedString(szRsp, szIpAddr2, MAX_IPADDR_SIZE, szRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() -"
                            " Unable to parse <PDP_addr2>!\r\n");
                    goto Error;
                }

                RIL_LOG_INFO("CTE_XMM6360::ParseIpAddress() - IPV4 address: %s\r\n", szIpAddr1);
                RIL_LOG_INFO("CTE_XMM6360::ParseIpAddress() - IPV6 address: %s\r\n", szIpAddr2);
            }
        }
        else
        {
            //  Extract ,<Pdp_Addr1>
            if (!SkipString(szRsp, ",", szRsp) ||
                !ExtractQuotedString(szRsp, szPdpAddr, MAX_IPADDR_SIZE, szRsp))
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() - "
                        "Unable to parse <PDP_addr1>!\r\n");
                goto Error;
            }

            //  The AT+CGPADDR command doesn't return IPV4V6 format
            if (!ConvertIPAddressToAndroidReadable(szPdpAddr, szIpAddr1, MAX_IPADDR_SIZE,
                    szIpAddr2, MAX_IPADDR_SIZE))
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() -"
                        " ConvertIPAddressToAndroidReadable failed!\r\n");
                goto Error;
            }

            //  Extract ,<PDP_Addr2>
            //  Converted address is in szIpAddr2.
            if (SkipString(szRsp, ",", szRsp))
            {
                if (!ExtractQuotedString(szRsp, szPdpAddr, MAX_IPADDR_SIZE, szRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() -"
                            " Unable to parse <PDP_addr2>!\r\n");
                    goto Error;
                }

                //  The AT+CGPADDR command doesn't return IPV4V6 format.
                if (!ConvertIPAddressToAndroidReadable(szPdpAddr, szIpAddr1, MAX_IPADDR_SIZE,
                        szIpAddr2, MAX_IPADDR_SIZE))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() -"
                            " ConvertIPAddressToAndroidReadable failed! m_szIpAddr2\r\n");
                    goto Error;
                }

                //  Extract ,<PDP_Addr2>
                //  Converted address is in pChannelData->m_szIpAddr2.
                if (SkipString(szRsp, ",", szRsp))
                {
                    if (!ExtractQuotedString(szRsp, szPdpAddr, MAX_IPADDR_SIZE, szRsp))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() -"
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
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() -"
                                " ConvertIPAddressToAndroidReadable failed! m_szIpAddr2\r\n");
                        goto Error;
                    }

                RIL_LOG_INFO("CTE_XMM6360::ParseIpAddress() - IPV4 address: %s\r\n", szIpAddr1);
                RIL_LOG_INFO("CTE_XMM6360::ParseIpAddress() - IPV6 address: %s\r\n", szIpAddr2);
                }
            }
        }

        // First clear IP addresses
        pChannelData->DeleteAddressesString(pChannelData->ADDR_IP);
        // Now add IP addresses
        pChannelData->AddAddressString(pChannelData->ADDR_IP, szIpAddr1);
        pChannelData->AddAddressString(pChannelData->ADDR_IP, szIpAddr2);

        // Set the PDP type
        if (szIpAddr2[0] == '\0')
        {
            pChannelData->SetPdpType(PDPTYPE_IP);
        }
        else if (szIpAddr1[0] == '\0')
        {
            pChannelData->SetPdpType(PDPTYPE_IPV6);
        }
        else
        {
            pChannelData->SetPdpType(PDPTYPE_IPV4V6);
        }

        res = RRIL_RESULT_OK;
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseIpAddress() - uiCID=[%u] not valid!\r\n", uiCID);
    }

Error:
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseIpAddress() - Exit\r\n");
    return res;
}

//
// Response to AT+XDNS?
//
RIL_RESULT_CODE CTE_XMM6360::ParseDns(RESPONSE_DATA & rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseDns() - Enter\r\n");

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
        int state = E_DATA_STATE_IDLE;

        // Parse <cid>
        if (!ExtractUInt32(szRsp, uiCID, szRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() - Unable to parse <cid>!\r\n");
            continue;
        }

        pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
        if (NULL == pChannelData)
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() - No Data Channel for CID %u.\r\n",
                    uiCID);
            continue;
        }

        state = pChannelData->GetDataState();
        if (E_DATA_STATE_ACTIVE != state)
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() - Wrong data state: %d\r\n",
                    state);
            continue;
        }

        //  The response could come back as:
        //    +XDNS: <cid>,<Primary_DNS>,<Secondary_DNS>
        //  Primary_DNS and Secondary_DNS could be in IPV4, IPV6, or IPV4V6 format.
        //
        //  When +CGPIAF is set (CGPIAF=1,1,0,1), the returned address format
        //  is the expected format by Android.
        //    IPV4: a1.a2.a3.a4
        //    IPV6: a1a2:a3a4:a5a6:a7a8:a9a10:a11a12:a13a14:a15a16
        //    IPV4V6: a1a2:a3a4:a5a6:a7a8:a9a10:a11a12:a13a14:a15a16:a17a18:a19a20

        // Parse <primary DNS>
        if (SkipString(szRsp, ",", szRsp))
        {
            // Allow reverting to old implementation for IMC IPV6 AT commands when using CGPIAF
            if (m_cte.IsSupportCGPIAF())
            {
                if (!ExtractQuotedString(szRsp, szIpDNS1, MAX_IPADDR_SIZE, szRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() - Unable to extact szDNS 1!\r\n");
                    continue;
                }

                RIL_LOG_INFO("CTE_XMM6360::ParseDns() - Primary DNS server: %s\r\n", szIpDNS1);
            }
            else
            {
                if (!ExtractQuotedString(szRsp, szDNS, MAX_IPADDR_SIZE, szRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() - Unable to extact szDNS 1!\r\n");
                    continue;
                }

                //  Now convert to Android-readable format (and split IPv4v6 parts (if applicable)
                if (!ConvertIPAddressToAndroidReadable(szDNS,
                                                        szIpDNS1,
                                                        MAX_IPADDR_SIZE,
                                                        szIpV6DNS1,
                                                        MAX_IPADDR_SIZE))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() -"
                            " ConvertIPAddressToAndroidReadable failed! m_szDNS1\r\n");
                    continue;
                }

                RIL_LOG_INFO("CTE_XMM6360::ParseDns() - szIpDNS1: %s\r\n", szIpDNS1);

                if (strlen(szIpV6DNS1) > 0)
                {
                    RIL_LOG_INFO("CTE_XMM6360::ParseDns() - szIpV6DNS1: %s\r\n", szIpV6DNS1);
                }
                else
                {
                    RIL_LOG_INFO("CTE_XMM6360::ParseDns() - szIpV6DNS1: <NONE>\r\n");
                }
            }
        }

        // Parse <secondary DNS>
        if (SkipString(szRsp, ",", szRsp))
        {
            if (m_cte.IsSupportCGPIAF())
            {
                if (!ExtractQuotedString(szRsp, szIpDNS2, MAX_IPADDR_SIZE, szRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() - Unable to extact szDNS 2!\r\n");
                    continue;
                }

                RIL_LOG_INFO("CTE_XMM6360::ParseDns() - Secondary DNS server: %s\r\n", szIpDNS2);
            }
            else
            {
                if (!ExtractQuotedString(szRsp, szDNS, MAX_IPADDR_SIZE, szRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() - Unable to extact szDNS 2!\r\n");
                    continue;
                }

                //  Now convert to Android-readable format (and split IPv4v6 parts (if applicable)
                if (!ConvertIPAddressToAndroidReadable(szDNS,
                                                        szIpDNS2,
                                                        MAX_IPADDR_SIZE,
                                                        szIpV6DNS2,
                                                        MAX_IPADDR_SIZE))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseDns() -"
                            " ConvertIPAddressToAndroidReadable failed! szIpDNS2\r\n");
                    continue;
                }

                RIL_LOG_INFO("CTE_XMM6360::ParseDns() - szIpDNS2: %s\r\n", szIpDNS2);

                if (strlen(szIpV6DNS2) > 0)
                {
                    RIL_LOG_INFO("CTE_XMM6360::ParseDns() - szIpV6DNS2: %s\r\n", szIpV6DNS2);
                }
                else
                {
                    RIL_LOG_INFO("CTE_XMM6360::ParseDns() - szIpV6DNS2: <NONE>\r\n");
                }
            }
        }

        if (m_cte.IsSupportCGPIAF())
        {
            // For IPV6 addresses, copy to szIpV6DNS variables and set szIpDNS ones to empty
            if (strstr(szIpDNS1, ":"))
            {
                RIL_LOG_INFO("CTE_XMM6360::ParseDns() - IPV6 DNS addresses\r\n");

                CopyStringNullTerminate(szIpV6DNS1, szIpDNS1, MAX_IPADDR_SIZE);
                szIpDNS1[0] = '\0';

                CopyStringNullTerminate(szIpV6DNS2, szIpDNS2, MAX_IPADDR_SIZE);
                szIpDNS2[0] = '\0';
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
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseDns() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
//
RIL_RESULT_CODE CTE_XMM6360::CoreReportStkServiceRunning(REQUEST_DATA& rReqData,
                                                                     void* /*pData*/,
                                                                     UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::CoreReportStkServiceRunning - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CFUN=6\r"))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM6360::CoreReportStkServiceRunning() - Exit\r\n");
    return res;
}

const char* CTE_XMM6360::GetEnableFetchingString()
{
    /*
     * STK is disabled by sending all bytes of terminal profile set to 0.
     * This is already taken care as part of nvm configuration file. In order
     * to get the EAP-SIM authentication working, rapid ril needs to enable
     * proactive fetching. By sending CFUN=7, SIMTK is disabled and proactive fetching is enabled.
     */

     const char* pszEnableFetching = NULL;
     if (!m_cte.IsStkCapable())
     {
        pszEnableFetching = "|+CFUN=7";
     }

    return pszEnableFetching;
}

//
// RIL_REQUEST_SIM_AUTHENTICATION
//
RIL_RESULT_CODE CTE_XMM6360::CoreSimAuthentication(REQUEST_DATA& reqData,
        void* pData, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::CoreSimAuthentication() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    // authentication token length is 128 bits, ASCII representation will require
    // 32 bytes + null terminated
    char szAutn[AUTN_LENGTH + 1] = {'\0'};
    // random challenge token length is 128 bits, ASCII representation will require
    // 32 bytes + null terminated
    char szRand[RAND_LENGTH + 1] = {'\0'};
    RIL_SimAuthentication* pSimAuthArgs = NULL;
    int sessionId = -1;
    int authContextType = 0;
    const int AUTH_BUFFER_LEN = 34;
    const UINT8 dataBuffer[AUTH_BUFFER_LEN] = { 0 };
    int dataLen = 0;
    int randLen = 0;
    int autnLen = 0;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSimAuthentication() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    pSimAuthArgs = (RIL_SimAuthentication*)pData;

    if (P2_AUTH_IMS_AKA == pSimAuthArgs->authContext)
    {
        char szAid[MAX_AID_SIZE] = {'\0'};
        GetSimAppId(RIL_APPTYPE_ISIM, szAid, sizeof(szAid));

        if (RIL_APPSTATE_READY == GetIsimAppState()
                && NULL != pSimAuthArgs->aid
                && (0 == strcmp(pSimAuthArgs->aid, szAid)))
        {
            RIL_LOG_INFO("CTE_XMM6360::CoreSimAuthentication() -"
                    " open a logical channel for IMS\r\n");

            sessionId = GetSessionId(RIL_APPTYPE_ISIM);
            POST_CMD_HANDLER_DATA data;
            memset(&data, 0, sizeof(data));
            data.uiChannel = g_pReqInfo[RIL_REQUEST_SIM_IO].uiChannel;
            data.requestId = RIL_REQUEST_SIM_IO;

            CEvent::Reset(m_pUiccOpenLogicalChannelEvent);

            if (-1 == sessionId && OpenLogicalChannel(data, szAid))
            {
                CEvent::Wait(m_pUiccOpenLogicalChannelEvent, WAIT_FOREVER);
            }

            sessionId = GetSessionId(RIL_APPTYPE_ISIM);
            if (sessionId < 0)
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::CoreSimAuthentication() -"
                        " OpenLogicalChannel failed\r\n");
                goto Error;
            }
            authContextType = XAUTH_CONTEXT_TYPE_IMS;
        }
    }

    switch (pSimAuthArgs->authContext)
    {
        case P2_AUTH_GSM_CONTEXT:
            sessionId = 0;
            authContextType = XAUTH_CONTEXT_TYPE_GSM;
            break;
        case P2_AUTH_3G_CONTEXT: // = P2_AUTH_IMS_AKA
            if (XAUTH_CONTEXT_TYPE_IMS != authContextType)
            {
                // 3G security context
                sessionId = 0;
                authContextType = XAUTH_CONTEXT_TYPE_3G;
            }
            break;
        default:
            goto Error;
    }

    // the challenge string authData is encoded in Base64 format, it must be decoded
    dataLen = util_b64_pton(pSimAuthArgs->authData, (unsigned char*)dataBuffer, AUTH_BUFFER_LEN);
    if (dataLen < 0 || dataLen > AUTH_BUFFER_LEN)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSimAuthentication() -"
                " Could not convert from base64 \r\n");
        goto Error;
    }

    // read the first byte to get the length
    randLen = 0xff & dataBuffer[0];

    if ((randLen > RAND_LENGTH/2) || ((1 + randLen) > dataLen))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSimAuthentication() -"
                " Received length higher than buffer size\r\n");
        goto Error;
    }
    if (!convertByteArrayIntoString(&dataBuffer[1], randLen, szRand))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSimAuthentication() -"
                " Could not convert input into string\r\n");
        goto Error;
    }

    if (P2_AUTH_3G_CONTEXT == pSimAuthArgs->authContext) // == P2_AUTH_IMS_AKA
    {
        /*
         * Either 3G security context or IMS AKA:
         * Both processed the same way with RAND, AUTN challenge.
         * Differentiation done by authContextType.
         */
        autnLen = 0xff & dataBuffer[1 + randLen];
        if ((autnLen > AUTN_LENGTH/2) || ((1 + randLen + 1 + autnLen) > dataLen))
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::CoreSimAuthentication() -"
                    " Received length higher than buffer size\r\n");
            goto Error;
        }

        // 2 = RAND length field(1) + AUTN length field(1)
        if (!convertByteArrayIntoString(&dataBuffer[randLen + 2], autnLen, szAutn))
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::CoreSimAuthentication() -"
                    " Could not convert input into string\r\n");
            goto Error;
        }
    }

    if (!PrintStringNullTerminate(reqData.szCmd1, sizeof(reqData.szCmd1),
            "AT+XAUTH=%d,%d,\"%s\",\"%s\"\r", sessionId, authContextType, szRand, szAutn))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::CoreSimAuthentication() -"
                " Cannot create XAUTH command \r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK == res)
    {
        int* type = (int*)malloc(sizeof(int));
        if (type != NULL)
        {
            *type = authContextType;
            reqData.pContextData = type;
            reqData.cbContextData = sizeof(int);
        }
    }
    RIL_LOG_VERBOSE("CTE_XMM6360::CoreSimAuthentication() - Exit\r\n");
    return res;
}

/*
 * The response is received in separate strings representing response keys
 * (bytes array presented as hex strings).
 * The strings will be parsed, and the response is built in a byte buffer containing
 * the AUTH RESPONSE message as specified in 3GPP 31.102.
 * Byte buffer is encoded into a Base64 string representation to fill in response parameter
 * of response structure.
 */
RIL_RESULT_CODE CTE_XMM6360::ParseSimAuthentication(RESPONSE_DATA& rspData)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::ParseSimAuthentication() - Enter\r\n");

    const int MAX_SIM_AUTH_PARAMS = 4;
    const int SUCCESS_3G_AUTH_TAG = 0xDB;
    const int SYNC_FAILURE_TAG = 0xDC;
    // <status> values returned by +XAUTH
    const int STATUS_SUCCESS = 0;
    const int STATUS_FAIL_SYNC = 1;
    const int STATUS_FAIL_MAC = 2;
    const int STATUS_FAIL_SECURITY_CONTEXT = 3;
    const size_t MAX_SRES_PARAM_LEN = 8;
    const size_t KC_PARAM_LEN = 16;
    const size_t MIN_RES_PARAM_LEN = 8;
    const size_t MAX_RES_PARAM_LEN = 32;
    const size_t CK_PARAM_LEN = 32;
    const size_t IK_PARAM_LEN = 32;
    const size_t AUTS_PARAM_LEN = 28;
    const size_t MAX_AUTN_PARAM_LEN = 32;
    const char* pszRsp = rspData.szResponse;
    const size_t MAX_DATA_BUFFER_SIZE = 1/*3G AUTH TAG BYTE*/
            + 1/*RES LEN BYTE*/ + MAX_RES_PARAM_LEN/2/*MAX RES LEN*/
            + 1/*CK LEN BYTE*/ + CK_PARAM_LEN/2/*CK LEN*/
            + 1/*IK LEN BYTE*/ + IK_PARAM_LEN/2/*IK LEN*/
            + 1/*Kc LEN BYTE*/ + KC_PARAM_LEN/2/*Kc LEN*/;

    enum SimAuthParams
    {
        GSM_KC = 0,
        RES = 0,
        GSM_SRES = 1,
        CK = 1,
        IK = 2,
        KC = 3,
        AUTS = 0
    };

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int respLen = 0;
    char* pszResult = NULL;
    int status;
    int sw1 = 0;
    int sw2 = 0;
    char szTemp[MAX_SIM_AUTH_PARAMS][MAX_AUTN_PARAM_LEN + 1] = {'\0'};
    char szBase64Rsp[2 * MAX_DATA_BUFFER_SIZE] = {'\0'};
    UINT8 dataBuffer[MAX_DATA_BUFFER_SIZE] = { 0 };
    size_t paramLen[MAX_SIM_AUTH_PARAMS] = { 0 };
    int writeMarker = 0;
    size_t simAuthLen = 0;
    int authContextType = 0;
    RIL_SIM_IO_Response* pResponse = NULL;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                " Response String pointer is NULL.\r\n");
        goto Error;
    }

    /*
     * +XAUTH: <status>[,<Kc>,<SRES>][<RES/AUTS>,<CK>,<IK>,<kc>][ Ks_ext_NAF]
     */
    if (!FindAndSkipString(pszRsp, "+XAUTH: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                "Unable to parse \"+XAUTH: \" prefix.\r\n");
        goto Error;
    }

    if (!ExtractInt(pszRsp, status, pszRsp))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                " Error parsing status.\r\n");
        goto Error;
    }

    if (rspData.pContextData != NULL && sizeof(int) == rspData.cbContextData)
    {
        authContextType = *((int*)rspData.pContextData);
    }
    else
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                " Invalid authContextType\r\n");
        goto Error;
    }

    if (STATUS_SUCCESS == status || STATUS_FAIL_SYNC == status)
    {
        // Success, need to parse the extra parameters...
        if (!SkipString(pszRsp, ",", pszRsp))
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                    " Error parsing response.\r\n");
            goto Error;
        }

        switch (status)
        {
            case STATUS_SUCCESS:
                /*
                 * GSM context auth success
                 * +XAUTH: 0,<Kc>,<SRES>
                 */
                if (XAUTH_CONTEXT_TYPE_GSM == authContextType)
                {
                    /*
                     * Format of the keys parameters in response is a hex string representing
                     * the bytes of the key.
                     */
                    if (!ExtractQuotedString(pszRsp, szTemp[GSM_KC], MAX_AUTN_PARAM_LEN + 1,
                            pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Error parsing Kc.\r\n");
                        goto Error;
                    }
                    paramLen[GSM_KC] = strlen(szTemp[GSM_KC]);
                    if (paramLen[GSM_KC] != KC_PARAM_LEN)
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Invalid parameter length.\r\n");
                        goto Error;
                    }

                    if (!SkipString(pszRsp, ",", pszRsp)
                            || (!ExtractQuotedString(pszRsp, szTemp[GSM_SRES],
                                    MAX_AUTN_PARAM_LEN + 1, pszRsp)))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Error parsing Res.\r\n");
                        goto Error;
                    }
                    paramLen[GSM_SRES] = strlen(szTemp[GSM_SRES]);
                    if (paramLen[GSM_SRES] != MAX_SRES_PARAM_LEN)
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Invalid parameter length.\r\n");
                        goto Error;
                    }
                }
                else
                {
                    /*
                     * 3G context auth success
                     * +XAUTH: 0,<RES>,<CK>,<IK>,<kc>   // kc could be empty/not sent
                     *
                     * IMS AKA context auth success
                     * +XAUTH: 0,<RES>,<CK>,<IK>
                     *
                     * Format of the keys parameters in response is a hex string representing
                     * the bytes of the key.
                     */
                    if (!ExtractQuotedString(pszRsp, szTemp[RES], MAX_AUTN_PARAM_LEN + 1, pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Error parsing Res.\r\n");
                        goto Error;
                    }
                    paramLen[RES] = strlen(szTemp[RES]);
                    if (paramLen[RES] < MIN_RES_PARAM_LEN
                            || paramLen[RES] > MAX_RES_PARAM_LEN)
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Invalid parameter length.\r\n");
                        goto Error;
                    }

                    /*
                     * Format of the keys parameters in response is a hex string representing
                     * the bytes of the key.
                     */
                    if (!SkipString(pszRsp, ",", pszRsp)
                            || (!ExtractQuotedString(pszRsp, szTemp[CK], MAX_AUTN_PARAM_LEN + 1,
                                    pszRsp)))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Error parsing CK.\r\n");
                        goto Error;
                    }
                    paramLen[CK] = strlen(szTemp[CK]);
                    if (paramLen[CK] != CK_PARAM_LEN)
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Invalid parameter length.\r\n");
                        goto Error;
                    }

                    /*
                     * Format of the keys parameters in response is a hex string representing
                     * the bytes of the key.
                     */
                    if (!SkipString(pszRsp, ",", pszRsp)
                            || (!ExtractQuotedString(pszRsp, szTemp[IK], MAX_AUTN_PARAM_LEN + 1,
                                    pszRsp)))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Error parsing IK.\r\n");
                        goto Error;
                    }
                    paramLen[IK] = strlen(szTemp[IK]);
                    if (paramLen[IK] != IK_PARAM_LEN)
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Invalid parameter length.\r\n");
                        goto Error;
                    }

                    /*
                     * Format of the keys parameters in response is a hex string representing
                     * the bytes of the key.
                     */
                    if (XAUTH_CONTEXT_TYPE_3G == authContextType
                            && SkipString(pszRsp, ",", pszRsp))
                    {
                        if (ExtractQuotedString(pszRsp, szTemp[KC], MAX_AUTN_PARAM_LEN + 1, pszRsp))
                        {
                            paramLen[KC] = strlen(szTemp[KC]);
                            if (paramLen[KC] != KC_PARAM_LEN)
                            {
                                RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                        " Invalid parameter length.\r\n");
                                goto Error;
                            }
                        }
                        else
                        {
                            RIL_LOG_INFO("CTE_XMM6360::ParseSimAuthentication() -"
                                    " Error parsing or Kc missing.\r\n");
                        }
                    }
                }
                break;
            case STATUS_FAIL_SYNC:
                /*
                 * Format of the keys parameters in response is a hex string representing
                 * the bytes of the key.
                 */
                if (!ExtractQuotedString(pszRsp, szTemp[AUTS], MAX_AUTN_PARAM_LEN + 1, pszRsp))
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                            " Error parsing AUTS.\r\n");
                    goto Error;
                }
                paramLen[AUTS] = strlen(szTemp[AUTS]);
                if (paramLen[AUTS] != AUTS_PARAM_LEN)
                {
                    RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                            " Invalid parameter length.\r\n");
                    goto Error;
                }
                break;
            default:
                break;
        }
    }

    switch (status)
    {
        case STATUS_SUCCESS:
            switch (authContextType)
            {
                case XAUTH_CONTEXT_TYPE_GSM:
                    // output length = byte(SRES length) + SRES array + byte(Kc length) + Kc array
                    simAuthLen = 1 + paramLen[GSM_SRES]/2 + 1 + paramLen[GSM_KC]/2;
                    if (simAuthLen > MAX_DATA_BUFFER_SIZE)
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " received size higher than buffer length\r\n");
                        goto Error;
                    }
                    dataBuffer[writeMarker] = paramLen[GSM_SRES]/2;
                    writeMarker++;
                    if (!extractByteArrayFromString(szTemp[GSM_SRES], paramLen[GSM_SRES],
                            &dataBuffer[writeMarker]))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Could not extract byte array from String\r\n");
                        goto Error;
                    }
                    writeMarker += paramLen[GSM_SRES]/2;

                    dataBuffer[writeMarker] = paramLen[GSM_KC]/2;
                    writeMarker++;
                    if (!extractByteArrayFromString(szTemp[GSM_KC], paramLen[GSM_KC],
                            &dataBuffer[writeMarker]))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Could not extract byte array from String\r\n");
                        goto Error;
                    }
                    writeMarker += paramLen[GSM_KC]/2;
                    sw1 = 0x90;
                    sw2 = 0x00;
                    break;
                case XAUTH_CONTEXT_TYPE_3G:
                case XAUTH_CONTEXT_TYPE_IMS:
                    /*
                     * output length = byte(DB) + byte(RES length) + RES array + byte(CK length)
                     * + CK array + byte(IK) + IK array + byte(Kc) + Kc array
                     */
                    simAuthLen = (paramLen[KC] > 0)
                            ? 1 + 1 + paramLen[RES]/2 + 1 + paramLen[CK]/2 + 1 + paramLen[IK]/2 + 1
                                    + paramLen[KC]/2
                            : 1 + 1 + paramLen[RES]/2 + 1 + paramLen[CK]/2 + 1 + paramLen[IK]/2;
                    if (simAuthLen > MAX_DATA_BUFFER_SIZE)
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " received size higher than buffer length\r\n");
                        goto Error;
                    }

                    dataBuffer[writeMarker] = SUCCESS_3G_AUTH_TAG;
                    writeMarker++;
                    dataBuffer[writeMarker] = paramLen[RES]/2;
                    writeMarker++;
                    if (!extractByteArrayFromString(szTemp[RES], paramLen[RES],
                            &dataBuffer[writeMarker]))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Could not extract byte array from String\r\n");
                        goto Error;
                    }
                    writeMarker += paramLen[RES]/2;

                    dataBuffer[writeMarker] = paramLen[CK]/2;
                    writeMarker++;
                    if (!extractByteArrayFromString(szTemp[CK], paramLen[CK],
                            &dataBuffer[writeMarker]))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Could not extract byte array from String\r\n");
                        goto Error;
                    }
                    writeMarker += paramLen[CK]/2;

                    dataBuffer[writeMarker] = paramLen[IK]/2;
                    writeMarker++;
                    if (!extractByteArrayFromString(szTemp[IK], paramLen[IK],
                            &dataBuffer[writeMarker]))
                    {
                        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                " Could not extract byte array from String\r\n");
                        goto Error;
                    }
                    writeMarker += paramLen[IK]/2;

                    // applies only for 3G authentication
                    if (paramLen[KC] > 0)
                    {
                        dataBuffer[writeMarker] = paramLen[KC]/2;
                        writeMarker++;
                        if (!extractByteArrayFromString(szTemp[KC], paramLen[KC],
                                &dataBuffer[writeMarker]))
                        {
                            RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                                    " Could not extract byte array from String\r\n");
                            goto Error;
                        }
                        writeMarker += paramLen[KC]/2;
                    }

                    sw1 = 0x90;
                    sw2 = 0x00;
                    break;
                default:
                    break;
            }
            break;
        case STATUS_FAIL_SYNC:
            // output length = byte(tag DC) + byte(AUTS length) + AUTS array
            simAuthLen = 1 + 1 + paramLen[AUTS]/2;
            if (simAuthLen > MAX_DATA_BUFFER_SIZE)
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                        " received size higher than buffer length\r\n");
                goto Error;
            }
            dataBuffer[writeMarker] = SYNC_FAILURE_TAG;
            writeMarker++;
            dataBuffer[writeMarker] = paramLen[AUTS]/2;
            writeMarker++;
            if (!extractByteArrayFromString(szTemp[AUTS], paramLen[AUTS], &dataBuffer[writeMarker]))
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                        " Could not extract byte array from String\r\n");
                goto Error;
            }
            sw1 = 0x98;
            sw2 = 0x63;
            break;
        case STATUS_FAIL_MAC:
            sw1 = 0x98;
            sw2 = 0x62;
            break;
        case STATUS_FAIL_SECURITY_CONTEXT:
            sw1 = 0x98;
            sw2 = 0x64;
            break;
        default:
            sw1 = 0x6F;
            sw2 = 0x00;
            break;
    }

    if (STATUS_SUCCESS == status || STATUS_FAIL_SYNC == status)
    {
        // the response is encoded in Base64 format, see 3GPP TS 31.102 7.1.2
        respLen = util_b64_ntop(dataBuffer, simAuthLen, szBase64Rsp, sizeof(szBase64Rsp));
        if (respLen < 0)
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                    " Could not encode the response in Base64 format.\r\n");
            goto Error;
        }
    }

    pResponse = (RIL_SIM_IO_Response*)malloc(sizeof(RIL_SIM_IO_Response) + respLen + 1);
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                " Could not allocate memory for a RIL_SIM_IO_Response struct.\r\n");
        goto Error;
    }
    memset(pResponse, 0, sizeof(RIL_SIM_IO_Response) + respLen + 1);

    pResponse->simResponse = ((char*)pResponse) + sizeof(RIL_SIM_IO_Response);
    if (!CopyStringNullTerminate(pResponse->simResponse, szBase64Rsp, respLen + 1))
    {
        RIL_LOG_CRITICAL("CTE_XMM6360::ParseSimAuthentication() -"
                " Could not copy the base64 response.\r\n");
        goto Error;
    }

    pResponse->sw1 = sw1;
    pResponse->sw2 = sw2;

    rspData.pData = pResponse;
    rspData.uiDataSize = sizeof(RIL_SIM_IO_Response);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    RIL_LOG_VERBOSE("CTE_XMM6360::ParseSimAuthentication() - Exit\r\n");
    return res;
}

void CTE_XMM6360::PostSimAuthentication(POST_CMD_HANDLER_DATA& data)
{
    if (data.pContextData != NULL && sizeof(int) == data.uiContextDataSize)
    {
        if (XAUTH_CONTEXT_TYPE_IMS == *((int*)data.pContextData))
        {
            CloseLogicalChannel(RIL_APPTYPE_ISIM);
        }
    }

    free(data.pContextData);
    data.pContextData = NULL;
    data.uiContextDataSize = 0;

    if (data.pRilToken != NULL)
    {
        RIL_onRequestComplete(data.pRilToken, (RIL_Errno) data.uiResultCode,
                data.pData, data.uiDataSize);
    }
}

void CTE_XMM6360::CloseLogicalChannel(const int appType)
{
    RIL_LOG_VERBOSE("CTE_XMM6360::CloseLogicalChannel - Enter\r\n");

    REQUEST_DATA rReqData;
    int sessionId = 0;

    sessionId = GetSessionId(appType);
    if (sessionId < 0)
    {
        return;
    }
    else
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CCHC=%d\r", sessionId))
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::CloseLogicalChannel -"
                    " Cannot create CCHC command\r\n");
            return;
        }

        CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_SIM_IO].uiChannel,
                NULL, RIL_REQUEST_SIM_IO, rReqData, &CTE::ParseSimCloseChannel);

        if (pCmd)
        {
            if (!CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("CTE_XMM6360::CloseLogicalChannel -"
                        " Unable to queue command!\r\n");
                delete pCmd;
                pCmd = NULL;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CTE_XMM6360::CloseLogicalChannel - "
                    "Unable to allocate memory for new command!\r\n");
        }
    }

    RIL_LOG_VERBOSE("CTE_XMM6360::CloseLogicalChannel - Exit\r\n");
}
