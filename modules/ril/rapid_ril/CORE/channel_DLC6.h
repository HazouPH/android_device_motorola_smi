////////////////////////////////////////////////////////////////////////////
// channel_DLC6.h
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CChannel_DLC6 class, which is used to
//    facilitate the use of multiple AT channels.
//    Call settings, SMS, supplementary services
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(RIL_CHANNEL_DLC6_H)
#define RIL_CHANNEL_DLC6_H

#include "channel_nd.h"

class CChannel_DLC6 : public CChannel
{
public:
    CChannel_DLC6(UINT32 uiChannel);
    virtual ~CChannel_DLC6();

    //  public port interface
    BOOL OpenPort();

protected:
    BOOL FinishInit();
};

#endif  // RIL_CHANNEL_DLC6_H

