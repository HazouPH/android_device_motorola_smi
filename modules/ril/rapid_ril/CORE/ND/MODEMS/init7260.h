////////////////////////////////////////////////////////////////////////////
// init7260.h
//
// Copyright (C) Intel 2013.
//
//
// Description:
//      Defines a specialized initializer for 7260 modem.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_INIT7260_H
#define RRIL_INIT7260_H

#include "util.h"
#include "rilchannels.h"
#include "initializer.h"

class CInit7260 : public CInitIPCHSI
{
public:
    CInit7260();
    ~CInit7260();

protected:
    virtual BOOL Initialize();
};

#endif // RRIL_INIT7260_H
