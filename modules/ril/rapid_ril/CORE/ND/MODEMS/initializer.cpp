////////////////////////////////////////////////////////////////////////////
// initializer.cpp
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines the CInitializer base class and subclasses used to
//      create channels/silos and initialize the modem at start-up.
//
/////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>
#include "types.h"
#include "rril.h"
#include "rillog.h"
#include "thread_ops.h"
#include "rildmain.h"
#include "repository.h"
#include "reset.h"
#include "te.h"
#include "rilchannels.h"
#include "channel_atcmd.h"
#include "channel_data.h"
#include "channel_DLC2.h"
#include "channel_DLC6.h"
#include "channel_DLC8.h"
#include "channel_DLC22.h"
#include "channel_DLC23.h"
#include "channel_Sms.h"
#include "channel_URC.h"
#include "channel_OEM.h"
#include "silo_voice.h"
#include "silo_sim.h"
#include "silo_sms.h"
#include "silo_data.h"
#include "silo_network.h"
#include "silo_phonebook.h"
#include "silo_misc.h"
#include "silo_ims.h"
#include "silo_rfcoexistence.h"
#include "silo_common.h"
#include "util.h"
#include "initializer.h"

// used for 6360 and 7160 modems
int m_hsiChannelsReservedForClass1 = -1;
int m_hsiDataDirect = -1;
int m_dataProfilePathAssignation[NUMBER_OF_APN_PROFILE] = {0};


CInitializer::CInitializer()
: m_pPortsManagerMutex(NULL),
  m_pModemBasicInitCompleteEvent(NULL),
  m_pSimUnlockedEvent(NULL),
  m_pRadioPoweredOnEvent(NULL),
  m_pModemPoweredOffEvent(NULL),
  m_pInitStringCompleteEvent(NULL)
{
    RIL_LOG_VERBOSE("CInitializer::CInitializer() - Enter\r\n");

    memset(m_rgfChannelCompletedInit, 0, sizeof(m_rgfChannelCompletedInit));
    memset(m_cancelWaitPipeFds, -1, sizeof(m_cancelWaitPipeFds));

    RIL_LOG_VERBOSE("CInitializer::CInitializer() - Exit\r\n");
}

CInitializer::~CInitializer()
{
    RIL_LOG_VERBOSE("CInitializer::~CInitializer() - Enter\r\n");
    Destroy();
    RIL_LOG_VERBOSE("CInitializer::~CInitializer() - Exit\r\n");
}

BOOL CInitializer::Initialize()
{
    RIL_LOG_VERBOSE("CInitializer::Initialize() - Enter\r\n");
    BOOL bRetVal = FALSE;

    if (m_pPortsManagerMutex)
    {
        RIL_LOG_WARNING("CInitializer::Initialize() - m_pPortsManagerMutex was"
                " already created!\r\n");
    }
    else
    {
        m_pPortsManagerMutex = new CMutex();
        if (!m_pPortsManagerMutex)
        {
            RIL_LOG_CRITICAL("CInitializer::Initialize() - Could not create"
                    " m_pPortsManagerMutex.\r\n");
            goto Done;
        }
    }

    if (m_pModemBasicInitCompleteEvent)
    {
        RIL_LOG_WARNING("CInitializer::Initialize() - m_pModemBasicInitCompleteEvent"
                " was already created!\r\n");
    }
    else
    {
        m_pModemBasicInitCompleteEvent = new CEvent(NULL, TRUE);
        if (NULL == m_pModemBasicInitCompleteEvent)
        {
            RIL_LOG_CRITICAL("CInitializer::Initialize() - Could not create"
                    " Basic init complete Event.\r\n");
            goto Done;
        }
    }

    if (m_pSimUnlockedEvent)
    {
        RIL_LOG_WARNING("CInitializer::Initialize() - m_pSimUnlockedEvent was already"
                " created!\r\n");
    }
    else
    {
        m_pSimUnlockedEvent = new CEvent(NULL, TRUE);
        if (!m_pSimUnlockedEvent)
        {
            RIL_LOG_CRITICAL("CInitializer::Initialize() - Could not create"
                    " Sim Unlocked Event.\r\n");
            goto Done;
        }
    }

    if (m_pRadioPoweredOnEvent)
    {
        RIL_LOG_WARNING("CInitializer::Initialize() - m_pRadioPoweredOnEvent"
                " was already created!\r\n");
    }
    else
    {
        m_pRadioPoweredOnEvent = new CEvent(NULL, TRUE);
        if (!m_pRadioPoweredOnEvent)
        {
            RIL_LOG_CRITICAL("CInitializer::Initialize() - Could not create radio"
                    " Powered On Event.\r\n");
            goto Done;
        }
    }

    if (m_pModemPoweredOffEvent)
    {
        RIL_LOG_WARNING("CInitializer::Initialize() - m_pModemPoweredOffEvent"
                " was already created!\r\n");
    }
    else
    {
        m_pModemPoweredOffEvent = new CEvent(NULL, TRUE);
        if (!m_pModemPoweredOffEvent)
        {
            RIL_LOG_CRITICAL("CInitializer::Initialize() - Could not create Modem"
                    " Powered off Event.\r\n");
            goto Done;
        }
    }

    if (m_pInitStringCompleteEvent)
    {
        RIL_LOG_WARNING("CInitializer::Initialize() - m_pInitStringCompleteEvent"
                " was already created!\r\n");
    }
    else
    {
        m_pInitStringCompleteEvent = new CEvent(NULL, TRUE);
        if (!m_pInitStringCompleteEvent)
        {
            RIL_LOG_CRITICAL("CInitializer::Initialize() - Could not create Init commands"
                    " complete Event.\r\n");
            goto Done;
        }
    }

    bRetVal = TRUE;

Done:
    if (!bRetVal)
    {
        Destroy();
    }

    RIL_LOG_VERBOSE("CInitializer::Initialize() - Exit\r\n");
    return bRetVal;
}

