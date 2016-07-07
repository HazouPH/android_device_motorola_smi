////////////////////////////////////////////////////////////////////////////
// channelbase.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implements the CChannelBase class, which provides the
//    infrastructure for the various AT Command channels.
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "types.h"
#include "util.h"
#include "thread_manager.h"
#include "rilqueue.h"
#include "rillog.h"
#include "systemmanager.h"
#include "cmdcontext.h"
#include "thread_ops.h"
#include "silo.h"
#include "rildmain.h"
#include "reset.h"
#include "repository.h"
#include "channelbase.h"
#include "te.h"
#include "hardwareconfig.h"

CChannelBase::CChannelBase(UINT32 uiChannel)
  : m_uiRilChannel(uiChannel),
    m_bTimeoutWaitingForResponse(0),
    m_fWaitingForRsp(FALSE),
    m_fLastCommandTimedOut(FALSE),
    m_fFinalInitOK(FALSE),
    m_pCmdThread(NULL),
    m_pReadThread(NULL),
    m_pBlockReadThreadEvent(NULL),
    m_bReadThreadBlocked(TRUE),
    m_uiLockCommandQueue(0),
    m_uiLockCommandQueueTimeout(0),
    m_paInitCmdStrings(NULL),
    m_bPossibleInvalidFD(FALSE),
    m_pPossibleInvalidFDMutex(NULL),
    m_pResponseObjectAccessMutex(NULL)
{
    RIL_LOG_VERBOSE("CChannelBase::CChannelBase() - Enter\r\n");

    memset(&m_SiloContainer, 0, sizeof(SILO_CONTAINER));

    m_szChannelBasicInitCmd[0] = '\0';
    m_szChannelUnlockInitCmd[0] = '\0';

    // default init string sent to all channels
    ConcatenateStringNullTerminate(m_szChannelBasicInitCmd,
            sizeof(m_szChannelBasicInitCmd), "E0V1Q0X4|+CMEE=1|S0=0");

    RIL_LOG_VERBOSE("CChannelBase::CChannelBase() - Exit\r\n");
}

CChannelBase::~CChannelBase()
{
    RIL_LOG_VERBOSE("CChannelBase::~CChannelBase() - Enter\r\n");

    if (m_pCmdThread)
    {
        delete m_pCmdThread;
        m_pCmdThread = NULL;
    }

    if (m_pReadThread)
    {
        delete m_pReadThread;
        m_pReadThread = NULL;
    }

    if (m_pBlockReadThreadEvent)
    {
        delete m_pBlockReadThreadEvent;
        m_pBlockReadThreadEvent = NULL;
    }

    if (m_pPossibleInvalidFDMutex)
    {
        delete m_pPossibleInvalidFDMutex;
        m_pPossibleInvalidFDMutex = NULL;
    }

    if (m_pResponseObjectAccessMutex)
    {
        delete m_pResponseObjectAccessMutex;
        m_pResponseObjectAccessMutex = NULL;
    }

    int count = m_SiloContainer.nSilos;
    for (int i = 0; i < count; ++i)
    {
        CSilo* pSilo = NULL;
        pSilo = m_SiloContainer.rgpSilos[i];
        delete pSilo;
        pSilo = NULL;
    }

    m_SiloContainer.nSilos = 0;

    RIL_LOG_VERBOSE("CChannelBase::~CChannelBase() - Exit\r\n");
}

//
// Initialize a channel
//
BOOL CChannelBase::Initialize()
{
    RIL_LOG_VERBOSE("CChannelBase::Initialize() - Enter\r\n");
    BOOL bResult = FALSE;

    if (m_uiRilChannel >= g_uiRilChannelCurMax)
    {
        RIL_LOG_CRITICAL("CChannelBase::Initialize() - chnl=[%d] Channel is invalid!\r\n",
                m_uiRilChannel);
        goto Error;
    }

    m_pBlockReadThreadEvent = new CEvent(NULL, TRUE, TRUE);
    if (!m_pBlockReadThreadEvent)
    {
        RIL_LOG_CRITICAL("CChannelBase::Initialize() - chnl=[%d] Failed to create block read"
                " thread event!\r\n", m_uiRilChannel);
        goto Error;
    }
    // Unblock read thread by default.
    UnblockReadThread();

    m_pPossibleInvalidFDMutex = new CMutex();
    if (!m_pPossibleInvalidFDMutex)
    {
        RIL_LOG_CRITICAL("CChannelBase::Initialize() - chnl=[%d] Failed to create"
                " m_pPossibleInvalidFDMutex!\r\n", m_uiRilChannel);
        goto Error;
    }

    m_pResponseObjectAccessMutex = new CMutex();
    if (!m_pResponseObjectAccessMutex)
    {
        RIL_LOG_CRITICAL("CChannelBase::Initialize() - chnl=[%u] Failed to create "
                "m_pResponseObjectAccessMutex!\r\n", m_uiRilChannel);
        goto Error;
    }

    if (!FinishInit())
    {
        RIL_LOG_CRITICAL("CChannelBase::Initialize() - chnl=[%d] FinishInit() failed!\r\n",
                m_uiRilChannel);
        goto Error;
    }

    bResult = TRUE;

Error:
    if (!bResult)
    {
        delete m_pBlockReadThreadEvent;
        m_pBlockReadThreadEvent = NULL;

        delete m_pPossibleInvalidFDMutex;
        m_pPossibleInvalidFDMutex = NULL;

        delete m_pResponseObjectAccessMutex;
        m_pResponseObjectAccessMutex = NULL;
    }

    RIL_LOG_VERBOSE("CChannelBase::Initialize() - Exit\r\n");
    return bResult;
}

