////////////////////////////////////////////////////////////////////////////
// request_info_table.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Stores and provides information regarding a particular request.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "request_info_table.h"
#include "repository.h"
#include "te.h"
#include "hardwareconfig.h"

///////////////////////////////////////////////////////////////////////////////
CRequestInfoTable::CRequestInfoTable()
{
    m_rgpRequestInfos = new REQ_INFO* [REQ_ID_TOTAL];

    if (m_rgpRequestInfos != NULL)
    {
        memset(m_rgpRequestInfos, 0x00, REQ_ID_TOTAL * sizeof(REQ_INFO*));
    }
    else
    {
        RIL_LOG_CRITICAL("CRequestInfoTable::CRequestInfoTable() - \r\n"
                "Couldn't allocate m_rgpRequestInfos");
    }
    m_pCacheAccessMutex = new CMutex();
}

///////////////////////////////////////////////////////////////////////////////
CRequestInfoTable::~CRequestInfoTable()
{
    if (m_rgpRequestInfos != NULL)
    {
        for (int i = 0; i < REQ_ID_TOTAL; i++)
        {
            delete m_rgpRequestInfos[i];
            m_rgpRequestInfos[i] = NULL;
        }

        delete[] m_rgpRequestInfos;

        if (m_pCacheAccessMutex != NULL)
        {
            delete m_pCacheAccessMutex;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void CRequestInfoTable::GetRequestInfo(int requestID, REQ_INFO& rReqInfo)
{
    RIL_LOG_VERBOSE("CRequestInfoTable::GetRequestInfo() - Enter\r\n");

    // Set defaults if we can't find the request id
    rReqInfo.uiTimeout = CTE::GetTE().GetTimeoutAPIDefault();

    if (m_rgpRequestInfos == NULL)
    {
        RIL_LOG_INFO("CRequestInfoTable::GetRequestInfo() - m_rgpRequestInfos is NULL\r\n");
        goto Error;
    }
    else if (REQ_ID_NONE == requestID)
    {
        RIL_LOG_INFO("CRequestInfoTable::GetRequestInfo() - Request ID NONE given; assigning"
                " default values.\r\n");
        goto Error;
    }
    else if ((requestID >= REQ_ID_TOTAL && requestID < INTERNAL_REQ_ID_START) || (requestID < 0))
    {
        RIL_LOG_CRITICAL("CRequestInfoTable::GetRequestInfo() - Invalid request ID [0x%x]\r\n",
                requestID);
        goto Error;
    }

    // Internal request
    if (requestID >= INTERNAL_REQ_ID_START)
    {
        int index = requestID - INTERNAL_REQ_ID_START;

        CRepository repository;
        int iTemp = 0;

        memset(&rReqInfo, 0, sizeof(rReqInfo));

        rReqInfo.uiTimeout = CTE::GetTE().GetTimeoutAPIDefault();

        if (index < INTERNAL_REQ_ID_TOTAL)
        {
            if (repository.Read(g_szGroupInternalRequestTimeouts,
                    g_ReqInternal[index].reqInfo.szName, iTemp))
            {
                rReqInfo.uiTimeout = (UINT32)iTemp;
            }
        }
    }
    // Request from ril
    else if (NULL == m_rgpRequestInfos[requestID])
    {
        CRepository repository;
        int iTemp = 0;

        memset(&rReqInfo, 0, sizeof(rReqInfo));

        if (repository.Read(g_szGroupRequestTimeouts, g_pReqInfo[requestID].szName, iTemp))
        {
            rReqInfo.uiTimeout = (UINT32)iTemp;
        }
        else
        {
            rReqInfo.uiTimeout = CTE::GetTE().GetTimeoutAPIDefault();
        }

        // Use WAIT_FOREVER timeout if given time was 0
        if (!rReqInfo.uiTimeout)
        {
            rReqInfo.uiTimeout = WAIT_FOREVER;
        }

        else if (CHardwareConfig::GetInstance().IsMultiSIM())
        {
            // Timeout values need to be extended for Multi SIM capable modems,
            // depending on type of command (ie. network/non-network)
            switch (requestID)
            {
                case RIL_REQUEST_SETUP_DATA_CALL: // +CGACT, +CGDATA, +CGDCONT, +CGPADDR, +XDNS
                case RIL_REQUEST_DEACTIVATE_DATA_CALL:
                    rReqInfo.uiTimeout = 2 * rReqInfo.uiTimeout + 50000;
                    break;

                // network commands
                case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS: // +CCFC
                case RIL_REQUEST_SET_CALL_FORWARD:          // +CCFC
                case RIL_REQUEST_QUERY_CALL_WAITING:        // +CCWA
                case RIL_REQUEST_SET_CALL_WAITING:          // +CCWA
                case RIL_REQUEST_RADIO_POWER:               // +CFUN
                case RIL_REQUEST_DATA_CALL_LIST:            // +CGACT?
                case RIL_REQUEST_HANGUP:                    // +CHLD
                case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND: // +CHLD
                case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND: // +CHLD
                case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE: // +CHLD
                case RIL_REQUEST_CONFERENCE:                // +CHLD
                case RIL_REQUEST_UDUB:                      // +CHLD
                case RIL_REQUEST_SEPARATE_CONNECTION:       // +CHLD
                case RIL_REQUEST_EXPLICIT_CALL_TRANSFER:    // +CHLD
                case RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION: // +CLCK
                case RIL_REQUEST_QUERY_FACILITY_LOCK:       // +CLCK
                case RIL_REQUEST_SET_FACILITY_LOCK:         // +CLCK
                case RIL_REQUEST_GET_CLIR:                  // +CLIR?
                case RIL_REQUEST_SET_CLIR:                  // +CLIR
                case RIL_REQUEST_SEND_SMS:                  // +CMGS
                case RIL_REQUEST_SMS_ACKNOWLEDGE:           // +CNMA
                case RIL_REQUEST_OPERATOR:                  // +XCOPS
                case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE: // +COPS?
                case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS:  // +COPS=?
                case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC: // +COPS
                case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL: // +COPS
                case RIL_REQUEST_SEND_USSD:                 // +CUSD
                case RIL_REQUEST_CANCEL_USSD:               // +CUSD
                case RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM: // +SATD
                case RIL_REQUEST_DTMF_START:                // +XVTS
                case RIL_REQUEST_DTMF_STOP:                 // +XVTS
                case RIL_REQUEST_DIAL:                      // ATD
                // non-network cmds requiring response
                case RIL_REQUEST_DELETE_SMS_ON_SIM:         // +CMGD
                case RIL_REQUEST_SEND_SMS_EXPECT_MORE:      // +CMMS, +CMGS
                case RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG: // +CSCB?
                case RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION: // +CSCB
                case RIL_REQUEST_GET_NEIGHBORING_CELL_IDS:  // +XCELLINFO
                case RIL_REQUEST_GET_CELL_INFO_LIST:        // +XCELLINFO
                case RIL_REQUEST_REPORT_SMS_MEMORY_STATUS:  // +XTESM
                    rReqInfo.uiTimeout = 2 * rReqInfo.uiTimeout + 10000;
                    break;
            }
        }

        // Cache the data we just read (taking the cache access lock)
        if (m_pCacheAccessMutex)
        {
            CMutex::Lock(m_pCacheAccessMutex);
        }
        // Recheck if the cache is still empty
        if (NULL == m_rgpRequestInfos[requestID])
        {
            REQ_INFO* pNewReqInfo = new REQ_INFO;
            if (NULL != pNewReqInfo)
            {
                *pNewReqInfo = rReqInfo;
                m_rgpRequestInfos[requestID] = pNewReqInfo;
            }
        }
        if (m_pCacheAccessMutex)
        {
            CMutex::Unlock(m_pCacheAccessMutex);
        }
    }
    else
    {
        rReqInfo = *m_rgpRequestInfos[requestID];
    }

Error:
    RIL_LOG_INFO("CRequestInfoTable::GetRequestInfo() - RequestID %d: Timeout [%u]\r\n",
            requestID, rReqInfo.uiTimeout);

    RIL_LOG_VERBOSE("CRequestInfoTable::GetRequestInfo() - Exit\r\n");
}
