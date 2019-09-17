////////////////////////////////////////////////////////////////////////////
// silo_SMS.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides response handlers and parsing functions for the SMS-related
//    RIL components.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "channel_nd.h"
#include "response.h"
#include "extract.h"
#include "rildmain.h"
#include "callbacks.h"
#include "silo_sms.h"
#include "te.h"
#include <arpa/inet.h>

//
//
CSilo_SMS::CSilo_SMS(CChannel* pChannel)
: CSilo(pChannel)
{
    RIL_LOG_VERBOSE("CSilo_SMS::CSilo_SMS() - Enter\r\n");

    // AT Response Table
    static ATRSPTABLE pATRspTable[] =
    {
        { "+CMT: "   , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseCMT  },
        { "+CBM: "   , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseCBM  },
        { "+CDS: "   , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseCDS  },
        { "+CMTI: "  , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseCMTI },
        { "+CBMI: "  , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseCBMI },
        { "+CDSI: "  , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseCDSI },
        { "+XETWPRIWARN: "  , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseXETWPRIWARN },
        { "+XETWSECWARN: "  , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseXETWSECWARN },
        { ""         , (PFN_ATRSP_PARSE)&CSilo_SMS::ParseNULL }
    };

    m_pATRspTable = pATRspTable;
    RIL_LOG_VERBOSE("CSilo_SMS::CSilo_SMS() - Exit\r\n");
}

//
//
CSilo_SMS::~CSilo_SMS()
{
    RIL_LOG_VERBOSE("CSilo_SMS::~CSilo_SMS() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_SMS::~CSilo_SMS() - Exit\r\n");
}


char* CSilo_SMS::GetBasicInitString()
{
    // sms silo-related channel basic init string
    const char szSmsBasicInitString[] = "+CMGF=0";

    if (CTE::GetTE().IsSmsCapable())
    {
        if (!ConcatenateStringNullTerminate(m_szBasicInitString,
                sizeof(m_szBasicInitString), szSmsBasicInitString))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::GetBasicInitString() : Failed to copy basic init "
                    "string!\r\n");
            return NULL;
        }
    }
    return m_szBasicInitString;
}

char* CSilo_SMS::GetUnlockInitString()
{
    // sms silo-related channel unlock init string
    const char szSmsUnlockInitString[] = "+CSMS=1|+CGSMS=3";

    if (CTE::GetTE().IsSmsCapable())
    {
        if (!ConcatenateStringNullTerminate(m_szUnlockInitString,
                sizeof(m_szUnlockInitString), szSmsUnlockInitString))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::GetUnlockInitString() : Failed to copy unlock init "
                    "string!\r\n");
            return NULL;
        }
    }
    return m_szUnlockInitString;
}

char* CSilo_SMS::GetURCInitString()
{
    // sms silo-related URC channel basic init string
    const char szSmsURCInitString[] = "+CMGF=0|+CSCS=\"UCS2\"";
    if (CTE::GetTE().IsSmsCapable())
    {
        if (!ConcatenateStringNullTerminate(m_szURCInitString,
                sizeof(m_szURCInitString), szSmsURCInitString))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::GetURCInitString() : Failed to copy URC init "
                    "string!\r\n");
            return NULL;
        }
    }
    return m_szURCInitString;
}

char* CSilo_SMS::GetURCUnlockInitString()
{
    // sms silo-related URC channel unlock init string
    const char szSmsURCUnlockInitString[] = "+CNMI=2,2,2,1";
    if (CTE::GetTE().IsSmsCapable())
    {
        if (!ConcatenateStringNullTerminate(m_szURCUnlockInitString,
                sizeof(m_szURCUnlockInitString), szSmsURCUnlockInitString))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::GetURCUnlockInitString() : Failed to copy URC unlock "
                    "init string!\r\n");
            return NULL;
        }
    }
    return m_szURCUnlockInitString;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//  Parse functions here
