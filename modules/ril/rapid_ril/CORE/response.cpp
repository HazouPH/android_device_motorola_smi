////////////////////////////////////////////////////////////////////////////
// response.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for generic helper functions
//    used to process modem responses to AT Commands.
//
/////////////////////////////////////////////////////////////////////////////

// system include
#include <stdio.h>

// local includes
#include "types.h"
#include "rril.h"
#include "util.h"
#include "thread_ops.h"
#include "extract.h"
#include "silo.h"
#include "channelbase.h"
#include "channel_nd.h"
#include "systemmanager.h"
#include "rildmain.h"
#include "reset.h"
#include "response.h"
#include "command.h"
#include "te.h"
#include "ril_result.h"

const char* pszOkResponse    = "OK";
const char* pszErrorResponse = "ERROR";
const char* pszSMSResponse   = "> ";
const char* pszCMEError      = "+CME ERROR:";
const char* pszCMSError      = "+CMS ERROR:";
const char* pszConnectResponse = "CONNECT";
const char* pszNoCarrierResponse = "NO CARRIER";
const char* pszAborted       = "ABORTED";


///////////////////////////////////////////////////////////////////////////////
CResponse::CResponse(CChannel* pChannel) :
    m_uiResultCode(RRIL_RESULT_OK),
    m_uiErrorCode(0),
    m_pData(NULL),
    m_uiDataSize(0),
    m_pChannel(pChannel),
    m_uiResponseEndMarker(0),
    m_uiFlags(0)
{
    CopyStringNullTerminate(m_szNewLine, "\r\n", sizeof(m_szNewLine));
}


///////////////////////////////////////////////////////////////////////////////
CResponse::~CResponse()
{
    if (m_uiDataSize)
    {
        FreeData();
    }
}


//
//
//
BOOL CResponse::IsCompleteResponse()
{
    BOOL bRet = FALSE;
    RIL_LOG_VERBOSE("CResponse::IsCompleteResponse() : Enter\r\n");

    if (m_uiUsed > 0)
    {
        // NULL-terminate the response string temporarily
        m_szBuffer[m_uiUsed] = '\0';

        // data in the buffer; check the type of data received
        if (IsUnsolicitedResponse())
        {
            bRet = TRUE;
        }
        else if (IsExtendedError(pszCMEError))
        {
            bRet = TRUE;
        }
        else if (IsExtendedError(pszCMSError))
        {
            bRet = TRUE;
        }
        else if (IsOkResponse())
        {
            bRet = TRUE;
        }
        else if (IsErrorResponse())
        {
            bRet = TRUE;
        }
        else if (IsConnectResponse())
        {
            bRet = TRUE;
        }
        else if (IsAbortedResponse())
        {
            bRet = TRUE;
        }
        else if (IsCorruptResponse())
        {
            bRet = TRUE;
        }
    }

    RIL_LOG_VERBOSE("CResponse::IsCompleteResponse() : Exit [%d]\r\n", bRet);
    return bRet;
}


//
//
//
BOOL CResponse::IsUnsolicitedResponse()
{
    BOOL bRet = FALSE;
    BOOL bGotoError = FALSE;
    const char* szPointer = m_szBuffer;

    RIL_LOG_VERBOSE("CResponse::IsUnsolicitedResponse() : Enter\r\n");

    // Parse "<prefix>" if it exists.
    SkipRspStart(szPointer, m_szNewLine, szPointer);

    SetUnsolicitedFlag(FALSE);

    m_pChannel->ParseUnsolicitedResponse(this, szPointer, bGotoError);

    if (IsUnsolicitedFlag())
    {
        bRet = TRUE;
        if (!FindAndSkipRspEnd(szPointer, m_szNewLine, szPointer))
        {
            RIL_LOG_CRITICAL("CResponse::IsUnsolicitedResponse() - chnl=[%d] no CRLF at end of"
                    " response: \"%s\"\r\n", m_pChannel->GetRilChannel(),
                    CRLFExpandedString(szPointer, strlen(szPointer)).GetString());
        }
        else
        {
            m_uiResponseEndMarker = szPointer - m_szBuffer;
        }
    }

Error:
    RIL_LOG_VERBOSE("CResponse::IsUnsolicitedResponse() : Exit[%d]\r\n", bRet);
    return bRet;
}


