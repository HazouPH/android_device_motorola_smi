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


#include "types.h"
#include "rillog.h"
#include "util.h"
#include "sync_ops.h"
#include "rilqueue.h"
#include "rilchannels.h"
#include "response.h"
#include "repository.h"
#include "te.h"
#include "rildmain.h"
#include "mmgr_cli.h"
#include "reset.h"
#include "initializer.h"
#include "systemmanager.h"
#include "hardwareconfig.h"

#include <utils/Log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

// Tx Queue
CRilQueue<CCommand*>* g_pTxQueue[RIL_CHANNEL_MAX];
CEvent* g_TxQueueEvent[RIL_CHANNEL_MAX];

// Rx Queue
CRilQueue<CResponse*>* g_pRxQueue[RIL_CHANNEL_MAX];
CEvent* g_RxQueueEvent[RIL_CHANNEL_MAX];

//  Array of CChannels
CChannel* g_pRilChannel[RIL_CHANNEL_MAX] = { NULL };

CSystemManager* CSystemManager::m_pInstance = NULL;

CSystemManager& CSystemManager::GetInstance()
{
    //RIL_LOG_VERBOSE("CSystemManager::GetInstance() - Enter\r\n");
    if (!m_pInstance)
    {
        m_pInstance = new CSystemManager;
        if (!m_pInstance)
        {
            RIL_LOG_CRITICAL("CSystemManager::GetInstance() - Cannot create instance\r\n");

            //  Can't call TriggerRadioError here as SystemManager isn't even up yet.
            //  Just call exit and let rild clean everything up.
            exit(0);
        }
    }
    //RIL_LOG_VERBOSE("CSystemManager::GetInstance() - Exit\r\n");
    return *m_pInstance;
}

