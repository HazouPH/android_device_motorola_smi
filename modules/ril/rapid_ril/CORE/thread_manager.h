////////////////////////////////////////////////////////////////////////////
// thread_manager.h
//
// Copyright 2005-2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Manages starting and termination of threads for all command queues
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_THREAD_MANAGER_H
#define RRIL_THREAD_MANAGER_H

#include "rril.h"
#include "sync_ops.h"
#include "types.h"

class CThreadManager
{
public:
    static BOOL Start(UINT32 nChannels);
    static BOOL Stop();
    static void RegisterThread();

private:
    CThreadManager(UINT32 nChannels);
    ~CThreadManager();

    //  Prevent assignment: Declared but not implemented.
    CThreadManager(const CThreadManager& rhs);  // Copy Constructor
    CThreadManager& operator=(const CThreadManager& rhs);  //  Assignment operator

    BOOL Initialize();
    BOOL StopThreads();
    void Register();
    void SignalComplete();
    BOOL StartChannelThreads();

    static CThreadManager* m_pInstance;

    UINT32          m_nChannelsTotal;
    UINT32          m_nChannelsActive;
    CMutex*         m_pTManMutex;
    CEvent*         m_pStartupCompleteEvent;
};

#endif // RRIL_THREAD_MANAGER_H
