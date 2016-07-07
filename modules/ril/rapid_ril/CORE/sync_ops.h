////////////////////////////////////////////////////////////////////////////
// sync_ops.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//  Syncronization interfaces
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_SYNC_OPS_H
#define RRIL_SYNC_OPS_H

#include "types.h"

#include <pthread.h>
#include <utils/List.h>
using namespace android;

const UINT32 WAIT_EVENT_0_SIGNALED = 0;
const UINT32 WAIT_TIMEDOUT = 0xFFFFFFFF;
const UINT32 WAIT_OBJ_NULL = 0xFFFF0001;


class CMutex
{
    public:
        CMutex();
        virtual ~CMutex();

        static void Lock(CMutex* pMutex);
        static BOOL TryLock(CMutex* pMutex);
        static void Unlock(CMutex* pMutex);

    protected:
        BOOL EnterMutex(BOOL fTryLock = FALSE);
        void LeaveMutex();

        pthread_mutex_t m_mutex;
};


class CEvent;

// One MultipleEvent for each call to WaitForAnyEvent. It contains pointers to
// all associated events. It creates observers for each event when they are added
// and deletes them when removed. Will get signaled when any of the events watched
// are signaled.
class CMultipleEvent : public CMutex
{
    public:
        CMultipleEvent(UINT32 nEvents);
        ~CMultipleEvent();

    private:
        //  Prevent assignment: Declared but not implemented.
        CMultipleEvent(const CMultipleEvent& rhs);  // Copy Constructor
        CMultipleEvent& operator=(const CMultipleEvent& rhs);  //  Assignment operator

    public:
        // Sets last signaled event and the multievent condition
        BOOL Update(int iEventIndex);

        // Attach event with associated new Observer to the ME
        void AddEvent(int iEventIndex , CEvent* pEvent);

        // Detach event at iEventIndex and delete Observer
        void RemoveEvent(int iEventIndex);

        int Wait(UINT32 uiTimeoutInMS = 0);

    protected:
        pthread_cond_t m_MultipleEventCond;
        CEvent**       m_pEvents;
        UINT32           m_nEvents;

    private:
        volatile int   m_nLastSignaledEvent;
        BOOL           m_fSignaled;
};

// Used by MultipleEvent objects to link together multiple events.
// These are owned by the associated event. An observer is only associated
// to one MultipleEvent.
class CObserver
{
    public:
        CObserver(CMultipleEvent* pMultipleEvent, int iEventIndex)
        {
            m_iEventIndex = iEventIndex;
            m_pMultipleEvent = pMultipleEvent;
            m_pNext = m_pPrev = NULL;
        }

    public:
        CObserver*     m_pNext;
        CObserver*     m_pPrev;

        CMultipleEvent* m_pMultipleEvent;
        int            m_iEventIndex;
};

typedef List<CObserver*> mapObserver;
typedef mapObserver::iterator mapObserverIterator;



// Each event can have many observers, one for each time someone includes it
// in a WaitForAnyEvent call parameter list. The event is also registered with
// a MultipleEvent object if used in a WaitForAnyEvent call. Otherwise it operates
// on it's own with no observer or associated MultipleEvent object.
class CEvent
 : public CMutex
{
    public:

        CEvent(const char* szName = NULL, BOOL fManual = FALSE, BOOL fInitial = FALSE);
        ~CEvent();

        static BOOL Signal(CEvent* pEvent);
        static BOOL Reset(CEvent* pEvent);
        static UINT32 Wait(CEvent* pEvent, UINT32 uiTimeoutInMS);

        static UINT32 WaitForAnyEvent(UINT32 nEvents, CEvent** rgpEvents, UINT32 uiTimeoutInMS);
        static UINT32 WaitForAllEvents(UINT32 nEvents, CEvent** rgpEvents, UINT32 uiTimeoutInMS);



    private:
        BOOL Reset();
        BOOL Signal();
        int Wait(UINT32 uiTimeoutInMS = 0);

        void CreateObserver(CMultipleEvent* pMultiEvent, int iValue);
        void DeleteObserver(CMultipleEvent*);

        friend void CMultipleEvent::AddEvent(int iEventIndex , CEvent* pEvent);
        friend void CMultipleEvent::RemoveEvent(int iEventIndex);

        char*           m_szName;
        BOOL            m_fManual;
        mapObserver     m_mObservers;

        BOOL SignalMultipleEventObject();

    protected:
        pthread_cond_t m_EventCond;
        BOOL           m_fSignaled;


};

#endif