///////////////////////////////////////////////////////////////////////////////////////////////

//
//
BOOL CSilo_SMS::ParseMessage(CResponse* const /*pResponse*/, const char*& /*rszPointer*/,
                                                      SILO_SMS_MSG_TYPES /*msgType*/)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseMessage() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_SMS::ParseMessage() - Exit\r\n");
    return FALSE;
}

//
//
//
BOOL CSilo_SMS::ParseMessageInSim(CResponse* const pResponse, const char*& rszPointer,
                                                           SILO_SMS_MSG_TYPES msgType)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseMessageInSim() - Enter\r\n");

    BOOL   fRet = FALSE;
    const char* szDummy;
    UINT32 Location;
    UINT32 Index;
    int* pIndex = NULL;

    if (SILO_SMS_MSG_IN_SIM == msgType)
    {
        CTE::GetTE().TriggerQuerySimSmsStoreStatus();
    }

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseMessageInSim() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    pIndex = (int*)malloc(sizeof(int));
    if (NULL == pIndex)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseMessageInSim() - Could not alloc mem for int.\r\n");
        goto Error;
    }

    // Look for a "<postfix>" to be sure we got a whole message
    if (!FindAndSkipRspEnd(rszPointer, m_szNewLine, szDummy))
    {
        goto Error;
    }

    // Look for a "<anything>,<Index>"
    if ( (!FindAndSkipString(rszPointer, ",", rszPointer))     ||
         (!ExtractUInt32(rszPointer, Index, rszPointer)))
    {
        goto Error;
    }

    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM);

    *pIndex = Index;

    if (!pResponse->SetData((void*)pIndex, sizeof(int), FALSE))
    {
        goto Error;
    }

    fRet = TRUE;

Error:
    if (!fRet)
    {
        free(pIndex);
        pIndex = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseMessageInSim() - Exit\r\n");

    return fRet;
}

//
// SMS-DELIVER notification
//
BOOL CSilo_SMS::ParseCMT(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCMT() - Enter\r\n");

    BOOL   fRet     = FALSE;
    UINT32 uiLength = 0;
    char*  szPDU    = NULL;
    char   szAlpha[MAX_BUFFER_SIZE];
    const char* szDummy;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCMT() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Throw out the alpha chars if there are any
    (void)ExtractQuotedString(rszPointer, szAlpha, MAX_BUFFER_SIZE, rszPointer);

    // Parse ",<length><CR><LF>
    if (!SkipString(rszPointer, ",", rszPointer)       ||
        !ExtractUInt32(rszPointer, uiLength, rszPointer) ||
        !SkipString(rszPointer, m_szNewLine, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCMT() - Could not parse PDU Length.\r\n");
        goto Error;
    }

    //RIL_LOG_INFO("CMT=[%s]\r\n", CRLFExpandedString(rszPointer,strlen(rszPointer)).GetString() );

    // The PDU will be followed by m_szNewLine, so look for m_szNewLine
    // and use its position to determine the length of the PDU string.

    if (!FindAndSkipString(rszPointer, m_szNewLine, szDummy))
    {
        // This isn't a complete message notification -- no need to parse it
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCMT() - Could not find postfix; assuming this is an"
                " incomplete response.\r\n");
        goto Error;
    }
    else
    {
        // Override the given length with the actual length. Don't forget the '\0'.
        uiLength = ((UINT32)(szDummy - rszPointer)) - strlen(m_szNewLine) + 1;
        RIL_LOG_INFO("CSilo_SMS::ParseCMT() - Calculated PDU String length: %u chars.\r\n",
                uiLength);
    }

    szPDU = (char*)malloc(sizeof(char) * uiLength);
    if (NULL == szPDU)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCMT() - Could not allocate memory for szPDU.\r\n");
        goto Error;
    }
    memset(szPDU, 0, sizeof(char) * uiLength);

    if (!ExtractUnquotedString(rszPointer, m_cTerminator, szPDU, uiLength, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCMT() - Could not parse PDU String.\r\n");
        goto Error;
    }

    // Ensure NULL Termination.
    szPDU[uiLength-1] = '\0';

    RIL_LOG_INFO("CSilo_SMS::ParseCMT() - PDU String: \"%s\".\r\n", szPDU);

    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_NEW_SMS);

    if (!pResponse->SetData((void*)szPDU, sizeof(char) * (uiLength-1), FALSE))
    {
        goto Error;
    }

    fRet = TRUE;

