////////////////////////////////////////////////////////////////////////////
// silo_voice.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides response handlers and parsing functions for the voice-related
//    RIL components.
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "types.h"
#include "rillog.h"
#include "channel_nd.h"
#include "response.h"
#include "extract.h"
#include "rildmain.h"
#include "silo_voice.h"
#include "callbacks.h"
#include "te.h"
#include "oemhookids.h"

//
//
CSilo_Voice::CSilo_Voice(CChannel* pChannel)
: CSilo(pChannel),
  m_uiCallId(0)
{
    RIL_LOG_VERBOSE("CSilo_Voice::CSilo_Voice() - Enter\r\n");

    // AT Response Table
    static ATRSPTABLE pATRspTable[] =
    {
        { "+CRING: "      , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseExtRing },
        { "DISCONNECT"  , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseDISCONNECT },
        { "+XCALLSTAT: "  , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseXCALLSTAT },
        { "CONNECT"       , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseConnect },
        { "+CNAP: ", (PFN_ATRSP_PARSE)&CSilo_Voice::ParseCNAP },
        { "+CLIP: ", (PFN_ATRSP_PARSE)&CSilo_Voice::ParseCLIP },
        { "+CCWA: "       , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseCallWaitingInfo },
        { "+CSSU: "       , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseUnsolicitedSSInfo },
        { "+CSSI: "       , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseIntermediateSSInfo },
        { "+CCCM: "       , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseCallMeter },
        { "+CUSD: "       , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseUSSDInfo },
        { "+COLP: "       , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseUnrecognized },
        { "+COLR: "       , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseConnLineIdRestriction },
        { "+XCIEV: "      , (PFN_ATRSP_PARSE)/*&CSilo_Voice::ParseIndicatorEvent*/
                &CSilo_Voice::ParseUnrecognized },
        { "+XCIEV:"      , (PFN_ATRSP_PARSE)/*&CSilo_Voice::ParseIndicatorEvent*/
                &CSilo_Voice::ParseUnrecognized },
        { "+XCALLINFO: "  , (PFN_ATRSP_PARSE)/*&CSilo_Voice::ParseCallProgressInformation*/
                &CSilo_Voice::ParseUnrecognized },
        { "BUSY"          , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseBusy },
        { "NO ANSWER"     , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseNoAnswer },
        { "CTM CALL"      , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseCTMCall },
        { "NO CTM CALL"   , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseNoCTMCall },
        { "WAITING CALL CTM" , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseWaitingCallCTM },
        { "+XUCCI: ", (PFN_ATRSP_PARSE)&CSilo_Voice::ParseXUCCI },
        { ""              , (PFN_ATRSP_PARSE)&CSilo_Voice::ParseNULL }
    };

    m_pATRspTable = pATRspTable;
    RIL_LOG_VERBOSE("CSilo_Voice::CSilo_Voice() - Exit\r\n");
}

//
//
CSilo_Voice::~CSilo_Voice()
{
    RIL_LOG_VERBOSE("CSilo_Voice::~CSilo_Voice() - Enter / Exit\r\n");
}

char* CSilo_Voice::GetBasicInitString()
{
    if (CTE::GetTE().IsVoiceCapable())
    {
        // voice silo-related channel basic init string
        const char szVoiceBasicInitString[] = "+XCALLNBMMI=1";

        if (!ConcatenateStringNullTerminate(m_szBasicInitString,
                sizeof(m_szBasicInitString), szVoiceBasicInitString))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::GetBasicInitString() : Failed to copy "
                    "basic init string!\r\n");
            return NULL;
        }
    }

    return m_szBasicInitString;
}

char* CSilo_Voice::GetURCInitString()
{
    // voice silo-related URC channel basic init string
    const char* pszVoiceURCInitString = CTE::GetTE().GetSiloVoiceURCInitString();

    if (!ConcatenateStringNullTerminate(m_szURCInitString,
            sizeof(m_szURCInitString), pszVoiceURCInitString))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::GetURCInitString() : Failed to copy "
                "basic init string!\r\n");
        return NULL;
    }

    return m_szURCInitString;
}

char* CSilo_Voice::GetURCUnlockInitString()
{
    // voice silo-related URC channel unlock init string
    const char szVoiceUnlockInitString[] = "+CUSD=1|+CCWA=1";

    if (CTE::GetTE().IsVoiceCapable())
    {
        if (!ConcatenateStringNullTerminate(m_szURCUnlockInitString,
                sizeof(m_szURCUnlockInitString), szVoiceUnlockInitString))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::GetURCUnlockInitString() : Failed to copy URC "
                    "unlock init string!\r\n");
            return NULL;
        }
    }
    return m_szURCUnlockInitString;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//  Parse functions here
///////////////////////////////////////////////////////////////////////////////////////////////