//
//
//
BOOL CResponse::IsExtendedError(const char* pszToken)
{
    BOOL bRet = FALSE;
    const char* szPointer = m_szBuffer;

    RIL_LOG_VERBOSE("CResponse::IsExtendedError() : Enter\r\n");

    if (FindAndSkipString(szPointer, pszToken, szPointer))
    {
        RIL_LOG_INFO("chnl=[%d] Got %s response\r\n", m_pChannel->GetRilChannel(), pszToken);

        // get error code
        UINT32 nCode;
        if (!RetrieveErrorCode(szPointer, nCode, pszToken))
        {
            RIL_LOG_CRITICAL("CResponse::IsExtendedError() - chnl=[%d] could not extract error"
                    " code\r\n", m_pChannel->GetRilChannel());
            // treat as unrecognized - discard everything until the CR LF or end of buffer
            if (!SkipRspEnd(szPointer, m_szNewLine, szPointer))
            {
                // no CR LF - discard everything
                m_uiResponseEndMarker = m_uiUsed;
            }
            else
            {
                m_uiResponseEndMarker = szPointer - m_szBuffer;
            }
            SetUnrecognizedFlag(TRUE);
            bRet = TRUE;
            goto Error;
        }

        if (!SkipRspEnd(szPointer, m_szNewLine, szPointer))
        {
            RIL_LOG_CRITICAL("CResponse::IsExtendedError() - chnl=[%d] no CRLF at end of response:"
                    " \"%s\"\r\n",
                    m_pChannel->GetRilChannel(),
                    CRLFExpandedString(szPointer, strlen(szPointer)).GetString());
            goto Error;
        }
        m_uiResponseEndMarker = szPointer - m_szBuffer;
        bRet = TRUE;
    }

Error:
    RIL_LOG_VERBOSE("CResponse::IsExtendedError() : Exit[%d]\r\n", bRet);
    return bRet;
}


//
//
//
BOOL CResponse::IsCorruptResponse()
{
    BOOL bRet = FALSE;
    const char* szDummy = NULL;

    RIL_LOG_VERBOSE("CResponse::IsCorruptResponse() : Enter\r\n");

    if (IsCorruptFlag())
    {
        RIL_LOG_INFO("CResponse::IsCorruptResponse() : chnl=[%d] Attempting to filter corrupt"
                " response\r\n", m_pChannel->GetRilChannel());
        SetCorruptFlag(FALSE);

        // heuristic used is to parse everything up to the first CR.
        const char* szPointer = m_szBuffer;
        if (FindAndSkipString(szPointer, "\r", szPointer))
        {
            // if the CR is followed by a LF, then consume that, too.
            if (SkipString(szPointer, "\n", szPointer))
            {
                //  If next item is \r, then stop here
                if (SkipString(szPointer+1, "\r", szDummy))
                {


                    // treat as unrecognized
                    SetUnrecognizedFlag(TRUE);
                    m_uiResponseEndMarker = szPointer - m_szBuffer;
                    bRet = TRUE;
                }
            }
        }
    }

    RIL_LOG_VERBOSE("CResponse::IsCorruptResponse() : Exit[%d]\r\n", bRet);
    return bRet;
}


//
//
//
BOOL CResponse::IsOkResponse()
{
    const char* szPointer = m_szBuffer;
    char szToken[MAX_BUFFER_SIZE] = {0};
    BOOL bRet;

    RIL_LOG_VERBOSE("CResponse::IsOkResponse() : Enter\r\n");

    // look for "OK" in response data
    snprintf(szToken, sizeof(szToken), "%s%s%s", m_szNewLine, pszOkResponse, m_szNewLine);
    bRet = FindAndSkipString(szPointer, szToken, szPointer);

    if (!bRet)
    {
        //  Maybe we have just OK<cr><lf> due to modem missing a character
        snprintf(szToken, sizeof(szToken), "%s%s", pszOkResponse, m_szNewLine);
        bRet = FindAndSkipString(szPointer, szToken, szPointer);
    }

    if (!bRet)
    {
        // No "OK" in buffer - is this an SMS intermediate prompt?
        // look for SMS token from the beginning of the buffer only
        bRet = SkipRspStart(szPointer, m_szNewLine, szPointer) &&
               SkipString(szPointer, pszSMSResponse, szPointer);
    }

    if (bRet)
    {
        SetUnsolicitedFlag(FALSE);
        m_uiResponseEndMarker = szPointer - m_szBuffer;
        m_uiResultCode = RIL_E_SUCCESS;
    }

Error:
    RIL_LOG_VERBOSE("CResponse::IsOkResponse() : Exit [%d]\r\n", bRet);
    return bRet;
}