Error:
    if (!fRet)
    {
        free(szPDU);
        szPDU = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseCMT() - Exit\r\n");
    return fRet;
}

//
//  Incoming cell broadcast.
//
BOOL CSilo_SMS::ParseCBM(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCBM() - Enter\r\n");

    BOOL   fRet     = FALSE;
    UINT32   uiLength = 0;
    char*  szPDU    = NULL;
    BYTE*  pByteBuffer = NULL;
    char   szAlpha[MAX_BUFFER_SIZE];
    const char* szDummy;
    UINT32 bytesUsed = 0;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCBM() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Throw out the alpha chars if there are any
    (void)ExtractQuotedString(rszPointer, szAlpha, MAX_BUFFER_SIZE, rszPointer);

    // Parse "<length><CR><LF>
    if (!ExtractUInt32(rszPointer, uiLength, rszPointer) ||
        !SkipString(rszPointer, m_szNewLine, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCBM() - Could not parse PDU Length.\r\n");
        goto Error;
    }

    // The PDU will be followed by m_szNewLine, so look for m_szNewLine
    // and use its position to determine the length of the PDU string.

    if (!FindAndSkipString(rszPointer, m_szNewLine, szDummy))
    {
        // This isn't a complete message notification -- no need to parse it
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCBM() - Could not find postfix; assuming this is an"
                " incomplete response.\r\n");
        goto Error;
    }
    else
    {
        uiLength = (UINT32)(szDummy - rszPointer) - strlen(m_szNewLine);
        RIL_LOG_INFO("CSilo_SMS::ParseCBM() - Calculated PDU String length: %u chars.\r\n",
                uiLength);
    }

    // Don't forget the '\0'.
    szPDU = (char*) malloc(sizeof(char) * (uiLength + 1));
    pByteBuffer = (BYTE*) malloc(sizeof(BYTE) * (uiLength / 2) + 1);

    if ((NULL == szPDU) || (NULL == pByteBuffer))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCBM() - Could not allocate memory for szPDU or"
                " pByteBuffer\r\n");
        goto Error;
    }

    memset(szPDU, 0, sizeof(char) * (uiLength + 1));
    memset(pByteBuffer, 0, sizeof(BYTE) * (uiLength / 2) + 1);

    if (!ExtractUnquotedString(rszPointer, m_cTerminator, szPDU, (uiLength + 1), rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCBM() - Could not parse PDU String.\r\n");
        goto Error;
    }

    if (!GSMHexToGSM(szPDU, uiLength, pByteBuffer, uiLength / 2, bytesUsed))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCBM() - GSMHexToGSM conversion failed\r\n");
        goto Error;
    }

    pByteBuffer[bytesUsed] = '\0';

    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS);

    if (!pResponse->SetData(pByteBuffer, bytesUsed, FALSE))
    {
        goto Error;
    }

    fRet = TRUE;

Error:
    free(szPDU);
    szPDU = NULL;

    if (!fRet)
    {
        free(pByteBuffer);
        pByteBuffer = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseCBM() - Exit\r\n");
    return fRet;
}


// Length of a Etws Primary notification's PDU.
const int CSilo_SMS::ETWS_PRIMARY_NOTIFICATION_PDU_SIZE = 56;