BOOL CSystemManager::Destroy()
{
    RIL_LOG_INFO("CSystemManager::Destroy() - Enter\r\n");
    if (m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }
    else
    {
        RIL_LOG_VERBOSE("CSystemManager::Destroy() - WARNING - Called with no instance\r\n");
    }
    RIL_LOG_INFO("CSystemManager::Destroy() - Exit\r\n");
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
CSystemManager::CSystemManager()
  : m_pInitializer(NULL),
    m_pCancelWaitEvent(NULL),
    m_pSysInitCompleteEvent(NULL),
    m_pDataChannelAccessorMutex(NULL),
    m_pMMgrLibHandle(NULL),
    m_RequestInfoTable(),
    m_bIsSystemInitialized(FALSE),
    m_bIsModemResourceAcquired(FALSE),
    m_bIsDeviceDecrypted(FALSE)
{
    RIL_LOG_INFO("CSystemManager::CSystemManager() - Enter\r\n");

    // TODO / FIXME : Someone is locking this mutex outside of the destructor or system init
    //                functions
    //                Need to track down when time is available. Workaround for now is to call
    //                TryLock
    //                so we don't block during suspend.
    m_pSystemManagerMutex = new CMutex();

    m_pSpoofCommandsStatusAccessMutex = new CMutex();

    m_pTEAccessMutex = new CMutex();

    RIL_LOG_INFO("CSystemManager::CSystemManager() - Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////
CSystemManager::~CSystemManager()
{
    RIL_LOG_INFO("CSystemManager::~CSystemManager() - Enter\r\n");
    BOOL fLocked = TRUE;

    for (int x = 0; x < 3; x++)
    {
        Sleep(300);

        if (!CMutex::TryLock(m_pSystemManagerMutex))
        {
            RIL_LOG_CRITICAL("CSystemManager::~CSystemManager() - Failed to lock mutex!\r\n");
            fLocked = FALSE;
        }
        else
        {
            RIL_LOG_INFO("CSystemManager::~CSystemManager() - DEBUG: Mutex Locked!\r\n");
            fLocked = TRUE;
            break;
        }
    }

    m_bIsSystemInitialized = FALSE;

    if (m_pInitializer)
    {
        RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before CloseChannelPorts\r\n");
        // Close the COM ports
        m_pInitializer->CloseChannelPorts();

        RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before DeleteChannels\r\n");
        //  Delete channels
        m_pInitializer->DeleteChannels();
    }

    // destroy events
    if (m_pCancelWaitEvent)
    {
        RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before delete m_pCancelWaitEvent\r\n");
        delete m_pCancelWaitEvent;
        m_pCancelWaitEvent = NULL;
    }

    RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before DeleteQueues\r\n");
    // free queues
    DeleteQueues();

    if (m_pSysInitCompleteEvent)
    {
        RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before delete"
                " m_pSysInitCompleteEvent\r\n");
        delete m_pSysInitCompleteEvent;
        m_pSysInitCompleteEvent = NULL;
    }

    if (m_pDataChannelAccessorMutex)
    {
        RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before delete"
                " m_pDataChannelAccessorMutex\r\n");
        delete m_pDataChannelAccessorMutex;
        m_pDataChannelAccessorMutex = NULL;
    }

    RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before delete TE object\r\n");
    CTE::GetTE().DeleteTEObject();
    m_pInitializer = NULL;

    if (m_pSpoofCommandsStatusAccessMutex)
    {
        CMutex::Unlock(m_pSpoofCommandsStatusAccessMutex);
        RIL_LOG_INFO("CSystemManager::~CSystemManager() - "
                "Before delete m_pSpoofCommandsStatusAccessMutex\r\n");
        delete m_pSpoofCommandsStatusAccessMutex;
        m_pSpoofCommandsStatusAccessMutex = NULL;
    }

    if (m_pTEAccessMutex)
    {
        CMutex::Unlock(m_pTEAccessMutex);
        RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before delete m_pTEAccessMutex\r\n");
        delete m_pTEAccessMutex;
        m_pTEAccessMutex = NULL;
    }

    if (fLocked)
    {
        CMutex::Unlock(m_pSystemManagerMutex);
    }

    if (m_pSystemManagerMutex)
    {
        RIL_LOG_INFO("CSystemManager::~CSystemManager() - Before delete"
                " m_pSystemManagerMutex\r\n");
        delete m_pSystemManagerMutex;
        m_pSystemManagerMutex = NULL;
    }

    RIL_LOG_INFO("CSystemManager::~CSystemManager() - Exit\r\n");
}


///////////////////////////////////////////////////////////////////////////////
// Start initialization
//
BOOL CSystemManager::InitializeSystem()
{
    RIL_LOG_INFO("CSystemManager::InitializeSystem() - Enter\r\n");

    CMutex::Lock(m_pSystemManagerMutex);

    CRepository repository;
    int iTemp = 0;
    BOOL bRetVal = FALSE;

    char szModem[MAX_MODEM_NAME_LEN];
    UINT32 uiModemType = MODEM_TYPE_UNKNOWN;

    char szBuildTypeProperty[PROPERTY_VALUE_MAX] = {'\0'};
    char szImsSupport[PROPERTY_VALUE_MAX] = {'\0'};

    // read the modem type used from repository
    if (repository.Read(g_szGroupModem, g_szSupportedModem, szModem, MAX_MODEM_NAME_LEN))
     {
        if (0 == strcmp(szModem, szXMM6260))
        {
            RIL_LOG_INFO("CSystemManager::InitializeSystem() - Using XMM6260\r\n");
            uiModemType = MODEM_TYPE_XMM6260;
        }
        else if (0 == strcmp(szModem, szXMM6360))
        {
            RIL_LOG_INFO("CSystemManager::InitializeSystem() - Using XMM6360\r\n");
            uiModemType = MODEM_TYPE_XMM6360;
        }
        else if (0 == strcmp(szModem, szXMM7160))
        {
            RIL_LOG_INFO("CSystemManager::InitializeSystem() - Using XMM7160\r\n");
            uiModemType = MODEM_TYPE_XMM7160;
        }
        else if (0 == strcmp(szModem, szXMM7260))
        {
            RIL_LOG_INFO("CSystemManager::InitializeSystem() - Using XMM7260\r\n");
            uiModemType = MODEM_TYPE_XMM7260;
        }
        else
        {
            RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() - Unknown modem type-"
                    " Calling exit(0)\r\n");
            exit(0);
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() -"
                " Failed to read the modem type!\r\n");
        goto Done;
    }

    if (m_pSysInitCompleteEvent)
    {
        RIL_LOG_WARNING("CSystemManager::InitializeSystem() - WARN: m_pSysInitCompleteEvent was"
                " already created!\r\n");
    }
    else
    {
        m_pSysInitCompleteEvent = new CEvent(NULL, TRUE);
        if (!m_pSysInitCompleteEvent)
        {
            RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() - Could not create System init"
                    " complete Event.\r\n");
            goto Done;
        }
    }

    if (m_pDataChannelAccessorMutex)
    {
        RIL_LOG_WARNING("CSystemManager::InitializeSystem() - WARN: m_pDataChannelAccessorMutex was"
                " already created!\r\n");
    }
    else
    {
        m_pDataChannelAccessorMutex = new CMutex();
        if (!m_pDataChannelAccessorMutex)
        {
            RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() - Could not create"
                    " m_pDataChannelAccessorMutex.\r\n");
            goto Done;
        }
    }

    // The modem-specific TE Object is created here. This should be done before the
    // channels start sending the initialization commands.
    CTE::CreateTE(uiModemType);

    // Get an initializer to create the channels and initialize the modem.
    // CTEBase is responsible for the deletion of the initializer object in its destructor.
    m_pInitializer = CTE::GetTE().GetInitializer();
    if (NULL == m_pInitializer)
    {
        RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() - Failed to get Initializer!");
        goto Done;
    }

    if (!m_pInitializer->Initialize())
    {
        RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() - Failed to init Initializer!\r\n");
        goto Done;
    }

    ResetSystemState();

    if (repository.Read(g_szGroupModem, g_szEnableModemOffInFlightMode, iTemp))
    {
        CTE::GetTE().SetModemOffInFlightModeState((UINT32)iTemp);
    }

    if (repository.Read(g_szGroupOtherTimeouts, g_szTimeoutCmdInit, iTemp))
    {
        CTE::GetTE().SetTimeoutCmdInit((UINT32)iTemp);
    }

    if (repository.Read(g_szGroupOtherTimeouts, g_szTimeoutAPIDefault, iTemp))
    {
        CTE::GetTE().SetTimeoutAPIDefault((UINT32)iTemp);
    }

    if (repository.Read(g_szGroupOtherTimeouts, g_szTimeoutWaitForInit, iTemp))
    {
        CTE::GetTE().SetTimeoutWaitForInit((UINT32)iTemp);
    }

    if (repository.Read(g_szGroupRILSettings, g_szTimeoutThresholdForRetry, iTemp))
    {
        CTE::GetTE().SetTimeoutThresholdForRetry((UINT32)iTemp);
    }

    if (repository.Read(g_szGroupRILSettings, g_szPinCacheMode, iTemp))
    {
        CTE::GetTE().SetPinCacheMode((UINT32)iTemp);
    }

    // store initial value of Fast Dormancy Mode
    if (repository.Read(g_szGroupModem, g_szFDMode, iTemp))
    {
        CTE::GetTE().SetFastDormancyMode((UINT32)iTemp);
    }

    // get system capabilities from repository
    if (repository.Read(g_szGroupModem, g_szVoiceCapable, iTemp))
    {
        CTE::GetTE().SetVoiceCapable(iTemp == 1 ? TRUE : FALSE);
    }

    if (repository.Read(g_szGroupModem, g_szDataCapable, iTemp))
    {
        CTE::GetTE().SetDataCapable(iTemp == 1 ? TRUE : FALSE);
    }

    if (repository.Read(g_szGroupModem, g_szSmsOverCSCapable, iTemp))
    {
        CTE::GetTE().SetSmsOverCSCapable(iTemp == 1 ? TRUE : FALSE);
    }

    if (repository.Read(g_szGroupModem, g_szSmsOverPSCapable, iTemp))
    {
        CTE::GetTE().SetSmsOverPSCapable(iTemp == 1 ? TRUE : FALSE);
    }

    CTE::GetTE().SetSmsCapable(CTE::GetTE().IsSmsOverCSCapable()
            || CTE::GetTE().IsSmsOverPSCapable());

    if (repository.Read(g_szGroupModem, g_szStkCapable, iTemp))
    {
        CTE::GetTE().SetStkCapable(iTemp == 1 ? TRUE : FALSE);
    }

    if (repository.Read(g_szGroupModem, g_szEnableXDATASTATURC, iTemp))
    {
        CTE::GetTE().SetXDATASTATReporting(iTemp == 1 ? TRUE : FALSE);
    }

    if (repository.Read(g_szGroupModem, g_szSupportCGPIAF, iTemp))
    {
        CTE::GetTE().SetSupportCGPIAF(iTemp == 1 ? TRUE : FALSE);
    }

    if (repository.Read(g_szGroupModem, g_szEnableSignalStrengthURC, iTemp))
    {
        CTE::GetTE().SetSignalStrengthReporting(iTemp == 1 ? TRUE : FALSE);
    }

    if (repository.Read(g_szGroupModem, g_szEnableCellInfo, iTemp))
    {
        CTE::GetTE().SetCellInfoEnabled(iTemp == 1 ? TRUE : FALSE);
    }

    if  (repository.Read(g_szGroupModem, g_szTempOoSNotificationEnable, iTemp))
    {
        CTE::GetTE().SetTempOoSNotificationReporting(iTemp == 1 ? TRUE : FALSE);
    }

    // Retrieve IMS capability based on system property
    if (property_get("persist.ims_support", szImsSupport, "0"))
    {
        // ims_support = 1 means IMS Stack is BP centric
        // ims_support = 2 means IMS Stack is AP centric
        CTE::GetTE().SetIMSCapable((strncmp(szImsSupport, "1",
                PROPERTY_VALUE_MAX) == 0) ? TRUE : FALSE);

        // ims_support = 2 means IMS Stack is AP centric
        CTE::GetTE().SetIMSApCentric((strncmp(szImsSupport, "2",
                PROPERTY_VALUE_MAX) == 0) ? TRUE : FALSE);
    }

    if (CTE::GetTE().IsIMSCapable())
    {
        // Set SMS over IMS support in case of BP centric
        if (repository.Read(g_szGroupModem, g_szEnableSMSOverIP, iTemp))
        {
            CTE::GetTE().SetSMSOverIPCapable(iTemp == 1 ? TRUE : FALSE);
        }
    }

    // Call drop reporting is available only for eng or userdebug build
    if (property_get("ro.build.type", szBuildTypeProperty, NULL))
    {
        const char szTypeEng[] = "eng";
        const char szTypeUserDebug[] = "userdebug";
        if ((strncmp(szBuildTypeProperty, szTypeEng, strlen(szTypeEng)) == 0)
                || (strncmp(szBuildTypeProperty, szTypeUserDebug, strlen(szTypeUserDebug)) == 0))
        {
            if (repository.Read(g_szGroupLogging, g_szCallDropReporting, iTemp))
            {
                CTE::GetTE().SetCallDropReportingState(iTemp == 1 ? TRUE : FALSE);
            }
        }
    }

    //  Create and initialize the channels (don't open ports yet)
    if (!m_pInitializer->CreateChannels())
    {
        RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() - Failed to create channels!\r\n");
        goto Done;
    }

    //  Need to establish communication with MMgr here.
    if (!MMgrConnectionInit())
    {
        RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() - Unable to connect to MMgr lib\r\n");
        goto Done;
    }

    bRetVal = TRUE;