void CInitializer::Destroy()
{
    if (m_pModemBasicInitCompleteEvent)
    {
        RIL_LOG_INFO("CInitializer::Destroy() - Before delete m_pModemBasicInitCompleteEvent\r\n");
        delete m_pModemBasicInitCompleteEvent;
        m_pModemBasicInitCompleteEvent = NULL;
    }

    if (m_pSimUnlockedEvent)
    {
        RIL_LOG_INFO("CInitializer::Destroy() - Before delete m_pSimUnlockedEvent\r\n");
        delete m_pSimUnlockedEvent;
        m_pSimUnlockedEvent = NULL;
    }

    if (m_pRadioPoweredOnEvent)
    {
        RIL_LOG_INFO("CInitializer::Destroy() - Before delete m_pRadioPoweredOnEvent\r\n");
        delete m_pRadioPoweredOnEvent;
        m_pRadioPoweredOnEvent = NULL;
    }

    if (m_pModemPoweredOffEvent)
    {
        RIL_LOG_INFO("CInitializer::Destroy() - Before delete m_pModemPoweredOffEvent\r\n");
        delete m_pModemPoweredOffEvent;
        m_pModemPoweredOffEvent = NULL;
    }

    if (m_pInitStringCompleteEvent)
    {
        RIL_LOG_INFO("CInitializer::Destroy() - Before delete"
                " m_pInitStringCompleteEvent\r\n");
        delete m_pInitStringCompleteEvent;
        m_pInitStringCompleteEvent = NULL;
    }

    if (m_pPortsManagerMutex)
    {
        CMutex::Unlock(m_pPortsManagerMutex);
        RIL_LOG_INFO("CInitializer::Destroy() - Before delete m_pPortsManagerMutex\r\n");
        delete m_pPortsManagerMutex;
        m_pPortsManagerMutex = NULL;
    }
}

BOOL CInitializer::InitializeModem()
{
    RIL_LOG_VERBOSE("CInitializer::InitializeModem() - Enter\r\n");
    BOOL bRetVal = FALSE;
    CThread* pModemThread = NULL;

    ResetStartupEvents();

    if (!SendModemInitCommands(COM_BASIC_INIT_INDEX))
    {
        RIL_LOG_CRITICAL("CInitializer::InitializeModem() -"
                " Unable to send basic init commands!\r\n");
        goto Error;
    }

    pModemThread = new CThread(StartModemInitializationThreadWrapper, this, THREAD_FLAGS_NONE, 0);
    if (!pModemThread || !CThread::IsRunning(pModemThread))
    {
        RIL_LOG_CRITICAL("CInitializer::InitializeModem() -"
                " Unable to launch modem init thread\r\n");
        goto Error;
    }

    bRetVal = TRUE;

Error:
    if (pModemThread)
    {
        delete pModemThread;
        pModemThread = NULL;
    }

    RIL_LOG_VERBOSE("CInitializer::InitializeModem() - Exit\r\n");
    return bRetVal;
}

BOOL CInitializer::SendModemInitCommands(eComInitIndex eInitIndex)
{
    RIL_LOG_VERBOSE("CInitializer::SendModemInitCommands() - Enter\r\n");

    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];

        if (g_pRilChannel[i])
        {
            if (!g_pRilChannel[i]->SendModemConfigurationCommands(eInitIndex))
            {
                RIL_LOG_CRITICAL("CInitializer::SendModemInitCommands() :"
                        " Channel=[%d] returned ERROR\r\n", i);
                return FALSE;
            }
        }
    }

    RIL_LOG_VERBOSE("CInitializer::SendModemInitCommands() - Exit\r\n");
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
void CInitializer::TriggerInitStringCompleteEvent(UINT32 uiChannel, eComInitIndex eInitIndex)
{
    SetChannelCompletedInit(uiChannel, eInitIndex);

    switch (eInitIndex)
    {
        case COM_READY_INIT_INDEX:
            if (VerifyAllChannelsCompletedInit(COM_READY_INIT_INDEX))
            {
                RIL_LOG_VERBOSE("CInitializer::TriggerInitStringCompleteEvent() -"
                        " DEBUG: All channels complete ready init!\r\n");
                CEvent::Signal(m_pInitStringCompleteEvent);
            }
            break;
        case COM_UNLOCK_INIT_INDEX:
            if (VerifyAllChannelsCompletedInit(COM_UNLOCK_INIT_INDEX))
            {
                RIL_LOG_INFO("CInitializer::TriggerInitStringCompleteEvent() -"
                        " DEBUG: All channels complete unlock init!\r\n");
                CTE::GetTE().HandleChannelsUnlockInitComplete();
            }
            break;
        case COM_BASIC_INIT_INDEX:
            if (VerifyAllChannelsCompletedInit(COM_BASIC_INIT_INDEX))
            {
                RIL_LOG_INFO("CInitializer::TriggerInitStringCompleteEvent() -"
                        " DEBUG: All channels complete basic init!\r\n");
                CEvent::Signal(m_pModemBasicInitCompleteEvent);

                CTE::GetTE().HandleChannelsBasicInitComplete();
            }
            break;
        case COM_POWER_ON_INIT_INDEX:
            if (VerifyAllChannelsCompletedInit(COM_POWER_ON_INIT_INDEX))
            {
                RIL_LOG_VERBOSE("CInitializer::TriggerInitStringCompleteEvent() -"
                        " DEBUG: All channels complete power on init!\r\n");
            }
            break;
        default:
            RIL_LOG_VERBOSE("CInitializer::TriggerInitStringCompleteEvent() -"
                    " DEBUG: Channel [%d] complete! Still waiting for other channels"
                    " to complete index [%d]!\r\n", uiChannel, eInitIndex);
            break;
    }
}

