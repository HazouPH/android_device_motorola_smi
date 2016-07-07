////////////////////////////////////////////////////////////////////////////
// silo_rfcoexistence.h
//
// Copyright 2012 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CSilo_Rfcoexistence class, which provides response handlers and
//    parsing functions for the Rfcoexistence features.
//
/////////////////////////////////////////////////////////////////////////////
//
//  Rfcoexistence silo class.  This class handles all network functionality including:
//  - XMETRIC
//  - XNRTCWS

#ifndef RRIL_SILO_RFCOEXISTENCE_H
#define RRIL_SILO_RFCOEXISTENCE_H


#include "silo.h"


class CSilo_rfcoexistence : public CSilo
{
public:
    CSilo_rfcoexistence(CChannel* pChannel);
    virtual ~CSilo_rfcoexistence();


protected:


    virtual BOOL ParseXMETRIC(CResponse* const pResponse, const char*& rszPointer);

    virtual BOOL ParseXNRTCWSI(CResponse* const pResponse, const char*& rszPointer);

    // Parse the URCs (+XMETRIC, +XNRTCWS) needed for RF Coexistence
    virtual BOOL ParseCoexURC(CResponse* const pResponse, const char*& rszPointer,
                              const char* pUrcPrefix);

    // New parser for RF Coexistence, for new COEX API
    virtual BOOL ParseCoexReportURC(CResponse* const pResponse, const char*& rszPointer,
                              const char* pUrcPrefix);
};

#endif // RRIL_SILO_RFCOEXISTENCE_H
