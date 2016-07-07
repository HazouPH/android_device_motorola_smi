////////////////////////////////////////////////////////////////////////////
// channel_OEM.h
//
// Copyright 2005-2012 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CChannel_OEM class, which is used to
//    facilitate the use of multiple AT channels.
//    Diagnostic commands using OEM HOOK API are sent on this channel.
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(RIL_CHANNEL_OEM_H)
#define RIL_CHANNEL_OEM_H

#include "channel_nd.h"

class CChannel_OEM : public CChannel
{
public:
    CChannel_OEM(UINT32 uiChannel);
    virtual ~CChannel_OEM();

    //  public port interface
    BOOL OpenPort();

protected:
    BOOL FinishInit();
};

#endif  // RIL_CHANNEL_OEM_H