BOOL CSilo_Voice::ParseExtRing(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseExtRing() - Enter\r\n");

    BOOL fRet = FALSE;
    const char* szDummy = NULL;
    char szType[MAX_BUFFER_SIZE] = {0};

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseExtRing() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Make sure this is a complete notification
    if(!FindAndSkipRspEnd(rszPointer, m_szNewLine, szDummy))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseExtRing() : incomplete notification\r\n");
        goto Error;
    }

    //  Extract <type>
    if (!ExtractUnquotedString(rszPointer, m_cTerminator, szType, MAX_BUFFER_SIZE, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseExtRing() : cannot extract <type>\r\n");
        goto Error;
    }

    //  Determine what kind of RING this is.
    if (0 == strcmp(szType, "VOICE") || 0 == strcmp(szType, "CTM"))
    {
        //  Normal case, just send ring notification.
        RIL_LOG_INFO("CSilo_Voice::ParseExtRing() : Incoming voice call\r\n");
        pResponse->SetResultCode(RIL_UNSOL_CALL_RING);

        /*
         * XCALLSTAT received before CRING.
         * Since the call type is not known via XCALLSTAT URC,
         * CALL_STATE_CHANGED notification is triggered from here.
         */
        notifyChangedCallState(NULL);
    }
    //  Check to see if incoming video telephony call
    else if (0 == strcmp(szType, "SYNC"))
    {
        RIL_LOG_INFO("CSilo_Voice::ParseExtRing() : Incoming video telephony call\r\n");

        pResponse->SetIgnoreFlag(TRUE);

        triggerHangup(m_uiCallId);
    }
    else if (NULL != strstr(szType, "GPRS"))
    {
        RIL_LOG_INFO("CSilo_Voice::ParseExtRing() : NW initiated PDP context or"
                " dedicated EPS bearer\r\n");
        pResponse->SetIgnoreFlag(TRUE);
        CTE::GetTE().AcceptOrRejectNwInitiatedContext();
    }
    else
    {
        //  Unknown incoming ring
        RIL_LOG_INFO("CSilo_Voice::ParseExtRing() : Unknown incoming call type=[%s]\r\n", szType);
        pResponse->SetIgnoreFlag(TRUE);
        triggerHangup(m_uiCallId);
    }
    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseExtRing() - Exit\r\n");
    return fRet;
}

#undef Snprintf


//
//
BOOL CSilo_Voice::ParseConnect(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseConnect() - Enter\r\n");

    BOOL fRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseConnect() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);

    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseConnect() - Exit\r\n");
    return fRet;
}