BOOL CInitializer::VerifyAllChannelsCompletedInit(eComInitIndex eInitIndex)
{
    BOOL bRetVal = TRUE;

    for (UINT32 i=0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (!IsChannelCompletedInit(i, eInitIndex))
        {
            bRetVal = FALSE;
            break;
        }
    }

    return bRetVal;
}

void CInitializer::SetChannelCompletedInit(UINT32 uiChannel, eComInitIndex eInitIndex)
{
    if ((uiChannel < g_uiRilChannelCurMax) && (eInitIndex < COM_MAX_INDEX))
    {
        m_rgfChannelCompletedInit[uiChannel][eInitIndex] = TRUE;
    }
    else
    {
        RIL_LOG_CRITICAL("CInitializer::SetChannelCompletedInit() - Invalid channel [%d] or init"
                " index [%d]\r\n", uiChannel, eInitIndex);
    }
}

BOOL CInitializer::IsChannelCompletedInit(UINT32 uiChannel, eComInitIndex eInitIndex)
{
    if ((uiChannel < g_uiRilChannelCurMax) && (eInitIndex < COM_MAX_INDEX))
    {
        return m_rgfChannelCompletedInit[uiChannel][eInitIndex];
    }
    else
    {
        RIL_LOG_CRITICAL("CInitializer::IsChannelCompletedInit() - Invalid channel [%d] or init"
                " index [%d]\r\n", uiChannel, eInitIndex);
        return FALSE;
    }
}

BOOL CInitializer::IsChannelUndefined(int channel)
{
    switch(channel) {
        case RIL_CHANNEL_ATCMD:
            if (!g_szCmdPort)
                return TRUE;
            break;
        case RIL_CHANNEL_DLC2:
            if (!g_szDLC2Port)
                return TRUE;
            break;
        case RIL_CHANNEL_DLC6:
            if (!g_szDLC6Port)
                return TRUE;
            break;
        case RIL_CHANNEL_DLC8:
            if (!g_szDLC8Port)
                return TRUE;
            break;
        case RIL_CHANNEL_DLC22:
            if (!g_szDLC22Port)
                return TRUE;
             break;
        case RIL_CHANNEL_DLC23:
            if (!g_szDLC23Port)
                return TRUE;
             break;
        case RIL_CHANNEL_URC:
            if (!g_szURCPort)
                return TRUE;
            break;
        case RIL_CHANNEL_OEM:
            if (!g_szOEMPort)
                return TRUE;
            break;
        case RIL_CHANNEL_DATA1:
            if (!g_szDataPort1)
                return TRUE;
            break;
        case RIL_CHANNEL_DATA2:
            if (!g_szDataPort2)
                return TRUE;
            break;
        case RIL_CHANNEL_DATA3:
            if (!g_szDataPort3)
                return TRUE;
            break;
        case RIL_CHANNEL_DATA4:
            if (!g_szDataPort4)
                return TRUE;
            break;
        case RIL_CHANNEL_DATA5:
            if (!g_szDataPort5)
                return TRUE;
            break;
        case RIL_CHANNEL_SMS:
            if (!g_szSmsPort)
                return TRUE;
             break;
        default: return FALSE;
    }

    return FALSE;
}

void CInitializer::ResetChannelCompletedInit()
{
    memset(m_rgfChannelCompletedInit, 0, sizeof(m_rgfChannelCompletedInit));
}

