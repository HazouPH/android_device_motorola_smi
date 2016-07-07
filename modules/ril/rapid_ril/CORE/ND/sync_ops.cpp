////////////////////////////////////////////////////////////////////////////
// sync_ops.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//  Syncronization interfaces
//
/////////////////////////////////////////////////////////////////////////////

#include "sync_ops.h"
#include "rillog.h"
#include "rril.h"
#include "types.h"
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>

CMutex::CMutex()
{
    int rc = 0;

    pthread_mutexattr_t _attr;

    rc = pthread_mutexattr_init(&_attr);

    if (rc)
    {
        perror("CMutex::CMutex");

        switch (rc)
        {
            case ENOMEM:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutexattr_init ENOMEM code"
                        " returned\r\n");
                break;

            default:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutexattr_init [%d] code"
                        " returned\r\n", rc);
                break;
        }
    }

    rc = pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE_NP);

    if (rc)
    {
        perror("CMutex::CMutex");

        switch (rc)
        {
            case EINVAL:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutexattr_settype EINVAL code"
                        " returned\r\n");
                break;

            default:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutexattr_settype [%d] code"
                        " returned\r\n", rc);
                break;
        }
    }

    rc = pthread_mutex_init(&m_mutex, &_attr);

    if (rc)
    {
        perror("CMutex::CMutex");

        switch (rc)
        {
            case EAGAIN:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutex_init EAGAIN code returned\r\n");
                break;

            case EINVAL:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutex_init EINVAL code returned\r\n");
                break;

            case ENOMEM:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutex_init ENOMEM code returned\r\n");
                break;

            case EPERM:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutex_init EPERM code returned\r\n");
                break;

            case EBUSY:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutex_init EBUSY code returned\r\n");
                break;

            default:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutex_init [%d] code returned\r\n",
                        rc);
                break;
        }
    }

    rc = pthread_mutexattr_destroy(&_attr);

    if (rc)
    {
        perror("CMutex::CMutex");

        switch (rc)
        {
            case EINVAL:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutexattr_destroy EINVAL code"
                        " returned\r\n");
                break;

            default:
                RIL_LOG_CRITICAL("CMutex::CMutex() - pthread_mutexattr_destroy [%d] code returned"
                        "\r\n", rc);
                break;
        }
    }
}

CMutex::~CMutex()
{
    int rc = pthread_mutex_destroy(&m_mutex);

    if (rc)
    {
        perror("CMutex::~CMutex");

        switch (rc)
        {
            case EINVAL:
                RIL_LOG_CRITICAL("CMutex::~CMutex() - pthread_mutex_destroy EINVAL code"
                        " returned\r\n");
                break;

            case EBUSY:
                RIL_LOG_CRITICAL("CMutex::~CMutex() - pthread_mutex_destroy EBUSY code"
                        " returned\r\n");
                break;

            default:
                RIL_LOG_CRITICAL("CMutex::~CMutex() - pthread_mutex_destroy [%d] code"
                        " returned\r\n", rc);
                break;
        }
    }
}

BOOL CMutex::EnterMutex(BOOL fTryLock)
{
    int rc = 1;
    BOOL fRet = FALSE;

    if (fTryLock)
    {
        rc = pthread_mutex_trylock(&m_mutex);
    }
    else
    {
        rc = pthread_mutex_lock(&m_mutex);
    }

    if (rc)
    {
        perror("CMutex::EnterMutex");

        switch (rc)
        {
            case EINVAL:
                RIL_LOG_CRITICAL("CMutex::EnterMutex() - EINVAL code returned\r\n");
                break;

            case EBUSY:
                RIL_LOG_CRITICAL("CMutex::EnterMutex() - EBUSY code returned\r\n");
                break;

            default:
                RIL_LOG_CRITICAL("CMutex::EnterMutex() - [%d] code returned\r\n", rc);
                break;
        }
    }
    else
    {
        fRet = TRUE;
    }

    return fRet;
}