//
//
//
BOOL CSilo_Voice::ParseXCALLSTAT(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseXCALLSTAT() - Enter\r\n");

    char szAddress[MAX_BUFFER_SIZE];
    BOOL fRet = FALSE;

    const char* szDummy = NULL;
    UINT32 uiID = 0;
    UINT32 uiStat = 0;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXCALLSTAT() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Look for a "<postfix>"
    if (!FindAndSkipRspEnd(rszPointer, m_szNewLine, szDummy))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXCALLSTAT() : Incomplete notification\r\n");
        goto Error;
    }

    //  Extract <id>
    if (!ExtractUInt32(rszPointer, uiID, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXCALLSTAT() : Could not extract uiID\r\n");
        goto Error;
    }

    //  Extract ,<stat>
    if (!SkipString(rszPointer, ",", rszPointer) ||
        !ExtractUInt32(rszPointer, uiStat, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXCALLSTAT() : Could not extract uiStat\r\n");
        goto Error;
    }

    switch (uiStat)
    {
        case E_CALL_STATUS_INCOMING:
            CTE::GetTE().SetIncomingCallStatus(uiID, uiStat);
        case E_CALL_STATUS_WAITING:
            /*
             * Since the call type is not known, CALL_STATE_CHANGED is postponed
             * until the call type is known.
             * For incoming call, +CRING: provides the call type.
             * For waiting call, +CCWA: provides the call type.
             */
            m_uiCallId = uiID;
            pResponse->SetIgnoreFlag(TRUE);
            break;

        case E_CALL_STATUS_ACTIVE:
        case E_CALL_STATUS_CONNECTED:
            /*
             * If the status change is for an incoming call, then update
             * that the ANSWER request is successfully completed by network.
             */
            if (CTE::GetTE().GetIncomingCallId() == uiID)
            {
                CTE::GetTE().SetIncomingCallStatus(uiID, uiStat);
            }

            pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
            break;

        case E_CALL_STATUS_DISCONNECTED:
            m_uiCallId = 0;
            CTE::GetTE().ResetCnapParameters();
            CTE::GetTE().ResetNumberParameters();
            CTE::GetTE().SetIncomingCallStatus(0, uiStat);
            // set the flag to clear all pending chld requests
            CTE::GetTE().SetClearPendingCHLDs(TRUE);
            // Fall through
        default:
            pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
            break;
    }

    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseXCALLSTAT() - Exit\r\n");
    return fRet;
}


//
//
//
BOOL CSilo_Voice::ParseCallWaitingInfo(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCallWaitingInfo() - Enter\r\n");

    char szAddress[MAX_BUFFER_SIZE];
    BOOL fRet = FALSE;
    char* pszTempBuffer = NULL;
    int nNumParams = 1;
    const char* szDummy;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseCallWaitingInfo() : pResponse was NULL\r\n");
        goto Error;
    }

    // Look for a "<postfix>"
    if (!FindAndSkipRspEnd(rszPointer, m_szNewLine, szDummy))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseCallWaitingInfo() : Could not find response end\r\n");
        goto Error;
    }

    pszTempBuffer = new char[szDummy-rszPointer+1];
    if (!pszTempBuffer)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseCallWaitingInfo() -"
                " cannot allocate pszTempBuffer\r\n");
        goto Error;
    }
    memset(pszTempBuffer, 0, szDummy-rszPointer+1);
    strncpy(pszTempBuffer, rszPointer, szDummy-rszPointer);
    pszTempBuffer[szDummy-rszPointer] = '\0';  // Klokworks fix

    //  Loop and count parameters
    szDummy = pszTempBuffer;
    while(FindAndSkipString(szDummy, ",", szDummy))
    {
        nNumParams++;
    }

    RIL_LOG_INFO("CSilo_Voice::ParseCallWaitingInfo(): Number of parameters in +CCWA=%d\r\n",
            nNumParams);
    //If number of parameters is more than 2, +CCWA came as part of voice call and is a URC
    //otherwise this is a response to AT+CCWA= command and need not be processed here.
    if (nNumParams > 2)
    {
        const char* pDummy;
        char number[MAX_BUFFER_SIZE];
        UINT32 uiType = 0;
        UINT32 uiClass = 0;

        pResponse->SetUnsolicitedFlag(TRUE);

        // Look for a "<postfix>"
        if (!FindAndSkipRspEnd(rszPointer, m_szNewLine, pDummy))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseCallWaitingInfo() : Incomplete notification\r\n");
            goto Error;
        }

        if (!ExtractQuotedString(rszPointer, number, MAX_BUFFER_SIZE, rszPointer))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseCallWaitingInfo :"
                    " Error Could not extract <number>\r\n");
            goto Error;
        }

        //  Extract <type>
        if (!SkipString(rszPointer, ",", rszPointer) ||
                !ExtractUInt32(rszPointer, uiType, rszPointer))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseCallWaitingInfo() : Could not extract <type>\r\n");
            goto Error;
        }

        //  Extract ,<class>
        if (!SkipString(rszPointer, ",", rszPointer) ||
                !ExtractUInt32(rszPointer, uiClass, rszPointer))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseCallWaitingInfo() :"
                    " Could not extract <class>\r\n");
            goto Error;
        }

        switch (uiClass)
        {
            case 1: // Voice
                pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
                break;
            default:
                pResponse->SetIgnoreFlag(TRUE);
                triggerHangup(m_uiCallId);
                break;
        }

        fRet = TRUE;
    }

Error:
    delete[] pszTempBuffer;
    pszTempBuffer = NULL;
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCallWaitingInfo() - Exit\r\n");
    return fRet;
}

//
//
//
BOOL CSilo_Voice::ParseUnsolicitedSSInfo(CResponse* const pResponse, const char*& szPointer)
{
// Parsing for the +CSSU notification. The format is:
//    "<code2>[,<index>[,<address>,<type>[,<subaddress>,<satype>]]]"

    RIL_LOG_VERBOSE("CSilo_Voice::ParseUnsolicitedSSInfo() - Enter\r\n");

    UINT32 nValue;
    const char* szPostfix;
    const char* szDummy;
    BOOL fRet = FALSE;
    RIL_SuppSvcNotification* pSuppSvcBlob = NULL;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseUnsolicitedSSInfo() : pResponse was NULL\r\n");
        goto Error;
    }

    if (!FindAndSkipRspEnd(szPointer, m_szNewLine, szPostfix))
    {
        // This isn't a complete Supplementary services notification -- no need to parse it
        RIL_LOG_CRITICAL("CSilo_Voice::ParseUnsolicitedSSInfo: Failed to find rsp end!\r\n");
        goto Error;
    }

    // We need to alloc the memory for the string as well to ensure the data gets passed along
    // to the socket
    pSuppSvcBlob = (RIL_SuppSvcNotification*)malloc(
            sizeof(RIL_SuppSvcNotification) + (sizeof(char) * MAX_BUFFER_SIZE));
    if (NULL == pSuppSvcBlob)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseUnsolicitedSSInfo: Failed to alloc pSuppSvcBlob!\r\n");
        goto Error;
    }

    memset(pSuppSvcBlob, 0, sizeof(RIL_SuppSvcNotification) + (sizeof(char) * MAX_BUFFER_SIZE));

    // This is an MT Notification
    pSuppSvcBlob->notificationType = 1;

    // Extract <code2>
    if (ExtractUInt32(szPointer, nValue, szPointer) && (5 != nValue))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseUnsolicitedSSInfo : Found code: %d\r\n", nValue);

        pSuppSvcBlob->code = nValue;

        // Extract [,<index>
        if (!SkipString(szPointer, ",", szPointer))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseUnsolicitedSSInfo : Goto continue\r\n");
            goto Continue;
        }
        if (ExtractUInt32(szPointer, nValue, szPointer))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseUnsolicitedSSInfo : Found index: %d\r\n", nValue);
            pSuppSvcBlob->index = nValue;
        }

        // Extract [,<number>
        if (!SkipString(szPointer, ",", szPointer))
        {
            goto Continue;
        }

        // Setup the char* number to use the extra memory at the end of the struct
        pSuppSvcBlob->number = ((char*)pSuppSvcBlob + sizeof(RIL_SuppSvcNotification));

        if(!ExtractQuotedString(szPointer, pSuppSvcBlob->number, MAX_BUFFER_SIZE, szPointer))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseUnsolicitedSSInfo : Error during extraction of"
                    " number\r\n");
            goto Error;
        }

        // Skip "," and look for <type>
        if (SkipString(szPointer, ",", szPointer))
        {
            if(!ExtractUInt32(szPointer, nValue, szPointer))
            {
                RIL_LOG_CRITICAL("CSilo_Voice::ParseUnsolicitedSSInfo : Error during extraction of"
                        " type\r\n");
                goto Error;
            }

            pSuppSvcBlob->type = nValue;
        }

        // Android doesn't support subaddr or satype fields so just skip to the end