//
// Add a silo to the channel.  Add CSilo to m_gpcSilos collection.
//
BOOL CChannelBase::AddSilo(CSilo* pSilo)
{
    RIL_LOG_VERBOSE("CChannelBase::AddSilo() - Enter\r\n");

    BOOL bResult = FALSE;

    if (!pSilo)
    {
        goto Done;
    }

    if (SILO_MAX > m_SiloContainer.nSilos)
    {
        m_SiloContainer.rgpSilos[m_SiloContainer.nSilos++] = pSilo;
    }
    else
    {
        RIL_LOG_CRITICAL("CChannelBase::AddSilo() : Unable to add silo as container is full\r\n");
        goto Done;
    }

    bResult = TRUE;

Done:
    RIL_LOG_VERBOSE("CChannelBase::AddSilo() - Exit\r\n");
    return bResult;
}


//
// Thread responsible for sending commands from the Command Queue to COM port
//
void* ChannelCommandThreadStart(void* pVoid)
{
    RIL_LOG_VERBOSE("ChannelCommandThreadStart() - Enter\r\n");

    if (NULL == pVoid)
    {
        RIL_LOG_CRITICAL("ChannelCommandThreadStart() : lpParameter was NULL\r\n");
        return NULL;
    }

    CChannelBase* pChannel = (CChannelBase*)pVoid;

    if (pChannel)
    {
        RIL_LOG_INFO("ChannelCommandThreadStart() : chnl=[%d] Entering, pChannel=0x%X\r\n",
                pChannel->GetRilChannel(), pChannel);
        pChannel->CommandThread();
        RIL_LOG_INFO("ChannelCommandThreadStart() : chnl=[%d] THREAD IS EXITING\r\n",
                pChannel->GetRilChannel());
    }

    RIL_LOG_VERBOSE("ChannelCommandThreadStart() - Exit\r\n");
    return NULL;
}

//
// Thread responsible for reading responses from COM port into the Response Queue
//
void* ChannelResponseThreadStart(void* pVoid)
{
    RIL_LOG_VERBOSE("ChannelResponseThreadStart() - Enter\r\n");

    if (NULL == pVoid)
    {
        RIL_LOG_CRITICAL("ChannelResponseThreadStart() : pVoid was NULL\r\n");
        return NULL;
    }

    CChannelBase* pChannel = (CChannelBase*)pVoid;

    if (pChannel)
    {
        RIL_LOG_INFO("ChannelResponseThreadStart() : chnl=[%d] Entering, pChannel=0x%X\r\n",
                pChannel->GetRilChannel(), pChannel);
        pChannel->ResponseThread();
        RIL_LOG_INFO("ChannelResponseThreadStart() : chnl=[%d] THREAD IS EXITING\r\n",
                pChannel->GetRilChannel());
    }

    RIL_LOG_VERBOSE("ChannelResponseThreadStart() - Exit\r\n");
    return NULL;
}

