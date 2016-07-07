////////////////////////////////////////////////////////////////////////////
// reset.cpp
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implementation of modem reset.
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include "types.h"
#include "rillog.h"
#include "thread_ops.h"
#include "sync_ops.h"
#include "repository.h"
#include "util.h"
#include "rildmain.h"
#include "reset.h"
#include "channel_data.h"
#include "data_util.h"
#include "te.h"
#include "te_base.h"
#include "hardwareconfig.h"
#include <sys/ioctl.h>
#include <cutils/properties.h>

#include <cutils/sockets.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/file.h>

static const char* const PIN_CACHE_FILE_FS = "/config/telephony/dump";
static const char* const PIN_CACHE_DUMP_FILE = "/sys/kernel/modem_nvram/dump";
static const char* const PIN_CACHE_SIZE_FILE = "/sys/kernel/modem_nvram/size";
static const char* const PIN_CACHE_CLEAR_FILE = "/sys/kernel/modem_nvram/clear";

// ENCRYPTED_PIN_SIZE should be multiple of 4.
static const int ENCRYPTED_PIN_SIZE = 20;
static const int UICCID_SIZE = 10;
static const int CACHED_UICCID_PIN_SIZE = UICCID_SIZE + ENCRYPTED_PIN_SIZE;
static const int CACHED_TOTAL_PIN_SIZE = CACHED_UICCID_PIN_SIZE * 2;

///////////////////////////////////////////////////////////
// Helper static class to handle modem recovery
//
int CModemRestart::m_nStoredCauses;
mmgr_cli_recovery_cause_t* CModemRestart::m_pStoredCauses;
CMutex* CModemRestart::m_pErrorCauseMutex;

BOOL CModemRestart::Init()
{
    m_nStoredCauses = 0;
    m_pErrorCauseMutex = new CMutex();
    m_pStoredCauses = (mmgr_cli_recovery_cause_t*) malloc(MMGR_CLI_MAX_RECOVERY_CAUSES *
            sizeof(mmgr_cli_recovery_cause_t));

    if ((m_pErrorCauseMutex == NULL) || (m_pStoredCauses == NULL))
    {
        RIL_LOG_CRITICAL("CModemRestart::Init() - could not allocate memory\r\n");
        return FALSE;
    }

    return TRUE;
}

void CModemRestart::Destroy()
{
    free(m_pStoredCauses);
    m_pStoredCauses = NULL;

    delete m_pErrorCauseMutex;
    m_pErrorCauseMutex = NULL;
}

void CModemRestart::RequestModemRestart(int lineNum, const char* pszFileName,
        int nParams, ...)
{
    RIL_LOG_VERBOSE("CModemRestart::RequestModemRestart() - ENTER\r\n");

    if (CTE::GetTE().IsPlatformShutDownRequested())
    {
        RIL_LOG_INFO("CModemRestart::RequestModemRestart() - "
                "Ignore modem recovery request in platform shutdown\r\n");
        return;
    }

    CMutex::Lock(m_pErrorCauseMutex);

    // If Spoof commands, log and return
    if (CTE::GetTE().TestAndSetSpoofCommandsStatus(TRUE))
    {
        RIL_LOG_INFO("CModemRestart::RequestModemRestart() - ignore recovery request.\r\n");
    }
    else
    {
        //  Doesn't matter what the error is, we are notifying MMGR that
        //  something is wrong. Let the modem status socket watchdog get
        //  a MODEM_UP when things are OK again.

        CSystemManager::GetInstance().SetInitializationUnsuccessful();

        if (E_MMGR_EVENT_MODEM_UP == CTE::GetTE().GetLastModemEvent())
        {
            RIL_LOG_INFO("CModemRestart::RequestModemRestart() - "
                    "file=[%s], line num=[%d] num params=[%d]\r\n",
                    pszFileName, lineNum, nParams);

            // If user provides recovery reasons, put them in recovery storage to
            // share code between both use cases.
            if (nParams >= 0)
            {
                CleanRequestReason();

                if (nParams > MMGR_CLI_MAX_RECOVERY_CAUSES)
                {
                    RIL_LOG_WARNING("CModemRestart::RequestModemRestart() - "
                                    "too many causes %d, truncating to %d\r\n",
                                    nParams, MMGR_CLI_MAX_RECOVERY_CAUSES);
                    nParams = MMGR_CLI_MAX_RECOVERY_CAUSES;
                }

                va_list ap;
                va_start(ap, nParams);
                StoreRequestReason(nParams, ap);
                va_end(ap);
            }

            // Store in last recovery cause position (4) - if available - the file / line number
            if (m_nStoredCauses < MMGR_CLI_MAX_RECOVERY_CAUSES)
            {
                for (int i = m_nStoredCauses; i < (MMGR_CLI_MAX_RECOVERY_CAUSES - 1); i++)
                {
                    m_pStoredCauses[i].cause = NULL;
                }
                m_pStoredCauses[MMGR_CLI_MAX_RECOVERY_CAUSES - 1].cause =
                        (char*) malloc(PATH_MAX + MAX_STRING_SIZE_FOR_INT);
                snprintf(m_pStoredCauses[MMGR_CLI_MAX_RECOVERY_CAUSES - 1].cause,
                        PATH_MAX + MAX_STRING_SIZE_FOR_INT,
                        "%s:%d", pszFileName, lineNum);
                m_nStoredCauses = MMGR_CLI_MAX_RECOVERY_CAUSES;
            }

            // MMGR API expects length field to be filled in. Replace also all NULL pointers
            // with empty strings.
            for (int i = 0; i < m_nStoredCauses; i++)
            {
                if (m_pStoredCauses[i].cause == NULL)
                {
                    m_pStoredCauses[i].cause = (char*) malloc(1);
                    m_pStoredCauses[i].cause[0] = '\0';
                    m_pStoredCauses[i].len = 1;
                }
                else
                {
                    m_pStoredCauses[i].len = strlen(m_pStoredCauses[i].cause) + 1;
                }
            }

            //  Voice calls disconnected, no more data connections
            ModemResetUpdate();

            // Needed for resetting registration states in framework
            CTE::GetTE().SetRadioStateAndNotify(RRIL_RADIO_STATE_UNAVAILABLE);

            //  Send recovery request to MMgr
            if (!CSystemManager::GetInstance().SendRequestModemRecovery(m_pStoredCauses,
                    m_nStoredCauses))
            {
                RIL_LOG_CRITICAL("CModemRestart::RequestModemRestart() - CANNOT SEND "
                        "MODEM RESTART REQUEST\r\n");
            }
        }
        else
        {
            RIL_LOG_INFO("CModemRestart::RequestModemRestart() - "
                    "received in modem_state != E_MMGR_EVENT_MODEM_UP state\r\n");
        }
    }

    CleanRequestReason();

    CMutex::Unlock(m_pErrorCauseMutex);

    RIL_LOG_VERBOSE("CModemRestart::RequestModemRestart() - EXIT\r\n");
}