Continue:
        pResponse->SetUnsolicitedFlag(TRUE);
        pResponse->SetResultCode(RIL_UNSOL_SUPP_SVC_NOTIFICATION);

        if (!pResponse->SetData((void*)pSuppSvcBlob, sizeof(RIL_SuppSvcNotification), FALSE))
        {
            free(pSuppSvcBlob);
            pSuppSvcBlob = NULL;
            goto Error;
        }
        else
        {
            fRet = TRUE;
        }
    }
    else if (5 == nValue)
    {
        RIL_LOG_INFO("CSilo_Voice::ParseUnsolicitedSSInfo : Found nValue 5, reporting call state"
                " changed\r\n");
        pResponse->SetUnsolicitedFlag(TRUE);
        pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
        free(pSuppSvcBlob);
        pSuppSvcBlob = NULL;
        fRet = TRUE;
    }

    szPointer = szPostfix - strlen(m_szNewLine);

Error:
    if (!fRet)
    {
        free(pSuppSvcBlob);
        pSuppSvcBlob = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_Voice::ParseUnsolicitedSSInfo() - Exit\r\n");
    return fRet;
}

//
//
//
BOOL CSilo_Voice::ParseIntermediateSSInfo(CResponse* const pResponse, const char*& szPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseIntermediateSSInfo() - Enter\r\n");

    UINT32 nValue;
    const char* szPostfix;
    const char* szDummy;
    BOOL fRet = FALSE;
    RIL_SuppSvcNotification* prssn = NULL;


    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseIntermediateSSInfo() : pResponse was NULL\r\n");
        goto Error;
    }

    prssn = (RIL_SuppSvcNotification*)malloc(sizeof(RIL_SuppSvcNotification));
    if (NULL == prssn)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseIntermediateSSInfo() : cannot allocate prssn=[%d]"
                " bytes\r\n", sizeof(RIL_SuppSvcNotification));
        goto Error;
    }
    memset(prssn, 0, sizeof(RIL_SuppSvcNotification));

    if (!FindAndSkipRspEnd(szPointer, m_szNewLine, szPostfix))
    {
        // This isn't a complete Supplementary services notification -- no need to parse it
        RIL_LOG_CRITICAL("CSilo_Voice::ParseIntermediateSSInfo() : Could not find response"
                " end\r\n");
        goto Error;
    }

    // This is an MO Notification
    prssn->notificationType = 0;

    // Extract <code1>
    if (!ExtractUInt32(szPointer, nValue, szPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseIntermediateSSInfo() : Can't extract <code1>\r\n");
        goto Error;
    }

    prssn->code = nValue;

    if (4 == nValue)
    {
        // Extract ,<index>
        if (SkipString(szPointer, ",", szPointer))
        {
            if (ExtractUInt32(szPointer, nValue, szPointer))
            {
                prssn->index = nValue;
            }
        }
    }

    // We have the parameters, look for the postfix
    if (!SkipRspEnd(szPointer, m_szNewLine, szDummy))
    {
        szPointer = szPostfix - strlen(m_szNewLine);
        pResponse->SetUnrecognizedFlag(TRUE);
        fRet = TRUE;
        free(prssn);
        prssn = NULL;
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    pResponse->SetResultCode(RIL_UNSOL_SUPP_SVC_NOTIFICATION);
    if (!pResponse->SetData((void*)prssn, sizeof(RIL_SuppSvcNotification), FALSE))
    {
        free(prssn);
        prssn = NULL;
        goto Error;
    }
    else
    {
        fRet = TRUE;
    }

Error:
    if (!fRet)
    {
        free(prssn);
        prssn = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_Voice::ParseIntermediateSSInfo() - Exit\r\n");
    return fRet;
}

//
//
BOOL CSilo_Voice::ParseCallMeter(CResponse* const /*pResponse*/, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCallMeter() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCallMeter() - Exit\r\n");

    return FALSE;
}

//
//
BOOL CSilo_Voice::ParseUSSDInfo(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseUSSDInfo() - Enter\r\n");
    P_ND_USSD_STATUS pUssdStatus = NULL;
    UINT32 uiStatus = 0;
    char* pszDataString = NULL;
    UINT32 uiDataStringLen = 0;
    UINT32 uiDCS = 0;
    UINT32 uiAllocSize = 0;
    BOOL fRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseUSSDInfo() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Extract "<status>"
    if (!ExtractUInt32(rszPointer, uiStatus, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseUSSDInfo() : Couldn't extract status\r\n");
        goto Error;
    }

    // Skip ","
    if (!SkipString(rszPointer, ",", rszPointer))
    {
        // No data parameter present
        pUssdStatus = (P_ND_USSD_STATUS) malloc(sizeof(S_ND_USSD_STATUS));
        if (!pUssdStatus)
        {
            goto Error;
        }
        memset(pUssdStatus, 0, sizeof(S_ND_USSD_STATUS));
        snprintf(pUssdStatus->szType, 2, "%u", uiStatus);
        pUssdStatus->sStatusPointers.pszType    = pUssdStatus->szType;
        pUssdStatus->sStatusPointers.pszMessage = NULL;
        uiAllocSize = sizeof(char*);
    }
    else
    {
        // Extract "<data>,<dcs>"
        // NOTE: we take ownership of allocated szDataString
        if (!ExtractQuotedStringWithAllocatedMemory(rszPointer, pszDataString, uiDataStringLen,
                rszPointer))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseUSSDInfo() : Couldn't extract"
                    " ExtractQuotedStringWithAllocatedMemory\r\n");
            goto Error;
        }

        if (!SkipString(rszPointer, ",", rszPointer) ||
                !ExtractUInt32(rszPointer, uiDCS, rszPointer))
        {
            // default uiDCS.  this parameter is supposed to be mandatory but we've
            // seen it missing from otherwise normal USSD responses.
            uiDCS = 0;
        }

        // Reduce -1 to get the actual length
        uiDataStringLen--;

        //  Allocate blob.
        pUssdStatus = (P_ND_USSD_STATUS) malloc(sizeof(S_ND_USSD_STATUS));
        if (!pUssdStatus)
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseUSSDInfo() - malloc failed\r\n");
            goto Error;
        }
        memset(pUssdStatus, 0, sizeof(S_ND_USSD_STATUS));
        pUssdStatus->szMessage[0] = '\0';

        snprintf(pUssdStatus->szType, 2, "%u", uiStatus);

        // For more information on DCS bits, refer 3GPP TS 23.038 section 5
        int codingGroupBits = uiDCS >> 4;
        if (NULL != pszDataString && uiDataStringLen > 0 &&
                codingGroupBits != 0x03 && codingGroupBits != 0x08 &&
                (codingGroupBits < 0x0A || codingGroupBits > 0x0E))
        {
            /*
             * Note: Character set is set to UCS2. When changing the TE character set
             * to anything other than UCS2, conversion functions needs to be added.
             */
            char* pUCS2String = pszDataString;
            UINT32 uiUCS2StrLength = uiDataStringLen;
            const UINT32 LANGUAGE_CODE_LENGTH = 4;
            char* pUtf8Buffer = NULL;

            if (17 == uiDCS) // UCS2; message preceded by language indication
            {
                if (uiUCS2StrLength > LANGUAGE_CODE_LENGTH)
                {
                    /*
                     * Skip the two GSM 7-bit default alphabet character
                     * representing the language code. Since the source UCS2 buffer
                     * is a byte array containing values in semi-octets, first 4 values in
                     * the source buffer needs to be skipped.
                     */
                    pUCS2String += LANGUAGE_CODE_LENGTH;
                    uiUCS2StrLength -= LANGUAGE_CODE_LENGTH;
                    pUtf8Buffer = ConvertUCS2ToUTF8(pUCS2String,
                                                            uiUCS2StrLength);
                }
            }
            else
            {
                pUtf8Buffer = ConvertUCS2ToUTF8(pUCS2String, uiUCS2StrLength);
            }

            /*
             * CopyString done only when both pUssdStatus->szMessage and
             * pUtf8Buffer are not null.
             */
            CopyStringNullTerminate(pUssdStatus->szMessage, pUtf8Buffer,
                                                            MAX_USSD_MESSAGE_SIZE);
            delete[] pUtf8Buffer;
            pUtf8Buffer = NULL;
        }
        else
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseUSSDInfo() - codingGroupBits: 0x%x"
                    " not handled\r\n", codingGroupBits);
        }

        RIL_LOG_INFO("CSilo_Voice::ParseUSSDInfo() - %s\r\n", pUssdStatus->szMessage);

        pUssdStatus->sStatusPointers.pszType    = pUssdStatus->szType;
        pUssdStatus->sStatusPointers.pszMessage =
                (pUssdStatus->szMessage[0] != '\0') ? pUssdStatus->szMessage : NULL;
        uiAllocSize = 2 * sizeof(char*);
    }

    pResponse->SetResultCode(RIL_UNSOL_ON_USSD);

    if (!pResponse->SetData((void*)pUssdStatus, uiAllocSize, FALSE))
    {
        goto Error;
    }
    fRet = TRUE;

