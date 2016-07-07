////////////////////////////////////////////////////////////////////////////
// silo_common.h
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CSilo_Common class, which provides response handlers and
//    parsing functions for URCs received in all channels.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_SILO_COMMON_H
#define RRIL_SILO_COMMON_H

#include "silo.h"

class CSilo_Common : public CSilo
{
public:
    CSilo_Common(CChannel *pChannel);
    virtual ~CSilo_Common();
};

#endif // RRIL_SILO_COMMON_H
