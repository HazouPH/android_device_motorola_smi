////////////////////////////////////////////////////////////////////////////
// channel_URC.h
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CChannel_URC class, which is used to
//    facilitate the use of multiple AT channels.
//    All URCs go on this channel.
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(RIL_CHANNEL_URC_H)
#define RIL_CHANNEL_URC_H

#include "channel_nd.h"

class CChannel_URC : public CChannel
{
public:
    CChannel_URC(UINT32 uiChannel);
    virtual ~CChannel_URC();

    //  public port interface
    BOOL OpenPort();

protected:
    BOOL FinishInit();
};

#endif  // RIL_CHANNEL_URC_H

