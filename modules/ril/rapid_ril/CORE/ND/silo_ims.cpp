////////////////////////////////////////////////////////////////////////////
// silo_IMS.cpp
//
// Copyright 2013 Intel Corporation, All Rights Reserved.
//
//
// Description:
//    Provides unsolicited response handlers for IMS-related RIL components.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "channel_nd.h"
#include "response.h"
#include "extract.h"
#include "rildmain.h"
#include "callbacks.h"
#include "oemhookids.h"
#include "silo_ims.h"
#include "te.h"

//
//
CSilo_IMS::CSilo_IMS(CChannel* pChannel)
: CSilo(pChannel)
{
    RIL_LOG_VERBOSE("CSilo_IMS::CSilo_IMS() - Enter\r\n");

    // AT Response Table
    static ATRSPTABLE pATRspTable[] =
    {
        { "+CIREPI: "        , (PFN_ATRSP_PARSE)&CSilo_IMS::ParseCIREPI  },
        { "+CIREPH: "        , (PFN_ATRSP_PARSE)&CSilo_IMS::ParseCIREPH  },
        { "+CIREGU: "        , (PFN_ATRSP_PARSE)&CSilo_IMS::ParseCIREGU },
        { "+XISRVCCI: "       , (PFN_ATRSP_PARSE)&CSilo_IMS::ParseXISRVCCI },
        { "+XISRVCCSTATI: "  , (PFN_ATRSP_PARSE)&CSilo_IMS::ParseUnrecognized },
        // FIXME: Following strings to be confirmed and parsed.
        { "+IMSCALLSTAT: "   , (PFN_ATRSP_PARSE)&CSilo_IMS::ParseNULL },
        { "+IMSSMSSTAT: "    , (PFN_ATRSP_PARSE)&CSilo_IMS::ParseNULL },
        { ""                 , (PFN_ATRSP_PARSE)&CSilo_IMS::ParseNULL }
    };

    m_pATRspTable = pATRspTable;

    RIL_LOG_VERBOSE("CSilo_IMS::CSilo_IMS() - Exit\r\n");
}

//
//
CSilo_IMS::~CSilo_IMS()
{
    RIL_LOG_VERBOSE("CSilo_IMS::~CSilo_IMS() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_IMS::~CSilo_IMS() - Exit\r\n");
}

char* CSilo_IMS::GetURCInitString()
{
    char szEnableIMS[MAX_BUFFER_SIZE] = {'\0'};

    // IMS Modem Centric
    if (CTE::GetTE().IsIMSCapable())
    {
        PrintStringNullTerminate(szEnableIMS, MAX_BUFFER_SIZE,
                "|+CISRVCC=1|+CIREP=1|+CIREG=1|+XISMSCFG=%d",
                CTE::GetTE().IsSMSOverIPCapable() ? 1 : 0);
    }
    // IMS AP Centric
    else if (CTE::GetTE().IsIMSApCentric())
    {
        PrintStringNullTerminate(szEnableIMS, MAX_BUFFER_SIZE, "+CIREP=1|+XISRVCC=1");
    }

    if (*szEnableIMS != '\0')
    {
        if (!ConcatenateStringNullTerminate(m_szURCInitString,
                sizeof(m_szURCInitString), szEnableIMS))
        {
            RIL_LOG_CRITICAL("CSilo_IMS::GetURCInitString() : Failed to concat XICFG CISRVCC "
                    "CIREP XISMSCFG to URC init string!\r\n");
            return NULL;
        }
    }
    return m_szURCInitString;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//  Parse functions here
///////////////////////////////////////////////////////////////////////////////////////////////

//
// IMS network support reporting
//
BOOL CSilo_IMS::ParseCIREPI(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_IMS::ParseCIREPI() - Enter\r\n");

    BOOL   fRet = FALSE;
    UINT32 uiNwimsvops = 0;
    char   szAlpha[MAX_BUFFER_SIZE];
    int pos = 0;
    sOEM_HOOK_RAW_UNSOL_IMS_SUPPORT_STATUS data;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_IMS::ParseCIREPI() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Throw out the alpha chars if there are any
    (void)ExtractQuotedString(rszPointer, szAlpha, MAX_BUFFER_SIZE, rszPointer);

    // Parse "<nwimsvops>"
    if (!ExtractUInt32(rszPointer, uiNwimsvops, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_IMS::ParseCIREPI() - Could not parse IMS Voice over PS"
                "support indication\r\n");
        goto Error;
    }

    data.command = RIL_OEM_HOOK_RAW_UNSOL_IMS_SUPPORT_STATUS;
    data.status = uiNwimsvops;

    // Framework will trigger IMS registration depending on this information.
    RIL_LOG_INFO("CSilo_IMS::ParseCIREPI() - CIREPI=[%d]\r\n", uiNwimsvops);

    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData((void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_IMS_SUPPORT_STATUS), TRUE))
    {
        goto Error;
    }

    fRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CSilo_IMS::ParseCIREPI() - Exit\r\n");
    return fRet;
}