Done:
    if (!bRetVal)
    {
        if (m_pSysInitCompleteEvent)
        {
            delete m_pSysInitCompleteEvent;
            m_pSysInitCompleteEvent = NULL;
        }

        if (m_pDataChannelAccessorMutex)
        {
            delete m_pDataChannelAccessorMutex;
            m_pDataChannelAccessorMutex = NULL;
        }

        if (m_pMMgrLibHandle)
        {
            mmgr_cli_disconnect(m_pMMgrLibHandle);
            mmgr_cli_delete_handle(m_pMMgrLibHandle);
            m_pMMgrLibHandle = NULL;
        }

        CTE::GetTE().DeleteTEObject();
        m_pInitializer = NULL;
    }

    CMutex::Unlock(m_pSystemManagerMutex);

    if (bRetVal)
    {
        if (IsDeviceDecrypted())
        {
            char szWakeSrc[PROPERTY_VALUE_MAX] = {'\0'};

            if (property_get("ro.boot.wakesrc", szWakeSrc, "") > 0)
            {
                const int WAKE_KERNEL_WATCHDOG_RESET = 0x08;
                const int WAKE_SECURITY_WATCHDOG_RESET = 0x09;

                LONG wakeSrc = strtol(szWakeSrc, NULL, 16);
                if (wakeSrc != WAKE_KERNEL_WATCHDOG_RESET
                        && wakeSrc != WAKE_SECURITY_WATCHDOG_RESET)
                {
                    PCache_Clear();
                }
            }
            else
            {
                RIL_LOG_INFO("CSystemManager::InitializeSystem(): wake src not known\r\n");
                PCache_Clear();
            }

            if (!GetModem())
            {
                RIL_LOG_CRITICAL("CSystemManager::InitializeSystem() : "
                        "GetModem Resource failed\r\n");

                CTE::GetTE().SetRestrictedMode(TRUE);
            }
            else
            {
                RIL_LOG_INFO("CSystemManager::InitializeSystem() : Waiting for "
                        "System Initialization Complete event\r\n");
                CTE::GetTE().SetRestrictedMode(FALSE);
                CEvent::Wait(m_pSysInitCompleteEvent, WAIT_FOREVER);
            }
        }

        RIL_LOG_INFO("CSystemManager::InitializeSystem() : Rapid Ril initialization completed\r\n");
    }

    RIL_LOG_INFO("CSystemManager::InitializeSystem() - Exit\r\n");

    return bRetVal;
}


