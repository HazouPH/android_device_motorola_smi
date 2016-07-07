////////////////////////////////////////////////////////////////////////////
// silo_Common.cpp
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides response handlers and parsing functions for the URCs which are
//    received in all channels.
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "rillog.h"
#include "channel_nd.h"
#include "silo_common.h"

//
//
CSilo_Common::CSilo_Common(CChannel* pChannel)
: CSilo(pChannel)
{
    RIL_LOG_VERBOSE("CSilo_Common::CSilo_Common() - Enter\r\n");

    // AT Response Table
    static ATRSPTABLE pATRspTable[] =
    {
        { "+PBREADY", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "RING CTM", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "RING", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "CTM CALL", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "NO CTM CALL", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "WAITING CALL CTM", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "NO CARRIER", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "+XBIPI: ", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "+XCSFBI: ", (PFN_ATRSP_PARSE)&CSilo_Common::ParseUnrecognized },
        { "", (PFN_ATRSP_PARSE)&CSilo_Common::ParseNULL }
    };

    m_pATRspTable = pATRspTable;

    RIL_LOG_VERBOSE("CSilo_Common::CSilo_Common() - Exit\r\n");
}

//
//
CSilo_Common::~CSilo_Common()
{
    RIL_LOG_VERBOSE("CSilo_Common::~CSilo_Common() - Enter/Exit\r\n");
}
