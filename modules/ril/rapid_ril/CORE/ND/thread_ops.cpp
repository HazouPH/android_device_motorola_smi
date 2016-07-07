////////////////////////////////////////////////////////////////////////////
// thread_ops.cpp
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//  Thread handling interfaces
//
/////////////////////////////////////////////////////////////////////////////


#include <sched.h>
#include <errno.h>
#include <stdio.h>

#include "types.h"
#include "rillog.h"
#include "sync_ops.h"
#include "thread_ops.h"

typedef struct sThreadData
{
    void*          pvDataObj;
    THREAD_PROC_PTR pvThreadProc;
} THREAD_DATA;

typedef struct sThreadWaitData
{
    pthread_t   thread;
    CEvent*     pEvent;
} THREAD_WAIT_DATA;

void* ThreadProcStartStop(void* pVoid)
{
    void* pvRet = NULL;

    if (NULL == pVoid)
    {
        RIL_LOG_CRITICAL("ThreadProcStartStop() - pVoid was NULL!\r\n");
        goto Error;
    }

    {
        THREAD_DATA* pThreadData = (THREAD_DATA*)pVoid;

        THREAD_PROC_PTR pvThreadProc = pThreadData->pvThreadProc;
        void* pvDataObj = pThreadData->pvDataObj;

        delete pThreadData;
        pThreadData = NULL;

        pvRet = pvThreadProc(pvDataObj);
    }

Error:
    pthread_exit(pvRet);
    return NULL;
}

void* ThreadWaitProc(void* pVoid)
{
    void** ppvRetVal = NULL;

    if (NULL == pVoid)
    {
        RIL_LOG_CRITICAL("ThreadWaitProc() - pVoid was NULL!\r\n");
        return NULL;
    }

    THREAD_WAIT_DATA* pThreadWaitData = (THREAD_WAIT_DATA*)pVoid;

    pthread_t thread = pThreadWaitData->thread;
    CEvent* pEvent = pThreadWaitData->pEvent;

    delete pThreadWaitData;
    pThreadWaitData = NULL;

    int nRet = pthread_join(thread, ppvRetVal);

    if (0 == nRet)
    {
        CEvent::Signal(pEvent);
    }
    else if (ESRCH == nRet)
    {
        RIL_LOG_CRITICAL("ThreadWaitProc() - Given thread is not initialized!\r\n");
    }
    else if (EINVAL == nRet)
    {
        RIL_LOG_CRITICAL("ThreadWaitProc() - Given thread is not joinable!\r\n");
    }
    else if (EDEADLK == nRet)
    {
        RIL_LOG_CRITICAL("ThreadWaitProc() - Deadlock detected, pthread_join bailed out!\r\n");
    }
    else
    {
        RIL_LOG_CRITICAL("ThreadWaitProc() - Error code returned: 0x%X\r\n", nRet);
    }

    return NULL;
}