BOOL CChannelBase::StartChannelThreads()
{
    RIL_LOG_VERBOSE("CChannelBase::StartChannelThreads() - Enter\r\n");
    BOOL bResult = FALSE;

    //  Launch command thread.
    m_pCmdThread = new CThread(ChannelCommandThreadStart, (void*)this, THREAD_FLAGS_JOINABLE, 0);
    if (!m_pCmdThread)
    {
        RIL_LOG_CRITICAL("CChannelBase::StartChannelThreads() -"
                " Unable to launch command thread\r\n");
        goto Done;
    }

    //  Launch response thread.
    m_pReadThread = new CThread(ChannelResponseThreadStart, (void*)this, THREAD_FLAGS_JOINABLE, 0);
    if (!m_pReadThread)
    {
        RIL_LOG_CRITICAL("CChannelBase::StartChannelThreads() -"
                " Unable to launch response thread\r\n");
        goto Done;
    }

    // Switch the read thread into higher priority
    // (to guarantee that the module's in buffer doesn't get overflown)
    if (!CThread::SetPriority(m_pReadThread, THREAD_PRIORITY_LEVEL_HIGH))
    {
        // RIL_LOG_WARNING("CChannelBase::StartChannelThreads() : WARN : Unable to raise"
        //         "priority of read thread!!\r\n");
    }

    bResult = TRUE;

Done:
    if (!bResult)
    {
        if (m_pCmdThread)
        {
            delete m_pCmdThread;
            m_pCmdThread = NULL;
        }

        if (m_pReadThread)
        {
            delete m_pReadThread;
            m_pReadThread = NULL;
        }
    }

    RIL_LOG_VERBOSE("CChannelBase::StartChannelThreads() - Exit\r\n");
    return bResult;
}


//
//  Terminate the threads and close the handles.
//
BOOL CChannelBase::StopChannelThreads()
{
    RIL_LOG_VERBOSE("CChannelBase::StopChannelThreads() - Enter\r\n");
    BOOL bResult = TRUE;
    const UINT32 uiThreadTime = 15000;

    UnblockReadThread();

    RIL_LOG_INFO("CChannelBase::StopChannelThreads() : INFO : Wait for Command Thread!\r\n");

    // Wait for auxiliary threads to terminate
    if (THREAD_WAIT_TIMEOUT == CThread::Wait(m_pCmdThread, uiThreadTime))
    {
        RIL_LOG_CRITICAL("CChannelBase::StopChannelThreads() : We timed out waiting on command"
                " thread!\r\n");
        bResult = FALSE;
    }
    else
    {
        RIL_LOG_INFO("CChannelBase::StopChannelThreads() : INFO : Command Thread has successfully"
                " exited!\r\n");
    }

    RIL_LOG_INFO("CChannelBase::StopChannelThreads() : INFO : Wait for Response Thread!\r\n");

    if (THREAD_WAIT_TIMEOUT == CThread::Wait(m_pReadThread, uiThreadTime))
    {
        RIL_LOG_CRITICAL("CChannelBase::StopChannelThreads() : We timed out waiting on response"
                " thread!\r\n");
        bResult = FALSE;
    }
    else
    {
        RIL_LOG_INFO("CChannelBase::StopChannelThreads() : INFO : Response Thread has successfully"
                " exited!\r\n");
    }

    if (m_pCmdThread)
    {
        delete m_pCmdThread;
        m_pCmdThread = NULL;
    }

    if (m_pReadThread)
    {
        delete m_pReadThread;
        m_pReadThread = NULL;
    }

    RIL_LOG_INFO("CChannelBase::StopChannelThreads() : INFO : Thread pointers were deleted!\r\n");

    RIL_LOG_VERBOSE("CChannelBase::StopChannelThreads() - Exit\r\n");

    return bResult;
}

void CChannelBase::ClearCommandQueue()
{
    RIL_LOG_VERBOSE("CChannelBase::ClearCommandQueue() - Enter\r\n");

    CCommand* pCmd = NULL;

    while (!g_pTxQueue[m_uiRilChannel]->IsEmpty())
    {
        if (!g_pTxQueue[m_uiRilChannel]->Dequeue(pCmd))
        {
            //  Dequeue() returns false when the queue is empty.
            //  In this case, log the error, but continue processing other commands.
            RIL_LOG_CRITICAL("CChannelBase::ClearCommandQueue() : chnl=[%d] Dequeue TxQueue "
                    "returned FALSE\r\n", m_uiRilChannel);
            return;
        }

        if (NULL != pCmd)
        {
            RIL_Token rilToken = pCmd->GetToken();

            if (NULL != rilToken)
            {
                RIL_LOG_VERBOSE("CChannelBase::ClearCommandQueue() - Complete for token "
                        "0x%08x, error: %d\r\n", rilToken, RIL_E_GENERIC_FAILURE);
                RIL_onRequestComplete(rilToken, RIL_E_GENERIC_FAILURE, NULL, 0);
            }

            pCmd->FreeContextData();

            delete pCmd;
            pCmd = NULL;
        }
    }

    RIL_LOG_VERBOSE("CChannelBase::ClearCommandQueue() - Exit\r\n");
}

