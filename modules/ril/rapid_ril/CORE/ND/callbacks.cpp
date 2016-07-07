////////////////////////////////////////////////////////////////////////////
// callbacks.cpp
//
// Copyright 2005-2008 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implementations for all functions provided to RIL_requestTimedCallback
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "types.h"
#include "command.h"
#include "rildmain.h"
#include "rillog.h"
#include "te.h"
#include "util.h"
#include "oemhookids.h"
#include "channel_data.h"

void notifyNetworkStateChanged(void* /*param*/)
{
    if (CTE::GetTE().TestAndSetNetworkStateChangeTimerRunning(false))
    {
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED, NULL, 0);
    }
}

void notifyChangedCallState(void* /*param*/)
{
    RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED, NULL, 0);
}

void triggerRadioOffInd(void* /*param*/)
{
    if (RADIO_STATE_UNAVAILABLE == CTE::GetTE().GetRadioState())
    {
       CTE::GetTE().SetRadioStateAndNotify(RRIL_RADIO_STATE_OFF);
    }
}

void triggerDataResumedInd(void* /*param*/)
{
    const int DATA_RESUMED = 1;
    sOEM_HOOK_RAW_UNSOL_DATA_STATUS_IND data;

    CTE::GetTE().SetDataSuspended(FALSE);

    data.command = RIL_OEM_HOOK_RAW_UNSOL_DATA_STATUS_IND;
    data.status = DATA_RESUMED;

    RIL_onUnsolicitedResponse(RIL_UNSOL_OEM_HOOK_RAW, (void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_DATA_STATUS_IND));
}

void triggerDataSuspendInd(void* /*param*/)
{
    if (!CTE::GetTE().IsDataSuspended() || (RADIO_STATE_ON != CTE::GetTE().GetRadioState()))
        return;

    const int DATA_SUSPENDED = 0;
    sOEM_HOOK_RAW_UNSOL_DATA_STATUS_IND data;

    data.command = RIL_OEM_HOOK_RAW_UNSOL_DATA_STATUS_IND;
    data.status = DATA_SUSPENDED;

    RIL_onUnsolicitedResponse(RIL_UNSOL_OEM_HOOK_RAW, (void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_DATA_STATUS_IND));
}

void triggerHangup(UINT32 uiCallId)
{
    REQUEST_DATA rReqData;

    memset(&rReqData, 0, sizeof(REQUEST_DATA));
    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
             "AT+XSETCAUSE=1,21;+CHLD=1%u\r", uiCallId))
    {
        RIL_LOG_CRITICAL("triggerHangup() - Unable to create hangup command!\r\n");
        return;
    }

    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_HANGUP].uiChannel,
                                    NULL, REQ_ID_NONE, rReqData);
    if (pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd, TRUE))
        {
            RIL_LOG_CRITICAL("triggerHangup() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("triggerHangup() - Unable to allocate memory for new command!\r\n");
    }
}

void triggerSignalStrength(void* /*param*/)
{
    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_SIGNAL_STRENGTH].uiChannel,
                                    NULL, REQ_ID_NONE, "AT+CSQ\r",
                                    &CTE::ParseUnsolicitedSignalStrength);

    if (pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("triggerSignalStrength() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("triggerSignalStrength() - Unable to allocate memory for new command!\r\n");
    }
}

