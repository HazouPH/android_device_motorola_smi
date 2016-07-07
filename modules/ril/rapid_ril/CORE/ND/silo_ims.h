////////////////////////////////////////////////////////////////////////////
// silo_IMS.h
//
// Copyright 2013 Intel Corporation, All Rights Reserved.
//
//
// Description:
//    Defines the CSilo_IMS class, which provides response handlers and
//    parsing functions for the IMS-related RIL components.
//
/////////////////////////////////////////////////////////////////////////////
//
//  IMS silo class.  This class handles IMS functionality including:
//  - IMS register/unregister notification
//  - Network IMS support notification


#ifndef RRIL_SILO_IMS_H
#define RRIL_SILO_IMS_H


#include "silo.h"


class CSilo_IMS : public CSilo
{
public:
    CSilo_IMS(CChannel *pChannel);
    virtual ~CSilo_IMS();

    virtual char* GetURCInitString();

protected:
    //  Parse notification functions here.
    virtual BOOL ParseCIREPI(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL ParseCIREPH(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL ParseCIREGU(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL ParseXISRVCCI(CResponse* const pResponse, const char*& rszPointer);
};

#endif // RRIL_SILO_IMS_H