UINT32 CChannelBase::CommandThread()
{
    RIL_LOG_VERBOSE("CChannelBase::CommandThread() - Enter\r\n");
    CCommand* pCmd = NULL;
    RIL_RESULT_CODE resCode;

    //  Double-check our channel is valid.
    if (m_uiRilChannel >= g_uiRilChannelCurMax)
    {
        RIL_LOG_CRITICAL("CChannelBase::CommandThread() - Invalid channel value: %d\r\n",
                m_uiRilChannel);
        return 0;
    }

    CThreadManager::RegisterThread();
    while (TRUE)
    {
        if (g_pTxQueue[m_uiRilChannel]->IsEmpty())
        {
            //RIL_LOG_INFO("CChannelBase::CommandThread() :"
            // "TxQueue queue empty, waiting for command\r\n");
            // wait for a command, or exit event
            if (!WaitForCommand())
            {
                RIL_LOG_CRITICAL("CChannelBase::CommandThread() : WaitForCommand returns False,"
                        " exiting channel [%d]\r\n",m_uiRilChannel);
                break;
            }
        }

        if (E_MMGR_EVENT_MODEM_UP != CTE::GetTE().GetLastModemEvent())
        {
            RIL_LOG_CRITICAL("CChannelBase::CommandThread() : chnl=[%d] Failed init, returning"
                    " RIL_E_GENERIC_FAILURE\r\n", m_uiRilChannel);

            ClearCommandQueue();

            break;
        }

        if (!g_pTxQueue[m_uiRilChannel]->Dequeue(pCmd))
        {
            //  Dequeue() returns false when the queue is empty.
            //  In this case, log the error, but continue processing other commands.
            RIL_LOG_CRITICAL("CChannelBase::CommandThread() : chnl=[%d] Dequeue TxQueue returned"
                    " FALSE\r\n", m_uiRilChannel);
            continue;
        }

        if (NULL != pCmd)
        {
            if (!CTE::GetTE().IsRequestAllowed(pCmd->GetRequestID(),
                    pCmd->GetToken(), pCmd->GetChannel(), pCmd->IsInitCommand(), pCmd->GetCallId()))
            {
                delete pCmd;
                pCmd = NULL;

                continue;
            }
        }

        if (!SendCommand(pCmd))
        {
            RIL_LOG_CRITICAL("CChannelBase::CommandThread() :"
                    "chnl=[%d] Unable to send command!\r\n", m_uiRilChannel);

            delete pCmd;
            pCmd = NULL;

            continue;
        }

        if (NULL != pCmd)
        {
            RIL_LOG_CRITICAL("CChannelBase::CommandThread() : chnl=[%d] pCmd was not NULL following"
                    " SendCommand()\r\n");
            goto Done;
        }

    }

Done:

    if (pCmd)
    {
        delete pCmd;
        pCmd = NULL;
    }

    RIL_LOG_VERBOSE("CChannelBase::CommandThread() - Exit\r\n");
    return 0;
}

BOOL CChannelBase::WaitForCommand()
{
    CEvent* pCancelWaitEvent = CSystemManager::GetInstance().GetCancelWaitEvent();
    CEvent* rpEvents[] = { g_TxQueueEvent[m_uiRilChannel], pCancelWaitEvent };
    UINT32 uiRet = WAIT_EVENT_0_SIGNALED;

    CEvent::Reset(g_TxQueueEvent[m_uiRilChannel]);

    // Wait for an event only if the TxQueue is empty.
    // This is necessary because the above Reset() could clear the event set
    // in CCommand::AddCmdToQueue()
    if (g_pTxQueue[m_uiRilChannel]->IsEmpty())
    {
        uiRet = CEvent::WaitForAnyEvent(2, rpEvents, WAIT_FOREVER);
    }

    return  (WAIT_EVENT_0_SIGNALED == uiRet);
}

char* CChannelBase::GetTESpecificInitCommands(eComInitIndex eInitIndex)
{
    char* pInitCommands = NULL;

    if (COM_BASIC_INIT_INDEX == eInitIndex)
    {
        pInitCommands = CTE::GetTE().GetBasicInitCommands( m_uiRilChannel);
    }
    else if (COM_UNLOCK_INIT_INDEX == eInitIndex)
    {
        pInitCommands = CTE::GetTE().GetUnlockInitCommands( m_uiRilChannel);
    }

    return pInitCommands;
}

