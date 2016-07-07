////////////////////////////////////////////////////////////////////////////
// silo_rfcoexistence.cpp
//
// Copyright 2012 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides response handlers and parsing functions for the rfcoexistence-related
//    RIL components.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "response.h"
#include "rildmain.h"
#include "callbacks.h"
#include "silo_rfcoexistence.h"
#include "extract.h"
#include "oemhookids.h"
#include "te.h"

#include <arpa/inet.h>

//
//
CSilo_rfcoexistence::CSilo_rfcoexistence(CChannel* pChannel)
: CSilo(pChannel)
{
    RIL_LOG_VERBOSE("CSilo_rfcoexistence::CSilo_rfcoexistence() - Enter\r\n");

    // AT Response Table
    static ATRSPTABLE pATRspTable[] =
    {
        { "+XMETRIC: "  , (PFN_ATRSP_PARSE)&CSilo_rfcoexistence::ParseXMETRIC },
        { "+XNRTCWSI: " , (PFN_ATRSP_PARSE)&CSilo_rfcoexistence::ParseXNRTCWSI },
        { ""           , (PFN_ATRSP_PARSE)&CSilo_rfcoexistence::ParseNULL }
    };

    m_pATRspTable = pATRspTable;
    RIL_LOG_VERBOSE("CSilo_rfcoexistence::CSilo_rfcoexistence() - Exit\r\n");
}


//
//
CSilo_rfcoexistence::~CSilo_rfcoexistence()
{
    RIL_LOG_VERBOSE("CSilo_rfcoexistence::~CSilo_rfcoexistence() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_rfcoexistence::~CSilo_rfcoexistence() - Exit\r\n");
}



BOOL CSilo_rfcoexistence::ParseXMETRIC(CResponse* const pResponse, const char*& rszPointer)
{
    // We have to prepend the URC name to its value
    return (CTE::GetTE().IsCoexReportActivated())
        ? ParseCoexReportURC(pResponse, rszPointer, "+XMETRIC: ")
        : ParseCoexURC(pResponse, rszPointer, "+XMETRIC: ");
}


BOOL CSilo_rfcoexistence::ParseXNRTCWSI(CResponse* const pResponse, const char*& rszPointer)
{
    // We have to prepend the URC name to its value
    return (CTE::GetTE().IsCoexReportActivated())
        ? ParseCoexReportURC(pResponse, rszPointer, "+XNRTCWSI: ")
        : ParseCoexURC(pResponse, rszPointer, "+XNRTCWSI: ");
}


//
// No complexity is needed to parse the URC received for Coexistence purpose (+XMETRIC/+XNRTCWS).
// The goal is just to extract the URC name/value and notify the up-layer (CWS Manager) about it.
//
BOOL CSilo_rfcoexistence::ParseCoexURC(CResponse* const pResponse, const char*& rszPointer,
                              const char* pUrcPrefix)
{
    RIL_LOG_VERBOSE("CSilo_rfcoexistence::ParseCoexURC() - Enter\r\n");

    BOOL fRet = FALSE;
    // prefix can be "+XMETRIC: " or "+XNRTCWSI: "
    const int MAX_PREFIX_SIZE = 12;
    // KW fix, total size of pData->response is COEX_INFO_BUFFER_SIZE
    // pData->response is made by pUrcPrefix + szExtInfo
    char szExtInfo[COEX_INFO_BUFFER_SIZE - MAX_PREFIX_SIZE] = {0};
    sOEM_HOOK_RAW_UNSOL_COEX_INFO* pData = NULL;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_rfcoexistence::ParseCoexURC() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Performing a backup of the URC string (rszPointer) into szExtInfo, to not modify rszPointer
    ExtractUnquotedString(rszPointer, '\r', szExtInfo,
                             (COEX_INFO_BUFFER_SIZE - MAX_PREFIX_SIZE), rszPointer);

    RIL_LOG_VERBOSE("CSilo_rfcoexistence::ParseCoexURC() - URC prefix=[%s] URC value=[%s]\r\n",
            pUrcPrefix, szExtInfo);

    // Creating the response
    pData = (sOEM_HOOK_RAW_UNSOL_COEX_INFO*)malloc(
            sizeof(sOEM_HOOK_RAW_UNSOL_COEX_INFO));
    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CSilo_rfcoexistence::ParseCoexURC() -"
                " Could not allocate memory for pData.\r\n");
        goto Error;
    }

    memset(pData, 0, sizeof(sOEM_HOOK_RAW_UNSOL_COEX_INFO));

    // pData.response will contain the final result (URC name + URC value)
    // Adding the prefix of the URC (+XMETRIC or +XNRTCWS) to pData->response
    if (!CopyStringNullTerminate(pData->response, pUrcPrefix, MAX_PREFIX_SIZE))
    {
        RIL_LOG_CRITICAL("CSilo_rfcoexistence:ParseCoexURC - Copy of URC prefix failed\r\n");
        goto Error;
    }

    // Adding the value of the URC to pData->response
    if (!ConcatenateStringNullTerminate(pData->response, sizeof(pData->response), szExtInfo))
    {
        RIL_LOG_CRITICAL("CSilo_rfcoexistence::ParseCoexURC() : Failed to concat the URC "
                "prefix to its value!\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CSilo_rfcoexistence::ParseCoexURC() - Final Response=[%s]\r\n", pData->response);

    pData->command = RIL_OEM_HOOK_RAW_UNSOL_COEX_INFO;
    pData->responseSize = strnlen(pData->response, COEX_INFO_BUFFER_SIZE);

    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData((void*)pData,
            sizeof(sOEM_HOOK_RAW_UNSOL_COEX_INFO), FALSE))
    {
        goto Error;
    }

    fRet = TRUE;
