////////////////////////////////////////////////////////////////////////////
// channel_ATCmd.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for helper functions used
//    to facilitate the use of multiple command channels.
//    Call control commands, misc commands
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "channelbase.h"
#include "channel_atcmd.h"

extern char* g_szCmdPort;
extern BOOL  g_bIsSocket;

// Init strings for this channel.
//  Call control commands, misc commands

// Add any init cmd strings for this channel during PowerOn or Ready boot phase
INITSTRING_DATA ATCmdPowerOnInitString = { "" };
INITSTRING_DATA ATCmdReadyInitString = { "" };

CChannel_ATCmd::CChannel_ATCmd(UINT32 uiChannel)
: CChannel(uiChannel)
{
    RIL_LOG_VERBOSE("CChannel_ATCmd::CChannel_ATCmd() - Enter\r\n");
    RIL_LOG_VERBOSE("CChannel_ATCmd::CChannel_ATCmd() - Exit\r\n");
}

CChannel_ATCmd::~CChannel_ATCmd()
{
    RIL_LOG_VERBOSE("CChannel_ATCmd::~CChannel_ATCmd() - Enter\r\n");
    delete[] m_paInitCmdStrings;
    RIL_LOG_VERBOSE("CChannel_ATCmd::~CChannel_ATCmd() - Exit\r\n");
    m_paInitCmdStrings = NULL;
}

//  Override from base class
BOOL CChannel_ATCmd::OpenPort()
{
    BOOL bRetVal = FALSE;

    RIL_LOG_INFO("CChannel_ATCmd::OpenPort() - Opening COM Port: %s...\r\n", g_szCmdPort);
    RIL_LOG_INFO("CChannel_ATCmd::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);

    bRetVal = m_Port.Open(g_szCmdPort, g_bIsSocket);

    RIL_LOG_INFO("CChannel_ATCmd::OpenPort() - Opening COM Port: %s\r\n", bRetVal ? "SUCCESS"
            : "FAILED!");

    return bRetVal;
}


BOOL CChannel_ATCmd::FinishInit()
{
    RIL_LOG_VERBOSE("CChannel_ATCmd::FinishInit() - Enter\r\n");
    BOOL bRet = FALSE;

    // Init our channel AT init commands.
    m_paInitCmdStrings = new INITSTRING_DATA[COM_MAX_INDEX];
    if (!m_paInitCmdStrings)
    {
        RIL_LOG_CRITICAL("CChannel_ATCmd::FinishInit() : chnl=[%d] Could not create new "
                "INITSTRING_DATA\r\n", m_uiRilChannel);
        goto Error;
    }

    // Set the init command strings for this channel
    m_paInitCmdStrings[COM_BASIC_INIT_INDEX].szCmd = m_szChannelBasicInitCmd;
    m_paInitCmdStrings[COM_UNLOCK_INIT_INDEX].szCmd = m_szChannelUnlockInitCmd;

    m_paInitCmdStrings[COM_POWER_ON_INIT_INDEX] = ATCmdPowerOnInitString;
    m_paInitCmdStrings[COM_READY_INIT_INDEX] = ATCmdReadyInitString;

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CChannel_ATCmd::FinishInit() - Exit\r\n");
    return bRet;
}
