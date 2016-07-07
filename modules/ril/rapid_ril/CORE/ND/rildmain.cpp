////////////////////////////////////////////////////////////////////////////
// rildmain.cpp
//
// Copyright 2005-2008 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for top-level RIL functions.
//
/////////////////////////////////////////////////////////////////////////////

#include <telephony/ril.h>
#include <stdio.h>
#include <errno.h>

#include "types.h"
#include "rillog.h"
#include "te.h"
#include "thread_ops.h"
#include "systemmanager.h"
#include "repository.h"
#include "rildmain.h"
#include "reset.h"
#include <cutils/properties.h>
#include <utils/Log.h>
#include "hardwareconfig.h"

///////////////////////////////////////////////////////////
//  FUNCTION PROTOTYPES
//

static void onRequest(int request, void* data, size_t datalen, RIL_Token t);
static RIL_RadioState onGetCurrentRadioState();
static int onSupports(int requestCode);
static void onCancel(RIL_Token t);
static const char* getVersion();


///////////////////////////////////////////////////////////
//  GLOBAL VARIABLES
//

char* g_szCmdPort  = NULL;
BOOL  g_bIsSocket = FALSE;
char* g_szDataPort1 = NULL;
char* g_szDataPort2 = NULL;
char* g_szDataPort3 = NULL;
char* g_szDataPort4 = NULL;
char* g_szDataPort5 = NULL;
char* g_szDLC2Port = NULL;
char* g_szDLC6Port = NULL;
char* g_szDLC8Port = NULL;
char* g_szDLC22Port = NULL;
char* g_szDLC23Port = NULL;
char* g_szSmsPort = NULL;
char* g_szURCPort = NULL;
char* g_szOEMPort = NULL;
char* g_szClientId = NULL;

// Upper limit on number of RIL channels to create
UINT32 g_uiRilChannelUpperLimit = RIL_CHANNEL_MAX;

// Current RIL channel index maximum (depends on number of data channels created)
UINT32 g_uiRilChannelCurMax = 0;

static const RIL_RadioFunctions gs_callbacks =
{
    RIL_VERSION,
    onRequest,
    onGetCurrentRadioState,
    onSupports,
    onCancel,
    getVersion
};

static const struct RIL_Env* gs_pRilEnv;

#define USAGE  "\n"\
    "--------------------------------------------------\n"\
    "RapidRIL Usage:\n"\
    "\tMandatory parameters:\n"\
    "\t\t - NONE -\n"\
    "\tOptional parameters:\n"\
    "\t\t-h show this help message and exit\n"\
    "\t\t-i <Subscription ID>\n"\
    "--------------------------------------------------\n\n"\