//
// Send an intialization command string to the COM port
//
BOOL CChannelBase::SendModemConfigurationCommands(eComInitIndex eInitIndex)
{
    RIL_LOG_VERBOSE("CChannelBase::SendModemConfigurationCommands() - Enter\r\n");
    RIL_LOG_INFO("CChannelBase::SendModemConfigurationCommands() : chnl=[%d] index=[%d]\r\n",
            m_uiRilChannel, eInitIndex);

    char*        szInit;
    BOOL         bRetVal  = FALSE;
    CCommand*    pCmd = NULL;
    BOOL         bLastCmd;
    CRepository  repository;
    char         szTemp[MAX_BUFFER_SIZE];
    int SIMId = CHardwareConfig::GetInstance().GetSIMId();

    szInit = new char[MAX_BUFFER_SIZE];
    if (!szInit)
    {
        RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() : Out of memory\r\n");
        goto Done;
    }
    szInit[0] = '\0';

    if (eInitIndex >= COM_MAX_INDEX)
    {
        RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() : Invalid index [%d]\r\n",
                eInitIndex);
        goto Done;
    }

    if (NULL == m_paInitCmdStrings)
    {
        RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() : m_paInitCmdStrings was"
                " NULL!\r\n");
        goto Done;
    }

    // send xsimsel command first

    if (eInitIndex == COM_BASIC_INIT_INDEX &&
            CHardwareConfig::GetInstance().IsMultiSIM() &&
            !CHardwareConfig::GetInstance().IsMultiModem())
    {
        RIL_LOG_INFO("CChannelBase::SendModemConfigurationCommands() : Concat XSIMSEL id=%d,"
                "  eInitIndex=[%d]\r\n", SIMId, eInitIndex);
        PrintStringNullTerminate(szTemp, MAX_BUFFER_SIZE, "+XSIMSEL=%d|",
                SIMId);
        ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, szTemp);
    }

    // Get any pre-init commands from non-volatile memory

    if (repository.Read(g_szGroupInitCmds, g_szPreInitCmds[eInitIndex], szTemp, MAX_BUFFER_SIZE))
    {
        if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, szTemp))
        {
            RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() : Concat szTemp"
                    " failed\r\n");
            goto Done;
        }
        if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, "|"))
        {
            RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() :"
                    " Concat szTemp | failed\r\n");
            goto Done;
        }
    }

    // Add the hard-coded Init Commands to the Init String

    if (NULL == m_paInitCmdStrings[eInitIndex].szCmd)
    {
        RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() :"
                " m_paInitCmdStrings[%d].szCmd was NULL!\r\n", eInitIndex);
        goto Done;
    }

    if (m_paInitCmdStrings[eInitIndex].szCmd[0])
    {
        if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE,
                m_paInitCmdStrings[eInitIndex].szCmd))
        {
            RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() : Concat szCmd failed"
                    "  eInitIndex=[%d]r\n", eInitIndex);
            goto Done;
        }
    }

    // Get any post-init commands from non-volatile memory

    if (repository.Read(g_szGroupInitCmds, g_szPostInitCmds[eInitIndex], szTemp, MAX_BUFFER_SIZE))
    {
        if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, "|"))
        {
            RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() : Concat | failed"
                    "  eInitIndex=[%d]\r\n", eInitIndex);
            goto Done;
        }
        if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, szTemp))
        {
            RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() : Concat szTemp"
                    " failed  eInitIndex=[%d]\r\n", eInitIndex);
            goto Done;
        }
    }

    // Add any post-init commands that depend on parameters in the repository

    //  Only send "Fast Dormancy Timer" and "RxDiversity" commands on AT command channel
    //  and in first init index.
    if ((COM_BASIC_INIT_INDEX == eInitIndex) && (RIL_CHANNEL_ATCMD == m_uiRilChannel))
    {
        // Read the "conformance" property and disable FD if it is set to "true"
        char szConformanceProperty[PROPERTY_VALUE_MAX] = {'\0'};
        property_get("persist.conformance", szConformanceProperty, NULL);

        if (strncmp(szConformanceProperty, "true", PROPERTY_VALUE_MAX))
        {
            // Data for Fast Dormancy Mode
            char szFDCmdString[MAX_BUFFER_SIZE] = {0};
            char szFDDelayTimer[MAX_BUFFER_SIZE] = {0};
            char szSCRITimer[MAX_BUFFER_SIZE] = {0};

            // Read Fast Dormancy Timers from repository
            repository.ReadFDParam(g_szGroupModem, g_szFDDelayTimer,
                    szFDDelayTimer, MAX_BUFFER_SIZE, XFDOR_MIN_FDDELAY_TIMER,
                    XFDOR_MAX_FDDELAY_TIMER);
            repository.ReadFDParam(g_szGroupModem, g_szSCRITimer,
                    szSCRITimer, MAX_BUFFER_SIZE, XFDOR_MIN_SCRI_TIMER, XFDOR_MAX_SCRI_TIMER);

            // define XFDOR command according to FD mode
            switch (CTE::GetTE().GetFastDormancyMode())
            {
                case E_FD_MODE_ALWAYS_ON :
                    if (!PrintStringNullTerminate(szFDCmdString,
                                          sizeof(szFDCmdString),
                                          "|+XFDOR=2,%s,%s",
                                         szFDDelayTimer, szSCRITimer))
                    {
                        RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() :"
                                "Cannot create Fast Dormancy command\r\n");
                        goto Done;
                    }
                    break;
                 case E_FD_MODE_OEM_MANAGED :
                 case E_FD_MODE_DISPLAY_DRIVEN :
                 default :
                    break;
            }

            if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, szFDCmdString))
            {
                RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() :"
                        "Concat szFDCmdString failed\r\n");
                goto Done;
            }
        }
    }

    if (COM_BASIC_INIT_INDEX == eInitIndex
            || COM_UNLOCK_INIT_INDEX == eInitIndex)
    {
        char* pInitCommands = NULL;
        BOOL bSuccess = TRUE;

        pInitCommands = GetTESpecificInitCommands(eInitIndex);
        if (NULL != pInitCommands && '\0' != pInitCommands[0])
        {
            if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, "|"))
            {
                RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() :"
                        " Concat | failed\r\n");
                bSuccess = FALSE;
            }
            else if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, pInitCommands))
            {
                RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() : Concatenate"
                        " TE specific init commands failed\r\n");
                bSuccess = FALSE;
            }
        }

        free(pInitCommands);
        pInitCommands = NULL;

        if (!bSuccess)
        {
            goto Done;
        }
    }

    // Set the real time clock
    if ((COM_POWER_ON_INIT_INDEX == eInitIndex) && (RIL_CHANNEL_URC == m_uiRilChannel))
    {
#if defined(HAVE_LOCALTIME_R)
        struct tm tm;
#endif
        struct tm* ptm;
        time_t t;

        time(&t);
#if defined(HAVE_LOCALTIME_R)
        ptm = localtime_r(&t, &tm);
#else
        ptm = localtime(&t);
#endif
        if (ptm != NULL)
        {
            char URCClockInitString[32];
            memset(URCClockInitString, 0, sizeof(URCClockInitString));
            strftime(URCClockInitString, sizeof(URCClockInitString),
                    "|+CCLK=\"%y/%m/%d,%H:%M:%S\"", ptm);

            if (!ConcatenateStringNullTerminate(szInit, MAX_BUFFER_SIZE, URCClockInitString))
            {
                RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() :"
                        " Concat DataPath failed\r\n");
                goto Done;
            }
        }
        else
        {
            RIL_LOG_WARNING("CChannelBase::SendModemConfigurationCommands() - localtime error");
        }
    }

    RIL_LOG_INFO("CChannelBase::SendModemConfigurationCommands() : String [%s]\r\n", szInit);
    if (NULL == szInit || '\0' == szInit[0])
    {
        bRetVal = TRUE;
        CSystemManager::GetInstance().TriggerInitStringCompleteEvent(m_uiRilChannel, eInitIndex);
        goto Done;
    }

    // Now go through the string and break it up into individual commands separated by a '|'
    char  szCmd[MAX_BUFFER_SIZE];
    char* pszStart;
    char* pszEnd;
    pszStart = szInit;
    for (;;)
    {
        // Look for the end of the current command
        pszEnd = strchr(pszStart, '|');
        if (pszEnd)
        {
            // If we found a termination char, terminate the command there
            *pszEnd = '\0';
            bLastCmd = FALSE;
        }
        else
        {
            bLastCmd = TRUE;
        }

        if ('\0' != pszStart[0])
        {
            // Send the command
            if (!PrintStringNullTerminate(szCmd, MAX_BUFFER_SIZE, "AT%s\r", pszStart))
            {
                RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() - Could not"
                        " make command.\r\n");
                goto Done;
            }
            pCmd = new CCommand(m_uiRilChannel,
                                0,
                                REQ_ID_NONE,
                                szCmd);

            if (pCmd)
            {
                CContext* pContext = new CContextInitString(eInitIndex, m_uiRilChannel, bLastCmd);
                pCmd->SetContext(pContext);
                pCmd->SetTimeout(CTE::GetTE().GetTimeoutCmdInit());
                pCmd->SetHighPriority();
                pCmd->SetInitCommand();
            }

            if (!pCmd || !CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() - Could not queue"
                        " command.\r\n");
                goto Done;
            }
        }
        else if (bLastCmd)
        {
            // If last command is empty, trigger the complete event here
            CSystemManager::GetInstance().TriggerInitStringCompleteEvent(m_uiRilChannel,
                    eInitIndex);
        }

        // If this was the last command, get out now
        if (!pszEnd)
        {
            break;
        }

        // Get the next command
        pszStart = pszEnd+1;
    }

    bRetVal = TRUE;