void triggerSMSAck(void* /*param*/)
{
    CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_SMS_ACKNOWLEDGE].uiChannel,
                                    NULL, REQ_ID_NONE, "AT+CNMA=1\r");

    if (pCmd)
    {
        if (!CCommand::AddCmdToQueue(pCmd))
        {
            RIL_LOG_CRITICAL("triggerSMSAck() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("triggerSMSAck() - Unable to allocate memory for new command!\r\n");
    }
}

void triggerUSSDNotification(void* param)
{
    P_ND_USSD_STATUS pUssdStatus = (P_ND_USSD_STATUS)param;

    RIL_onUnsolicitedResponse (RIL_UNSOL_ON_USSD, pUssdStatus, sizeof(S_ND_USSD_POINTERS));

    free(pUssdStatus);
}

// [in] param = context id.
void triggerDeactivateDataCall(void* param)
{
    RIL_LOG_VERBOSE("triggerDeactivateDataCall() - Enter\r\n");

    UINT32 uiCID;
    REQUEST_DATA rReqData;
    BOOL bSuccess = FALSE;
    CCommand* pCmd = NULL;
    UINT32* pCID = NULL;

    if (param == NULL)
        return;

    uiCID = (uintptr_t)param;

    pCID = (UINT32*)malloc(sizeof(UINT32));
    if (NULL == pCID)
        return;

    *pCID = uiCID;
    memset(&rReqData, 0, sizeof(REQUEST_DATA));

    if (!PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1),
                                                "AT+CGACT=0,%d\r", uiCID))
    {
        RIL_LOG_CRITICAL("triggerDeactivateDataCall() - Unable to create CGACT command!\r\n");
        goto Error;
    }

    rReqData.pContextData = pCID;
    rReqData.cbContextData = sizeof(UINT32);

    pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_DEACTIVATE_DATA_CALL].uiChannel,
                            NULL, RIL_REQUEST_DEACTIVATE_DATA_CALL, rReqData,
                            &CTE::ParseDeactivateDataCall,
                            &CTE::PostDeactivateDataCallCmdHandler);

    if (pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd, TRUE))
        {
            RIL_LOG_CRITICAL("triggerDeactivateDataCall() - Unable to queue command!\r\n");
            goto Error;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("triggerDeactivateDataCall() - Unable to allocate memory for new command!\r\n");
    }

    bSuccess = TRUE;
Error:
    if (!bSuccess)
    {
        free(pCID);
        delete pCmd;
    }

    RIL_LOG_VERBOSE("triggerDeactivateDataCall() - Exit\r\n");
}