///////////////////////////////////////////////////////////////////////////////
//  This function continues the init in the function InitializeSystem() left
//  off from InitChannelPorts().  Called when MODEM_UP status is received.
BOOL CSystemManager::ContinueInit()
{
    RIL_LOG_INFO("CSystemManager::ContinueInit() - ENTER\r\n");

    BOOL bRetVal = FALSE;

    CMutex::Lock(m_pSystemManagerMutex);

    // Open the serial ports only (g_pRilChannel should already be populated)
    if (!m_pInitializer->OpenChannelPortsOnly())
    {
        RIL_LOG_CRITICAL("CSystemManager::ContinueInit() - Couldn't open VSPs.\r\n");
        goto Done;
    }
    RIL_LOG_INFO("CSystemManager::ContinueInit() - VSPs were opened successfully.\r\n");

    m_pCancelWaitEvent = new CEvent(NULL, TRUE);
    if (NULL == m_pCancelWaitEvent)
    {
        RIL_LOG_CRITICAL("CSystemManager::ContinueInit() - Could not create exit event.\r\n");
        goto Done;
    }

    // Create the Queues
    if (!CreateQueues())
    {
        RIL_LOG_CRITICAL("CSystemManager::ContinueInit() - Unable to create queues\r\n");
        goto Done;
    }

    if (!CThreadManager::Start(g_uiRilChannelCurMax * 2))
    {
        RIL_LOG_CRITICAL("CSystemManager::ContinueInit() - Thread manager failed to start.\r\n");
    }

    m_bIsSystemInitialized = TRUE;

    if (!m_pInitializer->InitializeModem())
    {
        RIL_LOG_CRITICAL("CSystemManager::ContinueInit() -"
                " Couldn't start Modem initialization!\r\n");
        goto Done;
    }

    bRetVal = TRUE;

    if (CTE::GetTE().GetModemOffInFlightModeState() &&
            (RADIO_STATE_OFF != CTE::GetTE().GetRadioState()))
    {
        CTE::GetTE().SetRadioStateAndNotify(RRIL_RADIO_STATE_OFF);
    }

    // Signal that we have initialized, so that framework
    // can start using the rild socket.
    CEvent::Signal(m_pSysInitCompleteEvent);

Done:
    if (!bRetVal)
    {
        m_bIsSystemInitialized = FALSE;
    }

    CMutex::Unlock(m_pSystemManagerMutex);

    return bRetVal;
    RIL_LOG_INFO("CSystemManager::ContinueInit() - EXIT\r\n");
}

