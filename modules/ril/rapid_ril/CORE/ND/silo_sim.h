////////////////////////////////////////////////////////////////////////////
// silo_SIM.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CSilo_SIM class, which provides response handlers and
//    parsing functions for the SIM-related RIL components.
//
/////////////////////////////////////////////////////////////////////////////
//
//  SIM silo class.  This class handles SIM commands and SIM Toolkit functionality including:
//  -Send SIM commands and send restricted SIM commands
//  -Get SIM records and SIM status
//  -Lock/Unlock/PIN/PUK password SIM functions
//  -Fixed dialing/PIN2/PUK2 SIM functions
//  -Call barring
//  -Get user identity
//  -Handle SIM Toolkit functions
//

#ifndef RRIL_SILO_SIM_H
#define RRIL_SILO_SIM_H


#include "silo.h"

class CSilo_SIM : public CSilo
{
public:
    CSilo_SIM(CChannel *pChannel);
    virtual ~CSilo_SIM();

    virtual char* GetURCInitString();

protected:
    //  Parse notification functions here.
    virtual BOOL    ParseIndicationSATI(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseIndicationSATN(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseTermRespConfirm(CResponse* const pResponse, const char*& rszPointer);

    virtual BOOL    ParseXSIM(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseXLOCK(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseXLEMA(CResponse* const pResponse, const char*& rszPointer);

    virtual BOOL    ParseIndicationCusats(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseIndicationCusatp(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseIndicationCusatend(CResponse* const pResponse, const char*& rszPointer);

    //  Emergency Call Codes list
    char m_szECCList[MAX_BUFFER_SIZE];

private:
    BOOL IsProactiveCmd(const char* szUrcPointer, UINT8* puiCmdId);
    void ParsePduForRefresh(const char* pszPdu);
};

#endif // RRIL_SILO_SIM_H