Error:
    if (!fRet)
    {
        free(pUssdStatus);
        pUssdStatus = NULL;
    }

    delete[] pszDataString;
    pszDataString = NULL;

    RIL_LOG_VERBOSE("CSilo_Voice::ParseUSSDInfo() - Exit\r\n");
    return fRet;
}

//
//
BOOL CSilo_Voice::ParseConnLineIdRestriction(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseConnLineIdRestriction() - Enter\r\n");
    BOOL fRet = FALSE;
    const char* szDummy;
    UINT32 uiStatus = 0;
    P_ND_USSD_STATUS pUssdStatus = NULL;
    char* szDataString = NULL;
    UINT32 uiTypeCode = 0; // USSD Notify

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseConnLineIdRestriction() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Extract "<status>"
    if (!ExtractUInt32(rszPointer, uiStatus, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseConnLineIdRestriction() : Couldn't extract status\r\n");
        goto Error;
    }

    if (!uiStatus)
    {
        asprintf(&szDataString, "%s", "Connected Line ID Restriction Service is Disabled");
    }
    else if (uiStatus == 1)
    {
         asprintf(&szDataString, "%s", "Connected Line ID Restriction Service is Enabled");
    }
    else if (uiStatus == 2)
    {
        asprintf(&szDataString, "%s", "Connected Line ID Restriction Service Status is Unknown");
    }

    // allocate memory for message string
    pUssdStatus = (P_ND_USSD_STATUS) malloc(sizeof(S_ND_USSD_STATUS));
    if (!pUssdStatus)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseConnLineIdRestriction() - malloc failed\r\n");
        goto Error;
    }
    memset(pUssdStatus, 0, sizeof(S_ND_USSD_STATUS));
    snprintf(pUssdStatus->szType, 2, "%u", uiTypeCode);
    if (!CopyStringNullTerminate(pUssdStatus->szMessage, szDataString, MAX_USSD_MESSAGE_SIZE))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseConnLineIdRestriction() - Cannot"
                " CopyStringNullTerminate szDataString\r\n");
        goto Error;
    }

    // update status pointers
    pUssdStatus->sStatusPointers.pszType    = pUssdStatus->szType;
    pUssdStatus->sStatusPointers.pszMessage = pUssdStatus->szMessage;

    RIL_LOG_VERBOSE("CSilo_Voice::ParseConnLineIdRestriction() - %s\r\n", pUssdStatus->szMessage);

    pResponse->SetResultCode(RIL_UNSOL_ON_USSD);

    if (!pResponse->SetData((void*)pUssdStatus, 2 * sizeof(char*), FALSE))
    {
        goto Error;
    }

    fRet = TRUE;

