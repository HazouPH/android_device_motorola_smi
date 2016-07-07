////////////////////////////////////////////////////////////////////////////
// init6360.cpp
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines a specialized initializer for 6360 modem.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "init6360.h"

CInit6360::CInit6360()
{
    RIL_LOG_INFO("CInit6360::CInit6360() - Enter\r\n");
    RIL_LOG_INFO("CInit6360::CInit6360() - Exit\r\n");
}

CInit6360::~CInit6360()
{
    RIL_LOG_INFO("CInit6360::~CInit6360() - Enter\r\n");
    RIL_LOG_INFO("CInit6360::~CInit6360() - Exit\r\n");
}

BOOL CInit6360::Initialize()
{
    RIL_LOG_VERBOSE("CInit6360::Initialize() - Enter / Exit\r\n");
    return CInitIPCHSI::InitializeHSI();
}
