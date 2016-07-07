/*
 *
 *
 * Copyright (C) 2009 Intrinsyc Software International,
 * Inc.  All Rights Reserved
 *
 * Use of this code is subject to the terms of the
 * written agreement between you and Intrinsyc.
 *
 * UNLESS OTHERWISE AGREED IN WRITING, THIS WORK IS
 * DELIVERED ON AN AS IS BASIS WITHOUT WARRANTY,
 * REPRESENTATION OR CONDITION OF ANY KIND, ORAL OR
 * WRITTEN, EXPRESS OR IMPLIED, IN FACT OR IN LAW
 * INCLUDING WITHOUT LIMITATION, THE IMPLIED WARRANTIES
 * OR CONDITIONS OF MERCHANTABLE QUALITY
 * AND FITNESS FOR A PARTICULAR PURPOSE
 *
 * This work may be subject to patent protection in the
 *  United States and other jurisdictions
 *
 * Description:
 *    General utilities and system start-up and
 *    shutdown management
 *
 */


#ifndef RRIL_SYSTEMMANAGER_H
#define RRIL_SYSTEMMANAGER_H

#include "types.h"
#include "request_info_table.h"
#include "rilqueue.h"
#include "thread_manager.h"
#include "rilchannels.h"
#include "initializer.h"
#include "com_init_index.h"
#include <cutils/properties.h>
#include "mmgr_cli.h"

class CCommand;
class CChannel;
class CResponse;

// Queue Containers and Associated Events
extern CRilQueue<CCommand*>* g_pTxQueue[RIL_CHANNEL_MAX];
extern CRilQueue<CResponse*>* g_pRxQueue[RIL_CHANNEL_MAX];
extern CEvent* g_TxQueueEvent[RIL_CHANNEL_MAX];
extern CEvent* g_RxQueueEvent[RIL_CHANNEL_MAX];
extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];

///////////////////////////////////////////////////////////////////////////////
class CSystemManager
{
public:
    static CSystemManager& GetInstance();
    static BOOL Destroy();

private:
    CSystemManager();
    ~CSystemManager();

    //  Prevent assignment: Declared but not implemented.
    CSystemManager(const CSystemManager& rhs);  // Copy Constructor
    CSystemManager& operator=(const CSystemManager& rhs);  //  Assignment operator

public:
    // Start system initialization process
    BOOL InitializeSystem(const char* szModemName);

    BOOL IsExitRequestSignalled() const;

    CEvent* GetCancelWaitEvent() { return m_pCancelWaitEvent; }
    CEvent* GetModemBasicInitCompleteEvent()
    {
        return m_pInitializer->GetModemBasicInitCompleteEvent();
    }

    CMutex* GetDataChannelAccessorMutex() { return m_pDataChannelAccessorMutex; }
    CMutex* GetTEAccessMutex() { return m_pTEAccessMutex; }
    CMutex* GetSpoofCommandsStatusAccessMutex() { return m_pSpoofCommandsStatusAccessMutex; }

    void TriggerSimUnlockedEvent() { m_pInitializer->TriggerSimUnlockedEvent(); }
    void TriggerRadioPoweredOnEvent() const { m_pInitializer->TriggerRadioPoweredOnEvent(); }

    CEvent* GetModemPoweredOffEvent() { return m_pInitializer->GetModemPoweredOffEvent(); }
    void TriggerModemPoweredOffEvent() const { m_pInitializer->TriggerModemPoweredOffEvent(); }

    void TriggerInitStringCompleteEvent(UINT32 eChannel, eComInitIndex eInitIndex)
    {
        m_pInitializer->TriggerInitStringCompleteEvent(eChannel, eInitIndex);
    }

    BOOL IsInitializationSuccessful() const { return m_bIsSystemInitialized; }
    void SetInitializationUnsuccessful() { m_bIsSystemInitialized = FALSE; }

    void GetRequestInfo(int reqID, REQ_INFO& rReqInfo);

    /*
     * Signals the command, response thread to exit and also stops the thread before
     * closing the ports.
     */
    void CloseChannelPorts() { m_pInitializer->CloseChannelPorts(); }

    BOOL SendRequestModemRecovery(mmgr_cli_recovery_cause_t* pCauses, int nCauses);
    BOOL SendRequestModemShutdown();
    BOOL SendAckModemShutdown();
    BOOL SendAckModemColdReset();

    BOOL GetModem();
    BOOL ReleaseModem();

    //  This function continues the init in the function InitializeSystem() left
    //  off from InitChannelPorts().  Called when MODEM_UP status is received.
    BOOL ContinueInit();

    void ResetChannelInfo();

    // Internal Init helper functions
    void ResetSystemState();

    int GetCancelWaitPipeFd() { return m_pInitializer->GetCancelWaitPipeFd(); }

    // This function will return true if device is not encrypted or decrypted.
    BOOL IsDeviceDecrypted();

private:
    // Framework Init Functions
    BOOL CreateQueues();
    void DeleteQueues();

    BOOL MMgrConnectionInit();

    // RIL Component Initialization functions (called by system init function)
    BOOL InitializeSim();

private:
    static CSystemManager* m_pInstance;
    CInitializer* m_pInitializer;

    CEvent* m_pCancelWaitEvent;
    CEvent* m_pSysInitCompleteEvent;

    CMutex* m_pSystemManagerMutex;

    CMutex* m_pDataChannelAccessorMutex;

    mmgr_cli_handle_t* m_pMMgrLibHandle;

    CRequestInfoTable m_RequestInfoTable;

    BOOL m_bIsSystemInitialized;

    BOOL m_bIsModemResourceAcquired;

    BOOL m_bIsDeviceDecrypted;

    CMutex* m_pSpoofCommandsStatusAccessMutex;
    CMutex* m_pTEAccessMutex;
};

#endif // SYSTEMMANAGER