Error:
    if (!fRet)
    {
        free(pUssdStatus);
        pUssdStatus = NULL;
    }

    free(szDataString);

    RIL_LOG_VERBOSE("CSilo_Voice::ParseConnLineIdRestriction() - Exit\r\n");
    return fRet;
}

//
//
BOOL CSilo_Voice::ParseCallProgressInformation(CResponse* const /*pResponse*/,
                                                       const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCallProgressInformation() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCallProgressInformation() - Exit\r\n");

    return FALSE;
}

//
//
BOOL CSilo_Voice::ParseIndicatorEvent(CResponse* const /*pResponse*/, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseIndicatorEvent() - Enter\r\n");

    BOOL fRet = FALSE;

    RIL_LOG_VERBOSE("CSilo_Voice::ParseIndicatorEvent() - Exit\r\n");

    return fRet;
}

//
//
BOOL CSilo_Voice::ParseDISCONNECT(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseDISCONNECT() - Enter\r\n");

    BOOL fRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseDISCONNECT() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);

    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseDISCONNECT() - Exit\r\n");
    return fRet;

}

BOOL CSilo_Voice::ParseBusy(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseBusy() - Enter\r\n");

    BOOL fRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseBusy() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseBusy() - Exit\r\n");
    return fRet;
}

BOOL CSilo_Voice::ParseNoAnswer(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseNoAnswer() - Enter\r\n");

    BOOL fRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseNoAnswer() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseNoAnswer() - Exit\r\n");
    return fRet;
}