//
//
//
BOOL CResponse::IsErrorResponse()
{
    const char* szPointer = m_szBuffer;
    char szToken[MAX_BUFFER_SIZE] = {0};
    BOOL bRet = FALSE;

    RIL_LOG_VERBOSE("CResponse::IsErrorResponse() : Enter\r\n");

    // look for "ERROR" in response data
    snprintf(szToken, sizeof(szToken), "%s%s%s", m_szNewLine, pszErrorResponse, m_szNewLine);
    bRet = FindAndSkipString(szPointer, szToken, szPointer);

    if (bRet)
    {
        // found ERROR response
        RIL_RESULT_CODE resCode;
        SetUnsolicitedFlag(FALSE);
        m_uiResultCode = RIL_E_GENERIC_FAILURE;

        m_uiResponseEndMarker = szPointer - m_szBuffer;
        bRet = TRUE;
    }

Error:
    RIL_LOG_VERBOSE("CResponse::IsErrorResponse() : Exit [%d]\r\n", bRet);
    return bRet;
}

//
//
//
BOOL CResponse::IsConnectResponse()
{
    const char* szPointer = m_szBuffer;
    char szToken[MAX_BUFFER_SIZE] = {0};
    BOOL bRet;

    RIL_LOG_VERBOSE("CResponse::IsConnectResponse() : Enter\r\n");

    // look for "CONNECT" in response data
    snprintf(szToken, sizeof(szToken), "%s%s%s", m_szNewLine, pszConnectResponse, m_szNewLine);
    bRet = FindAndSkipString(szPointer, szToken, szPointer);

    if (bRet)
    {
        SetUnsolicitedFlag(FALSE);
        m_uiResponseEndMarker = szPointer - m_szBuffer;
        m_uiResultCode = RIL_E_SUCCESS;
    }

Error:
    RIL_LOG_VERBOSE("CResponse::IsConnectResponse() : Exit [%d]\r\n", bRet);
    return bRet;
}

//
//
//
BOOL CResponse::IsAbortedResponse()
{
    const char* szPointer = m_szBuffer;
    char szToken[MAX_BUFFER_SIZE] = {0};
    BOOL bRet;

    RIL_LOG_VERBOSE("CResponse::IsAbortedResponse() : Enter\r\n");

    // look for "ABORTED" in response data
    snprintf(szToken, sizeof(szToken), "%s%s%s", m_szNewLine, pszAborted, m_szNewLine);
    bRet = FindAndSkipString(szPointer, szToken, szPointer);

    if (bRet)
    {
        SetUnsolicitedFlag(FALSE);
        m_uiResponseEndMarker = szPointer - m_szBuffer;
        m_uiResultCode = RIL_E_SUCCESS;
    }

Error:
    RIL_LOG_VERBOSE("CResponse::IsAbortedResponse() : Exit [%d]\r\n", bRet);
    return bRet;
}