Done:
    if (!bRetVal)
    {
        char szChannel[MAX_STRING_SIZE_FOR_INT] = { '\0' };
        snprintf(szChannel, sizeof(szChannel), "%u", m_uiRilChannel);

        RIL_LOG_CRITICAL("CChannelBase::SendModemConfigurationCommands() - "
                "chnl=[%d] Cannot send channel init cmds."
                "  Request modem restart !\r\n", m_uiRilChannel);

        // Couldn't send an init string -- trigger radio error
        DO_REQUEST_CLEAN_UP(3, "Could not send init string", "", szChannel);
    }

    delete[] szInit;
    szInit = NULL;
    delete pCmd;
    pCmd = NULL;
    RIL_LOG_VERBOSE("CChannelBase::SendModemConfigurationCommands() - Exit\r\n");

    return bRetVal;
}




UINT32 CChannelBase::ResponseThread()
{
    RIL_LOG_VERBOSE("CChannelBase::ResponseThread() chnl=[%d] - Enter\r\n", m_uiRilChannel);
    const UINT32 uiRespDataBufSize = 1024;
    char         szData[uiRespDataBufSize];
    UINT32       uiRead;
    UINT32       uiNumEvents;
    UINT32       uiReadError = 0;
    const UINT32 MAX_READERROR = 3;

    CThreadManager::RegisterThread();

    Sleep(200);

    while (TRUE)
    {
        // If the error count is accumulated
        // to 3, we trigger a radio error and request for recovery.
        if (uiReadError >= MAX_READERROR)
        {
            RIL_LOG_CRITICAL("CChannelBase::ResponseThread() - chnl=[%d] uiReadError > = %d!"
                    " Trigger radio error!\r\n", m_uiRilChannel, MAX_READERROR);
            DO_REQUEST_CLEAN_UP(); // Reason saved in WaitForAvailableData

            // the modem is down and we're switching off, so no need to hang around
            // listening on the COM port, bail out now
            break;
        }

        // Wait until Read Thread get unblocked (if blocked)
        if (m_bReadThreadBlocked)
        {
            CEvent* pEvents[] = { m_pBlockReadThreadEvent };
            uiNumEvents = 1;
            CEvent::WaitForAnyEvent(uiNumEvents, pEvents, WAIT_FOREVER);
        }

        // Wait for more data
        RIL_LOG_VERBOSE("CChannelBase::ResponseThread() chnl=[%d] - Waiting for data\r\n",
                m_uiRilChannel);
        if (!WaitForAvailableData(WAIT_FOREVER))
        {
            if (CTE::GetTE().IsPlatformShutDownRequested()
                    || CTE::GetTE().GetSpoofCommandsStatus())
            {
                // If we are in platform shutdown or spoof mode, don't report error in this case.
                // Simply end the thread.
                return 0;
            }

            RIL_LOG_CRITICAL("CChannelBase::ResponseThread() chnl=[%d] -"
                    " Waiting for data failed!\r\n", m_uiRilChannel);
            CMutex::Lock(m_pPossibleInvalidFDMutex);
            BOOL bPossibleInvalidFD = m_bPossibleInvalidFD;
            CMutex::Unlock(m_pPossibleInvalidFDMutex);
            if (!bPossibleInvalidFD)
            {
                ++uiReadError;

                // if the port is not open, request for recovery
                if (!IsPortOpen())
                {
                    RIL_LOG_CRITICAL("CChannelBase::ResponseThread() chnl=[%d] - Port closed,"
                            " requesting cleanup\r\n", m_uiRilChannel);
                    DO_REQUEST_CLEAN_UP(1, "Port closed");
                    break;
                }
            }
            else
            {
                //  Wait small amount of time for port to be open
                Sleep(50);
            }
            continue;
        }
        RIL_LOG_VERBOSE("CChannelBase::ResponseThread() chnl=[%d] - Data received\r\n",
                m_uiRilChannel);

        BOOL bFirstRead = TRUE;
        do
        {
            if (!ReadFromPort(szData, uiRespDataBufSize, uiRead))
            {
                if (CTE::GetTE().GetSpoofCommandsStatus())
                {
                    // If we are in spoof mode, this means that the modem is not ready.
                    //  Don't report error in this case. Simply end the thread.
                    return 0;
                }

                RIL_LOG_CRITICAL("CChannelBase::ResponseThread() chnl=[%d] -"
                        "Read failed\r\n", m_uiRilChannel);

                if (m_bPossibleInvalidFD)
                {
                    //  We could be closing and opening the DLC port. (For AT timeout case)
                    RIL_LOG_CRITICAL("CChannelBase::ResponseThread() chnl=[%d] - "
                            "m_bPossibleInvalidFD = TRUE\r\n");
                    Sleep(50);
                    break;
                }
                else
                {
                    // read() < 0, call DO_REQUEST_CLEAN_UP()
                    DO_REQUEST_CLEAN_UP(); // Reason saved in 'ReadFromPort'
                    //  exit thread
                    return 0;
                }
            }

            if (!uiRead)
            {
                if (bFirstRead)
                {
                    if (CTE::GetTE().GetSpoofCommandsStatus())
                    {
                        // If we are in "spoof" mode this means that a call to DO_REQUEST_CLEAN_UP()
                        // was done. In this case, we must exit the thread to end the RRIL.
                        return 0;
                    }

                    RIL_LOG_CRITICAL("CChannelBase::ResponseThread() chnl=[%d] -"
                            "Data available but uiRead is 0!\r\n", m_uiRilChannel);
                    Sleep(100);
                }
                break;
            }
            else
            {
                uiReadError = 0;
            }

            // If the thread is blocked don't take into account the data
            // This can occur if the thread was blocked when we are running WaitForAvailableData
            if (m_bReadThreadBlocked)
            {
                break;
            }

            if (!ProcessModemData(szData, uiRead))
            {
                RIL_LOG_CRITICAL("CChannelBase::ResponseThread() - chnl=[%d] ProcessModemData"
                        " failed?!\r\n", m_uiRilChannel);
                break;
            }

            bFirstRead = FALSE;
            // Loop until there is nothing left in the buffer to read
        } while (uiRead != 0);
    }

Done:
    RIL_LOG_INFO("CChannelBase::ResponseThread() chnl=[%d] - Exit\r\n", m_uiRilChannel);
    return 0;
}


