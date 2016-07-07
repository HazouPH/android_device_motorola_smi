////////////////////////////////////////////////////////////////////////////
// channel_Sms.cpp
//
// Copyright (C) Intel 2014.
//
//
// Description:
//    Provides implementations for helper functions used
//    to facilitate the use of blocking commands.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "channelbase.h"
#include "channel_Sms.h"

extern char* g_szSmsPort;
extern BOOL  g_bIsSocket;

// Init strings for this channel.
// Blocking commands

// Add any init cmd strings for this channel during PowerOn or Ready boot phase
INITSTRING_DATA SmsPowerOnInitString = { "" };
INITSTRING_DATA SmsReadyInitString   = { "" };

CChannel_Sms::CChannel_Sms(UINT32 uiChannel)
: CChannel(uiChannel)
{
    RIL_LOG_VERBOSE("CChannel_Sms::CChannel_Sms() - Enter/Exit\r\n");
}

CChannel_Sms::~CChannel_Sms()
{
    RIL_LOG_VERBOSE("CChannel_Sms::~CChannel_Sms() - Enter/Exit\r\n");
    delete[] m_paInitCmdStrings;
    m_paInitCmdStrings = NULL;
}

//  Override from base class
BOOL CChannel_Sms::OpenPort()
{
    BOOL bRetVal = FALSE;

    RIL_LOG_INFO("CChannel_Sms::OpenPort() - Opening COM Port: %s... "
            "g_bIsSocket=[%d]...\r\n", g_szSmsPort, g_bIsSocket);

    bRetVal = m_Port.Open(g_szSmsPort, g_bIsSocket);

    RIL_LOG_INFO("CChannel_Sms::OpenPort() - Opening COM Port: %s\r\n",
            bRetVal ? "SUCCESS" : "FAILED!");

    return bRetVal;
}

BOOL CChannel_Sms::FinishInit()
{
    RIL_LOG_VERBOSE("CChannel_Sms::FinishInit() - Enter\r\n");
    BOOL bRet = FALSE;

    //  Init our channel AT init commands.
    m_paInitCmdStrings = new INITSTRING_DATA[COM_MAX_INDEX];
    if (!m_paInitCmdStrings)
    {
        RIL_LOG_CRITICAL("CChannel_Sms::FinishInit() - chnl=[%d] Could not create new "
                "INITSTRING_DATA\r\n", m_uiRilChannel);
        goto Error;
    }

    // Set the init command strings for this channel
    m_paInitCmdStrings[COM_BASIC_INIT_INDEX].szCmd = m_szChannelBasicInitCmd;
    m_paInitCmdStrings[COM_UNLOCK_INIT_INDEX].szCmd = m_szChannelUnlockInitCmd;

    m_paInitCmdStrings[COM_POWER_ON_INIT_INDEX] = SmsPowerOnInitString;
    m_paInitCmdStrings[COM_READY_INIT_INDEX] = SmsReadyInitString;

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CChannel_Sms::FinishInit() - Exit\r\n");
    return bRet;
}
