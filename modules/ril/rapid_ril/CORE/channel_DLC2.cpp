////////////////////////////////////////////////////////////////////////////
// channel_DLC2.cpp
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for helper functions used
//    to facilitate the use of multiple AT channels.
//    GPRS/UMTS management (GPRS attach/detach), network commands
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "channelbase.h"
#include "channel_DLC2.h"

extern char* g_szDLC2Port;
extern BOOL  g_bIsSocket;

// Init strings for this channel.
//  GPRS/UMTS management (GPRS attach/detach), network commands

// Add any init cmd strings for this channel during PowerOn or Ready boot phase
INITSTRING_DATA DLC2PowerOnInitString = { "" };
INITSTRING_DATA DLC2ReadyInitString   = { "" };

CChannel_DLC2::CChannel_DLC2(UINT32 uiChannel)
: CChannel(uiChannel)
{
    RIL_LOG_VERBOSE("CChannel_DLC2::CChannel_DLC2() - Enter\r\n");
    RIL_LOG_VERBOSE("CChannel_DLC2::CChannel_DLC2() - Exit\r\n");
}

CChannel_DLC2::~CChannel_DLC2()
{
    RIL_LOG_VERBOSE("CChannel_DLC2::~CChannel_DLC2() - Enter\r\n");
    delete[] m_paInitCmdStrings;
    m_paInitCmdStrings = NULL;
    RIL_LOG_VERBOSE("CChannel_DLC2::~CChannel_DLC2() - Exit\r\n");
}

//  Override from base class
BOOL CChannel_DLC2::OpenPort()
{
    BOOL bRetVal = FALSE;

    RIL_LOG_INFO("CChannel_DLC2::OpenPort() - Opening COM Port: %s...\r\n", g_szDLC2Port);
    RIL_LOG_INFO("CChannel_DLC2::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);

    bRetVal = m_Port.Open(g_szDLC2Port, g_bIsSocket);

    RIL_LOG_INFO("CChannel_DLC2::OpenPort() - Opening COM Port: %s\r\n",
            bRetVal ? "SUCCESS" : "FAILED!");

    return bRetVal;
}

BOOL CChannel_DLC2::FinishInit()
{
    RIL_LOG_VERBOSE("CChannel_DLC2::FinishInit() - Enter\r\n");
    BOOL bRet = FALSE;

    //  Init our channel AT init commands.
    m_paInitCmdStrings = new INITSTRING_DATA[COM_MAX_INDEX];
    if (!m_paInitCmdStrings)
    {
        RIL_LOG_CRITICAL("CChannel_DLC2::FinishInit() - chnl=[%d] Could not create new "
                "INITSTRING_DATA\r\n", m_uiRilChannel);
        goto Error;
    }

    // Set the init command strings for this channel
    m_paInitCmdStrings[COM_BASIC_INIT_INDEX].szCmd = m_szChannelBasicInitCmd;
    m_paInitCmdStrings[COM_UNLOCK_INIT_INDEX].szCmd = m_szChannelUnlockInitCmd;

    m_paInitCmdStrings[COM_POWER_ON_INIT_INDEX] = DLC2PowerOnInitString;
    m_paInitCmdStrings[COM_READY_INIT_INDEX] = DLC2ReadyInitString;

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CChannel_DLC2::FinishInit() - Exit\r\n");
    return bRet;
}
