////////////////////////////////////////////////////////////////////////////
// init2230.h
//
// Copyright (C) Intel 2014.
//
//
// Description:
//      Defines a specialized initializer for 2230 modem.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_INIT2230_H
#define RRIL_INIT2230_H

#include "util.h"
#include "rilchannels.h"
#include "initializer.h"

class CInit2230 : public CInitIPCHSI
{
public:
    CInit2230();
    ~CInit2230();
};

#endif // RRIL_INIT2230_H