//
//
//
BOOL CResponse::TransferData(CResponse*& rpRspIn, CResponse*& rpRspOut)
{
    BOOL       bRet = FALSE;
    CResponse* pRspTmp;
    int        iRemainder;

    RIL_LOG_VERBOSE("CResponse::TransferData() : Enter\r\n");

    // rpRspIn must be NOT NULL, rpRspOut must be NULL
    if (NULL == rpRspIn || NULL != rpRspOut
            || 0 == rpRspIn->m_uiUsed || 0 == rpRspIn->m_uiResponseEndMarker)
    {
        RIL_LOG_CRITICAL("CResponse::TransferData() : Invalid parameters\r\n");
        goto Error;
    }

    pRspTmp = rpRspIn;
    rpRspIn = new CResponse(pRspTmp->m_pChannel);
    if (!rpRspIn)
    {
        RIL_LOG_CRITICAL("CResponse::TransferData() : Out of memory\r\n");
        rpRspIn = pRspTmp;
        goto Error;
    }

    // RspOut contains the valid response, RspIn keeps the remainder
    rpRspOut = pRspTmp;
    iRemainder = rpRspOut->m_uiUsed - rpRspOut->m_uiResponseEndMarker;
    if (iRemainder > 0)
    {
        rpRspIn->Append(rpRspOut->m_szBuffer + rpRspOut->m_uiResponseEndMarker, iRemainder);
        rpRspOut->m_uiUsed -= iRemainder;
    }
    rpRspOut->m_szBuffer[rpRspOut->m_uiUsed] = '\0';

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CResponse::TransferData() : Exit [%d]\r\n", bRet);
    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
BOOL CResponse::SetData(void* pData, const UINT32 nSize, const BOOL fCpyMem)
{
    RIL_LOG_VERBOSE("CResponse::SetData() - Enter\r\n");
    BOOL bRet = FALSE;

    if (NULL != m_pData || 0 != m_uiDataSize)
    {
        RIL_LOG_WARNING("CResponse::SetData() : WARN : m_pData or m_uiDataSize were not NULL\r\n");
    }

    free(m_pData);
    m_pData = NULL;
    m_uiDataSize = 0;

    if (nSize)
    {
        if (NULL == pData)
        {
            RIL_LOG_CRITICAL("CResponse::SetData() : pData was NULL  nSize=[%d]\r\n", nSize);
            goto Error;
        }

        if (fCpyMem)
        {
            m_pData = malloc(nSize);
            if (!m_pData)
            {
                // Critically low on memory
                DO_REQUEST_CLEAN_UP(1, "Out of memory");
                goto Error;
            }
            memcpy(m_pData, pData, nSize);
        }
        else
        {
            // Just take ownership of the allocated memory and free it later
            m_pData = pData;
        }
    }

    m_uiDataSize = nSize;
    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CResponse::SetData() - Exit\r\n");
    return bRet;
}

BOOL CResponse::RetrieveErrorCode(const char*& rszPointer, UINT32& nCode, const char* pszToken)
{
    RIL_LOG_VERBOSE("CResponse::RetrieveErrorCode() - Enter\r\n");
    RIL_RESULT_CODE resCode = RIL_E_GENERIC_FAILURE;
    const char* szDummy;
    BOOL bRet = FALSE;

    // Look for a "<cr>" to make sure we got the whole number
    if (!FindAndSkipString(rszPointer, "\r", szDummy))
    {
        // This isn't a complete error notification -- no need to parse it
        goto Error;
    }

    bRet = ExtractUInt32(rszPointer, nCode, rszPointer);

    if (!bRet)
    {
        goto Error;
    }

    switch(nCode)
    {
        case CME_ERROR_ILLEGAL_MS:
        case CME_ERROR_ILLEGAL_ME:
            SetResultCode(RIL_E_ILLEGAL_SIM_OR_ME);
            break;
        default:
            SetResultCode(RIL_E_GENERIC_FAILURE);
            break;
    }

    SetErrorCode(nCode);
    SetUnsolicitedFlag(FALSE);

    RIL_LOG_INFO("CResponse::RetrieveErrorCode() : Result: 0x%X   %s %d\r\n", m_uiResultCode,
            pszToken, nCode);

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CResponse::RetrieveErrorCode() - Exit\r\n");
    return bRet;
}

//
//
//
BOOL CResponse::ParseResponse(CCommand*& rpCmd)
{
    RIL_LOG_VERBOSE("CResponse::ParseResponse() - Enter\r\n");
    RIL_RESULT_CODE resCode;
    PFN_TE_PARSE parser = rpCmd->GetParseFcn();

    if (parser)
    {
        free(m_pData);
        m_pData = NULL;
        m_uiDataSize = 0;

        RESPONSE_DATA rspData;
        memset(&rspData, 0, sizeof(RESPONSE_DATA));

        rspData.szResponse = m_szBuffer;
        rspData.uiChannel = m_pChannel->GetRilChannel();

        rspData.pContextData = rpCmd->GetContextData();
        rspData.cbContextData = rpCmd->GetContextDataSize();

        rspData.pContextData2 = rpCmd->GetContextData2();
        rspData.cbContextData2 = rpCmd->GetContextDataSize2();

        rspData.uiErrorCode = GetErrorCode();
        rspData.uiResultCode = GetResultCode();

        RIL_LOG_VERBOSE("CResponse::ParseResponse() : chnl=[%d] Calling Parsing Function\r\n",
                m_pChannel->GetRilChannel() );
        resCode = (CTE::GetTE().*parser)(rspData);

        m_pData = rspData.pData;
        m_uiDataSize = rspData.uiDataSize;

        if (RIL_E_SUCCESS != resCode)
        {
            RIL_LOG_CRITICAL("CResponse::ParseResponse() - chnl=[%d] Error parsing response:"
                    " \"%s\"; resCode = 0x%x\r\n", m_pChannel->GetRilChannel(),
                    CRLFExpandedString(m_szBuffer, strlen(m_szBuffer)).GetString(),
                    resCode);

            SetResultCode(resCode);
            SetUnsolicitedFlag(FALSE);
        }
        else
        {
            SetResultCode(rspData.uiResultCode);
        }
    }

Error:
    RIL_LOG_VERBOSE("CResponse::ParseResponse() - Exit\r\n");
    return TRUE;
}