///////////////////////////////////////////////////////////
// FUNCTION DEFINITIONS
//

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
void RIL_onRequestComplete(RIL_Token tRIL, RIL_Errno eErrNo, void* pResponse, size_t responseLen)
{
    gs_pRilEnv->OnRequestComplete(tRIL, eErrNo, pResponse, responseLen);
    RIL_LOG_INFO("After OnRequestComplete(): token=0x%08x, eErrNo=%d, pResponse=[0x%08x],"
            " len=[%d]\r\n", tRIL, eErrNo, pResponse, responseLen);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
void RIL_onUnsolicitedResponse(int unsolResponseID, const void* pData, size_t dataSize)
{
    bool bSendNotification = true;

    if ((CTE::GetTE().IsPlatformShutDownRequested() || CTE::GetTE().IsRadioRequestPending())
            && RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED != unsolResponseID
            && RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED != unsolResponseID
            && RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED != unsolResponseID
            && RIL_UNSOL_DATA_CALL_LIST_CHANGED != unsolResponseID
            && RIL_UNSOL_OEM_HOOK_RAW != unsolResponseID)
    {
        RIL_LOG_INFO("RIL_onUnsolicitedResponse() - ignoring id=%d due to "
                "radio on/off requested\r\n", unsolResponseID);
        return;
    }

    switch (unsolResponseID)
    {
        case RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                    " RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED\r\n");
            break;

        case RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                    " RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED\r\n");
            break;

        case RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                    " RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED\r\n");
            break;

        case RIL_UNSOL_RESPONSE_NEW_SMS:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                    " RIL_UNSOL_RESPONSE_NEW_SMS\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - PDU=\"%s\"\r\n", (char*)pData);
            }
            break;

        case RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                    " RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - PDU=\"%s\"\r\n", (char*)pData);
            }
            break;

        case RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - index=%d\r\n", ((int*)pData)[0]);
            }
            break;

        case RIL_UNSOL_ON_USSD:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_ON_USSD\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - type code=\"%s\"\r\n",
                        ((char**)pData)[0]);
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - msg=\"%s\"\r\n", ((char**)pData)[1]);
            }
            break;

        case RIL_UNSOL_ON_USSD_REQUEST:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_ON_USSD_REQUEST\r\n");
            break;

        case RIL_UNSOL_NITZ_TIME_RECEIVED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_NITZ_TIME_RECEIVED\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - NITZ info=\"%s\"\r\n", (char*)pData);
            }
            break;

        case RIL_UNSOL_SIGNAL_STRENGTH:
            RIL_LOG_VERBOSE("RIL_onUnsolicitedResponse() - RIL_UNSOL_SIGNAL_STRENGTH\r\n");
            break;

        case RIL_UNSOL_DATA_CALL_LIST_CHANGED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_DATA_CALL_LIST_CHANGED\r\n");
            if (pData && dataSize)
            {
                int nDataCallResponseNum = dataSize / sizeof(RIL_Data_Call_Response_v6);
                RIL_Data_Call_Response_v6* pDCR = (RIL_Data_Call_Response_v6*)pData;
                for (int i=0; i<nDataCallResponseNum; i++)
                {
                    RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_Data_Call_Response_v6[%d]"
                            " status=%d suggRetryTime=%d cid=%d active=%d type=\"%s\" ifname=\"%s\""
                            " addresses=\"%s\" dnses=\"%s\" gateways=\"%s\"\r\n",
                            i,
                            pDCR[i].status,
                            pDCR[i].suggestedRetryTime,
                            pDCR[i].cid, pDCR[i].active,
                            pDCR[i].type, pDCR[i].ifname,
                            pDCR[i].addresses,
                            pDCR[i].dnses,
                            pDCR[i].gateways);
                }
            }
            break;

        case RIL_UNSOL_SUPP_SVC_NOTIFICATION:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_SUPP_SVC_NOTIFICATION\r\n");
            if (pData && dataSize)
            {
                RIL_SuppSvcNotification* pSSN = (RIL_SuppSvcNotification*)pData;
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - notification type=%d code=%d index=%d"
                        " type=%d number=\"%s\"\r\n",
                        pSSN->notificationType,
                        pSSN->code,
                        pSSN->index,
                        pSSN->type,
                        pSSN->number);
            }
            break;

        case RIL_UNSOL_STK_SESSION_END:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_STK_SESSION_END\r\n");
            break;

        case RIL_UNSOL_STK_PROACTIVE_COMMAND:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_STK_PROACTIVE_COMMAND\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - data=\"%s\"\r\n", (char*)pData);
            }
            else
            {
                // If there is no data, this unsolicited command will generate a JAVA CRASH
                // So ignore it if we are in this case
                bSendNotification = false;
            }
            break;

        case RIL_UNSOL_STK_EVENT_NOTIFY:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_STK_EVENT_NOTIFY\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - data=\"%s\"\r\n", (char*)pData);
            }
            break;

        case RIL_UNSOL_STK_CALL_SETUP:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_STK_CALL_SETUP\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - timeout=%d ms\r\n", ((int*)pData)[0]);
            }
            break;

        case RIL_UNSOL_SIM_SMS_STORAGE_FULL:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_SIM_SMS_STORAGE_FULL\r\n");
            break;

        case RIL_UNSOL_SIM_REFRESH:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_SIM_REFRESH\r\n");
            if (pData && dataSize)
            {
                if (dataSize == sizeof(RIL_SimRefreshResponse_v7))
                {
                    RIL_SimRefreshResponse_v7* pSimRefreshRsp = (RIL_SimRefreshResponse_v7*)pData;

                    RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_SimRefreshResult - result=%d"
                            " efid=%d aid=%s\r\n", pSimRefreshRsp->result, pSimRefreshRsp->ef_id,
                            (NULL == pSimRefreshRsp->aid) ? "" : pSimRefreshRsp->aid);
                }
            }
            break;

        case RIL_UNSOL_CALL_RING:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_CALL_RING\r\n");
            break;

        case RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - "
                    "RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED\r\n");
            break;

        case RIL_UNSOL_RESPONSE_CDMA_NEW_SMS:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RESPONSE_CDMA_NEW_SMS\r\n");
            bSendNotification = false;
            break;

        case RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - PDU=\"%s\"\r\n", (char*)pData);
            }
            break;

        case RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL\r\n");
            bSendNotification = false;
            break;

        case RIL_UNSOL_RESTRICTED_STATE_CHANGED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RESTRICTED_STATE_CHANGED\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                        " RIL_RESTRICTED_STATE_* bitmask=0x%08X\r\n", ((int*)pData)[0]);
            }
            break;

        case RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                    " RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE\r\n");
            break;

        case RIL_UNSOL_CDMA_CALL_WAITING:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_CDMA_CALL_WAITING\r\n");
            bSendNotification = false;
            break;

        case RIL_UNSOL_CDMA_OTA_PROVISION_STATUS:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_CDMA_OTA_PROVISION_STATUS\r\n");
            bSendNotification = false;
            break;

        case RIL_UNSOL_CDMA_INFO_REC:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_CDMA_INFO_REC\r\n");
            bSendNotification = false;
            break;

        case RIL_UNSOL_OEM_HOOK_RAW:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_OEM_HOOK_RAW\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - dataSize=%d\r\n", dataSize);
            }
            else
            {
                // If there is no data, this unsolicited command will generate a JAVA CRASH
                // So ignore it if we are in this case
                bSendNotification = false;
            }
            break;

        case RIL_UNSOL_RINGBACK_TONE:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RINGBACK_TONE\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RINGBACK_TONE=[%d]\r\n",
                        ((int*)pData)[0]);
            }
            break;

        case RIL_UNSOL_RESEND_INCALL_MUTE:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RESEND_INCALL_MUTE\r\n");
            break;

        case RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                    " RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED\r\n");
            bSendNotification = false;
            break;

        case RIL_UNSOL_CDMA_PRL_CHANGED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_CDMA_PRL_CHANGED\r\n");
            bSendNotification = false;
            break;

        case RIL_UNSOL_EXIT_EMERGENCY_CALLBACK_MODE:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() -"
                    " RIL_UNSOL_EXIT_EMERGENCY_CALLBACK_MODE\r\n");
            break;

        case RIL_UNSOL_RIL_CONNECTED:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RIL_CONNECTED\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_RIL_CONNECTED=[%d]\r\n",
                        ((int*)pData)[0]);
            }
            break;

        case RIL_UNSOL_CELL_INFO_LIST:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - RIL_UNSOL_CELL_INFO_LIST\r\n");
            if (pData && dataSize)
            {
                RIL_LOG_INFO("RIL_onUnsolicitedResponse() - "
                        "RIL_UNSOL_CELL_INFO_LIST data size=[%d]\r\n", dataSize);
            }
            break;
        //  ************************* END OF REGULAR NOTIFICATIONS *******************************

        default:
            RIL_LOG_INFO("RIL_onUnsolicitedResponse() - Ignoring Unknown Notification id=0x%08X,"
                    " %d\r\n", unsolResponseID, unsolResponseID);
            bSendNotification = false;
            break;
    }

    if ( (NULL == pData) && (0 == dataSize) )
    {
        RIL_LOG_INFO("RIL_onUnsolicitedResponse() - pData is NULL! id=%d\r\n", unsolResponseID);
    }

    if (bSendNotification)
    {
        RIL_LOG_VERBOSE("Calling gs_pRilEnv->OnUnsolicitedResponse()... id=%d\r\n",
                unsolResponseID);
        gs_pRilEnv->OnUnsolicitedResponse(unsolResponseID, pData, dataSize);
    }
    else
    {
        RIL_LOG_INFO("RIL_onUnsolicitedResponse() - ignoring id=%d\r\n", unsolResponseID);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
void RIL_requestTimedCallback(RIL_TimedCallback callback, void* pParam,
                                   const struct timeval* pRelativeTime)
{
    if (pRelativeTime)
    {
        RIL_LOG_INFO("Calling gs_pRilEnv->RequestTimedCallback() timeval sec=[%d]  usec=[%d]\r\n",
                pRelativeTime->tv_sec, pRelativeTime->tv_usec);
    }
    else
    {
        RIL_LOG_INFO("Calling gs_pRilEnv->RequestTimedCallback() timeval sec=[0]  usec=[0]\r\n");
    }
    gs_pRilEnv->RequestTimedCallback(callback, pParam, pRelativeTime);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
void RIL_requestTimedCallback(RIL_TimedCallback callback, void* pParam,
                                           const unsigned long seconds,
                                           const unsigned long microSeconds)
{
    RIL_LOG_INFO("Calling gs_pRilEnv->RequestTimedCallback() sec=[%d]  usec=[%d]\r\n",
            seconds, microSeconds);
    struct timeval myTimeval = {0,0};
    myTimeval.tv_sec = seconds;
    myTimeval.tv_usec = microSeconds;
    gs_pRilEnv->RequestTimedCallback(callback, pParam, &myTimeval);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Call from RIL to us to make a RIL_REQUEST
 *
 * Must be completed with a call to RIL_onRequestComplete()
 *
 * RIL_onRequestComplete() may be called from any thread, before or after
 * this function returns.
 *
 * Will always be called from the same thread, so returning here implies
 * that the radio is ready to process another command (whether or not
 * the previous command has completed).
 */
static void onRequest(int requestID, void* pData, size_t datalen, RIL_Token hRilToken)
{
    RIL_LOG_INFO("onRequest() - id=%d token: 0x%08lx\r\n", requestID, hRilToken);

    CTE::GetTE().HandleRequest(requestID, pData, datalen, hRilToken);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Synchronous call from the RIL to us to return current radio state.
 *
 * RADIO_STATE_OFF
 * RADIO_STATE_UNAVAILABLE
 * RADIO_STATE_SIM_NOT_READY
 * RADIO_STATE_SIM_LOCKED_OR_ABSENT
 * RADIO_STATE_SIM_READY
 */
static RIL_RadioState onGetCurrentRadioState()
{
    return CTE::GetTE().GetRadioState();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Call from RIL to us to find out whether a specific request code
 * is supported by this implementation.
 *
 * Return 1 for "supported" and 0 for "unsupported"
 */
static int onSupports(int requestCode)
{
    RIL_LOG_INFO("onSupports() - Request [%d]\r\n", requestCode);

    return CTE::GetTE().IsRequestSupported(requestCode);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static void onCancel(RIL_Token t)
{
    RIL_LOG_INFO("onCancel() - *******************************************************\r\n");
    RIL_LOG_INFO("onCancel() - Enter - Exit  token=0x%08lX\r\n", t);
    RIL_LOG_INFO("onCancel() - *******************************************************\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static const char* getVersion(void)
{
    /*
     * Since modem is powered on and sim status is read even before RILJ is connected,
     * RIL_UNSOL_SIM_STATUS_CHANGED message is not delivered to framework. As a result of this,
     * framework doesn't query the sim status. So, notify sim status changed on getVersion to make
     * sure framework queries the sim state.
     *
     * Note: getVersion is called after libril has completed RIL_UNSOL_RIL_CONNECTED to framework.
     */
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);

    return "Intrinsyc Rapid-RIL M6.59 for Android 4.2 (Build September 17/2013)";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool copyDlc(char **out, channel_t* ch, int channelNumber)
{
    bool ret = true;

    if (ch->device[0] != '\0')
    {
        *out = strdup(ch->device);
        RLOGI("%s - ch[%d] = \"%s\"", __FUNCTION__, channelNumber, *out);
    }
    else
        ret = false;

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool RIL_SetGlobals(channels_rril_t *ch)
{
    int opt;
    bool ret = true;

    g_uiRilChannelUpperLimit = RIL_CHANNEL_MAX;
    g_pReqInfo = (REQ_INFO*)g_ReqInfoDefault;

    ret &= copyDlc(&g_szCmdPort, &ch->control, RIL_CHANNEL_ATCMD);
    ret &= copyDlc(&g_szDLC2Port, &ch->registration, RIL_CHANNEL_DLC2);
    ret &= copyDlc(&g_szDLC6Port, &ch->settings, RIL_CHANNEL_DLC6);
    ret &= copyDlc(&g_szDLC8Port, &ch->sim_toolkit, RIL_CHANNEL_DLC8);
    ret &= copyDlc(&g_szDLC22Port, &ch->cops_commands, RIL_CHANNEL_DLC22);
    ret &= copyDlc(&g_szDLC23Port, &ch->rfcoexistence, RIL_CHANNEL_DLC23);
    ret &= copyDlc(&g_szSmsPort, &ch->sms, RIL_CHANNEL_SMS);
    ret &= copyDlc(&g_szURCPort, &ch->monitoring, RIL_CHANNEL_URC);
    ret &= copyDlc(&g_szOEMPort, &ch->field_test, RIL_CHANNEL_OEM);

    if (ret)
    {
        CRepository repository;
        int dataCapable = 0;
        bool bConfigPresent = repository.Read(g_szGroupModem, g_szDataCapable, dataCapable);

        g_uiRilChannelCurMax = RIL_CHANNEL_DATA1;

        // when config read unsuccessfully,  DataCapable will be enable by default
        if (!bConfigPresent || dataCapable)
        {
            ret &= copyDlc(&g_szDataPort1, &ch->packet_data_1, RIL_CHANNEL_DATA1);
            ret &= copyDlc(&g_szDataPort2, &ch->packet_data_2, RIL_CHANNEL_DATA2);
            ret &= copyDlc(&g_szDataPort3, &ch->packet_data_3, RIL_CHANNEL_DATA3);
            ret &= copyDlc(&g_szDataPort4, &ch->packet_data_4, RIL_CHANNEL_DATA4);
            ret &= copyDlc(&g_szDataPort5, &ch->packet_data_5, RIL_CHANNEL_DATA5);

            if (ret)
            {
                g_uiRilChannelCurMax += 5;
            }
        }
    }

    return ret;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static inline void freeDlc(char **szDlc)
{
    free(*szDlc);
    *szDlc = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static void freeDlcs(void)
{
    freeDlc(&g_szCmdPort);
    freeDlc(&g_szDLC2Port);
    freeDlc(&g_szDLC6Port);
    freeDlc(&g_szDLC8Port);
    freeDlc(&g_szDLC22Port);
    freeDlc(&g_szDLC23Port);
    freeDlc(&g_szSmsPort);
    freeDlc(&g_szURCPort);
    freeDlc(&g_szOEMPort);
    freeDlc(&g_szDataPort1);
    freeDlc(&g_szDataPort2);
    freeDlc(&g_szDataPort3);
    freeDlc(&g_szDataPort4);
    freeDlc(&g_szDataPort5);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static const char* cfg = "repository.txt";

static void* mainLoop(void* /*param*/)
{
    RLOGI("mainLoop() - Enter\r\n");

    UINT32 dwRet = 1;
    int modemId = 0;
    int subscriptionId = 0;
    int SIMId = 0;

    // Create the hardware config from the configuration file
    if (!CHardwareConfig::GetInstance().CreateHardwareConfig())
    {
        RLOGE("%s - Failed to create hardware config", __func__);

        dwRet = 0;
        goto Error;
    }

    // Get the subscription, modem and SIM ids
    subscriptionId = CHardwareConfig::GetInstance().GetSubscriptionId();
    modemId = CHardwareConfig::GetInstance().GetModemId();
    SIMId = CHardwareConfig::GetInstance().GetSIMId();

    // Make sure each instances can access Non-Volatile Memory
    if (!CRepository::Init(cfg))
    {
        RLOGE("%s - could not initialize configuration file for modem ID %d",
                __func__,
                modemId);

        dwRet = 0;
        goto Error;
    }

    if (!RIL_SetGlobals())
    {
         RLOGE("%s - Multi Modem - Failed to initialize globals for modemID %d SIMId %d",
                __func__,
                modemId,
                SIMId);
        dwRet = 0;
        goto Error;
    }

    // Initialize logging class
    CRilLog::Init(subscriptionId);

    // Initialize storage mechanism for error causes
    CModemRestart::Init();

    // Initialize helper thread that processes MMGR callbacks
    if (!CDeferThread::Init())
    {
        RIL_LOG_CRITICAL("mainLoop() - InitModemManagerEventHelpers() FAILED\r\n");

        dwRet = 0;
        goto Error;
    }

    // Create and start system manager
    if (!CSystemManager::GetInstance().InitializeSystem())
    {
        RIL_LOG_CRITICAL("mainLoop() - RIL InitializeSystem() FAILED for modem ID: %d\r\n",
                modemId);

        dwRet = 0;
        goto Error;
    }

    RIL_LOG_INFO("[RIL STATE] RIL INIT COMPLETED\r\n");

Error:
    if (!dwRet)
    {
        RIL_LOG_CRITICAL("mainLoop() - RIL Initialization FAILED\r\n");

        CModemRestart::Destroy();
        CDeferThread::Destroy();
        CSystemManager::Destroy();
        freeDlcs();
    }

    RIL_LOG_INFO("mainLoop() - Exit\r\n");
    return (void*)(intptr_t)dwRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool getCmdLineOptions(int argc, char** argv)
{
    bool ret = true;
    int opt  = 0;

    while (-1 != (opt = getopt(argc, argv, "hc:")))
    {
        switch (opt)
        {
            case 'c':
                g_szClientId = optarg;
                RLOGI("%s - Using ClientId %s\r\n", __FUNCTION__, g_szClientId);
                break;

            case 'h':
                puts(USAGE);
                ret = false;
                break;
        }
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
const RIL_RadioFunctions* RIL_Init(const struct RIL_Env* pRilEnv, int argc, char** argv)
{
    const RIL_RadioFunctions* cbs = NULL;

    RIL_LOG_INFO("RIL_Init() - Enter\r\n");

    gs_pRilEnv = pRilEnv;

    if (getCmdLineOptions(argc, argv))
    {
        //  Call mainLoop()
        //  This returns when init is complete.
        mainLoop(NULL);
        RLOGE("%s - returning gs_callbacks\r\n", __FUNCTION__);
        cbs = &gs_callbacks;
    }
    else
    {
        RLOGE("%s - returning NULL\r\n", __FUNCTION__);
    }

    return cbs;
}
