////////////////////////////////////////////////////////////////////////////
// channel_Sms.h
//
// Copyright (C) Intel 2014.
//
//
// Description:
//    Provides implementations for helper functions used
//    to facilitate the use of blocking commands.
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(RIL_CHANNEL_SMS_H)
#define RIL_CHANNEL_SMS_H

#include "channel_nd.h"

class CChannel_Sms : public CChannel
{
public:
    CChannel_Sms(UINT32 uiChannel);
    virtual ~CChannel_Sms();

    //  public port interface
    BOOL OpenPort();

protected:
    BOOL FinishInit();
};

#endif  // RIL_CHANNEL_SMS_H

