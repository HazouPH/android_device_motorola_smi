////////////////////////////////////////////////////////////////////////////
// init7260.cpp
//
// Copyright (C) Intel 2013.
//
//
// Description:
//      Defines a specialized initializer for 7260 modem.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "init7260.h"

CInit7260::CInit7260()
{
    RIL_LOG_INFO("CInit7260::CInit7260() - Enter\r\n");
    RIL_LOG_INFO("CInit7260::CInit7260() - Exit\r\n");
}

CInit7260::~CInit7260()
{
    RIL_LOG_INFO("CInit7260::~CInit7260() - Enter\r\n");
    RIL_LOG_INFO("CInit7260::~CInit7260() - Exit\r\n");
}

BOOL CInit7260::Initialize()
{
    RIL_LOG_VERBOSE("CInit7260::Initialize() - Enter / Exit\r\n");
    return CInitIPCHSI::InitializeHSI();
}
