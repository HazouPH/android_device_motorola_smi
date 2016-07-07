////////////////////////////////////////////////////////////////////////////
// silo_misc.h
//
// Copyright 2012 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CSilo_MISC class, which provides response handlers and
//    parsing functions for the MISC features.
//
/////////////////////////////////////////////////////////////////////////////
//
//  MISC silo class.  This class handles all network functionality including:
//  - Get thermal sensor notification
//  - Get device diagnostic metrics
//  - Get IDC CWS info
//

#ifndef RRIL_SILO_MISC_H
#define RRIL_SILO_MISC_H


#include "silo.h"


class CSilo_MISC : public CSilo
{
public:
    CSilo_MISC(CChannel* pChannel);
    virtual ~CSilo_MISC();

    virtual char* GetBasicInitString();

protected:
    //  Parse notification functions here.
    virtual BOOL ParseXDRVI(CResponse* const pResponse, const char*& rszPointer);

    virtual BOOL ParseXTS(CResponse* const pResponse, const char*& pszPointer);

    virtual BOOL ParseXADPCLKFREQINFO(CResponse* const pResponse, const char*& pszPointer);

};

#endif // RRIL_SILO_MISC_H
