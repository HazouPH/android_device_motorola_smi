////////////////////////////////////////////////////////////////////////////
// channel_ATCmd.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CChannel_ATCmd class, which is used to
//    facilitate the use of multiple command channels.
//    Call control commands, misc commands
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(RIL_CHANNEL_ATCMD_H)
#define RIL_CHANNEL_ATCMD_H

#include "channel_nd.h"


class CChannel_ATCmd : public CChannel
{
public:
    CChannel_ATCmd(UINT32 uiChannel);
    virtual ~CChannel_ATCmd();

    //  public port interface
    BOOL OpenPort();

protected:
    BOOL FinishInit();
};

#endif // RIL_CHANNEL_ATCMD_H