CChannel* CInitializer::CreateChannel(UINT32 eIndex)
{
    RIL_LOG_VERBOSE("CInitializer::CreateChannel() - Enter\r\n");
    CChannel* pChannel = NULL;

    switch(eIndex)
    {
        case RIL_CHANNEL_ATCMD:
            pChannel = new CChannel_ATCmd(eIndex);
            break;

        case RIL_CHANNEL_DLC2:
            pChannel = new CChannel_DLC2(eIndex);
            break;

        case RIL_CHANNEL_DLC6:
            pChannel = new CChannel_DLC6(eIndex);
            break;

        case RIL_CHANNEL_DLC8:
            pChannel = new CChannel_DLC8(eIndex);
            break;

        case RIL_CHANNEL_DLC22:
            pChannel = new CChannel_DLC22(eIndex);
            break;

        case RIL_CHANNEL_DLC23:
            pChannel = new CChannel_DLC23(eIndex);
            break;

        case RIL_CHANNEL_URC:
            pChannel = new CChannel_URC(eIndex);
            break;

        case RIL_CHANNEL_OEM:
            pChannel = new CChannel_OEM(eIndex);
            break;

        case RIL_CHANNEL_SMS:
            pChannel = new CChannel_Sms(eIndex);
            break;

        default:
            if (eIndex >= RIL_CHANNEL_DATA1) {
                pChannel = new CChannel_Data(eIndex);
            }
            break;
    }

    RIL_LOG_VERBOSE("CInitializer::CreateChannel() - Exit\r\n");
    return pChannel;
}

//
// Get the bitmask of silos for a channel from repository
//
// Returns a bitmask with format:
//      bit 11: Common silo      1000 0000 0000
//      bit 9-10: Reserved for new silos
//      bit 8: rfcoexistence silo 0001 0000 0000
//      bit 7: IMS silo           0000 1000 0000
//      bit 6: Misc silo          0000 0100 0000
//      bit 5: Phonebook silo     0000 0010 0000
//      bit 4: Network silo       0000 0001 0000
//      bit 3: Data silo          0000 0000 1000
//      bit 2: SMS silo           0000 0000 0100
//      bit 1: SIM silo           0000 0000 0010
//      bit 0: Voice silo         0000 0000 0001
//
int CInitializer::GetSiloConfig(UINT32 channel)
{
    RIL_LOG_VERBOSE("CInitializer::GetSiloConfig() - Enter\r\n");
    CRepository repository;
    int siloConfig = 0;

    // default silo configurations
    const int DefSiloConfigATCmd = 0x840; // Misc and common silo
    const int DefSiloConfigNetwork = 0x810; // Network and common silo
    const int DefSiloConfigVoice = 0x801; // Voice and common silo
    const int DefSiloConfigSms = 0x804; // SMS and common silo
    const int DefSiloConfigSIM = 0x802; // SIM and common silo
    const int DefSiloConfigURC = 0x877; // All silo's except Data and IMS
    const int DefSiloConfigData = 0x808; // Data and common silo
    const int DefSiloConfigCommon = 0x800; // common silo only
    const int DefSiloCopsCommon = 0x800; // common silo only
    const int DefSiloCopsRfcoexistence = 0x900; // Common silo and Rfcoexistence

    // Array of repository channel key names, ordered according to rilchannels.h
    // and default silo configuration if not available in repository
    SILO_CONFIG chanSiloConfig[RIL_CHANNEL_MAX] = {
        {g_szSilosATCmd, DefSiloConfigATCmd}, {g_szSilosDLC2, DefSiloConfigNetwork},
        {g_szSilosDLC6,  DefSiloConfigVoice}, {g_szSilosDLC8, DefSiloConfigSIM},
        {g_szSilosURC,   DefSiloConfigURC},  {g_szSilosOEM,  DefSiloConfigCommon},
        {g_szSilosDLC22, DefSiloCopsCommon}, {g_szSilosDLC23, DefSiloCopsRfcoexistence},
        {g_szSilosSms,   DefSiloConfigSms},  {g_szSilosData, DefSiloConfigData},
        {g_szSilosData,  DefSiloConfigData}, {g_szSilosData, DefSiloConfigData},
        {g_szSilosData,  DefSiloConfigData}, {g_szSilosData, DefSiloConfigData}
    };

    if (channel < g_uiRilChannelCurMax && channel < RIL_CHANNEL_MAX)
    {
        // Read bitmask of silos to add to channel from repository
        if (!repository.Read(g_szGroupChannelSilos, chanSiloConfig[channel].channel, siloConfig))
        {
            RIL_LOG_CRITICAL("CInitializer::GetSiloConfig() : Failed to read silo config "
                    "for chnl[%u]--MISSING from repository!!!\r\n", channel);

            // set default silos for channel
            siloConfig = chanSiloConfig[channel].silosDefault;
            RIL_LOG_INFO("CInitializer::GetSiloConfig() : Using default silo config [0x%x] "
                    "for chnl[%u]\r\n", siloConfig, channel);
        }
    }

    RIL_LOG_INFO("CInitializer::GetSiloConfig() - chnl=[%u], siloConfig=[0x%x]\r\n",
            channel, siloConfig);

Error:
    RIL_LOG_VERBOSE("CInitializer::GetSiloConfig() - Exit\r\n");
    return siloConfig;
}