BOOL CSilo_Voice::ParseCTMCall(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCTMCall() - Enter\r\n");

    BOOL fRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseCTMCall() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCTMCall() - Exit\r\n");
    return fRet;
}

BOOL CSilo_Voice::ParseNoCTMCall(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseNoCTMCall() - Enter\r\n");

    BOOL fRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseNoCTMCall() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseNoCTMCall() - Exit\r\n");
    return fRet;
}

BOOL CSilo_Voice::ParseWaitingCallCTM(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseWaitingCallCTM() - Enter\r\n");

    BOOL fRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseWaitingCallCTM() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseWaitingCallCTM() - Exit\r\n");
    return fRet;
}

//
// Parsing function for
// +XUCCI: <RAT>, <conn_status>, <ciphering_status>, <domain>, <key_status>, <key_domain>
//
BOOL CSilo_Voice::ParseXUCCI(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseXUCCI() - Enter\r\n");

    BOOL fRet = FALSE;
    UINT32 uiCipheringStatus = 0;
    UINT32 uiDomain = 0;
    UINT32 uiKeyStatus = 0;
    UINT32 uiKeyDomain = 0;
    UINT32 uiCurrRat = 0; // 1; GSM, 2: UMTS, 3: LTE
    UINT32 uiConnState = 0;
    sOEM_HOOK_RAW_UNSOL_CIPHERING_IND* pData = NULL;
    UINT32 uiCurrStatus = CTE::GetTE().GetCurrentCipheringStatus();
    UINT32 uiPrevStatus = uiCurrStatus;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : pResponse was NULL\r\n");
        goto end;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Response format is
    // +XUCCI: <RAT>,<Conn_Status>,<Ciphering_Status>,<Domain>,
    // <key_Status>, <Key_Context>

    //  Extract <current rat>
    if (!ExtractUInt32(rszPointer, uiCurrRat, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : Could not extract current rat\r\n");
        goto end;
    }

    //  Extract ,<conn status>
    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractUInt32(rszPointer, uiConnState, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : Could not extract connection state\r\n");
        goto end;
    }

    //  Extract, <ciphering status>
    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractUInt32(rszPointer, uiCipheringStatus, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : Could not extract uiCipheringStatus\r\n");
        goto end;
    }

    //  Extract ,<domain>
    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractUInt32(rszPointer, uiDomain, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : Could not extract uiDomain\r\n");
        goto end;
    }

    //  Extract ,<key status>
    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractUInt32(rszPointer, uiKeyStatus, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : Could not extract uiKeyStatus\r\n");
        goto end;
    }

    //  Extract ,<Key domain>
    if (!SkipString(rszPointer, ",", rszPointer)
            || !ExtractUInt32(rszPointer, uiKeyDomain, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : Could not extract uiKeyDomain\r\n");
        goto end;
    }

    RIL_LOG_INFO("CSilo_Voice::ParseXUCCI() :"
            " Ciphering Status = %u and Domain = %u RAT = %u\r\n",
             uiCipheringStatus, uiDomain, uiCurrRat);

    switch (uiDomain)
    {
    case 0: // CS domain
        uiCurrStatus = (uiCipheringStatus == 0)
                ? uiCurrStatus & ~(1 << 0) : uiCurrStatus | (1 << 0);
        break;
    case 1: // PS domain
        uiCurrStatus = (uiCipheringStatus == 0)
                ? uiCurrStatus & ~(1 << 1) : uiCurrStatus | (1 << 1);
        break;
    case 2: // Both PS and CS domain
       uiCurrStatus = (uiCipheringStatus == 1) ? 3 : 0;
       break;
    default:
        break;
    }

    // send ciphering indications only if there is a change in ciphering
    // status and the change is that ciphering is not active
    if ((uiCurrStatus == 3) != (uiPrevStatus == 3))
    {
        pData = (sOEM_HOOK_RAW_UNSOL_CIPHERING_IND*)malloc(
                sizeof(sOEM_HOOK_RAW_UNSOL_CIPHERING_IND));
        if (pData == NULL)
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : Could not allocate memory\r\n");
            goto end;
        }

        memset(pData, 0, sizeof(sOEM_HOOK_RAW_UNSOL_CIPHERING_IND));
        pData->command = RIL_OEM_HOOK_RAW_UNSOL_CIPHERING_IND;
        pData->cipheringStatus = (uiCurrStatus == 3) ? 1 : 0;

        if (!pResponse->SetData((void*)pData, sizeof(sOEM_HOOK_RAW_UNSOL_CIPHERING_IND), FALSE))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseXUCCI() : Could not set data in reply\r\n");
            goto end;
        }
        pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

        // store the new values if sending of message is successful
        CTE::GetTE().SetCurrentCipheringStatus(uiCurrStatus);
    }

    fRet = TRUE;

end:
    if (!fRet)
    {
         free(pData);
         pData = NULL;
    }
    RIL_LOG_VERBOSE("CSilo_Voice::ParseXUCCI() - Exit\r\n");
    return fRet;
}