void CMutex::LeaveMutex()
{
    int rc = pthread_mutex_unlock(&m_mutex);

    if (rc)
    {
        perror("CMutex::LeaveMutex");

        switch (rc)
        {
            case EINVAL:
                RIL_LOG_CRITICAL("CMutex::LeaveMutex() - EINVAL code returned\r\n");
                break;

            case EAGAIN:
                RIL_LOG_CRITICAL("CMutex::LeaveMutex() - EAGAIN code returned\r\n");
                break;

            case EPERM:
                RIL_LOG_CRITICAL("CMutex::LeaveMutex() - EPERM code returned\r\n");
                break;

            default:
                RIL_LOG_CRITICAL("CMutex::LeaveMutex() - [%d] code returned\r\n", rc);
                break;
        }
    }
}

void CMutex::Lock(CMutex* pMutex)
{
    if (pMutex)
    {
        pMutex->EnterMutex();
    }
    else
    {
        RIL_LOG_CRITICAL("CMutex::Lock() : Unable to lock mutex as it is NULL!\r\n");
    }
}

BOOL CMutex::TryLock(CMutex* pMutex)
{
    if (pMutex)
    {
        return pMutex->EnterMutex(TRUE);
    }
    else
    {
        RIL_LOG_CRITICAL("CMutex::TryLock() : Unable to lock mutex as it is NULL!\r\n");
    }

    return FALSE;
}

void CMutex::Unlock(CMutex* pMutex)
{
    if (pMutex)
    {
        pMutex->LeaveMutex();
    }
    else
    {
        RIL_LOG_CRITICAL("CMutex::Unlock() : Unable to unlock mutex as it is NULL!\r\n");
    }
}

// Use to convert our timeout to a timespec in the future
timespec msFromNowToTimespec(UINT32 msInFuture)
{
    timespec FutureTime;
    timeval Now;
    UINT32 usFromNow;

    gettimeofday(&Now, NULL);

    usFromNow = ((msInFuture % 1000) * 1000) + Now.tv_usec;

    if (usFromNow >= 1000000 )
    {
        Now.tv_sec++;
        FutureTime.tv_sec = Now.tv_sec + (msInFuture / 1000);
        FutureTime.tv_nsec = (usFromNow - 1000000) * 1000;
    }
    else
    {
        FutureTime.tv_nsec = usFromNow * 1000;
        FutureTime.tv_sec = Now.tv_sec + (msInFuture / 1000);
    }

    return FutureTime;
}

CEvent::CEvent(const char* szName, BOOL fManual, BOOL fInitial) : CMutex()
{
    EnterMutex();

    pthread_cond_init(&m_EventCond, NULL);

    m_fSignaled = fInitial;
    m_fManual   = fManual;
    m_szName    = NULL;

    if (szName)
    {
        // TODO Add support for named events if required
        m_szName = strdup(szName);
    }

    LeaveMutex();
}

CEvent::~CEvent()
{
    pthread_cond_destroy(&m_EventCond);
    if (m_szName)
    {
        free((void*)m_szName);
        m_szName = NULL;
    }
}

void CEvent::CreateObserver(CMultipleEvent* pMultipleEvent, int iValue)
{
    EnterMutex();

    CObserver* pNewObserver = new CObserver(pMultipleEvent, iValue);
    m_mObservers.push_front(pNewObserver);

    LeaveMutex();
}

void CEvent::DeleteObserver(CMultipleEvent* pMultipleEvent)
{
    EnterMutex();

    for (mapObserverIterator it = m_mObservers.begin(); it != m_mObservers.end(); it++)
    {
        CObserver* pObserver = *it;
        if (pObserver && (pObserver->m_pMultipleEvent == pMultipleEvent))
        {
            m_mObservers.erase(it);
            delete pObserver;
            pObserver = NULL;
            break;
        }
    }

    LeaveMutex();
}

BOOL CEvent::SignalMultipleEventObject()
{
    BOOL fEventHandled = FALSE;

    for (mapObserverIterator it = m_mObservers.begin(); it != m_mObservers.end(); it++)
    {
        CObserver* pObserver = *it;
        if (pObserver)
        {
            int iEventIndex = pObserver->m_iEventIndex;
            CMultipleEvent* pMultipleEvent = pObserver->m_pMultipleEvent;

            fEventHandled = pMultipleEvent->Update(iEventIndex);
        }
    }

    return fEventHandled;
}