BOOL CInitializer::CreateChannels()
{
    RIL_LOG_VERBOSE("CInitializer::CreateChannels() - Enter\r\n");
    BOOL bRet = FALSE;
    int siloConfig = 0;

    CMutex::Lock(m_pPortsManagerMutex);

    //  Init our array of global CChannel pointers.
    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (i == RIL_CHANNEL_RESERVED)
            continue;

        if (IsChannelUndefined(i))
            continue;

        // create and initialize a channel
        g_pRilChannel[i] = CreateChannel(i);
        if (!g_pRilChannel[i] || !g_pRilChannel[i]->Initialize())
        {
            RIL_LOG_CRITICAL("CInitializer::CreateChannels() : Channel[%d] (0x%X)"
                    " Init failed\r\n", i, (intptr_t)g_pRilChannel[i]);
            goto Error;
        }

        // get silo configuration for channel from repository
        siloConfig = GetSiloConfig(i);
        if (!siloConfig)
        {
            // no silos to add!
            RIL_LOG_INFO("CInitializer::CreateChannels() : chnl=[%d] No silos to add to "
                    "channel!\r\n", i);
            continue;
        }

        // create silos for channel
        if (!CreateSilos(g_pRilChannel[i], siloConfig))
        {
            RIL_LOG_CRITICAL("CInitializer::CreateChannels() : chnl=[%d] Failed to add silos to "
                    "channel!\r\n", i);
            goto Error;
        }
    }

    bRet = TRUE;

Error:
    CMutex::Unlock(m_pPortsManagerMutex);

    if (!bRet)
    {
        //  We had an error.
        DeleteChannels();
    }

    RIL_LOG_VERBOSE("CInitializer::CreateChannels() - Exit\r\n");
    return bRet;
}

BOOL CInitializer::OpenChannelPortsOnly()
{
    RIL_LOG_VERBOSE("CInitializer::OpenChannelPortsOnly() - Enter\r\n");

    BOOL bRet = FALSE;

    CMutex::Lock(m_pPortsManagerMutex);

    if (pipe(m_cancelWaitPipeFds) == -1)
    {
        RIL_LOG_WARNING("CInitializer::OpenChannelPortsOnly() - pipe creation failed\r\n");
        goto Done;
    }

    //  Init our array of global CChannel pointers.
    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (i == RIL_CHANNEL_RESERVED)
            continue;

        if (IsChannelUndefined(i))
            continue;

        /*
         * In flight mode, changing the WiFi state will result in MODEM_UP and
         * NOTIFY_MODEM_SHUTDOWN received within few milliseconds. Upon receiving MODEM_UP, init
         * thread will start opening the ports. Even upon receiving NOTIFY_MODEM_SHUTDOWN, port
         * opening will continue. Port opening fails only if the mux closes the tty ports. Mux
         * will close the tty only if the MMGR switches off/closes the ttyIFX0. Since it takes
         * quite a long time for the port opening failure, it is better to check modem status
         * before opening each port to avoid delays in cleaning up the resources.
         */
        if (E_MMGR_EVENT_MODEM_UP != CTE::GetTE().GetLastModemEvent())
        {
            RIL_LOG_CRITICAL("CInitializer::OpenChannelPortsOnly() : Channel[%d] OpenPort()"
                    " failed due to modem not up\r\n", i);
            goto Done;
        }

        if (!g_pRilChannel[i]->OpenPort())
        {
            RIL_LOG_CRITICAL("CInitializer::OpenChannelPortsOnly() : Channel[%d] OpenPort()"
                    " failed\r\n", i);
            goto Done;
        }

        if (!g_pRilChannel[i]->InitPort())
        {
            RIL_LOG_CRITICAL("CInitializer::OpenChannelPortsOnly() : Channel[%d] InitPort()"
                    " failed\r\n", i);
            goto Done;
        }
    }

    //  We made it this far, return TRUE.
    bRet = TRUE;

Done:
    CMutex::Unlock(m_pPortsManagerMutex);

    RIL_LOG_VERBOSE("CInitializer::OpenChannelPortsOnly() - Exit\r\n");
    return bRet;
}

void CInitializer::CloseChannelPorts()
{
    RIL_LOG_VERBOSE("CInitializer::CloseChannelPorts() - Enter\r\n");

    CEvent* pCancelWaitEvent = CSystemManager::GetInstance().GetCancelWaitEvent();

    // signal the cancel event to stop command thread
    if (NULL != pCancelWaitEvent)
    {
        CEvent::Signal(pCancelWaitEvent);
    }

    CMutex::Lock(m_pPortsManagerMutex);

    // Signal the response thread to exit by writing to pipe
    if (m_cancelWaitPipeFds[0] >= 0 && m_cancelWaitPipeFds[1] >= 0)
    {
        BOOL bTerminateResponseThread = 1;
        write(m_cancelWaitPipeFds[1], &bTerminateResponseThread, sizeof(BOOL));
    }

    RIL_LOG_INFO("CInitializer::CloseChannelPorts() - Before CThreadManager::Stop\r\n");
    CThreadManager::Stop();

    // Close the pipes
    close(m_cancelWaitPipeFds[0]);
    m_cancelWaitPipeFds[0] = -1;

    close(m_cancelWaitPipeFds[1]);
    m_cancelWaitPipeFds[1] = -1;

    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (g_pRilChannel[i])
        {
            g_pRilChannel[i]->ClosePort();
        }
    }

    CMutex::Unlock(m_pPortsManagerMutex);

    RIL_LOG_VERBOSE("CInitializer::CloseChannelPorts() - Exit\r\n");
}