CThread::CThread(THREAD_PROC_PTR pvThreadProc, void* pvDataObj, UINT32 dwFlags, UINT32 dwStackSize) :
    m_pvDataObj(pvDataObj),
    m_uiPriority(THREAD_PRIORITY_LEVEL_UNKNOWN),
    m_fJoinable(FALSE),
    m_fRunning(FALSE),
    m_fInitialized(FALSE)
{
    if (dwFlags & THREAD_FLAGS_START_SUSPENDED)
    {
        RIL_LOG_WARNING("CThread::CThread() -"
                " WARNING: We don't support start from suspended at this time\r\n");
    }

    if (0 != dwStackSize)
    {
        RIL_LOG_WARNING("CThread::CThread() -"
                " WARNING: We don't support stack size parameter at this time\r\n");
    }

    int iResult = 0;

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    THREAD_DATA* pThreadData = new THREAD_DATA;
    pThreadData->pvDataObj = pvDataObj;
    pThreadData->pvThreadProc = pvThreadProc;

    if (THREAD_FLAGS_JOINABLE & dwFlags)
    {
        m_fJoinable = TRUE;
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    }
    else
    {
        m_fJoinable = FALSE;
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    iResult = pthread_create(&m_thread, &attr, ThreadProcStartStop, pThreadData);

    pthread_attr_destroy(&attr);

    if (iResult != 0)
    {
        errno = iResult;
        perror("pthread_create");

        RIL_LOG_CRITICAL("CThread::CThread() - Failed to create thread!\r\n");
    }
    else
    {
        m_fInitialized = TRUE;
        // TODO: temporary code to prevent race condition where
        // parent checks for running flag before is set in child
        m_fRunning = TRUE;

        if (!SetPriority(THREAD_PRIORITY_LEVEL_NORMAL))
        {
            //RIL_LOG_CRITICAL("CThread::CThread() -"
            //        "Failed to set priority to THREAD_PRIORITY_LEVEL_NORMAL\r\n");
        }
    }
}

CThread::~CThread()
{
    if (m_fRunning)
    {
        RIL_LOG_WARNING("CThread::~CThread() - WARNING: Thread process is still running!\r\n");
    }
}

void CThread::Kill()
{
    if (m_fRunning)
    {
        RIL_LOG_INFO("CThread::Kill() - Killing thread!\r\n");
        pthread_kill(m_thread, SIGTERM);
        m_fRunning = FALSE;
    }
    else
    {
        RIL_LOG_CRITICAL("CThread::Kill() - Unable to kill thread as it is not running!\r\n");
    }
}

BOOL CThread::SetPriority(UINT32 dwPriority)
{
    BOOL fRet = FALSE;
    int nRes = 0;

    if (m_fInitialized)
    {
        if (m_uiPriority == dwPriority)
        {
            fRet = TRUE;
        }
        else if (THREAD_PRIORITY_LEVEL_HIGHEST >= dwPriority)
        {
            struct sched_param sc;
            int MinPriority = sched_get_priority_min(SCHED_RR);
            int MaxPriority = sched_get_priority_max(SCHED_RR);
            int NormalPriority = (MaxPriority - MinPriority) / 2;
            int Delta = 2;

            switch (dwPriority)
            {
                case THREAD_PRIORITY_LEVEL_LOWEST:
                    sc.sched_priority = NormalPriority - (Delta*2);
                    break;

                case THREAD_PRIORITY_LEVEL_LOW:
                    sc.sched_priority = NormalPriority - (Delta*1);
                    break;

                case THREAD_PRIORITY_LEVEL_NORMAL:
                    sc.sched_priority = NormalPriority;
                    break;

                case THREAD_PRIORITY_LEVEL_HIGH:
                    sc.sched_priority = NormalPriority + (Delta*1);
                    break;

                case THREAD_PRIORITY_LEVEL_HIGHEST:
                    sc.sched_priority = NormalPriority + (Delta*2);
                    break;

                default:
                    sc.sched_priority = NormalPriority;
                    break;
            }

            nRes = pthread_setschedparam(m_thread, SCHED_RR, &sc);

            if (!nRes)
            {
                fRet = TRUE;
                m_uiPriority = dwPriority;
            }
            else
            {
                fRet = FALSE;
                //perror("pthread_setschedparam");
                //RIL_LOG_CRITICAL("CThread::SetPriority() -"
                //        "pthread_setschedparam returned failed response: %d\r\n", nRes);
                //RIL_LOG_CRITICAL("CThread::SetPriority() -"
                //        "errno=[%d],[%s]\r\n", errno, strerror(errno));
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CThread::SetPriority() - Given priority out of range: %d\r\n",
                    dwPriority);
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CThread::SetPriority() - Thread is not initialized!\r\n");
    }

    return fRet;
}

UINT32 CThread::GetPriority()
{
    return m_uiPriority;
}

UINT32 CThread::Wait(UINT32 dwTimeout)
{
    THREAD_WAIT_DATA* pThreadWaitData = NULL;
    UINT32 dwRet = THREAD_WAIT_GEN_FAILURE;

    if (!m_fJoinable)
    {
        RIL_LOG_CRITICAL("CThread::Wait() - Unable to wait for non-joinable thread!\r\n");
        return THREAD_WAIT_NOT_JOINABLE;
    }
    else if (!m_fInitialized)
    {
        RIL_LOG_CRITICAL("CThread::Wait() - Unable to wait for non-intialized thread!\r\n");
        return THREAD_WAIT_NOT_INITIALIZED;
    }

    int iResult = 0;
    pthread_t helperThread = 0;

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    CEvent* pEvent = new CEvent();
    if (NULL == pEvent)
    {
        RIL_LOG_CRITICAL("CThread::Wait() - Failed to create event!\r\n");
        goto Error;
    }

    pThreadWaitData = new THREAD_WAIT_DATA;
    pThreadWaitData->thread = m_thread;
    pThreadWaitData->pEvent = pEvent;

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    iResult = pthread_create(&helperThread, &attr, ThreadWaitProc, pThreadWaitData);

    pthread_attr_destroy(&attr);

    if (iResult != 0)
    {
        errno = iResult;
        perror("pthread_create");

        RIL_LOG_CRITICAL("CThread::Wait() - Failed to create thread!\r\n");
    }

    switch(CEvent::Wait(pEvent, dwTimeout))
    {
        case WAIT_EVENT_0_SIGNALED:
        {
            dwRet = THREAD_WAIT_0;
            break;
        }

        case WAIT_TIMEDOUT:
        default:
        {
            dwRet = THREAD_WAIT_TIMEOUT;

            // Kill our helper thread
            pthread_kill(helperThread, SIGTERM);
            break;
        }
    }

Error:
    delete pEvent;
    pEvent = NULL;
    return dwRet;
}

BOOL CThread::SetPriority(CThread* pThread, UINT32 dwPriority)
{
    BOOL fRet = FALSE;

    if (pThread)
    {
        fRet = pThread->SetPriority(dwPriority);
    }
    else
    {
        RIL_LOG_CRITICAL("CThread::SetPriority() - Unable to call as pThread was NULL\r\n");
    }

    return fRet;
}

UINT32 CThread::GetPriority(CThread* pThread)
{
    if (pThread)
    {
        return pThread->GetPriority();
    }
    else
    {
        RIL_LOG_CRITICAL("CThread::GetPriority() - Unable to call as pThread was NULL\r\n");
        return THREAD_PRIORITY_LEVEL_UNKNOWN;
    }
}

UINT32 CThread::Wait(CThread* pThread, UINT32 dwTimeoutInMS)
{
    if (pThread)
    {
        return pThread->Wait(dwTimeoutInMS);
    }
    else
    {
        RIL_LOG_CRITICAL("CThread::Wait() - Unable to call as pThread was NULL\r\n");
        return THREAD_WAIT_GEN_FAILURE;
    }
}

BOOL CThread::IsRunning(CThread* pThread)
{
    if (pThread)
    {
        return pThread->m_fRunning;
    }
    else
    {
        RIL_LOG_CRITICAL("CThread::IsRunning() - Unable to call as pThread was NULL\r\n");
        return FALSE;
    }
}

BOOL CThread::IsInitialized(CThread* pThread)
{
    if (pThread)
    {
        return pThread->m_fInitialized;
    }
    else
    {
        RIL_LOG_CRITICAL("CThread::IsInitialized() - Unable to call as pThread was NULL\r\n");
        return FALSE;
    }
}
