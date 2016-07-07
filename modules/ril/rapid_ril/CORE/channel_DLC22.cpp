////////////////////////////////////////////////////////////////////////////
// channel_DLC22.cpp
//
// Copyright (c) 2014, Intel Corporation
// All rights reserved.
//
// Description:
//    Provides implementations for helper functions used
//    to facilitate the use of blocking commands commands.
//
// ---------------------------------------------------------------------------
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// Neither the name of the Intel Corporation nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES,
// (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "channelbase.h"
#include "channel_DLC22.h"

extern char* g_szDLC22Port;
extern BOOL  g_bIsSocket;

// Init strings for this channel.
// Blocking commands

// Add any init cmd strings for this channel during PowerOn or Ready boot phase
INITSTRING_DATA DLC22PowerOnInitString = { "" };
INITSTRING_DATA DLC22ReadyInitString   = { "" };

CChannel_DLC22::CChannel_DLC22(UINT32 uiChannel)
: CChannel(uiChannel)
{
    RIL_LOG_VERBOSE("CChannel_DLC22::CChannel_DLC22() - Enter\r\n");
    RIL_LOG_VERBOSE("CChannel_DLC22::CChannel_DLC22() - Exit\r\n");
}

CChannel_DLC22::~CChannel_DLC22()
{
    RIL_LOG_VERBOSE("CChannel_DLC22::~CChannel_DLC22() - Enter\r\n");
    delete[] m_paInitCmdStrings;
    m_paInitCmdStrings = NULL;
    RIL_LOG_VERBOSE("CChannel_DLC22::~CChannel_DLC22() - Exit\r\n");
}

//  Override from base class
BOOL CChannel_DLC22::OpenPort()
{
    BOOL bRetVal = FALSE;

    RIL_LOG_INFO("CChannel_DLC22::OpenPort() - Opening COM Port: %s...\r\n", g_szDLC22Port);
    RIL_LOG_INFO("CChannel_DLC22::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);

    bRetVal = m_Port.Open(g_szDLC22Port, g_bIsSocket);

    RIL_LOG_INFO("CChannel_DLC22::OpenPort() - Opening COM Port: %s\r\n",
            bRetVal ? "SUCCESS" : "FAILED!");

    return bRetVal;
}

BOOL CChannel_DLC22::FinishInit()
{
    RIL_LOG_VERBOSE("CChannel_DLC22::FinishInit() - Enter\r\n");
    BOOL bRet = FALSE;

    //  Init our channel AT init commands.
    m_paInitCmdStrings = new INITSTRING_DATA[COM_MAX_INDEX];
    if (!m_paInitCmdStrings)
    {
        RIL_LOG_CRITICAL("CChannel_DLC22::FinishInit() - chnl=[%d] Could not create new "
                "INITSTRING_DATA\r\n", m_uiRilChannel);
        goto Error;
    }

    // Set the init command strings for this channel
    m_paInitCmdStrings[COM_BASIC_INIT_INDEX].szCmd = m_szChannelBasicInitCmd;
    m_paInitCmdStrings[COM_UNLOCK_INIT_INDEX].szCmd = m_szChannelUnlockInitCmd;

    m_paInitCmdStrings[COM_POWER_ON_INIT_INDEX] = DLC22PowerOnInitString;
    m_paInitCmdStrings[COM_READY_INIT_INDEX] = DLC22ReadyInitString;

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CChannel_DLC22::FinishInit() - Exit\r\n");
    return bRet;
}
