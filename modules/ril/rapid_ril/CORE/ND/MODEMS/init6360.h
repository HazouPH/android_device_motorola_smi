////////////////////////////////////////////////////////////////////////////
// init6360.h
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines a specialized initializer for 6360 modem.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_INIT6360_H
#define RRIL_INIT6360_H

#include "util.h"
#include "rilchannels.h"
#include "initializer.h"

class CInit6360 : public CInitIPCHSI
{
public:
    CInit6360();
    ~CInit6360();

protected:
    virtual BOOL Initialize();
};

#endif // RRIL_INIT6360_H
