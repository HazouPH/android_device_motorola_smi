////////////////////////////////////////////////////////////////////////////
// silo_voice.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CSilo_Voice class, which provides response handlers and
//    parsing functions for the voice-related RIL components.
//
/////////////////////////////////////////////////////////////////////////////
//
//  Voice silo class.  This class handles the voice call and CSD call functionality including:
//  -Dial/Answer/Hangup voice call
//  -Dial/Answer/Hangup CSD call
//  -DTMF
//  -Call management (hold, transfer, conference)
//  -Supplementary services including:
//    -Call forwarding
//    -Call waiting
//    -Caller ID
//    -USSD
//    -Handle GSM 02.30 commands
//  -Voice call audio volume and muting
//  -Set audio device
//  -Get/Set equipment state
//  -Misc functions


#ifndef RRIL_SILO_VOICE_H
#define RRIL_SILO_VOICE_H


#include "silo.h"


class CSilo_Voice : public CSilo
{
public:
    CSilo_Voice(CChannel* pChannel);
    virtual ~CSilo_Voice();

    virtual char* GetBasicInitString();
    virtual char* GetURCInitString();
    virtual char* GetURCUnlockInitString();

protected:
    //  Parse notification functions here.

    virtual BOOL    ParseExtRing(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseConnect(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseCallWaitingInfo(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseUnsolicitedSSInfo(CResponse* const pResponse, const char*& szPointer);
    virtual BOOL    ParseIntermediateSSInfo(CResponse* const pResponse, const char*& szPointer);
    virtual BOOL    ParseCallMeter(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseCallProgressInformation(CResponse* const pResponse,
                                                   const char*& rszPointer);
    virtual BOOL    ParseUSSDInfo(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseConnLineIdRestriction(CResponse* const pResponse,
                                                 const char*& rszPointer);
    virtual BOOL    ParseDISCONNECT(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseIndicatorEvent(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseXCALLSTAT(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseBusy(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseNoAnswer(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseCTMCall(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseNoCTMCall(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseWaitingCallCTM(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL    ParseXUCCI(CResponse* const pResponse, const char*& rszPointer);

    virtual BOOL ParseCNAP(CResponse* const pResponse, const char*& rszPointer);
    virtual BOOL ParseCLIP(CResponse* const pResponse, const char*& rszPointer);

private:
    UINT32 m_uiCallId;
};

#endif // RRIL_SILO_VOICE_H