//
//  Set the flags to lock the command queue for uiTimeout ms.
//  returns FALSE if queue is already locked.
// TODO: CHECK IF WE NEED THIS
/*
BOOL CChannelBase::LockCommandQueue(UINT32 uiTimeout)
{
    if (m_uiLockCommandQueue)
    {
        //  Already being locked.
        return FALSE;
    }
    RIL_LOG_INFO("CChannelBase::LockCommandQueue() : chnl=[%d] Locking command queue"
            "uiTimeout=[%ld]\r\n", m_uiRilChannel, uiTimeout);
    m_uiLockCommandQueueTimeout = uiTimeout;
    m_uiLockCommandQueue = GetTickCount();

    return TRUE;
}
*/
//
//  Iterate through each silo in this channel to ParseNotification.
//
BOOL CChannelBase::ParseUnsolicitedResponse(CResponse* const pResponse,
                                               const char*& rszPointer,
                                               BOOL& fGotoError)
{
    //RIL_LOG_VERBOSE("CChannelBase::ParseUnsolicitedResponse() - Enter\r\n");
    BOOL bResult = TRUE;
    int count = m_SiloContainer.nSilos;

    for (int i = 0; i < count; ++i)
    {
        CSilo* pSilo = NULL;
        pSilo = m_SiloContainer.rgpSilos[i];

        if (pSilo)
        {
            if (pSilo->ParseUnsolicitedResponse(pResponse, rszPointer, fGotoError))
            {
                //  we're done.
                goto Done;
            }
            else if (fGotoError)
            {
                //  done, but need to goto error.
                break;
            }
        }
    }

    bResult = FALSE;
Done:
    return bResult;
}

