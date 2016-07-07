////////////////////////////////////////////////////////////////////////////
// channel_DLC2.h
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CChannel_DLC2 class, which is used to
//    facilitate the use of multiple AT channels.
//    GPRS/UMTS management (GPRS attach/detach), network commands
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(RIL_CHANNEL_DLC2_H)
#define RIL_CHANNEL_DLC2_H

#include "channel_nd.h"

class CChannel_DLC2 : public CChannel
{
public:
    CChannel_DLC2(UINT32 uiChannel);
    virtual ~CChannel_DLC2();

    //  public port interface
    BOOL OpenPort();

protected:
    BOOL FinishInit();
};

#endif  // RIL_CHANNEL_DLC2_H