void CInitializer::DeleteChannels()
{
    RIL_LOG_VERBOSE("CInitializer::DeleteChannels() - Enter\r\n");

    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (g_pRilChannel[i])
        {
            delete g_pRilChannel[i];
            g_pRilChannel[i] = NULL;
        }
    }
    RIL_LOG_VERBOSE("CInitializer::DeleteChannels() - Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////
CSilo* CInitializer::CreateSilo(CChannel* pChannel, int siloType)
{
    RIL_LOG_VERBOSE("CInitializer::CreateSilo() - Enter\r\n");
    CSilo* pSilo = NULL;

    switch (siloType)
    {
        case SILO_TYPE_VOICE:
            pSilo = new CSilo_Voice(pChannel);
            break;
        case SILO_TYPE_SIM:
            pSilo = new CSilo_SIM(pChannel);
            break;
        case SILO_TYPE_SMS:
            pSilo = new CSilo_SMS(pChannel);
            break;
        case SILO_TYPE_DATA:
            pSilo = new CSilo_Data(pChannel);
            break;
        case SILO_TYPE_NETWORK:
            pSilo = new CSilo_Network(pChannel);
            break;
        case SILO_TYPE_PHONEBOOK:
            pSilo = new CSilo_Phonebook(pChannel);
            break;
        case SILO_TYPE_MISC:
            pSilo = new CSilo_MISC(pChannel);
            break;
        case SILO_TYPE_IMS:
            pSilo = new CSilo_IMS(pChannel);
            break;
        case SILO_TYPE_RFCOEXISTENCE:
            pSilo = new CSilo_rfcoexistence(pChannel);
            break;
        case SILO_TYPE_COMMON:
            pSilo = new CSilo_Common(pChannel);
            break;
        default:
            RIL_LOG_CRITICAL("CInitializer::CreateSilo() - Unknown silo type [%d]!\r\n", siloType);
            break;
    }

    RIL_LOG_VERBOSE("CInitializer::CreateSilo() - Exit\r\n");
    return pSilo;
}

//
// Create silos for a channel, depending on silo config bitmask parameter
// Note: The CChannelbase destructor will destroy these CSilo objects.
//
BOOL CInitializer::CreateSilos(CChannel* pChannel, int siloConfig)
{
    RIL_LOG_VERBOSE("CInitializer::CreateSilos() : ENTER\r\n");
    BOOL bRet = FALSE;
    CSilo* pSilo = NULL;

    char* pszBasicInitString = pChannel->GetBasicInitCmd();
    char* pszUnlockInitString = pChannel->GetUnlockInitCmd();
    UINT32 uiRilChannel = pChannel->GetRilChannel();
    char* pszSiloInitString = NULL;

    for (int silo = 0; silo < SILO_MAX; silo++)
    {
        // only create a silo if set in siloConfig bitmask
        if (siloConfig & (1 << silo))
        {
            // create a silo and add it to the silo container
            pSilo = CreateSilo(pChannel, silo);
            if (!pSilo || !(pChannel->AddSilo(pSilo)))
            {
                RIL_LOG_CRITICAL("CInitializer::CreateSilos() : chnl=[%u] Could not add "
                        "silo(%d) to container!\r\n", uiRilChannel, silo);
                goto Error;
            }

            // get basic init string from a silo
            pszSiloInitString = (RIL_CHANNEL_URC == uiRilChannel) ? pSilo->GetURCInitString() :
                            pSilo->GetBasicInitString();
            if (pszSiloInitString != NULL && pszSiloInitString[0] != '\0')
            {
                if (!ConcatenateStringNullTerminate(pszBasicInitString, MAX_BUFFER_SIZE, "|"))
                {
                    RIL_LOG_CRITICAL("CInitializer::CreateSilos() : chnl=[%u] Failed to "
                            "concat | to basic init string from silo(%d)!\r\n",
                            uiRilChannel, silo);
                    goto Error;
                }
                if (!ConcatenateStringNullTerminate(pszBasicInitString,
                        MAX_BUFFER_SIZE, pszSiloInitString))
                {
                    RIL_LOG_CRITICAL("CInitializer::CreateSilos() : chnl=[%u] Failed to "
                            "concat basic init string from silo(%d)!\r\n",
                            uiRilChannel, silo);
                    goto Error;
                }
            }

            // get unlock init string from a silo
            pszSiloInitString = (RIL_CHANNEL_URC == uiRilChannel) ?
                            pSilo->GetURCUnlockInitString() : pSilo->GetUnlockInitString();
            if (pszSiloInitString != NULL && pszSiloInitString[0] != '\0')
            {
                if (!ConcatenateStringNullTerminate(pszUnlockInitString, MAX_BUFFER_SIZE, "|"))
                {
                    RIL_LOG_CRITICAL("CInitializer::CreateSilos() : chnl=[%u] Failed to "
                            "concat | to unlock init string from silo(%d)!\r\n",
                            uiRilChannel, silo);
                    goto Error;
                }
                if (!ConcatenateStringNullTerminate(pszUnlockInitString,
                            MAX_BUFFER_SIZE, pszSiloInitString))
                {
                    RIL_LOG_CRITICAL("CInitializer::CreateSilos() : chnl=[%u] Failed to "
                            "concat unlock init string from silo(%d)!\r\n",
                            uiRilChannel, silo);
                    goto Error;
                }
            }
        }
    }

    RIL_LOG_INFO("CInitializer::CreateSilos() : chnl=[%u], BasicInitCmd=[%s], "
            "UnlockInitCmd=[%s]\r\n", uiRilChannel, pszBasicInitString, pszUnlockInitString);

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CInitializer::CreateSilos() : EXIT\r\n");
    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
void* CInitializer::StartModemInitializationThreadWrapper(void* pArg)
{
    static_cast<CInitializer*>(pArg)->StartModemInitializationThread();
    return NULL;
}

void CInitializer::StartModemInitializationThread()
{
    RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() :"
            " Start Modem initialization thread\r\n");
    BOOL fUnlocked = FALSE;
    BOOL fPowerOn = FALSE;
    UINT32 uiNumEvents = 0;
    CEvent* pCancelWaitEvent = CSystemManager::GetInstance().GetCancelWaitEvent();

    while (!fUnlocked || !fPowerOn)
    {
        UINT32 ret = 0;

        if (!fUnlocked && !fPowerOn)
        {
            RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() - DEBUG: Waiting"
                    " for unlock, power on or cancel\r\n");
            CEvent* rgpEvents[] = { m_pSimUnlockedEvent, m_pRadioPoweredOnEvent,
                    pCancelWaitEvent };
            uiNumEvents = 3;
            ret = CEvent::WaitForAnyEvent(uiNumEvents, rgpEvents, WAIT_FOREVER);
        }
        else if (fUnlocked)
        {
            RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() - DEBUG: Waiting for"
                    " power on or cancel\r\n");
            CEvent* rgpEvents[] = { m_pRadioPoweredOnEvent, pCancelWaitEvent };
            uiNumEvents = 2;
            ret = CEvent::WaitForAnyEvent(uiNumEvents, rgpEvents, WAIT_FOREVER);
        }
        else
        {
            RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() - DEBUG: Waiting for"
                    " unlock or cancel\r\n");
            CEvent* rgpEvents[] = { m_pSimUnlockedEvent, pCancelWaitEvent };
            uiNumEvents = 2;
            ret = CEvent::WaitForAnyEvent(uiNumEvents, rgpEvents, WAIT_FOREVER);
        }

        if (3 == uiNumEvents)
        {
            switch (ret)
            {
                case WAIT_EVENT_0_SIGNALED:
                {
                    RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() -"
                            " DEBUG: Unlocked signaled\r\n");

                    if (!SendModemInitCommands(COM_UNLOCK_INIT_INDEX))
                    {
                        RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                                " Unable to send unlock init commands!\r\n");
                        goto Done;
                    }

                    fUnlocked = true;
                    break;
                }

                case WAIT_EVENT_0_SIGNALED + 1:
                {
                    RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() -"
                            " DEBUG: Power on signaled\r\n");

                    if (!SendModemInitCommands(COM_POWER_ON_INIT_INDEX))
                    {
                        RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                                " Unable to send power on init commands!\r\n");
                        goto Done;
                    }

                    fPowerOn = true;
                    break;
                }
                case WAIT_EVENT_0_SIGNALED + 2:
                    RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                            " Exit RIL event signaled!\r\n");
                    goto Done;
                    break;

                default:
                    RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                            " Failed waiting for events!\r\n");
                    goto Done;
                    break;
            }
        }
        else
        {
            switch (ret)
            {
                case WAIT_EVENT_0_SIGNALED:
                    if (fUnlocked)
                    {
                        RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() -"
                                " DEBUG: Power on signaled\r\n");

                        if (!SendModemInitCommands(COM_POWER_ON_INIT_INDEX))
                        {
                            RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                                    " Unable to send power on init commands!\r\n");
                            goto Done;
                        }

                        fPowerOn = true;
                    }
                    else
                    {
                        RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() -"
                                " DEBUG: Unlocked signaled\r\n");

                        if (!SendModemInitCommands(COM_UNLOCK_INIT_INDEX))
                        {
                            RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                                    " Unable to send unlock init commands!\r\n");
                            goto Done;
                        }

                        fUnlocked = true;
                    }
                    break;

                case WAIT_EVENT_0_SIGNALED + 1:
                    RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                            " Exit RIL event signaled!\r\n");
                    goto Done;
                    break;

                default:
                    RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                            " Failed waiting for events!\r\n");
                    goto Done;
                    break;
            }
        }
    }

    if (!SendModemInitCommands(COM_READY_INIT_INDEX))
    {
        RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                " Unable to send ready init commands!\r\n");
        goto Done;
    }

    {
        CEvent* rgpEvents[] = { m_pInitStringCompleteEvent, pCancelWaitEvent };

        switch(CEvent::WaitForAnyEvent(2, rgpEvents, WAIT_FOREVER))
        {
            case WAIT_EVENT_0_SIGNALED:
            {
                RIL_LOG_INFO("CInitializer::StartModemInitializationThread() -"
                        " INFO: Initialization strings complete\r\n");
                goto Done;
                break;
            }

            case WAIT_EVENT_0_SIGNALED + 1:
            default:
            {
                RIL_LOG_CRITICAL("CInitializer::StartModemInitializationThread() -"
                        " Exiting ril!\r\n");
                goto Done;
                break;
            }
        }
    }