BOOL CChannelBase::InitPort()
{
    return m_Port.Init();
}

BOOL CChannelBase::ClosePort()
{
    return m_Port.Close();
}

BOOL CChannelBase::WriteToPort(const char* pData, UINT32 uiBytesToWrite, UINT32& ruiBytesWritten)
{
    RIL_LOG_INFO("CChannelBase::WriteToPort() - INFO: chnl=[%d] TX [%s]\r\n",
                       m_uiRilChannel,
                       CRLFExpandedString(pData,uiBytesToWrite).GetString());

    return m_Port.Write(pData, uiBytesToWrite, ruiBytesWritten);
}

BOOL CChannelBase::ReadFromPort(char* pszReadBuf, UINT32 uiReadBufSize, UINT32& ruiBytesRead)
{
    return m_Port.Read(pszReadBuf, uiReadBufSize, ruiBytesRead);
}

BOOL CChannelBase::IsPortOpen()
{
    return m_Port.IsOpen();
}

BOOL CChannelBase::WaitForAvailableData(UINT32 uiTimeout)
{
    return m_Port.WaitForAvailableData(uiTimeout);
}

BOOL CChannelBase::BlockReadThread()
{
    m_bReadThreadBlocked = TRUE;
    return CEvent::Reset(m_pBlockReadThreadEvent);
}

BOOL CChannelBase::UnblockReadThread()
{
    m_bReadThreadBlocked = FALSE;
    return CEvent::Signal(m_pBlockReadThreadEvent);
}
