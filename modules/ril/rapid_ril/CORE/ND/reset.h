////////////////////////////////////////////////////////////////////////////
// reset.h
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implementation of modem reset.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_RESET_H
#define RRIL_RESET_H

#include <stdarg.h>

#include "rilqueue.h"
#include "sync_ops.h"
#include "mmgr_cli.h"

class CResetQueueNode
{
public:
    virtual ~CResetQueueNode() { /* none */ }
    virtual void Execute() = 0;
};

class CDeferThread
{
public:
    static BOOL Init();
    static void Destroy();
    static BOOL QueueWork(CResetQueueNode* pNode, BOOL bNeedDeferring);

    static BOOL DequeueWork(CResetQueueNode*& pNode) { return m_pResetQueue->Dequeue(pNode); }
    static void Lock()   { CMutex::Lock(m_pThreadStartLock); }
    static void Unlock() { CMutex::Unlock(m_pThreadStartLock); }
    static void SetThreadFinished() { m_bIsThreadRunning = FALSE; }

private:
    static CRilQueue<CResetQueueNode*>* m_pResetQueue;
    static CMutex* m_pThreadStartLock;
    static BOOL m_bIsThreadRunning;
};

void ModemResetUpdate();

// Used for int => string convertion for calls to modem restart APIs
const int MAX_STRING_SIZE_FOR_INT = 12;

class CModemRestart
{
public:
    static BOOL Init();
    static void Destroy();
    static void RequestModemRestart(int lineNum, const char* pszFileName,
            int nParams, ...);
    static void RequestModemRestart(int lineNum, const char* pszFileName)
            { RequestModemRestart(lineNum, pszFileName, -1); }
    static void SaveRequestReason(int nParams, ...);

private:
    static int m_nStoredCauses;
    static mmgr_cli_recovery_cause_t* m_pStoredCauses;
    static CMutex* m_pErrorCauseMutex;

    static void CleanRequestReason();
    static void StoreRequestReason(int nParams, va_list ap);
};

#define DO_REQUEST_CLEAN_UP(...) CModemRestart::RequestModemRestart(__LINE__, __FILE__, \
        ## __VA_ARGS__);

int ModemManagerEventHandler(mmgr_cli_event_t* param);

enum ePCache_Code
{
    PIN_NO_PIN_AVAILABLE = -4,
    PIN_WRONG_INTEGRITY = -3,
    PIN_INVALID_UICC = -2,
    PIN_NOK = -1,
    PIN_OK = 0
};

ePCache_Code PCache_Store_PIN(const char* szUICC, const char* szPIN);
ePCache_Code PCache_Get_PIN(const char* szUICC, char* szPIN);
void PCache_Clear();

#endif // RRIL_RESET_H