BOOL CEvent::Signal(void)
{
    EnterMutex();

    m_fSignaled = TRUE;

    pthread_cond_broadcast(&m_EventCond);

    BOOL fHandled = SignalMultipleEventObject();

    LeaveMutex();

    return TRUE;
}

BOOL CEvent::Reset()
{
    EnterMutex();

    m_fSignaled = FALSE;

    LeaveMutex();

    return TRUE;
};

int CEvent::Wait(UINT32 uiTimeout)
{
    int rc = WAIT_EVENT_0_SIGNALED;
    struct timespec Time;

    do
    {
        EnterMutex();

        if (m_fSignaled)
        {
            // No need to wait
            break;
        }

        if (uiTimeout == WAIT_FOREVER)
        {
            pthread_cond_wait(&m_EventCond, &m_mutex);
        }
        else
        {
            Time = msFromNowToTimespec(uiTimeout);

            rc = pthread_cond_timedwait(&m_EventCond, &m_mutex, &Time);

            if (rc != 0)
            {
                if (rc != ETIMEDOUT)
                {
                    RIL_LOG_CRITICAL("CEvent::Wait() : pthread_cond_timedwait(): returned %d\r\n",
                            rc);
                }
            }
        }
    }
    while (0);

    if (!m_fManual)
    {
        m_fSignaled = FALSE;
    }

    LeaveMutex();

    if (rc == ETIMEDOUT)
    {
        rc = WAIT_TIMEDOUT;
    }

    if (rc == 0)
    {
        rc = WAIT_EVENT_0_SIGNALED;
    }

    return rc;
}

BOOL CEvent::Signal(CEvent* pEvent)
{
    if (pEvent)
    {
        return pEvent->Signal();
    }
    else
    {
        RIL_LOG_CRITICAL("CEvent::Signal() : Unable to signal event as pointer was NULL!\r\n");
        return FALSE;
    }
}

BOOL CEvent::Reset(CEvent* pEvent)
{
    if (pEvent)
    {
        return pEvent->Reset();
    }
    else
    {
        RIL_LOG_CRITICAL("CEvent::Reset() : Unable to reset event as pointer was NULL!\r\n");
        return FALSE;
    }
}

UINT32 CEvent::Wait(CEvent* pEvent, UINT32 uiTimeoutInMS)
{
    if (pEvent)
    {
        return pEvent->Wait(uiTimeoutInMS);
    }
    else
    {
        RIL_LOG_CRITICAL("CEvent::Wait() : Unable to wait on event as pointer was NULL!\r\n");
        return WAIT_OBJ_NULL;
    }
}

UINT32 CEvent::WaitForAnyEvent(UINT32 nEvents, CEvent** rgpEvents, UINT32 uiTimeoutInMS)
{
    CMultipleEvent* pMultipleEvents = new CMultipleEvent(nEvents);
    UINT32 uiRet = WAIT_TIMEDOUT;

    // load the events into the MultipleEvent object
    for (UINT32 index = 0; index < nEvents; index++)
    {
        CEvent* pEvent = (CEvent*)rgpEvents[index];

        if (pEvent != NULL)
        {
            // synchronize with CEvent::Signal()
            pEvent->EnterMutex();

            pMultipleEvents->AddEvent(index, pEvent);

            if(pEvent->m_fSignaled)
            {
                pMultipleEvents->Update(index);
            }

            pEvent->LeaveMutex();
        }
        else
        {
            RIL_LOG_CRITICAL("CEvent::WaitForAnyEvent() : Item %d was NULL\r\n", index);
        }
    }

    uiRet = pMultipleEvents->Wait(uiTimeoutInMS);

    // Reset non-manual reset event after a successful wait, if any.
    if(uiRet < nEvents && !rgpEvents[uiRet]->m_fManual)
    {
        Reset(rgpEvents[uiRet]);
    }

    for (UINT32 index = 0; index < nEvents; index++)
    {
        CEvent* pEvent = (CEvent*)rgpEvents[index];

        if (pEvent != NULL)
        {
            pEvent->EnterMutex();
            pMultipleEvents->RemoveEvent(index);
            pEvent->LeaveMutex();
        }
    }

    delete pMultipleEvents;
    pMultipleEvents = NULL;

    return uiRet;
}