//
// IMS SRVCC Handover reporting
//
BOOL CSilo_IMS::ParseCIREPH(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_IMS::ParseCIREPH() - Enter\r\n");

    BOOL   fRet     = FALSE;
    UINT32 uiSrvcch = 0;
    char   szAlpha[MAX_BUFFER_SIZE];
    sOEM_HOOK_RAW_UNSOL_IMS_SRVCCH_STATUS data;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_IMS::ParseCIREPH() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Throw out the alpha chars if there are any
    (void)ExtractQuotedString(rszPointer, szAlpha, MAX_BUFFER_SIZE, rszPointer);

    // Parse "<srvcch>"
    if (!ExtractUInt32(rszPointer, uiSrvcch, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_IMS::ParseCIREPH() - Could not parse IMS SRVCC"
                "Handover information\r\n");
        goto Error;
    }

    data.command = RIL_OEM_HOOK_RAW_UNSOL_IMS_SRVCCH_STATUS;
    data.status = uiSrvcch;

    RIL_LOG_INFO("CSilo_IMS::ParseCIREPH() - CIREPH=[%d]\r\n", uiSrvcch);

    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData((void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_IMS_SRVCCH_STATUS), TRUE))
    {
        goto Error;
    }

    fRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CSilo_IMS::ParseCIREPH() - Exit\r\n");
    return fRet;
}

//
// IMS-Register notification
//
BOOL CSilo_IMS::ParseCIREGU(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_IMS::ParseCIREGU() - Enter\r\n");

    BOOL   fRet     = FALSE;
    UINT32 uiRegInfo = 0;
    char   szExtInfo[MAX_BUFFER_SIZE];
    char   szAlpha[MAX_BUFFER_SIZE];
    int pos = 0;
    sOEM_HOOK_RAW_UNSOL_IMS_REG_STATUS data;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_IMS::ParseCIREGU() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Throw out the alpha chars if there are any
    (void)ExtractQuotedString(rszPointer, szAlpha, MAX_BUFFER_SIZE, rszPointer);

    // Parse "<reg_info>"
    if (!ExtractUInt32(rszPointer, uiRegInfo, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_IMS::ParseCIREGU() - Could not parse IMS registration"
                "status indication\r\n");
        goto Error;
    }

    // Parse ",<ext_info>" if any. Debug purpose only.
    if (SkipString(rszPointer, ",", rszPointer)
            && ExtractUnquotedString(rszPointer, m_cTerminator, szExtInfo,
                    MAX_BUFFER_SIZE, rszPointer))
    {
        RIL_LOG_INFO("CSilo_IMS::ParseCIREGU() - IMS capabilities: %s", szExtInfo);
    }

    data.command = RIL_OEM_HOOK_RAW_UNSOL_IMS_REG_STATUS;
    data.status = uiRegInfo;

    CTE::GetTE().SetImsRegistrationStatus(uiRegInfo);

    RIL_LOG_VERBOSE("CSilo_IMS::ParseCIREGU() - CIREGU=[%d]\r\n", uiRegInfo);

    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData((void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_IMS_REG_STATUS), TRUE))
    {
        goto Error;
    }

    /*
     * When the IMS registration status change force the framework to query the data
     * registration state by completing RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED.
     */
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED, NULL, 0);

    fRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CSilo_IMS::ParseCIREGU() - Exit\r\n");
    return fRet;
}

//
// IMS-SRVCC sync notification
//
BOOL CSilo_IMS::ParseXISRVCCI(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_IMS::ParseXISRVCCI() - Enter\r\n");

    BOOL   fRet     = FALSE;
    UINT32 uiSrvccHoStatus = 0;
    char   szAlpha[MAX_BUFFER_SIZE];
    sOEM_HOOK_RAW_UNSOL_IMS_SRVCC_HO_STATUS data;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_IMS::ParseXISRVCCI() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Throw out the alpha chars if there are any
    (void)ExtractQuotedString(rszPointer, szAlpha, MAX_BUFFER_SIZE, rszPointer);

    // Parse "<srvcc_ho_status>"
    if (!ExtractUInt32(rszPointer, uiSrvccHoStatus, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_IMS::ParseXISRVCCI() - Could not parse SRVCC HO"
                "status indication\r\n");
        goto Error;
    }

    data.command = RIL_OEM_HOOK_RAW_UNSOL_IMS_SRVCC_HO_STATUS;
    data.status = uiSrvccHoStatus;

    RIL_LOG_VERBOSE("CSilo_IMS::ParseXISRVCCI() - XISRVCC=[%u]\r\n", uiSrvccHoStatus);

    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData((void*)&data,
            sizeof(sOEM_HOOK_RAW_UNSOL_IMS_SRVCC_HO_STATUS), TRUE))
    {
        goto Error;
    }

    fRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CSilo_IMS::ParseXISRVCCI() - Exit\r\n");
    return fRet;
}
