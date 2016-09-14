/////////////////////////////////////////////////////////////////////////////
// te_base.cpp
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

#include <wchar.h>
#include <limits.h>
#include <cutils/properties.h>

#include "types.h"
#include "rillog.h"
#include "util.h"
#include "extract.h"
#include "nd_structs.h"
#include "callbacks.h"
#include "oemhookids.h"
#include "command.h"
#include "cmdcontext.h"
#include "repository.h"
#include "rildmain.h"
#include "te.h"
#include "te_base.h"
#include "reset.h"
#include "channel_data.h"
#include "data_util.h"
#include <cutils/properties.h>
#include "ril_result.h"
#include "initializer.h"

const int AT_MAXARGS = 20;
const int WAIT_TIMEOUT_DTMF_STOP = 20000;

const char* CTEBase::PDPTYPE_IPV4V6 = "IPV4V6";
const char* CTEBase::PDPTYPE_IPV6 = "IPV6";
const char* CTEBase::PDPTYPE_IP = "IP";
const char* CTEBase::UNREGISTERED_SEARCHING = "2";

CTEBase::CTEBase(CTE& cte)
: m_cte(cte),
  m_cTerminator('\r'),
  m_pInitializer(NULL),
  m_ePin2State(RIL_PINSTATE_UNKNOWN),
  m_pDtmfStopReqEvent(NULL),
  m_bReadyForAttach(FALSE),
  m_bRefreshWithUSIMInitOn(FALSE),
  m_pUiccOpenLogicalChannelEvent(NULL),
  m_bRegStatusAndBandIndActivated(false),
  m_bNeedGetInfoOnCellChange(false),
  m_bCoexRegStatusAndBandIndActivated(false),
  m_bCoexReportActivated(false)
{
    CRepository repository;
    strcpy(m_szNetworkInterfaceNamePrefix, "");
    CopyStringNullTerminate(m_szNewLine, "\r\n", sizeof(m_szNewLine));

    //  Grab the network interface name
    if (!repository.Read(g_szGroupModem, g_szNetworkInterfaceNamePrefix,
            m_szNetworkInterfaceNamePrefix, MAX_BUFFER_SIZE))
    {
        RIL_LOG_CRITICAL("CTEBase::CTEBase() - Could not read network interface name prefix from"
                " repository\r\n");
        strcpy(m_szNetworkInterfaceNamePrefix, "");
    }
    else
    {
        RIL_LOG_INFO("CTEBase::CTEBase() - m_szNetworkInterfaceNamePrefix=[%s]\r\n",
                m_szNetworkInterfaceNamePrefix);
    }
    memset(m_szPIN, 0, MAX_PIN_SIZE);
    memset(&m_IncomingCallInfo, 0, sizeof(m_IncomingCallInfo));
    memset(&m_PinRetryCount, -1, sizeof(m_PinRetryCount));
    memset(&m_VoiceCallInfo, -1, sizeof(m_VoiceCallInfo));
    memset(&m_sRegStatusAndBandInfo, 0, sizeof(sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND));

    m_pDtmfStopReqEvent = new CEvent(NULL, TRUE);
    m_pCardStatusUpdateLock = new CMutex();

    m_pUiccOpenLogicalChannelEvent = new CEvent(NULL, TRUE);

    ResetInitialAttachApn();

    ResetCardStatus(TRUE);
}

void CTEBase::ResetCardStatus(BOOL bForceReset)
{
    CMutex::Lock(m_pCardStatusUpdateLock);

    m_szUICCID[0] = '\0';

    if (bForceReset)
    {
        memset(&m_CardStatusCache, 0, sizeof(m_CardStatusCache));

        // Fill in the default values
        m_CardStatusCache.gsm_umts_subscription_app_index = -1;
        m_CardStatusCache.cdma_subscription_app_index = -1;
        m_CardStatusCache.ims_subscription_app_index = -1;
        m_CardStatusCache.universal_pin_state = RIL_PINSTATE_UNKNOWN;
        m_CardStatusCache.card_state = RIL_CARDSTATE_ABSENT;
        m_CardStatusCache.num_applications = 0;
    }
    else if (RIL_CARDSTATE_PRESENT == m_CardStatusCache.card_state)
    {
        for (int i = 0; i < m_CardStatusCache.num_applications; i++)
        {
            m_CardStatusCache.applications[i].app_type = RIL_APPTYPE_UNKNOWN;
            m_CardStatusCache.applications[i].app_state = RIL_APPSTATE_UNKNOWN;
            m_CardStatusCache.applications[i].perso_substate = RIL_PERSOSUBSTATE_UNKNOWN;
            m_CardStatusCache.applications[i].aid_ptr = NULL;
            m_CardStatusCache.applications[i].app_label_ptr = NULL;
            m_CardStatusCache.applications[i].pin1_replaced = 0;
            m_CardStatusCache.applications[i].pin1 = RIL_PINSTATE_UNKNOWN;
            m_CardStatusCache.applications[i].pin2 = RIL_PINSTATE_UNKNOWN;
#if defined(M2_PIN_RETRIES_FEATURE_ENABLED)
            m_CardStatusCache.applications[i].pin1_num_retries = -1;
            m_CardStatusCache.applications[i].puk1_num_retries = -1;
            m_CardStatusCache.applications[i].pin2_num_retries = -1;
            m_CardStatusCache.applications[i].puk2_num_retries = -1;
#endif // M2_PIN_RETRIES_FEATURE_ENABLE
        }
    }

    memset(&m_SimAppListData, 0, sizeof(m_SimAppListData));

    for (int i = 0; i < RIL_CARD_MAX_APPS; i++)
    {
        m_SimAppListData.aAppInfo[i].sessionId = -1;
    }

    CMutex::Unlock(m_pCardStatusUpdateLock);
}

CTEBase::~CTEBase()
{
    RIL_LOG_INFO("CTEBase::~CTEBase() - Deleting initializer\r\n");

    delete m_pUiccOpenLogicalChannelEvent;
    m_pUiccOpenLogicalChannelEvent = NULL;

    if (m_pCardStatusUpdateLock)
    {
        CMutex::Unlock(m_pCardStatusUpdateLock);
        delete m_pCardStatusUpdateLock;
        m_pCardStatusUpdateLock = NULL;
    }

    delete m_pInitializer;
    m_pInitializer = NULL;

    delete m_pDtmfStopReqEvent;
    m_pDtmfStopReqEvent = NULL;
}

BOOL CTEBase::IsRequestSupported(int requestId)
{
    switch (requestId)
    {
        case RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE:
        case RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE:
        case RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE:
        case RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE:
        case RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE:
        case RIL_REQUEST_CDMA_FLASH:
        case RIL_REQUEST_CDMA_BURST_DTMF:
        case RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY:
        case RIL_REQUEST_CDMA_SEND_SMS:
        case RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE:
        case RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG:
        case RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG:
        case RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION:
        case RIL_REQUEST_CDMA_SUBSCRIPTION:
        case RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM:
        case RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM:
        case RIL_REQUEST_DEVICE_IDENTITY:
        case RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE:
        case RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE:
            return FALSE;
        case RIL_REQUEST_RESET_RADIO:
            return FALSE;
        default:
            return TRUE;
    }
}

char* CTEBase::GetBasicInitCommands(UINT32 /*uiChannelType*/)
{
    return NULL;
}

char* CTEBase::GetUnlockInitCommands(UINT32 /*uiChannelType*/)
{
    return NULL;
}

const char* CTEBase::GetRegistrationInitString()
{
    return "+CREG=2|+CGREG=2";
}

const char* CTEBase::GetCsRegistrationReadString()
{
    return "AT+CREG=2;+CREG?;+CREG=0\r";
}

const char* CTEBase::GetPsRegistrationReadString()
{
    return "AT+CGREG=2;+CGREG?;+CGREG=0\r";
}

const char* CTEBase::GetLocationUpdateString(BOOL bIsLocationUpdateEnabled)
{
    return bIsLocationUpdateEnabled ? "AT+CREG=2\r" : "AT+CREG=1\r";
}

const char* CTEBase::GetScreenOnString()
{
    return "AT+CREG=2;+CGREG=2\r";
}

const char* CTEBase::GetScreenOffString()
{
    if (m_cte.IsLocationUpdatesEnabled())
    {
        return "AT+CGREG=1\r";
    }
    else
    {
        return "AT+CREG=1;+CGREG=1\r";
    }
}

const char* CTEBase::GetSignalStrengthReportingStringAlloc()
{
    return NULL;
}

//
// RIL_REQUEST_GET_SIM_STATUS
//
RIL_RESULT_CODE CTEBase::CoreGetSimStatus(REQUEST_DATA& rReqData,
                                                  void* /*pData*/,
                                                  UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetSimStatus() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CPIN?\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreGetSimStatus() - Exit\r\n");
    return res;
}


RIL_RESULT_CODE CTEBase::ParseSimPin(const char*& pszRsp)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimPin() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char szSimState[MAX_BUFFER_SIZE];
    int appIndex = SIM_USIM_APP_INDEX;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimPin() - Response string is NULL!\r\n");
        goto Error;
    }

    // Parse "<prefix>+CPIN: <state><postfix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimPin() - Could not skip response prefix.\r\n");
        goto Error;
    }

    if (!SkipString(pszRsp, "+CPIN: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimPin() - Could not skip \"+CPIN: \".\r\n");
        goto Error;
    }

    if (!ExtractUnquotedString(pszRsp, m_cTerminator, szSimState, MAX_BUFFER_SIZE, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimPin() - Could not extract SIM State.\r\n");
        goto Error;
    }

    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimPin() - Could not skip response postfix.\r\n");
        goto Error;
    }

    // Number of apps is 1 (gsm) if SIM present. Set to 0 if absent.
    m_CardStatusCache.num_applications = 1;

    if (0 == strcmp(szSimState, "READY"))
    {
        RIL_LOG_INFO("CTEBase::ParseSimPin() - SIM Status: RIL_SIM_READY\r\n");
        m_CardStatusCache.card_state = RIL_CARDSTATE_PRESENT;

        RIL_AppStatus& appStatus = m_CardStatusCache.applications[appIndex];

        appStatus.app_state = RIL_APPSTATE_READY;
        appStatus.perso_substate = RIL_PERSOSUBSTATE_READY;
        appStatus.aid_ptr = NULL;
        appStatus.app_label_ptr = NULL;
        appStatus.pin1_replaced = 0;
        appStatus.pin1 = RIL_PINSTATE_UNKNOWN;
        appStatus.pin2 = RIL_PINSTATE_UNKNOWN;
    }
    else if (0 == strcmp(szSimState, "SIM PIN"))
    {
        RIL_LOG_INFO("CTEBase::ParseSimPin() - SIM Status: RIL_SIM_PIN\r\n");
        m_CardStatusCache.card_state = RIL_CARDSTATE_PRESENT;

        RIL_AppStatus& appStatus = m_CardStatusCache.applications[appIndex];

        appStatus.app_state = RIL_APPSTATE_PIN;
        appStatus.perso_substate = RIL_PERSOSUBSTATE_UNKNOWN;
        appStatus.aid_ptr = NULL;
        appStatus.app_label_ptr = NULL;
        appStatus.pin1_replaced = 0;
        appStatus.pin1 = RIL_PINSTATE_ENABLED_NOT_VERIFIED;
        appStatus.pin2 = RIL_PINSTATE_UNKNOWN;
    }
    else if (0 == strcmp(szSimState, "SIM PUK"))
    {
        RIL_LOG_INFO("CTEBase::ParseSimPin() - SIM Status: RIL_SIM_PUK\r\n");
        m_CardStatusCache.card_state = RIL_CARDSTATE_PRESENT;

        RIL_AppStatus& appStatus = m_CardStatusCache.applications[appIndex];

        appStatus.app_state = RIL_APPSTATE_PUK;
        appStatus.perso_substate = RIL_PERSOSUBSTATE_UNKNOWN;
        appStatus.aid_ptr = NULL;
        appStatus.app_label_ptr = NULL;
        appStatus.pin1_replaced = 0;
        appStatus.pin1 = RIL_PINSTATE_ENABLED_BLOCKED;
        appStatus.pin2 = RIL_PINSTATE_UNKNOWN;
    }
    else if (0 == strcmp(szSimState, "PH-NET PIN"))
    {
        RIL_LOG_INFO("CTEBase::ParseSimPin() - SIM Status: RIL_SIM_NETWORK_PERSONALIZATION\r\n");
        m_CardStatusCache.card_state = RIL_CARDSTATE_PRESENT;

        RIL_AppStatus& appStatus = m_CardStatusCache.applications[appIndex];

        appStatus.app_state = RIL_APPSTATE_SUBSCRIPTION_PERSO;
        appStatus.perso_substate = RIL_PERSOSUBSTATE_SIM_NETWORK;
        appStatus.aid_ptr = NULL;
        appStatus.app_label_ptr = NULL;
        appStatus.pin1_replaced = 0;
        appStatus.pin1 = RIL_PINSTATE_ENABLED_NOT_VERIFIED;
        appStatus.pin2 = RIL_PINSTATE_UNKNOWN;
    }
    else if (0 == strcmp(szSimState, "PH-NET PUK"))
    {
        RIL_LOG_INFO("CTEBase::ParseSimPin() -"
                " SIM Status: RIL_SIM_NETWORK_PERSONALIZATION PUK\r\n");
        m_CardStatusCache.card_state = RIL_CARDSTATE_PRESENT;

        RIL_AppStatus& appStatus = m_CardStatusCache.applications[appIndex];

        appStatus.app_state = RIL_APPSTATE_SUBSCRIPTION_PERSO;
        appStatus.perso_substate = RIL_PERSOSUBSTATE_SIM_NETWORK_PUK;
        appStatus.aid_ptr = NULL;
        appStatus.app_label_ptr = NULL;
        appStatus.pin1_replaced = 0;
        appStatus.pin1 = RIL_PINSTATE_ENABLED_NOT_VERIFIED;
        appStatus.pin2 = RIL_PINSTATE_UNKNOWN;
    }
    else if (0 == strcmp(szSimState, "SIM PIN2"))
    {
        RIL_LOG_INFO("CTEBase::ParseSimPin() - SIM Status: RIL_SIM_PIN2\r\n");
        m_CardStatusCache.card_state = RIL_CARDSTATE_PRESENT;

        RIL_AppStatus& appStatus = m_CardStatusCache.applications[appIndex];

        appStatus.app_state = RIL_APPSTATE_READY;
        appStatus.perso_substate = RIL_PERSOSUBSTATE_UNKNOWN;
        appStatus.aid_ptr = NULL;
        appStatus.app_label_ptr = NULL;
        appStatus.pin1_replaced = 0;
        appStatus.pin1 = RIL_PINSTATE_UNKNOWN;
        appStatus.pin2 = RIL_PINSTATE_ENABLED_NOT_VERIFIED;
    }
    else if (0 == strcmp(szSimState, "SIM PUK2"))
    {
        RIL_LOG_INFO("CTEBase::ParseSimPin() - SIM Status: RIL_SIM_PUK2\r\n");
        m_CardStatusCache.card_state = RIL_CARDSTATE_PRESENT;

        RIL_AppStatus& appStatus = m_CardStatusCache.applications[appIndex];

        appStatus.app_state = RIL_APPSTATE_READY;
        appStatus.perso_substate = RIL_PERSOSUBSTATE_UNKNOWN;
        appStatus.aid_ptr = NULL;
        appStatus.app_label_ptr = NULL;
        appStatus.pin1_replaced = 0;
        appStatus.pin1 = RIL_PINSTATE_UNKNOWN;
        appStatus.pin2 = RIL_PINSTATE_ENABLED_BLOCKED;
    }
    else
    {
        // Anything not covered above gets treated as NO SIM
        RIL_LOG_INFO("CTEBase::ParseSimPin() - SIM Status: RIL_SIM_ABSENT\r\n");
        m_CardStatusCache.card_state = RIL_CARDSTATE_ABSENT;
        m_CardStatusCache.num_applications = 0;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::ParseSimPin() - Exit\r\n");

    return res;
}

RIL_RESULT_CODE CTEBase::ParseGetSimStatus(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetSimStatus() - Enter\r\n");

    const char* pszRsp = rRspData.szResponse;
    RIL_CardStatus_v6* pCardStatus = NULL;
    RIL_RESULT_CODE res = ParseSimPin(pszRsp);
    if (res != RRIL_RESULT_OK)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetSimStatus() - Could not parse Sim Pin.\r\n");
        goto Error;
    }

    rRspData.pData   = (void*)pCardStatus;
    rRspData.uiDataSize  = sizeof(RIL_CardStatus_v6);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pCardStatus);
        pCardStatus = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetSimStatus() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_ENTER_SIM_PIN
//
RIL_RESULT_CODE CTEBase::CoreEnterSimPin(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreEnterSimPin() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char** pszCmdData = NULL;
    char* pszPassword = NULL;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPin() - Invalid input\r\n");
        goto Error;
    }

    if (sizeof(char*) > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPin() - Request data was of unexpected size!\r\n");
        goto Error;
    }

    pszCmdData = (char**)pData;
    pszPassword = pszCmdData[0];

    if (NULL == pszPassword)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPin() - SIM PIN string was NULL!\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CPIN=\"%s\"\r", pszPassword))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPin() - Failed to write command to buffer!\r\n");
        goto Error;
    }

    //  Store PIN
    strncpy(m_szPIN, pszPassword, MAX_PIN_SIZE-1);
    m_szPIN[MAX_PIN_SIZE-1] = '\0';  //  KW fix

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreEnterSimPin() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseEnterSimPin(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseEnterSimPin() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnRetries = NULL;
    const char* pszRsp = rRspData.szResponse;

    pnRetries = (int*)malloc(sizeof(int));
    if (NULL == pnRetries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseEnterSimPin() - Could not alloc int\r\n");
        goto Error;
    }

    //  Unknown number of retries remaining
    *pnRetries = (int)-1;

    rRspData.pData    = (void*) pnRetries;
    rRspData.uiDataSize   = sizeof(int);

    //  Cache PIN1 value
    PCache_Store_PIN(m_szUICCID, m_szPIN);

    //  Clear it locally.
    memset(m_szPIN, 0, MAX_PIN_SIZE);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnRetries);
        pnRetries = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseEnterSimPin() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_ENTER_SIM_PUK
//
RIL_RESULT_CODE CTEBase::CoreEnterSimPuk(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreEnterSimPuk() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszPUK;
    char* pszNewPIN;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPuk() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if ((2 * sizeof(char*)) > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPuk() - Invalid data size.\r\n");
        goto Error;
    }

    pszPUK = ((char**) pData)[0];
    pszNewPIN = ((char**) pData)[1];

    if ((NULL == pszPUK) || (NULL == pszNewPIN))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPuk() - PUK or new PIN string was NULL!\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "ATD**05*%s*%s*%s#\r",
            pszPUK, pszNewPIN, pszNewPIN))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPuk() -"
                " Unable to write command string to buffer\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreEnterSimPuk() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseEnterSimPuk(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseEnterSimPuk() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnRetries = NULL;

    // If the response to PUK unlock is OK, then we are unlocked.
    // rril will get XSIM: <status> which will indicate whether the
    // SIM is ready or not.

    pnRetries = (int*)malloc(sizeof(int));
    if (NULL == pnRetries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseEnterSimPuk() - Could not alloc int\r\n");
        goto Error;
    }

    //  Unknown number of retries remaining
    *pnRetries = (int)-1;

    rRspData.pData    = (void*) pnRetries;
    rRspData.uiDataSize   = sizeof(int);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnRetries);
        pnRetries = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseEnterSimPuk() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_ENTER_SIM_PIN2
//
RIL_RESULT_CODE CTEBase::CoreEnterSimPin2(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreEnterSimPin2() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char** pszCmdData = NULL;
    char* pszPassword = NULL;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPin2() - Invalid input\r\n");
        goto Error;
    }

    if (sizeof(char*) > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPin2() - Request data was of unexpected size!\r\n");
        goto Error;
    }

    pszCmdData = (char**)pData;
    pszPassword = pszCmdData[0];

    if (NULL == pszPassword)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPin2() - SIM PIN string was NULL!\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CPIN2=\"%s\"\r",
            pszPassword))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPin2() - Failed to write command to buffer!\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreEnterSimPin2() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseEnterSimPin2(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseEnterSimPin2() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnRetries = NULL;

    pnRetries = (int*)malloc(sizeof(int));
    if (NULL == pnRetries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseEnterSimPin2() - Could not alloc int\r\n");
        goto Error;
    }

    //  Unknown number of retries remaining
    *pnRetries = (int)-1;

    rRspData.pData    = (void*) pnRetries;
    rRspData.uiDataSize   = sizeof(int);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnRetries);
        pnRetries = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseEnterSimPin2() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_ENTER_SIM_PUK2
//
RIL_RESULT_CODE CTEBase::CoreEnterSimPuk2(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreEnterSimPuk2() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszPUK2;
    char* pszNewPIN2;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPuk2() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if ((2 * sizeof(char*)) > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPuk2() - Invalid data size.\r\n");
        goto Error;
    }

    pszPUK2 = ((char**) pData)[0];
    pszNewPIN2 = ((char**) pData)[1];

    if ((NULL == pszPUK2) || (NULL == pszNewPIN2))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPuk2() - PUK2 or new PIN2 was NULL!\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "ATD**052*%s*%s*%s#\r",
            pszPUK2, pszNewPIN2, pszNewPIN2))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterSimPuk2() -"
                " Unable to write command string to buffer\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreEnterSimPuk2() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseEnterSimPuk2(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseEnterSimPuk2() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnRetries = NULL;

    pnRetries = (int*)malloc(sizeof(int));
    if (NULL == pnRetries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseEnterSimPuk2() - Could not alloc int\r\n");
        goto Error;
    }

    //  Unknown number of retries remaining
    *pnRetries = (int)-1;

    rRspData.pData    = (void*) pnRetries;
    rRspData.uiDataSize   = sizeof(int);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnRetries);
        pnRetries = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseEnterSimPuk2() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CHANGE_SIM_PIN
//
RIL_RESULT_CODE CTEBase::CoreChangeSimPin(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreChangeSimPin() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszOldPIN;
    char* pszNewPIN;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeSimPin() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if ((2 * sizeof(char*)) > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeSimPin() - Invalid data size.\r\n");
        goto Error;
    }

    pszOldPIN = ((char**) pData)[0];
    pszNewPIN = ((char**) pData)[1];

    if ((NULL == pszOldPIN) || (NULL == pszNewPIN))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeSimPin() - old or new PIN was NULL!\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CPWD=\"SC\",\"%s\",\"%s\"\r", pszOldPIN, pszNewPIN))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeSimPin() -"
                " Unable to write command string to buffer\r\n");
        goto Error;
    }

    //  Store PIN
    strncpy(m_szPIN, pszNewPIN, MAX_PIN_SIZE-1);
    m_szPIN[MAX_PIN_SIZE-1] = '\0';  //  KW fix

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreChangeSimPin() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseChangeSimPin(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseChangeSimPin() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    int* pnRetries = NULL;

    pnRetries = (int*)malloc(sizeof(int));
    if (NULL == pnRetries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseChangeSimPin() - Could not alloc int\r\n");
        goto Error;
    }

    //  Unknown number of retries remaining
    *pnRetries = (int)-1;

    rRspData.pData    = (void*) pnRetries;
    rRspData.uiDataSize   = sizeof(int);

    //  Cache PIN1 value
    PCache_Store_PIN(m_szUICCID, m_szPIN);

    //  Clear it locally.
    memset(m_szPIN, 0, MAX_PIN_SIZE);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnRetries);
        pnRetries = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseChangeSimPin() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CHANGE_SIM_PIN2
//
RIL_RESULT_CODE CTEBase::CoreChangeSimPin2(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreChangeSimPin2() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszOldPIN2;
    char* pszNewPIN2;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeSimPin2() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if ((2 * sizeof(char*)) > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeSimPin2() - Invalid data size.\r\n");
        goto Error;
    }

    pszOldPIN2 = ((char**) pData)[0];
    pszNewPIN2 = ((char**) pData)[1];

    if ((NULL == pszOldPIN2) || (NULL == pszNewPIN2))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeSimPin2() - old or new PIN2 was NULL!\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CPWD=\"P2\",\"%s\",\"%s\"\r", pszOldPIN2, pszNewPIN2))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeSimPin2() -"
                " Unable to write command string to buffer\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreChangeSimPin2() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseChangeSimPin2(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseChangeSimPin2() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnRetries = NULL;

    pnRetries = (int*)malloc(sizeof(int));
    if (NULL == pnRetries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseChangeSimPin2() - Could not alloc int\r\n");
        goto Error;
    }

    //  Unknown number of retries remaining
    *pnRetries = (int)-1;

    rRspData.pData    = (void*) pnRetries;
    rRspData.uiDataSize   = sizeof(int);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnRetries);
        pnRetries = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseChangeSimPin2() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION
//
RIL_RESULT_CODE CTEBase::CoreEnterNetworkDepersonalization(REQUEST_DATA& rReqData,
                                                                      void* pData,
                                                                      UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreEnterNetworkDepersonalization() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszPassword;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterNetworkDepersonalization() -"
                " Data pointer is NULL.\r\n");
        goto Error;
    }

    if (sizeof(char*) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterNetworkDepersonalization() - Invalid data size.\r\n");
        goto Error;
    }

    pszPassword = ((char**) pData)[0];

    if (NULL == pszPassword)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterNetworkDepersonalization() -"
                " Depersonalization code was NULL!\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CLCK=\"PN\",0,\"%s\"\r", pszPassword))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreEnterNetworkDepersonalization() -"
                " Unable to write command string to buffer\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreEnterNetworkDepersonalization() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseEnterNetworkDepersonalization(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseEnterNetworkDepersonalization() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnRetries = NULL;

    pnRetries = (int*)malloc(sizeof(int));
    if (NULL == pnRetries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseEnterNetworkDepersonalization() -"
                " Could not alloc int\r\n");
        goto Error;
    }

    //  Unknown number of retries remaining
    *pnRetries = (int)-1;

    rRspData.pData    = (void*) pnRetries;
    rRspData.uiDataSize   = sizeof(int);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnRetries);
        pnRetries = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseEnterNetworkDepersonalization() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_CURRENT_CALLS
//
RIL_RESULT_CODE CTEBase::CoreGetCurrentCalls(REQUEST_DATA& rReqData,
                                                        void* /*pData*/,
                                                        UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetCurrentCalls() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CLCC\r"))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreGetCurrentCalls() -"
                " Unable to write command string to buffer\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreGetCurrentCalls() - Exit\r\n");
    return res;
}

/**
This function is handling the response of RIL_REQUEST_GET_CURRENT_CALLS request.
It parses the AT string response of the cmd AT+CLCC.

@param[in, out] rRspData Contains parsed values.
@return A long value's result code.
*/
RIL_RESULT_CODE CTEBase::ParseGetCurrentCalls(RESPONSE_DATA& rRspData)
{
    RIL_RESULT_CODE res = RIL_E_GENERIC_FAILURE;
    UINT32 uiValue;
    UINT32 uinUsed = 0;
    UINT32 uinArgPtrs = 0;
    const char* pszRsp = rRspData.szResponse;
    const char* pTmpPtr = NULL;
    char szAddress[MAX_NUMBER_SIZE];
    char* aPtrArgs[AT_MAXARGS];
    P_ND_CALL_LIST_DATA pCallListData = NULL;

    if (!pszRsp)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetCurrentCalls() - String to parse is NULL\r\n");
        goto Error;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetCurrentCalls() - Parsing[%s]\r\n", pszRsp);

    /*
     * Parsing of AT response for +CLCC, list of current calls.
     * REF: 3GPP 27.007 v10.3.0
     * [+CLCC: <idx>,<dir>,<stat>,<mode>,<mpty>[,<number>,<type>[,<alpha>
     * [,<priority>[,<cli_validty>]]]]<cr><lf>[+CLCC:...]<cr><lf>OK<cr><lf>
     *
     * <idx> : integer index
     * <dir> : 0 (MO) or 1 (MT)
     * <stat> : 0..5 (active, held, dialing, alerting, incoming, waiting)
     * <mode> : 0,1,3 or 9 (Voice, Data, Voice-Data, Unknown)
     * <mpty> : 0 (not multiparty) 1 (multiparty/conference)
     * [<number>, : Double quoted string phone number,
     * <type>] : Integer identifying the format of <number>.
     * [<alpha>] : String corresponding to number in phonebook
     * The following parameters are not used by current implementation.
     * However, they are parsed by parser but skipped.
     * [<priority>] : Digit indicating the eMLPP priority level of the call
     * [<cli_validity>] : 0..4 (Id indicating why number does not contain a
     * calling party BCD number).
     */

    // Search for first +CLCC line (ended by <cr><lf> characters)
    if (FindAndSkipString(pszRsp, "+CLCC:", pszRsp))
    {
        // Allocates the response structure
        pCallListData = (P_ND_CALL_LIST_DATA)malloc(sizeof(S_ND_CALL_LIST_DATA));
        if (NULL == pCallListData)
        {
            RIL_LOG_CRITICAL("CTEBase::ParseGetCurrentCalls() - "
                    "ERROR malloc S_ND_CALL_LIST_DATA\r\n");
            goto Error;
        }
        memset(pCallListData, 0, sizeof(S_ND_CALL_LIST_DATA));
    }
    else
    {
        RIL_LOG_VERBOSE("CTEBase::ParseGetCurrentCalls() - "
                "ERROR Could not find +CLCC line\r\n");
        goto Continue;
    }

    // INFO: szRsp points to first char after "+CLCC:"
    // Loop on +CLCC lines
    do
    {
        // Retrieve an array of pointers to the different response's arguments.
        uinArgPtrs = FindRspArgs(pszRsp, m_szNewLine, aPtrArgs, AT_MAXARGS);
        RIL_LOG_VERBOSE("CTEBase::ParseGetCurrentCalls() - "
                "FOUND %d arguments\r\n", uinArgPtrs);

        // Check mandatory arguments
        if (uinArgPtrs < 5)
        {
            RIL_LOG_CRITICAL("CTEBase::ParseGetCurrentCalls() - "
                    "ERROR Not enough arguments for +CLCC response\r\n");
            goto Error;
        }

        // Parse "<idx>"
        if (!ExtractUInt32(aPtrArgs[0], uiValue, pTmpPtr))
        {
            // Error on mandatory argument
            goto Error;
        }

        pCallListData->aRilCall[uinUsed].index = uiValue;

        // Parse ",<dir>"
        if (!ExtractUpperBoundedUInt32(aPtrArgs[1], 2, uiValue, pTmpPtr))
        {
            // Error on mandatory argument
            goto Error;
        }
        pCallListData->aRilCall[uinUsed].isMT = uiValue;

        // Parse ",<stat>"
        if (!ExtractUInt32(aPtrArgs[2], uiValue, pTmpPtr))
        {
            // Error on mandatory argument
            goto Error;
        }
        pCallListData->aRilCall[uinUsed].state = (RIL_CallState)uiValue;

        // Parse ",<mode>"
        if (!ExtractUInt32(aPtrArgs[3], uiValue, pTmpPtr))
        {
            // Error on mandatory argument
            goto Error;
        }
        // If uiValue is non-zero, then we are not in a voice call
        pCallListData->aRilCall[uinUsed].isVoice = uiValue ? FALSE : TRUE;
        pCallListData->aRilCall[uinUsed].isVoicePrivacy = 0; // not used in GSM

        // Parse ",<mpty>"
        if (!ExtractUpperBoundedUInt32(aPtrArgs[4], 2, uiValue, pTmpPtr))
        {
            // Error on mandatory argument
            goto Error;
        }
        pCallListData->aRilCall[uinUsed].isMpty = uiValue;
        // END of mandatory arguments

        // set previously stored CNAP parameters to the INCOMING call
        if (pCallListData->aRilCall[uinUsed].state == 4)
        {
            if (strlen(m_cte.GetCnapName()) != 0)
            {
                CopyStringNullTerminate(pCallListData->aszName[uinUsed],
                        m_cte.GetCnapName(), MAX_CNAP_NAME_SIZE);
                // set RIL_Call.name pointer to our name buffer
                pCallListData->aRilCall[uinUsed].name = pCallListData->aszName[uinUsed];
            }

            pCallListData->aRilCall[uinUsed].namePresentation = m_cte.GetCnapCniValidity();
        }

        if (uinArgPtrs >= 7)
        {
            // <number> and <type>
            if (ExtractQuotedString(aPtrArgs[5], szAddress, MAX_NUMBER_SIZE, pTmpPtr))
            {
                pCallListData->aRilCall[uinUsed].number =
                        pCallListData->aszNumber[uinUsed];
                strncpy(pCallListData->aszNumber[uinUsed], szAddress, MAX_NUMBER_SIZE);

                // If name is restricted, then set the number to restricted as well
                if (pCallListData->aRilCall[uinUsed].namePresentation == 1)
                {
                    pCallListData->aRilCall[uinUsed].numberPresentation = 1;
                }
                // Get the CLI_validity from +CLIP for the incomming call
                else if (pCallListData->aRilCall[uinUsed].state == 4
                        || strcmp(szAddress, m_cte.GetNumber()) == 0)
                {
                    pCallListData->aRilCall[uinUsed].numberPresentation
                            = m_cte.GetNumberCliValidity();
                }

                // Parse ",<type>"
                if (ExtractUpperBoundedUInt32(aPtrArgs[6], 0x100, uiValue, pTmpPtr))
                {
                    pCallListData->aRilCall[uinUsed].toa = uiValue;
                }
                else
                {
                    RIL_LOG_VERBOSE("\t<type>=No value found\r\n");
                }
            }
            else
            {
                RIL_LOG_VERBOSE("\t<Number><type>=No value found\r\n");
            }
        }
        if (uinArgPtrs >= 8)
        {
            // <alpha>
            // Parameter not used in current implementation
            char szDummy[MAX_BUFFER_SIZE];
            if (!ExtractQuotedString(aPtrArgs[7], szDummy, MAX_BUFFER_SIZE, pTmpPtr))
            {
                RIL_LOG_VERBOSE("\t<Description>=No value found\r\n");
            }
        }
        if (uinArgPtrs >= 9)
        {
            // <priority>
            // Parameter not used in current implementation
            if (!ExtractUInt32(aPtrArgs[8], uiValue, pTmpPtr))
            {
                RIL_LOG_VERBOSE("\t<Priority>=No value found\r\n");
            }
        }
        if (uinArgPtrs >= 10)
        {
            // <cli_validity>
            if (ExtractUInt32(aPtrArgs[9], uiValue, pTmpPtr))
            {
                switch (uiValue)
                {
                    case 0:  // cli valid
                        pCallListData->aRilCall[uinUsed].numberPresentation = 0;
                        break;

                    case 1:  // restricted
                        pCallListData->aRilCall[uinUsed].numberPresentation = 1;
                        if (pCallListData->aRilCall[uinUsed].namePresentation != 1)
                        {
                            RIL_LOG_INFO("\t<cli_validity> Overwrite the namePresentation"
                                    " value \r\n");
                            pCallListData->aRilCall[uinUsed].namePresentation = 1;
                        }
                        break;

                    case 2:  // not available code "Interaction with other service"
                    case 4:  // not available code "Unavailable"
                        pCallListData->aRilCall[uinUsed].numberPresentation = 2;
                        break;

                    case 3:  // not available payphone
                        pCallListData->aRilCall[uinUsed].numberPresentation = 3;
                        break;
                    default:  // error
                        RIL_LOG_INFO("\t<cli_validity>=Error:%d\r\n", uiValue);
                        break;
                }
            }
            else
            {
                RIL_LOG_VERBOSE("\t<cli_validity>=No value found\r\n");
            }
        }

        // Indicates the call 'line' used (Default: 0 = line 1)
        pCallListData->aRilCall[uinUsed].als = 0;
        pCallListData->apRilCall[uinUsed] = &(pCallListData->aRilCall[uinUsed]);

        // Increment the array index
        uinUsed++;
    } while ((uinUsed < RRIL_MAX_CALL_ID_COUNT)
             && (FindAndSkipString(pszRsp, "+CLCC:", pszRsp)));

    if (uinUsed >= RRIL_MAX_CALL_ID_COUNT)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetCurrentCalls() - "
                "Too many ongoing calls:%d\r\n", uinUsed);
        goto Error;
    }

Continue:
    if (pCallListData)
    {
        if (uinUsed > 0)
        {
            rRspData.pData  = (void*)pCallListData;
            rRspData.uiDataSize = uinUsed * sizeof(RIL_Call*);
        }
        else
        {
            RIL_LOG_CRITICAL("CTEBase::ParseGetCurrentCalls() - "
                    "No argument in +CLCC response\r\n");
            goto Error;
        }
    }
    else
    {
        RIL_LOG_VERBOSE("CTEBase::ParseGetCurrentCalls() - No calls.\r\n");
        rRspData.pData  = NULL;
        rRspData.uiDataSize = 0;
    }

    memset(&m_VoiceCallInfo, -1, sizeof(m_VoiceCallInfo));

    if (pCallListData != NULL)
    {
        UINT32 uiCallState;
        for (UINT32 i = 0; i < uinUsed; i++)
        {
            m_VoiceCallInfo[i].id = pCallListData->aRilCall[i].index;
            m_VoiceCallInfo[i].state = pCallListData->aRilCall[i].state;

            uiCallState = pCallListData->aRilCall[i].state;
            if (uiCallState == E_CALL_STATUS_DIALING
                    || uiCallState == E_CALL_STATUS_ALERTING
                    || uiCallState == E_CALL_STATUS_ACTIVE
                    || uiCallState == E_CALL_STATUS_CONNECTED)
            {
                m_VoiceCallInfo[i].bDtmfAllowed = TRUE;
            }
            else
            {
                m_VoiceCallInfo[i].bDtmfAllowed = FALSE;
            }
            RIL_LOG_VERBOSE("CTEBase::ParseGetCurrentCalls() - "
                    "Call[%d]: State:%d\r\n", m_VoiceCallInfo[i].id, uiCallState);
        }
    }

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetCurrentCalls() - "
                "Error parsing +CLCC response\r\n");
        free(pCallListData);
        pCallListData = NULL;
        rRspData.pData  = NULL;
        rRspData.uiDataSize = 0;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetCurrentCalls() - "
            "Exit with result:%d\r\n", res);
    return res;
}

//
// RIL_REQUEST_DIAL
//
RIL_RESULT_CODE CTEBase::CoreDial(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreDial() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_Dial* pRilDial   = NULL;
    const char* clir;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDial() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if (sizeof(RIL_Dial) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDial() - Invalid data size=%d.\r\n, uiDataSize");
        goto Error;
    }

    pRilDial = (RIL_Dial*)pData;

    switch (pRilDial->clir)
    {
        case 1:  // invocation
            clir = "I";
            break;

        case 2:  // suppression
            clir = "i";
            break;

        default:  // subscription default
            clir = "";
            break;
    }

    if (PrintStringNullTerminate(rReqData.szCmd1,  sizeof(rReqData.szCmd1), "ATD%s%s;\r",
                pRilDial->address, clir))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreDial() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseDial(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseDial() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseDial() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_IMSI
//
RIL_RESULT_CODE CTEBase::CoreGetImsi(REQUEST_DATA& rReqData,
                                             void* /*pData*/,
                                             UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetImsi() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CIMI\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreGetImsi() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGetImsi(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetImsi() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    char* szSerialNumber = NULL;

    if (NULL == pszRsp)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImsi() - Response string is NULL!\r\n");
        goto Error;
    }

    szSerialNumber = (char*)malloc(PROPERTY_VALUE_MAX);
    if (NULL == szSerialNumber)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImsi() -"
                " Could not allocate memory for a %u-char string.\r\n", MAX_BUFFER_SIZE);
        goto Error;
    }
    memset(szSerialNumber, 0x00, PROPERTY_VALUE_MAX);

    // Parse "<prefix><serial_number><postfix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp) ||
        !ExtractUnquotedString(pszRsp, m_cTerminator, szSerialNumber, PROPERTY_VALUE_MAX, pszRsp) ||
        !SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImsi() - Could not extract the IMSI string.\r\n");
        goto Error;
    }

    rRspData.pData   = (void*)szSerialNumber;
    rRspData.uiDataSize  = sizeof(char*);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(szSerialNumber);
        szSerialNumber = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetImsi() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_HANGUP
//
RIL_RESULT_CODE CTEBase::CoreHangup(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreHangup() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnLine = NULL;

    if (sizeof(int) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreHangup() - Passed data size mismatch. Found %d bytes\r\n",
                uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreHangup() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pnLine = (int*)pData;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CHLD=1%u\r", pnLine[0]))
    {
        res = RRIL_RESULT_OK;
    }

Error:

    RIL_LOG_VERBOSE("CTEBase::CoreHangup() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseHangup(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseHangup() - Enter\r\n");

    if (m_cte.GetCallDropReportingState())
    {
        UINT32 mobileRelease = 1;
        triggerDropCallEvent((void*)(intptr_t)mobileRelease);
    }

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseHangup() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND
//
RIL_RESULT_CODE CTEBase::CoreHangupWaitingOrBackground(REQUEST_DATA& rReqData,
                                                                  void* /*pData*/,
                                                                  UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreHangupWaitingOrBackground() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    /*
     * Fix to end the call which is answered from the UI perspective but connection not established
     * due to missing network acknowledgement.
     */
    if (m_IncomingCallInfo.callId && m_IncomingCallInfo.isAnswerReqSent
            && m_IncomingCallInfo.status == E_CALL_STATUS_INCOMING)
    {
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CHLD=1%u\r",
                m_IncomingCallInfo.callId))
        {
            res = RRIL_RESULT_OK;
        }
    }
    else
    {
        if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CHLD=0\r", sizeof(rReqData.szCmd1)))
        {
            res = RRIL_RESULT_OK;
        }
    }

    RIL_LOG_VERBOSE("CTEBase::CoreHangupWaitingOrBackground() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseHangupWaitingOrBackground(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseHangupWaitingOrBackground() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseHangupWaitingOrBackground() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND
//
RIL_RESULT_CODE CTEBase::CoreHangupForegroundResumeBackground(REQUEST_DATA& rReqData,
                                                                         void* /*pData*/,
                                                                         UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreHangupForegroundResumeBackground() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CHLD=1\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreHangupForegroundResumeBackground() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseHangupForegroundResumeBackground(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseHangupForegroundResumeBackground() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseHangupForegroundResumeBackground() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE
// RIL_REQUEST_SWITCH_HOLDING_AND_ACTIVE
//
RIL_RESULT_CODE CTEBase::CoreSwitchHoldingAndActive(REQUEST_DATA& rReqData,
                                                               void* /*pData*/,
                                                               UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSwitchHoldingAndActive() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    bool isWaitingCall = false;
    bool isActiveCall = false;
    bool isHeldCall = false;

    if (E_DTMF_STATE_START == m_cte.GetDtmfState())
    {
        CEvent::Reset(m_pDtmfStopReqEvent);
        HandleInternalDtmfStopReq();
        CEvent::Wait(m_pDtmfStopReqEvent, WAIT_TIMEOUT_DTMF_STOP);
    }

    // Framework sends this request, when user accepts a waiting call when there
    // is an active and held call.
    for (UINT32 i = 0; i < RRIL_MAX_CALL_ID_COUNT; i++)
    {
        switch (m_VoiceCallInfo[i].state)
        {
            case E_CALL_STATUS_WAITING:
                isWaitingCall = true;
                break;

            case E_CALL_STATUS_HELD:
                isHeldCall = true;
                break;

            case E_CALL_STATUS_ACTIVE:
                isActiveCall = true;
                break;

            default:
                break;
        }
    }

    if (CopyStringNullTerminate(rReqData.szCmd1,
            isWaitingCall && isHeldCall && isActiveCall ? "AT+CHLD=1\r" : "AT+CHLD=2\r",
            sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreSwitchHoldingAndActive() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSwitchHoldingAndActive(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSwitchHoldingAndActive() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseSwitchHoldingAndActive() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CONFERENCE
//
RIL_RESULT_CODE CTEBase::CoreConference(REQUEST_DATA& rReqData,
                                                void* /*pData*/,
                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreConference() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (E_DTMF_STATE_START == m_cte.GetDtmfState())
    {
        CEvent::Reset(m_pDtmfStopReqEvent);
        HandleInternalDtmfStopReq();
        CEvent::Wait(m_pDtmfStopReqEvent, WAIT_TIMEOUT_DTMF_STOP);
    }

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CHLD=3\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreConference() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseConference(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseConference() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseConference() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_UDUB
//
RIL_RESULT_CODE CTEBase::CoreUdub(REQUEST_DATA& rReqData,
                                          void* /*pData*/,
                                          UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreUdub() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    /*
     * This is ugly fix to provide a way to end the call which is answered from the UI
     * perspective but connection not establised due to the missing network conformance.
     */
    if (m_IncomingCallInfo.callId && m_IncomingCallInfo.isAnswerReqSent &&
                                        m_IncomingCallInfo.status == E_CALL_STATUS_INCOMING)
    {
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CHLD=1%u\r",
                                                                    m_IncomingCallInfo.callId))
        {
            res = RRIL_RESULT_OK;
        }
    }
    else
    {
        if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CHLD=0\r", sizeof(rReqData.szCmd1)))
        {
            res = RRIL_RESULT_OK;
        }
    }

    RIL_LOG_VERBOSE("CTEBase::CoreUdub() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseUdub(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseUdub() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseUdub() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_LAST_CALL_FAIL_CAUSE
//
RIL_RESULT_CODE CTEBase::CoreLastCallFailCause(REQUEST_DATA& rReqData,
                                                          void* /*pData*/,
                                                          UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreLastCallFailCause() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CEER\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreLastCallFailCause() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseLastCallFailCause(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseLastCallFailCause() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32      uiCause  = 0;
    int*      pCause   = NULL;

    const char* pszEnd = NULL;
    const char* pszStart = NULL;
    char szBackup[255] = {0};

    // Backup the +CEER response string to report data on crashtool
    pszStart = rRspData.szResponse;
    SkipRspStart(pszStart, m_szNewLine, pszStart);
    ExtractUnquotedString(pszStart, m_szNewLine, szBackup, 255, pszEnd);
    m_cte.SaveCEER(szBackup);

    if (!ParseCEER(rRspData, uiCause))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseLastCallFailCause() - Parsing of CEER failed\r\n");
        goto Error;
    }

    pCause= (int*) malloc(sizeof(int));
    if (NULL == pCause)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseLastCallFailCause() -"
                " Could not allocate memory for an integer.\r\n");
        goto Error;
    }

    //  Some error cases here are different.
    if (279 == uiCause)
        uiCause = CALL_FAIL_FDN_BLOCKED;
    else if ( (280 == uiCause) || (8 == uiCause) )
        uiCause = CALL_FAIL_CALL_BARRED;
    else if ( (500 == uiCause) || (510 == uiCause) || (511 == uiCause) )
        uiCause = CALL_FAIL_NORMAL;

    //@TODO: cause code mapping needs to be revisited
    switch(uiCause)
    {
        case CALL_FAIL_NORMAL:
        case CALL_FAIL_BUSY:
        case CALL_FAIL_CONGESTION:
        case CALL_FAIL_ACM_LIMIT_EXCEEDED:
        case CALL_FAIL_CALL_BARRED:
        case CALL_FAIL_FDN_BLOCKED:
        case CALL_FAIL_IMSI_UNKNOWN_IN_VLR:
        case CALL_FAIL_IMEI_NOT_ACCEPTED:
            *pCause = (int) uiCause;
            break;

        default:
            *pCause = (int) CALL_FAIL_ERROR_UNSPECIFIED;
            break;
    }

    if (m_cte.GetCallDropReportingState())
    {
        UINT32 mobileRelease = 0;
        triggerDropCallEvent((void*)(intptr_t)mobileRelease);
    }

    rRspData.pData    = (void*) pCause;
    rRspData.uiDataSize   = sizeof(int);
    RIL_LOG_INFO("CTEBase::ParseLastCallFailCause() - Last call fail cause [%d]\r\n", uiCause);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pCause);
        pCause = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseLastCallFailCause() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIGNAL_STRENGTH
//
RIL_RESULT_CODE CTEBase::CoreSignalStrength(REQUEST_DATA& rReqData,
                                                       void* /*pData*/,
                                                       UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSignalStrength() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CSQ\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreSignalStrength() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSignalStrength(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSignalStrength() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    RIL_SignalStrength_v6* pSigStrData = ParseQuerySignalStrength(rRspData);
    if (NULL == pSigStrData)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSignalStrength() -"
                " Could not allocate memory for RIL_SignalStrength_v6.\r\n");
        goto Error;
    }


    rRspData.pData   = (void*)pSigStrData;
    rRspData.uiDataSize  = sizeof(RIL_SignalStrength_v6);

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::ParseSignalStrength() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_VOICE_REGISTRATION_STATE
//
RIL_RESULT_CODE CTEBase::CoreRegistrationState(REQUEST_DATA& rReqData,
                                                          void* /*pData*/,
                                                          UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreRegistrationState() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, GetCsRegistrationReadString(),
            sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreRegistrationState() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseRegistrationState(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseRegistrationState() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    S_ND_REG_STATUS regStatus;
    P_ND_REG_STATUS pRegStatus = NULL;

    pRegStatus = (P_ND_REG_STATUS)malloc(sizeof(S_ND_REG_STATUS));
    if (NULL == pRegStatus)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseRegistrationState() -"
                " Could not allocate memory for a S_ND_REG_STATUS struct.\r\n");
        goto Error;
    }
    memset(pRegStatus, 0, sizeof(S_ND_REG_STATUS));

    if (!m_cte.ParseCREG(pszRsp, FALSE, regStatus))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseRegistrationState() - ERROR in parsing response.\r\n");
        goto Error;
    }

    // As CREG: 2 is an intermediate state, don't store the queried registration state in
    // cache.
    if (strcmp(regStatus.szStat, UNREGISTERED_SEARCHING))
    {
        m_cte.StoreRegistrationInfo(&regStatus, E_REGISTRATION_TYPE_CREG);
    }

    m_cte.CopyCachedRegistrationInfo(pRegStatus, FALSE);

    // We cheat with the size here.
    // Although we have allocated a S_ND_REG_STATUS struct, we tell
    // Android that we have only allocated a S_ND_REG_STATUS_POINTERS
    // struct since Android is expecting to receive an array of string pointers.
    // Note that we only tell Android about the 14 pointers it supports for CREG notifications.

    rRspData.pData  = (void*)pRegStatus;
    rRspData.uiDataSize = sizeof(S_ND_REG_STATUS_POINTERS);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pRegStatus);
        pRegStatus = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseRegistrationState() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_DATA_REGISTRATION_STATE
//
RIL_RESULT_CODE CTEBase::CoreGPRSRegistrationState(REQUEST_DATA& rReqData,
                                                              void* /*pData*/,
                                                              UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGPRSRegistrationState() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, GetPsRegistrationReadString(),
            sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreGPRSRegistrationState() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGPRSRegistrationState(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGPRSRegistrationState() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    S_ND_GPRS_REG_STATUS psRegStatus;
    P_ND_GPRS_REG_STATUS pGPRSRegStatus = NULL;

    pGPRSRegStatus = (P_ND_GPRS_REG_STATUS)malloc(sizeof(S_ND_GPRS_REG_STATUS));
    if (NULL == pGPRSRegStatus)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGPRSRegistrationState() -"
                " Could not allocate memory for a S_ND_GPRS_REG_STATUS struct.\r\n");
        goto Error;
    }
    memset(pGPRSRegStatus, 0, sizeof(S_ND_GPRS_REG_STATUS));

    if (!m_cte.ParseCGREG(pszRsp, FALSE, psRegStatus))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGPRSRegistrationState() - ERROR in parsing response.\r\n");
        goto Error;
    }

    m_cte.StoreRegistrationInfo(&psRegStatus, E_REGISTRATION_TYPE_CGREG);
    m_cte.CopyCachedRegistrationInfo(pGPRSRegStatus, TRUE);

    // We cheat with the size here.
    // Although we have allocated a S_ND_GPRS_REG_STATUS struct, we tell
    // Android that we have only allocated a S_ND_GPRS_REG_STATUS_POINTERS
    // struct since Android is expecting to receive an array of string pointers.

    rRspData.pData  = (void*)pGPRSRegStatus;
    rRspData.uiDataSize = sizeof(S_ND_GPRS_REG_STATUS_POINTERS);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pGPRSRegStatus);
        pGPRSRegStatus = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGPRSRegistrationState() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_OPERATOR
//
RIL_RESULT_CODE CTEBase::CoreOperator(REQUEST_DATA& rReqData,
                                              void* /*pData*/,
                                              UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreOperator() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1,
            "AT+XCOPS=12;+XCOPS=11;+XCOPS=13;+XCOPS=9;+XCOPS=8\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreOperator() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseOperator(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    UINT32 uiType = 0;
    const int NAME_SIZE = 50;
    char szEONSFullName[NAME_SIZE] = {0};
    char szEONSShortName[NAME_SIZE] = {0};
    char szFullName[NAME_SIZE] = {0};
    char szShortName[NAME_SIZE] = {0};
    BOOL isNitzNameAvailable = FALSE;
    char szPlmnName[NAME_SIZE] = {0};
    BOOL isEONSAvailable = FALSE;
    P_ND_OP_NAMES pOpNames = NULL;

    pOpNames = (P_ND_OP_NAMES)malloc(sizeof(S_ND_OP_NAMES));
    if (NULL == pOpNames)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseOperator() -"
                " Could not allocate memory for S_ND_OP_NAMES struct.\r\n");
        goto Error;
    }
    memset(pOpNames, 0, sizeof(S_ND_OP_NAMES));

    /*
     * XCOPS follows a fall back mechanism if a requested type is not available.
     * For requested type 9, fallback types are 6 or 4 or 2 or 0
     * For requested type 8, fallback types are 5 or 3 or 1 or 0
     * When registered, requested type 11,12 and 13 will always return the
     * currently registered PLMN information
     * Other details can be found in the C-AT specifications.
     *
     * Fallback's are ignored as framework already has this information.
     */

    // Parse "<prefix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseOperator() - Could not skip response prefix.\r\n");
        goto Error;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - Response: %s\r\n",
            CRLFExpandedString(pszRsp, strlen(pszRsp)).GetString());

    // Skip "+XCOPS: "
    while (SkipString(pszRsp, "+XCOPS: ", pszRsp))
    {
        // Extract "<Type>"
        if (!ExtractUInt32(pszRsp, uiType, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseOperator() - Could not extract <mode>.\r\n");
            goto Error;
        }

        // Extract ","
        if (SkipString(pszRsp, ",", pszRsp))
        {
            // Based on type get the long/short/numeric network name
            switch (uiType)
            {
                // EONS long operator name from EF-PNN
                case 9:
                {
                    if (!ExtractQuotedString(pszRsp, szEONSFullName, NAME_SIZE, pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTEBase::ParseOperator() -"
                                " ERROR: Could not extract the Long Format Operator Name.\r\n");
                        goto Error;
                    }
                    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - EONS Long name: %s\r\n",
                            szEONSFullName);
                    isEONSAvailable = TRUE;
                }
                break;

                // EONS Short name from EF-PNN
                case 8:
                {
                    if (!ExtractQuotedString(pszRsp, szEONSShortName, NAME_SIZE, pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTEBase::ParseOperator() -"
                                " ERROR: Could not extract the Short Format Operator Name.\r\n");
                        goto Error;
                    }
                    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - EONS Short name: %s\r\n",
                            szEONSShortName);
                    isEONSAvailable = TRUE;
                }
                break;

                // Long name based on NITZ
                case 6:
                {
                    if (!ExtractQuotedString(pszRsp, szFullName, NAME_SIZE, pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTEBase::ParseOperator() -"
                                " Could not extract the Long Format Operator Name.\r\n");
                        goto Error;
                    }
                    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - NITZ Long name: %s\r\n",
                            szFullName);
                    isNitzNameAvailable = TRUE;
                }
                break;

                // Short name based on NITZ
                case 5:
                {
                    if (!ExtractQuotedString(pszRsp, szShortName, NAME_SIZE, pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTEBase::ParseOperator() -"
                                " Could not extract the Short Format Operator Name.\r\n");
                        goto Error;
                    }
                    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - NITZ Short name: %s\r\n",
                            szShortName);
                    isNitzNameAvailable = TRUE;
                }
                break;

                // Long PLMN name(When PS or CS is registered)
                case 12:
                {
                    if (!ExtractQuotedString(pszRsp, szPlmnName, sizeof(szPlmnName), pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTEBase::ParseOperator() -"
                                " Could not extract the Long Format Operator Name.\r\n");
                        goto Error;
                    }
                    CopyStringNullTerminate(pOpNames->szOpNameLong, szPlmnName, MAX_OP_NAME_LONG);
                    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - PLMN Long Name: %s\r\n",
                            pOpNames->szOpNameLong);
                }
                break;

                // Short PLMN name(When PS or CS is registered)
                case 11:
                {
                    if (!ExtractQuotedString(pszRsp, szPlmnName, sizeof(szPlmnName), pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTEBase::ParseOperator() -"
                                " Could not extract the Short Format Operator Name.\r\n");
                        goto Error;
                    }
                    CopyStringNullTerminate(pOpNames->szOpNameShort, szPlmnName, MAX_OP_NAME_SHORT);
                    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - PLMN Short Name: %s\r\n",
                            pOpNames->szOpNameShort);
                }
                break;

                // numeric format of network MCC/MNC even in limited service
                case 13:
                {
                    if (!ExtractQuotedString(pszRsp, szPlmnName, sizeof(szPlmnName), pszRsp))
                    {
                        RIL_LOG_CRITICAL("CTEBase::ParseOperator() -"
                                " Could not extract the Long Format Operator Name.\r\n");
                        goto Error;
                    }
                    CopyStringNullTerminate(pOpNames->szOpNameNumeric, szPlmnName, MAX_OP_NAME_NUM);
                    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - PLMN Numeric code: %s\r\n",
                            pOpNames->szOpNameNumeric);
                }
                break;

                default:
                {
                    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - Format not handled.\r\n");
                    break;
                }
            }
        }
        else
        {
            RIL_LOG_VERBOSE("CTEBase::ParseOperator() - <network name> not present.\r\n");
            pOpNames->sOpNamePtrs.pszOpNameLong    = NULL;
            pOpNames->sOpNamePtrs.pszOpNameShort   = NULL;
            pOpNames->sOpNamePtrs.pszOpNameNumeric = NULL;
        }

        // Extract "<CR><LF>"
        if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseOperator() - Could not extract response postfix.\r\n");
            goto Error;
        }

        // If we have another line to parse, get rid of its prefix now.
        // Note that this will do nothing if we don't have another line to parse.
        SkipRspStart(pszRsp, m_szNewLine, pszRsp);

        RIL_LOG_VERBOSE("CTEBase::ParseOperator() - Response: %s\r\n",
                CRLFExpandedString(pszRsp, strlen(pszRsp)).GetString());
    }

    if (isEONSAvailable || isNitzNameAvailable)
    {
        memset(pOpNames->szOpNameLong, 0, sizeof(pOpNames->szOpNameLong));
        memset(pOpNames->szOpNameShort, 0, sizeof(pOpNames->szOpNameShort));
    }

    if (isEONSAvailable)
    {
        CopyStringNullTerminate(pOpNames->szOpNameLong, szEONSFullName, MAX_OP_NAME_LONG);
        CopyStringNullTerminate(pOpNames->szOpNameShort, szEONSShortName, MAX_OP_NAME_SHORT);
    }
    else if (isNitzNameAvailable)
    {
        CopyStringNullTerminate(pOpNames->szOpNameLong, szFullName, MAX_OP_NAME_LONG);
        CopyStringNullTerminate(pOpNames->szOpNameShort, szShortName, MAX_OP_NAME_SHORT);
    }

    pOpNames->sOpNamePtrs.pszOpNameLong    = pOpNames->szOpNameLong;
    pOpNames->sOpNamePtrs.pszOpNameShort   = pOpNames->szOpNameShort;
    pOpNames->sOpNamePtrs.pszOpNameNumeric = pOpNames->szOpNameNumeric;

    m_cte.SaveNetworkData(LAST_NETWORK_OP_NAME_NUMERIC, pOpNames->szOpNameNumeric);
    m_cte.SaveNetworkData(LAST_NETWORK_OP_NAME_SHORT, pOpNames->szOpNameShort);

    RIL_LOG_VERBOSE("CTEBase::ParseOperator() -"
            " Long Name: \"%s\", Short Name: \"%s\", Numeric Name: \"%s\"\r\n",
            pOpNames->sOpNamePtrs.pszOpNameLong, pOpNames->sOpNamePtrs.pszOpNameShort,
            pOpNames->sOpNamePtrs.pszOpNameNumeric);


    // We cheat with the size here.
    // Although we have allocated a S_ND_OP_NAMES struct, we tell
    // Android that we have only allocated a S_ND_OP_NAME_POINTERS
    // struct since Android is expecting to receive an array of string pointers.

    rRspData.pData  = pOpNames;
    rRspData.uiDataSize = sizeof(S_ND_OP_NAME_POINTERS);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pOpNames);
        pOpNames = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseOperator() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_RADIO_POWER
//
RIL_RESULT_CODE CTEBase::CoreRadioPower(REQUEST_DATA& /*rReqData*/,
                                                void* pData,
                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreRadioPower() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    CCommand* pCmd = NULL;
    char szCmd[MAX_BUFFER_SIZE] = {'\0'};
    int radioOffReason = m_cte.GetRadioOffReason();
    BOOL bTurnRadioOn = (0 == ((int*)pData)[0]) ? FALSE : TRUE;

    CEvent* pRadioStateChangedEvent = m_cte.GetRadioStateChangedEvent();

    if (bTurnRadioOn)
    {
        switch (m_cte.GetLastModemEvent())
        {
            case E_MMGR_NOTIFY_CORE_DUMP:
            case E_MMGR_EVENT_MODEM_OUT_OF_SERVICE:
            case E_MMGR_NOTIFY_PLATFORM_REBOOT:
                /*
                 * Don't acquire the resource as it may fail due to MMGR busy
                 * core dump or platform reboot.
                 */
                res = RRIL_RESULT_ERROR;
                break;

            default:
                // Modem might be already powered on when rapid ril is started.
                // Acquiring of modem resource here is redundant.
                if (!CSystemManager::GetInstance().GetModem())
                {
                    RIL_LOG_CRITICAL("CTEBase::CoreRadioPower() - "
                            "GetModem Resource failed\r\n");

                    m_cte.SetRestrictedMode(TRUE);
                    res = RRIL_RESULT_ERROR;
                }
                else
                {
                    m_cte.SetRestrictedMode(FALSE);
                }
                break;
        }

        if (RRIL_RESULT_ERROR == res)
        {
            RIL_LOG_INFO("CTEBase::CoreRadioPower - Issue seen in RADIO_POWER ON\r\n");
            goto Error;
        }
    }
    else // Turn radio off
    {
        switch (m_cte.GetLastModemEvent())
        {
            case E_MMGR_EVENT_MODEM_UP:
                if (E_RADIO_OFF_REASON_SHUTDOWN == radioOffReason)
                {
                    if (RADIO_STATE_UNAVAILABLE == GetRadioState())
                    {
                        if (CSystemManager::GetInstance().SendRequestModemShutdown())
                        {
                            WaitForModemPowerOffEvent();
                        }
                        res = RRIL_RESULT_ERROR;
                    }
                }
                // Do nothing. Actions will be taken on radio state changed event
                break;

            case E_MMGR_EVENT_MODEM_DOWN:
                if (E_RADIO_OFF_REASON_SHUTDOWN == radioOffReason)
                {
                    CSystemManager::GetInstance().CloseChannelPorts();
                }

                /*
                 * If the radio power off reason is none or airplane mode, then
                 * it is better to set the radio state to off and wait for
                 * RADIO_POWER ON request.
                 */
                SetRadioStateAndNotify(RRIL_RADIO_STATE_OFF);
                res = RRIL_RESULT_ERROR;
                RIL_LOG_INFO("CTEBase::CoreRadioPower - Already in expected state\r\n");
                break;

            default:
                if (E_RADIO_OFF_REASON_SHUTDOWN == radioOffReason)
                {
                    CSystemManager::GetInstance().CloseChannelPorts();
                    SetRadioStateAndNotify(RRIL_RADIO_STATE_OFF);
                }
                else if (E_RADIO_OFF_REASON_AIRPLANE_MODE == radioOffReason)
                {
                    /*
                     * Note that RRIL still needs to release the radio resource to prevent having
                     * the modem restart uselessly if RRIL is the sole client that required it.
                     */
                    if (m_cte.GetModemOffInFlightModeState())
                    {
                        CSystemManager::GetInstance().ReleaseModem();
                    }
                }

                res = RRIL_RESULT_ERROR;
                RIL_LOG_INFO("CTEBase::CoreRadioPower - "
                        "handling RADIO_POWER OFF in modem state %d\r\n",
                        m_cte.GetLastModemEvent());
                break;
        }

        if (RRIL_RESULT_ERROR == res)
        {
            res = RRIL_RESULT_OK;
            goto Error;
        }
    }

    if (!GetRadioPowerCommand(bTurnRadioOn, radioOffReason, szCmd, sizeof(szCmd)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreRadioPower() - GetRadioPowerCommand failed\r\n");
        goto Error;
    }

    if (!CSystemManager::GetInstance().IsInitializationSuccessful())
    {
        /*
         * This timeout is based on test results. Timeout is the sum of
         * time taken for powering up the modem(~6seconds) + opening of ports(<1second)
         * + modem basic initialization(1second). In case of flashless modem, powering
         * up the modem sometimes takes >10seconds. To make sure that the RADIO_POWER
         * request gets processed successfully, increase the waiting timer to 20seconds.
         */
        UINT32 WAIT_TIMEOUT_IN_MS = 20000;
        CEvent* pModemBasicInitCompleteEvent =
                    CSystemManager::GetInstance().GetModemBasicInitCompleteEvent();

        RIL_LOG_INFO("CTEBase::CoreRadioPower() - Waiting for "
                "modem initialization completion event\r\n");

        CEvent::Reset(pModemBasicInitCompleteEvent);

        if (WAIT_EVENT_0_SIGNALED !=
                CEvent::Wait(pModemBasicInitCompleteEvent, WAIT_TIMEOUT_IN_MS))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreRadioPower() - Timeout Waiting for"
                    "modem initialization completion event\r\n");

            res = RRIL_RESULT_ERROR;
            goto Error;
        }
    }

    /*
     * The RadioStateChangedEvent is triggered in the PostRadioPower function.
     * To avoid to be blocked in the wait of this event, we need to reset it before the sending
     * of the AT and not just before to perform the wait
     */
    if (NULL != pRadioStateChangedEvent)
    {
        CEvent::Reset(pRadioStateChangedEvent);
    }

    /*
     * Note: RIL_Token is not provided as part of the command creation.
     * If the RIL_REQUEST_RADIO_POWER is for platform shutdown, then the
     * main thread(request handling thread) waits for ModemPoweredOffEvent.
     * If the request is for radio state change(on/off), then the main
     * thread waits for RadioStateChangedEvent. This events are triggered
     * on MODEM_DOWN or from PostRadioPower(on response of CFUN commands).
     * On this events, main thread will be unblocked and RIL_REQUEST_RADIO_POWER
     * request is completed in RequestRadioPower function in te.cpp with the valid
     * RIL_Token.
     */
    pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_RADIO_POWER].uiChannel,
            NULL, RIL_REQUEST_RADIO_POWER, szCmd, &CTE::ParseRadioPower, &CTE::PostRadioPower);

    if (pCmd)
    {
        pCmd->SetHighPriority();

        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreRadioPower() -"
                    " Unable to add command to queue\r\n");
            res = RRIL_RESULT_ERROR;
            delete pCmd;
            pCmd = NULL;
            goto Error;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::CoreRadioPower() -"
                " Unable to allocate memory for command\r\n");
        res = RRIL_RESULT_ERROR;
        goto Error;
    }

    if (E_RADIO_OFF_REASON_SHUTDOWN == radioOffReason)
    {
        WaitForModemPowerOffEvent();
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::CoreRadioPower() - Waiting for radio state changed event\r\n");

        /*
         * Incase of radio on/off request, wait for radio state change or
         * cancel event. Radio state change event will be signalled when
         * the command is sent to modem and also response is received from
         * modem. Cancel event is signalled on modem events. Cancel event
         * means that there is no some issue in handling RADIO_POWER request.
         *
         * RADIO_POWER ON sequence is as follows:
         *     - RADIO_POWER request from framework.
         *     - If modem is not powered on, acquire the modem resource
         *     - Wait for modem powered on and basic initialization completion
         *       event.
         *     - Add modem specific RADIO_POWER ON commands to command queue
         *       and wait for radio state changed event
         *     - Channel specific command thread sends the command to modem
         *     - Upon response from modem, parser and then post command
         *     - handlers will be called.
         *     - Post command handler set the radio state to ON and notifies
         *       the framework.
         *     - Post command handler signals radio state changed event
         *     - Radio state changed event unblocks the ril request handling
         *       thread which was blocked in handing RADIO_POWER ON request.
         *     - Upon radio state changed event complete the RADIO_POWER ON
         *       request.
         */
        CEvent* pCancelWaitEvent = CSystemManager::GetInstance().GetCancelWaitEvent();

        if (NULL != pRadioStateChangedEvent && NULL != pCancelWaitEvent)
        {
            CEvent* rgpEvents[] = { pRadioStateChangedEvent, pCancelWaitEvent };

            CEvent::WaitForAnyEvent(2/*NumEvents*/, rgpEvents, WAIT_FOREVER);
        }
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreRadioPower() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseRadioPower(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseRadioPower() - Enter / Exit\r\n");
    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_DTMF
//
RIL_RESULT_CODE CTEBase::CoreDtmf(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreDtmf() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char tone;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDtmf() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != sizeof(char*))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDtmf() - Invalid data size.\r\n");
        goto Error;
    }

    tone = ((char*)pData)[0];

    //  Need to stop any outstanding tone first.
    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+VTS=%c\r", tone))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDtmf() - Unable to write VTS=tone string to buffer\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreDtmf() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseDtmf(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseDtmf() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseDtmf() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SEND_SMS
//
RIL_RESULT_CODE CTEBase::CoreSendSms(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSendSms() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char**       ppszCmdData   = NULL;
    char*        pszSMSAddress = NULL;
    char*        pszPDU        = NULL;

    int nPDULength = 0;
    char szNoAddress[] = "00";

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSms() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != 2 * sizeof(char*))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSms() - Invalid data size. Was given %d bytes\r\n",
                uiDataSize);
        goto Error;
    }

    ppszCmdData   = (char**)pData;

    pszSMSAddress = ppszCmdData[0];

    pszPDU        = ppszCmdData[1];

    if (NULL == pszPDU)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSms() - Invalid input(s).\r\n");
        goto Error;
    }

    // 2 chars per byte.
    nPDULength = (strlen(pszPDU) / 2);

    if (NULL == pszSMSAddress)
    {
        pszSMSAddress = szNoAddress;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CMGS=%u\r",
            nPDULength))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSms() - Cannot create CMGS command\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd2, sizeof(rReqData.szCmd2), "%s%s\x1a",
            pszSMSAddress, pszPDU))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSms() - Cannot create CMGS PDU\r\n");
        goto Error;
    }

    RIL_LOG_INFO("Payload: %s\r\n", CRLFExpandedString(rReqData.szCmd2,
            strlen(rReqData.szCmd2)).GetString());

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSendSms() - Exit\r\n");
    return res;

}

RIL_RESULT_CODE CTEBase::ParseSendSms(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSendSms() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    P_ND_SEND_MSG pSendMsg = NULL;
    UINT32          uiMsgRef;

    pSendMsg = (P_ND_SEND_MSG)malloc(sizeof(S_ND_SEND_MSG));
    if (NULL == pSendMsg)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSms() -"
                " Could not allocate memory for a S_ND_SEND_MSG struct.\r\n");
        goto Error;
    }
    memset(pSendMsg, 0, sizeof(S_ND_SEND_MSG));

    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSms() - Could not parse response prefix.\r\n");
        goto Error;
    }

    //  Sometimes modems add another rspStart here due to sending of a PDU.
    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    if (!SkipString(pszRsp, "+CMGS: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSms() - Could not parse \"+CMGS: \".\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, uiMsgRef, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSms() - Could not parse <msgRef>.\r\n");
        goto Error;
    }
    else
    {
        pSendMsg->smsRsp.messageRef = (int)uiMsgRef;
    }

    if (SkipString(pszRsp, ",", pszRsp))
    {
        if (!ExtractUnquotedString(pszRsp, m_cTerminator, pSendMsg->szAckPDU, 160, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseSendSms() - Could not parse <ackPdu>.\r\n");
            goto Error;
        }

        pSendMsg->smsRsp.ackPDU = pSendMsg->szAckPDU;
    }
    else
    {
        pSendMsg->smsRsp.ackPDU = NULL;
    }

    //  Error code is n/a.
    pSendMsg->smsRsp.errorCode = -1;

    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSms() - Could not parse response postfix.\r\n");
        goto Error;
    }

    rRspData.pData   = (void*)pSendMsg;
    rRspData.uiDataSize  = sizeof(RIL_SMS_Response);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pSendMsg);
        pSendMsg = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseSendSms() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SEND_SMS_EXPECT_MORE
//
RIL_RESULT_CODE CTEBase::CoreSendSmsExpectMore(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSendSmsExpectMore() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char**       pszCmdData   = NULL;
    char*        szSMSAddress = NULL;
    char*        szPDU        = NULL;

    int nPDULength = 0;
    char szNoAddress[] = "00";

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSmsExpectMore() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if (uiDataSize != 2 * sizeof(char*))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSmsExpectMore() -"
                " Invalid data size.  uiDataSize=[%d]\r\n", uiDataSize);
        goto Error;
    }

    pszCmdData   = (char**)pData;

    szSMSAddress = pszCmdData[0];

    szPDU        = pszCmdData[1];

    if (NULL == szPDU)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSmsExpectMore() - Invalid input(s).\r\n");
        goto Error;
    }

    // 2 chars per byte.
    nPDULength = (strlen(szPDU) / 2);

    if (NULL == szSMSAddress)
    {
        szSMSAddress = szNoAddress;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CMMS=1;+CMGS=%u\r", nPDULength))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSmsExpectMore() - Cannot create CMGS command\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd2, sizeof(rReqData.szCmd2), "%s%s\x1a",
            szSMSAddress, szPDU))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendSmsExpectMore() - Cannot create CMGS PDU\r\n");
        goto Error;
    }

    RIL_LOG_INFO("Payload: %s\r\n", CRLFExpandedString(rReqData.szCmd2,
            strlen(rReqData.szCmd2)).GetString());

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSendSmsExpectMore() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSendSmsExpectMore(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSendSmsExpectMore() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    P_ND_SEND_MSG pSendMsg = NULL;
    UINT32          uiMsgRef;

    pSendMsg = (P_ND_SEND_MSG)malloc(sizeof(S_ND_SEND_MSG));
    if (NULL == pSendMsg)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSmsExpectMore() -"
                " Could not allocate memory for a S_ND_SEND_MSG struct.\r\n");
        goto Error;
    }
    memset(pSendMsg, 0, sizeof(S_ND_SEND_MSG));

    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSmsExpectMore() -"
                " Could not parse response prefix.\r\n");
        goto Error;
    }

    if (!SkipString(pszRsp, "+CMGS: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSmsExpectMore() -"
                " Could not parse \"+CMGS: \".\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, uiMsgRef, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSmsExpectMore() - Could not parse <msgRef>.\r\n");
        goto Error;
    }
    else
    {
        pSendMsg->smsRsp.messageRef = (int)uiMsgRef;
    }

    if (!SkipString(pszRsp, ",", pszRsp))
    {
        if (!ExtractUnquotedString(pszRsp, m_cTerminator, pSendMsg->szAckPDU, 160, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseSendSmsExpectMore() - Could not parse <ackPdu>.\r\n");
            goto Error;
        }

        pSendMsg->smsRsp.ackPDU = pSendMsg->szAckPDU;
    }
    else
    {
        pSendMsg->smsRsp.ackPDU = NULL;
    }

    //  Error code is n/a.
    pSendMsg->smsRsp.errorCode = -1;

    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSendSmsExpectMore() -"
                " Could not parse response postfix.\r\n");
        goto Error;
    }

    rRspData.pData   = (void*)pSendMsg;
    rRspData.uiDataSize  = sizeof(RIL_SMS_Response);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pSendMsg);
        pSendMsg = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseSendSmsExpectMore() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SETUP_DATA_CALL
//
RIL_RESULT_CODE CTEBase::CoreSetupDataCall(REQUEST_DATA& /*rReqData*/, void* /*pData*/,
                                            UINT32 /*uiDataSize*/, UINT32& /*uiCID*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetupDataCall() - Enter / Exit\r\n");
    return RIL_E_REQUEST_NOT_SUPPORTED; // only supported at modem level

}

RIL_RESULT_CODE CTEBase::ParseSetupDataCall(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetupDataCall() - Enter / Exit\r\n");
    return RRIL_RESULT_OK; // only supported at modem level
}

//
// RIL_REQUEST_SIM_IO
//
RIL_RESULT_CODE CTEBase::CoreSimIo(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSimIo() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_SIM_IO_v6*   pSimIOArgs = NULL;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimIo() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if (sizeof(RIL_SIM_IO_v6) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimIo() - Invalid data size. Given %d bytes\r\n",
                uiDataSize);
        goto Error;
    }

    // extract data
    pSimIOArgs = (RIL_SIM_IO_v6*)pData;

#if defined(DEBUG)
    RIL_LOG_VERBOSE("CTEBase::CoreSimIo() -"
            " command=%d fileid=%04X path=\"%s\" p1=%d p2=%d p3=%d data=\"%s\""
            " pin2=\"%s\" aidPtr=\"%s\"\r\n",
            pSimIOArgs->command, pSimIOArgs->fileid, pSimIOArgs->path,
            pSimIOArgs->p1, pSimIOArgs->p2, pSimIOArgs->p3,
        pSimIOArgs->data, pSimIOArgs->pin2, pSimIOArgs->aidPtr);
#else
    RIL_LOG_VERBOSE("CTEBase::CoreSimIo() - command=%d fileid=%04X path=\"%s\""
            " p1=%d p2=%d p3=%d data=\"%s\" aidPtr=\"%s\"\r\n",
            pSimIOArgs->command, pSimIOArgs->fileid, pSimIOArgs->path,
            pSimIOArgs->p1, pSimIOArgs->p2, pSimIOArgs->p3,
            pSimIOArgs->data, pSimIOArgs->aidPtr);
#endif

    //  If PIN2 is required, send out AT+CPIN2 request
    if (pSimIOArgs->pin2)
    {
        RIL_LOG_INFO("CTEBase::CoreSimIo() - PIN2 required\r\n");

        if (!PrintStringNullTerminate(rReqData.szCmd1,
                     sizeof(rReqData.szCmd1),
                     "AT+CPIN2=\"%s\"\r",
                     pSimIOArgs->pin2))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSimIo() - cannot create CPIN2 command\r\n");
            goto Error;
        }


        if (NULL == pSimIOArgs->data)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd2,
                         sizeof(rReqData.szCmd2),
                         "AT+CRSM=%d,%d,%d,%d,%d\r",
                         pSimIOArgs->command,
                         pSimIOArgs->fileid,
                         pSimIOArgs->p1,
                         pSimIOArgs->p2,
                         pSimIOArgs->p3))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSimIo() - cannot create CRSM command 1\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(rReqData.szCmd2,
                         sizeof(rReqData.szCmd2),
                         "AT+CRSM=%d,%d,%d,%d,%d,\"%s\"\r",
                         pSimIOArgs->command,
                         pSimIOArgs->fileid,
                         pSimIOArgs->p1,
                         pSimIOArgs->p2,
                         pSimIOArgs->p3,
                         pSimIOArgs->data))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSimIo() - cannot create CRSM command 2\r\n");
                goto Error;
            }
        }



    }
    else
    {
        //  No PIN2


        if (NULL == pSimIOArgs->data)
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
                RIL_LOG_CRITICAL("CTEBase::CoreSimIo() - cannot create CRSM command 3\r\n");
                goto Error;
            }
        }
        else
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
                RIL_LOG_CRITICAL("CTEBase::CoreSimIo() - cannot create CRSM command 4\r\n");
                goto Error;
            }
        }
    }

    //  Set the context of this command to the SIM_IO command-type.
    rReqData.pContextData = (void*)(uintptr_t)pSimIOArgs->command;


    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSimIo() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSimIo(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimIo() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    UINT32  uiSW1 = 0;
    UINT32  uiSW2 = 0;
    char* szResponseString = NULL;
    UINT32  cbResponseString = 0;

    RIL_SIM_IO_Response* pResponse = NULL;

    if (NULL == rRspData.szResponse)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimIo() - Response String pointer is NULL.\r\n");
        goto Error;
    }

    // Parse "<prefix>+CRSM: <sw1>,<sw2>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimIo() - Could not skip over response prefix.\r\n");
        goto Error;
    }

    if (!SkipString(pszRsp, "+CRSM: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimIo() - Could not skip over \"+CRSM: \".\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, uiSW1, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimIo() - Could not extract SW1 value.\r\n");
        goto Error;
    }

    if (!SkipString(pszRsp, ",", pszRsp) ||
        !ExtractUInt32(pszRsp, uiSW2, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimIo() - Could not extract SW2 value.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTEBase::ParseSimIo() - Extracted SW1 = %u and SW2 = %u\r\n", uiSW1, uiSW2);

    // Parse ","
    if (SkipString(pszRsp, ",", pszRsp))
    {
        // Parse <response>
        // NOTE: we take ownership of allocated szResponseString
        if (!ExtractQuotedStringWithAllocatedMemory(pszRsp, szResponseString,
                cbResponseString, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseSimIo() - Could not extract data string.\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CTEBase::ParseSimIo() - Extracted data string: \"%s\" (%u chars)\r\n",
                    szResponseString, cbResponseString);
        }

        if (0 != (cbResponseString - 1) % 2)
        {
            RIL_LOG_CRITICAL("CTEBase::ParseSimIo() : String was not a multiple of 2.\r\n");
            goto Error;
        }
    }

    // Allocate memory for the response struct PLUS a buffer for the response string
    // The char* in the RIL_SIM_IO_Response will point to the buffer allocated directly
    // after the RIL_SIM_IO_Response
    // When the RIL_SIM_IO_Response is deleted, the corresponding response string will be
    // freed as well.
    pResponse = (RIL_SIM_IO_Response*)malloc(sizeof(RIL_SIM_IO_Response) + cbResponseString + 1);
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimIo() -"
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
            RIL_LOG_CRITICAL("CTEBase::ParseSimIo() -"
                    " Could not CopyStringNullTerminate szResponseString\r\n");
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

    RIL_LOG_VERBOSE("CTEBase::ParseSimIo() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SEND_USSD
//
RIL_RESULT_CODE CTEBase::CoreSendUssd(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSendUssd() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char *szUssdString = NULL;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendUssd() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if (sizeof(char *) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendUssd() - Invalid data size.\r\n");
        goto Error;
    }

    // extract data
    szUssdString = (char *)pData;

    /*
     * According to Android ril.h , CUSD messages are allways sent as utf8,
     * but the dcs field does not have an entry for this.
     * The nearest "most correct" would be 15 = unspecified,
     * not adding the dcs would result in the default "0" meaning German,
     * and some networks are not happy with this.
     */
    if (szUssdString[0] == '\0')
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSendUssd() -"
                " USSD String empty, don't forward to the Modem\r\n");
        goto Error;
    }
    else
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CUSD=0,\"%s\",15\r", szUssdString))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSendUssd() - cannot create CUSD command\r\n");
            goto Error;
        }

        if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSendUssd() - Cannot create CEER command\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSendUssd() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSendUssd(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSendUssd() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        res = (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseSendUssd() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CANCEL_USSD
//
RIL_RESULT_CODE CTEBase::CoreCancelUssd(REQUEST_DATA& rReqData,
                                                void* /*pData*/,
                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCancelUssd() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CUSD=2\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreCancelUssd() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCancelUssd(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCancelUssd() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseCancelUssd() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_CLIR
//
RIL_RESULT_CODE CTEBase::CoreGetClir(REQUEST_DATA& rReqData, void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetClir() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+CLIR?\r", sizeof(rReqData.szCmd1)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreGetClir() - Unable to write command to buffer\r\n");
        goto Error;
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreGetClir() - Cannot create CEER command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreGetClir() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGetClir(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetClir() - Enter\r\n");

    int* pCLIRBlob = NULL;

    UINT32 nValue;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        return (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    pCLIRBlob = (int *)malloc(sizeof(int) * 2);

    if (NULL == pCLIRBlob)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetClir() - Could not allocate memory for response.\r\n");
        goto Error;
    }
    memset(pCLIRBlob, 0, sizeof(int) * 2);

    // Parse "<prefix>+CLIR: <status>"
    if (!SkipRspStart(szRsp, m_szNewLine, szRsp)          ||
        !SkipString(szRsp, "+CLIR: ", szRsp) ||
        !ExtractUInt32(szRsp, nValue, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetClir() - Could not find status value\r\n");
        goto Error;
    }

    pCLIRBlob[0] = nValue;

    // Parse ",<provisioning><postfix>"
    if (!SkipString(szRsp, ",", szRsp)     ||
        !ExtractUInt32(szRsp, nValue, szRsp) ||
        !SkipRspEnd(szRsp, m_szNewLine, szRsp))
    {
        goto Error;
    }

    pCLIRBlob[1] = nValue;

    rRspData.pData  = (void*)pCLIRBlob;
    rRspData.uiDataSize = 2 * sizeof(int);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pCLIRBlob);
        pCLIRBlob = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetClir() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_CLIR
//
RIL_RESULT_CODE CTEBase::CoreSetClir(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetClir() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnClir = NULL;

    if (sizeof(int) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetClir() - Passed data size mismatch. Found %d bytes\r\n",
                uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetClir() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pnClir = (int*)pData;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CLIR=%u\r", pnClir[0]))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetClir() - Unable to write command to buffer\r\n");
        goto Error;
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetClir() - Cannot create CEER command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetClir() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetClir(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetClir() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        res = (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseSetClir() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
//
RIL_RESULT_CODE CTEBase::CoreQueryCallForwardStatus(REQUEST_DATA& rReqData,
                                                               void* pData,
                                                               UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryCallForwardStatus() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_CallForwardInfo* pCallFwdInfo = NULL;

    if (sizeof(RIL_CallForwardInfo) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryCallForwardStatus() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryCallForwardStatus() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    pCallFwdInfo = (RIL_CallForwardInfo*)pData;

    if ((RRIL_INFO_CLASS_NONE == pCallFwdInfo->serviceClass) ||
            ((RRIL_INFO_CLASS_VOICE | RRIL_INFO_CLASS_DATA | RRIL_INFO_CLASS_FAX) ==
                pCallFwdInfo->serviceClass))
    {
        if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                        sizeof(rReqData.szCmd1),
                                        "AT+CCFC=%u,2\r",
                                        pCallFwdInfo->reason))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreQueryCallForwardStatus() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }
    else
    {
        if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                        sizeof(rReqData.szCmd1),
                                        "AT+CCFC=%u,2,,,%u\r",
                                        pCallFwdInfo->reason,
                                        pCallFwdInfo->serviceClass))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreQueryCallForwardStatus() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryCallForwardStatus() - Cannot create CEER command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

    //  Store the reason data for the query, which will be used for preparing response
    rReqData.pContextData = (void*)(intptr_t)pCallFwdInfo->reason;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreQueryCallForwardStatus() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseQueryCallForwardStatus(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryCallForwardStatus() - Enter\r\n");

    P_ND_CALLFWD_DATA pCallFwdBlob = NULL;

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;
    UINT32 nEntries = 0;
    UINT32 nCur = 0;
    UINT32 nValue;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        return (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    while (FindAndSkipString(szRsp, "+CCFC: ", szRsp))
    {
        nEntries++;
    }

    RIL_LOG_INFO("CTEBase::ParseQueryCallForwardStatus() - INFO: Found %d CCFC entries!\r\n",
            nEntries);

    if (RIL_MAX_CALLFWD_ENTRIES < nEntries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryCallForwardStatus() - Too many CCFC entries!\r\n");
        goto Error;
    }

    pCallFwdBlob = (P_ND_CALLFWD_DATA)malloc(sizeof(S_ND_CALLFWD_DATA));

    if (NULL == pCallFwdBlob)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryCallForwardStatus() -"
                " Could not allocate memory for a S_ND_CALLFWD_DATA struct.\r\n");
        goto Error;
    }
    memset(pCallFwdBlob, 0, sizeof(S_ND_CALLFWD_DATA));

    // Reset our buffer to the beginning of the response
    szRsp = rRspData.szResponse;

    // Parse "+CCFC: "
    while (FindAndSkipString(szRsp, "+CCFC: ", szRsp))
    {
        // Stored reason value is updated when we have some data to send
        pCallFwdBlob->aRilCallForwardInfo[nCur].reason = (intptr_t)rRspData.pContextData;

        // Parse "<status>"
        if (!ExtractUInt32(szRsp, nValue, szRsp))
        {
            RIL_LOG_WARNING("CTEBase::ParseQueryCallForwardStatus() -"
                    " WARN: Could not find status value, skipping entry\r\n");
            goto Continue;
        }

        pCallFwdBlob->aRilCallForwardInfo[nCur].status = nValue;

        // Parse ",<serviceClass>"
        if (!SkipString(szRsp, ",", szRsp) ||
            !ExtractUInt32(szRsp, nValue, szRsp))
        {
            RIL_LOG_WARNING("CTEBase::ParseQueryCallForwardStatus() -"
                    " WARN: Could not find service class value, skipping entry\r\n");
            goto Continue;
        }

        pCallFwdBlob->aRilCallForwardInfo[nCur].serviceClass = nValue;

        // Parse ","
        if (SkipString(szRsp, ",", szRsp))
        {
            // Parse "<address>,<type>"
            if (!ExtractQuotedString(szRsp, pCallFwdBlob->aszCallForwardNumber[nCur],
                    MAX_NUMBER_SIZE, szRsp) ||
                    !SkipString(szRsp, ",", szRsp))
            {
                RIL_LOG_WARNING("CTEBase::ParseQueryCallForwardStatus() -"
                        " WARN: Could not find address string, skipping entry\r\n");
                goto Continue;
            }

            //  Parse type if available.
            if (ExtractUpperBoundedUInt32(szRsp, 0x100, nValue, szRsp))
            {
                pCallFwdBlob->aRilCallForwardInfo[nCur].toa = nValue;
            }
            else
            {
                pCallFwdBlob->aRilCallForwardInfo[nCur].toa = 0;
            }

            // Parse ","
            if (SkipString(szRsp, ",", szRsp))
            {
                // No support for subaddress in Android... skipping over
                // Parse "<subaddr>,<subaddr_type>"

                // Parse ","
                if (!FindAndSkipString(szRsp, ",", szRsp))
                {
                    RIL_LOG_WARNING("CTEBase::ParseQueryCallForwardStatus() -"
                            " WARN: Couldn't find comma after subaddress, skipping entry\r\n");
                    goto Continue;
                }

                // Parse ","
                if (FindAndSkipString(szRsp, ",", szRsp))
                {
                    // Parse "<time>"
                    if (!ExtractUInt32(szRsp, nValue, szRsp))
                    {
                        RIL_LOG_WARNING("CTEBase::ParseQueryCallForwardStatus() -"
                                " WARN: Couldn't find comma after time, skipping entry\r\n");
                        goto Continue;
                    }

                    pCallFwdBlob->aRilCallForwardInfo[nCur].timeSeconds = nValue;
                }
            }
        }

        pCallFwdBlob->aRilCallForwardInfo[nCur].number = pCallFwdBlob->aszCallForwardNumber[nCur];
        pCallFwdBlob->apRilCallForwardInfo[nCur] = &(pCallFwdBlob->aRilCallForwardInfo[nCur]);

        // Increment the array index
        nCur++;

Continue:
        if (!FindAndSkipRspEnd(szRsp, m_szNewLine, szRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQueryCallForwardStatus() -"
                    " Could not find response end\r\n");
            goto Error;
        }
    }

    rRspData.pData  = (void*)pCallFwdBlob;
    rRspData.uiDataSize = nCur * sizeof(RIL_CallForwardInfo*);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pCallFwdBlob);
        pCallFwdBlob = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryCallForwardStatus() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_CALL_FORWARD
//
RIL_RESULT_CODE CTEBase::CoreSetCallForward(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetCallForward() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 nValue = 0;
    RIL_CallForwardInfo* pCallFwdInfo = NULL;
    const int nNumberBufLen = 255;
    char szNumber[nNumberBufLen] = {0};

    if (sizeof(RIL_CallForwardInfo) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetCallForward() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetCallForward() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pCallFwdInfo = (RIL_CallForwardInfo*)pData;

    if (NULL != pCallFwdInfo->number)
    {
        RIL_LOG_INFO("CTEBase::CoreSetCallForward() - status: %d   info class: %d   reason: %d"
                "   number: %s   toa: %d   time: %d\r\n",
                pCallFwdInfo->status,
                pCallFwdInfo->serviceClass,
                pCallFwdInfo->reason,
                pCallFwdInfo->number,
                pCallFwdInfo->toa,
                pCallFwdInfo->timeSeconds);
        strncpy(szNumber, pCallFwdInfo->number, nNumberBufLen-1);
        szNumber[nNumberBufLen-1] = '\0';  //  KW fix
    }
    else
    {
        RIL_LOG_INFO("CTEBase::CoreSetCallForward() - status: %d   info class: %d   reason: %d"
                "   number: %s   toa: %d   time: %d\r\n",
                pCallFwdInfo->status,
                pCallFwdInfo->serviceClass,
                pCallFwdInfo->reason,
                "NULL",
                pCallFwdInfo->toa,
                pCallFwdInfo->timeSeconds);
        strncpy(szNumber, "", nNumberBufLen-1);
    }

    if (pCallFwdInfo->serviceClass)
    {
        if (pCallFwdInfo->timeSeconds)
        {
            if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                            sizeof(rReqData.szCmd1),
                                            "AT+CCFC=%u,%u,\"%s\",%u,%u,,,%u\r",
                                            pCallFwdInfo->reason,
                                            pCallFwdInfo->status,
                                            szNumber  /*pCallFwdInfo->number*/,
                                            pCallFwdInfo->toa,
                                            pCallFwdInfo->serviceClass,
                                            pCallFwdInfo->timeSeconds))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSetCallForward() -"
                        " Unable to write command to buffer\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                            sizeof(rReqData.szCmd1),
                                            "AT+CCFC=%u,%u,\"%s\",%u,%u\r",
                                            pCallFwdInfo->reason,
                                            pCallFwdInfo->status,
                                            szNumber /*pCallFwdInfo->number*/,
                                            pCallFwdInfo->toa,
                                            pCallFwdInfo->serviceClass))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSetCallForward() -"
                        " Unable to write command to buffer\r\n");
                goto Error;
            }
        }
    }
    else
    {
        if (pCallFwdInfo->timeSeconds)
        {
            if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                            sizeof(rReqData.szCmd1),
                                            "AT+CCFC=%u,%u,\"%s\",%u,,,,%u\r",
                                            pCallFwdInfo->reason,
                                            pCallFwdInfo->status,
                                            szNumber /*pCallFwdInfo->number*/,
                                            pCallFwdInfo->toa,
                                            pCallFwdInfo->timeSeconds))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSetCallForward() -"
                        " Unable to write command to buffer\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                            sizeof(rReqData.szCmd1),
                                            "AT+CCFC=%u,%u,\"%s\",%u\r",
                                            pCallFwdInfo->reason,
                                            pCallFwdInfo->status,
                                            szNumber /*pCallFwdInfo->number*/ ,
                                            pCallFwdInfo->toa))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSetCallForward() -"
                        " Unable to write command to buffer\r\n");
                goto Error;
            }
        }
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetCallForward() - Cannot create CEER command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetCallForward() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetCallForward(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetCallForward() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        res = (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseSetCallForward() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_QUERY_CALL_WAITING
//
RIL_RESULT_CODE CTEBase::CoreQueryCallWaiting(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryCallWaiting() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnInfoClasses = NULL;

    if (sizeof(int) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryCallWaiting() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryCallWaiting() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pnInfoClasses = (int*)pData;

    if ((RRIL_INFO_CLASS_NONE == pnInfoClasses[0]) ||
        ((RRIL_INFO_CLASS_VOICE | RRIL_INFO_CLASS_DATA | RRIL_INFO_CLASS_FAX) == pnInfoClasses[0]))
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1,
                        sizeof(rReqData.szCmd1),
                        "AT+CCWA=1,2\r"))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreQueryCallWaiting() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }
    else
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1,
                        sizeof(rReqData.szCmd1),
                        "AT+CCWA=1,2,%u\r",
                        pnInfoClasses[0]))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreQueryCallWaiting() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryCallWaiting() - Cannot create CEER command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreQueryCallWaiting() - Exit\r\n");
    return res;

}

RIL_RESULT_CODE CTEBase::ParseQueryCallWaiting(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryCallWaiting() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* prgnCallWaiting = NULL;
    const char* szRsp = rRspData.szResponse;
    UINT32 nStatus;
    UINT32 nClass;
    UINT32 dwServiceInfo = 0, dwStatus = 0;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        return (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    // Parse "+CCWA: "
    while (FindAndSkipString(szRsp, "+CCWA: ", szRsp))
    {
        // Parse "<status>,<class>"
        if (!ExtractUInt32(szRsp, nStatus, szRsp) ||
            !SkipString(szRsp, ",", szRsp) ||
            !ExtractUInt32(szRsp, nClass, szRsp))
        {
            RIL_LOG_WARNING("CTEBase::ParseQueryCallWaiting() -"
                    " WARN: Unable to extract UINTS, skip to next entry\r\n");
            goto Continue;
        }

        RIL_LOG_INFO("CTEBase::ParseQueryCallWaiting() - INFO: Status= %d    Class=%d\r\n",
                nStatus, nClass);

        if (1 == nStatus)
        {
            dwStatus = 1;

            dwServiceInfo |= nClass;

            RIL_LOG_INFO("CTEBase::ParseQueryCallWaiting() -"
                    " INFO: Recording service %d. Current mask: %d\r\n", nClass, dwServiceInfo);
        }

Continue:
        // Find "<postfix>"
        if (!FindAndSkipRspEnd(szRsp, m_szNewLine, szRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQueryCallWaiting() - Unable to find response end\r\n");
            goto Error;
        }
    }

    RIL_LOG_INFO("CTEBase::ParseQueryCallWaiting() -"
            " INFO: No more +CCWA strings found, building result struct\r\n");

    // If we have an active infoclass, we return 2 ints.
    if (1 == dwStatus)
    {
        RIL_LOG_INFO("CTEBase::ParseQueryCallWaiting() - INFO: Returning 2 ints\r\n");
        rRspData.uiDataSize = 2 * sizeof(int);

        prgnCallWaiting = (int *)malloc(rRspData.uiDataSize);
        if (!prgnCallWaiting)
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQueryCallWaiting() -"
                    " Could not allocate memory for response.\r\n");
            goto Error;
        }

        prgnCallWaiting[0] = dwStatus;
        prgnCallWaiting[1] = dwServiceInfo;
    }
    else
    {
        RIL_LOG_INFO("CTEBase::ParseQueryCallWaiting() - INFO: Returning 1 int\r\n");
        rRspData.uiDataSize = sizeof(int);

        prgnCallWaiting = (int *)malloc(rRspData.uiDataSize);
        if (!prgnCallWaiting)
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQueryCallWaiting() -"
                    " Could not allocate memory for response.\r\n");
            goto Error;
        }

        prgnCallWaiting[0] = dwStatus;
    }

    rRspData.pData   = (void*)prgnCallWaiting;
    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(prgnCallWaiting);
        prgnCallWaiting = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryCallWaiting() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_CALL_WAITING
//
RIL_RESULT_CODE CTEBase::CoreSetCallWaiting(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetCallWaiting() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    UINT32 nStatus;
    UINT32 nInfoClasses;

    if ((2 * sizeof(int)) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetCallWaiting() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetCallWaiting() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    nStatus = ((int*)pData)[0];
    nInfoClasses = ((int*)pData)[1];

    if (RRIL_INFO_CLASS_NONE == nInfoClasses)
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CCWA=1,%u\r", nStatus))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSetCallWaiting() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }
    else
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CCWA=1,%u,%u\r", nStatus, nInfoClasses))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSetCallWaiting() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetCallWaiting() - Cannot create CEER command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetCallWaiting() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetCallWaiting(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetCallWaiting() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        res = (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseSetCallWaiting() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SMS_ACKNOWLEDGE
//
RIL_RESULT_CODE CTEBase::CoreSmsAcknowledge(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSmsAcknowledge() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    int* pCmdData = NULL;
    int status = 0;
    int failCause = 0;
    const int MAX_PDU_LENGTH = 3; // only 3 octets needed for DELIVER-REPORT
    char szPdu[8] = {0};

    if ((sizeof(int) != uiDataSize) && ((2 * sizeof(int)) != uiDataSize))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSmsAcknowledge() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSmsAcknowledge() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pCmdData = (int*)pData;

    status = pCmdData[0];
    failCause = pCmdData[1];

    //  We ack the SMS in silo_sms so just send AT here to get OK response and keep
    //  upper layers happy. Unless the want to send an unsuccessful ACK, then do so.

    if (1 == status) // successful receipt
    {
        if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+CNMA=1\r", sizeof(rReqData.szCmd1)))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSmsAcknowledge() - Cannot create CNMA command\r\n");
            goto Error;
        }
    }
    else if (0 == status) // failed receipt
    {
        // SMS-DELIVER-REPORT TPDU (negative ack) format, 3GPP TS 23.040, 9.2.2.1a:
        //   octet 1: Message Type Indicator - bits 0 & 1: 0 for SMS-DELIVER-REPORT
        //   octet 2: Failure Cause
        //   octet 3: Parameter Indicator - bits 0-7: set to 0, optional fields not used

        switch (failCause)
        {
            case CMS_ERROR_MEMORY_CAPACITY_EXCEEDED: // 0xD3
                RIL_LOG_INFO("CTEBase::CoreSmsAcknowledge() - MEMORY_CAPACIY_EXCEEDED\r\n");
                snprintf(szPdu, sizeof(szPdu), "%s", "00D300");
                break;

            case CMS_ERROR_UNSPECIFIED_FAILURE_CAUSE: // 0xFF
            default:
                RIL_LOG_INFO("CTEBase::CoreSmsAcknowledge() - Unspecified failure cause\r\n");
                snprintf(szPdu, sizeof(szPdu), "%s", "00FF00");
                break;
        }

        if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CNMA=2,%u\r", MAX_PDU_LENGTH))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSmsAcknowledge() - Cannot create CNMA command\r\n");
            goto Error;
        }

        if (!PrintStringNullTerminate(rReqData.szCmd2, sizeof(rReqData.szCmd2), "%s\x1a", szPdu))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSmsAcknowledge() - Cannot create CNMA PDU\r\n");
            goto Error;
        }

        RIL_LOG_INFO("pdu: %s\r\n",
                CRLFExpandedString(rReqData.szCmd2, strlen(rReqData.szCmd2)).GetString());
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSmsAcknowledge() - Invalid parameter!\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSmsAcknowledge() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSmsAcknowledge(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSmsAcknowledge() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseSmsAcknowledge() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_IMEI
//
RIL_RESULT_CODE CTEBase::CoreGetImei(REQUEST_DATA& rReqData, void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetImei() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CGSN\r"))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreGetImei() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGetImei(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetImei() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    const char* szRsp = rRspData.szResponse;
    char szBuildTypeProperty[PROPERTY_VALUE_MAX] = {'\0'};
    char* szIMEI = (char*)malloc(PROPERTY_VALUE_MAX);

    if (NULL == szIMEI)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImei() -"
                " Could not allocate memory for a %u-char string.\r\n", PROPERTY_VALUE_MAX);
        goto Error;
    }
    memset(szIMEI, 0, PROPERTY_VALUE_MAX);

    if (!SkipRspStart(szRsp, m_szNewLine, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImei() - Could not find response start\r\n");
        goto Error;
    }

    if (!ExtractUnquotedString(szRsp, m_cTerminator, szIMEI, PROPERTY_VALUE_MAX, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImei() - Could not find unquoted string\r\n");
        goto Error;
    }
    RIL_LOG_INFO("CTEBase::ParseGetImei() - szIMEI=[%s]\r\n", szIMEI);

    res = RRIL_RESULT_OK;

    rRspData.pData   = (void*)szIMEI;
    rRspData.uiDataSize  = sizeof(char*);

    // Update IMEI property if build type is "eng" or "userdebug"
    if (property_get("ro.build.type", szBuildTypeProperty, NULL))
    {
        const char szTypeEng[] = "eng";
        const char szTypeUserDebug[] = "userdebug";
        char szProductImeiProperty[PROPERTY_VALUE_MAX] = {'\0'};

        if ((strncmp(szBuildTypeProperty, szTypeEng, strlen(szTypeEng)) == 0)
                || (strncmp(szBuildTypeProperty, szTypeUserDebug, strlen(szTypeUserDebug)) == 0))
        {
            snprintf(szProductImeiProperty, sizeof(szProductImeiProperty), "%s", szIMEI);
            property_set("persist.radio.device.imei", szProductImeiProperty);
        }
        else
        {
            // Since the property persists on OTA update, set the property to null if present.
            int valueLen = property_get("persist.radio.device.imei", szProductImeiProperty, "");
            if (valueLen > 0)
            {
                property_set("persist.radio.device.imei", NULL);
            }
        }
    }

    // check for default flashed IMEI
    CheckImeiBlacklist(szIMEI);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(szIMEI);
        szIMEI = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetImei() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_IMEISV
//
RIL_RESULT_CODE CTEBase::CoreGetImeisv(REQUEST_DATA& rReqData,
                                               void* /*pData*/,
                                               UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetImeisv() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CGMR\r"))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreGetImeisv() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGetImeisv(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetImeisv() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    const char* szRsp = rRspData.szResponse;
    char* szIMEISV = (char*)malloc(PROPERTY_VALUE_MAX);
    char szSV[MAX_BUFFER_SIZE] = {0};
    char szSVDigits[MAX_BUFFER_SIZE] = {0};
    int nIndex = 0;

    if (NULL == szIMEISV)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImeisv() -"
                " Could not allocate memory for a %u-char string.\r\n", PROPERTY_VALUE_MAX);
        goto Error;
    }
    memset(szIMEISV, 0, PROPERTY_VALUE_MAX);

    //  Skip over <prefix> if there.
    if (!SkipRspStart(szRsp, m_szNewLine, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImeisv() - Could not find response start\r\n");
        goto Error;
    }

    //  Skip spaces (if any)
    SkipSpaces(szRsp, szRsp);

    //  Grab SV into szSV
    if (!ExtractUnquotedString(szRsp, m_cTerminator, szSV, MAX_BUFFER_SIZE, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImeisv() - Could not find unquoted string szSV\r\n");
        goto Error;
    }

    //  The output of CGMR is "Vx.y,d1d2" where d1d2 are the two digits of SV and x.y integers
    //  Only copy the digits into szSVDigits
    for (UINT32 i=0; i<strlen(szSV) && i<MAX_BUFFER_SIZE; i++)
    {
        if ( ('0' <= szSV[i]) && ('9' >= szSV[i]) )
        {
            szSVDigits[nIndex] = szSV[i];
            nIndex++;
        }
    }

    // Get only the last two digits as SV digits.
    if(nIndex > 2)
    {
        szSVDigits[0] = szSVDigits[nIndex - 2];
        szSVDigits[1] = szSVDigits[nIndex - 1];
    }

    //  Copy 2 digits SV into szIMEISV
    if (!PrintStringNullTerminate(szIMEISV, PROPERTY_VALUE_MAX,
            "%c%c", szSVDigits[0], szSVDigits[1]))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetImeisv() - Could not copy string szIMEISV\r\n");
        goto Error;
    }
    RIL_LOG_INFO("CTEBase::ParseGetImeisv() - szIMEISV=[%s]\r\n", szIMEISV);

    res = RRIL_RESULT_OK;

    rRspData.pData   = (void*)szIMEISV;
    rRspData.uiDataSize  = sizeof(char*);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(szIMEISV);
        szIMEISV = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetImeisv() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_ANSWER
//
RIL_RESULT_CODE CTEBase::CoreAnswer(REQUEST_DATA& rReqData, void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreAnswer() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "ATA\r"))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreAnswer() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseAnswer(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseAnswer() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    m_IncomingCallInfo.isAnswerReqSent = true;

    RIL_LOG_VERBOSE("CTEBase::ParseAnswer() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_DEACTIVATE_DATA_CALL
//
RIL_RESULT_CODE CTEBase::CoreDeactivateDataCall(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreDeactivateDataCall() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszCid = NULL;
    int nCid = 0;

    if (uiDataSize < (1 * sizeof(char*)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDeactivateDataCall() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDeactivateDataCall() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pszCid = ((char**)pData)[0];
    if (NULL == pszCid || '\0' == pszCid[0])
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDeactivateDataCall() - pszCid was NULL\r\n");
        goto Error;
    }

    //  Get CID as int.
    if (sscanf(pszCid, "%d", &nCid) == EOF)
    {
        // Error
        RIL_LOG_CRITICAL("CTEBase::CoreDeactivateDataCall() - cannot convert %s to int\r\n",
                pszCid);
        goto Error;
    }


    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CGACT=0,%s;+CGDCONT=%s\r", pszCid, pszCid))
    {
        res = RRIL_RESULT_OK;
    }

    //  Set the context of this command to the CID (for multiple context support).
    rReqData.pContextData = (void*)(intptr_t)nCid;  // Store this as an int.


Error:
    RIL_LOG_VERBOSE("CTEBase::CoreDeactivateDataCall() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseDeactivateDataCall(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseDeactivateDataCall() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseDeactivateDataCall() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_QUERY_FACILITY_LOCK
//
RIL_RESULT_CODE CTEBase::CoreQueryFacilityLock(REQUEST_DATA& rReqData,
                                                          void* pData,
                                                          UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryFacilityLock() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char* pszFacility = NULL;
    char* pszPassword = NULL;
    char* pszClass = NULL;

    if ((3 * sizeof(char*)) > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryFacilityLock() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryFacilityLock() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pszFacility = ((char**)pData)[0];
    pszPassword = ((char**)pData)[1];
    pszClass    = ((char**)pData)[2];

    if (NULL == pszFacility)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryFacilityLock() -"
                " Facility string pointer is NULL.\r\n");
        goto Error;
    }

    if (NULL == pszPassword || '\0' == pszPassword[0])
    {
        if (NULL == pszClass || '\0' == pszClass[0])
        {
            if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                            sizeof(rReqData.szCmd1),
                                            "AT+CLCK=\"%s\",2\r",
                                            pszFacility))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreQueryFacilityLock() -"
                        " Unable to write command to buffer\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                            sizeof(rReqData.szCmd1),
                                            "AT+CLCK=\"%s\",2,,%s\r",
                                            pszFacility,
                                            pszClass))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreQueryFacilityLock() -"
                        " Unable to write command to buffer\r\n");
                goto Error;
            }
        }
    }
    else
    {
        if (NULL == pszClass || '\0' == pszClass[0])
        {
            if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                            sizeof(rReqData.szCmd1),
                                            "AT+CLCK=\"%s\",2,\"%s\"\r",
                                            pszFacility,
                                            pszPassword))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreQueryFacilityLock() -"
                        " Unable to write command to buffer\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                            sizeof(rReqData.szCmd1),
                                            "AT+CLCK=\"%s\",2,\"%s\",%s\r",
                                            pszFacility,
                                            pszPassword,
                                            pszClass))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreQueryFacilityLock() -"
                        " Unable to write command to buffer\r\n");
                goto Error;
            }
        }
    }

    /*
     * In case of the call barring related lock query failure, query the extended error report.
     * This is to findout whether the query failed due to fdn check or general failure
     */
    if (0 == strcmp(pszFacility, "AO") ||
            0 == strcmp(pszFacility, "OI") ||
            0 == strcmp(pszFacility, "OX") ||
            0 == strcmp(pszFacility, "AI") ||
            0 == strcmp(pszFacility, "IR") ||
            0 == strcmp(pszFacility, "AB") ||
            0 == strcmp(pszFacility, "AG") ||
            0 == strcmp(pszFacility, "AC")) {
        if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreQueryFacilityLock() - Cannot create CEER command\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreQueryFacilityLock() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseQueryFacilityLock(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryFacilityLock() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnClass = NULL;
    const char* szRsp = rRspData.szResponse;
    UINT32 dwStatus = 0, dwClass = 0, dwServices = 0;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        return (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    pnClass = (int*)malloc(sizeof(int));
    if (NULL == pnClass)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryFacilityLock() -"
                " Could not allocate memory for an integer.\r\n");
        goto Error;
    }

    // Parse "+CLCK: "
    while (FindAndSkipString(szRsp, "+CLCK: ", szRsp))
    {
        // Parse "<status>"
        if (!ExtractUInt32(szRsp, dwStatus, szRsp))
        {
            RIL_LOG_WARNING("CTEBase::ParseQueryFacilityLock() -"
                    " WARN: Unable to extract <status>, skip to next entry\r\n");
            goto Continue;
        }

        //  Optionally parse <class> if there.
        if (SkipString(szRsp, ",", szRsp))
        {
            if(!ExtractUInt32(szRsp, dwClass, szRsp))
            {
                RIL_LOG_WARNING("CTEBase::ParseQueryFacilityLock() -"
                        " WARN: Unable to extract <class>, skip to next entry\r\n");
                goto Continue;
            }
        }
        else
        {
            //  Assume voice class
            dwClass = 1;
        }

        RIL_LOG_INFO("CTEBase::ParseQueryFacilityLock() - INFO: Status= %d    Class=%d,"
                " 0x%02x\r\n", dwStatus, dwClass, dwClass);

        //  If the status was active, add bit to dwServices.
        if (1 == dwStatus)
        {
            dwServices |= dwClass;

            RIL_LOG_INFO("CTEBase::ParseQueryFacilityLock() - INFO: Recording service %d,"
                    " 0x%02x. Current mask: %d, 0x%02x\r\n",
                dwClass, dwClass, dwServices, dwServices);
        }

Continue:
        // Find "<postfix>"
        if (!FindAndSkipRspEnd(szRsp, m_szNewLine, szRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQueryFacilityLock() -"
                    " Unable to find response end\r\n");
            goto Error;
        }
    }

    *pnClass = (int)dwServices;
    res = RRIL_RESULT_OK;

    rRspData.pData   = (void*)pnClass;
    rRspData.uiDataSize  = sizeof(int);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnClass);
        pnClass = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryFacilityLock() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_FACILITY_LOCK
//
RIL_RESULT_CODE CTEBase::CoreSetFacilityLock(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetFacilityLock() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char* pszFacility = NULL;
    char* pszMode = NULL;
    char* pszPassword = NULL;
    char* pszClass = NULL;
    S_SET_FACILITY_LOCK_CONTEXT_DATA* pContextData = NULL;

    if ((4 * sizeof(char *)) > uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetFacilityLock() - Passed data size mismatch."
                " Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetFacilityLock() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pszFacility = ((char**)pData)[0];
    pszMode     = ((char**)pData)[1];
    pszPassword = ((char**)pData)[2];
    pszClass    = ((char**)pData)[3];

    if ((NULL == pszFacility) || (NULL == pszMode))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetFacilityLock() -"
                " Facility or Mode strings were NULL\r\n");
        goto Error;
    }
    // Facility and Mode provided
    else if (NULL == pszPassword || '\0' == pszPassword[0])
    {
        if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                        sizeof(rReqData.szCmd1),
                                        "AT+CLCK=\"%s\",%s\r",
                                        pszFacility,
                                        pszMode))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSetFacilityLock() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }
    // Password provided
    else if (NULL == pszClass || '\0' == pszClass[0] || '0' == pszClass[0])
    {
        if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                        sizeof(rReqData.szCmd1),
                                        "AT+CLCK=\"%s\",%s,\"%s\"\r",
                                        pszFacility,
                                        pszMode,
                                        pszPassword))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSetFacilityLock() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }
    // Password and Class provided
    else
    {
        if (!PrintStringNullTerminate( rReqData.szCmd1,
                                                sizeof(rReqData.szCmd1),
                                                "AT+CLCK=\"%s\",%s,\"%s\",%s\r",
                                                pszFacility,
                                                pszMode,
                                                pszPassword,
                                                pszClass))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSetFacilityLock() -"
                    " Unable to write command to buffer\r\n");
            goto Error;
        }
    }

    /*
     * In case of the call barring related lock set failure, query the extended error report.
     * This is to find out whether the set failed due to fdn check or general failure.
     */
    if (0 == strcmp(pszFacility, "AO") ||
            0 == strcmp(pszFacility, "OI") ||
            0 == strcmp(pszFacility, "OX") ||
            0 == strcmp(pszFacility, "AI") ||
            0 == strcmp(pszFacility, "IR") ||
            0 == strcmp(pszFacility, "AB") ||
            0 == strcmp(pszFacility, "AG") ||
            0 == strcmp(pszFacility, "AC")) {
        if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSetFacilityLock() - Cannot create CEER command\r\n");
            goto Error;
        }
    }

    //  Store PIN
    if (0 == strcmp(pszFacility, "SC"))
    {
        if (0 == strcmp(pszMode, "1") && NULL != pszPassword)
        {
            strncpy(m_szPIN, pszPassword, MAX_PIN_SIZE-1);
            m_szPIN[MAX_PIN_SIZE-1] = '\0';  //  KW fix
        }
        else
        {
            strcpy(m_szPIN, "CLR");
        }
    }

    /*
     * Upon any failure in adding the request, pContextData will be freed in te.cpp.
     * If not set to NULL, then free(pContextData) will result in crash. So, better
     * to set it to NULL.
     */
    rReqData.pContextData = NULL;

    /*
     * Store the lock code which is required for determining the number of
     * retry counts left based on the lock requested.
     */
    if (0 == strncmp(pszFacility, "SC", 2) ||
            0 == strncmp(pszFacility, "FD", 2))
    {
        pContextData =
                (S_SET_FACILITY_LOCK_CONTEXT_DATA*)malloc(
                     sizeof(S_SET_FACILITY_LOCK_CONTEXT_DATA));
        if (NULL == pContextData)
        {
            RIL_LOG_INFO("CTEBase::CoreSetFacilityLock() - Not able to allocate context data\r\n");
        }
        else
        {
            strncpy(pContextData->szFacilityLock, pszFacility, MAX_FACILITY_CODE - 1);
            pContextData->szFacilityLock[MAX_FACILITY_CODE-1] = '\0';
            rReqData.pContextData = pContextData;
            rReqData.cbContextData = sizeof(S_SET_FACILITY_LOCK_CONTEXT_DATA);
        }
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetFacilityLock() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetFacilityLock(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetFacilityLock() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    int* pnRetries = NULL;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        return (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    pnRetries = (int*)malloc(sizeof(int));
    if (NULL == pnRetries)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSetFacilityLock() - Could not alloc int\r\n");
        goto Error;
    }

    //  Unknown number of retries remaining
    *pnRetries = -1;

    rRspData.pData = pnRetries;
    rRspData.uiDataSize = sizeof(int);

    if (NULL != rRspData.pContextData
            && rRspData.cbContextData == sizeof(S_SET_FACILITY_LOCK_CONTEXT_DATA))
    {
        /*
         * Context Data will be set only for SC(SIM CARD) and FD(Fixed Dialing) locks.
         * This is because modem only supports retry count information for SC and FD
         * locks via XPINCNT.
         *
         * Note: No point in calling this on success but ril documentation not clear
         */
        S_SET_FACILITY_LOCK_CONTEXT_DATA* pContextData =
                (S_SET_FACILITY_LOCK_CONTEXT_DATA*) rRspData.pContextData;

        if ((0 == strncmp(pContextData->szFacilityLock, "SC", 2))
                && (0 != strcmp(m_szPIN, "CLR")))
        {
            PCache_Store_PIN(m_szUICCID, m_szPIN);
        }
    }

    //  Clear it locally.
    memset(m_szPIN, 0, MAX_PIN_SIZE);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnRetries);
        pnRetries = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseSetFacilityLock() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CHANGE_BARRING_PASSWORD
//
RIL_RESULT_CODE CTEBase::CoreChangeBarringPassword(REQUEST_DATA& rReqData,
                                                              void* pData,
                                                              UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreChangeBarringPassword() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    const char* pszFacility;
    const char* pszOldPassword;
    const char* pszNewPassword;

    if ((3 * sizeof(char *) != uiDataSize))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeBarringPassword() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeBarringPassword() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

     pszFacility    = ((const char**)pData)[0];
     pszOldPassword = ((const char**)pData)[1];
     pszNewPassword = ((const char**)pData)[2];

    if (!PrintStringNullTerminate(  rReqData.szCmd1,
                                    sizeof(rReqData.szCmd1),
                                    "AT+CPWD=\"%s\",\"%s\",\"%s\"\r",
                                    pszFacility,
                                    pszOldPassword,
                                    pszNewPassword))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeBarringPassword() -"
                " Unable to write command to buffer\r\n");
        goto Error;
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreChangeBarringPassword() - Cannot create CEER command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreChangeBarringPassword() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseChangeBarringPassword(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseChangeBarringPassword() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        res = (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseChangeBarringPassword() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE
//
RIL_RESULT_CODE CTEBase::CoreQueryNetworkSelectionMode(REQUEST_DATA& rReqData,
                                                                  void* /*pData*/,
                                                                  UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryNetworkSelectionMode() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+COPS?\r"))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreQueryNetworkSelectionMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseQueryNetworkSelectionMode(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryNetworkSelectionMode() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int* pnMode = NULL;
    const char* szRsp = rRspData.szResponse;

    pnMode = (int*)malloc(sizeof(int));
    if (NULL == pnMode)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryNetworkSelectionMode() -"
                " Could not allocate memory for an int.\r\n");
        goto Error;
    }

    if (!FindAndSkipString(szRsp, "+COPS: ", szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryNetworkSelectionMode() - Could not find +COPS:\r\n");
        goto Error;
    }

    if (!ExtractUInt32(szRsp, (UINT32&)*pnMode, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryNetworkSelectionMode() -"
                " Could not extract the mode.\r\n");
        goto Error;
    }

    //  If we have a +COPS value that's not 0 or 1,
    //  then return RIL_E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW.
    //  This API is only supposed to return 0 or 1.
    if (*pnMode >= 2)
    {
        res = RIL_E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW;
        goto Error;
    }

    res = RRIL_RESULT_OK;

    rRspData.pData   = (void*)pnMode;
    rRspData.uiDataSize  = sizeof(int);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pnMode);
        pnMode = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryNetworkSelectionMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
//
RIL_RESULT_CODE CTEBase::CoreSetNetworkSelectionAutomatic(REQUEST_DATA& /*rReqData*/,
                                                                     void* /*pData*/,
                                                                     UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetNetworkSelectionAutomatic() - Enter\r\n");

    m_NetworkSelectionModeParams.mode = E_NETWORK_SELECTION_MODE_AUTOMATIC;

    RIL_LOG_VERBOSE("CTEBase::CoreSetNetworkSelectionAutomatic() - Exit\r\n");
    return RRIL_RESULT_OK;
}

RIL_RESULT_CODE CTEBase::ParseSetNetworkSelectionAutomatic(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetNetworkSelectionAutomatic() - Enter / Exit\r\n");
    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL
//
RIL_RESULT_CODE CTEBase::CoreSetNetworkSelectionManual(REQUEST_DATA& /*rReqData*/,
                                                                  void* pData,
                                                                  UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetNetworkSelectionManual() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    const char* pszNumeric = NULL;

    if (sizeof(char*) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetNetworkSelectionManual() - Passed data size mismatch."
                " Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetNetworkSelectionManual() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    pszNumeric = (char*)pData;

    m_NetworkSelectionModeParams.mode = E_NETWORK_SELECTION_MODE_MANUAL;
    CopyStringNullTerminate(m_NetworkSelectionModeParams.szOperatorNumeric,
            pszNumeric, sizeof(m_NetworkSelectionModeParams.szOperatorNumeric));

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetNetworkSelectionManual() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetNetworkSelectionManual(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetNetworkSelectionManual() - Enter / Exit\r\n");
    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_QUERY_AVAILABLE_NETWORKS
//
RIL_RESULT_CODE CTEBase::CoreQueryAvailableNetworks(REQUEST_DATA& rReqData,
                                                               void* /*pData*/,
                                                               UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryAvailableNetworks() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (!m_cte.IsEPSRegistered() && m_cte.IsRegistered())
    {
        /*
         * Since CS/PS signalling is given higher priority, manual network search may
         * get interrupted by CS/PS signalling from network. In order to get the response in
         * an acceptable time for manual network search, data has to be disabled
         * before starting the manual network search.
         *
         * Note: Deactivation of data calls done only if the device is not registered to LTE.
         * Note: Deactivation of data calls is not needed if device is not registered as
         *       no IP packets will interrupt the scan procedure.
         */
        DeactivateAllDataCalls();
    }

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+COPS=?\r"))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreQueryAvailableNetworks() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseQueryAvailableNetworks(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryAvailableNetworks() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    UINT32 nValue;
    UINT32 nEntries = 0;
    UINT32 nCurrent = 0;
    UINT32 numericNameNumber = 0;
    BOOL find = false;
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 k = 0;
    UINT32 shortNameLength = 0;

    char* pszShortOpName = NULL;

    P_ND_OPINFO_PTRS pOpInfoPtr = NULL;
    P_ND_OPINFO_DATA pOpInfoData = NULL;

    void* pOpInfoPtrBase = NULL;
    void* pOpInfoDataBase = NULL;

    P_ND_OPINFO_PTRS pOpInfoPtrEnd = NULL;
    P_ND_OPINFO_DATA pOpInfoDataEnd = NULL;

    void* pOpInfoPtrBaseEnd = NULL;
    void* pOpInfoDataBaseEnd = NULL;

    const char* szRsp = rRspData.szResponse;
    const char* szDummy = NULL;

    // Skip "<prefix>+COPS: "
    SkipRspStart(szRsp, m_szNewLine, szRsp);

    if (!FindAndSkipString(szRsp, "+COPS: ", szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                " Unable to find +COPS: in response\r\n");
        goto Error;
    }

    szDummy = szRsp;

    while (FindAndSkipString(szDummy, "(", szDummy))
    {
        nEntries++;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryAvailableNetworks() -"
            " DEBUG: Found %d entries. Allocating memory...\r\n", nEntries);

    rRspData.pData = malloc(nEntries * (sizeof(S_ND_OPINFO_PTRS) + sizeof(S_ND_OPINFO_DATA)));
    if (NULL == rRspData.pData)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                " Cannot allocate rRspData.pData  size=%d\r\n",
                nEntries * (sizeof(S_ND_OPINFO_PTRS) + sizeof(S_ND_OPINFO_DATA)) );
        goto Error;
    }
    memset(rRspData.pData, 0, nEntries * (sizeof(S_ND_OPINFO_PTRS) + sizeof(S_ND_OPINFO_DATA)));
    rRspData.uiDataSize = nEntries * sizeof(S_ND_OPINFO_PTRS);

    pOpInfoPtrBase = rRspData.pData;
    pOpInfoDataBase = ((char*)rRspData.pData + (nEntries * sizeof(S_ND_OPINFO_PTRS)));

    pOpInfoPtr = (P_ND_OPINFO_PTRS)pOpInfoPtrBase;
    pOpInfoData = (P_ND_OPINFO_DATA)pOpInfoDataBase;

    // Skip "("
    while (SkipString(szRsp, "(", szRsp))
    {
        // Extract "<stat>"
        if (!ExtractUInt32(szRsp, nValue, szRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                    " Unable to extract status\r\n");
            goto Error;
        }

        switch(nValue)
        {
            case 0:
            {
                const char* szTemp = "unknown";
                strncpy(pOpInfoData[nCurrent].szOpInfoStatus, szTemp, MAX_OP_NAME_STATUS-1);
                pOpInfoData[nCurrent].szOpInfoStatus[MAX_OP_NAME_STATUS-1] = '\0';  //  KW fix
                pOpInfoPtr[nCurrent].pszOpInfoStatus = pOpInfoData[nCurrent].szOpInfoStatus;
                break;
            }

            case 1:
            {
                const char* szTemp = "available";
                strncpy(pOpInfoData[nCurrent].szOpInfoStatus, szTemp, MAX_OP_NAME_STATUS-1);
                pOpInfoData[nCurrent].szOpInfoStatus[MAX_OP_NAME_STATUS-1] = '\0';  //  KW fix
                pOpInfoPtr[nCurrent].pszOpInfoStatus = pOpInfoData[nCurrent].szOpInfoStatus;
                break;
            }

            case 2:
            {
                const char* szTemp = "current";
                strncpy(pOpInfoData[nCurrent].szOpInfoStatus, szTemp, MAX_OP_NAME_STATUS-1);
                pOpInfoData[nCurrent].szOpInfoStatus[MAX_OP_NAME_STATUS-1] = '\0';  //  KW fix
                pOpInfoPtr[nCurrent].pszOpInfoStatus = pOpInfoData[nCurrent].szOpInfoStatus;
                break;
            }

            case 3:
            {
                const char* szTemp = "forbidden";
                strncpy(pOpInfoData[nCurrent].szOpInfoStatus, szTemp, MAX_OP_NAME_STATUS-1);
                pOpInfoData[nCurrent].szOpInfoStatus[MAX_OP_NAME_STATUS-1] = '\0';  //  KW fix
                pOpInfoPtr[nCurrent].pszOpInfoStatus = pOpInfoData[nCurrent].szOpInfoStatus;
                break;
            }

            default:
            {
                RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                        " Invalid status found: %d\r\n", nValue);
                goto Error;
            }
        }

        // Extract ",<long_name>"
        if (!SkipString(szRsp, ",", szRsp)
                || !ExtractQuotedString(szRsp, pOpInfoData[nCurrent].szOpInfoLong,
                    MAX_OP_NAME_LONG, szRsp))
        {
            pOpInfoData[nCurrent].szOpInfoLong[0] = '\0';

            RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                    " Could not extract the Long Format Operator Name.\r\n");
            goto Error;
        }
        else
        {
            pOpInfoPtr[nCurrent].pszOpInfoLong = pOpInfoData[nCurrent].szOpInfoLong;
            RIL_LOG_INFO("CTEBase::ParseQueryAvailableNetworks() - Long oper: %s\r\n",
                    pOpInfoData[nCurrent].szOpInfoLong);
        }
        // Extract ",<short_name>"
        if (!SkipString(szRsp, ",", szRsp)
                || !ExtractQuotedStringWithAllocatedMemory(szRsp, pszShortOpName,
                    shortNameLength, szRsp))
        {
            pOpInfoData[nCurrent].szOpInfoShort[0] = '\0';

            RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                    " Could not extract the Short Format Operator Name.\r\n");
            goto Error;
        }
        else
        {
            strncpy(pOpInfoData[nCurrent].szOpInfoShort, pszShortOpName, MAX_OP_NAME_SHORT-1);
            pOpInfoData[nCurrent].szOpInfoShort[MAX_OP_NAME_SHORT - 1] = '\0';

            pOpInfoPtr[nCurrent].pszOpInfoShort = pOpInfoData[nCurrent].szOpInfoShort;
            RIL_LOG_INFO("CTEBase::ParseQueryAvailableNetworks() - Short oper: %s\r\n",
                    pOpInfoData[nCurrent].szOpInfoShort);
       }

       // delete here pszShortOpName is no longer used
       delete[] pszShortOpName;
       pszShortOpName = NULL;

        // Extract ",<num_name>"
        if (!SkipString(szRsp, ",", szRsp)
                || !ExtractQuotedString(szRsp, pOpInfoData[nCurrent].szOpInfoNumeric,
                    MAX_OP_NAME_NUM, szRsp))
        {
            pOpInfoData[nCurrent].szOpInfoNumeric[0] = '\0';

            RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                    " Could not extract the Numeric Format Operator Name.\r\n");
            goto Error;
        }
        else
        {
            pOpInfoPtr[nCurrent].pszOpInfoNumeric = pOpInfoData[nCurrent].szOpInfoNumeric;
            RIL_LOG_INFO("CTEBase::ParseQueryAvailableNetworks() - Numeric oper: %s\r\n",
                    pOpInfoData[nCurrent].szOpInfoNumeric);
       }

        // Extract ")"
        if (!FindAndSkipString(szRsp, ")", szRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                    " Did not find closing bracket\r\n");
            goto Error;
        }

        // Increment the array index
        nCurrent++;

        // Extract ","
        if (!FindAndSkipString(szRsp, ",", szRsp))
        {
            RIL_LOG_INFO("CTEBase::ParseQueryAvailableNetworks() -"
                    " INFO: Finished parsing %d entries\r\n", nCurrent);

            // As memory was allocated using the 'find the (' method, it can be bigger than the
            // number of PLMNs if the latter have '(' in their name so fix uiDataSize to match
            // the number of filled entries in the array.
            rRspData.uiDataSize = nCurrent * sizeof(S_ND_OPINFO_PTRS);

            // Suppression of duplication.

            // Detection of the network number
            if (nCurrent > 0)
            {
                for (i = 0; i < nCurrent; i++)
                {
                    find = false;
                    for (j = 0; j < i; j++)
                    {
                        if (strcmp(pOpInfoData[i].szOpInfoNumeric,
                                pOpInfoData[j].szOpInfoNumeric) == 0)
                        {
                            find = true;
                        }
                    }
                    if (!find)
                    {
                        numericNameNumber++;
                    }
                }
            }

            RIL_LOG_INFO("CTEBase::ParseQueryAvailableNetworks() -"
                    " There is %d network and %d double.\r\n",
                    numericNameNumber, (nCurrent - numericNameNumber));

            if (numericNameNumber < nCurrent)
            {
                RIL_LOG_INFO("CTEBase::ParseQueryAvailableNetworks() - a new table is build.\r\n");

                // Declaration of the final table.
                rRspData.pData = malloc(
                        numericNameNumber * (sizeof(S_ND_OPINFO_PTRS) + sizeof(S_ND_OPINFO_DATA)));
                if (NULL == rRspData.pData)
                {
                    RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                            " Cannot allocate rRspData.pData  size=%d\r\n",
                            numericNameNumber * (sizeof(S_ND_OPINFO_PTRS) +
                                sizeof(S_ND_OPINFO_DATA)) );
                    goto Error;
                }
                memset(rRspData.pData, 0, numericNameNumber *
                        (sizeof(S_ND_OPINFO_PTRS) + sizeof(S_ND_OPINFO_DATA)));
                rRspData.uiDataSize = numericNameNumber * sizeof(S_ND_OPINFO_PTRS);

                pOpInfoPtrBaseEnd = rRspData.pData;
                pOpInfoDataBaseEnd = ((char*)rRspData.pData +
                        (numericNameNumber * sizeof(S_ND_OPINFO_PTRS)));

                pOpInfoPtrEnd = (P_ND_OPINFO_PTRS)pOpInfoPtrBaseEnd;
                pOpInfoDataEnd = (P_ND_OPINFO_DATA)pOpInfoDataBaseEnd;

                // Fill first element of the table.
                strncpy(pOpInfoDataEnd[0].szOpInfoLong,
                        pOpInfoData[0].szOpInfoLong, MAX_OP_NAME_LONG-1);
                pOpInfoDataEnd[0].szOpInfoLong[MAX_OP_NAME_LONG-1] = '\0';  //  KW fix
                strncpy(pOpInfoDataEnd[0].szOpInfoShort,
                        pOpInfoData[0].szOpInfoShort, MAX_OP_NAME_SHORT-1);
                pOpInfoDataEnd[0].szOpInfoShort[MAX_OP_NAME_SHORT-1] = '\0';  //  KW fix
                strncpy(pOpInfoDataEnd[0].szOpInfoNumeric,
                        pOpInfoData[0].szOpInfoNumeric, MAX_OP_NAME_NUM-1);
                pOpInfoDataEnd[0].szOpInfoNumeric[MAX_OP_NAME_NUM-1] = '\0';  //  KW fix
                strncpy(pOpInfoDataEnd[0].szOpInfoStatus,
                        pOpInfoData[0].szOpInfoStatus, MAX_OP_NAME_STATUS-1);
                pOpInfoDataEnd[0].szOpInfoStatus[MAX_OP_NAME_STATUS-1] = '\0';  //  KW fix
                pOpInfoPtrEnd[0].pszOpInfoLong = pOpInfoDataEnd[0].szOpInfoLong;
                pOpInfoPtrEnd[0].pszOpInfoShort = pOpInfoDataEnd[0].szOpInfoShort;
                pOpInfoPtrEnd[0].pszOpInfoNumeric = pOpInfoDataEnd[0].szOpInfoNumeric;
                pOpInfoPtrEnd[0].pszOpInfoStatus = pOpInfoDataEnd[0].szOpInfoStatus;

                // Fill the rest of the table.
                find = false;
                j = 1;
                for (i=1;j<numericNameNumber;i++)
                {
                    for (k=0;k<j;k++)
                    {
                        if (strcmp(pOpInfoDataEnd[k].szOpInfoNumeric,
                                pOpInfoData[i].szOpInfoNumeric) == 0)
                        {
                            find = true;
                        }
                    }
                    if (find == false)
                    {
                        strncpy(pOpInfoDataEnd[j].szOpInfoLong,
                                pOpInfoData[i].szOpInfoLong, MAX_OP_NAME_LONG-1);
                        pOpInfoDataEnd[j].szOpInfoLong[MAX_OP_NAME_LONG-1] = '\0';
                        strncpy(pOpInfoDataEnd[j].szOpInfoShort,
                                pOpInfoData[i].szOpInfoShort, MAX_OP_NAME_SHORT-1);
                        pOpInfoDataEnd[j].szOpInfoShort[MAX_OP_NAME_SHORT-1] = '\0';
                        strncpy(pOpInfoDataEnd[j].szOpInfoNumeric,
                                pOpInfoData[i].szOpInfoNumeric, MAX_OP_NAME_NUM-1);
                        pOpInfoDataEnd[j].szOpInfoNumeric[MAX_OP_NAME_NUM-1] = '\0';  //  KW fix
                        strncpy(pOpInfoDataEnd[j].szOpInfoStatus,
                                pOpInfoData[i].szOpInfoStatus, MAX_OP_NAME_STATUS-1);
                        pOpInfoDataEnd[j].szOpInfoStatus[MAX_OP_NAME_STATUS-1] = '\0';  //  KW fix
                        pOpInfoPtrEnd[j].pszOpInfoLong = pOpInfoDataEnd[j].szOpInfoLong;
                        pOpInfoPtrEnd[j].pszOpInfoShort = pOpInfoDataEnd[j].szOpInfoShort;
                        pOpInfoPtrEnd[j].pszOpInfoNumeric = pOpInfoDataEnd[j].szOpInfoNumeric;
                        pOpInfoPtrEnd[j].pszOpInfoStatus = pOpInfoDataEnd[j].szOpInfoStatus;
                        j++;
                    }
                    find = false;
                }

                for (i=0;i<numericNameNumber;i++)
                {
                    RIL_LOG_INFO("CTEBase::ParseQueryAvailableNetworks() -"
                            " INFO: new table : %s\r\n", pOpInfoDataEnd[i].szOpInfoLong);
                }

                // Free the old table.
                free(pOpInfoPtrBase);
            }
            else
            {
                RIL_LOG_CRITICAL("CTEBase::ParseQueryAvailableNetworks() -"
                        " INFO: a new table is not build.\r\n");
            }

            break;
        }
    }

    // NOTE: there may be more data here, but we don't care about it

    // Find "<postfix>"
    if (!FindAndSkipRspEnd(szRsp, m_szNewLine, szRsp))
    {
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(rRspData.pData);
        rRspData.pData   = NULL;
        rRspData.uiDataSize  = 0;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryAvailableNetworks() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_DTMF_START
//
RIL_RESULT_CODE CTEBase::CoreDtmfStart(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreDtmfStart() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char cTone;

    if (sizeof(char*) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDtmfStart() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDtmfStart() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    cTone = ((char*)pData)[0];

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+XVTS=%c\r", cTone))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreDtmfStart() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseDtmfStart(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseDtmfStart() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseDtmfStart() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_DTMF_STOP
//
RIL_RESULT_CODE CTEBase::CoreDtmfStop(REQUEST_DATA& rReqData,
                                              void* /*pData*/,
                                              UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreDtmfStop() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+XVTS\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreDtmfStop() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseDtmfStop(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseDtmfStop() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseDtmfStop() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_BASEBAND_VERSION
//
RIL_RESULT_CODE CTEBase::CoreBasebandVersion(REQUEST_DATA& rReqData,
                                                        void* /*pData*/,
                                                        UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreBasebandVersion() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "at@vers:sw_version()\r",
            sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreBasebandVersion() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseBasebandVersion(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseBasebandVersion() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    char szTemp[MAX_BUFFER_SIZE] = {0};
    char* szBasebandVersion = (char*)malloc(PROPERTY_VALUE_MAX);
    if (NULL == szBasebandVersion)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseBasebandVersion() -"
                " Could not allocate memory for a %u-char string.\r\n", PROPERTY_VALUE_MAX);
        goto Error;
    }
    memset(szBasebandVersion, 0x00, PROPERTY_VALUE_MAX);

    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseBasebandVersion() - Could not find response start\r\n");
        goto Error;
    }

    // There is two NewLine before the sw_version, so do again a SkipRspStart
    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    if (!ExtractUnquotedString(pszRsp, m_cTerminator, szTemp, MAX_BUFFER_SIZE, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseBasebandVersion() -"
                " Could not extract the baseband version string.\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(szBasebandVersion, PROPERTY_VALUE_MAX, "%s", szTemp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseBasebandVersion() -"
                " Could not create szBasebandVersion\r\n");
        goto Error;
    }

    if (strlen(szBasebandVersion) <= 0)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseBasebandVersion() - Invalid baseband version string.\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTEBase::ParseBasebandVersion() - szBasebandVersion=[%s]\r\n",
            szBasebandVersion);

    rRspData.pData   = (void*)szBasebandVersion;
    rRspData.uiDataSize  = sizeof(char*);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(szBasebandVersion);
        szBasebandVersion = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseBasebandVersion() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SEPARATE_CONNECTION
//
RIL_RESULT_CODE CTEBase::CoreSeparateConnection(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSeparateConnection() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int callId = 0;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSeparateConnection() - Data pointer is NULL.\r\n");
        goto Error;
    }

    callId = ((int *)pData)[0];

    if (E_DTMF_STATE_START == m_cte.GetDtmfState())
    {
        CEvent::Reset(m_pDtmfStopReqEvent);
        HandleInternalDtmfStopReq();
        CEvent::Wait(m_pDtmfStopReqEvent, WAIT_TIMEOUT_DTMF_STOP);
    }

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CHLD=2%u\r", callId))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSeparateConnection() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSeparateConnection(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSeparateConnection() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseSeparateConnection() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_MUTE
//
RIL_RESULT_CODE CTEBase::CoreSetMute(REQUEST_DATA& rReqData, void* pData, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetMute() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int nEnable = 0;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetMute() - Data pointer is NULL.\r\n");
        goto Error;
    }

    nEnable = ((int *)pData)[0];

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CMUT=%d\r",
            (nEnable ? 1 : 0) ))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetMute() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetMute(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetMute() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseSetMute() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_MUTE
//
RIL_RESULT_CODE CTEBase::CoreGetMute(REQUEST_DATA& rReqData, void* /*pData*/, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetMute() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CMUT?\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreGetMute() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGetMute(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetMute() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    int* pMuteVal = NULL;
    UINT32 nValue = 0;

    pMuteVal = (int*)malloc(sizeof(int));
    if (!pMuteVal)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetMute() - Can't allocate an int.\r\n");
        goto Error;
    }
    memset(pMuteVal, 0, sizeof(int));


    // Parse "<prefix>+CMUT: <enabled><postfix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp)                         ||
        !SkipString(pszRsp, "+CMUT: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetMute() - Can't parse prefix.\r\n");
        goto Error;
    }

    if (!ExtractUpperBoundedUInt32(pszRsp, 2, nValue, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetMute() - Can't parse nValue.\r\n");
        goto Error;
    }
    *pMuteVal = nValue;

    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetMute() - Can't parse postfix.\r\n");
        goto Error;
    }

    rRspData.pData = (void*)pMuteVal;
    rRspData.uiDataSize = sizeof(int);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pMuteVal);
        pMuteVal = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetMute() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_QUERY_CLIP
//
RIL_RESULT_CODE CTEBase::CoreQueryClip(REQUEST_DATA& rReqData,
                                               void* /*pData*/,
                                               UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryClip() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+CLIP?\r", sizeof(rReqData.szCmd1)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryClip() - Unable to write command to buffer\r\n");
        goto Error;
    }

    if (!CopyStringNullTerminate(rReqData.szCmd2, "AT+CEER\r", sizeof(rReqData.szCmd2)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryClip() - Cannot create CEER command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreQueryClip() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseQueryClip(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryClip() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    int* pClipVal = NULL;
    UINT32 nValue = 0;
    UINT32 uiCause;

    //  Could have +CEER response here, if AT command returned CME error.
    if (ParseCEER(rRspData, uiCause))
    {
        return (279 == uiCause) ? RRIL_RESULT_FDN_FAILURE : RRIL_RESULT_ERROR;
    }

    pClipVal = (int*)malloc(sizeof(int));
    if (!pClipVal)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryClip() - Can't allocate an int.\r\n");
        goto Error;
    }
    memset(pClipVal, 0, sizeof(int));


    // Parse "<prefix>+CLIP: <n>,<value><postfix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp)                         ||
        !SkipString(pszRsp, "+CLIP: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryClip() - Can't parse prefix.\r\n");
        goto Error;
    }

    if (!ExtractUInt32(pszRsp, nValue, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryClip() - Can't parse nValue1.\r\n");
        goto Error;
    }

    if (SkipString(pszRsp, ",", pszRsp))
    {
        if (!ExtractUInt32(pszRsp, nValue, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQueryClip() - Can't parse nValue2.\r\n");
            goto Error;
        }
    }
    *pClipVal = nValue;

    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryClip() - Can't parse postfix.\r\n");
        goto Error;
    }

    rRspData.pData = (void*)pClipVal;
    rRspData.uiDataSize = sizeof(int);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pClipVal);
        pClipVal = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryClip() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE
//
RIL_RESULT_CODE CTEBase::CoreLastDataCallFailCause(REQUEST_DATA& rReqData,
                                                              void* /*pData*/,
                                                              UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreLastDataCallFailCause() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CEER\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreLastDataCallFailCause() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseLastDataCallFailCause(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseLastDataCallFailCause() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32      uiCause  = 0;
    int*      pCause   = NULL;

    if (!ParseCEER(rRspData, uiCause))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseLastDataCallFailCause() - Parsing of CEER failed\r\n");
        goto Error;
    }

    pCause= (int*) malloc(sizeof(int));
    if (NULL == pCause)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseLastDataCallFailCause() -"
                " Could not allocate memory for an integer.\r\n");
        goto Error;
    }

    //@TODO: cause code mapping needs to be revisited
    *pCause = MapErrorCodeToRilDataFailCause(uiCause);

    rRspData.pData    = (void*) pCause;
    rRspData.uiDataSize   = sizeof(int);

    RIL_LOG_INFO("CTEBase::ParseLastDataCallFailCause() - Last call fail cause [%d]\r\n", uiCause);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pCause);
        pCause = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseLastDataCallFailCause() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseReadContextParams(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseReadContextParams() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char * pszRsp = rRspData.szResponse;

    UINT32 uiPrevCID = 0;
    UINT32 uiCID = 0;
    UINT32 uiBearerID = 0;
    char szTmpBuffer[MAX_BUFFER_SIZE] = {'\0'};
    char szTempAddress1[MAX_IPADDR_SIZE] = {'\0'};
    char szTempAddress2[MAX_IPADDR_SIZE] = {'\0'};
    char szTempAddress3[MAX_IPADDR_SIZE] = {'\0'};
    char szTempAddress4[MAX_IPADDR_SIZE] = {'\0'};
    bool isIPV4 = false;
    bool isIPV6 = false;
    CChannel_Data *pChannelData = NULL;

    RIL_LOG_VERBOSE("CTEBase::ParseReadContextParams() - %s\r\n", pszRsp);

    // Parse +CGCONTRDP response.
    // If the MT has dual stack capabilities, at least one pair of lines with information
    // is returned per <cid>. First one line with the IPv4 parameters followed by one line
    // with the IPv6 parameters. If this MT with dual stack capabilities indicates more
    // than two IP addresses of P-CSCF servers or more than two IP addresses of DNS servers,
    // multiple of such pairs of lines are returned.
    while (FindAndSkipString(pszRsp, "+CGCONTRDP:", pszRsp))
    {
       // Parse <cid>
        if (!ExtractUInt32(pszRsp, uiCID, pszRsp) ||  ( 0 == uiCID ))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                    "Could not extract CID.\r\n");
            goto Error;
        }

        //  Grab the pChannelData for this CID.
        pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
        if (NULL == pChannelData)
        {
            goto Error;
        }

        // Parse ,<bearer_id>
        // not used yet
        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiBearerID, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - Could not extract"
                             " Bearer id.\r\n");
            goto Error;
        }

        // Skip over ,<APN>
        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractQuotedString(pszRsp, szTmpBuffer, MAX_BUFFER_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                    "Could not extract APN.\r\n");
            goto Error;
        }

        if (szTmpBuffer[0] != '\0')
        {
            RIL_LOG_INFO("CTEBase::ParseReadContextParams() - "
                    "Set APN: %s for context Id: %u\r\n", szTmpBuffer, uiCID);
            pChannelData->SetApn(szTmpBuffer);
        }

        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractQuotedString(pszRsp, szTmpBuffer, MAX_BUFFER_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - Could not extract"
                     " source address.\r\n");
            goto Error;
        }

        // Clean-up the context for each cid, only once per CID and per parsing.
        if (uiCID != uiPrevCID)
        {
            pChannelData->DeleteAddressesString(pChannelData->ADDR_IP);
            pChannelData->DeleteAddressesString(pChannelData->ADDR_GATEWAY);
            pChannelData->DeleteAddressesString(pChannelData->ADDR_DNS);
            pChannelData->DeleteAddressesString(pChannelData->ADDR_PCSCF);
        }
        uiPrevCID = uiCID;

        /*
         * If IPv4, then first line will have the IPv4 address.
         * If IPv6, then first line will have the IPv6 address.
         * If IPv4v6, then first line will have the IPv4 address and
         * second line will have the IPv6 address.
         */
        if (!ExtractLocalAddressAndSubnetMask(szTmpBuffer, szTempAddress1, MAX_IPADDR_SIZE,
                szTempAddress2, MAX_IPADDR_SIZE, szTempAddress3, MAX_IPADDR_SIZE,
                szTempAddress4, MAX_IPADDR_SIZE))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                    "ExtractLocalAddressAndSubnetMask failed\r\n");
            goto Error;
        }

        pChannelData->AddAddressString(pChannelData->ADDR_IP, szTempAddress1);
        pChannelData->AddAddressString(pChannelData->ADDR_IP, szTempAddress2);

        if (szTempAddress2[0] == '\0')
        {
            isIPV4 = true;
        }
        if (szTempAddress1[0] == '\0')
        {
            isIPV6 = true;
        }

        if (!SkipString(pszRsp, ",", pszRsp) ||
                !ExtractQuotedString(pszRsp, szTmpBuffer, MAX_BUFFER_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                    "Could not extract Gateway.\r\n");
            goto Error;
        }

        if (!ConvertIPAddressToAndroidReadable(szTmpBuffer, szTempAddress1, MAX_IPADDR_SIZE,
                szTempAddress2, MAX_IPADDR_SIZE))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                    "ConvertIPAddressToAndroidReadable - Ipv4/v6 Gateway address failed\r\n");
            goto Error;
        }

        pChannelData->AddAddressString(pChannelData->ADDR_GATEWAY, szTempAddress1);
        pChannelData->AddAddressString(pChannelData->ADDR_GATEWAY, szTempAddress2);

        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractQuotedString(pszRsp, szTmpBuffer, MAX_BUFFER_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - Could not extract"
                             " Primary DNS.\r\n");
            goto Error;
        }

        /*
         * If IPv4, then first line will have the IPv4 address.
         * If IPv6, then first line will have the IPv6 address.
         * If IPv4v6, then first line will have the IPv4 address and
         * second line will have the IPv6 address.
         */
        if (!ConvertIPAddressToAndroidReadable(szTmpBuffer, szTempAddress1,
                MAX_IPADDR_SIZE, szTempAddress2, MAX_IPADDR_SIZE))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                    "ConvertIPAddressToAndroidReadable - Primary DNS IPv4/IPv6 "
                    "conversion failed\r\n");

            goto Error;
        }

        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractQuotedString(pszRsp, szTmpBuffer, MAX_BUFFER_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - Could not extract "
                             "Secondary DNS.\r\n");
        }
        else
        {
            /*
             * If IPv4, then first line will have the IPv4 address.
             * If IPv6, then first line will have the IPv6 address.
             * If IPv4v6, then first line will have the IPv4 address and
             * second line will have the IPv6 address.
             */
            if (!ConvertIPAddressToAndroidReadable(szTmpBuffer, szTempAddress3, MAX_IPADDR_SIZE,
                    szTempAddress4, MAX_IPADDR_SIZE))
            {
                RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                        "ConvertIPAddressToAndroidReadable - Secondary DNS IPv4/IPv6 "
                        "conversion failed\r\n");

                goto Error;
            }
        }

        pChannelData->AddAddressString(pChannelData->ADDR_DNS, szTempAddress1);
        pChannelData->AddAddressString(pChannelData->ADDR_DNS, szTempAddress3);
        pChannelData->AddAddressString(pChannelData->ADDR_DNS, szTempAddress2);
        pChannelData->AddAddressString(pChannelData->ADDR_DNS, szTempAddress4);

        // Parse ,<P-CSCF_prim_addr>
        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractQuotedString(pszRsp, szTmpBuffer, MAX_IPADDR_SIZE, pszRsp))
        {
            RIL_LOG_INFO("CTEBase::ParseReadContextParams() - "
                    "Could not extract P-CSCF primary address.\r\n");
        }
        else
        {
            /*
             * If IPv4, then first line will have the IPv4 address.
             * If IPv6, then first line will have the IPv6 address.
             * If IPv4v6, then first line will have the IPv4 address and
             * second line will have the IPv6 address.
             */
            if (!ConvertIPAddressToAndroidReadable(szTmpBuffer, szTempAddress1, MAX_IPADDR_SIZE,
                    szTempAddress2, MAX_IPADDR_SIZE))
            {
                RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                        "ConvertIPAddressToAndroidReadable - Primary PCSCF IPv4/IPv6 "
                        "conversion failed\r\n");

                goto Error;
            }
        }

        // Parse ,<P-CSCF_sec_addr>
        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractQuotedString(pszRsp, szTmpBuffer, MAX_IPADDR_SIZE, pszRsp))
        {
            RIL_LOG_INFO("CTEBase::ParseReadContextParams() - Could not extract "
                    "P-CSCF sec addr.\r\n");
        }
        else
        {
            /*
             * If IPv4, then first line will have the IPv4 address.
             * If IPv6, then first line will have the IPv6 address.
             * If IPv4v6, then first line will have the IPv4 address and
             * second line will have the IPv6 address.
             */
            if (!ConvertIPAddressToAndroidReadable(szTmpBuffer, szTempAddress3, MAX_IPADDR_SIZE,
                    szTempAddress4, MAX_IPADDR_SIZE))
            {
                RIL_LOG_CRITICAL("CTEBase::ParseReadContextParams() - "
                        "ConvertIPAddressToAndroidReadable - Secondary PCSCF IPv4/IPv6 "
                        "conversion failed\r\n");

                goto Error;
            }
        }

        pChannelData->AddAddressString(pChannelData->ADDR_PCSCF, szTempAddress1);
        pChannelData->AddAddressString(pChannelData->ADDR_PCSCF, szTempAddress3);
        pChannelData->AddAddressString(pChannelData->ADDR_PCSCF, szTempAddress2);
        pChannelData->AddAddressString(pChannelData->ADDR_PCSCF, szTempAddress4);

        // Clear temp buffers for next line parsing
        szTempAddress1[0] = '\0';
        szTempAddress2[0] = '\0';
        szTempAddress3[0] = '\0';
        szTempAddress4[0] = '\0';

    }

    res = RRIL_RESULT_OK;
Error:
    if (RRIL_RESULT_OK == res)
    {
        if (NULL != pChannelData)
        {
            if (isIPV4 && !isIPV6)
            {
                pChannelData->SetPdpType(PDPTYPE_IP);
            }
            else if (!isIPV4 && isIPV6)
            {
                pChannelData->SetPdpType(PDPTYPE_IPV6);
            }
            else if (isIPV4 && isIPV6)
            {
                pChannelData->SetPdpType(PDPTYPE_IPV4V6);
            }

            NotifyNetworkApnInfo();
        }
    }

    RIL_LOG_VERBOSE("CTEBase::ParseReadContextParams() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseReadBearerTFTParams(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseReadBearerTFTParams() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    char szTmpBuffer[MAX_BUFFER_SIZE] = {'\0'};
    CChannel_Data* pChannelData = NULL;
    sOEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS* pTFTParams = NULL;
    sTFT_PARAM* pTFTParam = NULL;
    int index = 0;

    RIL_LOG_VERBOSE("CTEBase::ParseReadBearerTFTParams() - %s\r\n", pszRsp);

    pTFTParams = (sOEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS*)
            malloc(sizeof(sOEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS));

    if (NULL == pTFTParams)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                "memory allocation failed\r\n");
        goto Error;
    }

    memset(pTFTParams, 0 , sizeof(sOEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS));

    pTFTParams->command = RIL_OEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS;

    pChannelData = (CChannel_Data*) rRspData.pContextData;
    pTFTParams->uiPcid = pChannelData->GetContextID();

    pChannelData->GetInterfaceName(pTFTParams->szIfName,MAX_INTERFACE_NAME_SIZE);
    if (0 == strlen(pTFTParams->szIfName))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - No Interface"
                " found for PCID=[%u]\r\n", pTFTParams->uiPcid);
        goto Error;
    }

    // Parse +CGTFTRDP response, will return up to 2 lines of data (if MT has
    // dual stack capability. 1st line for IPV4 data, 2nd for IPV6
    while (FindAndSkipString(pszRsp, "+CGTFTRDP:", pszRsp) && index < MAX_TFT_PARAMS)
    {
        pTFTParam = &pTFTParams->params[index++];

        // Parse <cid>
        if (!ExtractUInt32(pszRsp, pTFTParam->uiCid, pszRsp)
                || 0 == pTFTParam->uiCid )
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract CID.\r\n");
            goto Error;
        }

        // Parse <packet filter identifier>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pTFTParam->uiPacketFilterIdentifier, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - Could not extract"
                    " packet filter identifier.\r\n");
            goto Error;
        }

        // Parse <evaluation precedence index>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pTFTParam->uiEvaluationPrecedenceIndex, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract evaluation precedence index.\r\n");
            goto Error;
        }

        // Parse <source address and subnet mask>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractQuotedString(pszRsp, szTmpBuffer, MAX_BUFFER_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract source address and subnet mask.\r\n");
            goto Error;
        }

        /*
        * If IPv4, then first line will have the IPv4 address.
        * If IPv6, then first line will have the IPv6 address.
        * If IPv4v6, then first line will have the IPv4 address and
        * second line will have the IPv6 address.
        */
        if (!ExtractLocalAddressAndSubnetMask(szTmpBuffer, pTFTParam->szSourceIpV4Addr,
                MAX_IPADDR_SIZE, pTFTParam->szSourceIpV6Addr, MAX_IPADDR_SIZE,
                pTFTParam->szSourceIpv4SubnetMask, MAX_IPADDR_SIZE,
                pTFTParam->szSourceIpv6SubnetMask, MAX_IPADDR_SIZE))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "ExtractLocalAddressAndSubnetMask failed\r\n");
            goto Error;
        }

        // Parse <protocol number>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pTFTParam->uiProtocolNumber, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract protocol number.\r\n");
            goto Error;
        }

        // Parse <destination port range>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUnquotedString(pszRsp, ",", pTFTParam->szDestinationPortRange,
                        MAX_RANGE_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract destination port range.\r\n");
            goto Error;
        }

        // Parse <source port range>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUnquotedString(pszRsp, ",", pTFTParam->szSourcePortRange,
                        MAX_RANGE_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract source port range.\r\n");
            goto Error;
        }

        // Parse <spi>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractHexUInt32(pszRsp, pTFTParam->uiIpSecParamIndex, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract spi.\r\n");
            goto Error;
        }

        // Parse <tos>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUnquotedString(pszRsp, ",", pTFTParam->szTOS, MAX_RANGE_SIZE, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract tos.\r\n");
            goto Error;
        }

        // Parse <flow label>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractHexUInt32(pszRsp, pTFTParam->uiFlowLabel, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract flow label.\r\n");
            goto Error;
        }

        // Parse <direction>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pTFTParam->uiDirection, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract direction.\r\n");
            goto Error;
        }

        // Parse <NW packet filter identifier>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pTFTParam->uiNwPacketFilterIdentifier, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - "
                    "Could not extract NW packet filter identifier.\r\n");
            goto Error;
        }

        RIL_LOG_INFO("CTEBase::ParseReadBearerTFTParams() - "
                "uiPcid: %u, "
                "uiCID: %u, uiPacketFilterIdentifier: %u, uiEvaluationPrecedenceIndex: %u, "
                "szSourceIpV4Addr: %s, szSourceIpV6Addr: %s, "
                "szSourceIpv4SubnetMask: %s, szSourceIpv6SubnetMask: %s, "
                "uiProtocolNumber: %u, szDestinationPortRange: %s,"
                "szSourcePortRange: %s, uiIpSecParamIndex: %u, szTOS: %s, "
                "uiFlowLabel: %u, uiDirection: %u, uiNwPacketFilterIdentifier: %u\r\n",
                pTFTParams->uiPcid,
                pTFTParam->uiCid, pTFTParam->uiPacketFilterIdentifier,
                pTFTParam->uiEvaluationPrecedenceIndex,
                pTFTParam->szSourceIpV4Addr, pTFTParam->szSourceIpV6Addr,
                pTFTParam->szSourceIpv4SubnetMask, pTFTParam->szSourceIpv6SubnetMask,
                pTFTParam->uiProtocolNumber, pTFTParam->szDestinationPortRange,
                pTFTParam->szSourcePortRange, pTFTParam->uiIpSecParamIndex,
                pTFTParam->szTOS, pTFTParam->uiFlowLabel, pTFTParam->uiDirection,
                pTFTParam->uiNwPacketFilterIdentifier);
    }
    if (index == MAX_TFT_PARAMS)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseReadBearerTFTParams() - too many TFT params lines (%d)",
                index);
        index -= 1;
    }
    pTFTParams->count = index;
    res = RRIL_RESULT_OK;

    RIL_onUnsolicitedResponse(RIL_UNSOL_OEM_HOOK_RAW, (void*)pTFTParams,
            sizeof(sOEM_HOOK_RAW_UNSOL_BEARER_TFT_PARAMS));
Error:
    free(pTFTParams);
    RIL_LOG_VERBOSE("CTEBase::ParseReadBearerTFTParams() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseReadBearerQOSParams(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseReadBearerQOSParams() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char * pszRsp = rRspData.szResponse;

    char szTmpBuffer[MAX_BUFFER_SIZE] = {'\0'};
    CChannel_Data *pChannelData = NULL;
    sOEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS* pQOSParams = NULL;

    RIL_LOG_VERBOSE("CTEBase::ParseReadBearerQOSParams() - %s\r\n", pszRsp);

    pQOSParams = (sOEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS*)
            malloc(sizeof(sOEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS));

    if (NULL == pQOSParams)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - "
                "memory allocation failed\r\n");
        goto Error;
    }

    memset(pQOSParams, 0 , sizeof(sOEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS));

    pQOSParams->command = RIL_OEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS;

    if (!FindAndSkipString(pszRsp, "+CGEQOSRDP:", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - "
                "+CGEQOSRDP not found in response\r\n");
        goto Error;
    }

    pChannelData = (CChannel_Data*) rRspData.pContextData;
    pQOSParams->uiPcid = pChannelData->GetContextID();

    pChannelData->GetInterfaceName(pQOSParams->szIfName, MAX_INTERFACE_NAME_SIZE);
    if (0 == strlen(pQOSParams->szIfName))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - No Interface"
                " found for PCID=[%u]\r\n", pQOSParams->uiPcid);
        goto Error;
    }

    // Parse <cid>
    if (!ExtractUInt32(pszRsp, pQOSParams->uiCid, pszRsp)
            || ( 0 == pQOSParams->uiCid ))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - "
                "Could not extract CID.\r\n");
        goto Error;
    }

    // Parse <qci>
    if (!SkipString(pszRsp, ",", pszRsp)
            || !ExtractUInt32(pszRsp, pQOSParams->uiQci, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - "
                "Could not extract qci.\r\n");
        goto Error;
    }

    // if QCI is in Guaranted bit rate range ( [1 - 4] )
    if (pQOSParams->uiQci >= 1 && pQOSParams->uiQci <= 4)
    {
        // Parse <DL_GBR>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pQOSParams->uiDlGbr, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - Could not extract"
                    " DL_GBR.\r\n");
            goto Error;
        }

        // Parse <UL_GBR>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pQOSParams->uiUlGbr, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - Could not extract"
                    " UL_GBR.\r\n");
            goto Error;
        }

        // Parse <DL_MBR>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pQOSParams->uiDlMbr, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - Could not extract"
                    " DL_MBR.\r\n");
            goto Error;
        }

        // Parse <UL_MBR>
        if (!SkipString(pszRsp, ",", pszRsp)
                || !ExtractUInt32(pszRsp, pQOSParams->uiUlMbr, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseReadBearerQOSParams() - Could not extract"
                    " UL_MBR.\r\n");
            goto Error;
        }
    }

    RIL_LOG_INFO("CTEBase::ParseReadBearerQOSParams() - "
            "uiQci: %u, uiDlGbr: %u, "
            "uiUlGbr: %u, uiDlMbr: %u, "
            "uiUlMbr: %u\r\n",
            pQOSParams->uiQci, pQOSParams->uiDlGbr,
            pQOSParams->uiUlGbr, pQOSParams->uiDlMbr,
            pQOSParams->uiUlMbr);

    res = RRIL_RESULT_OK;

    RIL_onUnsolicitedResponse(RIL_UNSOL_OEM_HOOK_RAW, (void*)pQOSParams,
            sizeof(sOEM_HOOK_RAW_UNSOL_BEARER_QOS_PARAMS));
Error:
    free(pQOSParams);
    RIL_LOG_VERBOSE("CTEBase::ParseReadBearerQOSParams() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_RESET_RADIO
//
RIL_RESULT_CODE CTEBase::CoreResetRadio(REQUEST_DATA& /*rReqData*/,
                                                void* /*pData*/,
                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreResetRadio() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreResetRadio() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseResetRadio(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseResetRadio() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseResetRadio() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_OEM_HOOK_RAW
//
RIL_RESULT_CODE CTEBase::CoreHookRaw(REQUEST_DATA& /*rReqData*/,
                                                void* /*pData*/,
                                                UINT32 /*uiDataSize*/,
                                                UINT32& /*uiRilChannel*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreHookRaw() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreHookRaw() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseHookRaw(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseHookRaw() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseHookRaw() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_OEM_HOOK_STRINGS
//
RIL_RESULT_CODE CTEBase::CoreHookStrings(REQUEST_DATA& /*rReqData*/,
                                                     void* /*pData*/,
                                                     UINT32 /*uiDataSize*/,
                                                     UINT32& /*uiRilChannel*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreHookStrings() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_NOTSUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreHookStrings() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseHookStrings(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseHookStrings() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseHookStrings() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SCREEN_STATE
//
RIL_RESULT_CODE CTEBase::CoreScreenState(REQUEST_DATA& /*rReqData*/,
                                                    void* pData,
                                                    UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreScreenState() - Enter\r\n");

    return HandleScreenStateReq(((int*)pData)[0]);
}

RIL_RESULT_CODE CTEBase::ParseScreenState(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseScreenState() - Enter\r\n");

    //  Extract screen state from context
    int nScreenState = (intptr_t)rRspData.pContextData;

    if (1 == nScreenState)
    {
        /*
         * This will result in quite a few traffic between AP and BP when the screen
         * state is changed frequently.
         */
        if (m_cte.IsSignalStrengthReportEnabled())
        {
            QuerySignalStrength();
        }
        CTE::GetTE().TestAndSetNetworkStateChangeTimerRunning(false);
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED, NULL, 0);
    }

    m_cte.ResetRegistrationCache();

    RIL_LOG_VERBOSE("CTEBase::ParseScreenState() - Exit\r\n");
    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION
//
RIL_RESULT_CODE CTEBase::CoreSetSuppSvcNotification(REQUEST_DATA& rReqData,
                                                               void* pData,
                                                               UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetSuppSvcNotification() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (sizeof(int) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetSuppSvcNotification() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetSuppSvcNotification() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }


    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CSSN=%u,%u\r",
            ((int *)pData)[0], ((int *)pData)[0]))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetSuppSvcNotification() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetSuppSvcNotification(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetSuppSvcNotification() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseSetSuppSvcNotification() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_WRITE_SMS_TO_SIM
//
RIL_RESULT_CODE CTEBase::CoreWriteSmsToSim(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreWriteSmsToSim() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_SMS_WriteArgs* pSmsArgs;
    UINT32 nPDULength = 0;
    UINT32 nSMSCLen = 0;

    if (sizeof(RIL_SMS_WriteArgs) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreWriteSmsToSim() - Passed data size mismatch."
                " Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreWriteSmsToSim() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    pSmsArgs = (RIL_SMS_WriteArgs*)pData;

    // 2 chars per byte.
    nPDULength = (strlen(pSmsArgs->pdu) / 2);

    if ((PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CMGW=%u,%u\r", nPDULength, pSmsArgs->status)) &&
            (PrintStringNullTerminate(rReqData.szCmd2, sizeof(rReqData.szCmd2),
                "%s%s\x1a", (NULL != pSmsArgs->smsc ? pSmsArgs->smsc : "00"), pSmsArgs->pdu)))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreWriteSmsToSim() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseWriteSmsToSim(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseWriteSmsToSim() - Enter\r\n");

    int* pIndex = NULL;
    const char* szRsp = rRspData.szResponse;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    pIndex = (int*)malloc(sizeof(int));
    if (NULL == pIndex)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseWriteSmsToSim() - Unable to allocate memory for int\r\n");
        goto Error;
    }

    if (!FindAndSkipString(szRsp, "+CMGW: ", szRsp) ||
        !ExtractUInt32(szRsp, (UINT32&)*pIndex, szRsp)  ||
        !SkipRspEnd(szRsp, m_szNewLine, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseWriteSmsToSim() -"
                " Could not extract the Message Index.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

    rRspData.pData   = (void*)pIndex;
    rRspData.uiDataSize  = sizeof(int);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pIndex);
        pIndex = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseWriteSmsToSim() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_DELETE_SMS_ON_SIM
//
RIL_RESULT_CODE CTEBase::CoreDeleteSmsOnSim(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreDeleteSmsOnSim() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (sizeof(int) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDeleteSmsOnSim() - Passed data size mismatch."
                " Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreDeleteSmsOnSim() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CMGD=%u\r",
            ((int*)pData)[0]))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreDeleteSmsOnSim() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseDeleteSmsOnSim(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseDeleteSmsOnSim() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseDeleteSmsOnSim() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_BAND_MODE
//
RIL_RESULT_CODE CTEBase::CoreSetBandMode(REQUEST_DATA& /*rReqData*/,
                                                 void* /*pData*/,
                                                 UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetBandMode() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetBandMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetBandMode(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetBandMode() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseSetBandMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE
//
RIL_RESULT_CODE CTEBase::CoreQueryAvailableBandMode(REQUEST_DATA& /*rReqData*/,
                                                               void* /*pData*/,
                                                               UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryAvailableBandMode() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreQueryAvailableBandMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseQueryAvailableBandMode(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryAvailableBandMode() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseQueryAvailableBandMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_GET_PROFILE
//
RIL_RESULT_CODE CTEBase::CoreStkGetProfile(REQUEST_DATA& /*rReqData*/,
                                                   void* /*pData*/,
                                                   UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreStkGetProfile() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreStkGetProfile() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseStkGetProfile(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseStkGetProfile() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseStkGetProfile() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SET_PROFILE
//
RIL_RESULT_CODE CTEBase::CoreStkSetProfile(REQUEST_DATA& /*rReqData*/,
                                                   void* /*pData*/,
                                                   UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreStkSetProfile() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreStkSetProfile() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseStkSetProfile(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseStkSetProfile() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseStkSetProfile() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
//
RIL_RESULT_CODE CTEBase::CoreStkSendEnvelopeCommand(REQUEST_DATA& /*rReqData*/,
                                                               void* /*pData*/,
                                                               UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreStkSendEnvelopeCommand() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreStkSendEnvelopeCommand() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseStkSendEnvelopeCommand(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseStkSendEnvelopeCommand() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseStkSendEnvelopeCommand() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
//
RIL_RESULT_CODE CTEBase::CoreStkSendTerminalResponse(REQUEST_DATA& /*rReqData*/,
                                                                void* /*pData*/,
                                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreStkSendTerminalResponse() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreStkSendTerminalResponse() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseStkSendTerminalResponse(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseStkSendTerminalResponse() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseStkSendTerminalResponse() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM
//
RIL_RESULT_CODE CTEBase::CoreStkHandleCallSetupRequestedFromSim(REQUEST_DATA& /*rReqData*/,
                                                                           void* /*pData*/,
                                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreStkHandleCallSetupRequestedFromSim() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreStkHandleCallSetupRequestedFromSim() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseStkHandleCallSetupRequestedFromSim(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseStkHandleCallSetupRequestedFromSim() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseStkHandleCallSetupRequestedFromSim() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_EXPLICIT_CALL_TRANSFER
//
RIL_RESULT_CODE CTEBase::CoreExplicitCallTransfer(REQUEST_DATA& rReqData, void* /*pData*/,
                                                                    UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreExplicitCallTransfer() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CHLD=4\r", sizeof(rReqData.szCmd1)))
    {
        res = RRIL_RESULT_OK;
    }

    RIL_LOG_VERBOSE("CTEBase::CoreExplicitCallTransfer() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseExplicitCallTransfer(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseExplicitCallTransfer() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseExplicitCallTransfer() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
//
RIL_RESULT_CODE CTEBase::CoreSetPreferredNetworkType(REQUEST_DATA& /*rReqData*/,
                                                                void* /*pData*/,
                                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetPreferredNetworkType() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreSetPreferredNetworkType() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetPreferredNetworkType(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetPreferredNetworkType() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseSetPreferredNetworkType() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
//
RIL_RESULT_CODE CTEBase::CoreGetPreferredNetworkType(REQUEST_DATA& /*rReqData*/,
                                                                void* /*pData*/,
                                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetPreferredNetworkType() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreGetPreferredNetworkType() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGetPreferredNetworkType(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetPreferredNetworkType() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseGetPreferredNetworkType() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_NEIGHBORING_CELL_IDS
//
RIL_RESULT_CODE CTEBase::CoreGetNeighboringCellIDs(REQUEST_DATA& /*rReqData*/,
                                                              void* /*pData*/,
                                                              UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetNeighboringCellIDs() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreGetNeighboringCellIDs() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseNeighboringCellInfo(P_ND_N_CELL_DATA /*pCellData*/,
                                                            const char* /*pszRsp*/,
                                                            UINT32 /*uiIndex*/,
                                                            UINT32 /*uiMode*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseNeighboringCellInfo() - Enter\r\n");

    // this is modem dependent, to be implemented in te_xmmxxx.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseNeighboringCellInfo() - Exit\r\n");
    return res;

}

RIL_RESULT_CODE CTEBase::ParseGetNeighboringCellIDs(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetNeighboringCellIDs() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    UINT32 uiIndex = 0;
    UINT32 uiMode = 0;

    P_ND_N_CELL_DATA pCellData = NULL;

    pCellData = (P_ND_N_CELL_DATA)malloc(sizeof(S_ND_N_CELL_DATA));
    if (NULL == pCellData)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetNeighboringCellIDs() -"
                " Could not allocate memory for a S_ND_N_CELL_DATA struct.\r\n");
        goto Error;
    }
    memset(pCellData, 0, sizeof(S_ND_N_CELL_DATA));


    // Loop on +XCELLINFO until no more entries are found
    while (FindAndSkipString(pszRsp, "+XCELLINFO: ", pszRsp))
    {
        if (RRIL_MAX_CELL_ID_COUNT == uiIndex)
        {
            //  We're full.
            RIL_LOG_CRITICAL("CTEBase::ParseGetNeighboringCellIDs() -"
                    " Exceeded max count = %d\r\n", RRIL_MAX_CELL_ID_COUNT);
            break;
        }

        // Get <mode>
        if (!ExtractUInt32(pszRsp, uiMode, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseGetNeighboringCellIDs() -"
                    " cannot extract <mode>\r\n");
            goto Error;
        }

        RIL_LOG_INFO("CTEBase::ParseGetNeighboringCellIDs() - found mode=%d\r\n",
                uiMode);
        RIL_RESULT_CODE result = RRIL_RESULT_ERROR;
        switch (uiMode)
        {
            // GSM/UMTS/LTE
            case 0:
            case 1:
            case 2:
            case 3:
            case 5:
            case 6:
                result = ParseNeighboringCellInfo(pCellData, pszRsp, uiIndex, uiMode);
                if (result == RRIL_RESULT_OK)
                {
                    // Connect the pointer
                    pCellData->apRilNeighboringCell[uiIndex] =
                            &(pCellData->aRilNeighboringCell[uiIndex]);
                    uiIndex++;
                    RIL_LOG_INFO("CTEBase::ParseGetNeighboringCellIDs() - Index=%d\r\n",
                            uiIndex);
                }
            break;
            default:
            break;
        }
    }

    if (uiIndex > 0)
    {
        rRspData.pData  = (void*)pCellData;
        rRspData.uiDataSize = uiIndex * sizeof(RIL_NeighboringCell*);
    }
    else
    {
        rRspData.pData  = NULL;
        rRspData.uiDataSize = 0;
        free(pCellData);
        pCellData = NULL;
    }

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pCellData);
        pCellData = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetNeighboringCellIDs() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_LOCATION_UPDATES
//
RIL_RESULT_CODE CTEBase::CoreSetLocationUpdates(REQUEST_DATA& rReqData,
                                                void* pData, UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetLocationUpdates() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int enableLocationUpdates = 0;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetLocationUpdates() - Data pointer is NULL.\r\n");
        goto Error;
    }

    enableLocationUpdates = ((int*)pData)[0];

    if (!CopyStringNullTerminate(rReqData.szCmd1,
            GetLocationUpdateString(enableLocationUpdates), sizeof(rReqData.szCmd1)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetLocationUpdates() - Cannot create command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetLocationUpdates() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetLocationUpdates(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetLocationUpdates() - Enter / Exit\r\n");

    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_CDMA_SET_SUBSCRIPTION
//
RIL_RESULT_CODE CTEBase::CoreCdmaSetSubscription(REQUEST_DATA& /*rReqData*/,
                                                            void* /*pData*/,
                                                            UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSetSubscription() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSetSubscription() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaSetSubscription(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSetSubscription() - Enter\r\n");

    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSetSubscription() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE
//
RIL_RESULT_CODE CTEBase::CoreCdmaSetRoamingPreference(REQUEST_DATA& /*rReqData*/,
                                                                 void* /*pData*/,
                                                                 UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSetRoamingPreference() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSetRoamingPreference() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaSetRoamingPreference(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSetRoamingPreference() - Enter\r\n");

    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSetRoamingPreference() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE
//
RIL_RESULT_CODE CTEBase::CoreCdmaQueryRoamingPreference(REQUEST_DATA& /*rReqData*/,
                                                                   void* /*pData*/,
                                                                   UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaQueryRoamingPreference() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaQueryRoamingPreference() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaQueryRoamingPreference(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaQueryRoamingPreference() - Enter\r\n");

    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaQueryRoamingPreference() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_TTY_MODE
//
RIL_RESULT_CODE CTEBase::CoreSetTtyMode(REQUEST_DATA& /*rReqData*/,
                                                void* /*pData*/,
                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetTtyMode() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreSetTtyMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetTtyMode(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetTtyMode() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseSetTtyMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_QUERY_TTY_MODE
//
RIL_RESULT_CODE CTEBase::CoreQueryTtyMode(REQUEST_DATA& /*rReqData*/,
                                                  void* /*pData*/,
                                                  UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryTtyMode() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreQueryTtyMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseQueryTtyMode(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryTtyMode() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseQueryTtyMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE
//
RIL_RESULT_CODE CTEBase::CoreCdmaSetPreferredVoicePrivacyMode(REQUEST_DATA& /*rReqData*/,
                                                                         void* /*pData*/,
                                                                         UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSetPreferredVoicePrivacyMode() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSetPreferredVoicePrivacyMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaSetPreferredVoicePrivacyMode(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSetPreferredVoicePrivacyMode() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSetPreferredVoicePrivacyMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE
//
RIL_RESULT_CODE CTEBase::CoreCdmaQueryPreferredVoicePrivacyMode(REQUEST_DATA& /*rReqData*/,
                                                                           void* /*pData*/,
                                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaQueryPreferredVoicePrivacyMode() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaQueryPreferredVoicePrivacyMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaQueryPreferredVoicePrivacyMode(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaQueryPreferredVoicePrivacyMode() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaQueryPreferredVoicePrivacyMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_FLASH
//
RIL_RESULT_CODE CTEBase::CoreCdmaFlash(REQUEST_DATA& /*rReqData*/,
                                               void* /*pData*/,
                                               UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaFlash() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaFlash() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaFlash(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaFlash() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaFlash() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_BURST_DTMF
//
RIL_RESULT_CODE CTEBase::CoreCdmaBurstDtmf(REQUEST_DATA& /*rReqData*/,
                                                   void* /*pData*/,
                                                   UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaBurstDtmf() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaBurstDtmf() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaBurstDtmf(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaBurstDtmf() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaBurstDtmf() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY
//
RIL_RESULT_CODE CTEBase::CoreCdmaValidateAndWriteAkey(REQUEST_DATA& /*rReqData*/,
                                                                 void* /*pData*/,
                                                                 UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaValidateAndWriteAkey() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaValidateAndWriteAkey() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaValidateAndWriteAkey(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaValidateAndWriteAkey() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaValidateAndWriteAkey() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_SEND_SMS
//
RIL_RESULT_CODE CTEBase::CoreCdmaSendSms(REQUEST_DATA& /*rReqData*/,
                                                 void* /*pData*/,
                                                 UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSendSms() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSendSms() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaSendSms(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSendSms() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSendSms() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE
//
RIL_RESULT_CODE CTEBase::CoreCdmaSmsAcknowledge(REQUEST_DATA& /*rReqData*/,
                                                           void* /*pData*/,
                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSmsAcknowledge() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSmsAcknowledge() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaSmsAcknowledge(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSmsAcknowledge() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSmsAcknowledge() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG
//
RIL_RESULT_CODE CTEBase::CoreGsmGetBroadcastSmsConfig(REQUEST_DATA& rReqData,
                                                                 void* /*pData*/,
                                                                 UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGsmGetBroadcastSmsConfig() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CSCB?\r"))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreGsmGetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGsmGetBroadcastSmsConfig(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGsmGetBroadcastSmsConfig() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    UINT32 nSelected = 0;
    char szChannels[MAX_BUFFER_SIZE] = {0};
    char szLangs[MAX_BUFFER_SIZE] = {0};
    const char* szRsp = rRspData.szResponse;
    const char* pszChannels = NULL;
    const char* pszLangs = NULL;
    P_ND_BROADCASTSMSCONFIGINFO_DATA pBroadcastSmsConfigInfoBlob = NULL;
    UINT32 nCount = 0;
    UINT32 nStructsChannels = 0, nStructsLangs = 0;

    pBroadcastSmsConfigInfoBlob =
            (P_ND_BROADCASTSMSCONFIGINFO_DATA)malloc(sizeof(S_ND_BROADCASTSMSCONFIGINFO_DATA));
    if (NULL == pBroadcastSmsConfigInfoBlob)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGsmGetBroadcastSmsConfig() -"
                " Could not allocate memory for pBroadcastSmsConfigInfoBlob.\r\n");
        goto Error;
    }
    //  Set bytes to 0xFF because 0 values mean something in this structure.
    memset(pBroadcastSmsConfigInfoBlob, 0xFF, sizeof(S_ND_BROADCASTSMSCONFIGINFO_DATA));

    // Parse "<prefix>+CSCB: <mode>,<mids>,<dcss><postfix>"
    if (!SkipRspStart(szRsp, m_szNewLine, szRsp) ||
        !SkipString(szRsp, "+CSCB: ", szRsp) ||
        !ExtractUInt32(szRsp, nSelected, szRsp) ||
        !SkipString(szRsp, ",", szRsp) ||
        !ExtractQuotedString(szRsp, szChannels, MAX_BUFFER_SIZE, szRsp) ||
        !SkipString(szRsp, ",", szRsp) ||
        !ExtractQuotedString(szRsp, szLangs, MAX_BUFFER_SIZE, szRsp) ||
        !SkipRspEnd(szRsp, m_szNewLine, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGsmGetBroadcastSmsConfig() - Could not extract data.\r\n");
        goto Error;
    }

    //  Determine number of structures needed to hold channels and languages.
    //  This is equal to max(nStructsChannels, nStructsLangs).

    //  Count ',' in szChannels, then add one.
    for (UINT32 i=0; i < strlen(szChannels); i++)
    {
        if (',' == szChannels[i])
        {
            nStructsChannels++;
        }
    }
    nStructsChannels++;

    //  Count ',' in szLangs, then add one.
    for (UINT32 i=0; i < strlen(szLangs); i++)
    {
        if (',' == szLangs[i])
        {
            nStructsLangs++;
        }
    }
    nStructsLangs++;

    nCount = ((nStructsChannels > nStructsLangs) ? nStructsChannels : nStructsLangs);
    if (nCount > RIL_MAX_BROADCASTSMSCONFIGINFO_ENTRIES)
    {
        //  Too many.  Error out.
        RIL_LOG_CRITICAL("CTEBase::ParseGsmGetBroadcastSmsConfig() -"
                " Too many structs needed!  nCount=%d.\r\n", nCount);
        goto Error;
    }

    //  Parse through szChannels.
    pszChannels = szChannels;
    for (UINT32 i = 0; (i < nStructsChannels) && (i < RIL_MAX_BROADCASTSMSCONFIGINFO_ENTRIES); i++)
    {
        UINT32 nValue1, nValue2;
        if (!ExtractUInt32(pszChannels, nValue1, pszChannels))
        {
            //  Use -1 as no channels.
            nValue1 = -1;
            nValue2 = -1;
        }
        else
        {
            if (SkipString(pszChannels, "-", pszChannels))
            {
                //  It is a range.
                if (!ExtractUInt32(pszChannels, nValue2, pszChannels))
                {
                    //  Nothing after the "-" is an error.
                    RIL_LOG_CRITICAL("CTEBase::ParseGsmGetBroadcastSmsConfig() -"
                            " Parsing szChannels range. nStructsChannels=%d i=%d\r\n",
                            nStructsChannels, i);
                    goto Error;
                }
            }
            else
            {
                //  Not a range.
                nValue2 = nValue1;
            }
        }

        pBroadcastSmsConfigInfoBlob->aRilGsmBroadcastSmsConfigInfo[i].fromServiceId = nValue1;
        pBroadcastSmsConfigInfoBlob->aRilGsmBroadcastSmsConfigInfo[i].toServiceId = nValue2;
        pBroadcastSmsConfigInfoBlob->apRilGSMBroadcastSmsConfigInfo[i] =
                &(pBroadcastSmsConfigInfoBlob->aRilGsmBroadcastSmsConfigInfo[i]);

        //  Parse the next ",".
        if (!SkipString(pszChannels, ",", pszChannels))
        {
            //  We are done.
            break;
        }
    }

    //  Parse through szLangs.
    pszLangs = szLangs;
    for (UINT32 i = 0; (i < nStructsLangs) && (i < RIL_MAX_BROADCASTSMSCONFIGINFO_ENTRIES); i++)
    {
        UINT32 nValue1, nValue2;
        if (!ExtractUInt32(pszLangs, nValue1, pszLangs))
        {
            //  Use -1 as error for now.
            nValue1 = -1;
            nValue2 = -1;
        }
        else
        {
            if (SkipString(pszLangs, "-", pszLangs))
            {
                //  It is a range.
                if (!ExtractUInt32(pszLangs, nValue2, pszLangs))
                {
                    //  Nothing after the "-" is an error.
                    RIL_LOG_CRITICAL("CTEBase::ParseGsmGetBroadcastSmsConfig() -"
                            " Parsing szLangs range. nStructsLangs=%d i=%d\r\n", nStructsLangs, i);
                    goto Error;
                }
            }
            else
            {
                //  Not a range.
                nValue2 = nValue1;
            }
        }

        pBroadcastSmsConfigInfoBlob->aRilGsmBroadcastSmsConfigInfo[i].fromCodeScheme = nValue1;
        pBroadcastSmsConfigInfoBlob->aRilGsmBroadcastSmsConfigInfo[i].toCodeScheme = nValue2;
        pBroadcastSmsConfigInfoBlob->apRilGSMBroadcastSmsConfigInfo[i] =
                &(pBroadcastSmsConfigInfoBlob->aRilGsmBroadcastSmsConfigInfo[i]);

        //  Parse the next ",".
        if (!SkipString(pszChannels, ",", pszChannels))
        {
            //  We are done.
            break;
        }
    }

    //  Loop through each struct, populate "selected".
    for (UINT32 i=0; (i < nCount) && (i < RIL_MAX_BROADCASTSMSCONFIGINFO_ENTRIES); i++)
    {
        pBroadcastSmsConfigInfoBlob->aRilGsmBroadcastSmsConfigInfo[i].selected =
                (unsigned char)nSelected;
    }

    res = RRIL_RESULT_OK;

    rRspData.pData   = (void*)pBroadcastSmsConfigInfoBlob;
    rRspData.uiDataSize  = nCount * sizeof(RIL_GSM_BroadcastSmsConfigInfo*);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pBroadcastSmsConfigInfoBlob);
        pBroadcastSmsConfigInfoBlob = NULL;
    }


    RIL_LOG_VERBOSE("CTEBase::ParseGsmGetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG
//
RIL_RESULT_CODE CTEBase::CoreGsmSetBroadcastSmsConfig(REQUEST_DATA& /*rReqData*/,
                                                                 void* pData,
                                                                 UINT32 uiDataSize)
{
    int nConfigInfos = 0;

    RIL_LOG_VERBOSE("CTEBase::CoreGsmSetBroadcastSmsConfig() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    RIL_GSM_BroadcastSmsConfigInfo** ppConfigInfo = (RIL_GSM_BroadcastSmsConfigInfo**)pData;
    m_vBroadcastSmsConfigInfo.clear();

    if ((0 != uiDataSize) && (0 != (uiDataSize % sizeof(RIL_GSM_BroadcastSmsConfigInfo**))))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreGsmSetBroadcastSmsConfig() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (0 != uiDataSize && NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreGsmSetBroadcastSmsConfig() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    nConfigInfos = uiDataSize / sizeof(RIL_GSM_BroadcastSmsConfigInfo**);
    RIL_LOG_INFO("CTEBase::CoreGsmSetBroadcastSmsConfig() - nConfigInfos = %d.\r\n", nConfigInfos);

    for (int i = 0; i < nConfigInfos; i++)
    {
        m_vBroadcastSmsConfigInfo.push_back(*(ppConfigInfo[i]));
    }

    if (m_vBroadcastSmsConfigInfo.empty())
    {
        RIL_LOG_CRITICAL("CTEBase::CoreGsmSetBroadcastSmsConfig() -"
                " m_vBroadcastSmsConfigInfo empty.\r\n");
    }

    res = RRIL_RESULT_OK_IMMEDIATE;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreGsmSetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGsmSetBroadcastSmsConfig(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGsmSetBroadcastSmsConfig() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseGsmSetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION
//
RIL_RESULT_CODE CTEBase::CoreGsmSmsBroadcastActivation(REQUEST_DATA& rReqData,
                                                                  void* pData,
                                                                  UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGsmSmsBroadcastActivation() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (m_vBroadcastSmsConfigInfo.empty())
    {
        int fBcActivate = 0;

        if (sizeof(int) != uiDataSize)
        {
            RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                    " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
            goto Error;
        }

        if (NULL == pData)
        {
            RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                    " Passed data pointer was NULL\r\n");
            goto Error;
        }

        fBcActivate = *((int*)pData);
        RIL_LOG_INFO("CTEBase::CoreGsmSmsBroadcastActivation() - fBcActivate=[%u]\r\n",
                fBcActivate);

        //  According to ril.h, 0 = activate, 1 = disable
        if (0 == fBcActivate)
        {
            //  activate
            // This command activates all channels with all code schemes.
            if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CSCB=1\r", sizeof(rReqData.szCmd1)))
            {
                res = RRIL_RESULT_OK;
            }
        }
        else
        {
            //  disable
            // This command deactivates all channels with all code schemes.
            if (CopyStringNullTerminate(rReqData.szCmd1, "AT+CSCB=0\r", sizeof(rReqData.szCmd1)))
            {
                res = RRIL_RESULT_OK;
            }
        }
    }
    else
    {
        char szChannels[MAX_BUFFER_SIZE] = {0};
        char szLangs[MAX_BUFFER_SIZE] = {0};
        char szChannelsInt[MAX_BUFFER_SIZE] = {0};
        char szLangsInt[MAX_BUFFER_SIZE] = {0};
        int isSelected = 1;
        int nChannelToAccept = 0;
        int fromServiceIdMem = 0xFFFF;
        int toServiceIdMem = 0xFFFF;
        int fromCodeSchemeMem = 0xFFFF;
        int toCodeSchemeMem = 0xFFFF;
        RIL_GSM_BroadcastSmsConfigInfo _tConfigInfo;
        int i;

        RIL_LOG_INFO("CTEBase::CoreGsmSmsBroadcastActivation() -"
                " m_vBroadcastSmsConfigInfo.size() = %d.\r\n", m_vBroadcastSmsConfigInfo.size());

        //  Loop through each RIL_GSM_BroadcastSmsConfigInfo structure.
        //  Make string szChannels our list of channels to add.
        for (i = 0; i < (int)m_vBroadcastSmsConfigInfo.size(); i++)
        {
            _tConfigInfo = m_vBroadcastSmsConfigInfo[i];

            // Default Value. If no channel selected the mode must at the not accepted value (1).
            if (_tConfigInfo.selected == true)
            {
                isSelected = 0;
                nChannelToAccept++;
            }
        }

        RIL_LOG_INFO("CTEBase::CoreGsmSmsBroadcastActivation() - nChannelToAccept = %d.\r\n",
                nChannelToAccept);


        if (nChannelToAccept > 0)
        {
            //  Loop through each RIL_GSM_BroadcastSmsConfigInfo structure.
            //  Make string szChannels our list of channels to add.
            nChannelToAccept = 0;
            for (i = 0; i < (int)m_vBroadcastSmsConfigInfo.size(); i++)
            {
                _tConfigInfo = m_vBroadcastSmsConfigInfo[i];

                if (_tConfigInfo.selected == true)
                {

                    if (nChannelToAccept == 0)
                    {
                        if (_tConfigInfo.fromServiceId == _tConfigInfo.toServiceId)
                        {
                            if (!PrintStringNullTerminate(szChannels,
                                    MAX_BUFFER_SIZE - strlen(szChannels), "%u",
                                    _tConfigInfo.fromServiceId))
                            {
                                RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                        " Unable to print from service id of"
                                        " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                goto Error;
                            }
                        }
                        else
                        {
                            if (!PrintStringNullTerminate(szChannels,
                                    MAX_BUFFER_SIZE - strlen(szChannels), "%u-%u",
                                    _tConfigInfo.fromServiceId, _tConfigInfo.toServiceId))
                            {
                                RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                        " Unable to print to service id of"
                                        " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                goto Error;
                            }
                        }
                        nChannelToAccept++;
                    }
                    else
                    {
                        if ((fromServiceIdMem != _tConfigInfo.fromServiceId) &&
                                (toServiceIdMem != _tConfigInfo.toServiceId))
                        {
                            if (_tConfigInfo.fromServiceId == _tConfigInfo.toServiceId)
                            {
                                if (!PrintStringNullTerminate(szChannelsInt,
                                        MAX_BUFFER_SIZE - strlen(szChannelsInt), ",%u",
                                        _tConfigInfo.fromServiceId))
                                {
                                    RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                            " Unable to print from service id of"
                                            " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                    goto Error;
                                }
                                if (!ConcatenateStringNullTerminate(szChannels,
                                        sizeof(szChannels), szChannelsInt))
                                {
                                    RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                            " Unable to print from service id of"
                                            " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                    goto Error;
                                }
                            }
                            else
                            {
                                if (!PrintStringNullTerminate(szChannelsInt,
                                        MAX_BUFFER_SIZE - strlen(szChannelsInt),
                                        ",%u-%u", _tConfigInfo.fromServiceId,
                                        _tConfigInfo.toServiceId))
                                {
                                    RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                            " Unable to print to service id of"
                                            " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                    goto Error;
                                }
                                if (!ConcatenateStringNullTerminate(szChannels,
                                        sizeof(szChannels), szChannelsInt))
                                {
                                    RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                            " Unable to print to service id of"
                                            " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                    goto Error;
                                }
                            }
                            nChannelToAccept++;
                        }
                        fromServiceIdMem = _tConfigInfo.fromServiceId;
                        toServiceIdMem = _tConfigInfo.toServiceId;
                    }
                }
            }

            //  Loop through each RIL_GSM_BroadcastSmsConfigInfo structure.
            //  Make string szLangs our list of languages to add.
            nChannelToAccept = 0;
            for (i = 0; i < (int)m_vBroadcastSmsConfigInfo.size(); i++)
            {
                _tConfigInfo = m_vBroadcastSmsConfigInfo[i];

                if (_tConfigInfo.selected == true)
                {

                    if (nChannelToAccept == 0)
                    {
                        if (_tConfigInfo.fromCodeScheme == _tConfigInfo.toCodeScheme)
                        {
                            if (!PrintStringNullTerminate(szLangs,
                                    MAX_BUFFER_SIZE - strlen(szLangs), "%u",
                                    _tConfigInfo.fromCodeScheme))
                            {
                                RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                        " Unable to print from service id of"
                                        " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                goto Error;
                            }
                        }
                        else
                        {
                            if (!PrintStringNullTerminate(szLangs,
                                    MAX_BUFFER_SIZE - strlen(szLangs), "%u-%u",
                                    _tConfigInfo.fromCodeScheme, _tConfigInfo.toCodeScheme))
                            {
                                RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                        " Unable to print from from-to code scheme of"
                                        " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                goto Error;
                            }
                        }
                        nChannelToAccept++;
                    }
                    else
                    {
                        if ((fromCodeSchemeMem != _tConfigInfo.fromCodeScheme) &&
                                (toCodeSchemeMem != _tConfigInfo.toCodeScheme))
                        {
                            if (_tConfigInfo.fromCodeScheme == _tConfigInfo.toCodeScheme)
                            {
                                if (!PrintStringNullTerminate(szLangsInt,
                                        MAX_BUFFER_SIZE - strlen(szLangsInt), ",%u",
                                        _tConfigInfo.fromCodeScheme))
                                {
                                    RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                            " Unable to print from service id of"
                                            " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                    goto Error;
                                }
                                if (!ConcatenateStringNullTerminate(szLangs,
                                        sizeof(szLangs), szLangsInt))
                                {
                                    RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                            " Unable to print from service id of"
                                            " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                    goto Error;
                                }
                            }
                            else
                            {
                                if (!PrintStringNullTerminate(szLangsInt,
                                        MAX_BUFFER_SIZE - strlen(szLangsInt), ",%u-%u",
                                        _tConfigInfo.fromCodeScheme, _tConfigInfo.toCodeScheme))
                                {
                                    RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                            " Unable to print from from-to code scheme of"
                                            " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                    goto Error;
                                }
                                if (!ConcatenateStringNullTerminate(szLangs,
                                        sizeof(szLangs), szLangsInt))
                                {
                                    RIL_LOG_CRITICAL("CTEBase::CoreGsmSmsBroadcastActivation() -"
                                            " Unable to print from service id of"
                                            " m_vBroadcastSmsConfigInfo[%d]\r\n", i);
                                    goto Error;
                                }
                            }
                            nChannelToAccept++;
                        }
                    }
                    fromCodeSchemeMem = _tConfigInfo.fromCodeScheme;
                    toCodeSchemeMem = _tConfigInfo.toCodeScheme;
                }
            }

            //  Make the final string.
            if (PrintStringNullTerminate(rReqData.szCmd1,
                    sizeof(rReqData.szCmd1),
                    "AT+CSCB=%u,\"%s\",\"%s\"\r",
                    isSelected, szChannels, szLangs))
            {
                res = RRIL_RESULT_OK;
            }

        }
        else
        {
            //  Make the final string.
            if (PrintStringNullTerminate(rReqData.szCmd1,
                    sizeof(rReqData.szCmd1),
                    "AT+CSCB=0,\"\",\"\"\r"))
            {
                res = RRIL_RESULT_OK;
            }
        }
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreGsmSmsBroadcastActivation() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGsmSmsBroadcastActivation(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGsmSmsBroadcastActivation() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseGsmSmsBroadcastActivation() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG
//
RIL_RESULT_CODE CTEBase::CoreCdmaGetBroadcastSmsConfig(REQUEST_DATA& /*rReqData*/,
                                                                  void* /*pData*/,
                                                                  UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaGetBroadcastSmsConfig() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaGetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaGetBroadcastSmsConfig(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaGetBroadcastSmsConfig() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaGetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG
//
RIL_RESULT_CODE CTEBase::CoreCdmaSetBroadcastSmsConfig(REQUEST_DATA& /*rReqData*/,
                                                                  void* /*pData*/,
                                                                  UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSetBroadcastSmsConfig() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaSetBroadcastSmsConfig(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSetBroadcastSmsConfig() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSetBroadcastSmsConfig() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION
//
RIL_RESULT_CODE CTEBase::CoreCdmaSmsBroadcastActivation(REQUEST_DATA& /*rReqData*/,
                                                                   void* /*pData*/,
                                                                   UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSmsBroadcastActivation() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSmsBroadcastActivation() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaSmsBroadcastActivation(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSmsBroadcastActivation() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSmsBroadcastActivation() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_SUBSCRIPTION
//
RIL_RESULT_CODE CTEBase::CoreCdmaSubscription(REQUEST_DATA& /*rReqData*/,
                                                         void* /*pData*/,
                                                         UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSubscription() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaSubscription() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaSubscription(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSubscription() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaSubscription() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM
//
RIL_RESULT_CODE CTEBase::CoreCdmaWriteSmsToRuim(REQUEST_DATA& /*rReqData*/,
                                                           void* /*pData*/,
                                                           UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaWriteSmsToRuim() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaWriteSmsToRuim() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaWriteSmsToRuim(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaWriteSmsToRuim() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaWriteSmsToRuim() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM
//
RIL_RESULT_CODE CTEBase::CoreCdmaDeleteSmsOnRuim(REQUEST_DATA& /*rReqData*/,
                                                            void* /*pData*/,
                                                            UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreCdmaDeleteSmsOnRuim() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreCdmaDeleteSmsOnRuim() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCdmaDeleteSmsOnRuim(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCdmaDeleteSmsOnRuim() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCdmaDeleteSmsOnRuim() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_DEVICE_IDENTITY
//
RIL_RESULT_CODE CTEBase::CoreDeviceIdentity(REQUEST_DATA& /*rReqData*/,
                                                       void* /*pData*/,
                                                       UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreDeviceIdentity() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreDeviceIdentity() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseDeviceIdentity(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseDeviceIdentity() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseDeviceIdentity() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE
//
RIL_RESULT_CODE CTEBase::CoreExitEmergencyCallbackMode(REQUEST_DATA& /*rReqData*/,
                                                                  void* /*pData*/,
                                                                  UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreExitEmergencyCallbackMode() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreExitEmergencyCallbackMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseExitEmergencyCallbackMode(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseExitEmergencyCallbackMode() - Enter\r\n");
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseExitEmergencyCallbackMode() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_GET_SMSC_ADDRESS
//
RIL_RESULT_CODE CTEBase::CoreGetSmscAddress(REQUEST_DATA& rReqData,
                                                       void* /*pData*/,
                                                       UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetSmscAddress() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CSCA?\r"))
    {
        res = RRIL_RESULT_OK;
    }

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreGetSmscAddress() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseGetSmscAddress(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseGetSmscAddress() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* szRsp = rRspData.szResponse;

    char* szSCAddr = (char*)malloc(MAX_BUFFER_SIZE);
    if (NULL == szSCAddr)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetSMSCAddress() -"
                " Could not allocate memory for a %u-char string.\r\n", MAX_BUFFER_SIZE);
        goto Error;
    }
    memset(szSCAddr, 0, MAX_BUFFER_SIZE);

    // Parse "<prefix><sca>,<tosca><postfix>"
    //  We can ignore the , and <tosca>.
    SkipRspStart(szRsp, m_szNewLine, szRsp);

    if (!FindAndSkipString(szRsp, "+CSCA: ", szRsp) ||
        !ExtractQuotedString(szRsp, szSCAddr, MAX_BUFFER_SIZE, szRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseGetSMSCAddress() -"
                " Could not extract the SMS Service Center address string.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

    rRspData.pData   = (void*)szSCAddr;
    rRspData.uiDataSize  = sizeof(char*);

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(szSCAddr);
        szSCAddr = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseGetSMSCAddress() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_SMSC_ADDRESS
//
RIL_RESULT_CODE CTEBase::CoreSetSmscAddress(REQUEST_DATA& rReqData,
                                                       void* pData,
                                                       UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetSmscAddress() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char *szSMSC = NULL;

    if (sizeof(char*) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetSmscAddress() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetSmscAddress() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    szSMSC = (char*)pData;

    //  Modem works best with addr type of 145.  Need '+' in front.
    //  I noticed CMS ERROR: 96 when setting SMSC with addr type 129.
    if ('+' == szSMSC[0])
    {
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CSCA=\"%s\",145\r", szSMSC))
        {
            res = RRIL_RESULT_OK;
        }
    }
    else
    {
        if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                "AT+CSCA=\"+%s\",145\r", szSMSC))
        {
            res = RRIL_RESULT_OK;
        }
    }




Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSetSmscAddress() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetSmscAddress(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetSmscAddress() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseSetSmscAddress() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_REPORT_SMS_MEMORY_STATUS
//
RIL_RESULT_CODE CTEBase::CoreReportSmsMemoryStatus(REQUEST_DATA& /*rReqData*/,
                                                              void* /*pData*/,
                                                              UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreReportSmsMemoryStatus() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_inf_6260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTEBase::ParseReportSmsMemoryStatus(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseReportSmsMemoryStatus() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_inf_6260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

//
// RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
//
RIL_RESULT_CODE CTEBase::CoreReportStkServiceRunning(REQUEST_DATA& /*rReqData*/,
                                                                void* /*pData*/,
                                                                UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreReportStkServiceRunning() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_inf_6260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTEBase::ParseReportStkServiceRunning(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseReportStkServiceRunning() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_inf_6260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

//
// RIL_REQUEST_ISIM_AUTHENTICATE
//
RIL_RESULT_CODE CTEBase::CoreISimAuthenticate(REQUEST_DATA& /*rReqData*/,
                                                         void* /*pData*/,
                                                         UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreISimAuthenticate() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_inf_6260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTEBase::ParseISimAuthenticate(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseISimAuthenticate() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_inf_6260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}



//
// RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU
//
RIL_RESULT_CODE CTEBase::CoreAckIncomingGsmSmsWithPdu(REQUEST_DATA& rReqData,
                                                                 void* pData,
                                                                 UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreAckIncomingGsmSmsWithPdu() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    char** pszCmdData = NULL;
    char* szStatus = NULL;
    char* szPDU = NULL;
    int nPDULength = 0;
    BOOL fSuccess = FALSE;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreAckIncomingGsmSmsWithPdu() -"
                " Passed data pointer was NULL\r\n");
        goto Error;
    }

    if ((2 * sizeof(char *)) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreAckIncomingGsmSmsWithPdu() -"
                " Invalid data size. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    pszCmdData = (char**)pData;
    szStatus = pszCmdData[0];
    szPDU = pszCmdData[1];

    if (NULL == szStatus || NULL == szPDU)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreAckIncomingGsmSmsWithPdu() - Status or PDU was NULL!\r\n");
        goto Error;
    }

    // 2 chars per byte.
    nPDULength = (strlen(szPDU) / 2);

    fSuccess = (0 == strcmp(szStatus, "1")) ? TRUE : FALSE;

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CNMA=%u,%u\r",
            fSuccess ? 1 : 2, nPDULength))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreAckIncomingGsmSmsWithPdu() -"
                " Cannot create CNMA command\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(rReqData.szCmd2, sizeof(rReqData.szCmd2), "%s\x1a", szPDU))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreAckIncomingGsmSmsWithPdu() - Cannot create CNMA PDU\r\n");
        goto Error;
    }

    RIL_LOG_INFO("Ack pdu: %s\r\n",
            CRLFExpandedString(rReqData.szCmd2, strlen(rReqData.szCmd2)).GetString());

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreAckIncomingGsmSmsWithPdu() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseAckIncomingGsmSmsWithPdu(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseAckIncomingGsmSmsWithPdu() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseAckIncomingGsmSmsWithPdu() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
//
RIL_RESULT_CODE CTEBase::ParseStkSendEnvelopeWithStatus(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseStkSendEnvelopeWithStatus() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_inf_6260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

//
// RIL_REQUEST_GET_CELL_INFO_LIST
//
RIL_RESULT_CODE CTEBase::CoreGetCellInfoList(REQUEST_DATA& /*rReqData*/,
                                                        void* /*pData*/,
                                                        UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreGetCellInfoList() - Enter\r\n");

    // this is modem dependent, to be implemented in te_inf_6260.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::CoreGetCellInfoList() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCellInfo(P_ND_N_CELL_INFO_DATA /*pCellData*/,
                                                    const char* /*pszRsp*/,
                                                    UINT32 /*uiIndex*/,
                                                    UINT32 /*uiMode*/)
{

    RIL_LOG_VERBOSE("CTEBase::ParseCellInfo() - Enter\r\n");
    // this is modem dependent, to be implemented in te_xmmxxx.cpp
    RIL_RESULT_CODE res = RIL_E_REQUEST_NOT_SUPPORTED;

    RIL_LOG_VERBOSE("CTEBase::ParseCellInfo() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseCellInfoList(RESPONSE_DATA& rRspData, BOOL isUnsol)
{
    RIL_LOG_VERBOSE("CTEBase::ParseCellInfoList() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiIndex = 0;
    UINT32 uiMode = 0;
    const char* pszRsp = rRspData.szResponse;

    P_ND_N_CELL_INFO_DATA pCellData = NULL;

    pCellData = (P_ND_N_CELL_INFO_DATA)malloc(sizeof(S_ND_N_CELL_INFO_DATA));
    if (NULL == pCellData)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseCellInfoList() -"
                " Could not allocate memory for a S_ND_N_CELL_INFO_DATA struct.\r\n");
        goto Error;
    }
    memset(pCellData, 0, sizeof(S_ND_N_CELL_INFO_DATA));


    // Loop on +XCELLINFO until no more entries are found
    while (FindAndSkipString(pszRsp, "+XCELLINFO: ", pszRsp))
    {
        if (RRIL_MAX_CELL_ID_COUNT == uiIndex)
        {
            RIL_LOG_CRITICAL("CTEBase::ParseCellInfoList() -"
                    " Exceeded max count = %d\r\n", RRIL_MAX_CELL_ID_COUNT);
            break;
        }

        // Get <mode>
        if (!ExtractUInt32(pszRsp, uiMode, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseCellInfoList() -"
                    " cannot extract <mode>\r\n");
            goto Error;
        }

        RIL_LOG_INFO("CTEBase::ParseCellInfoList() - found mode=%d isUnsol = %d\r\n",
                uiMode, isUnsol);
        RIL_RESULT_CODE result = RRIL_RESULT_ERROR;
        switch (uiMode)
        {
            // GSM/UMTS/LTE cases
            case 0:
            case 1:
            case 2:
            case 3:
            case 5:
            case 6:
                // Call the TE specific parsing function
                result = ParseCellInfo(pCellData, pszRsp, uiIndex, uiMode);

                if (result == RRIL_RESULT_OK)
                {
                    uiIndex++;
                    RIL_LOG_INFO("CTEBase::ParseCellInfoList() - Index=%d\r\n", uiIndex);
                }
                break;
            default:
                break;
        }
    }
    res = RRIL_RESULT_OK;

    if (!isUnsol)
    {
        if (uiIndex > 0)
        {
            rRspData.pData  = (void*)pCellData;
            rRspData.uiDataSize = uiIndex * sizeof(RIL_CellInfo);
        }
        else
        {
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
        if (uiIndex > 0)
        {
            int requestedRate = (intptr_t)rRspData.pContextData;
            if (m_cte.updateCellInfoCache(pCellData, (INT32)uiIndex)
                    && requestedRate > 0 && requestedRate < INT32_MAX)
            {
                RIL_onUnsolicitedResponse(RIL_UNSOL_CELL_INFO_LIST,
                        (void*)pCellData->aRilCellInfo, (sizeof(RIL_CellInfo) * uiIndex));
            }
        }
        else
        {
            free(pCellData);
            pCellData = NULL;
        }

        // restart the timer now with the latest rate setting.
        if (!m_cte.IsCellInfoTimerRunning())
        {
            int newRate = m_cte.GetCellInfoListRate();
            RestartUnsolCellInfoListTimer(newRate);
        }
    }

Error:

    if (RRIL_RESULT_OK != res || isUnsol)
    {
        free(pCellData);
        pCellData = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseCellInfoList() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE
//
RIL_RESULT_CODE CTEBase::CoreSetCellInfoListRate(REQUEST_DATA& /*rReqData*/,
                                                            void* pData,
                                                            UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetCellInfoListRate() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int previousRate = m_cte.GetCellInfoListRate();

    if (pData == NULL)
    {
        RIL_LOG_WARNING("CTEBase::CoreGetCellInfoList() - pData[0] is NULL!\r\n");
        return res;
    }

    int newRate = ((int*)pData)[0];
    // Value = 0, ril rate has the default value, 5s.
    // Value = INT32_MAX, no reports
    // else rril rate is MAX(5s, newRate) when the information has changed
    if (newRate < 0)
    {
        RIL_LOG_WARNING("CTEBase::CoreGetCellInfoList() - newRate is invalid!\r\n");
        return res;
    }
    else if (newRate < INT32_MAX)
    {
        if (previousRate != newRate)
        {
            newRate = (newRate < 5000) ? 5000 : newRate;
        }
    }
    else
    {
        newRate = INT32_MAX;
    }

    RIL_LOG_INFO("CTEBase::CoreSetCellInfoListRate() - "
            "uiNewRate =%d uiPreviousRate = %d\r\n",
             newRate, previousRate);

    // First get the cellinfo information, to report the current values.
    // Get should be suppressed for rate value = INT32_MAX, no reports
    if (newRate < INT32_MAX)
    {
        REQUEST_DATA rReqData;
        memset(&rReqData, 0, sizeof(REQUEST_DATA));

        if (!CopyStringNullTerminate(rReqData.szCmd1, m_cte.GetReadCellInfoString(),
                sizeof(rReqData.szCmd1)))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreGetCellInfoList() - "
                    "Unable to create cellinfo command!\r\n");
            return res;
        }

        rReqData.pContextData = (void*)(intptr_t)newRate;

        CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_GET_CELL_INFO_LIST].uiChannel,
                NULL, RIL_REQUEST_GET_CELL_INFO_LIST, rReqData,
                &CTE::ParseUnsolCellInfoListRate, &CTE::PostUnsolCellInfoListRate);

        if (pCmd)
        {
            if (!CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreGetCellInfoList() - Unable to queue command!\r\n");
                delete pCmd;
                pCmd = NULL;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CTEBase::CoreGetCellInfoList() - "
                    "Unable to allocate memory for new command!\r\n");
            return res;
        }
    }

    m_cte.SetCellInfoListRate(newRate);
    RestartUnsolCellInfoListTimer(newRate);

    res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::CoreGetCellInfoList() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSetCellInfoListRate(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetCellInfoListRate() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_OK;

    RIL_LOG_VERBOSE("CTEBase::ParseSetCellInfoListRate() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseUnsolCellInfoListRate(RESPONSE_DATA& rRspData)
{
   RIL_LOG_VERBOSE("CTEBase::ParseUnsolCellInfoListRate() - Enter\r\n");

   return ParseCellInfoList(rRspData, TRUE);
}

void CTEBase::RestartUnsolCellInfoListTimer(int newRate)
{
    RIL_LOG_VERBOSE("CTEBase::RestartUnsolCellInfoListTimer() - Enter\r\n");

    // Start timer to query for CELLINFO  only if the value of >0 and <INT32_MAX
    if (newRate > 0 && newRate < INT32_MAX)
    {
        if (!m_cte.IsCellInfoTimerRunning())
        {
            m_cte.SetCellInfoTimerRunning(TRUE);
            RIL_LOG_INFO("CTEBase::RestartUnsolCellInfoListTimer() -"
                    "for %d milliseconds\r\n", newRate);
            RIL_requestTimedCallback(triggerCellInfoList,
                    (void*)(intptr_t)newRate, (newRate/1000), 0);
        }
    }
}

// RIL_REQUEST_SET_INITIAL_ATTACH_APN
RIL_RESULT_CODE CTEBase::CoreSetInitialAttachApn(REQUEST_DATA& /*reqData*/,
                                                            void* /*pData*/,
                                                            UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetInitialAttachApn() - Enter / Exit\r\n");
    // No action taken but complete as success
    return RRIL_RESULT_OK_IMMEDIATE;
}

RIL_RESULT_CODE CTEBase::ParseSetInitialAttachApn(RESPONSE_DATA& /*rspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetInitialAttachApn() - Enter / Exit\r\n");
    return RRIL_RESULT_OK;
}

// RIL_REQUEST_IMS_REGISTRATION_STATE
// RIL_REQUEST_IMS_SEND_SMS
// TODO

//
// RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC
//
RIL_RESULT_CODE CTEBase::CoreSimTransmitApduBasic(REQUEST_DATA& rReqData,
                                                         void* pData,
                                                         UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSimTransmitApduBasic() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_SIM_APDU* pRilSimApdu = NULL;
    int classByte;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduBasic() -"
                " Data pointer is NULL.\r\n");
        goto Error;
    }

    if (sizeof(RIL_SIM_APDU) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduBasic() -"
                " Invalid data size. Given %d bytes\r\n", uiDataSize);
        goto Error;
    }

    // extract data
    pRilSimApdu = (RIL_SIM_APDU*)pData;
    classByte = pRilSimApdu->cla;

    RIL_LOG_VERBOSE("CTEBase::CoreSimTransmitApduBasic() -"
            " cla=%02X instruction=%d p1=%d p2=%d p3=%d data=\"%s\"\r\n",
            classByte, pRilSimApdu->instruction,
            pRilSimApdu->p1, pRilSimApdu->p2, pRilSimApdu->p3,
            pRilSimApdu->data);

    if ((NULL == pRilSimApdu->data) || (strlen(pRilSimApdu->data) == 0))
    {
        if (pRilSimApdu->p3 < 0)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                    sizeof(rReqData.szCmd1),
                    "AT+CSIM=%d,\"%02x%02x%02x%02x\"\r",
                    8,
                    classByte,
                    pRilSimApdu->instruction,
                    pRilSimApdu->p1,
                    pRilSimApdu->p2))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduBasic() -"
                        " cannot create CSIM command 1\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                    sizeof(rReqData.szCmd1),
                    "AT+CSIM=%d,\"%02x%02x%02x%02x%02x\"\r",
                    10,
                    classByte,
                    pRilSimApdu->instruction,
                    pRilSimApdu->p1,
                    pRilSimApdu->p2,
                    pRilSimApdu->p3))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduBasic() -"
                        " cannot create CSIM command 2\r\n");
                goto Error;
            }
        }
    }
    else
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1,
                sizeof(rReqData.szCmd1),
                "AT+CSIM=%d,\"%02x%02x%02x%02x%02x%s\"\r",
                10 + strlen(pRilSimApdu->data),
                classByte,
                pRilSimApdu->instruction,
                pRilSimApdu->p1,
                pRilSimApdu->p2,
                pRilSimApdu->p3,
                pRilSimApdu->data))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduBasic() -"
                    "cannot create CSIM command 3\r\n");
            goto Error;
        }
    }


    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSimTransmitApduBasic() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSimTransmitApduBasic(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimTransmitApduBasic() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;

    int length = 0;
    char* pszResponseString = NULL;
    int respLen = 0;
    RIL_SIM_IO_Response* pResponse = NULL;
    size_t payloadSize = 0;
    size_t responseSize = 0;
    UINT32 uiResponseStrLen = 0;

    if (NULL == rRspData.szResponse)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduBasic() -"
                " Response String pointer is NULL.\r\n");
        goto Error;
    }

    // Parse "<prefix>+CSIM: <length>,"<response>"<postfix>"
    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    if (!SkipString(pszRsp, "+CSIM: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduBasic() -"
                " Could not skip over \"+CSIM: \".\r\n");
        goto Error;
    }

    if (!ExtractInt(pszRsp, length, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduBasic() -"
               " Could not extract <length>\r\n");
        goto Error;
    }

    /*
     * Parse <response>
     * Response hex string is formatted as:
     * <payload><sw1><sw2>
     * with payload is size 2*n chars (n>=0, n is the size in bytes of the APDU payload)
     * and sw1 and sw2 are 2 chars representing one byte
     */
    if (!FindAndSkipString(pszRsp, ",", pszRsp)
            || !ExtractQuotedStringWithAllocatedMemory(pszRsp, pszResponseString, uiResponseStrLen,
                    pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduBasic() -"
                " Could not extract <response>\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTEBase::ParseSimTransmitApduBasic() -"
            " Extracted data string: \"%s\" (%u chars)\r\n",
            pszResponseString, uiResponseStrLen);

    respLen = strlen(pszResponseString);
    /*
     * 4 chars is the minimal size to put SW1 SW2
     * <length> is the size in char of the string
     * <length> is double the size in bytes of the response APDU
     */
    if (respLen < 4 || respLen != length || length % 2)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduBasic() :"
                " invalid response\r\n");
        goto Error;
    }

    // Response payload is the string returned minus the 4 last characters (SW1,SW2).
    payloadSize = respLen - 4;

    // Response size is size of RIL_SIM_IO_Response + payloadSize + 1 for null character
    responseSize = sizeof(RIL_SIM_IO_Response) + payloadSize + 1;
    pResponse = (RIL_SIM_IO_Response*)malloc(responseSize);
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduBasic() -"
                 " Could not allocate memory for a RIL_SIM_IO_Response struct.\r\n");
        goto Error;
    }
    memset(pResponse, 0, responseSize);

    sscanf(&pszResponseString[respLen-4], "%02x%02x", &pResponse->sw1, &pResponse->sw2);

    pResponse->simResponse = ((char*)pResponse) + sizeof(RIL_SIM_IO_Response);
    strncpy(pResponse->simResponse, pszResponseString, payloadSize);
    pResponse->simResponse[payloadSize] = '\0';

    // Parse "<postfix>"
    SkipRspEnd(pszRsp, m_szNewLine, pszRsp);

    rRspData.pData = pResponse;
    rRspData.uiDataSize  = sizeof(RIL_SIM_IO_Response);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    delete[] pszResponseString;
    pszResponseString = NULL;

    RIL_LOG_VERBOSE("CTEBase::ParseSimTransmitApduBasic() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIM_OPEN_CHANNEL
//
RIL_RESULT_CODE CTEBase::CoreSimOpenChannel(REQUEST_DATA& rReqData, void* pData, UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSimOpenChannel() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    char* szAID = NULL;

    if (sizeof(char*) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimOpenChannel() -"
                " Passed data size mismatch. Found %d bytes\r\n", uiDataSize);
        goto Error;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimOpenChannel() - Passed data pointer was NULL\r\n");
        goto Error;
    }

    szAID = (char*)pData;


    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CCHO=\"%s\"\r",
            szAID))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimOpenChannel() - Cannot create CCHO command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSimOpenChannel() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSimOpenChannel(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimOpenChannel() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    int* pSessionId = NULL;

    if (NULL == rRspData.szResponse)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimOpenChannel() - Response String pointer is NULL.\r\n");
        goto Error;
    }

    pSessionId = (int*)malloc(sizeof(int));
    if (NULL == pSessionId)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimOpenChannel() -"
                " Could not allocate memory for an int.\r\n");
        goto Error;
    }
    memset(pSessionId, 0, sizeof(int));

    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    // As per 3GPP spec, response is <prefix><sessionid><postfix> but as modem sends
    // <prefix>+CCHO: <sessionid><postfix>" parsing also done for "+CCHO: "
    FindAndSkipString(pszRsp, "+CCHO: ", pszRsp);

    if (!ExtractInt(pszRsp, *pSessionId, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimOpenChannel() -"
                " Could not extract session Id.\r\n");
        goto Error;
    }

    // Parse "<postfix>"
    SkipRspEnd(pszRsp, m_szNewLine, pszRsp);

    rRspData.pData = pSessionId;
    rRspData.uiDataSize = sizeof(int);

    res = RRIL_RESULT_OK;
Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pSessionId);
        pSessionId = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseSimOpenChannel() - Exit\r\n");
    return res;
}

//
// RIL_REQUEST_SIM_CLOSE_CHANNEL
//
RIL_RESULT_CODE CTEBase::CoreSimCloseChannel(REQUEST_DATA& rReqData,
                                                        void* pData,
                                                        UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSimCloseChannel() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int sessionId = 0;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimCloseChannel() - Data pointer is NULL.\r\n");
        goto Error;
    }

    sessionId = ((int *)pData)[0];

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
            "AT+CCHC=%d\r", sessionId))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimCloseChannel() - Cannot create CCHC command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSimCloseChannel() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSimCloseChannel(RESPONSE_DATA& /*rspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimCloseChannel() - Enter / Exit\r\n");
    return RRIL_RESULT_OK;
}

//
// RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL
//
RIL_RESULT_CODE CTEBase::CoreSimTransmitApduChannel(REQUEST_DATA& rReqData,
                                                           void* pData,
                                                           UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSimTransmitApduChannel() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    RIL_SIM_APDU*   pSimIOArgs = NULL;
    int classByte;

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduChannel() - Data pointer is NULL.\r\n");
        goto Error;
    }

    if (sizeof(RIL_SIM_APDU) != uiDataSize)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduChannel() -"
                " Invalid data size. Given %d bytes\r\n", uiDataSize);
        goto Error;
    }

    // extract data
    pSimIOArgs = (RIL_SIM_APDU*)pData;
    classByte = pSimIOArgs->cla;

    RIL_LOG_VERBOSE("CTEBase::CoreSimTransmitApduChannel() -"
            " cla=%02X command=%d fileid=%04X p1=%d p2=%d p3=%d data=\"%s\"\r\n",
            classByte, pSimIOArgs->instruction, pSimIOArgs->sessionid,
            pSimIOArgs->p1, pSimIOArgs->p2, pSimIOArgs->p3,
            pSimIOArgs->data);

    if ((NULL == pSimIOArgs->data) || (strlen(pSimIOArgs->data) == 0))
    {
        if (pSimIOArgs->p3 < 0)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                    sizeof(rReqData.szCmd1),
                    "AT+CGLA=%d,%d,\"%02x%02x%02x%02x\"\r",
                    pSimIOArgs->sessionid,
                    8,
                    classByte,
                    pSimIOArgs->instruction,
                    pSimIOArgs->p1,
                    pSimIOArgs->p2))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduChannel() -"
                        " cannot create CGLA command 1\r\n");
                goto Error;
            }
        }
        else
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1,
                    sizeof(rReqData.szCmd1),
                    "AT+CGLA=%d,%d,\"%02x%02x%02x%02x%02x\"\r",
                    pSimIOArgs->sessionid,
                    10,
                    classByte,
                    pSimIOArgs->instruction,
                    pSimIOArgs->p1,
                    pSimIOArgs->p2,
                    pSimIOArgs->p3))
            {
                RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduChannel() -"
                        " cannot create CGLA command 2\r\n");
                goto Error;
            }
        }
    }
    else
    {
        if (!PrintStringNullTerminate(rReqData.szCmd1,
                sizeof(rReqData.szCmd1),
                "AT+CGLA=%d,%d,\"%02x%02x%02x%02x%02x%s\"\r",
                pSimIOArgs->sessionid,
                10 + strlen(pSimIOArgs->data),
                classByte,
                pSimIOArgs->instruction,
                pSimIOArgs->p1,
                pSimIOArgs->p2,
                pSimIOArgs->p3,
                pSimIOArgs->data))
        {
            RIL_LOG_CRITICAL("CTEBase::CoreSimTransmitApduChannel() -"
                    " cannot create CGLA command 3\r\n");
            goto Error;
        }
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreSimTransmitApduChannel() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseSimTransmitApduChannel(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimTransmitApduChannel() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    int length = 0;
    char* pszResponseString = NULL;
    int respLen = 0;
    RIL_SIM_IO_Response* pResponse = NULL;
    size_t payloadSize = 0;
    size_t responseSize = 0;
    UINT32 uiResponseStrLen = 0;

    if (NULL == rRspData.szResponse)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduChannel() -"
                " Response String pointer is NULL.\r\n");
        goto Error;
    }

    // Parse "<prefix>+CGLA: <length>,<response><postfix>"
    SkipRspStart(pszRsp, m_szNewLine, pszRsp);

    if (!SkipString(pszRsp, "+CGLA: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduChannel() -"
                 " Could not skip over \"+CGLA: \".\r\n");
        goto Error;
    }

    if (!ExtractInt(pszRsp, length, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduChannel() -"
                " Could not extract length value.\r\n");
        goto Error;
    }

    /*
     * Parse <response>
     * Response hex string is formatted as:
     * <payload><sw1><sw2>
     * with payload is size 2*n chars (n>=0, n is the size in bytes of the APDU payload)
     * and sw1 and sw2 are 2 chars representing one byte
     */
    if (!FindAndSkipString(pszRsp, ",", pszRsp)
            || !ExtractQuotedStringWithAllocatedMemory(pszRsp, pszResponseString, uiResponseStrLen,
                    pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduChannel() -"
                " Could not extract <response>\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CTEBase::ParseSimTransmitApduChannel() -"
            " Extracted data string: \"%s\" (%u chars)\r\n",
            pszResponseString, uiResponseStrLen);

    respLen = strlen(pszResponseString);
    /*
     * 4 chars is the minimal size to put SW1 SW2
     * <length> is the size in char of the string
     * <length> is double the size in bytes of the response APDU
     */
    if (respLen < 4 || respLen != length || length % 2)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduChannel() :"
                " invalid response\r\n");
        goto Error;
    }

    // Response size is size of RIL_SIM_IO_Response + payloadSize + 1 for null character
    responseSize = sizeof(RIL_SIM_IO_Response) + payloadSize + 1;
    pResponse = (RIL_SIM_IO_Response*)malloc(responseSize);
    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseSimTransmitApduChannel() -"
                " Could not allocate memory for a response\r\n");
        goto Error;
    }
    memset(pResponse, 0, responseSize);

    sscanf(&pszResponseString[respLen-4], "%02x%02x", &pResponse->sw1, &pResponse->sw2);

    pResponse->simResponse = ((char*)pResponse) + sizeof(RIL_SIM_IO_Response);
    strncpy(pResponse->simResponse, pszResponseString, payloadSize);
    pResponse->simResponse[payloadSize] = '\0';

    // Parse "<postfix>"
    SkipRspEnd(pszRsp, m_szNewLine, pszRsp);

    rRspData.pData = pResponse;
    rRspData.uiDataSize = sizeof(RIL_SIM_IO_Response);

    res = RRIL_RESULT_OK;
Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pResponse);
        pResponse = NULL;
    }

    delete[] pszResponseString;
    pszResponseString = NULL;

    RIL_LOG_VERBOSE("CTEBase::ParseSimTransmitApduChannel() - Exit\r\n");
    return res;
}

void CTEBase::CoreSetDataProfile(void* pData, size_t datalen)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSetDataProfile() - Enter\r\n");

    m_vDataProfileInfos.clear();

    if (datalen <= 0)
    {
        RIL_LOG_INFO("CTEBase::CoreSetDataProfile() - Empty DataProfileInfo\r\n");
        return;
    }

    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetDataProfile() -"
                " Passed data pointer was NULL\r\n");
        return;
    }

    if (0 != (datalen % sizeof(RIL_DataProfileInfo*)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreSetDataProfile() -"
                " Passed data size mismatch. Found %d bytes\r\n", datalen);
        return;
    }

    size_t nInfos = datalen / sizeof(RIL_DataProfileInfo*);
    RIL_DataProfileInfo** ppDataProfileInfo = (RIL_DataProfileInfo**)pData;
    for (size_t i = 0; i < nInfos; i++)
    {
        S_DATA_PROFILE_INFO info;
        info.profileId = ppDataProfileInfo[i]->profileId;
        info.szApn[0] = '\0';
        CopyStringNullTerminate(info.szApn, ppDataProfileInfo[i]->apn, sizeof(info.szApn));
        m_vDataProfileInfos.push_back(info);
    }

    RIL_LOG_VERBOSE("CTEBase::CoreSetDataProfile() - Exit\r\n");
}

RIL_RESULT_CODE CTEBase::CoreShutdown(REQUEST_DATA& /*reqData*/, void* /*pData*/,
        UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreShutdown() - Enter\r\n");

    HandleShutdownReq(RIL_REQUEST_SHUTDOWN);

    SetRadioStateAndNotify(RRIL_RADIO_STATE_UNAVAILABLE);

    RIL_LOG_VERBOSE("CTEBase::CoreShutdown() - Exit\r\n");
    return RRIL_RESULT_OK;
}

RIL_RESULT_CODE CTEBase::ParseShutdown(RESPONSE_DATA& /*rspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseShutdown() - Enter / Exit\r\n");

    return RRIL_RESULT_OK;
}

void CTEBase::HandleShutdownReq(int requestId)
{
    RIL_LOG_VERBOSE("CTEBase::HandleShutdownReq() - Enter\r\n");

    CCommand* pCmd = NULL;
    char szCmd[MAX_BUFFER_SIZE] = {'\0'};

    switch (m_cte.GetLastModemEvent())
    {
        case E_MMGR_EVENT_MODEM_UP:
            if (RADIO_STATE_UNAVAILABLE == GetRadioState())
            {
                if (CSystemManager::GetInstance().SendRequestModemShutdown())
                {
                    WaitForModemPowerOffEvent();
                    return;
                }
            }
            break;

        case E_MMGR_NOTIFY_MODEM_SHUTDOWN:
            RIL_LOG_INFO("CTEBase::HandleShutdownReq - Modem power off ongoing\r\n");

            /*
             * Modem power off is ongoing, so wait for modem power off event which
             * will be signalled on MODEM_DOWN event.
             */
            WaitForModemPowerOffEvent();

            RIL_LOG_INFO("CTEBase::HandleShutdownReq - Modem power off done\r\n");
            return;

        case E_MMGR_EVENT_MODEM_DOWN:
            RIL_LOG_INFO("CTEBase::HandleShutdownReq - Already in expected state\r\n");
            return;

        default:
            CSystemManager::GetInstance().CloseChannelPorts();

            RIL_LOG_INFO("CTEBase::HandleShutdownReq - "
                    "handling RADIO_POWER OFF in modem state %d\r\n",
                    m_cte.GetLastModemEvent());
            return;
    }

    if (!GetRadioPowerCommand(FALSE, E_RADIO_OFF_REASON_SHUTDOWN, szCmd, sizeof(szCmd)))
    {
        RIL_LOG_CRITICAL("CTEBase::HandleShutdownReq() - GetRadioPowerCommand failed\r\n");
        return;
    }

    pCmd = new CCommand(g_pReqInfo[requestId].uiChannel,
            NULL, requestId, szCmd, &CTE::ParseShutdown, &CTE::PostShutdown);

    if (pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::HandleShutdownReq() - Unable to add command to queue\r\n");
            delete pCmd;
            pCmd = NULL;
            return;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::HandleShutdownReq() - "
                "Unable to allocate memory for command\r\n");
        return;
    }

    WaitForModemPowerOffEvent();
    RIL_LOG_VERBOSE("CTEBase::HandleShutdownReq() - Exit\r\n");
}

RIL_SignalStrength_v6* CTEBase::ParseQuerySignalStrength(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQuerySignalStrength() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    int rssi = RSSI_UNKNOWN;
    int ber = BER_UNKNOWN;

    RIL_SignalStrength_v6* pSigStrData;

    pSigStrData = (RIL_SignalStrength_v6*)malloc(sizeof(RIL_SignalStrength_v6));
    if (NULL == pSigStrData)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQuerySignalStrength() -"
                " Could not allocate memory for RIL_SignalStrength_v6.\r\n");
        goto Error;
    }

    memset(pSigStrData, 0x00, sizeof(RIL_SignalStrength_v6));

    // Parse "<prefix>+CSQ: <rssi>,<ber><postfix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp) ||
        !SkipString(pszRsp, "+CSQ: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQuerySignalStrength() - Could not find AT response.\r\n");
        goto Error;
    }

    if (!ExtractInt(pszRsp, rssi, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQuerySignalStrength() - Could not extract <rssi>.\r\n");
        goto Error;
    }

    if (!SkipString(pszRsp, ",", pszRsp) || !ExtractInt(pszRsp, ber, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQuerySignalStrength() - Could not extract <ber>.\r\n");
        goto Error;
    }

    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQuerySignalStrength() -"
                " Could not extract the response end.\r\n");
        goto Error;
    }

    pSigStrData->GW_SignalStrength.signalStrength = rssi;
    pSigStrData->GW_SignalStrength.bitErrorRate = ber;
    pSigStrData->CDMA_SignalStrength.dbm = -1;
    pSigStrData->CDMA_SignalStrength.ecio = -1;
    pSigStrData->EVDO_SignalStrength.dbm = -1;
    pSigStrData->EVDO_SignalStrength.ecio = -1;
    pSigStrData->EVDO_SignalStrength.signalNoiseRatio = -1;
    pSigStrData->LTE_SignalStrength.signalStrength = -1;
    pSigStrData->LTE_SignalStrength.rsrp = INT_MAX;
    pSigStrData->LTE_SignalStrength.rsrq = INT_MAX;
    pSigStrData->LTE_SignalStrength.rssnr = INT_MAX;
    pSigStrData->LTE_SignalStrength.cqi = INT_MAX;

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pSigStrData);
        pSigStrData = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQuerySignalStrength - Exit()\r\n");
    return pSigStrData;
}

//
// RIL_REQUEST_SIM_AUTHENTICATION
//
RIL_RESULT_CODE CTEBase::CoreSimAuthentication(REQUEST_DATA& /*reqData*/, void* /*pData*/,
        UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CoreSimAuthentication() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_xmm7260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

RIL_RESULT_CODE CTEBase::ParseSimAuthentication(RESPONSE_DATA& /*rspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimAuthentication() - Enter / Exit\r\n");
    // this is modem dependent, to be implemented in te_xmm7260.cpp
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

void CTEBase::PostSimAuthentication(POST_CMD_HANDLER_DATA& /*data*/)
{
    RIL_LOG_VERBOSE("CTEBase::PostSimAuthentication() - Enter / Exit\r\n");
    // this is modem dependent
}

//
// RIL_UNSOL_SIGNAL_STRENGTH
//
RIL_RESULT_CODE CTEBase::ParseUnsolicitedSignalStrength(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseUnsolicitedSignalStrength() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    RIL_SignalStrength_v6* pSigStrData = ParseQuerySignalStrength(rRspData);
    if (NULL == pSigStrData)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseUnsolicitedSignalStrength() -"
                " Could not allocate memory for RIL_SignalStrength_v6.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

    RIL_onUnsolicitedResponse(RIL_UNSOL_SIGNAL_STRENGTH, (void*)pSigStrData,
            sizeof(RIL_SignalStrength_v6));

Error:
    free(pSigStrData);
    pSigStrData = NULL;

    RIL_LOG_VERBOSE("CTEBase::ParseUnsolicitedSignalStrength() - Exit\r\n");
    return res;
}

//
// Create CEER command string (called internally)
//
BOOL CTEBase::CreateQueryCEER(REQUEST_DATA& rReqData)
{
    RIL_LOG_VERBOSE("CTEBase::CreateQueryCEER() - Enter\r\n");
    BOOL bRet = FALSE;

    if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+CEER\r", sizeof(rReqData.szCmd1)))
    {
        RIL_LOG_CRITICAL("CTEBase::CreateQueryCEER() - Cannot create NEER command\r\n");
        goto Error;
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CTEBase::CreateQueryCEER() - Exit\r\n");
    return bRet;
}

//
// Parse response to CEER command (called internally)
//
RIL_RESULT_CODE CTEBase::ParseQueryCEER(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryCEER() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiCause = 0;

    if (ParseCEER(rRspData, uiCause))
    {
        RIL_LOG_INFO("CTEBase::ParseQueryCEER() - Cause= %d\r\n", uiCause);

        switch (uiCause)
        {
            /*
             * @TODO: Add cases for specific CEER causes and set the appropriate
             *        result code. If no result code is returned, will default
             *        to GENERIC_ERROR.
             */
        }
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryCEER() - Exit\r\n");
    return res;
}

//
// Parse Extended Error Report
//
BOOL CTEBase::ParseCEER(RESPONSE_DATA& rRspData, UINT32& rUICause)
{
    BOOL bRet = FALSE;
    char      szDummy[MAX_BUFFER_SIZE];
    const char* szRsp = rRspData.szResponse;

    if (FindAndSkipString(szRsp, "+CEER: ", szRsp))
    {
        bRet = TRUE;
        rUICause = 0;

        // skip first string
        if (!ExtractQuotedString(szRsp, szDummy, MAX_BUFFER_SIZE, szRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseCEER() - Could not find first string.\r\n");
            goto Error;
        }

        // Get failure cause (if it exists)
        if (SkipString(szRsp, ",", szRsp))
        {
            if (!ExtractUInt32(szRsp, rUICause, szRsp))
            {
                RIL_LOG_CRITICAL("CTEBase::ParseCEER() - Could not extract failure cause.\r\n");
                goto Error;
            }
        }

        //  Get verbose description (if it exists)
        if (SkipString(szRsp, ",", szRsp))
        {
            if (!ExtractQuotedString(szRsp, szDummy, MAX_BUFFER_SIZE, szRsp))
            {
                RIL_LOG_WARNING("CTEBase::ParseCEER() -"
                        " WARNING: Could not extract verbose cause.\r\n");
            }
            else if (!SkipRspEnd(szRsp, m_szNewLine, szRsp))
            {
                RIL_LOG_WARNING("CTEBase::ParseCEER() - WARNING: Could not extract RspEnd.\r\n");
            }
        }
        else if (!SkipRspEnd(szRsp, m_szNewLine, szRsp))
        {
            RIL_LOG_WARNING("CTEBase::ParseCEER() - WARNING: Could not extract RspEnd.\r\n");
        }
    }

Error:
    return bRet;
}

//
// QUERY SIM SMS Store Status (sent internally)
//
RIL_RESULT_CODE CTEBase::ParseQuerySimSmsStoreStatus(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQuerySimSmsStoreStatus() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    const char* pszRsp = rRspData.szResponse;
    char szMemStore[3];
    UINT32 uiUsed = 0, uiTotal = 0;
    int i = 0;
    //  The number of memory store information returned by AT+CPMS?.
    const int MAX_MEM_STORE_INFO = 3;

    // Parse "<prefix>+CPMS: <mem1>,<used1>,<total1>,<mem2>,<used2>,<total2>,
    // <mem3>,<used3>,<total3><postfix>"
    if (!SkipRspStart(pszRsp, m_szNewLine, pszRsp) ||
        !SkipString(pszRsp, "+CPMS: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQuerySimSmsStoreStatus() -"
                " Could not find AT response.\r\n");
        goto Error;
    }

    while (i < MAX_MEM_STORE_INFO)
    {
        if (!ExtractQuotedString(pszRsp, szMemStore, sizeof(szMemStore), pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQuerySimSmsStoreStatus() -"
                    " Could not extract <mem>.\r\n");
            goto Error;
        }

        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiUsed, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQuerySimSmsStoreStatus() -"
                    " Could not extract uiUsed.\r\n");
            goto Error;
        }

        if (!SkipString(pszRsp, ",", pszRsp) ||
            !ExtractUInt32(pszRsp, uiTotal, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseQuerySimSmsStoreStatus() -"
                    " Could not extract uiTotal.\r\n");
            goto Error;
        }

        if (0 == strcmp(szMemStore, "SM"))
        {
            if (uiUsed == uiTotal)
            {
                RIL_onUnsolicitedResponse(RIL_UNSOL_SIM_SMS_STORAGE_FULL, NULL, 0);
            }
            break;
        }

        SkipString(pszRsp, ",", pszRsp);

        i++;
    }

    if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQuerySimSmsStoreStatus() -"
                " Could not extract the response end.\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::ParseQuerySimSmsStoreStatus - Exit()\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParsePdpContextActivate(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParsePdpContextActivate() - Enter / Exit\r\n");

    return RRIL_RESULT_OK; // only supported at modem level
}

RIL_RESULT_CODE CTEBase::ParseEnterDataState(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseEnterDataState() - Enter / Exit\r\n");

    return RRIL_RESULT_OK; // only supported at modem level
}

RIL_RESULT_CODE CTEBase::ParseQueryIpAndDns(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryIpAndDns() - Enter / Exit\r\n");

    return RRIL_RESULT_OK; // only supported at modem level
}

void CTEBase::DeactivateAllDataCalls()
{
    RIL_LOG_VERBOSE("CTEBase::DeactivateAllDataCalls - Enter()\r\n");

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_DEACTIVATE_DATA_CALL].uiChannel,
            NULL, RIL_REQUEST_DEACTIVATE_DATA_CALL, "AT+CGACT=0\r",
            &CTE::ParseDeactivateAllDataCalls);

    if (pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::DeactivateAllDataCalls() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::DeactivateAllDataCalls() - "
                "Unable to allocate memory for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTEBase::DeactivateAllDataCalls - Exit()\r\n");
}

RIL_RESULT_CODE CTEBase::ParseDeactivateAllDataCalls(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseDeactivateAllDataCalls() - Enter\r\n");

    CleanupAllDataConnections();
    RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED, NULL, 0);

    RIL_LOG_VERBOSE("CTEBase::ParseDeactivateAllDataCalls() - Exit\r\n");
    return RRIL_RESULT_OK;
}

//
// Create NEER command string (called internally)
//
BOOL CTEBase::CreateQueryNEER(REQUEST_DATA& rReqData)
{
    RIL_LOG_VERBOSE("CTEBase::CreateQueryNEER() - Enter\r\n");
    BOOL bRet = FALSE;

    if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+NEER\r", sizeof(rReqData.szCmd1)))
    {
        RIL_LOG_CRITICAL("CTEBase::CreateQueryNEER() - Cannot create NEER command\r\n");
        goto Error;
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CTEBase::CreateQueryNEER() - Exit\r\n");
    return bRet;
}

//
// Parse response to NEER command (called internally)
//
RIL_RESULT_CODE CTEBase::ParseQueryNEER(RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryNEER() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    UINT32 uiCause = 0;

    if (ParseNEER(rRspData, uiCause))
    {
        RIL_LOG_INFO("CTEBase::ParseQueryNEER() - Cause= %d\r\n", uiCause);

        switch (uiCause)
        {
            /*
             * @TODO: Add cases for specific NEER causes and set the appropriate
             *        result code. If no result code is returned, will default
             *        to GENERIC_ERROR.
             */
        }
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryNEER() - Exit\r\n");
    return res;
}

//
// Parse Extended Error Report (called internally)
//
BOOL CTEBase::ParseNEER(RESPONSE_DATA& rRspData, UINT32& uiCause)
{
    RIL_LOG_VERBOSE("CTEBase::ParseNEER() - Enter\r\n");

    BOOL bRet = FALSE;
    const char* pszRsp = rRspData.szResponse;
    uiCause = 0;

    // Skip "+NEER: "
    if (!FindAndSkipString(pszRsp, "+NEER: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseNEER() - Could not find +NEER in response.\r\n");
        goto Error;
    }

    // Skip string upto "#"
    if (FindAndSkipString(pszRsp, "#", pszRsp))
    {
        if (!ExtractUInt32(pszRsp, uiCause, pszRsp))
        {
            RIL_LOG_CRITICAL("CTEBase::ParseNEER() - Could not extract failure cause.\r\n");
            goto Error;
        }
        RIL_LOG_INFO("CTEBase::ParseNEER() - Cause= %u\r\n", uiCause);
    }

    // Skip "<postfix>"
    if (!SkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseNEER() - Could not skip response postfix.\r\n");
        goto Error;
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CTEBase::ParseNEER() - Exit\r\n");
    return bRet;
}

void CTEBase::SetIncomingCallStatus(UINT32 uiCallId, UINT32 uiStatus)
{
    RIL_LOG_VERBOSE("CTEBase::SetIncomingCallStatus - Enter\r\n");

    m_IncomingCallInfo.callId = uiCallId;
    m_IncomingCallInfo.status = uiStatus;

    /*
     * If the status is ACTIVE/CONNECTED, then the Answer request is processed successfully
     * by the network.
     *
     * If the status is DISCONNECTED, either the user terminated the call or the
     * network terminated the call which means that ANSWER request is not valid anymore.
     */
    if (uiStatus == E_CALL_STATUS_ACTIVE || uiStatus == E_CALL_STATUS_CONNECTED ||
            uiStatus == E_CALL_STATUS_DISCONNECTED)
    {
        m_IncomingCallInfo.isAnswerReqSent = false;
    }

    RIL_LOG_VERBOSE("CTEBase::SetIncomingCallStatus - Exit()\r\n");
}

UINT32 CTEBase::GetIncomingCallId()
{
    RIL_LOG_VERBOSE("CTEBase::GetIncomingCallId - Enter/Exit \r\n");
    return m_IncomingCallInfo.callId;
}

RIL_RadioState CTEBase::GetRadioState()
{
    return m_RadioState.GetRadioState();
}

int CTEBase::GetSimCardState()
{
    return m_CardStatusCache.card_state;
}

int CTEBase::GetSimAppState()
{
    int appIndex = m_CardStatusCache.gsm_umts_subscription_app_index;
    if (appIndex >= 0)
    {
        return m_CardStatusCache.applications[appIndex].app_state;
    }

    return RIL_APPSTATE_UNKNOWN;
}

int CTEBase::GetSimPinState()
{
    int appIndex = m_CardStatusCache.gsm_umts_subscription_app_index;
    if (appIndex >= 0)
    {
        return m_CardStatusCache.applications[appIndex].app_state;
    }

    return RIL_PINSTATE_UNKNOWN;
}

void CTEBase::GetSimAppIdAndLabel(const int appType, char* pszAppId[], char* pszAppLabel[])
{
    RIL_LOG_VERBOSE("CTEBase::GetSimAppIdAndLabel() - Enter\r\n");

    for (int i = 0; i < m_SimAppListData.nApplications; i++)
    {
        if (appType == m_SimAppListData.aAppInfo[i].appType)
        {
            *pszAppId = strdup(m_SimAppListData.aAppInfo[i].szAid);
            *pszAppLabel = strdup(m_SimAppListData.aAppInfo[i].szAppLabel);
            break;
        }
    }

    RIL_LOG_VERBOSE("CTEBase::GetSimAppIdAndLabel() - Exit\r\n");
}

void CTEBase::GetSimAppId(const int appType, char* pszAppId, const int maxAppIdLength)
{
    RIL_LOG_VERBOSE("CTEBase::GetSimAppId) - Enter\r\n");

    for (int i = 0; i < m_SimAppListData.nApplications; i++)
    {
        if (appType == m_SimAppListData.aAppInfo[i].appType)
        {
            CopyStringNullTerminate(pszAppId, m_SimAppListData.aAppInfo[i].szAid, maxAppIdLength);
            break;
        }
    }

    RIL_LOG_VERBOSE("CTEBase::GetSimAppId() - Exit\r\n");
}

int CTEBase::GetIsimAppState()
{
    int appIndex = m_CardStatusCache.ims_subscription_app_index;
    if (appIndex > 0 && appIndex == ISIM_APP_INDEX)
    {
        return m_CardStatusCache.applications[appIndex].app_state;
    }

    return RIL_APPSTATE_UNKNOWN;
}

int CTEBase::GetSessionId(const int appType)
{
    for (int i = 0; i < m_SimAppListData.nApplications; i++)
    {
        if (appType == m_SimAppListData.aAppInfo[i].appType)
        {
            return m_SimAppListData.aAppInfo[i].sessionId;
        }
    }

    return -1;
}

void CTEBase::SetRadioState(const RRIL_Radio_State eRadioState)
{
    RIL_LOG_VERBOSE("CTEBase::SetRadioState() - Enter / Exit\r\n");

    m_RadioState.SetRadioState(eRadioState);
}

void CTEBase::SetRadioStateAndNotify(const RRIL_Radio_State eRadioState)
{
    RIL_LOG_VERBOSE("CTEBase::SetRadioStateAndNotify() - Enter / Exit\r\n");

    m_RadioState.SetRadioStateAndNotify(eRadioState);
}

void CTEBase::SetSimState(const int cardState, const int appState, const int pinState)
{
    RIL_LOG_VERBOSE("CTEBase::SetSimState() - Enter\r\n");

    CMutex::Lock(m_pCardStatusUpdateLock);

    m_CardStatusCache.card_state = (RIL_CardState)cardState;
    if (RIL_APPSTATE_UNKNOWN != appState)
    {
        /*
         * If the APP state is known, then there is an active application. In this
         * case, equate the number of applications to at least 1 and assume that
         * the active application is SIM or USIM.
         */
        m_CardStatusCache.num_applications =
                m_CardStatusCache.num_applications > 0 ? m_CardStatusCache.num_applications : 1;
        m_CardStatusCache.gsm_umts_subscription_app_index = SIM_USIM_APP_INDEX;
    }

    for (int i = 0; i < m_CardStatusCache.num_applications; i++)
    {
        m_CardStatusCache.applications[i].app_state = (RIL_AppState)appState;
        m_CardStatusCache.applications[i].pin1 = (RIL_PinState)pinState;
    }

    CMutex::Unlock(m_pCardStatusUpdateLock);

    RIL_LOG_VERBOSE("CTEBase::SetSimState() - Exit\r\n");
}

void CTEBase::SetSimAppState(const int appState)
{
    RIL_LOG_VERBOSE("CTEBase::SetSimAppState() - Enter\r\n");

    CMutex::Lock(m_pCardStatusUpdateLock);

    for (int i = 0; i < m_CardStatusCache.num_applications; i++)
    {
        m_CardStatusCache.applications[i].app_state = (RIL_AppState)appState;
    }

    CMutex::Unlock(m_pCardStatusUpdateLock);

    RIL_LOG_VERBOSE("CTEBase::SetSimAppState() - Exit\r\n");
}

void CTEBase::SetPersonalisationSubState(const int perso_substate)
{
    RIL_LOG_VERBOSE("CTEBase::SetPersonalisationSubState() - Enter\r\n");

    CMutex::Lock(m_pCardStatusUpdateLock);

    for (int i = 0; i < m_CardStatusCache.num_applications; i++)
    {
        m_CardStatusCache.applications[i].perso_substate = (RIL_PersoSubstate)perso_substate;
        if (RIL_PERSOSUBSTATE_UNKNOWN != perso_substate)
        {
            m_CardStatusCache.applications[i].app_state = RIL_APPSTATE_SUBSCRIPTION_PERSO;
            m_CardStatusCache.applications[i].pin1 = RIL_PINSTATE_ENABLED_NOT_VERIFIED;
        }
    }

    CMutex::Unlock(m_pCardStatusUpdateLock);

    RIL_LOG_VERBOSE("CTEBase::SetPersonalisationSubState() - Exit\r\n");
}

void CTEBase::SetSessionId(const int appType, const int sessionId)
{
    for (int i = 0; i < m_SimAppListData.nApplications; i++)
    {
        if (appType == m_SimAppListData.aAppInfo[i].appType)
        {
            m_SimAppListData.aAppInfo[i].sessionId = sessionId;
            break;
        }
    }
}

void CTEBase::UpdateIsimAppState()
{
    RIL_LOG_VERBOSE("CTEBase::UpdateIsimAppState() - Enter\r\n");

    CMutex::Lock(m_pCardStatusUpdateLock);

    m_CardStatusCache.applications[ISIM_APP_INDEX].app_type = RIL_APPTYPE_ISIM;
    m_CardStatusCache.applications[ISIM_APP_INDEX].app_state =
            m_CardStatusCache.applications[SIM_USIM_APP_INDEX].app_state;

    m_CardStatusCache.applications[ISIM_APP_INDEX].pin1 =
            m_CardStatusCache.applications[SIM_USIM_APP_INDEX].pin1;

    m_CardStatusCache.applications[ISIM_APP_INDEX].perso_substate =
            m_CardStatusCache.applications[SIM_USIM_APP_INDEX].perso_substate;

    CMutex::Unlock(m_pCardStatusUpdateLock);

    RIL_LOG_VERBOSE("CTEBase::UpdateIsimAppState() - Exit\r\n");
}

BOOL CTEBase::IsPinEnabled()
{
    RIL_LOG_VERBOSE("CTEBase::IsPinEnabled() - Enter\r\n");
    BOOL bPinEnabled = FALSE;

    for (int i = 0; i < m_CardStatusCache.num_applications; i++)
    {
        if (RIL_PINSTATE_ENABLED_NOT_VERIFIED == m_CardStatusCache.applications[i].pin1
                && RIL_APPSTATE_PIN == m_CardStatusCache.applications[i].app_state)
        {
            RIL_LOG_INFO("CTEBase::IsPinEnabled - PIN Enabled\r\n");
            bPinEnabled = TRUE;
            break;
        }
    }

    RIL_LOG_VERBOSE("CTEBase::IsPinEnabled() - Exit\r\n");
    return bPinEnabled;
}

BOOL CTEBase::HandleSilentPINEntry(void* /*pRilToken*/, void* /*pContextData*/,
                                                            int /*dataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::HandleSilentPINEntry - Enter/Exit \r\n");
    return FALSE; // only suported at modem level
}

//
// Silent Pin Entry (sent internally)
//
RIL_RESULT_CODE CTEBase::ParseSilentPinEntry(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSilentPinEntry - Enter/Exit \r\n");
    return RIL_E_REQUEST_NOT_SUPPORTED;  // only suported at modem level
}

RIL_RESULT_CODE CTEBase::QueryPinRetryCount(REQUEST_DATA& /*rReqData*/, void* /*pData*/,
        UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::QueryPinRetryCount() - Enter / Exit\r\n");
    return RIL_E_REQUEST_NOT_SUPPORTED;  // only suported at modem level
}

RIL_RESULT_CODE CTEBase::ParseSimPinRetryCount(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimPinRetryCount() - Enter / Exit\r\n");

    return RIL_E_REQUEST_NOT_SUPPORTED;  // only suported at modem level
}

void CTEBase::PostSetupDataCallCmdHandler(POST_CMD_HANDLER_DATA& /*rData*/)
{
    RIL_LOG_VERBOSE("CTEBase::PostSetupDataCallCmdHandler - Enter/Exit \r\n");
    // only suported at modem level
}

void CTEBase::PostPdpContextActivateCmdHandler(POST_CMD_HANDLER_DATA& /*rData*/)
{
    RIL_LOG_VERBOSE("CTEBase::PostPdpContextActivateCmdHandler - Enter/Exit \r\n");
    // only suported at modem level
}

void CTEBase::PostQueryIpAndDnsCmdHandler(POST_CMD_HANDLER_DATA& /*rData*/)
{
    RIL_LOG_VERBOSE("CTEBase::PostQueryIpAndDnsCmdHandler - Enter/Exit \r\n");
    // only suported at modem level
}

void CTEBase::PostEnterDataStateCmdHandler(POST_CMD_HANDLER_DATA& /*rData*/)
{
    RIL_LOG_VERBOSE("CTEBase::PostEnterDataStateCmdHandler - Enter/Exit \r\n");
    // only suported at modem level
}

void CTEBase::PostDeactivateDataCallCmdHandler(POST_CMD_HANDLER_DATA& /*rData*/)
{
    RIL_LOG_VERBOSE("CTEBase::PostDeactivateDataCallCmdHandler - Enter/Exit \r\n");
    // only suported at modem level
}

void CTEBase::CleanupAllDataConnections()
{
    RIL_LOG_VERBOSE("CTEBase::CleanupAllDataConnections() - Enter\r\n");

    //  Bring down all data contexts internally.
    CChannel_Data* pChannelData = NULL;

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        pChannelData = static_cast<CChannel_Data*>(g_pRilChannel[i]);

        //  We are taking down all data connections here, so we are looping over each data channel.
        //  Don't call DataConfigDown with invalid CID.
        if (pChannelData && pChannelData->GetContextID() > 0)
        {
            RIL_LOG_INFO("CTEBase::CleanupAllDataConnections() - Calling DataConfigDown  chnl=[%d],"
                    " cid=[%d]\r\n", i, pChannelData->GetContextID());
            DataConfigDown(pChannelData->GetContextID(), TRUE);
        }
    }

    RIL_LOG_VERBOSE("CTEBase::CleanupAllDataConnections() - Exit\r\n");
}

void CTEBase::HandleSetupDataCallSuccess(UINT32 uiCID, void* pRilToken)
{
    RIL_LOG_VERBOSE("CTEBase::HandleSetupDataCallSuccess() - Enter\r\n");

    RIL_Data_Call_Response_v6 dataCallResp;
    CChannel_Data* pChannelData = NULL;

    memset(&dataCallResp, 0, sizeof(RIL_Data_Call_Response_v6));
    dataCallResp.status = PDP_FAIL_ERROR_UNSPECIFIED;
    dataCallResp.suggestedRetryTime = -1;
    char szPdpType[MAX_PDP_TYPE_SIZE] = {'\0'};
    char szInterfaceName[MAX_INTERFACE_NAME_SIZE] = {'\0'};
    char szIPAddress[MAX_BUFFER_SIZE] = {'\0'};
    char szDNS[MAX_BUFFER_SIZE] = {'\0'};
    char szGateway[MAX_BUFFER_SIZE] = {'\0'};

    m_cte.SetupDataCallOngoing(FALSE);

    pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_CRITICAL("CTEBase::HandleSetupDataCallSuccess() -"
                " No Data Channel for CID %u.\r\n", uiCID);
        goto Error;
    }

    pChannelData->GetPdpType(szPdpType, sizeof(szPdpType));
    pChannelData->GetAddressString(szIPAddress, pChannelData->ADDR_IP, sizeof(szIPAddress));
    pChannelData->GetAddressString(szDNS, pChannelData->ADDR_DNS, sizeof(szDNS));
    pChannelData->GetAddressString(szGateway, pChannelData->ADDR_GATEWAY, sizeof(szGateway));
    pChannelData->GetInterfaceName(szInterfaceName, sizeof(szInterfaceName));

    dataCallResp.status = pChannelData->GetDataFailCause();
    dataCallResp.suggestedRetryTime = -1;
    dataCallResp.cid = uiCID;
    dataCallResp.active = 2;
    dataCallResp.type = szPdpType;
    dataCallResp.addresses = szIPAddress;
    dataCallResp.dnses = szDNS;
    dataCallResp.gateways = szGateway;
    dataCallResp.ifname = szInterfaceName;

    if (CRilLog::IsFullLogBuild())
    {
        RIL_LOG_INFO("status=%d suggRetryTime=%d cid=%d active=%d type=\"%s\" ifname=\"%s\""
                " addresses=\"%s\" dnses=\"%s\" gateways=\"%s\"\r\n",
                dataCallResp.status, dataCallResp.suggestedRetryTime,
                dataCallResp.cid, dataCallResp.active,
                dataCallResp.type, dataCallResp.ifname,
                dataCallResp.addresses, dataCallResp.dnses,
                dataCallResp.gateways);
    }

Error:
    if (NULL != pRilToken)
    {
        RIL_onRequestComplete(pRilToken, RIL_E_SUCCESS, &dataCallResp,
                sizeof(RIL_Data_Call_Response_v6));
    }

    RIL_LOG_VERBOSE("CTEBase::HandleSetupDataCallSuccess() - Exit\r\n");
}

void CTEBase::HandleSetupDataCallFailure(UINT32 uiCID, void* pRilToken, UINT32 /*uiResultCode*/)
{
    RIL_LOG_VERBOSE("CTEBase::HandleSetupDataCallFailure() - Enter\r\n");

    int state;
    int failCause = PDP_FAIL_ERROR_UNSPECIFIED;

    m_cte.SetupDataCallOngoing(FALSE);

    CChannel_Data* pChannelData = CChannel_Data::GetChnlFromContextID(uiCID);
    if (NULL == pChannelData)
    {
        RIL_LOG_INFO("CTEBase::HandleSetupDataCallFailure() -"
                " No data channel for CID: %u\r\n", uiCID);
        goto Complete;
    }

    state = pChannelData->GetDataState();
    failCause = pChannelData->GetDataFailCause();
    if (PDP_FAIL_NONE == failCause)
    {
        failCause = PDP_FAIL_ERROR_UNSPECIFIED;
    }

    RIL_LOG_INFO("CTEBase::HandleSetupDataCallFailure() - state: %d\r\n", state);

    switch (state)
    {
        case E_DATA_STATE_ACTIVE:
            /*
             * @TODO: Delay the completion of ril request till the
             * data call is deactivated successfully?
             */
            RIL_requestTimedCallback(triggerDeactivateDataCall, (void*)(intptr_t)uiCID, 0, 0);
            break;
        default:
            DataConfigDown(uiCID, TRUE);
            break;
    }

Complete:
    if (NULL != pRilToken)
    {
        RIL_Data_Call_Response_v6 dataCallResp;
        memset(&dataCallResp, 0, sizeof(RIL_Data_Call_Response_v6));
        dataCallResp.status = failCause;
        dataCallResp.suggestedRetryTime = -1;
        RIL_onRequestComplete(pRilToken, RIL_E_SUCCESS, &dataCallResp,
                sizeof(RIL_Data_Call_Response_v6));
    }

    RIL_LOG_VERBOSE("CTEBase::HandleSetupDataCallFailure() - Exit\r\n");
}


//
//  Call this function whenever data is activated
//
BOOL CTEBase::DataConfigUp(char* pszNetworkInterfaceName, CChannel_Data* pChannelData,
                                                PDP_TYPE eDataConnectionType)
{
    BOOL bRet = FALSE;
    RIL_LOG_INFO("CTEBase::DataConfigUp() ENTER\r\n");

    switch(eDataConnectionType)
    {
        case PDP_TYPE_IPV4:
            RIL_LOG_INFO("CTEBase::DataConfigUp() - IPV4 - Calling DataConfigUpIpV4()\r\n");
            bRet = DataConfigUpIpV4(pszNetworkInterfaceName, pChannelData);
            break;

        case PDP_TYPE_IPV6:
            RIL_LOG_INFO("CTEBase::DataConfigUp() - IPV6 - Calling DataConfigUpIpV6()\r\n");
            bRet = DataConfigUpIpV6(pszNetworkInterfaceName, pChannelData);
            break;

        case PDP_TYPE_IPV4V6:
            RIL_LOG_INFO("CTEBase::DataConfigUp() - IPV4V6 - Calling DataConfigUpIpV4V6()\r\n");
            bRet = DataConfigUpIpV4V6(pszNetworkInterfaceName, pChannelData);
            break;

        default:
            RIL_LOG_CRITICAL("CTEBase::DataConfigUp() -"
                    " Unknown PDP_TYPE!  eDataConnectionType=[%d]\r\n",
                    eDataConnectionType);
            bRet = FALSE;
            break;
    }

    RIL_LOG_INFO("CTEBase::DataConfigUp() EXIT=%d\r\n", bRet);
    return bRet;
}

BOOL CTEBase::DataConfigUpIpV4(char* pszNetworkInterfaceName, CChannel_Data* pChannelData)
{
    BOOL bRet = FALSE;
    int s = -1;
    char szIpAddr[2*MAX_IPADDR_SIZE + 1] = {'\0'};
    char szGatewayAddr[2*MAX_IPADDR_SIZE + 1] = {'\0'};

    if (NULL == pChannelData || NULL == pszNetworkInterfaceName)
    {
        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4() - Invalid input\r\n");
        goto Error;
    }

    pChannelData->GetAddressString(szIpAddr, pChannelData->ADDR_IP, sizeof(szIpAddr));

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV4() ENTER  pszNetworkInterfaceName=[%s]"
            "  szIpAddr=[%s]\r\n",
            pszNetworkInterfaceName, szIpAddr);

    //  Open socket for ifconfig command
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4() : cannot open control socket\n");
        goto Error;
    }

    //  Code in this function is from system/core/toolbox/ifconfig.c
    //  also from system/core/toolbox/route.c

    //  ifconfig rmnetX <ip address>
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, pszNetworkInterfaceName, IFNAMSIZ-1);
        ifr.ifr_name[IFNAMSIZ-1] = '\0';  //  KW fix

        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4() : Setting flags\r\n");
        if (!setflags(s, &ifr, IFF_UP | IFF_POINTOPOINT | IFF_NOARP, 0))
        {
            //goto Error;
            RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4() : Error setting flags\r\n");
        }

        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4() : Setting addr\r\n");
        if (!setaddr(s, &ifr, szIpAddr)) // ipaddr
        {
            //goto Error;
            RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4() : Error setting addr\r\n");
        }
    }

    pChannelData->GetAddressString(szGatewayAddr,
            pChannelData->ADDR_GATEWAY, sizeof(szGatewayAddr));

    if (0 == strlen(szGatewayAddr))
    {
        in_addr_t gw;
        struct in_addr gwaddr;
        in_addr_t addr;

        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4() : set default gateway to fake value\r\n");
        if (inet_pton(AF_INET, szIpAddr, &addr) <= 0)
        {
            RIL_LOG_INFO("CTEBase::DataConfigUpIpV4() : inet_pton() failed for %s!\r\n", szIpAddr);
            goto Error;
        }
        gw = ntohl(addr) & 0xFFFFFF00;
        gw |= 1;
        gwaddr.s_addr = htonl(gw);

        pChannelData->AddAddressString(pChannelData->ADDR_GATEWAY, inet_ntoa(gwaddr));
    }

    bRet = TRUE;

Error:
    if (s >= 0)
    {
        close(s);
    }

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV4() EXIT  bRet=[%d]\r\n", bRet);
    return bRet;
}

BOOL CTEBase::DataConfigUpIpV6(char* pszNetworkInterfaceName, CChannel_Data* pChannelData)
{
    BOOL bRet = FALSE;
    int s = -1;
    char szIpAddr2[2*MAX_IPADDR_SIZE + 1] = {'\0'};
    char szIpAddrOut[50];
    struct in6_addr ifIdAddr;
    struct in6_addr ifPrefixAddr;
    struct in6_addr ifOutAddr;

    if (NULL == pChannelData || NULL == pszNetworkInterfaceName)
    {
        RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() - Invalid input\r\n");
        goto Error;
    }

    pChannelData->GetAddressString(szIpAddr2, pChannelData->ADDR_IP, sizeof(szIpAddr2));

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() ENTER  pszNetworkInterfaceName=[%s]"
            "  szIpAddr2=[%s]\r\n",
            pszNetworkInterfaceName, szIpAddr2);

    //  Open socket for ifconfig command (note this is ipv6 socket)
    s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV6() : cannot open control socket\n");
        goto Error;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, pszNetworkInterfaceName, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';  //  KW fix

    inet_pton(AF_INET6, szIpAddr2, &ifIdAddr);
    inet_pton(AF_INET6, "FE80::", &ifPrefixAddr);


    // Set local prefix from FE80::
    memcpy(ifOutAddr.s6_addr,ifPrefixAddr.s6_addr,8);
    // Set interface identifier from address given by network
    memcpy((ifOutAddr.s6_addr)+8,(ifIdAddr.s6_addr)+8,8);

    inet_ntop(AF_INET6, &ifOutAddr, szIpAddrOut, sizeof(szIpAddrOut));
    strncpy(szIpAddr2, szIpAddrOut, sizeof(szIpAddrOut));

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() : Setting flags\r\n");
    if (!setflags(s, &ifr, IFF_UP | IFF_POINTOPOINT | IFF_NOARP, 0))
    {
        //goto Error;
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV6(): Error setting flags\r\n");
    }

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() : Setting addr :%s\r\n",szIpAddr2);
    if (!setaddr6(s, &ifr, szIpAddr2))
    {
        //goto Error;
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV6() : Error setting addr %s\r\n",
                                                                szIpAddr2);
    }

    //  Before setting interface UP, need to deactivate DAD on interface.
    char file_to_open[100];
    file_to_open[0]='\0';
    FILE * fp;

    //  Open dad_transmits file, write 0<lf>.
    snprintf(file_to_open, sizeof(file_to_open), "/proc/sys/net/ipv6/conf/%s/dad_transmits",
            pszNetworkInterfaceName);

    fp = fopen(file_to_open, "w");
    if (fp)
    {
        char szData[] = "0\n";

        RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() : Opened file=[%s]\r\n", file_to_open);
        if (EOF == fputs(szData, fp))
        {
            RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV6() : file=[%s] cannot write value [%s]\r\n",
                                                                    file_to_open, szData);
        }
        else
        {
            RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() : Wrote [%s] to file=[%s]\r\n",
                    CRLFExpandedString(szData, strlen(szData)).GetString(), file_to_open);
        }

        //  Close file handle
        fclose(fp);
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV6() : Cannot open [%s]\r\n", file_to_open);
    }
    //  Open accept_dad file, write 0<lf>.
    snprintf(file_to_open, sizeof(file_to_open), "/proc/sys/net/ipv6/conf/%s/accept_dad",
            pszNetworkInterfaceName);

    fp = fopen(file_to_open, "w");
    if (fp)
    {
        char szData[] = "0\n";

        RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() : Opened file=[%s]\r\n", file_to_open);
        if (EOF == fputs(szData, fp))
        {
            RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV6() : file=[%s] cannot write value [%s]\r\n",
                                                                    file_to_open, szData);
        }
        else
        {
            RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() : Wrote [%s] to file=[%s]\r\n",
                    CRLFExpandedString(szData, strlen(szData)).GetString(), file_to_open);
        }

        //  Close file handle.
        fclose(fp);
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV6() : Cannot open [%s]\r\n", file_to_open);
    }

    bRet = TRUE;

Error:
    if (s >= 0)
    {
        close(s);
    }

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV6() EXIT  bRet=[%d]\r\n", bRet);
    return bRet;
}

BOOL CTEBase::DataConfigUpIpV4V6(char* pszNetworkInterfaceName,
                                            CChannel_Data* pChannelData)
{
    BOOL bRet = FALSE;
    int s = -1;
    int s6 =-1;
    char szIpAddrOut[50];
    struct in6_addr ifIdAddr;
    struct in6_addr ifPrefixAddr;
    struct in6_addr ifOutAddr;
    char* pszIpAddr = NULL;
    char* pszIpAddr2 = NULL;
    char szIpAddrTemp[2*MAX_IPADDR_SIZE + 1] = {'\0'};

    if (NULL == pChannelData || NULL == pszNetworkInterfaceName)
    {
        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() - Invalid input\r\n");
        goto Error;
    }

    pChannelData->GetAddressString(szIpAddrTemp, pChannelData->ADDR_IP, sizeof(szIpAddrTemp));

    // IP addresses string : "IPV4 IPV6"
    pszIpAddr = szIpAddrTemp;
    pszIpAddr2 = strstr(szIpAddrTemp, " ");
    if (NULL == pszIpAddr2)
    {
        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() - Cannot extract pszIpAddr2\r\n");
        goto Error;
    }
    *pszIpAddr2 = '\0';
    pszIpAddr2++;

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() ENTER  pszNetworkInterfaceName=[%s]  pszIpAddr=[%s]"
            " pszIpAddr2=[%s]\r\n",
            pszNetworkInterfaceName, pszIpAddr, pszIpAddr2);

    //  Open socket for ifconfig and setFlags commands
    s = socket(AF_INET, SOCK_DGRAM, 0);
    s6 = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0 || s6 < 0)
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4V6() : cannot open control socket\n");
        goto Error;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, pszNetworkInterfaceName, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';  //  KW fix

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() : Setting addr\r\n");
    if (!setaddr(s, &ifr, pszIpAddr))
    {
        //goto Error;
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4V6() : Error setting add\r\n");
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, pszNetworkInterfaceName, IFNAMSIZ-1);
    ifr.ifr_name[IFNAMSIZ-1] = '\0';  //  KW fix

    // Set link local address to start the SLAAC process
    inet_pton(AF_INET6, pszIpAddr2, &ifIdAddr);
    inet_pton(AF_INET6, "FE80::", &ifPrefixAddr);

    // Set local prefix from FE80::
    memcpy(ifOutAddr.s6_addr, ifPrefixAddr.s6_addr, 8);
    // Set interface identifier from address given by network
    memcpy((ifOutAddr.s6_addr)+8, (ifIdAddr.s6_addr)+8, 8);

    inet_ntop(AF_INET6, &ifOutAddr, szIpAddrOut, sizeof(szIpAddrOut));
    strncpy(pszIpAddr2, szIpAddrOut, sizeof(szIpAddrOut));

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() : Setting flags\r\n");
    if (!setflags(s, &ifr, IFF_UP | IFF_POINTOPOINT | IFF_NOARP, 0))
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4V6() : Error setting flags\r\n");
    }

    if (!setaddr6(s6, &ifr, pszIpAddr2))
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4V6() : Error setting add\r\n");
    }

    //  Before setting interface UP, need to deactivate DAD on interface.
    char file_to_open[100];
    file_to_open[0]='\0';
    FILE* fp;

    //  Open dad_transmits file, write 0<lf>.
    snprintf(file_to_open, sizeof(file_to_open), "/proc/sys/net/ipv6/conf/%s/dad_transmits",
            pszNetworkInterfaceName);

    fp = fopen(file_to_open, "w");
    if (fp)
    {
        char szData[] = "0\n";

        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() : Opened file=[%s]\r\n", file_to_open);
        if (EOF == fputs(szData, fp))
        {
            RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4V6() : file=[%s] cannot write value"
                    " [%s]\r\n", file_to_open, szData);
        }
        else
        {
            RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() : Wrote [%s] to file=[%s]\r\n",
                    CRLFExpandedString(szData, strlen(szData)).GetString(), file_to_open);
        }

        //  Close file handle
        fclose(fp);
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4V6() : Cannot open [%s]\r\n",
                                                            file_to_open);
    }

    //  Open accept_dad file, write 0<lf>.
    snprintf(file_to_open, sizeof(file_to_open), "/proc/sys/net/ipv6/conf/%s/accept_dad",
            pszNetworkInterfaceName);

    fp = fopen(file_to_open, "w");
    if (fp)
    {
        char szData[] = "0\n";

        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() : Opened file=[%s]\r\n", file_to_open);
        if (EOF == fputs(szData, fp))
        {
            RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4V6() : file=[%s] cannot write value"
                    " [%s]\r\n", file_to_open, szData);
        }
        else
        {
            RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() : Wrote [%s] to file=[%s]\r\n",
                    CRLFExpandedString(szData, strlen(szData)).GetString(), file_to_open);
        }

        //  Close file handle.
        fclose(fp);
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::DataConfigUpIpV4V6() : Cannot open [%s]\r\n", file_to_open);
    }

    in_addr_t gw;
    struct in_addr gwaddr;
    in_addr_t addr;

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() : set default gateway to fake value\r\n");
    if (inet_pton(AF_INET, pszIpAddr, &addr) <= 0)
    {
        RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() : inet_pton() failed for %s!\r\n", pszIpAddr);
        goto Error;
    }
    gw = ntohl(addr) & 0xFFFFFF00;
    gw |= 1;
    gwaddr.s_addr = htonl(gw);

    // First clear GATEWAY addresses
    pChannelData->DeleteAddressesString(pChannelData->ADDR_GATEWAY);
    // Add GATEWAY address
    pChannelData->AddAddressString(pChannelData->ADDR_GATEWAY, inet_ntoa(gwaddr));

    bRet = TRUE;

Error:
    if (s >= 0)
    {
        close(s);
    }

    if (s6 >= 0)
    {
        close(s6);
    }

    RIL_LOG_INFO("CTEBase::DataConfigUpIpV4V6() EXIT  bRet=[%d]\r\n", bRet);
    return bRet;
}

RIL_RadioTechnology CTEBase::MapAccessTechnology(UINT32 uiStdAct, int regType)
{
    RIL_LOG_VERBOSE("CTEBase::MapAccessTechnology() ENTER "
            "uiStdAct=[%u] regType=[%u]\r\n", uiStdAct, regType);

    /*
     * When the network type from the CS registration (+CREG: ...<Act>...) is to be mapped
     * we are mapping to the following
     * In 2G: All Act types are mapped as RADIO_TECH_GSM
     * In 3G : All Act values are mapped to RADIO_TECH_UMTS
     * In LTE: All Act values are mapped to RADIO_TECH_LTE
     * This results in the telephony framework getting the correct voice reg network type
     * to correctly handle the voice-data concurrency.
     */

    RIL_RadioTechnology rtAct = RADIO_TECH_UNKNOWN;

    switch (regType)
    {
        case E_REGISTRATION_TYPE_CREG:
            switch (uiStdAct)
            {
                case 0: // GSM
                case 1: // GSM Compact
                case 3: // GSM w/EGPRS
                    rtAct = RADIO_TECH_GSM;
                    break;

                case 2: // UTRAN
                case 4: // UTRAN w/HSDPA
                case 5: // UTRAN w/HSUPA
                case 6: // UTRAN w/HSDPA and HSUPA
                case 8: // PS Emergency Only
                    rtAct = RADIO_TECH_UMTS;
                    break;

                case 7: // E-UTRAN
                    rtAct = RADIO_TECH_LTE; // 14
                    break;

                default:
                    rtAct = RADIO_TECH_UNKNOWN; // 0
                    break;
            }
        break;

    /*
     * 20111103: There is no 3GPP standard value defined for GPRS and HSPA+
     * access technology. So, values 1 and 8 are used in relation with the
     * IMC proprietary +XREG: <Act> parameter.
     *
     * Note: GSM Compact is not supported by IMC modem.
     */
        case E_REGISTRATION_TYPE_XREG:
        case E_REGISTRATION_TYPE_CGREG:
        case E_REGISTRATION_TYPE_CEREG:
            switch (uiStdAct)
            {
                case 0: // GSM
                    rtAct = RADIO_TECH_UNKNOWN; // 0 - GSM access technology
                    break;

                case 1: // GSM Compact
                    rtAct = RADIO_TECH_GPRS; // 1
                    break;

                case 2: // UTRAN
                    rtAct = RADIO_TECH_UMTS; // 3
                    break;

                case 3: // GSM w/EGPRS
                    rtAct = RADIO_TECH_EDGE; // 2
                    break;

                case 4: // UTRAN w/HSDPA
                    rtAct = RADIO_TECH_HSDPA; // 9
                    break;

                case 5: // UTRAN w/HSUPA
                    rtAct = RADIO_TECH_HSUPA; // 10
                    break;

                case 6: // UTRAN w/HSDPA and HSUPA
                    rtAct = RADIO_TECH_HSPA; // 11
                    break;

                case 7: // E-UTRAN
                    rtAct = RADIO_TECH_LTE; // 14
                    break;

                case 8: // PS Emergency Only
                    rtAct = RADIO_TECH_HSPAP; // 15
                    break;

                default:
                    rtAct = RADIO_TECH_UNKNOWN; // 0
                    break;
            }
            break;
        default:
            rtAct = RADIO_TECH_UNKNOWN;
            break;
    }

    RIL_LOG_VERBOSE("CTEBase::MapAccessTechnology() EXIT rtAct=[%u]\r\n", (UINT32)rtAct);
    return rtAct;
}

RIL_RESULT_CODE CTEBase::HandleScreenStateReq(int /*screenState*/)
{
    // should be derived in modem specific class
    return RRIL_RESULT_OK;
}

int CTEBase::GetCurrentCallId()
{
    for (UINT32 i = 0; i < RRIL_MAX_CALL_ID_COUNT; i++)
    {
        if (E_CALL_STATUS_DIALING == m_VoiceCallInfo[i].state
                || E_CALL_STATUS_ALERTING == m_VoiceCallInfo[i].state
                || E_CALL_STATUS_ACTIVE == m_VoiceCallInfo[i].state
                || E_CALL_STATUS_CONNECTED == m_VoiceCallInfo[i].state)
        {
            return m_VoiceCallInfo[i].id;
        }
    }

    return -1;
}

BOOL CTEBase::IsDtmfAllowed(int callId)
{
    for (UINT32 i = 0; i < RRIL_MAX_CALL_ID_COUNT; i++)
    {
        if (callId == m_VoiceCallInfo[i].id)
        {
            return m_VoiceCallInfo[i].bDtmfAllowed;
        }
    }

    return FALSE;
}

BOOL CTEBase::IsInCall()
{
    BOOL bIsInCall = FALSE;

    for (UINT32 i = 0; i < RRIL_MAX_CALL_ID_COUNT; i++)
    {
        if (-1 != m_VoiceCallInfo[i].state
                && E_CALL_STATUS_DISCONNECTED != m_VoiceCallInfo[i].state)
        {
            bIsInCall = TRUE;
            break;
        }
    }

    return bIsInCall;
}

void CTEBase::SetDtmfAllowed(int callId, BOOL bDtmfAllowed)
{
    for (UINT32 i = 0; i < RRIL_MAX_CALL_ID_COUNT; i++)
    {
        if (callId == m_VoiceCallInfo[i].id)
        {
            m_VoiceCallInfo[i].bDtmfAllowed = bDtmfAllowed;
            break;
        }
    }
}

BOOL CTEBase::GetRadioPowerCommand(BOOL /*bTurnRadioOn*/, int /*radioOffReason*/,
        char* /*pCmdBuffer*/, int /*cmdBufferLen*/)
{
    // should be derived in modem specific class
    return FALSE;
}

RIL_RESULT_CODE CTEBase::CreateIMSRegistrationReq(REQUEST_DATA& /*rReqData*/,
        const char** /*ppszRequest*/,
        const UINT32 /*uiDataSize*/)
{
    RIL_LOG_VERBOSE("CTEBase::CreateIMSRegistrationReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_NOTSUPPORTED;
    RIL_LOG_VERBOSE("CTEBase::CreateIMSRegistrationReq() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::CreateIMSConfigReq(REQUEST_DATA& /*rReqData*/,
        const char** /*ppszRequest*/,
        const int /*nNumStrings*/)
{
    RIL_LOG_VERBOSE("CTEBase::CreateIMSConfigReq() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_NOTSUPPORTED;
    RIL_LOG_VERBOSE("CTEBase::CreateIMSConfigReq() - Exit\r\n");
    return res;
}

void CTEBase::CheckImeiBlacklist(char* szImei)
{
    CRepository repository;
    char szImeiBlacklist[MAX_BUFFER_SIZE] = {'\0'};

    if (NULL != szImei && szImei[0] != '\0')
    {
        // Read IMEI blacklist from repository
        if (!repository.Read(g_szGroupModem, g_szImeiBlackList,
                szImeiBlacklist, MAX_BUFFER_SIZE))
        {
            RIL_LOG_INFO("CTEBase::CheckImeiBlacklist() - No IMEI Blacklist found in "
                    "repository\r\n");
            return;
        }

        // Get first IMEI in blacklist
        char* imei = strtok(szImeiBlacklist, " ");

        while (NULL != imei)
        {
            // Compare IMEI with one in the blacklist
            if (0 == strncmp(imei, szImei, strlen(szImei)))
            {
                RIL_LOG_CRITICAL("#############################################################"
                        "#############\r\n");
                RIL_LOG_CRITICAL("##                   WARNING  !!!                            "
                        "           ##\r\n");
                RIL_LOG_CRITICAL("##  DEFAULT FLASHED IMEI, it may be impossible to camp on "
                        "live network  ##\r\n");
                RIL_LOG_CRITICAL("#############################################################"
                        "#############\r\n");

                return;
            }

            // Get any other IMEIs
            imei = strtok(NULL, " ");
        }
    }
}

RIL_RESULT_CODE CTEBase::HandleSetupDefaultPDN(RIL_Token /*rilToken*/,
        CChannel_Data* /*pChannelData*/)
{
    RIL_LOG_VERBOSE("CTEBase::HandleSetupDefaultPDN() - Enter / Exit\r\n");
    return RIL_E_REQUEST_NOT_SUPPORTED;  // only suported at modem level
}

RIL_RESULT_CODE CTEBase::ParseSetupDefaultPDN(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSetupDefaultPDN - Enter/Exit \r\n");
    return RIL_E_REQUEST_NOT_SUPPORTED;  // only suported at modem level
}

void CTEBase::PostSetupDefaultPDN(POST_CMD_HANDLER_DATA& /*rData*/)
{
    RIL_LOG_VERBOSE("CTEBase::PostSetupDefaultPDN - Enter/Exit \r\n");
    // only suported at modem level
}

BOOL CTEBase::SetupInterface(UINT32 /*uiCID*/)
{
    RIL_LOG_VERBOSE("CTEBase::SetupInterface - Enter/Exit \r\n");
    // only suported at modem level
    return FALSE;
}

void CTEBase::HandleInternalDtmfStopReq()
{
    RIL_LOG_VERBOSE("CTEBase::HandleInternalDtmfStopReq() - Enter/Exit\r\n");
    // should be derived in modem specific class
}

RIL_RESULT_CODE CTEBase::CreateSetDefaultApnReq(REQUEST_DATA& /*rReqData*/,
        const char** /*ppszRequest*/, const int /*nNumStrings*/)
{
    RIL_LOG_VERBOSE("CTEBase::CreateSetDefaultApnReq() - Enter/Exit\r\n");
    return RRIL_RESULT_NOTSUPPORTED;
}

void CTEBase::HandleChannelsBasicInitComplete()
{
    RIL_LOG_VERBOSE("CTEBase::HandleChannelsBasicInitComplete() - Enter/Exit\r\n");
    // should be derived in modem specific class
}

RIL_RESULT_CODE CTEBase::ParseSimStateQuery(RESPONSE_DATA& /*rRspData*/)
{
    RIL_LOG_VERBOSE("CTEBase::ParseSimStateQuery() - Enter/Exit\r\n");
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

void CTEBase::HandleChannelsUnlockInitComplete()
{
    RIL_LOG_VERBOSE("CTEBase::HandleChannelsUnlockInitComplete() - Enter/Exit\r\n");
}

void CTEBase::QuerySimSmsStoreStatus()
{
    RIL_LOG_VERBOSE("CTEBase::QuerySimSmsStoreStatus() - Enter\r\n");

    CCommand* pCmd = new CCommand(
            g_ReqInternal[E_REQ_IDX_QUERY_SIM_SMS_STORE_STATUS].reqInfo.uiChannel,
            NULL, g_ReqInternal[E_REQ_IDX_QUERY_SIM_SMS_STORE_STATUS].reqId, "AT+CPMS?\r",
            &CTE::ParseQuerySimSmsStoreStatus);

    if (NULL != pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::QuerySimSmsStoreStatus() - "
                    "Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::QuerySimSmsStoreStatus() - Unable to allocate memory"
                " for new command!\r\n");
    }

    RIL_LOG_VERBOSE("CTEBase::QuerySimSmsStoreStatus() - Exit\r\n");
}

void CTEBase::NotifyNetworkApnInfo()
{
    RIL_LOG_VERBOSE("CTEBase::NotifyNetworkApnInfo() - Enter\r\n");

    CChannel_Data* pChannelData =
            CChannel_Data::GetChnlFromContextID(m_cte.GetDefaultPDNCid());

    if (NULL == pChannelData)
    {
        return;
    }

    sOEM_HOOK_RAW_UNSOL_NETWORK_APN_IND data;

    memset(&data, 0, sizeof(sOEM_HOOK_RAW_UNSOL_NETWORK_APN_IND));

    data.command = RIL_OEM_HOOK_RAW_UNSOL_NETWORK_APN_IND;
    pChannelData->GetApn(data.szApn, MAX_APN_SIZE);
    data.apnLength = strlen(data.szApn);
    pChannelData->GetPdpType(data.szPdpType, MAX_PDP_TYPE_SIZE);
    data.pdpTypeLength = strlen(data.szPdpType);

    RIL_onUnsolicitedResponse (RIL_UNSOL_OEM_HOOK_RAW, (void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_NETWORK_APN_IND));

    RIL_LOG_VERBOSE("CTEBase::NotifyNetworkApnInfo() - Exit\r\n");
}

RIL_RESULT_CODE CTEBase::HandleReleaseModemReq(REQUEST_DATA& /*reqData*/, const char** ppszRequest,
        const UINT32 uiDataSize)
{
    RIL_LOG_VERBOSE("CTEBase::HandleReleaseModemReq() - Enter\r\n");
    int reason;
    int data[1] = {RADIO_POWER_OFF};
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (uiDataSize < (2 * sizeof(char *)))
    {
        RIL_LOG_CRITICAL("CTEBase::HandleReleaseModemReq() :"
                " received_size < required_size\r\n");
        goto Error;
    }

    if (sscanf(ppszRequest[1], "%d", &reason) == EOF)
    {
        RIL_LOG_CRITICAL("CTEBase::HandleReleaseModemReq() -"
                " cannot convert %s to int\r\n", ppszRequest[1]);
        goto Error;
    }

    m_cte.SetRadioOffReason(reason);
    if (E_RADIO_OFF_REASON_SHUTDOWN == reason)
    {
        HandleShutdownReq(RIL_REQUEST_RADIO_POWER);
    }
    else if (E_RADIO_OFF_REASON_AIRPLANE_MODE == reason && RADIO_STATE_OFF == GetRadioState()
            && m_cte.GetModemOffInFlightModeState())
    {
        CSystemManager::GetInstance().ReleaseModem();
    }

    res = RRIL_RESULT_OK_IMMEDIATE;
Error:
    RIL_LOG_VERBOSE("CTEBase::HandleReleaseModemReq() - Exit\r\n");
    return res;
}

void CTEBase::WaitForModemPowerOffEvent()
{
    /*
     * In case of platform shutdown, wait for modem powered off event.
     * Modem powered off event will be signalled on MODEM_DOWN event.
     *
     * Platform shutdown sequence is as follows:
     *     - Modem power off request is received with property
     *       sys.shutdown.requested set to 0 or 1.
     *     - Add modem specific RADIO_POWER OFF commands added to command
     *       queue and wait for modem powered off event.
     *     - Channel specific command thread will sent the command to modem.
     *     - Upon response, send modem shutdown request to MMGR.
     *     - Upon MODEM_SHUTDOWN notification modem event, acknowledge
     *     - Upon MODEM_DOWN event, set the radio state to OFF and signal
     *       modem powered off.
     *     - Modem powered off event will unblock the ril request handling
     *       thread. Once unblocked, respective modem power off ril request
     *       will be completed.
     */
    CEvent* pModemPoweredOffEvent =
            CSystemManager::GetInstance().GetModemPoweredOffEvent();
    if (NULL != pModemPoweredOffEvent)
    {
        CEvent::Reset(pModemPoweredOffEvent);
        CEvent::Wait(pModemPoweredOffEvent, WAIT_FOREVER);
    }
}
void CTEBase::PostInternalDtmfStopReq(POST_CMD_HANDLER_DATA& /*rData*/)
{
    RIL_LOG_VERBOSE("CTEBase::PostInternalDtmfStopReq() Enter\r\n");
    CEvent::Signal(m_pDtmfStopReqEvent);
    RIL_LOG_VERBOSE("CTEBase::PostInternalDtmfStopReq() Exit\r\n");
}

void CTEBase::QuerySignalStrength()
{
    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_SIGNAL_STRENGTH].uiChannel, NULL,
            RIL_REQUEST_SIGNAL_STRENGTH, "AT+CSQ\r", &CTE::ParseUnsolicitedSignalStrength);

    if (pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::QuerySignalStrength() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::QuerySignalStrength() - "
                "Unable to allocate memory for new command!\r\n");
    }
}

RIL_SignalStrength* CTEBase::ParseXCESQ(const char*& /*rszPointer*/, const BOOL /*bUnsolicited*/)
{
    return NULL;
}

void CTEBase::QueryUiccInfo()
{
    // should be derived in modem specific class
    // do nothing
}

RIL_RESULT_CODE CTEBase::ParseQueryActiveApplicationType(RESPONSE_DATA& /*rRspData*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::ParseQueryAvailableApplications(RESPONSE_DATA& /*rRspData*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::ParseQueryIccId(RESPONSE_DATA& /*rRspData*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

BOOL CTEBase::ParseEFdir(const char* pszResponseString, const UINT32 uiResponseStringLen)
{
    RIL_LOG_VERBOSE("CTEBase::ParseEFdir() - Enter\r\n");

    const UINT32 MANDATORY_LENGTH = 4;
    BYTE* pBuffer = NULL;
    UINT32 uiByteBufferLength = 0;
    BYTE* pTemp = NULL;
    UINT32 uiAidLength = 0;
    UINT32 uiAppLabelLength = 0;
    int index = 0;
    BOOL bRet = FALSE;
    UINT32 uiAppInfoStringLength = 0;

    pBuffer = new BYTE[uiResponseStringLen / 2];
    if (NULL == pBuffer)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseEFdir() - Cannot allocate %u bytes for"
                "pBuffer\r\n", (uiResponseStringLen / 2));
        goto Done;
    }

    memset(pBuffer, 0, (uiResponseStringLen / 2));

    if (!GSMHexToGSM(pszResponseString, uiResponseStringLen, pBuffer,
            (uiResponseStringLen / 2 ), uiByteBufferLength))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseEFdir() - GSMHexToGSM conversion failed\r\n");
        goto Done;
    }

    pTemp = pBuffer;

    m_SimAppListData.nApplications = 0;

    /*
     * Coding of an application template entry
     *      Length          Description                                     Status
     *
     *          1           Application template tag = '61'                     M (Mandatory)
     *          1           Length of the Application template = '03' - '7F'    M
     *          1           Application identifier tag = '4F'                   M
     *          1           AID length = '01'-'10'                              M
     *      '01' to '10'    AID value. see TS 101 220                           M
     *          1           Application label tag = '50'                        O (Optional)
     *          1           Application label length                            O
     *          note 1      Application label value                             O
     *
     * Refer TS 102 221 for description on note 1 and for coding of applicable label value.
     */
    while (uiByteBufferLength > MANDATORY_LENGTH)
    {
        BOOL bIsAppSupported = FALSE;
        const UINT32 AID_LENGTH_INDEX = 3;
        const UINT32 AID_INDEX = 4;
        const UINT32 APP_CODE_INDEX = 9;
        const BYTE APP_TEMPLATE_TAG = 0x61;
        const BYTE APP_LABEL_TAG = 0x50;
        const BYTE SIM_AID[] = {0xA0, 0x00, 0x00, 0x00, 0x09};
        // Registered application provider IDentifier (RID)
        const BYTE RID_3G[] = {0xA0, 0x00, 0x00, 0x00, 0x87};
        // This should match the size of BYTE USIM_APP_CODE and BYTE ISIM_APP_CODE
        const UINT32 APP_CODE_LENGTH = 2;
        // This should match the size of SIM_AID and RID_3G
        const UINT32 RID_LENGTH = 5;

        index = m_SimAppListData.nApplications;

        uiAidLength = pTemp[AID_LENGTH_INDEX];

        if (0 == uiAidLength || 0xFF == uiAidLength)
        {
            RIL_LOG_CRITICAL("CTEBase::ParseEFdir() - INVALID AID length\r\n");
            break;
        }

        if (uiByteBufferLength > MANDATORY_LENGTH + RID_LENGTH + APP_CODE_LENGTH)
        {
            const BYTE USIM_APP_CODE[] = {0x10, 0x02};
            const BYTE ISIM_APP_CODE[] = {0x10, 0x04};

            if (0 == memcmp(&pTemp[AID_INDEX], RID_3G, sizeof(RID_3G)))
            {
                if (0 == memcmp(&pTemp[APP_CODE_INDEX], USIM_APP_CODE, sizeof(USIM_APP_CODE)))
                {
                    m_SimAppListData.aAppInfo[index].appType = RIL_APPTYPE_USIM;
                    m_CardStatusCache.gsm_umts_subscription_app_index = SIM_USIM_APP_INDEX;
                    bIsAppSupported = TRUE;
                }
                else if (0 == memcmp(&pTemp[APP_CODE_INDEX], ISIM_APP_CODE, sizeof(ISIM_APP_CODE)))
                {
                    m_SimAppListData.aAppInfo[index].appType = RIL_APPTYPE_ISIM;
                    m_CardStatusCache.ims_subscription_app_index = ISIM_APP_INDEX;
                    bIsAppSupported = TRUE;
                }
            }
            else if (0 == memcmp(&pTemp[AID_INDEX], SIM_AID, sizeof(SIM_AID)))
            {
                m_SimAppListData.aAppInfo[index].appType = RIL_APPTYPE_SIM;
                m_CardStatusCache.gsm_umts_subscription_app_index = SIM_USIM_APP_INDEX;
                bIsAppSupported = TRUE;
            }
            else
            {
                RIL_LOG_INFO("CTEBase::ParseEFdir() - unknown application\r\n");
            }
        }
        else
        {
            break;
        }

        if (bIsAppSupported)
        {
            m_SimAppListData.nApplications++;

            memset(m_SimAppListData.aAppInfo[index].szAid, 0,
                    sizeof(m_SimAppListData.aAppInfo[index].szAid));

            strncpy(m_SimAppListData.aAppInfo[index].szAid,
                    &pszResponseString[uiAppInfoStringLength + (AID_INDEX * 2)],
                    MIN(sizeof(m_SimAppListData.aAppInfo[index].szAid) - 1, uiAidLength * 2));
        }

        pTemp += (MANDATORY_LENGTH + uiAidLength);
        uiAppInfoStringLength += (MANDATORY_LENGTH + uiAidLength) * 2;

        if (uiByteBufferLength >= (MANDATORY_LENGTH + uiAidLength))
        {
            uiByteBufferLength -= (MANDATORY_LENGTH + uiAidLength);
        }
        else
        {
            RIL_LOG_INFO("CTEBase::ParseEFdir() - "
                    "uiByteBufferLength < MANDATORY_LENGTH + uiAidLength\r\n");
            break;
        }

        // 2 = APP LABEL TAG(1) + APP LABEL LENGTH(1)
        if (uiByteBufferLength > 2)
        {
            if (APP_LABEL_TAG == pTemp[0])
            {
                uiAppLabelLength = pTemp[1];

                if (uiByteBufferLength >= (uiAppLabelLength + 2))
                {
                    if (bIsAppSupported)
                    {
                        convertGsmToUtf8HexString(pTemp + 2, 0,
                                uiAppLabelLength, m_SimAppListData.aAppInfo[index].szAppLabel,
                                sizeof(m_SimAppListData.aAppInfo[index].szAppLabel) - 1);
                    }

                    pTemp += (uiAppLabelLength + 2);
                    uiByteBufferLength -= (uiAppLabelLength + 2);
                    uiAppInfoStringLength += (uiAppLabelLength + 2) * 2;
                }
                else
                {
                    RIL_LOG_INFO("CTEBase::ParseEFdir() - "
                            "uiByteBufferLength < uiAppLabelLength\r\n");
                    break;
                }
            }
        }
        else
        {
            RIL_LOG_INFO("CTEBase::ParseEFdir() - uiByteBufferLength <= 2\r\n");
            break;
        }

        // ignore other tlvs, padded bytes
        while (uiByteBufferLength >= 2)
        {
            if (*pTemp == 0xFF)
            {
                pTemp++;
                uiByteBufferLength--;
                uiAppInfoStringLength += 2;
            }
            else if (*pTemp == APP_TEMPLATE_TAG)
            {
                break;
            }
            else
            {
                UINT32 uiTlvSize = pTemp[1] + 2; // TAG(1) + LENGTH(1) field  + tlvLength

                pTemp += uiTlvSize;
                if (uiTlvSize >= uiByteBufferLength)
                {
                    uiByteBufferLength = 0;
                    break;
                }
                else
                {
                    uiByteBufferLength -= uiTlvSize;
                    uiAppInfoStringLength += (uiTlvSize * 2);
                }
            }
        }
    }

    if (m_SimAppListData.nApplications > 1
            && m_CardStatusCache.ims_subscription_app_index == ISIM_APP_INDEX)
    {
        UpdateIsimAppState();
    }

    m_CardStatusCache.num_applications = m_SimAppListData.nApplications;

    bRet = TRUE;

Done:
    if (NULL != pBuffer)
    {
        delete[] pBuffer;
        pBuffer = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseEFdir() - Exit\r\n");
    return bRet;
}

void CTEBase::CopyCardStatus(RIL_CardStatus_v6& cardStatus)
{
    memcpy(&cardStatus, &m_CardStatusCache, sizeof(RIL_CardStatus_v6));

    for (int i = 0; i < cardStatus.num_applications; i++)
    {
        GetSimAppIdAndLabel(cardStatus.applications[i].app_type,
                &cardStatus.applications[i].aid_ptr,
                &cardStatus.applications[i].app_label_ptr);
    }
}

void CTEBase::HandleSimState(const UINT32 /*uiSIMState*/, BOOL& /*bNotifySimStatusChange*/)
{
}

BOOL CTEBase::OpenLogicalChannel(POST_CMD_HANDLER_DATA& rData, const char* pszAid)
{
    RIL_LOG_VERBOSE("CTEBase::OpenLogicalChannel() - Enter\r\n");

    char szCmd[MAX_BUFFER_SIZE] = {'\0'};
    BOOL bRet = FALSE;
    CCommand* pCmd = NULL;

    if (NULL == pszAid || pszAid[0] == '\0')
    {
        RIL_LOG_CRITICAL("CTEBase::OpenLogicalChannel() - invalid aid_ptr\r\n");
        goto Error;
    }

    if (!PrintStringNullTerminate(szCmd, sizeof(szCmd), "AT+CCHO=\"%s\"\r", pszAid))
    {
        RIL_LOG_CRITICAL("CTEBase::OpenLogicalChannel() - Cannot create CCHO command\r\n");
        goto Error;
    }

    pCmd = new CCommand(rData.uiChannel, rData.pRilToken, rData.requestId, szCmd,
            &CTE::ParseSimOpenChannel, &CTE::PostInternalOpenLogicalChannel);
    if (NULL == pCmd)
    {
        RIL_LOG_CRITICAL("CTEBase::OpenLogicalChannel() - "
                "Unable to allocate memory for command\r\n");
        goto Error;
    }
    else
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::OpenLogicalChannel() - "
                    "Unable to add command to queue\r\n");
            delete pCmd;
            pCmd = NULL;
            goto Error;
        }
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CTEBase::OpenLogicalChannel() - Exit\r\n");
    return bRet;
}

RIL_RESULT_CODE CTEBase::HandleSimIO(RIL_SIM_IO_v6* pSimIOArgs, REQUEST_DATA& rReqData,
        const int sessionId)
{
    RIL_LOG_VERBOSE("CTEBase::HandleSimIO() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (NULL == pSimIOArgs->data)
    {
        if (NULL == pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+CRLA=%d,%d,%d,%d,%d,%d\r", sessionId, pSimIOArgs->command,
                    pSimIOArgs->fileid, pSimIOArgs->p1, pSimIOArgs->p2, pSimIOArgs->p3))
            {
                RIL_LOG_CRITICAL("CTEBase::HandleSimIO() - cannot create CRLA command\r\n");
                goto Error;
            }
        }
        else // (NULL != pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+CRLA=%d,%d,%d,%d,%d,%d,,\"%s\"\r", sessionId, pSimIOArgs->command,
                    pSimIOArgs->fileid, pSimIOArgs->p1, pSimIOArgs->p2, pSimIOArgs->p3,
                    pSimIOArgs->path))
            {
                RIL_LOG_CRITICAL("CTEBase::HandleSimIO() - cannot create CRLA command\r\n");
                goto Error;
            }
        }
    }
    else // (NULL != pSimIOArgs->data)
    {
        if (NULL == pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+CRLA=%d,%d,%d,%d,%d,%d,\"%s\"\r", sessionId, pSimIOArgs->command,
                    pSimIOArgs->fileid, pSimIOArgs->p1, pSimIOArgs->p2, pSimIOArgs->p3,
                    pSimIOArgs->data))
            {
                RIL_LOG_CRITICAL("CTEBase::HandleSimIO() - cannot create CRLA command\r\n");
                goto Error;
            }
        }
        else // (NULL != pSimIOArgs->path)
        {
            if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                    "AT+CRLA=%d,%d,%d,%d,%d,%d,\"%s\",\"%s\"\r", sessionId,
                    pSimIOArgs->command, pSimIOArgs->fileid, pSimIOArgs->p1, pSimIOArgs->p2,
                    pSimIOArgs->p3, pSimIOArgs->data, pSimIOArgs->path))
            {
                RIL_LOG_CRITICAL("CTEBase::HandleSimIO() - cannot create CRLA command\r\n");
                goto Error;
            }
        }
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::HandleSimIO() - Exit\r\n");
    return res;
}

void CTEBase::ResetInitialAttachApn()
{
    m_InitialAttachApnParams.szApn[0] = '\0';
    m_InitialAttachApnParams.szPdpType[0] = '\0';
}

RIL_RESULT_CODE CTEBase::RestoreSavedNetworkSelectionMode(RIL_Token rilToken, UINT32 uiChannel,
        PFN_TE_PARSE pParseFcn, PFN_TE_POSTCMDHANDLER pHandlerFcn)
{
    RIL_LOG_VERBOSE("CTEBase::RestoreSavedNetworkSelectionMode() - Enter\r\n");

    char szCmd[MAX_BUFFER_SIZE] = {0};
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    int requestId = RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC;
    CCommand* pCmd = NULL;

    if (m_NetworkSelectionModeParams.mode == E_NETWORK_SELECTION_MODE_AUTOMATIC)
    {
        if (!CopyStringNullTerminate(szCmd, m_cte.IsVoiceCapable() ? "AT+COPS=0\r" : "AT+CGATT=1\r",
                sizeof(szCmd)))
        {
            RIL_LOG_CRITICAL("CTEBase::RestoreSavedNetworkSelectionMode() - "
                    "Failed to write command to buffer!\r\n");
            goto Error;
        }
    }
    else if (m_NetworkSelectionModeParams.mode == E_NETWORK_SELECTION_MODE_MANUAL
            && strlen(m_NetworkSelectionModeParams.szOperatorNumeric) > 0)
    {
        requestId = RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL;

        if (!PrintStringNullTerminate(szCmd, sizeof(szCmd),
                "AT+COPS=1,2,\"%s\"\r", m_NetworkSelectionModeParams.szOperatorNumeric))
        {
            RIL_LOG_CRITICAL("CTEBase::RestoreSavedNetworkSelectionMode() - "
                    "Failed to write command to buffer!\r\n");
            goto Error;
        }
    }
    else
    {
        /*
         * Network selection mode parameters are not set. This is possible when the
         * network selection request is not yet sent by Android Telephony framework.
         */
        res = RRIL_RESULT_OK;
        goto Error;
    }

    pCmd = new CCommand(uiChannel, rilToken, requestId, szCmd, pParseFcn, pHandlerFcn);
    if (pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::RestoreSavedNetworkSelectionMode() -"
                    " Unable to add command to queue\r\n");
            delete pCmd;
            pCmd = NULL;
            goto Error;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::RestoreSavedNetworkSelectionMode() -"
                " Unable to allocate memory for command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTEBase::RestoreSavedNetworkSelectionMode() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::SetInitialAttachApn(RIL_Token rilToken, UINT32 uiChannel,
        PFN_TE_PARSE pParseFcn, PFN_TE_POSTCMDHANDLER pHandlerFcn, int nextState)
{
    RIL_LOG_VERBOSE("CTEBase::SetInitialAttachApn() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;
    REQUEST_DATA reqData;

    memset(&reqData, 0, sizeof(REQUEST_DATA));

    GetSetInitialAttachApnReqData(reqData);

    int* pNextState = (int*)malloc(sizeof(int));
    if (pNextState != NULL)
    {
        *pNextState = nextState;
        reqData.pContextData = pNextState;
        reqData.cbContextData = sizeof(int);
    }

    CCommand* pCmd = new CCommand(uiChannel, rilToken, RIL_REQUEST_SET_INITIAL_ATTACH_APN, reqData,
            pParseFcn, pHandlerFcn);
    if (pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::SetInitialAttachApn() -"
                    " Unable to add command to queue\r\n");
            delete pCmd;
            pCmd = NULL;
            goto Error;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::SetInitialAttachApn() -"
                " Unable to allocate memory for command\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;
Error:
    RIL_LOG_VERBOSE("CTEBase::SetInitialAttachApn() - Exit\r\n");
    return res;
}

BOOL CTEBase::GetSetInitialAttachApnReqData(REQUEST_DATA& /*rReqData*/)
{
    return TRUE;
}

void CTEBase::SetProfileDownloadForNextUiccStartup(UINT32 /* uiDownload */,
        UINT32 /* uiReporting */)
{
}

void CTEBase::ConfigureUsatProfileDownload(UINT32 /* uiDownload */, UINT32 /* uiReporting */)
{
}

void CTEBase::PostConfigureUsatProfileDownloadHandler(POST_CMD_HANDLER_DATA& /* data */)
{
}

RIL_RESULT_CODE CTEBase::ParseQueryUiccState(RESPONSE_DATA& /* rspData */)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

void CTEBase::PostQueryUiccStateHandler(POST_CMD_HANDLER_DATA& /* data */)
{
}

RIL_RESULT_CODE CTEBase::ParseReadUsatProfiles(RESPONSE_DATA& /* rspData */)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

void CTEBase::WriteUsatProfiles(const char* /* pszTeProfile */, const BOOL /* isTeWriteNeeded */,
        const char* /* pszMtProfile */, const BOOL /* isMtWriteNeeded */)
{
}

void CTEBase::WriteUsatProfile(const UINT32 /* uiProfileStorage */, const char* /* pszProfile */)
{
}

RIL_RESULT_CODE CTEBase::ParseWriteUsatProfile(RESPONSE_DATA& /* rspData */)
{
    return RIL_E_REQUEST_NOT_SUPPORTED;
}

void CTEBase::PostWriteUsatProfileHandler(POST_CMD_HANDLER_DATA& /* data */)
{
}

void CTEBase::ResetUicc()
{
}

void CTEBase::NotifyUiccReady()
{
}

void CTEBase::EnableProfileFacilityHandling()
{
}

void CTEBase::SendModemDownToUsatSM()
{
}

void CTEBase::GetRegStatusAndBandInfo(
        sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND& regStatusAndBandInfo)
{
    regStatusAndBandInfo = m_sRegStatusAndBandInfo;
}

void CTEBase::SetRegStatusAndBandInfo(
        sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND regStatusAndBandInfo)
{
    m_sRegStatusAndBandInfo = regStatusAndBandInfo;
}

RIL_RESULT_CODE CTEBase::SetCsgAutomaticSelection(REQUEST_DATA& /* reqData */)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::GetCsgCurrentState(REQUEST_DATA& /* reqData */)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::ParseXCSG(const char* /* pszRsp */, RESPONSE_DATA& /* rspData */)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

void CTEBase::ResetNetworkSelectionMode()
{
    m_NetworkSelectionModeParams.mode = -1;
    m_NetworkSelectionModeParams.szOperatorNumeric[0] = '\0';
}

const char* CTEBase::GetEnableFetchingString()
{
    return NULL;
}

const char* CTEBase::GetSiloVoiceURCInitString()
{
    const char* pszSiloVoiceURCInitString = "|+CSNN=1,1";
    return pszSiloVoiceURCInitString;
}

// The AT+CNAP? query is sent in the RIL_CHANNEL_ATCMD as set in the te_xmm6260.cpp
// file. This is made to avoid conflict between the +CNAP URC parsing and the
// response parsing, please be aware that there should not be any modification
// on the channel unless changes of code behavior.
RIL_RESULT_CODE CTEBase::CoreQueryCnap(REQUEST_DATA& rReqData)
{
    RIL_LOG_VERBOSE("CTEBase::CoreQueryCnap() - Enter\r\n");
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (!CopyStringNullTerminate(rReqData.szCmd1, "AT+CNAP?\r", sizeof(rReqData.szCmd1)))
    {
        RIL_LOG_CRITICAL("CTEBase::CoreQueryCnap() - Unable to write command to buffer\r\n");
        goto Error;
    }

    res = RRIL_RESULT_OK;

Error:
    RIL_LOG_VERBOSE("CTEBase::CoreQueryCnap() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::ParseQueryCnap(const char* pszRsp, RESPONSE_DATA& rRspData)
{
    RIL_LOG_VERBOSE("CTEBase::ParseQueryCnap() - Enter\r\n");

    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    P_ND_CNAP_CURRENT_STATE pCnapVal
            = (P_ND_CNAP_CURRENT_STATE) malloc(sizeof(S_ND_CNAP_CURRENT_STATE));
    UINT32 nValue = 0;
    UINT32 uiCause;

    if (NULL == pCnapVal)
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryCnap() - Could not allocate memory for response.\r\n");
        goto Error;
    }

    memset(pCnapVal, 0, sizeof(S_ND_CNAP_CURRENT_STATE));

    // Parse "<prefix>+CNAP: <n>,<m>"
    if (!SkipString(pszRsp, "+CNAP: ", pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryCnap() - Can't parse prefix. rsp=%s \r\n", pszRsp);
        goto Error;
    }


    if (!ExtractUInt32(pszRsp, nValue, pszRsp))
    {
        goto Error;
    }

    if (!SkipString(pszRsp, ",", pszRsp))
    {
        goto Error;
    }


    if (!CopyStringNullTerminate(pCnapVal->szCnapCurrentState, pszRsp, MAX_BUFFER_SIZE))
    {
        goto Error;
    }

    pCnapVal->sResponsePointer.pszCnapCurrentState = pCnapVal->szCnapCurrentState;

    rRspData.pData = (void*)pCnapVal;
    rRspData.uiDataSize = sizeof(S_ND_CNAP_CURRENT_STATE_PTR);


    if (!FindAndSkipRspEnd(pszRsp, m_szNewLine, pszRsp))
    {
        RIL_LOG_CRITICAL("CTEBase::ParseQueryCnap() -"
                   " Could not extract the response end.\r\n");
        goto Error;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryCnap() - CnapVal = %s", pCnapVal->szCnapCurrentState);

    res = RRIL_RESULT_OK;

Error:
    if (RRIL_RESULT_OK != res)
    {
        free(pCnapVal);
        pCnapVal = NULL;
    }

    RIL_LOG_VERBOSE("CTEBase::ParseQueryCnap() - Exit\r\n");
    return res;
}

RIL_RESULT_CODE CTEBase::CreateSetAdaptiveClockingReq(REQUEST_DATA& /*reqData*/,
            const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::CreateGetAdaptiveClockingFreqInfo(REQUEST_DATA& /*reqData*/,
            const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::ParseGetAdaptiveClockingFreqInfo(const char* /*pszRsp*/,
            RESPONSE_DATA& /*rspData*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::CreateSetRegStatusAndBandReport(REQUEST_DATA& /*reqData*/,
            const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::CreateSetCoexReport(REQUEST_DATA& /*reqData*/,
            const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::CreateSetCoexWlanParams(REQUEST_DATA& /*reqData*/,
            const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

RIL_RESULT_CODE CTEBase::CreateSetCoexBtParams(REQUEST_DATA& /*reqData*/,
            const char** /*ppszRequest*/, const UINT32 /*uiDataSize*/)
{
    // should be derived in modem specific class
    return RIL_E_REQUEST_NOT_SUPPORTED; // only suported at modem level
}

bool CTEBase::IsPdpTypeCompatible(const char* pszPdpType1, const char* pszPdpType2)
{
    if (0 == strcmp(pszPdpType1, PDPTYPE_IPV4V6)
            || 0 == strcmp(pszPdpType2, PDPTYPE_IPV4V6))
        return true;

    return (0 == strcmp(pszPdpType1, pszPdpType2));
}

bool CTEBase::IsApnEqual(const char* pszApn1, const char* pszApn2)
{
    bool bRet = true;

    if (pszApn1 != NULL && pszApn2 != NULL)
    {
        if (strcasestr(pszApn1, pszApn2) == NULL
                && strcasestr(pszApn2, pszApn1) == NULL)
        {
            bRet = false;
        }
    }

    return bRet;
}

void CTEBase::RequestDetachOnIAChange()
{
    REQUEST_DATA reqData;

    memset(&reqData, 0, sizeof(REQUEST_DATA));
    if (!CopyStringNullTerminate(reqData.szCmd1, "AT+CGATT=0\r", sizeof(reqData.szCmd1)))
    {
        return;
    }

    int* pNextState = (int*)malloc(sizeof(int));
    if (pNextState != NULL)
    {
        *pNextState = STATE_SET_INITIAL_ATTACH_APN;
        reqData.pContextData = pNextState;
        reqData.cbContextData = sizeof(int);
    }

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC].uiChannel,
            NULL, RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC, reqData, NULL,
            &CTE::PostSetInitialAttachApnCmdHandler);
    if (pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::RequestDetachOnIAChange() -"
                    " Unable to add command to queue\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::RequestDetachOnIAChange() -"
                " Unable to allocate memory for command\r\n");
    }
}

void CTEBase::RequestAttachOnIAChange(const UINT32 uiChannel, const int requestId)
{
    REQUEST_DATA reqData;

    memset(&reqData, 0, sizeof(REQUEST_DATA));
    if (!CopyStringNullTerminate(reqData.szCmd1, "AT+CGATT=1\r", sizeof(reqData.szCmd1)))
    {
        return;
    }

    CCommand* pCmd = new CCommand(uiChannel,NULL, requestId, reqData);
    if (pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("CTEBase::RequestAttachOnIAChange() -"
                    " Unable to add command to queue\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CTEBase::RequestAttachOnIAChange() -"
                " Unable to allocate memory for command\r\n");
    }
}