void CModemRestart::CleanRequestReason()
{
    for (int i = 0; i < m_nStoredCauses; i++)
    {
        free(m_pStoredCauses[i].cause);
    }
    m_nStoredCauses = 0;
}

void CModemRestart::StoreRequestReason(int nParams, va_list ap)
{
    m_nStoredCauses = nParams;
    for (int i = 0; i < nParams; i++)
    {
        char* cause = va_arg(ap, char*);

        if (cause == NULL)
        {
            m_pStoredCauses[i].cause = NULL;
        }
        else
        {
            m_pStoredCauses[i].cause = strdup(cause);
            if (m_pStoredCauses[i].cause == NULL)
            {
                m_nStoredCauses = i;
                RIL_LOG_CRITICAL("CModemRestart::StoreRequestReason() - "
                        "failure to allocate memory\r\n");
                break;
            }
        }
    }
}

void CModemRestart::SaveRequestReason(int nParams, ...)
{
    CMutex::Lock(m_pErrorCauseMutex);

    CleanRequestReason();

    if (nParams > MMGR_CLI_MAX_RECOVERY_CAUSES)
    {
        RIL_LOG_WARNING("CModemRestart::SaveRequestReason() - "
                "too many causes %d, truncating to %d\r\n",
                nParams, MMGR_CLI_MAX_RECOVERY_CAUSES);
        nParams = MMGR_CLI_MAX_RECOVERY_CAUSES;
    }

    va_list ap;
    va_start(ap, nParams);
    StoreRequestReason(nParams, ap);
    va_end(ap);

    CMutex::Unlock(m_pErrorCauseMutex);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Class handling modem shutdown
class CResetQueueNodeModemShutdown : public CResetQueueNode
{
public:
    CResetQueueNodeModemShutdown(BOOL bIsFlightMode);
    void Execute();

private:
    BOOL m_bIsFlightMode;
};

CResetQueueNodeModemShutdown::CResetQueueNodeModemShutdown(BOOL bIsFlightMode) :
    m_bIsFlightMode(bIsFlightMode)
{ /* none */
}

void CResetQueueNodeModemShutdown::Execute()
{
    RIL_LOG_VERBOSE("CResetQueueNodeModemShutdown::Execute() - Enter\r\n");

    //  Spoof commands from now on
    CTE::GetTE().SetSpoofCommandsStatus(TRUE);

    CSystemManager::GetInstance().ResetSystemState();

    if (m_bIsFlightMode)
    {
        RIL_LOG_INFO("E_MMGR_EVENT_MODEM_SHUTDOWN due to Flight mode\r\n");

        CTE::GetTE().ResetCardStatus(FALSE);

        // Inform Android of new state
        // Voice calls, data connections, sim state etc
        ModemResetUpdate();
    }

    CTE::GetTE().ResetInternalStates();

    CSystemManager::GetInstance().CloseChannelPorts();

    CSystemManager::GetInstance().SendAckModemShutdown();

    RIL_LOG_VERBOSE("CResetQueueNodeModemShutdown::Execute() - EXIT\r\n");
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//  Class handling various modem down scenarios (core dump, MRESET, down, ...)
class CResetQueueNodeModemDown : public CResetQueueNode
{
public:
    CResetQueueNodeModemDown(BOOL bDoStateReset,
            BOOL bClosePorts,
            BOOL bSendCloseResetAck,
            BOOL bIsPlatformShutdown);
    void Execute();

private:
    BOOL m_bDoStateReset;
    BOOL m_bClosePorts;
    BOOL m_bSendColdResetAck;
    BOOL m_bIsPlatformShutdown;
};

CResetQueueNodeModemDown::CResetQueueNodeModemDown(BOOL bDoStateReset,
        BOOL bClosePorts,
        BOOL bSendCloseResetAck,
        BOOL bIsPlatformShutdown) :
    m_bDoStateReset(bDoStateReset),
    m_bClosePorts(bClosePorts),
    m_bSendColdResetAck(bSendCloseResetAck),
    m_bIsPlatformShutdown(bIsPlatformShutdown)
{ /* none */
}

void CResetQueueNodeModemDown::Execute()
{
    RIL_LOG_VERBOSE("CResetQueueNodeModemDown::Execute() - Enter\r\n");

    if (m_bDoStateReset)
    {
        //  Spoof commands from now on
        CTE::GetTE().SetSpoofCommandsStatus(TRUE);

        CSystemManager::GetInstance().ResetSystemState();

        // Needed for resetting registration states in framework
        CTE::GetTE().SetRadioStateAndNotify(RRIL_RADIO_STATE_UNAVAILABLE);

        CTE::GetTE().ResetCardStatus(FALSE);

        //  Inform Android of new state
        //  Voice calls disconnected, no more data connections
        ModemResetUpdate();

        CTE::GetTE().ResetInternalStates();
    }

    if (m_bClosePorts)
    {
        CSystemManager::GetInstance().CloseChannelPorts();
    }

    if (m_bSendColdResetAck)
    {
        CSystemManager::GetInstance().SendAckModemColdReset();
    }

    if (m_bIsPlatformShutdown)
    {
        RIL_LOG_INFO("E_MMGR_EVENT_MODEM_DOWN due to PLATFORM_SHUTDOWN\r\n");

        CTE::GetTE().SetRadioStateAndNotify(RRIL_RADIO_STATE_OFF);

        CSystemManager::GetInstance().TriggerModemPoweredOffEvent();
    }
    RIL_LOG_VERBOSE("CResetQueueNodeModemDown::Execute() - EXIT\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Class handling modem out of service case
class CResetQueueNodeModemOutOfService : public CResetQueueNode
{
public:
    void Execute();
};

void CResetQueueNodeModemOutOfService::Execute()
{
    RIL_LOG_VERBOSE("CResetQueueNodeModemOutOfService::Execute() - Enter\r\n");

    //  Spoof commands from now on
    CTE::GetTE().SetSpoofCommandsStatus(TRUE);

    CSystemManager::GetInstance().ResetSystemState();

    //  Inform Android of new state
    //  Voice calls disconnected, no more data connections
    ModemResetUpdate();

    CTE::GetTE().ResetInternalStates();

    RIL_LOG_VERBOSE("CResetQueueNodeModemOutOfService::Execute() - EXIT\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Class handling modem up case
class CResetQueueNodeModemUp : public CResetQueueNode
{
public:
    void Execute();
};

void CResetQueueNodeModemUp::Execute()
{
    RIL_LOG_VERBOSE("CResetQueueNodeModemUp::Execute() - Enter\r\n");

    CSystemManager::GetInstance().ResetChannelInfo();

    //  turn off spoof
    CTE::GetTE().SetSpoofCommandsStatus(FALSE);

    if (!CSystemManager::GetInstance().ContinueInit())
    {
        RIL_LOG_CRITICAL("CResetQueueNodeModemUp::Execute() - "
                "MODEM_UP handling failed, try a restart\r\n");
        DO_REQUEST_CLEAN_UP(1, "CSystemManager::ContinueInit failed");
    }
    else
    {
        RIL_LOG_INFO("CResetQueueNodeModemUp::Execute() - Open ports OK\r\n");
    }

    RIL_LOG_VERBOSE("CResetQueueNodeModemUp::Execute() - EXIT\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Static class handling deferring thread

static void* MMGRDeferredEventTreadProc(void* pDummy);

CRilQueue<CResetQueueNode*>* CDeferThread::m_pResetQueue = NULL;
CMutex* CDeferThread::m_pThreadStartLock = NULL;
BOOL CDeferThread::m_bIsThreadRunning = FALSE;

BOOL CDeferThread::Init()
{
    CThread* pMMGREventHelperThread = NULL;

    m_pResetQueue = new CRilQueue<CResetQueueNode*>(TRUE);
    m_pThreadStartLock = new CMutex();

    if ((m_pResetQueue == NULL) || (m_pThreadStartLock == NULL))
    {
        RIL_LOG_CRITICAL("CDeferThread::Init() - could not allocate memory\r\n");
        return FALSE;
    }

    return TRUE;
}

void CDeferThread::Destroy()
{
    delete m_pResetQueue;
    m_pResetQueue = NULL;

    delete m_pThreadStartLock;
    m_pThreadStartLock = NULL;
}

BOOL CDeferThread::QueueWork(CResetQueueNode* pNode, BOOL bNeedDeferring)
{
    if (pNode == NULL)
    {
        RIL_LOG_CRITICAL("CDeferThread::QueueWork() - could not allocate memory\r\n");
        return FALSE;
    }

    CMutex::Lock(m_pThreadStartLock);
    if (m_bIsThreadRunning)
    {
        /* If a worker thread is still running, queue the node to have it executed in the
         * currently running thread (usage of the lock guarantees that the thread will not
         * exit while the node is being queued).
         *
         * Note that in this case the activity will be ran in the deferred thread (even if not
         * required) to guarantee sequential processing.
         */
        if (!m_pResetQueue->Enqueue(pNode))
        {
            RIL_LOG_CRITICAL("CDeferThread::QueueWork() - could not queue node\r\n");
            delete pNode;
            CMutex::Unlock(m_pThreadStartLock);
            return FALSE;
        }
    }
    else
    {
        if (bNeedDeferring)
        {
            /* Need to start a new worker thread to handle the new deferred activity and so to
             * queue the node to get it processed in the worker thread.
             */
            if (!m_pResetQueue->Enqueue(pNode))
            {
                RIL_LOG_CRITICAL("CDeferThread::QueueWork() - could not queue node\r\n");
                delete pNode;
                CMutex::Unlock(m_pThreadStartLock);
                return FALSE;
            }

            /* Start the worker thread */
            CThread* pMMGREventHelperThread = new CThread(MMGRDeferredEventTreadProc, NULL,
                    THREAD_FLAGS_NONE, 0);
            if (NULL == pMMGREventHelperThread)
            {
                RIL_LOG_CRITICAL("CDeferThread::QueueWork() - could not create helper thread\r\n");
                CResetQueueNode *pNode;
                m_pResetQueue->Dequeue(pNode);
                delete pNode;
                CMutex::Unlock(m_pThreadStartLock);
                return FALSE;
            }
            m_bIsThreadRunning = TRUE;

            delete pMMGREventHelperThread;
            pMMGREventHelperThread = NULL;
        }
        else
        {
            /* As no other MMGR activity is running, we can run this activity in the main MMGR
             * event thread as it does not require deferring to a separate thread.
             */
            pNode->Execute();
            delete pNode;
        }
    }
    CMutex::Unlock(m_pThreadStartLock);

    return TRUE;
}

///////////////////////////////////////////////////////////
// FUNCTION DEFINITIONS
//

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//  Main code of helper thread that handles deferred MMGR event processing
static void* MMGRDeferredEventTreadProc(void* /* pDummy */)
{
    RIL_LOG_INFO("MMGRDeferredEventTreadProc() - ENTER");

    while (1)
    {
        CResetQueueNode *pNode;

        CDeferThread::Lock();
        if (CDeferThread::DequeueWork(pNode))
        {
            CDeferThread::Unlock();
            if (pNode == NULL)
            {
                break;
            }
            pNode->Execute();
            delete pNode;
        }
        else
        {
            CDeferThread::SetThreadFinished();
            CDeferThread::Unlock();
            break;
        }
    }

    RIL_LOG_INFO("MMGRDeferredEventTreadProc() - EXIT");

    return NULL;
}

///////////////////////////////////////////////////////////
//  GLOBAL VARIABLES
//

///////////////////////////////////////////////////////////
// FUNCTION DEFINITIONS
//

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// This Function aggregates the actions of updating Android stack, when a reset is identified.
void ModemResetUpdate()
{

    RIL_LOG_VERBOSE("ModemResetUpdate() - Enter\r\n");

    sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND regStatusInfo;

    regStatusInfo.commandId = RIL_OEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND;
    regStatusInfo.regStatus = 0;
    regStatusInfo.bandLength = 0;
    regStatusInfo.szBand[0] = '\0';
    RIL_onUnsolicitedResponse(RIL_UNSOL_OEM_HOOK_RAW,
            (void*)&regStatusInfo, sizeof(sOEM_HOOK_RAW_UNSOL_REG_STATUS_AND_BAND_IND));

    CTE::GetTE().CleanupAllDataConnections();

    //  Tell Android no more data connection
    RIL_onUnsolicitedResponse(RIL_UNSOL_DATA_CALL_LIST_CHANGED, NULL, 0);

    //  If there was a voice call active, it is disconnected.
    //  This will cause a RIL_REQUEST_GET_CURRENT_CALLS to be sent
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED, NULL, 0);

    /*
     * Needs to be triggered so that operator, registration states and signal bars
     * gets updated when radio off or not available.
     */
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED, NULL, 0);

    /*
     * Needed for SIM hot swap to function properly when modem off in flight
     * is activated.
     */
    RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);

    RIL_LOG_VERBOSE("ModemResetUpdate() - Exit\r\n");
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//  This method handles MMgr events and notifications
int ModemManagerEventHandler(mmgr_cli_event_t* param)
{
    RIL_LOG_VERBOSE("ModemManagerEventHandler() - Enter\r\n");

    CThread* pContinueInitThread = NULL;
    int receivedModemEvent = (int) param->id;
    const int SLEEP_MS = 10000;
    int retValue = 0;

    //  Store the previous modem's state.  Only handle the toggle of the modem state.
    int previousModemState = CTE::GetTE().GetLastModemEvent();

    //  Now start polling for modem status...
    RIL_LOG_INFO("ModemManagerEventHandler() - Processing modem event or notification......\r\n");

    RIL_LOG_INFO("ModemManagerEventHandler() - previousModemState: %d, receivedModemEvent: %d\r\n",
            previousModemState, receivedModemEvent);

    //  Compare with previous modem status.  Only handle the toggle.
    if (receivedModemEvent != previousModemState)
    {
        switch(receivedModemEvent)
        {
            case E_MMGR_EVENT_MODEM_UP:
                RIL_LOG_INFO("[RIL STATE] (RIL <- MMGR) MODEM_UP\r\n");

                CTE::GetTE().SetLastModemEvent(receivedModemEvent);

                if (!CDeferThread::QueueWork(new CResetQueueNodeModemUp(), TRUE))
                {
                    RIL_LOG_CRITICAL("ModemManagerEventHandler() -"
                            "failed to defer MODEM_UP processing\r\n");
                    //  let's exit, init will restart us
                    RIL_LOG_INFO("ModemManagerEventHandler() - CALLING EXIT\r\n");
                    CSystemManager::Destroy();
                    exit(0);
                }
                break;

            case E_MMGR_EVENT_MODEM_OUT_OF_SERVICE:
                RIL_LOG_INFO("[RIL STATE] (RIL <- MMGR) MODEM OUT OF SERVICE\r\n");

                CTE::GetTE().SetLastModemEvent(receivedModemEvent);

                CDeferThread::QueueWork(new CResetQueueNodeModemOutOfService(), FALSE);

                // Don't exit the RRIL to avoid automatic restart: sleep for ever
                RIL_LOG_CRITICAL("ModemManagerEventHandler() -"
                        " OUT_OF_SERVICE Now sleeping till reboot\r\n");
                while(1) { sleep(SLEEP_MS); }
                break;

            case E_MMGR_EVENT_MODEM_DOWN:
                RIL_LOG_INFO("[RIL STATE] (RIL <- MMGR) MODEM_DOWN\r\n");
                CTE::GetTE().SendModemDownToUsatSM();

                CTE::GetTE().SetLastModemEvent(receivedModemEvent);

                CDeferThread::QueueWork(new CResetQueueNodeModemDown(
                        E_MMGR_NOTIFY_MODEM_SHUTDOWN != previousModemState,
                        FALSE, FALSE,
                        CTE::GetTE().IsPlatformShutDownRequested()), FALSE);

                if (CTE::GetTE().IsPlatformShutDownRequested())
                {
                    while(1) { sleep(SLEEP_MS); }
                }
                break;

            case E_MMGR_NOTIFY_PLATFORM_REBOOT:
                RIL_LOG_INFO("[RIL STATE] (RIL <- MMGR) PLATFORM_REBOOT done by MMGR\r\n");

                CTE::GetTE().SetLastModemEvent(receivedModemEvent);

                CDeferThread::QueueWork(new CResetQueueNodeModemDown(
                        E_MMGR_EVENT_MODEM_DOWN != previousModemState,
                        FALSE, FALSE, FALSE), FALSE);

                // Don't exit the RRIL to avoid automatic restart: sleep for ever
                // MMGR will reboot the platform
                RIL_LOG_CRITICAL("ModemManagerEventHandler() - Now sleeping till reboot\r\n");
                while(1) { sleep(SLEEP_MS); }
                break;

            case E_MMGR_NOTIFY_MODEM_COLD_RESET:
                RIL_LOG_INFO("[RIL STATE] (RIL <- MMGR) MODEM_COLD_RESET\r\n");

                CTE::GetTE().SetLastModemEvent(receivedModemEvent);

                CDeferThread::QueueWork(new CResetQueueNodeModemDown(
                        E_MMGR_EVENT_MODEM_DOWN != previousModemState,
                        TRUE, TRUE, FALSE), TRUE);

                // Set to -1. This will force the client library not to send ACK to mmgr
                retValue = -1;
                break;

            case E_MMGR_NOTIFY_CORE_DUMP:
                RIL_LOG_INFO("[RIL STATE] (RIL <- MMGR) E_MMGR_NOTIFY_CORE_DUMP\r\n");

                CTE::GetTE().SetLastModemEvent(receivedModemEvent);

                CDeferThread::QueueWork(new CResetQueueNodeModemDown(
                        E_MMGR_EVENT_MODEM_DOWN != previousModemState,
                        TRUE, FALSE, FALSE), TRUE);
                break;

            case E_MMGR_NOTIFY_MODEM_SHUTDOWN:
            {
                RIL_LOG_INFO("[RIL STATE] (RIL <- MMGR) MODEM_SHUTDOWN\r\n");

                CTE::GetTE().SetLastModemEvent(receivedModemEvent);

                BOOL is_flight_mode = CTE::GetTE().GetModemOffInFlightModeState()
                        && (E_RADIO_OFF_REASON_AIRPLANE_MODE == CTE::GetTE().GetRadioOffReason());
                CDeferThread::QueueWork(new CResetQueueNodeModemShutdown(is_flight_mode), TRUE);

                // Set to -1. This will force the client library not to send ACK to mmgr
                retValue = -1;
                break;
            }

            default:
                RIL_LOG_INFO("[RIL STATE] (RIL <- MMGR) UNKNOWN [%d]\r\n", receivedModemEvent);
                break;
        }
    }

    RIL_LOG_VERBOSE("ModemManagerEventHandler() - Exit\r\n");
    return retValue;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// XXTEA encryption / decryption algorithm.
// Code source is from http://en.wikipedia.org/wiki/XXTEA

const UINT32 DELTA = 0x9e3779b9;
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))

// btea: Encrypt or decrypt int array v of length n with 4-integer array key
//
// Parameters:
// UINT32 array v [in/out] : UINT32 array to be encrypted or decrypted
// int n [in] : Number of integers in array v.  If n is negative, then decrypt.
// UINT32 array key [in] : UINT32 array of size 4 that contains key to encrypt or decrypt
//
// Return values:
// None
void btea(UINT32* v, int n, UINT32 const key[4])
{
    UINT32 y, z, sum;
    UINT32 rounds, e;
    int p;

    if (n > 1)
    {
        // Coding Part
        rounds = 6 + 52/n;
        sum = 0;
        z = v[n-1];
        do
        {
            sum += DELTA;
            e = (sum >> 2) & 3;
            for (p=0; p<n-1; p++)
            {
                y = v[p+1];
                z = v[p] += MX;
            }
            y = v[0];
            z = v[n-1] += MX;
        } while (--rounds);
    }
    else if (n < -1)
    {
        // Decoding Part
        n = -n;
        rounds = 6 + 52/n;
        sum = rounds*DELTA;
        y = v[0];
        do
        {
            e = (sum >> 2) & 3;
            for (p=n-1; p>0; p--)
            {
                z = v[p-1];
                y = v[p] -= MX;
            }
            z = v[n-1];
            y = v[0] -= MX;
        } while ((sum -= DELTA) != 0);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// This function is a helper for the btea() function.  It converts a string to an 4-integer array
// to be passed in as the key to the btea() function.
//
// Parameters:
// string pszKey [in] : string to be converted to 4-integer array
// UINT32 array pKey [out] : 4-integer array of szKey (to be passed into btea function)
//                                 Array must be allocated by caller.
//
// Return values:
// PIN_INVALID_UICC if error with pszKey
// PIN_NOK if any other error
// PIN_OK if operation is successful
ePCache_Code ConvertKeyToInt4(const char* pszKey, UINT32* pKey)
{
    // Check inputs
    if (NULL == pszKey || strlen(pszKey) > 32)
    {
        return PIN_INVALID_UICC;
    }

    if (NULL == pKey)
    {
        return PIN_NOK;
    }

    // Algorithm:
    // Take pszKey, and prepend '0's to make 32 character string
    char szBuf[33] = {0};

    int len = (int)strlen(pszKey);
    int diff = 32 - len;

    // Front-fill buffer
    for (int i = 0; i < diff; i++)
    {
        szBuf[i] = '0';
    }

    strncat(szBuf, pszKey, 32 - strlen(szBuf));
    szBuf[32] = '\0';  //  KW fix

    // Now we have szBuf in format "0000.... <UICC>" which is exactly 32 characters.
    // Take chunks of 8 characters, use atoi on it.  Store atoi value in pKey.
    for (int i = 0; i < 4; i++)
    {
        char szChunk[9] = {0};
        strncpy(szChunk, &szBuf[i * 8], 8);
        pKey[i] = atoi(szChunk);
    }

    return PIN_OK;
}

ePCache_Code PCache_Read(unsigned char* buf, bool bAll, bool bLock)
{
    int fd = -1;
    ssize_t byteRead = 0;
    ePCache_Code res = PIN_NOK;
    int ret = -1;
    int count = 0;
    unsigned char cache1[CACHED_TOTAL_PIN_SIZE] = { '\0' };
    unsigned char* cache2 = cache1;

    UINT32 uiPinCacheMode = CTE::GetTE().GetPinCacheMode();

    if (E_PIN_CACHE_MODE_NVRAM == uiPinCacheMode)
    {
        fd = open(PIN_CACHE_DUMP_FILE, O_RDONLY);
    }
    else if (E_PIN_CACHE_MODE_FS == uiPinCacheMode)
    {
        fd = open(PIN_CACHE_FILE_FS, O_CREAT | O_RDONLY, S_IWUSR | S_IRUSR);
    }

    if (0 > fd)
    {
        goto Error;
    }

    if(bLock) {
        ret = flock(fd, LOCK_EX);

        if(ret != 0)
        {
            goto Error;
        }
    }

    byteRead = read(fd, cache1, CACHED_TOTAL_PIN_SIZE);

    if (byteRead != CACHED_TOTAL_PIN_SIZE &&
                byteRead != CACHED_UICCID_PIN_SIZE && byteRead != 0)
    {
        RIL_LOG_INFO("%s PCache byteRead %d", __FUNCTION__, byteRead);
        goto Error;
    }
    if (!bAll)
    {
        if (1 == CHardwareConfig::GetInstance().GetSIMId())
        {
            cache2 = cache1 + CACHED_UICCID_PIN_SIZE;
        }
        count = CACHED_UICCID_PIN_SIZE;

    } else {
        count = CACHED_TOTAL_PIN_SIZE;
    }

    memcpy(buf, cache2, count);

    res = PIN_OK;

Error:
    if (0 <= fd)
    {
        if (0 > close(fd))
        {
            res = PIN_NOK;
        }
    }

    return res;
}

ePCache_Code PCache_Write(const unsigned char* buf)
{
    int fd = -1;
    int lfd = -1;
    ssize_t byteRead = 0;
    ePCache_Code res = PIN_NOK;
    int ret = -1;
    unsigned char cache1[CACHED_TOTAL_PIN_SIZE] = { '\0' };
    unsigned char* cache2 = cache1;

    UINT32 uiPinCacheMode = CTE::GetTE().GetPinCacheMode();

    if (E_PIN_CACHE_MODE_NVRAM == uiPinCacheMode)
    {
        lfd = open(PIN_CACHE_DUMP_FILE, O_RDONLY);
    }
    else if (E_PIN_CACHE_MODE_FS == uiPinCacheMode)
    {
        lfd = open(PIN_CACHE_FILE_FS, O_CREAT | O_RDONLY, S_IWUSR | S_IRUSR);
    }

    if (0 > lfd)
    {
        goto Error;
    }

    ret = flock(lfd, LOCK_EX);

    if(ret != 0)
    {
        goto Error;
    }

    if(PCache_Read(cache1, true, false) != PIN_OK)
    {
        goto Error;
    }

    if (E_PIN_CACHE_MODE_NVRAM == uiPinCacheMode)
    {
        fd = open(PIN_CACHE_DUMP_FILE, O_WRONLY);
    }
    else if (E_PIN_CACHE_MODE_FS == uiPinCacheMode)
    {
        fd = open(PIN_CACHE_FILE_FS, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
    }

    if (0 > fd)
    {
        goto Error;
    }


    if (1 == CHardwareConfig::GetInstance().GetSIMId())
    {
        cache2 = cache1 + CACHED_UICCID_PIN_SIZE;
    }

    memcpy(cache2, buf, CACHED_UICCID_PIN_SIZE);

    if(write(fd, cache1, CACHED_TOTAL_PIN_SIZE) < CACHED_TOTAL_PIN_SIZE)
    {
        goto Error;
    }

    res = PIN_OK;

Error:
    if (0 <= fd)
    {
        if (0 > close(fd))
        {
            res = PIN_NOK;
        }
    }
    if (0 <= lfd)
    {
        if (0 > close(lfd))
        {
            res = PIN_NOK;
        }
    }

    return res;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// This function is a wrapper for the btea() function.  Encrypt szInput string, with key pszKey.
// Store the encrypted string as HEX-ASCII in storage location.
//
// Parameters:
// string pszInput [in] : string to be encrypted
// int nInputLen [in] : number of characters to encrypt (max value is MAX_PIN_SIZE-1)
// string pszKey [in] : key to encrypt szInput with
// string pFile [in] : file name with full patch containing the encrypted string
//
// Return values:
// PIN_NOK if error
// PIN_OK if operation successful
ePCache_Code encrypt(const char* pszInput, const int nInputLen, const char* pszKey,
        const char* pFile)
{
    ePCache_Code res = PIN_NOK;
    const int BUF_LEN = ENCRYPTED_PIN_SIZE / 2;
    UINT16 buf[BUF_LEN] = {0};
    UINT32 key[4] = {0};
    BYTE encryptedBuf[CACHED_UICCID_PIN_SIZE] = {0};
    ssize_t bytesWritten = 0;

    // Check inputs
    if (NULL == pszInput || '\0' == pszInput[0] || 0 == nInputLen
            || (MAX_PIN_SIZE - 1 < nInputLen) || NULL == pszKey)
    {
        goto Error;
    }

    // generate random salt
    srand((UINT32) time(NULL));

    // Front-fill the int buffer with random salt (first bits of int is FF
    // so we can identify later)
    for (int i = 0; i < BUF_LEN - nInputLen; i++)
    {
        buf[i] = rand() | 0xFF00;
    }

    // Copy the ASCII values after the random salt in the buffer.
    for (int i = 0; i < nInputLen; i++)
    {
        buf[i + (BUF_LEN - nInputLen)] = (UINT16)pszInput[i];
    }

    // Convert the UICC to format suitable for btea
    if (PIN_OK != ConvertKeyToInt4(pszKey, key))
    {
        goto Error;
    }

    // Encrypt all buffer including the random salt
    btea((UINT32*)buf, BUF_LEN / 2, key);

    for (int i = 0; i < UICCID_SIZE; i++)
    {
        encryptedBuf[i] = SemiByteCharsToByte(pszKey[i*2], pszKey[i*2+1]);
    }

    // Copy the encrypted data to buffer
    memcpy(encryptedBuf + UICCID_SIZE, buf, BUF_LEN * sizeof(UINT16));

    if(PCache_Write(encryptedBuf) != PIN_OK)
    {
        RIL_LOG_INFO("%s PCache PCache_Write return error", __FUNCTION__);
        goto Error;
    }

    res = PIN_OK;
Error:

    return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// This function is a wrapper for the btea() function.  Decrypt pin cached in storage location
// with string pszKey.
//
// Parameters:
// string pszOut [out] : the decrypted string.  It is up to caller to allocate pszOut and make
//                         sure pszOut is buffer size is large enough.
// string pszKey [in] : key to decrypt the encrypted string from storage location with.
//
// string pFile [in] : file name with full patch containing the encrypted string
//
// Return values:
// PIN_NO_PIN_AVAILABLE if PIN is not cached
// PIN_WRONG_INTEGRITY if decrypted text doesn't pass integrity checks
// PIN_NOK if inputs are invalid
// PIN_INVALID_UICC
// PIN_OK if operation successful
ePCache_Code decrypt(char* pszOut, const char* pszKey, const char* pFile)
{
    ePCache_Code ret = PIN_NOK;
    UINT32 key[4] = {0};
    const int BUF_LEN = ENCRYPTED_PIN_SIZE / 2;
    UINT16 buf[ENCRYPTED_PIN_SIZE] = {0};
    BYTE szEncryptedBuf[CACHED_UICCID_PIN_SIZE] = {0};
    BYTE uiccID[UICCID_SIZE] = {0};
    ssize_t bytesRead = 0;
    int readFd = -1;
    char* pChar = NULL;

    if (NULL == pszOut || NULL == pszKey)
    {
        goto Error;
    }

    if(PCache_Read(szEncryptedBuf, false, true) != PIN_OK)
    {
        RIL_LOG_INFO("%s PCache PCache_Read error", __FUNCTION__);
        goto Error;
    }

    // Convert the UICC to format suitable for btea
    if (PIN_OK != ConvertKeyToInt4(pszKey, key))
    {
        goto Error;
    }

    for (int i = 0; i < UICCID_SIZE; i++)
    {
        uiccID[i] = SemiByteCharsToByte(pszKey[i*2], pszKey[i*2+1]);
    }

    // Check if the PIN is cached for the UICC identifier provided.
    if (0 != memcmp(uiccID, szEncryptedBuf, UICCID_SIZE))
    {
        ret = PIN_INVALID_UICC;
        goto Error;
    }

    // Copy the encrypted buffer to local buffer
    memcpy(buf, szEncryptedBuf + UICCID_SIZE, ENCRYPTED_PIN_SIZE);

    // Actual decryption
    btea((UINT32*)buf, (-1 * BUF_LEN / 2), key);

    pChar = &(pszOut[0]);

    // We have our decrypted buffer. Figure out if it was successful and
    // throw away the random salt.
    for (int i = 0; i < BUF_LEN; i++)
    {
        if (0xFF00 == (buf[i] & 0xFF00))
        {
            // it was random salt, discard
            continue;
        }
        else if (buf[i] >= '0' && buf[i] <= '9')
        {
            // valid ASCII numeric character
            *pChar = (char)buf[i];
            pChar++;
        }
        else
        {
            // bad decoding
            ret = PIN_WRONG_INTEGRITY;
            goto Error;
        }
    }

    ret = PIN_OK;
Error:

    return ret;
}

BOOL IsRequiredCacheAvailable()
{
    int fd = -1;
    BOOL bRet = FALSE;
    UINT32 uiPinCacheMode = CTE::GetTE().GetPinCacheMode();

    if (E_PIN_CACHE_MODE_NVRAM == uiPinCacheMode)
    {
        long availableCacheSize = -1;

        fd = open(PIN_CACHE_SIZE_FILE, O_RDONLY);
        if (0 > fd)
        {
            goto Error;
        }
        else
        {
            char buffer[4] = {'\0'};
            if (read(fd, &buffer, (sizeof(buffer) - 1)) > 0)
            {
                availableCacheSize = strtol(buffer, NULL, 0);
            }

            close(fd);
        }

        int maxCacheSizeNeeded = CACHED_UICCID_PIN_SIZE;
        if (1 == CHardwareConfig::GetInstance().GetSIMId())
        {
            maxCacheSizeNeeded *= 2;
        }

        // If expected cache size is not available, return error.
        if (availableCacheSize < maxCacheSizeNeeded)
        {
            goto Error;
        }
    }
    else if (E_PIN_CACHE_MODE_FS == uiPinCacheMode)
    {
        fd = open(PIN_CACHE_FILE_FS, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
        if (0 > fd)
        {
            goto Error;
        }

        close(fd);
    }
    else
    {
        goto Error;
    }

    bRet = TRUE;
Error:
    return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// Ciphers {PIN code, UICC Id} pair and store ciphered object in a local location.
// Input: UICC Id, PIN code
// Output: {OK},{NOK}
//
ePCache_Code PCache_Store_PIN(const char* pszUICC, const char* pszPIN)
{
    ePCache_Code ret = PIN_NOK;
    if (IsRequiredCacheAvailable())
    {
        if (NULL != pszUICC && '\0' != pszUICC[0] && NULL != pszPIN && '\0' != pszPIN[0])
        {
            UINT32 uiPinCacheMode = CTE::GetTE().GetPinCacheMode();

            if (E_PIN_CACHE_MODE_NVRAM == uiPinCacheMode)
            {
                ret = encrypt(pszPIN, strnlen(pszPIN, MAX_PIN_SIZE - 1), pszUICC,
                        PIN_CACHE_DUMP_FILE);
            }
            else if (E_PIN_CACHE_MODE_FS == uiPinCacheMode)
            {
                ret = encrypt(pszPIN, strnlen(pszPIN, MAX_PIN_SIZE - 1), pszUICC,
                        PIN_CACHE_FILE_FS);
            }
        }
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// Returns PIN code previously cached and paired with the given UICC Id.
// Input: UICC Id
// Output: {NOK, invalid UICC},{NOK, wrong integrity},{NOK, No PIN available},{OK}
//
ePCache_Code PCache_Get_PIN(const char* pszUICC, char* pszPIN)
{
    ePCache_Code ret = PIN_NOK;

    if (NULL != pszUICC && '\0' != pszUICC[0] && NULL != pszPIN)
    {
        UINT32 uiPinCacheMode = CTE::GetTE().GetPinCacheMode();

        if (E_PIN_CACHE_MODE_NVRAM == uiPinCacheMode)
        {
            ret = decrypt(pszPIN, pszUICC, PIN_CACHE_DUMP_FILE);
        }
        else if (E_PIN_CACHE_MODE_FS == uiPinCacheMode)
        {
            ret = decrypt(pszPIN, pszUICC, PIN_CACHE_FILE_FS);
        }
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// Clear PIN code cache.
// Input: None
// Output: None
//
void PCache_Clear()
{
    UINT32 uiPinCacheMode = CTE::GetTE().GetPinCacheMode();
    RIL_LOG_VERBOSE("PCache_Clear Enter \r\n");
    unsigned char clear[CACHED_UICCID_PIN_SIZE] = {'\0'};
    if(PCache_Write(clear) != PIN_OK)
    {
        RIL_LOG_INFO("PCache_Clear PCache_Write return error \r\n");
        goto Error;
    }

Error:
    RIL_LOG_VERBOSE("PCache_Clear Exit \r\n");
}
