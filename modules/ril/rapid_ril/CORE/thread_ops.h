////////////////////////////////////////////////////////////////////////////
// thread_ops.h
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines CThread class for handling threads.
//
/////////////////////////////////////////////////////////////////////////////

#include <pthread.h>
typedef void* (*THREAD_PROC_PTR)(void*);


const int THREAD_PRIORITY_LEVEL_UNKNOWN = -1;
const int THREAD_PRIORITY_LEVEL_LOWEST = 0;
const int THREAD_PRIORITY_LEVEL_LOW = 1;
const int THREAD_PRIORITY_LEVEL_NORMAL = 2;
const int THREAD_PRIORITY_LEVEL_HIGH = 3;
const int THREAD_PRIORITY_LEVEL_HIGHEST = 4;

// Select any (bitmask)
const UINT32 THREAD_FLAGS_NONE = 0x0000;
const UINT32 THREAD_FLAGS_START_SUSPENDED = 0x0001;
const UINT32 THREAD_FLAGS_JOINABLE = 0x0002;

const UINT32 THREAD_WAIT_0 = 0x00000000;
const UINT32 THREAD_WAIT_TIMEOUT = 0xFFFFFFFF;
const UINT32 THREAD_WAIT_NOT_JOINABLE = 0xFFFF0001;
const UINT32 THREAD_WAIT_NOT_INITIALIZED = 0xFFFF0002;
const UINT32 THREAD_WAIT_GEN_FAILURE = 0xFFFF0003;

class CThread
{
public:
    CThread(THREAD_PROC_PTR pvThreadProc, void* pvDataObj, UINT32 dwFlags, UINT32 dwStackSize);

    // Note: Deleting the CThread will not stop the spawned thread.
    ~CThread();

    static BOOL SetPriority(CThread* pThread, UINT32 dwPriority);
    static UINT32 GetPriority(CThread* pThread);
    static UINT32 Wait(CThread* pThread, UINT32 dwTimeoutInMS);

    static BOOL IsRunning(CThread* pThread);
    static BOOL IsInitialized(CThread* pThread);

    // Will stop thread.
    static void Kill(CThread* pThread);

private:
    BOOL    SetPriority(UINT32 dwPriority);
    UINT32   GetPriority();

    UINT32   Wait(UINT32 dwTimeout);
    void    Kill();

    void*       m_pvDataObj;
    void*       m_pvThreadProc;

    UINT32      m_uiPriority;
    BOOL        m_fJoinable;
    BOOL        m_fRunning;
    BOOL        m_fInitialized;

    pthread_t   m_thread;


};
