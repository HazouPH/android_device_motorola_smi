////////////////////////////////////////////////////////////////////////////
// thread_manager.cpp
//
// Copyright 2005-2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Manages starting and termination of threads for all command queues
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "rilchannels.h"
#include "channel_nd.h"
#include "thread_manager.h"
#include "te.h"

CThreadManager* CThreadManager::m_pInstance = NULL;

BOOL CThreadManager::Start(UINT32 nChannels)
{
    BOOL fRet = FALSE;

    RIL_LOG_VERBOSE("CThreadManager::Start() - Enter\r\n");
    if (m_pInstance != NULL)
    {
        RIL_LOG_CRITICAL("CThreadManager::Start() - Called when already started\r\n");
    }
    else
    {
        m_pInstance = new CThreadManager(nChannels);
        if (NULL != m_pInstance)
        {
            fRet = m_pInstance->Initialize();
        }
    }

    RIL_LOG_VERBOSE("CThreadManager::Start() - Exit\r\n");
    return fRet;
}

BOOL CThreadManager::Stop()
{
    BOOL fRet = FALSE;
    RIL_LOG_VERBOSE("CThreadManager::Stop() - Enter\r\n");
    if (m_pInstance != NULL)
    {
        fRet = m_pInstance->StopThreads();
        delete m_pInstance;
        m_pInstance = NULL;
    }
    else
    {
        RIL_LOG_CRITICAL("CThreadManager::Stop() - Called when not started\r\n");
    }

    RIL_LOG_VERBOSE("CThreadManager::Stop() - Exit\r\n");
    return fRet;
}

CThreadManager::CThreadManager(UINT32 nChannels) :
    m_nChannelsTotal(nChannels),
    m_nChannelsActive(0),
    m_pStartupCompleteEvent(NULL)
{
    RIL_LOG_VERBOSE("CThreadManager::CThreadManager() - Enter\r\n");

    m_pTManMutex = new CMutex();

    RIL_LOG_VERBOSE("CThreadManager::CThreadManager() - Exit\r\n");
}

CThreadManager::~CThreadManager()
{
    RIL_LOG_VERBOSE("CThreadManager::~CThreadManager() - Enter\r\n");

    // Delete critical section
    delete m_pTManMutex;
    m_pTManMutex = NULL;

    // NOTE We do not free the pStopThreadsEvent as it isn't owned by CThreadManager

    RIL_LOG_VERBOSE("CThreadManager::~CThreadManager() - Exit\r\n");
}

BOOL CThreadManager::Initialize()
{
    RIL_LOG_VERBOSE("CThreadManager::Initialize() - Enter\r\n");
    BOOL fRet = FALSE;

    m_pStartupCompleteEvent = new CEvent(NULL, TRUE);

    if (NULL == m_pStartupCompleteEvent)
    {
        RIL_LOG_CRITICAL("CThreadManager::Initialize() : Unable to create complete event\r\n");
        goto Error;
    }

    if (!StartChannelThreads())
    {
        RIL_LOG_CRITICAL("CThreadManager::Initialize() : Failed to launch channel threads\r\n");
        goto Error;
    }

    if (WAIT_EVENT_0_SIGNALED !=
            CEvent::Wait(m_pStartupCompleteEvent, CTE::GetTE().GetTimeoutWaitForInit()))
    {
        RIL_LOG_CRITICAL("CThreadManager::Initialize() : Timed out waiting for threads to"
                " register\r\n");
        goto Error;
    }

    fRet = TRUE;

Error:
    if (m_pStartupCompleteEvent)
    {
        delete m_pStartupCompleteEvent;
        m_pStartupCompleteEvent = NULL;
    }

    RIL_LOG_INFO("CThreadManager::Initialize() : Result: %s\r\n", fRet ? "SUCCESS" : "FAIL");
    RIL_LOG_VERBOSE("CThreadManager::Initialize() - Exit\r\n");
    return fRet;
}

BOOL CThreadManager::StopThreads()
{
    RIL_LOG_VERBOSE("CThreadManager::StopThreads() - Enter\r\n");
    BOOL fRet = TRUE;
    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];

    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (g_pRilChannel[i])
        {
            if (!g_pRilChannel[i]->StopChannelThreads())
            {
                fRet = FALSE;
            }
        }
    }


    RIL_LOG_VERBOSE("CThreadManager::StopThreads() - Exit\r\n");
    return fRet;
}

void CThreadManager::RegisterThread()
{
    RIL_LOG_VERBOSE("CThreadManager::RegisterThread() - Enter\r\n");
    BOOL fRet = TRUE;

    if (m_pInstance != NULL)
    {
        m_pInstance->Register();
    }
    else
    {
        RIL_LOG_CRITICAL("CThreadManager::RegisterThread() called with no instance!\r\n");
    }

    RIL_LOG_VERBOSE("CThreadManager::RegisterThread() - Exit\r\n");
}

void CThreadManager::Register()
{
    RIL_LOG_VERBOSE("CThreadManager::Register() - Enter\r\n");

    CMutex::Lock(m_pTManMutex);

    ++m_nChannelsActive;
    RIL_LOG_INFO("CThreadManager::Register() : %d of %d threads registered\r\n",
            m_nChannelsActive, m_nChannelsTotal);

    if (m_nChannelsActive == m_nChannelsTotal)
    {
        RIL_LOG_INFO("CThreadManager::Register() : Channel threads are running."
                " Setting event.\r\n");
        SignalComplete();
    }

    CMutex::Unlock(m_pTManMutex);
    RIL_LOG_VERBOSE("CThreadManager::Register() - Exit\r\n");

}

void CThreadManager::SignalComplete()
{
    RIL_LOG_VERBOSE("CThreadManager::SignalComplete() - Enter\r\n");

    CEvent::Signal(m_pStartupCompleteEvent);

    RIL_LOG_VERBOSE("CThreadManager::SignalComplete() - Exit\r\n");
}

BOOL CThreadManager::StartChannelThreads()
{
    RIL_LOG_VERBOSE("CThreadManager::StartChannelThreads() - Enter\r\n");
    BOOL fRet = FALSE;
    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];

    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; ++i)
    {
        if (g_pRilChannel[i])
        {
            if (!g_pRilChannel[i]->StartChannelThreads())
            {
                goto Error;
            }
        }
    }

    fRet = TRUE;

Error:
    RIL_LOG_VERBOSE("CThreadManager::StartChannelThreads() - Exit\r\n");
    return fRet;
}
