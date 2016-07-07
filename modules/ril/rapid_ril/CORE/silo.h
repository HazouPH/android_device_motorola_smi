////////////////////////////////////////////////////////////////////////////
// silo.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the base class from which the various RIL Silos are derived.
//
/////////////////////////////////////////////////////////////////////////////
//
//  Silo abstract class
//

#ifndef RRIL_SILO_H
#define RRIL_SILO_H

#include "command.h"

class CSilo;
class CResponse;
class CChannel;
class CRilHandle;

typedef BOOL (CSilo::*PFN_ATRSP_PARSE) (CResponse* const pResponse, const char*& rszPointer);

typedef struct atrsptable_tag
{
    const char*     szATResponse;
    PFN_ATRSP_PARSE pfnATParseRsp;
} ATRSPTABLE;


class CSilo
{
public:
    CSilo(CChannel* pChannel);
    virtual ~CSilo();

    //  Called at beginning of CResponse::ParseUnsolicitedResponse().
    //  [in] pResponse = Pointer to CResponse class
    //  [in/out] rszPointer = Pointer to string response buffer.
    //  [in/out] fGotoError = Set to TRUE if we wish to stop response chain and goto Error in
    //  CResponse::ParseUnsolicitedResponse().
    //  Return values:
    //  TRUE if response is handled by this hook, then handling still stop.
    //  FALSE if response is not handled by this hook, and handling will continue to other silos,
    //  then framework.
    virtual BOOL ParseUnsolicitedResponse(CResponse* const pResponse, const char*& rszPointer,
            BOOL& fGotoError);

    // Functions to get silo-specific init strings
    virtual char* GetBasicInitString() { return NULL; }
    virtual char* GetUnlockInitString() { return NULL; }
    virtual char* GetURCInitString() { return NULL; }
    virtual char* GetURCUnlockInitString() { return NULL; }

protected:
    char m_cTerminator;
    char m_szNewLine[3];

    CChannel* m_pChannel;

    ATRSPTABLE* m_pATRspTable;
    ATRSPTABLE* m_pATRspTableExt;

    // Stub function that is never called but used by ParseUnsolicitedResponse to mark
    // the end of the parse tables.
    BOOL ParseNULL(CResponse* const pResponse, const char*& rszPointer);

    // General function to skip this response and flag as unrecognized.
    BOOL ParseUnrecognized(CResponse* const pResponse, const char*& rszPointer);

    // Store silo-specific init strings
    char m_szBasicInitString[MAX_BUFFER_SIZE];
    char m_szUnlockInitString[MAX_BUFFER_SIZE];
    char m_szURCInitString[MAX_BUFFER_SIZE];
    char m_szURCUnlockInitString[MAX_BUFFER_SIZE];

private:
    // helper functions
    PFN_ATRSP_PARSE FindParser(ATRSPTABLE* pRspTable, const char*& pszStr);
};

#endif // RRIL_SILO_H