///////////////////////////////////////////////////////////////////////////////
void CSystemManager::ResetSystemState()
{
    RIL_LOG_VERBOSE("CSystemManager::ResetSystemState() - Enter\r\n");

    SetInitializationUnsuccessful();
    m_pInitializer->ResetChannelCompletedInit();
    m_pInitializer->ResetStartupEvents();

    RIL_LOG_VERBOSE("CSystemManager::ResetSystemState() - Exit\r\n");
}

void CSystemManager::ResetChannelInfo()
{
    RIL_LOG_INFO("CSystemManager::ResetChannelInfo() - Enter\r\n");

    CMutex::Lock(m_pSystemManagerMutex);

    m_pInitializer->ResetChannelCompletedInit();

    if (m_pCancelWaitEvent)
    {
        delete m_pCancelWaitEvent;
        m_pCancelWaitEvent = NULL;
    }

    // free queues
    DeleteQueues();

    CMutex::Unlock(m_pSystemManagerMutex);

    RIL_LOG_INFO("CSystemManager::ResetChannelInfo() - Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////
BOOL CSystemManager::CreateQueues()
{
    RIL_LOG_VERBOSE("CSystemManager::CreateQueues() - Enter\r\n");
    BOOL bRet = FALSE;

    // Create command and response queues
    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; ++i)
    {
        if (NULL == (g_TxQueueEvent[i] = new CEvent(NULL, FALSE))     ||
            NULL == (g_pTxQueue[i] = new CRilQueue<CCommand*>(true)) ||
            NULL == (g_RxQueueEvent[i] = new CEvent(NULL, FALSE))     ||
            NULL == (g_pRxQueue[i] = new CRilQueue<CResponse*>(true)))
        {
            RIL_LOG_VERBOSE("CSystemManager::CreateQueues() - ERROR: Out of memory\r\n");
            goto Done;
        }
    }

    bRet = TRUE;

Done:
    if (!bRet)
    {
        DeleteQueues();
    }

    RIL_LOG_VERBOSE("CSystemManager::CreateQueues() - Exit\r\n");
    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
void CSystemManager::DeleteQueues()
{
    RIL_LOG_VERBOSE("CSystemManager::DeleteQueues() - Enter\r\n");

    for (UINT32 i = 0; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; ++i)
    {
        delete g_TxQueueEvent[i];
        g_TxQueueEvent[i] = NULL;

        delete g_pTxQueue[i];
        g_pTxQueue[i] = NULL;

        delete g_RxQueueEvent[i];
        g_RxQueueEvent[i] = NULL;

        delete g_pRxQueue[i];
        g_pRxQueue[i] = NULL;
    }

    RIL_LOG_VERBOSE("CSystemManager::DeleteQueues() - Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////
// Test the exit event
//
BOOL CSystemManager::IsExitRequestSignalled() const
{
    RIL_LOG_VERBOSE("CSystemManager::IsExitRequestSignalled() - Enter\r\n");

    BOOL bRetVal = WAIT_EVENT_0_SIGNALED == CEvent::Wait(m_pCancelWaitEvent, 0);

    RIL_LOG_VERBOSE("CSystemManager::IsExitRequestSignalled() - Result: %s\r\n",
            bRetVal ? "Set" : "Not Set");
    RIL_LOG_VERBOSE("CSystemManager::IsExitRequestSignalled() - Exit\r\n");
    return bRetVal;
}

///////////////////////////////////////////////////////////////////////////////
void CSystemManager::GetRequestInfo(int reqID, REQ_INFO& rReqInfo)
{
    m_RequestInfoTable.GetRequestInfo(reqID, rReqInfo);
}

///////////////////////////////////////////////////////////////////////////////
// This function initialize the connection with MMgr.
// MMgr handler is stored in the CSystemManager class.
BOOL CSystemManager::MMgrConnectionInit()
{
    RIL_LOG_INFO("CSystemManager::MMgrConnectionInit() - ENTER\r\n");

    BOOL bRet = FALSE;
    const int NUM_LOOPS = 10;
    const int SLEEP_MS = 1000;  // 1 sec between retries
    int subscriptionId = CHardwareConfig::GetInstance().GetSubscriptionId();
    int modemId = CHardwareConfig::GetInstance().GetModemId();

    char RRIL_NAME[CLIENT_NAME_LEN] = "RRIL";

    // Initialize internal state to unknown.
    CTE::GetTE().SetLastModemEvent(MODEM_STATE_UNKNOWN);

    if (subscriptionId)
    {
        snprintf(RRIL_NAME, CLIENT_NAME_LEN, "RRIL%d", subscriptionId);
    }

    if (E_ERR_CLI_SUCCEED != mmgr_cli_create_handle(&m_pMMgrLibHandle, RRIL_NAME, NULL))
    {
        m_pMMgrLibHandle = NULL;
        RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() -"
                         " Cannot create handle\n");
        goto out;
    }

    if (E_ERR_CLI_SUCCEED !=
          mmgr_cli_subscribe_event(m_pMMgrLibHandle,
                                     ModemManagerEventHandler,
                                     E_MMGR_EVENT_MODEM_UP))
    {
        RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() -"
                         " Cannot subscribe event %d\n",
                          E_MMGR_EVENT_MODEM_UP);
        goto out;
    }

    if (E_ERR_CLI_SUCCEED !=
          mmgr_cli_subscribe_event(m_pMMgrLibHandle,
                                     ModemManagerEventHandler,
                                     E_MMGR_EVENT_MODEM_DOWN))
    {
        RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() -"
                         " Cannot subscribe event %d\n",
                          E_MMGR_EVENT_MODEM_UP);
        goto out;
    }

    if (E_ERR_CLI_SUCCEED !=
          mmgr_cli_subscribe_event(m_pMMgrLibHandle,
                                     ModemManagerEventHandler,
                                     E_MMGR_EVENT_MODEM_OUT_OF_SERVICE))
    {
        RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() -"
                         " Cannot subscribe event %d\n",
                          E_MMGR_EVENT_MODEM_OUT_OF_SERVICE);
        goto out;
    }

    if (E_ERR_CLI_SUCCEED !=
          mmgr_cli_subscribe_event(m_pMMgrLibHandle,
                                     ModemManagerEventHandler,
                                     E_MMGR_NOTIFY_MODEM_COLD_RESET))
    {
        RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() -"
                         " Cannot subscribe notification %d\n",
                          E_MMGR_NOTIFY_MODEM_COLD_RESET);
        goto out;
    }

    if (E_ERR_CLI_SUCCEED !=
          mmgr_cli_subscribe_event(m_pMMgrLibHandle,
                                     ModemManagerEventHandler,
                                     E_MMGR_NOTIFY_MODEM_SHUTDOWN))
    {
        RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() -"
                         " Cannot subscribe notification %d\n",
                          E_MMGR_NOTIFY_MODEM_SHUTDOWN);
        goto out;
    }

    if (E_ERR_CLI_SUCCEED !=
          mmgr_cli_subscribe_event(m_pMMgrLibHandle,
                                     ModemManagerEventHandler,
                                     E_MMGR_NOTIFY_PLATFORM_REBOOT))
    {
        RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() -"
                         " Cannot subscribe notification %d\n",
                          E_MMGR_NOTIFY_PLATFORM_REBOOT);
        goto out;
    }

    if (E_ERR_CLI_SUCCEED !=
          mmgr_cli_subscribe_event(m_pMMgrLibHandle,
                                     ModemManagerEventHandler,
                                     E_MMGR_NOTIFY_CORE_DUMP))
    {
        RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() -"
                         " Cannot subscribe notification %d\n",
                          E_MMGR_NOTIFY_CORE_DUMP);
        goto out;
    }

    //  TODO: Change looping formula

    for (int i = 0; i < NUM_LOOPS; i++)
    {
        RIL_LOG_INFO("CSystemManager::MMgrConnectionInit() -"
                     " Attempting to connect to MMgr try=[%d] out of %d\r\n",
                       i+1,
                       NUM_LOOPS);

        if (E_ERR_CLI_SUCCEED != mmgr_cli_connect(m_pMMgrLibHandle))
        {
            if (i+1 < NUM_LOOPS)
                RIL_LOG_WARNING("CSystemManager::MMgrConnectionInit() "
                             "- Cannot connect to MMgr\r\n");
            else
                RIL_LOG_CRITICAL("CSystemManager::MMgrConnectionInit() "
                             "- Cannot connect to MMgr after %d tries\r\n",
                             NUM_LOOPS);

            Sleep(SLEEP_MS);
        }
        else
        {
            RIL_LOG_INFO("CSystemManager::MMgrConnectionInit() -"
                         " *** Connection opened ***\r\n");
            bRet = TRUE;
            break;
        }
    }