UINT32 CEvent::WaitForAllEvents(UINT32 /*nEvents*/,
                                        CEvent** /*rgpEvents*/,
                                        UINT32 /*uiTimeoutInMS*/)
{
    // FIXME Not currently supported... or needed?
    return WAIT_TIMEDOUT;
}

CMultipleEvent::CMultipleEvent(UINT32 nEvents)
{
    pthread_cond_init(&m_MultipleEventCond, NULL);
    m_nLastSignaledEvent = -1;
    m_fSignaled = FALSE;
    m_nEvents = nEvents;

    m_pEvents = new CEvent*[nEvents] ;
    if (m_pEvents != NULL)
    {
        for (UINT32 ii = 0; ii < nEvents; ii++)
            m_pEvents[ii] = NULL;
    }
};

CMultipleEvent::~CMultipleEvent()
{
    EnterMutex();
    {
        pthread_cond_destroy(&m_MultipleEventCond);
        delete [] m_pEvents;
        m_pEvents = NULL;
    }
    LeaveMutex();
};

void CMultipleEvent::AddEvent(int iEventIndex, CEvent* pEvent)
{
    assert((iEventIndex >= 0) && (iEventIndex < m_nEvents));

    EnterMutex();
    {
        if (m_pEvents[iEventIndex])
        {
            m_pEvents[iEventIndex]->DeleteObserver(this);
        }

        m_pEvents[iEventIndex] = pEvent;

        if (pEvent)
        {
            pEvent->CreateObserver(this, iEventIndex);
        }
    }

    LeaveMutex();
}

void CMultipleEvent::RemoveEvent(int iEventIndex)
{
    assert((iEventIndex >= 0) && (iEventIndex < m_nEvents));

    EnterMutex();
    {
        if (m_pEvents[iEventIndex])
            m_pEvents[iEventIndex]->DeleteObserver(this);

        m_pEvents[iEventIndex] = NULL;
    }

    LeaveMutex();
}

BOOL CMultipleEvent::Update(int iEventIndex)
{
    BOOL fHandled = FALSE;

    // synchronize with the wait() function to make update() and wait()
    // both atomic operations
    EnterMutex();

    if (!m_fSignaled)
    {
        m_nLastSignaledEvent = iEventIndex;
        m_fSignaled = TRUE;
        fHandled = TRUE;

        pthread_cond_signal(&m_MultipleEventCond);
    }

    LeaveMutex();
    return fHandled;
}

int CMultipleEvent::Wait(UINT32 uiTimeout)
{
    int rc = WAIT_TIMEDOUT;
    struct timespec Time;

    EnterMutex();

    if (m_fSignaled)
    {
        rc = m_nLastSignaledEvent;
    }
    else if (uiTimeout == WAIT_FOREVER)
    {
        pthread_cond_wait(&m_MultipleEventCond, &m_mutex);
        rc = m_nLastSignaledEvent;
    }
    else
    {
        Time = msFromNowToTimespec(uiTimeout);
        rc = pthread_cond_timedwait(&m_MultipleEventCond, &m_mutex, &Time);

        if (rc == 0)
        {
            rc = m_nLastSignaledEvent;
        }
        else
        {
            if (rc != ETIMEDOUT)
            {
                RIL_LOG_CRITICAL("CMultipleEvent::Wait() : pthread_cond_timedwait():"
                        " returned %d\r\n", rc);

                // TODO Deal with these errors
                rc = ETIMEDOUT;

            }
        }
    }

    // CMultipleEvent is always auto-reset after wait()
    m_fSignaled = FALSE;

    if (rc == ETIMEDOUT)
    {
        rc = WAIT_TIMEDOUT;
    }
    LeaveMutex();

    return rc;
}
