////////////////////////////////////////////////////////////////////////////
// channel_OEM.cpp
//
// Copyright 2005-2012 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for helper functions used
//    to facilitate the use of multiple AT channels.
//    Diagnostic commands using OEM HOOK API are sent on this channel.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "channelbase.h"
#include "channel_OEM.h"

extern char* g_szOEMPort;
extern BOOL  g_bIsSocket;

// Init strings for this channel.
//  OEM commands (diagnostic)

// Add any init cmd strings for this channel during PowerOn or Ready boot phase
INITSTRING_DATA OEMPowerOnInitString = { "" };
INITSTRING_DATA OEMReadyInitString   = { "" };

CChannel_OEM::CChannel_OEM(UINT32 uiChannel)
: CChannel(uiChannel)
{
    RIL_LOG_VERBOSE("CChannel_OEM::CChannel_OEM() - Enter\r\n");
    RIL_LOG_VERBOSE("CChannel_OEM::CChannel_OEM() - Exit\r\n");
}

CChannel_OEM::~CChannel_OEM()
{
    RIL_LOG_VERBOSE("CChannel_OEM::~CChannel_OEM() - Enter\r\n");
    delete[] m_paInitCmdStrings;
    m_paInitCmdStrings = NULL;
    RIL_LOG_VERBOSE("CChannel_OEM::~CChannel_OEM() - Exit\r\n");
}

//  Override from base class
BOOL CChannel_OEM::OpenPort()
{
    BOOL bRetVal = FALSE;

    RIL_LOG_INFO("CChannel_OEM::OpenPort() - Opening COM Port: %s...\r\n", g_szOEMPort);
    RIL_LOG_INFO("CChannel_OEM::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);

    bRetVal = m_Port.Open(g_szOEMPort, g_bIsSocket);

    RIL_LOG_INFO("CChannel_OEM::OpenPort() - Opening COM Port: %s\r\n",
            bRetVal ? "SUCCESS" : "FAILED!");

    return bRetVal;
}

BOOL CChannel_OEM::FinishInit()
{
    RIL_LOG_VERBOSE("CChannel_OEM::FinishInit() - Enter\r\n");
    BOOL bRet = FALSE;

    //  Init our channel AT init commands.
    m_paInitCmdStrings = new INITSTRING_DATA[COM_MAX_INDEX];
    if (!m_paInitCmdStrings)
    {
        RIL_LOG_CRITICAL("CChannel_OEM::FinishInit() - chnl=[%d] Could not create new"
                " INITSTRING_DATA\r\n", m_uiRilChannel);
        goto Error;
    }

    // Set the init command strings for this channel
    m_paInitCmdStrings[COM_BASIC_INIT_INDEX].szCmd = m_szChannelBasicInitCmd;
    m_paInitCmdStrings[COM_UNLOCK_INIT_INDEX].szCmd = m_szChannelUnlockInitCmd;

    m_paInitCmdStrings[COM_POWER_ON_INIT_INDEX] = OEMPowerOnInitString;
    m_paInitCmdStrings[COM_READY_INIT_INDEX] = OEMReadyInitString;

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CChannel_OEM::FinishInit() - Exit\r\n");
    return bRet;
}
