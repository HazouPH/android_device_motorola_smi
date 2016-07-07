////////////////////////////////////////////////////////////////////////////
// silo_Phonebook.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides response handlers and parsing functions for the phonebook-related
//    RIL components.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "channel_nd.h"
#include "silo_phonebook.h"
#include "response.h"
#include "te.h"

//
//
CSilo_Phonebook::CSilo_Phonebook(CChannel* pChannel)
: CSilo(pChannel)
{
    RIL_LOG_VERBOSE("CSilo_Phonebook::CSilo_Phonebook() - Enter\r\n");

    // AT Response Table
    static ATRSPTABLE pATRspTable[] =
    {
        { "+PBREADY", (PFN_ATRSP_PARSE)&CSilo_Phonebook::ParsePBREADY },
        { ""        , (PFN_ATRSP_PARSE)&CSilo_Phonebook::ParseNULL }
    };

    m_pATRspTable = pATRspTable;
    RIL_LOG_VERBOSE("CSilo_Phonebook::CSilo_Phonebook() - Exit\r\n");
}

//
//
CSilo_Phonebook::~CSilo_Phonebook()
{
    RIL_LOG_VERBOSE("CSilo_Phonebook::~CSilo_Phonebook() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_Phonebook::~CSilo_Phonebook() - Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//  Parse functions here
///////////////////////////////////////////////////////////////////////////////////////////////

BOOL CSilo_Phonebook::ParsePBREADY(CResponse* const pResponse, const char*& /*rszPointer*/)
{
    RIL_LOG_VERBOSE("CSilo_Phonebook::ParsePBREADY() - Enter\r\n");
    BOOL bRet = FALSE;

    if (pResponse == NULL)
    {
        RIL_LOG_CRITICAL("CSilo_Phonebook::ParsePBRREADY() : pResponse was NULL\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);
    CTE::GetTE().TriggerQuerySimSmsStoreStatus();
    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CSilo_Phonebook::ParsePBREADY() - Exit\r\n");
    return bRet;
}
