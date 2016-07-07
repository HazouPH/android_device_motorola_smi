////////////////////////////////////////////////////////////////////////////
// init7160.h
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines a specialized initializer for 6360 modem.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_INIT7160_H
#define RRIL_INIT7160_H

#include "util.h"
#include "rilchannels.h"
#include "initializer.h"

class CInit7160 : public CInitIPCHSI
{
public:
    CInit7160();
    ~CInit7160();

protected:
    virtual BOOL Initialize();
};

#endif // RRIL_INIT7160_H