//
// Parsing ETWS Primary warning cell broadcast message.
// PDU is constructed from URC parameters.
//
BOOL CSilo_SMS::ParseXETWPRIWARN(CResponse* const pResponse, const char*& respPointer)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWPRIWARN() - Enter\r\n");

    UINT32 uiSerialNumber = 0;
    UINT32 uiMid = 0;
    UINT32 uiWarningType = 0;
    UINT32 uiSecurityStatus = 0;
    unsigned char* pszPDU = NULL;
    BOOL fRet     = FALSE;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWPRIWARN() - pResponse is NULL.\r\n");
        goto Error;
    }

    /**
     * +XETWPRIWARN : <serial_number>, <mid>, <warning_type>, <security_status>
     * The two octets of the <Serial Number> field are divided into a 2-bit Geographical
     * Scope (GS) indicator, a 10-bit Message Code and a 4-bit Update Number.
     * This parameter is defined in 3GPP TS 23.041.
     * <mid> : The message identifier parameter identifies the source and type of the CBS message.
     * The Message Identifier is coded in binary. This parameter is defined in 3GPP TS 23.041.
     * <warning_type> : Value [0-6]
     * <security_status> : Value [0-3]
     *
     */

    pResponse->SetUnsolicitedFlag(TRUE);

    // Parse <serial_number>
    if (!ExtractUInt32(respPointer, uiSerialNumber, respPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWPRIWARN() - Could not parse <serial_number>.\r\n");
        goto Error;
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWPRIWARN() - uiSerialNumber:%u\r\n", uiSerialNumber);

    // Parse ,<mid>
    if (!FindAndSkipString(respPointer, ",", respPointer) ||
            !ExtractUInt32(respPointer, uiMid, respPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SIM::ParseXETWPRIWARN() - Could not parse <mid>.\r\n");
        goto Error;
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWPRIWARN() - uiMid:%u\r\n", uiMid);

    // Parse ,<warning_type>
    if (!FindAndSkipString(respPointer, ",", respPointer) ||
            !ExtractUInt32(respPointer, uiWarningType, respPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SIM::ParseXETWPRIWARN() - Could not parse <Warning_Type>.\r\n");
        goto Error;
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWPRIWARN() - uiWarningType:%u\r\n", uiWarningType);

    // Parse ,<security_status>
    if (!FindAndSkipString(respPointer, ",", respPointer) ||
            !ExtractUInt32(respPointer, uiSecurityStatus, respPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SIM::ParseXETWPRIWARN() - Could not parse <Security_Status>.\r\n");
        goto Error;
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWPRIWARN() - uiSecurityStatus:%u\r\n", uiSecurityStatus);

    // Don't forget the '\0'.
    pszPDU = (unsigned char*) malloc(sizeof(unsigned char) *
            (ETWS_PRIMARY_NOTIFICATION_PDU_SIZE + 1));

    if (pszPDU == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWPRIWARN() - "
                "Could not allocate memory for szPDU.\r\n");
        goto Error;
    }

    // 2 BYTES - Serial Number
    pszPDU[0] = (unsigned char) ((uiSerialNumber >> (1*8)) & 0xFF);
    pszPDU[1] = (unsigned char) (uiSerialNumber & 0xFF);

    // 2 BYTES - Msg Id
    pszPDU[2] = (unsigned char) ((uiMid >> (1*8)) & 0xFF);
    pszPDU[3] = (unsigned char) (uiMid & 0xFF);

    // 2 BYTES - Warning type
    pszPDU[4] = (unsigned char) ((uiWarningType >> (1*8)) & 0xFF);
    pszPDU[5] = (unsigned char) (uiWarningType & 0xFF);

    // Padding to ETWS_PRIMARY_NOTIFICATION_PDU_SIZE BYTES
    memset(pszPDU + 6, '\r', ETWS_PRIMARY_NOTIFICATION_PDU_SIZE - 6 + 1);

    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS);

    if (!pResponse->SetData((void*) pszPDU, sizeof(unsigned char) *
            ETWS_PRIMARY_NOTIFICATION_PDU_SIZE, FALSE))
    {
        goto Error;
    }

    fRet = TRUE;
Error:
    if (!fRet)
    {
        free(pszPDU);
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWPRIWARN() - Exit - Err:%d\r\n", fRet);
    return fRet;
}

//
// Parsing ETWS Secondary warning cell broadcast message.
// Normally URC is in PDU format, if not, PDU is reconstructed from parameters.
//
BOOL CSilo_SMS::ParseXETWSECWARN(CResponse* const pResponse, const char*& respPointer)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - Enter\r\n");

    UINT32 uiSerialNumber = 0;
    UINT32 uiMid = 0;
    UINT32 uiCodingScheme = 0;
    UINT32 uiSecurityStatus = 0;
    UINT32 uiCurrentPage = 0;
    UINT32 uiPage = 0;
    size_t uiLength = 0;
    BOOL fRet = FALSE;
    BYTE*  pByteBuffer = NULL;
    const char* pszDummy;
    UINT32 uiBytesUsed = 0;
    size_t uiCbDataLength = 0;
    const size_t uiHeaderSecondaryLength = 6;
    unsigned char* pszPDU = NULL;
    char* pszCbData = NULL;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() - pResponse is NULL.\r\n");
        goto Error;
    }

    /**
     * +XETWSECWARN : <serial_number>, <mid>, <coding_scheme>, <current_page>, <no_pages> <cb_data>
     * +XETWSECWARN: <length><CR><LF><pdu> : repeated <no_pages>
     */

    pResponse->SetUnsolicitedFlag(TRUE);

    /* Check first the message format */
    if (FindAndSkipString(respPointer, ",", pszDummy))
    {
        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - Found format PARAMETERS\r\n");
        // Parse <serial_number>
        if (!ExtractUInt32(respPointer, uiSerialNumber, respPointer))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() - "
                    "Could not parse <serial_number>.\r\n");
            goto Error;
        }

        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - "
                "uiSerialNumber:%u\r\n", uiSerialNumber);

        // Parse ,<mid>
        if (!FindAndSkipString(respPointer, ",", respPointer) ||
                !ExtractUInt32(respPointer, uiMid, respPointer))
        {
            RIL_LOG_CRITICAL("CSilo_SIM::ParseXETWSECWARN() - Could not parse <mid>.\r\n");
            goto Error;
        }

        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - uiMid:%u\r\n", uiMid);

        // Parse ,<warning_type>
        if (!FindAndSkipString(respPointer, ",", respPointer) ||
                !ExtractUInt32(respPointer, uiCodingScheme, respPointer))
        {
            RIL_LOG_CRITICAL("CSilo_SIM::ParseXETWSECWARN() - "
                    "Could not parse <coding_scheme>.\r\n");
            goto Error;
        }

        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - uiCodingScheme:%u\r\n", uiCodingScheme);

        // Parse ,<current_page>
        if (!FindAndSkipString(respPointer, ",", respPointer) ||
                !ExtractUInt32(respPointer, uiCurrentPage, respPointer))
        {
            RIL_LOG_CRITICAL("CSilo_SIM::ParseXETWSECWARN() - Could not parse <current_page>.\r\n");
            goto Error;
        }

        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - uiCurrentPage:%u\r\n", uiCurrentPage);

        // Parse ,<no_pages>
        if (!FindAndSkipString(respPointer, ",", respPointer) ||
                !ExtractUInt32(respPointer, uiPage, respPointer))
        {
            RIL_LOG_CRITICAL("CSilo_SIM::ParseXETWSECWARN() - Could not parse <current_page>.\r\n");
            goto Error;
        }

        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - uiPage:%u\r\n", uiPage);

        // During tests, noticed that <cb_data> is in between "\r\n" characters
        if (!FindAndSkipString(respPointer, m_szNewLine, respPointer))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() - "
                    "Could not find <cb_data> prefix <cr><ln> "
                    "assuming this is an incomplete response.\r\n");
            goto Error;
        }
        else if (!FindAndSkipString(respPointer, m_szNewLine, pszDummy))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() - "
                    "Could not find <cb_data> postfix <cr><ln> "
                    "assuming this is an incomplete response.\r\n");
            goto Error;
        }
        else
        {
            uiCbDataLength = (pszDummy - respPointer) - strlen(m_szNewLine);
            RIL_LOG_INFO("CSilo_SMS::ParseXETWSECWARN() - "
                    "Calculated PDU String length: %u chars.\r\n",
                    uiCbDataLength);
        }

        // Don't forget the '\0'.
        pszCbData = (char*) malloc(sizeof(char) * (uiCbDataLength + 1));
        if (NULL == pszCbData)
        {
            RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() -"
                    " Could not alloc mem for pszCbData.\r\n");
            goto Error;
        }

        pByteBuffer = (BYTE*) malloc(sizeof(BYTE) * (uiCbDataLength / 2) + 1);
        if (NULL == pByteBuffer)
        {
            RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() -"
                    " Could not alloc mem for pByteBuffer.\r\n");
            goto Error;
        }

        memset(pszCbData, 0, sizeof(char) * (uiCbDataLength + 1));
        memset(pByteBuffer, 0, sizeof(BYTE) * (uiCbDataLength / 2) + 1);

        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - Extracting CB_DATA sz:%u\r\n",
                uiCbDataLength);

        // TO BE DONE = Wrong
        if (!ExtractUnquotedString(respPointer, m_cTerminator,
                pszCbData, (uiCbDataLength + 1), respPointer))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() - "
                    "Could not parse CB_DATA String.\r\n");
            goto Error;
        }

        if (!GSMHexToGSM(pszCbData, uiCbDataLength, pByteBuffer, uiCbDataLength / 2, uiBytesUsed))
        {
            RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() - GSMHexToGSM conversion failed\r\n");
            goto Error;
        }

        pByteBuffer[uiBytesUsed] = '\0';

        // Update PDU length
        uiLength = uiHeaderSecondaryLength + uiBytesUsed;

        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - "
                "Extracted CB_DATA nbBytes:%u, PDU max len:%u\r\n", uiBytesUsed, uiLength);

        // Don't forget the '\0'.
        pszPDU = (unsigned char*) malloc(sizeof(unsigned char) * (uiLength + 1));

        if (pszPDU == NULL)
        {
            RIL_LOG_CRITICAL("CSilo_SMS::ParseXETWSECWARN() - "
                    "Could not allocate memory for szPDU.\r\n");
            goto Error;
        }

        memset(pszPDU, 0, sizeof(unsigned char) * (uiLength + 1));

        // One BYTE - Message Type
        pszPDU[0] = 1;
        // 2 BYTES - Message ID
        pszPDU[1] = (unsigned char) ((uiMid >> (1*8)) & 0xFF);
        pszPDU[2] = (unsigned char) (uiMid & 0xFF);

        // 2 BYTES - Serial Number
        pszPDU[3] = (unsigned char) ((uiSerialNumber >> (1*8)) & 0xFF);
        pszPDU[4] = (unsigned char) (uiSerialNumber & 0xFF);

        // One BYTE - Data Coding Scheme
        pszPDU[5] = (unsigned char) (uiCodingScheme & 0xFF);
        // index 6 - N - CB DATA

        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - Remaining PDU sz:%u, Sz Cb_Data:%u\r\n",
                uiLength-uiHeaderSecondaryLength, uiBytesUsed);

        memcpy(pszPDU + uiHeaderSecondaryLength, pByteBuffer, uiBytesUsed);

        pResponse->SetResultCode(RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS);

        if (!pResponse->SetData((void*) pszPDU, sizeof(unsigned char) * uiLength, FALSE))
        {
            goto Error;
        }
    }
    else
    {
        RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - Found format PDU\r\n");

        if (!ParseCBM(pResponse, respPointer))
        {
            RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - Parsing PDU ERROR\r\n");
            goto Error;
        }
    }

    fRet = TRUE;