out:
    if (!bRet && (m_pMMgrLibHandle != NULL))
    {
        mmgr_cli_delete_handle(m_pMMgrLibHandle);
        m_pMMgrLibHandle = NULL;
    }

    RIL_LOG_INFO("CSystemManager::MMgrConnectionInit() - EXIT\r\n");
    return bRet;
}


//  Send recovery request to MMgr
BOOL CSystemManager::SendRequestModemRecovery(mmgr_cli_recovery_cause_t* pCauses, int nCauses)
{
    RIL_LOG_INFO("CSystemManager::SendRequestModemRecovery() - ENTER\r\n");
    BOOL bRet = FALSE;
    mmgr_cli_requests_t request;

    MMGR_CLI_INIT_REQUEST(request, E_MMGR_REQUEST_MODEM_RECOVERY);
    request.len = nCauses * sizeof(mmgr_cli_recovery_cause_t);
    request.data = pCauses;

    if (m_pMMgrLibHandle)
    {
        RIL_LOG_INFO("CSystemManager::SendRequestModemRecovery() -"
                     " Send recovery request\r\n");

        if (E_ERR_CLI_SUCCEED != mmgr_cli_send_msg(m_pMMgrLibHandle, &request))
        {
            RIL_LOG_CRITICAL("CSystemManager::SendRequestModemRecovery() -"
                             " Failed to send REQUEST_MODEM_RECOVERY\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CSystemManager::SendRequestModemRecovery() -"
                         " Send request clean up  SUCCESSFUL\r\n");
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CSystemManager::SendRequestModemRecovery() -"
                         " unable to communicate with MMgr\r\n");
        goto Error;
    }

    bRet = TRUE;
Error:
    RIL_LOG_INFO("CSystemManager::SendRequestModemRecovery() - EXIT\r\n");
    return bRet;
}