Done:
    RIL_LOG_VERBOSE("CInitializer::StartModemInitializationThread() :"
            " Modem initialized, thread exiting\r\n");
}

void CInitializer::ResetStartupEvents()
{
    RIL_LOG_INFO("CInitializer::Resetting Startup events...\r\n");
    CEvent::Reset(m_pModemBasicInitCompleteEvent);
    CEvent::Reset(m_pSimUnlockedEvent);
    CEvent::Reset(m_pRadioPoweredOnEvent);
    CEvent::Reset(m_pInitStringCompleteEvent);
}

///////////////////////////////////////////////////////////////////////////////
//
// IPC Initializer subclasses

CInitIPCUSB::CInitIPCUSB()
{
    RIL_LOG_VERBOSE("CInitIPCUSB::CInitIPCUSB() - Enter / Exit\r\n");
}

CInitIPCUSB::~CInitIPCUSB()
{
    RIL_LOG_VERBOSE("CInitIPCUSB::~CInitIPCUSB() - Enter / Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////
CInitIPCHSI::CInitIPCHSI()
{
    RIL_LOG_VERBOSE("CInitIPCHSI::CInitIPCHSI() - Enter / Exit\r\n");
}

CInitIPCHSI::~CInitIPCHSI()
{
    RIL_LOG_VERBOSE("CInitIPCHSI::~CInitIPCHSI() - Enter / Exit\r\n");
}

BOOL CInitIPCHSI::InitializeHSI()
{
    RIL_LOG_VERBOSE("CInitIPCHSI::InitializeHSI() - Enter\r\n");

    CRepository repository;
    int apnType = 0;
    int ipcDataChannelMin = 0;

    if (!repository.Read(g_szGroupModem, g_szApnTypeDefault, apnType))
    {
        RIL_LOG_WARNING("CInitIPCHSI::InitializeHSI() : Could not read network apn type"
                " default from repository\r\n");
    }
    else
    {
        m_dataProfilePathAssignation[0] = apnType;
        RIL_LOG_WARNING("CInitIPCHSI::InitializeHSI() - ApnTypeDUN: %d...\r\n",
                apnType);
    }

    if (!repository.Read(g_szGroupModem, g_szApnTypeDUN, apnType))
    {
        RIL_LOG_WARNING("CInitIPCHSI::InitializeHSI() : Could not read network apn type"
                " Tethered from repository\r\n");
    }
    else
    {
        m_dataProfilePathAssignation[1] = apnType;
    }

    if (!repository.Read(g_szGroupModem, g_szApnTypeIMS, apnType))
    {
        RIL_LOG_WARNING("CInitIPCHSI::InitializeHSI() : Could not read network apn type"
                " IMS from repository\r\n");
    }
    else
    {
        m_dataProfilePathAssignation[2] = apnType;
    }

    if (!repository.Read(g_szGroupModem, g_szApnTypeFOTA, apnType))
    {
        RIL_LOG_WARNING("CInitIPCHSI::InitializeHSI() : Could not read network apn type"
                " MMS from repository\r\n");
    }
    else
    {
        m_dataProfilePathAssignation[3] = apnType;
    }

    if (!repository.Read(g_szGroupModem, g_szApnTypeCBS, apnType))
    {
        RIL_LOG_WARNING("CInitIPCHSI::InitializeHSI() : Could not read network apn type"
                " CBS from repository\r\n");
    }
    else
    {
        m_dataProfilePathAssignation[4] = apnType;
    }

    if (!repository.Read(g_szGroupModem, g_szHsiDataDirect, m_hsiDataDirect))
    {
        RIL_LOG_WARNING("CInitIPCHSI::InitializeHSI() : Could not read network apn type"
                " default from repository\r\n");
    }

    if (!repository.Read(g_szGroupModem, g_szIpcDataChannelMin, ipcDataChannelMin))
    {
        RIL_LOG_WARNING("CInitIPCHSI::InitializeHSI() : Could not read min"
            " IPC Data channel from repository\r\n");
        ipcDataChannelMin = RIL_DEFAULT_IPC_CHANNEL_MIN;
    }

    m_hsiChannelsReservedForClass1 = 0;
    for (UINT32 i = 0; i < NUMBER_OF_APN_PROFILE; i++)
    {
        if (m_dataProfilePathAssignation[i] == 1)
        {
            m_hsiChannelsReservedForClass1++;
        }
    }

    if (m_hsiChannelsReservedForClass1 > m_hsiDataDirect)
    {
        RIL_LOG_CRITICAL("CInitIPCHSI::InitializeHSI() : Too much class1 APN\r\n");
        goto Done;
    }

    if (m_hsiDataDirect > (RIL_MAX_NUM_IPC_CHANNEL - ipcDataChannelMin))
    {
        RIL_LOG_CRITICAL("CInitIPCHSI::InitializeHSI() : Too much hsi channel reserved"
                " for data\r\n");
        goto Done;
    }

Done:
    RIL_LOG_VERBOSE("CInitIPCHSI::InitializeHSI() - Exit\r\n");
    return CInitializer::Initialize();
}