BOOL CSilo_Voice::ParseCNAP(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCNAP() - Enter\r\n");

    BOOL bRet = FALSE;
    char szName[MAX_CNAP_NAME_SIZE] = {'\0'};
    UINT32 uiCniValidity = 2;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseCNAP() : pResponse was NULL\r\n");
        goto Exit;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Extract <name>
    ExtractQuotedString(rszPointer, szName, MAX_CNAP_NAME_SIZE, rszPointer);

    // Extract <CNI validity>, optional
    if (FindAndSkipString(rszPointer, ",", rszPointer))
    {
        if (!ExtractUInt32(rszPointer, uiCniValidity, rszPointer))
        {
            RIL_LOG_CRITICAL("CSilo_Voice::ParseCNAP() : Could not extract uiCniValidity\r\n");
            goto Exit;
        }
    }

    // <CNI validity>:
    //    0 CNI valid
    //    1 CNI has been withheld by the originator
    //    2 CNI is not available due to interworking problems or limitations of originating
    //      network
    CTE::GetTE().SetCnapCniValidity((uiCniValidity > 2) ? 2 : uiCniValidity);

    CTE::GetTE().SetCnapName(szName);

    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);

    bRet = TRUE;

Exit:

    RIL_LOG_VERBOSE("CSilo_Voice::ParseCNAP() - Exit\r\n");
    return bRet;

}

BOOL CSilo_Voice::ParseCLIP(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCLIP() - Enter\r\n");

    BOOL bRet = FALSE;
    char szDummy[MAX_BUFFER_SIZE] = {'\0'};
    UINT32 uiDummy = 0;
    char szNumber[MAX_BUFFER_SIZE] = {'\0'};
    UINT32 uiCliValidity = 2;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseCLIP() : pResponse was NULL\r\n");
        goto Error;
    }

    // URC format:
    //    +CLIP: <number>,<type>[,<subaddr>,<satype>[,[<alpha>][,<CLI_validity>]]]
    // Command reply format:
    //    +CLIP: <n>,<m>

    // Extract <number>,If not a string, consider this is a normal reply and not an URC.
    if (!ExtractQuotedString(rszPointer, szNumber, MAX_BUFFER_SIZE, rszPointer))
    {
        RIL_LOG_INFO("CSilo_Voice::ParseCLIP() : Could not extract number,"
                "consider it a normal reply instead of an URC.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Extract <type>
    // parameter not used in current implementation
    if (!FindAndSkipString(rszPointer, ",", rszPointer)
            || !ExtractUInt32(rszPointer, uiDummy, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_Voice::ParseCLIP() : Could not extract type\r\n");
        goto Exit;
    }

    bRet = TRUE;

    if (!FindAndSkipString(rszPointer, ",", rszPointer))
    {
        goto Exit;
    }

    // Extract optional <subaddr>
    // parameter not used in current implementation
    if (ExtractQuotedString(rszPointer, szDummy, MAX_BUFFER_SIZE, rszPointer))
    {

        // Extract <satype>
        // parameter not used in current implementation
        if (!FindAndSkipString(rszPointer, ",", rszPointer)
                || !ExtractUInt32(rszPointer, uiDummy, rszPointer))
        {
            RIL_LOG_INFO("CSilo_Voice::ParseCLIP() : Could not extract satype\r\n");
            goto Exit;
        }
    }
    else if (!FindAndSkipString(rszPointer, ",", rszPointer))
    {
        goto Exit;
    }

    // consume comma following <satype>
    if (!FindAndSkipString(rszPointer, ",", rszPointer))
    {
        goto Exit;
    }

    // Extract optional alpha>
    // parameter not used in current implementation
    ExtractQuotedString(rszPointer, szDummy, MAX_BUFFER_SIZE, rszPointer);

    if (!FindAndSkipString(rszPointer, ",", rszPointer))
    {
        goto Exit;
    }

    // Extract optional <CLI validity>
    if (!ExtractUInt32(rszPointer, uiCliValidity, rszPointer))
    {
        uiCliValidity = 2;
        RIL_LOG_INFO("CSilo_Voice::ParseCLIP() : Could not extract CLI validity\r\n");
        goto Exit;
    }

Exit:
    if (strcmp(szNumber, CTE::GetTE().GetNumber()) != 0
            || CTE::GetTE().GetNumberCliValidity() != static_cast<int>(uiCliValidity))
    {
        CTE::GetTE().SetNumberCliValidity(uiCliValidity);
        CTE::GetTE().SetNumber(szNumber);
        pResponse->SetResultCode(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED);
    }

Error:
    RIL_LOG_VERBOSE("CSilo_Voice::ParseCLIP() - Exit\r\n");
    return bRet;

}