//  Send shutdown request to MMgr
BOOL CSystemManager::SendRequestModemShutdown()
{
    RIL_LOG_INFO("CSystemManager::SendRequestModemShutdown() - ENTER\r\n");
    BOOL bRet = FALSE;
    mmgr_cli_requests_t request;

    MMGR_CLI_INIT_REQUEST(request, E_MMGR_REQUEST_FORCE_MODEM_SHUTDOWN);

    if (m_pMMgrLibHandle)
    {
        RIL_LOG_INFO("CSystemManager::SendRequestModemShutdown() -"
                     " Send request modem force shutdown\r\n");

        if (E_ERR_CLI_SUCCEED != mmgr_cli_send_msg(m_pMMgrLibHandle, &request))
        {
            RIL_LOG_CRITICAL("CSystemManager::SendRequestModemShutdown() -"
                             " Failed to send REQUEST_FORCE_MODEM_SHUTDOWN\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CSystemManager::SendRequestModemShutdown() -"
                         " Send request modem force shutdown SUCCESSFUL\r\n");
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CSystemManager::SendRequestModemShutdown() -"
                         " unable to communicate with MMgr\r\n");
        goto Error;
    }

    bRet = TRUE;
Error:
    RIL_LOG_INFO("CSystemManager::SendRequestModemShutdown() - EXIT\r\n");
    return bRet;
}

//  Send shutdown acknowledge to MMgr
BOOL CSystemManager::SendAckModemShutdown()
{
    RIL_LOG_INFO("CSystemManager::SendAckModemShutdown() - ENTER\r\n");
    BOOL bRet = FALSE;
    mmgr_cli_requests_t request;

    MMGR_CLI_INIT_REQUEST(request, E_MMGR_ACK_MODEM_SHUTDOWN);

    if (m_pMMgrLibHandle)
    {
        RIL_LOG_INFO("CSystemManager::SendAckModemShutdown() -"
                     " Acknowledging modem force shutdown\r\n");

        if (E_ERR_CLI_SUCCEED != mmgr_cli_send_msg(m_pMMgrLibHandle, &request))
        {
            RIL_LOG_CRITICAL("CSystemManager::SendAckModemShutdown() -"
                             " Failed to send REQUEST_ACK_MODEM_SHUTDOWN\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CSystemManager::SendAckModemShutdown() -"
                         " Modem force shutdown acknowledge SUCCESSFUL\r\n");
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CSystemManager::SendAckModemShutdown() -"
                         " unable to communicate with MMgr\r\n");
        goto Error;
    }

    bRet = TRUE;
Error:
    RIL_LOG_INFO("CSystemManager::SendAckModemShutdown() - EXIT\r\n");
    return bRet;
}