Error:
    free(pszCbData);
    free(pByteBuffer);
    if (!fRet)
    {
        free(pszPDU);
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseXETWSECWARN() - Exit - Err:%d\r\n", fRet);
    return fRet;
}

//
//
//
BOOL CSilo_SMS::ParseCDS(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCDS() - Enter\r\n");

    UINT32  uiLength = 0;
    const char* szDummy;
    char* szPDU = NULL;
    char   szAlpha[MAX_BUFFER_SIZE];
    BOOL  fRet = FALSE;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCDS() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Throw out the alpha chars if there are any
    (void)ExtractQuotedString(rszPointer, szAlpha, MAX_BUFFER_SIZE, rszPointer);

    // Parse ",<length><CR><LF>
    if (!ExtractUInt32(rszPointer, uiLength, rszPointer) ||
        !SkipString(rszPointer, m_szNewLine, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCDS() - Could not parse PDU Length.\r\n");
        goto Error;
    }

    //RIL_LOG_INFO("CDS=[%s]\r\n", CRLFExpandedString(rszPointer,strlen(rszPointer)).GetString() );

    // The PDU will be followed by m_szNewLine, so look for m_szNewLine
    // and use its position to determine the length of the PDU string
    if (!FindAndSkipString(rszPointer, m_szNewLine, szDummy))
    {
        // This isn't a complete message notification -- no need to parse it
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCDS() - Could not find postfix; assuming this is an"
                " incomplete response.\r\n");
        goto Error;
    }
    else
    {
        // Override the given length with the actual length. Don't forget the '\0'.
        uiLength = ((UINT32)(szDummy - rszPointer)) - strlen(m_szNewLine) + 1;
        RIL_LOG_INFO("CSilo_SMS::ParseCDS() - Calculated PDU String length: %u chars.\r\n",
                uiLength);
    }

    szPDU = (char*) malloc(sizeof(char) * uiLength);
    if (NULL == szPDU)
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCDS() - Could not allocate memory for szPDU.\r\n");
        goto Error;
    }
    memset(szPDU, 0, sizeof(char) * uiLength);

    if (!ExtractUnquotedString(rszPointer, m_cTerminator, szPDU, uiLength, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_SMS::ParseCDS() - Could not parse PDU String.\r\n");
        goto Error;
    }

    // Ensure NULL Termination.
    szPDU[uiLength-1] = '\0';

    RIL_LOG_INFO("CSilo_SMS::ParseCDS() - PDU String: \"%s\".\r\n", szPDU);

    pResponse->SetResultCode(RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT);

    if (!pResponse->SetData((void*) szPDU, sizeof(char) * uiLength, FALSE))
    {
        goto Error;
    }

    fRet = TRUE;

Error:
    if (!fRet)
    {
        free(szPDU);
        szPDU = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_SMS::ParseCDS() - Exit\r\n");
    return fRet;

}

//
//
//
BOOL CSilo_SMS::ParseCMTI(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCMTI() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCMTI() - Exit\r\n");
    return (ParseMessageInSim(pResponse, rszPointer, SILO_SMS_MSG_IN_SIM));
}

//
//
//
BOOL CSilo_SMS::ParseCBMI(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCBMI() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCBMI() - Exit\r\n");
    return (ParseMessageInSim(pResponse, rszPointer, SILO_SMS_CELLBC_MSG_IN_SIM));
}

//
//
//
BOOL CSilo_SMS::ParseCDSI(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCDSI() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_SMS::ParseCDSI() - Exit\r\n");
    return (ParseMessageInSim(pResponse, rszPointer, SILO_SMS_STATUS_MSG_IN_SIM));
}