Error:
    if (!fRet)
    {
        free(pData);
        pData = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_rfcoexistence::ParseCoexURC() - Exit\r\n");
    return fRet;
}

//
// No complexity is needed to parse the URC received for Coexistence purpose (+XMETRIC/+XNRTCWS).
// The goal is just to extract the URC name/value and notify the up-layer (CWS Manager) about it.
//
BOOL CSilo_rfcoexistence::ParseCoexReportURC(CResponse* const pResponse, const char*& rszPointer,
                              const char* pUrcPrefix)
{
    RIL_LOG_VERBOSE("CSilo_rfcoexistence::ParseCoexReportURC() - Enter\r\n");

    BOOL bRet = FALSE;
    // prefix can be "+XMETRIC: " or "+XNRTCWSI: "
    const int MAX_PREFIX_SIZE = 12;
    // KW fix, total size of pData->response is COEX_INFO_BUFFER_SIZE
    // pData->response is made by pUrcPrefix + szExtInfo
    char szExtInfo[COEX_INFO_BUFFER_SIZE - MAX_PREFIX_SIZE] = {0};
    sOEM_HOOK_RAW_UNSOL_COEX_REPORT* pData = NULL;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_rfcoexistence::ParseCoexReportURC() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Extract the URC string (rszPointer) into szExtInfo, to not modify rszPointer
    ExtractUnquotedString(rszPointer, '\r', szExtInfo,
                             (COEX_INFO_BUFFER_SIZE - MAX_PREFIX_SIZE), rszPointer);

    RIL_LOG_VERBOSE("CSilo_rfcoexistence::ParseCoexReportURC()- URC prefix=[%s] URC value=[%s]\r\n",
            pUrcPrefix, szExtInfo);

    // Creating the response
    pData = (sOEM_HOOK_RAW_UNSOL_COEX_REPORT*) malloc(sizeof(sOEM_HOOK_RAW_UNSOL_COEX_REPORT));
    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CSilo_rfcoexistence::ParseCoexReportURC() -"
                " Could not allocate memory for pData.\r\n");
        goto Error;
    }

    memset(pData, 0, sizeof(sOEM_HOOK_RAW_UNSOL_COEX_REPORT));

    // pData.response will contain the final result (URC name + URC value)
    // Adding the prefix of the URC (+XMETRIC or +XNRTCWS) to pData->response
    if (!CopyStringNullTerminate(pData->response, pUrcPrefix, MAX_PREFIX_SIZE))
    {
        RIL_LOG_CRITICAL("CSilo_rfcoexistence:ParseCoexReportURC - Copy of URC prefix failed\r\n");
        goto Error;
    }

    // Adding the value of the URC to pData->response
    if (!ConcatenateStringNullTerminate(pData->response, sizeof(pData->response), szExtInfo))
    {
        RIL_LOG_CRITICAL("CSilo_rfcoexistence::ParseCoexReportURC() : Failed to concat the URC "
                "prefix to its value!\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CSilo_rfcoexistence::ParseCoexReportURC() - Final Response=[%s]\r\n",
            pData->response);

    pData->commandId = RIL_OEM_HOOK_RAW_UNSOL_COEX_REPORT;
    pData->responseSize = strlen(pData->response);

    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData(pData, sizeof(sOEM_HOOK_RAW_UNSOL_COEX_REPORT), FALSE))
    {
        goto Error;
    }

    bRet = TRUE;
Error:
    if (!bRet)
    {
        free(pData);
        pData = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_rfcoexistence::ParseCoexReportURC() - Exit\r\n");
    return bRet;
}
