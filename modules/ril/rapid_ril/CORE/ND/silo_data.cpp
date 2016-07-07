////////////////////////////////////////////////////////////////////////////
// silo_data.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides response handlers and parsing functions for the data-related
//    RIL components.
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "types.h"
#include "rillog.h"
#include "channel_nd.h"
#include "response.h"
#include "extract.h"
#include "silo_data.h"
#include "channel_data.h"
#include "data_util.h"
#include "rildmain.h"
#include "callbacks.h"
#include "te.h"
#include "te_base.h"

//
//
CSilo_Data::CSilo_Data(CChannel* pChannel)
: CSilo(pChannel)
{
    RIL_LOG_VERBOSE("CSilo_Data::CSilo_Data() - Enter\r\n");

    // AT Response Table
    static ATRSPTABLE pATRspTable[] =
    {
        { "NO CARRIER"  , (PFN_ATRSP_PARSE)&CSilo_Data::ParseNoCarrier },
        { "+XCIEV: "      , (PFN_ATRSP_PARSE)&CSilo_Data::ParseUnrecognized },
        { "+XCIEV:"      , (PFN_ATRSP_PARSE)&CSilo_Data::ParseUnrecognized },
        { ""         , (PFN_ATRSP_PARSE)&CSilo_Data::ParseNULL         }
    };

    m_pATRspTable = pATRspTable;

    RIL_LOG_VERBOSE("CSilo_Data::CSilo_Data() - Exit\r\n");
}

//
//
CSilo_Data::~CSilo_Data()
{
    RIL_LOG_VERBOSE("CSilo_Data::~CSilo_Data() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_Data::~CSilo_Data() - Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//  Parse functions here
///////////////////////////////////////////////////////////////////////////////////////////////

//
//
//
BOOL CSilo_Data::ParseNoCarrier(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_INFO("CSilo_Data::ParseNoCarrier() - Enter\r\n");

    BOOL fRet = FALSE;
    CChannel_Data* pChannelData = NULL;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Data::ParseNoCarrier() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Free this channel's context ID.
    pChannelData =
            CChannel_Data::GetChnlFromRilChannelNumber(m_pChannel->GetRilChannel());
    if (NULL != pChannelData)
    {
        RIL_LOG_INFO("CSilo_Data::ParseNoCarrier() : Calling DataConfigDown  chnl=[%u],"
                " cid=[%u]\r\n", m_pChannel->GetRilChannel(), pChannelData->GetContextID());

        //  Release network interface
        if (!CTE::GetTE().DataConfigDown(pChannelData->GetContextID(), TRUE))
        {
            RIL_LOG_CRITICAL("CSilo_Data::ParseNoCarrier() - DataConfigDown FAILED chnl=[%u],"
                    " cid=[%u]\r\n", m_pChannel->GetRilChannel(), pChannelData->GetContextID());
        }
    }
    fRet = TRUE;

Error:
    RIL_LOG_INFO("CSilo_Data::ParseNoCarrier() - Exit\r\n");
    return fRet;
}