//  Send cold reset acknowledge to MMgr
BOOL CSystemManager::SendAckModemColdReset()
{
    RIL_LOG_INFO("CSystemManager::SendAckModemColdReset() - ENTER\r\n");
    BOOL bRet = FALSE;
    mmgr_cli_requests_t request;

    MMGR_CLI_INIT_REQUEST(request, E_MMGR_ACK_MODEM_COLD_RESET);

    if (m_pMMgrLibHandle)
    {
        RIL_LOG_INFO("CSystemManager::SendAckModemColdReset() -"
                     " Acknowledging cold reset \r\n");

        if (E_ERR_CLI_SUCCEED != mmgr_cli_send_msg(m_pMMgrLibHandle, &request))
        {
            RIL_LOG_CRITICAL("CSystemManager::SendAckModemColdReset() -"
                             " Failed to send ACK_MODEM_COLD_RESET\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CSystemManager::SendAckModemColdReset() -"
                         " Cold reset acknowledge SUCCESSFUL\r\n");
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CSystemManager::SendAckModemColdReset() -"
                         " unable to communicate with MMgr\r\n");
        goto Error;
    }

    bRet = TRUE;
Error:
    RIL_LOG_INFO("CSystemManager::SendAckModemColdReset() - EXIT\r\n");
    return bRet;
}

//  Get the modem resource
BOOL CSystemManager::GetModem()
{
    RIL_LOG_INFO("CSystemManager::GetModem() - ENTER\r\n");
    BOOL bRet = FALSE;
    e_err_mmgr_cli_t errorCode;

    if (m_bIsModemResourceAcquired)
    {
        RIL_LOG_INFO("CSystemManager::GetModem() - Modem resource already acquired\r\n");
        return TRUE;
    }

    if (m_pMMgrLibHandle)
    {
        RIL_LOG_INFO("CSystemManager::GetModem() - Getting modem resource\r\n");

        errorCode = mmgr_cli_lock(m_pMMgrLibHandle);
        if (E_ERR_CLI_SUCCEED != errorCode &&
                E_ERR_CLI_ALREADY_LOCK != errorCode)
        {
            RIL_LOG_CRITICAL("CSystemManager::GetModem() - Failed to get modem resource: %d\r\n",
                    errorCode);
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CSystemManager::GetModem() - Modem resource get SUCCESSFUL\r\n");
            m_bIsModemResourceAcquired = TRUE;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CSystemManager::GetModem() - unable to communicate with MMgr\r\n");
        goto Error;
    }

    bRet = TRUE;
Error:
    RIL_LOG_INFO("CSystemManager::GetModem() - EXIT\r\n");
    return bRet;
}

//  Release the modem resource
BOOL CSystemManager::ReleaseModem()
{
    RIL_LOG_INFO("CSystemManager::ReleaseModem() - ENTER\r\n");
    BOOL bRet = FALSE;

    if (!m_bIsModemResourceAcquired)
    {
        RIL_LOG_INFO("CSystemManager::ReleaseModem() - Modem resource already released\r\n");
        return TRUE;
    }

    if (m_pMMgrLibHandle)
    {
        RIL_LOG_INFO("CSystemManager::ReleaseModem() - Releasing modem resource\r\n");

        if (E_ERR_CLI_SUCCEED != mmgr_cli_unlock(m_pMMgrLibHandle))
        {
            RIL_LOG_CRITICAL("CSystemManager::ReleaseModem() - Modem resource release failed\r\n");
            goto Error;
        }
        else
        {
            RIL_LOG_INFO("CSystemManager::ReleaseModem() - Modem resource release SUCCESSFUL\r\n");
            m_bIsModemResourceAcquired = FALSE;
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CSystemManager::ReleaseModem() - unable to communicate with MMgr\r\n");
        goto Error;
    }

    bRet = TRUE;
Error:
    RIL_LOG_INFO("CSystemManager::ReleaseModem() - EXIT\r\n");
    return bRet;
}

BOOL CSystemManager::IsDeviceDecrypted()
{
    RIL_LOG_VERBOSE("CSystemManager::IsDeviceDecrypted() - Enter\r\n");

    if (!m_bIsDeviceDecrypted)
    {
        char cryptState[PROPERTY_VALUE_MAX] = {'\0'};
        char voldDecryptState[PROPERTY_VALUE_MAX] = {'\0'};

        property_get("ro.crypto.state", cryptState, "");
        property_get("vold.decrypt", voldDecryptState, "");
        if ((0 == strcmp(cryptState, "unencrypted"))
                || ((0 == strcmp(cryptState, "encrypted"))
                && (0 == strcmp(voldDecryptState, "trigger_restart_framework"))))
        {
            m_bIsDeviceDecrypted = TRUE;
        }
    }

    RIL_LOG_VERBOSE("CSystemManager::IsDeviceDecrypted() - Exit\r\n");
    return m_bIsDeviceDecrypted;
}
