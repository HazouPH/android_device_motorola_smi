////////////////////////////////////////////////////////////////////////////
// init7160.cpp
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines a specialized initializer for 7160 modem.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "init7160.h"

CInit7160::CInit7160()
{
    RIL_LOG_INFO("CInit7160::CInit7160() - Enter\r\n");
    RIL_LOG_INFO("CInit7160::CInit7160() - Exit\r\n");
}

CInit7160::~CInit7160()
{
    RIL_LOG_INFO("CInit7160::~CInit7160() - Enter\r\n");
    RIL_LOG_INFO("CInit7160::~CInit7160() - Exit\r\n");
}

BOOL CInit7160::Initialize()
{
    RIL_LOG_VERBOSE("CInit7160::Initialize() - Enter / Exit\r\n");
    return CInitIPCHSI::InitializeHSI();
}
