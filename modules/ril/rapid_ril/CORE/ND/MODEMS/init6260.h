////////////////////////////////////////////////////////////////////////////
// init6260.h
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines a specialized initializer for 6260 modem.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_INIT6260_H
#define RRIL_INIT6260_H

#include "util.h"
#include "rilchannels.h"
#include "initializer.h"

class CInit6260 : public CInitIPCHSI
{
public:
    CInit6260();
    ~CInit6260();
};

#endif // RRIL_INIT6260_H
