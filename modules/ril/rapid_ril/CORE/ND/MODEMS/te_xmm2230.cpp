////////////////////////////////////////////////////////////////////////////
// te_xmm2230.cpp
//
// Copyright (C) Intel 2014.
//
//
// Description:
//    Overlay for the IMC 2230 modem
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
#include "te_xmm2230.h"
#include "rildmain.h"
#include "callbacks.h"
#include "oemhookids.h"
#include "repository.h"
#include "reset.h"
#include "data_util.h"
#include "init2230.h"


CTE_XMM2230::CTE_XMM2230(CTE& cte)
: CTE_XMM6260(cte)
{
}

CTE_XMM2230::~CTE_XMM2230()
{
}

CInitializer* CTE_XMM2230::GetInitializer()
{
    RIL_LOG_VERBOSE("CTE_XMM2230::GetInitializer() - Enter\r\n");
    CInitializer* pRet = NULL;

    RIL_LOG_INFO("CTE_XMM2230::GetInitializer() - Creating CInit2230 initializer\r\n");
    m_pInitializer = new CInit2230();
    if (NULL == m_pInitializer)
    {
        RIL_LOG_CRITICAL("CTE_XMM2230::GetInitializer() - Failed to create a CInit2230 "
                "initializer!\r\n");
        goto Error;
    }

    pRet = m_pInitializer;

Error:
    RIL_LOG_VERBOSE("CTE_XMM2230::GetInitializer() - Exit\r\n");
    return pRet;
}

char* CTE_XMM2230::GetUnlockInitCommands(UINT32 uiChannelType)
{
    RIL_LOG_VERBOSE("CTE_XMM2230::GetUnlockInitCommands() - Enter\r\n");

    char szInitCmd[MAX_BUFFER_SIZE] = {'\0'};

    if (RIL_CHANNEL_URC == uiChannelType)
    {
        ConcatenateStringNullTerminate(szInitCmd, sizeof(szInitCmd), "|+CGAUTO=0|+CRC=1");
    }

    RIL_LOG_VERBOSE("CTE_XMM2230::GetUnlockInitCommands() - Exit\r\n");
    return strndup(szInitCmd, strlen(szInitCmd));
}

const char* CTE_XMM2230::GetRegistrationInitString()
{
    return "+CREG=2";
}

const char* CTE_XMM2230::GetCsRegistrationReadString()
{
    return "AT+CREG=2;+CREG?;+CREG=0\r";
}

const char* CTE_XMM2230::GetScreenOnString()
{
    return m_cte.IsSignalStrengthReportEnabled()
            ? "AT+CREG=2;+XCSQ=1\r" : "AT+CREG=2\r";
}

//
// RIL_REQUEST_BASEBAND_VERSION
//
RIL_RESULT_CODE CTE_XMM2230::CoreBasebandVersion(REQUEST_DATA& rReqData,
        void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTE_XMM2230::CoreBasebandVersion() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "at+xgendata\r",
            sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTE_XMM2230::CoreBasebandVersion() - Exit\r\n");
    return res;
}

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

RIL_RESULT_CODE CTE_XMM2230::ParseBasebandVersion(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTE_XMM2230::ParseBasebandVersion() - Enter\r\n");

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
        RIL_LOG_CRITICAL("CTE_XMM2230::ParseBasebandVersion() - Could not allocate memory"
                "for pszBasebandVersion\r\n");
        goto Error;
    }

    memset(pszBasebandVersion, 0, PROPERTY_VALUE_MAX + 1);

    /* Modem version is what is reported between '*' in the +XGENDATA reply:
     *    +XGENDATA: "    XMM2230_REV_2.0 2013-Jul-31 13:42:17
     *    *CLV_2230_MODEM_01.1332.A*
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
        RIL_LOG_CRITICAL("CTE_XMM2230::ParseBasebandVersion() - Could not "
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
        RIL_LOG_CRITICAL("CTE_XMM2230::ParseBasebandVersion() - "
                "Invalid baseband version string.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTE_XMM2230::ParseBasebandVersion() - "
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

    RIL_LOG_VERBOSE("CTE_XMM2230::ParseBasebandVersion() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIM_TRANSMIT_BASIC
//
RIL_RESULT_CODE CTE_XMM2230::CoreSimTransmitBasic(REQUEST_DATA& /*rReqData*/,
                                                         void* /*pData*/,
                                                         UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("XMM2230::CoreSimTransmitBasic() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_NOTSUPPORTED;

    RIL_LOG_VERBOSE("XMM2230::CoreSimTransmitBasic() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIM_OPEN_CHANNEL
//
RIL_RESULT_CODE CTE_XMM2230::CoreSimOpenChannel(REQUEST_DATA& /*rReqData*/,
                                                       void* /*pData*/,
                                                       UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM2230::CoreSimOpenChannel() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_NOTSUPPORTED;

    RIL_LOG_VERBOSE("CTE_XMM2230::CoreSimOpenChannel() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIM_CLOSE_CHANNEL
//
RIL_RESULT_CODE CTE_XMM2230::CoreSimCloseChannel(REQUEST_DATA& /*rReqData*/,
                                                        void* /*pData*/,
                                                        UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM2230::CoreSimCloseChannel() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_NOTSUPPORTED;

    RIL_LOG_VERBOSE("CTE_XMM2230::CoreSimCloseChannel() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIM_TRANSMIT_CHANNEL
//
RIL_RESULT_CODE CTE_XMM2230::CoreSimTransmitChannel(REQUEST_DATA& /*rReqData*/,
                                                           void* /*pData*/,
                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTE_XMM2230::CoreSimTransmitChannel() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_NOTSUPPORTED;

    RIL_LOG_VERBOSE("CTE_XMM2230::CoreSimTransmitChannel() - Exit\r\n");
    return res;
}