// [in] param = RIL token
void triggerManualNetworkSearch(void* param)
{
    RIL_RESULT_CODE res = CTE::GetTE().RequestQueryAvailableNetworks((RIL_Token)param, NULL, 0);

    if (res != RRIL_RESULT_OK)
    {
        RIL_onRequestComplete((RIL_Token)param, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void triggerQueryCEER(void* /*param*/)
{
    CCommand* pCmd = new CCommand(RIL_CHANNEL_DLC8, NULL, REQ_ID_NONE,
                                    "AT+CEER\r",
                                    &CTE::ParseLastDataCallFailCause);

    if (pCmd)
    {
        pCmd->SetHighPriority();
        if (!CCommand::AddCmdToQueue(pCmd, TRUE))
        {
            RIL_LOG_CRITICAL("triggerQueryCEER() - Unable to queue command!\r\n");
            delete pCmd;
            pCmd = NULL;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("triggerQueryCEER() - Unable to allocate memory for new command!\r\n");
    }
}

void triggerQueryDefaultPDNContextParams(void* param)
{
    REQUEST_DATA rReqData;
    memset(&rReqData, 0, sizeof(REQUEST_DATA));
    CChannel_Data* pChannelData = NULL;
    UINT32 uiCID = 0;

    if (param == NULL)
        return;

    pChannelData = (CChannel_Data*)param;

    uiCID = pChannelData->GetContextID();

    RIL_LOG_VERBOSE("triggerQueryDefaultPDNContextParams - uiCID: %u\r\n", uiCID);

    if (PrintStringNullTerminate(rReqData.szCmd1, sizeof(rReqData.szCmd1), "AT+CGCONTRDP=%u\r",
            uiCID))
    {
        CCommand* pCmd = new CCommand(pChannelData->GetRilChannel(), NULL, REQ_ID_NONE,
                rReqData, &CTE::ParseReadContextParams,
                &CTE::PostReadDefaultPDNContextParams);
        if (pCmd)
        {
            if (!CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("triggerQueryDefaultPDNContextParams - "
                        "Unable to queue AT+CGCONTRDP command!\r\n");
                delete pCmd;
            }
        }
    }
}

void triggerQueryBearerParams(void* param)
{
    REQUEST_DATA rReqDataTFT;
    REQUEST_DATA rReqDataQOS;
    memset(&rReqDataTFT, 0, sizeof(REQUEST_DATA));
    memset(&rReqDataQOS, 0, sizeof(REQUEST_DATA));
    CChannel_Data* pChannelData = NULL;
    UINT32 uiPCID = 0;
    UINT32 uiCID = 0;
    void** callbackParams = NULL;

    if (param == NULL)
        return;

    callbackParams = (void**)param;
    uiPCID = (uintptr_t)callbackParams[0];
    uiCID = (uintptr_t)callbackParams[1];
    pChannelData = (CChannel_Data*)callbackParams[2];

    delete[] callbackParams;

    rReqDataTFT.pContextData = (void*) pChannelData;
    rReqDataQOS.pContextData = (void*) pChannelData;

    RIL_LOG_VERBOSE("triggerQueryBearerParams - uiPCID: %u, uiCID: %u\r\n", uiPCID, uiCID);

    if (PrintStringNullTerminate(rReqDataTFT.szCmd1, sizeof(rReqDataTFT.szCmd1),
            "AT+CGTFTRDP=%u\r", uiCID))
    {
        CCommand* pCmdTFT = new CCommand(pChannelData->GetRilChannel(), NULL, REQ_ID_NONE,
                rReqDataTFT, &CTE::ParseReadBearerTFTParams);
        if (pCmdTFT)
        {
            if (!CCommand::AddCmdToQueue(pCmdTFT))
            {
                RIL_LOG_CRITICAL("triggerQueryBearerParams - "
                        "Unable to queue AT+CGTFTRDP command!\r\n");
                delete pCmdTFT;
            }
        }
    }

    if (PrintStringNullTerminate(rReqDataQOS.szCmd1, sizeof(rReqDataQOS.szCmd1),
            "AT+CGEQOSRDP=%u\r", uiCID))
    {
        CCommand* pCmdQOS = new CCommand(pChannelData->GetRilChannel(), NULL, REQ_ID_NONE,
                rReqDataQOS, &CTE::ParseReadBearerQOSParams);
        if (pCmdQOS)
        {
            if (!CCommand::AddCmdToQueue(pCmdQOS))
            {
                RIL_LOG_CRITICAL("triggerQueryBearerParams - "
                        "Unable to queue AT+CGEQOS command!\r\n");
                delete pCmdQOS;
            }
        }
    }
}

// [in] param = 1 for mobile release and 0 for network release
void triggerDropCallEvent(void* param)
{
    sOEM_HOOK_RAW_UNSOL_CRASHTOOL_EVENT_IND data;
    char szBuffer[CRASHTOOL_BUFFER_SIZE];
    int nTempSize = 0;

    BOOL bMobileRelease = (1 == (intptr_t)param);

    data.command = RIL_OEM_HOOK_RAW_UNSOL_CRASHTOOL_EVENT_IND;
    data.type = CRASHTOOL_STATS;
    PrintStringNullTerminate(data.name, CRASHTOOL_NAME_SIZE, "TFT_STAT_CDROP");
    data.nameSize = strnlen(data.name, CRASHTOOL_NAME_SIZE);

    data.data1[0] = '\0';

    // Pre-initialize all data size to 0
    for (int i = 0; i < CRASHTOOL_NB_DATA; i++)
    {
        data.dataSize[i] = 0;
    }

    // See the definition of sOEM_HOOK_RAW_UNSOL_CRASHTOOL_EVENT_IND in
    // CORE/oemhookids.h for the raw unsol content.
    if (bMobileRelease)
    {
        PrintStringNullTerminate(data.data0, CRASHTOOL_BUFFER_SIZE, "MOBILE RELEASE");
        data.dataSize[0] = strnlen(data.data0, CRASHTOOL_BUFFER_SIZE);
    }
    else
    {
        nTempSize = snprintf(szBuffer, CRASHTOOL_BUFFER_SIZE, "%s", CTE::GetTE().GetLastCEER());
        if(nTempSize > -1 && nTempSize < CRASHTOOL_BUFFER_SIZE)
        {
            data.dataSize[0] = nTempSize;
            CopyStringNullTerminate(data.data0, szBuffer, CRASHTOOL_BUFFER_SIZE);
        }
    }

    if (strlen(CTE::GetTE().GetNetworkData(LAST_NETWORK_CREG)) != 0)
    {
        nTempSize = snprintf(szBuffer, CRASHTOOL_BUFFER_SIZE, "+CREG: %s;",
                CTE::GetTE().GetNetworkData(LAST_NETWORK_CREG));
        if(nTempSize > -1 && nTempSize < CRASHTOOL_BUFFER_SIZE)
        {
            data.dataSize[1] = nTempSize;
            CopyStringNullTerminate(data.data1, szBuffer, CRASHTOOL_BUFFER_SIZE);
        }
    }

    if (strlen(CTE::GetTE().GetNetworkData(LAST_NETWORK_CGREG)) != 0)
    {
        nTempSize = snprintf(szBuffer, CRASHTOOL_BUFFER_SIZE - data.dataSize[1],
                "+CGREG: %s;", CTE::GetTE().GetNetworkData(LAST_NETWORK_CGREG));
        if(nTempSize > -1 && nTempSize < (CRASHTOOL_BUFFER_SIZE - data.dataSize[1]))
        {
            data.dataSize[1] += nTempSize;
            strcat(data.data1, szBuffer);
        }
    }

    if (strlen(CTE::GetTE().GetNetworkData(LAST_NETWORK_XREG)) != 0)
    {
        nTempSize = snprintf(szBuffer, CRASHTOOL_BUFFER_SIZE - data.dataSize[1],
                "+XREG: %s;", CTE::GetTE().GetNetworkData(LAST_NETWORK_XREG));
        if(nTempSize > -1 && nTempSize < (CRASHTOOL_BUFFER_SIZE - data.dataSize[1]))
        {
            data.dataSize[1] += nTempSize;
            strcat(data.data1, szBuffer);
        }
    }

    if (strlen(CTE::GetTE().GetNetworkData(LAST_NETWORK_XCSQ)) != 0)
    {
        nTempSize = snprintf(szBuffer, CRASHTOOL_BUFFER_SIZE, "%s;",
                CTE::GetTE().GetNetworkData(LAST_NETWORK_XCSQ));
        if(nTempSize > -1 && nTempSize < CRASHTOOL_BUFFER_SIZE)
        {
            data.dataSize[2] = nTempSize;
            CopyStringNullTerminate(data.data2, szBuffer, CRASHTOOL_BUFFER_SIZE);
        }
    }

    nTempSize = snprintf(szBuffer, CRASHTOOL_BUFFER_SIZE, "%s,%s,%s",
            CTE::GetTE().GetNetworkData(LAST_NETWORK_OP_NAME_NUMERIC),
            CTE::GetTE().GetNetworkData(LAST_NETWORK_LAC),
            CTE::GetTE().GetNetworkData(LAST_NETWORK_CID));
    if(nTempSize > -1 && nTempSize < CRASHTOOL_BUFFER_SIZE)
    {
        data.dataSize[3] = nTempSize;
        CopyStringNullTerminate(data.data3, szBuffer, CRASHTOOL_BUFFER_SIZE);
    }

    nTempSize = snprintf(szBuffer, CRASHTOOL_BUFFER_SIZE, "%s",
            CTE::GetTE().GetNetworkData(LAST_NETWORK_OP_NAME_SHORT));
    if(nTempSize > -1 && nTempSize < CRASHTOOL_BUFFER_SIZE)
    {
        data.dataSize[4] = nTempSize;
        CopyStringNullTerminate(data.data4, szBuffer, CRASHTOOL_BUFFER_SIZE);
    }

    RIL_onUnsolicitedResponse (RIL_UNSOL_OEM_HOOK_RAW, (void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_CRASHTOOL_EVENT_IND));
}

void triggerCellInfoList(void* param)
{
    // querying cell information when radio request pending or not registered
    // results in no response from modem.
    if (NULL == param || CTE::GetTE().IsRadioRequestPending() || !CTE::GetTE().IsRegistered())
    {
        CTE::GetTE().SetCellInfoTimerRunning(FALSE);
        return;
    }

    // Get the CellInfo rate and compare.
    // if the newly set rate is less or equal,continue reading cellinfo from modem
    // if it is more, then start a new timed call back with the difference in timeout
    RIL_LOG_VERBOSE("triggerCellInfoList- Enter\r\n");
    int storedRate = CTE::GetTE().GetCellInfoListRate();
    int requestedRate = (intptr_t)param;
    RIL_LOG_INFO("triggerCellInfoList- StoredRate %d Rate with callback %d\r\n",
            storedRate, requestedRate);
    if (requestedRate >= storedRate || requestedRate <= 0)
    {
        REQUEST_DATA rReqData;

        memset(&rReqData, 0, sizeof(REQUEST_DATA));
        if (!CopyStringNullTerminate(rReqData.szCmd1, CTE::GetTE().GetReadCellInfoString(),
                sizeof(rReqData.szCmd1)))
        {
            RIL_LOG_CRITICAL("triggerCellInfoList() - Unable to create cellinfo command!\r\n");
            return;
        }

        rReqData.pContextData = (void*)(intptr_t)requestedRate;

        // The rate setting has not changed while waiting for time out
        // read the cell information and report to framework
        CCommand* pCmd = new CCommand(g_pReqInfo[RIL_REQUEST_GET_CELL_INFO_LIST].uiChannel,
                NULL, RIL_REQUEST_GET_CELL_INFO_LIST, rReqData,
                &CTE::ParseUnsolCellInfoListRate, &CTE::PostUnsolCellInfoListRate);

        if (pCmd)
        {
            if (!CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("triggerCellInfoList() - Unable to queue command!\r\n");
                delete pCmd;
                pCmd = NULL;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("triggerCellInfoList() - "
                    "Unable to allocate memory for new command!\r\n");
        }

        /*
         * requestedRate <= 0 means that the cell information query is triggered on
         * cell information change. Stop the timer only if the query is due to
         * timer expiry.
         */
        if (requestedRate > 0)
        {
            CTE::GetTE().SetCellInfoTimerRunning(FALSE);
        }
    }
    // the settings have changed to not to report CELLINFO
    else if (INT_MAX == storedRate)
    {
        CTE::GetTE().SetCellInfoTimerRunning(FALSE);
        RIL_LOG_INFO("triggerCellInfoList- Unsol cell info disabled: %d\r\n", storedRate);
    }
    else
    {
         // A new rate setting, re run the timer for the difference
         if (storedRate > requestedRate)
         {
             RIL_requestTimedCallback(triggerCellInfoList,
                     (void*)(intptr_t)storedRate, ((storedRate - requestedRate) / 1000), 0);
         }
    }
    RIL_LOG_VERBOSE("triggerCellInfoList- Exit\r\n");
}

void triggerSIMAppError(const void* param)
{
    sOEM_HOOK_RAW_UNSOL_SIM_APP_ERR_IND data;

    data.command = RIL_OEM_HOOK_RAW_UNSOL_SIM_APP_ERR_IND;
    PrintStringNullTerminate(data.szSimError, SIM_APP_ERROR_SIZE+1, (char*)param);

    RIL_onUnsolicitedResponse(RIL_UNSOL_OEM_HOOK_RAW, (void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_SIM_APP_ERR_IND));
}

void triggerCellBroadcastActivation(void* /*param*/)
{
    CTE::GetTE().HandleCellBroadcastActivation();
}

void triggerQueryUiccInfo(void* /*param*/)
{
    CTE::GetTE().QueryUiccInfo();
}
