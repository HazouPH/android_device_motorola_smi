////////////////////////////////////////////////////////////////////////////
// usat_init_state_machine.cpp
//
// Copyright (C) Intel 2013.
//
//
// Description:
//    USAT Init State Machine for modem Rel.10+ with the new 3GPP USAT interface
//
/////////////////////////////////////////////////////////////////////////////

#include "rillog.h"
#include "thread_manager.h"
#include "thread_ops.h"
#include "usat_init_state_machine.h"
#include "usat_state.h"
#include "ccatprofile.h"
#include "sync_ops.h"

UsatInitStateMachine* UsatInitStateMachine::m_pStateMachineInstance = NULL;

const char* UsatInitStateMachine::DEFAULT_PROFILE
        = "0000000000000000000000000000000000000000000000000000000000000000";

const char* UsatInitStateMachine::DEFAULT_MASK
        = "0000000000000000000000000000000000000000000000000000000000000000";

UsatInitStateMachine::UsatInitStateMachine()
 : m_addProfileSupport(0),
   m_isReadyForActivation(false),
   m_isTeWriteNeeded(false),
   m_pszTeProfileToWrite(NULL),
   m_ppszProfiles(NULL),
   m_isStkServiceRunning(false),
   m_uiccState(0),
   m_simEvent(SIM_UNAVAILABLE)
{
    m_pCatProfile = new CCatProfile();
    m_currState = new InitState(*this);
    m_pUsatStateLock = new CMutex();
}

UsatInitStateMachine::~UsatInitStateMachine()
{
    free((void*) m_pszTeProfileToWrite);
    FreeProfileArray();
    delete m_currState;
    delete m_pCatProfile;
    delete m_pUsatStateLock;
}

UsatInitStateMachine& UsatInitStateMachine::GetStateMachine()
{
    if (NULL == m_pStateMachineInstance)
    {
        // WARNING: creation is not protected against multi-threading
        CreateStateMachine();
    }
    return *m_pStateMachineInstance;
}

// Creates the Usat Init State Machine Singleton Object
void UsatInitStateMachine::CreateStateMachine()
{
    if (NULL == m_pStateMachineInstance)
    {
        m_pStateMachineInstance = new UsatInitStateMachine();
        if (NULL == m_pStateMachineInstance)
        {
            RIL_LOG_CRITICAL("UsatInitStateMachine::CreateStateMachine() - "
                    "Unable to create State Machine! EXIT\r\n");
            exit(0);
        }
    }
}

void UsatInitStateMachine::FreeProfileArray()
{
    if (m_ppszProfiles != NULL)
    {
        for (UINT32 i = 0; i < PROFILE_READ_NUMBER; i++)
        {
            free(m_ppszProfiles[i]);
        }
        free(m_ppszProfiles);
        m_ppszProfiles = NULL;
    }
}

void UsatInitStateMachine::ProcessNextEvent(UsatState& state, int event)
{
    switch (event)
    {
        case SIM_READY:
        {
            RIL_LOG_INFO("UsatInitStateMachine::ProcessNextEvent() - SIM READY\r\n");
            state.SimReady();
            break;
        }
        case SIM_UNAVAILABLE:
        {
            RIL_LOG_INFO("UsatInitStateMachine::ProcessNextEvent() - SIM UNAVAILABLE\r\n");
            state.SimUnavailable();
            break;
        }
        case PROFILE_READ:
        {
            RIL_LOG_INFO("UsatInitStateMachine::ProcessNextEvent() - PROFILE READ\r\n");
            state.ProfileRead();
            break;
        }
        case SIM_READY_FOR_RESET:
        {
            RIL_LOG_INFO("UsatInitStateMachine::ProcessNextEvent() - SIM READY FOR RESET\r\n");
            state.SimReadyForReset();
            break;
        }
        case SIM_READY_TO_ACTIVATE:
        {
            RIL_LOG_INFO("UsatInitStateMachine::ProcessNextEvent() - SIM READY TO ACTIVATE\r\n");
            SetReadyToActivate(true);
            state.SimReadyToActivate();
            break;
        }
        case STK_SERVICE_RUNNING:
        {
            RIL_LOG_INFO("UsatInitStateMachine::ProcessNextEvent() - PROFILE ACTIVATION\r\n");
            SetStkServiceRunning(true);
            state.ProfileToActivate();
            break;
        }
        default:
            break;
     }
}

UsatState* UsatInitStateMachine::CreateNextEvent(int event)
{
    switch (event)
    {
        case SIM_UNAVAILABLE:
        {
            RIL_LOG_INFO("UsatInitStateMachine::CreateNextEvent() - SIM UNAVAILABLE\r\n");
            return new InitState(*m_pStateMachineInstance);
        }
        case SIM_READY:
        {
            RIL_LOG_INFO("UsatInitStateMachine::CreateNextEvent() - SIM READY\r\n");
            return new ProfileConfig(*m_pStateMachineInstance);
        }
        case PROFILE_READ:
        {
            RIL_LOG_INFO("UsatInitStateMachine::CreateNextEvent() - PROFILE READ\r\n");
            return new ProfileProcess(*m_pStateMachineInstance);
        }
        case SIM_READY_FOR_RESET:
        {
            RIL_LOG_INFO("UsatInitStateMachine::CreateNextEvent() - SIM READY FOR RESET\r\n");
            return new UiccReset(*m_pStateMachineInstance);
        }
        case SIM_READY_TO_ACTIVATE:
        {
            RIL_LOG_INFO("UsatInitStateMachine::CreateNextEvent() - SIM READY TO ACTIVATE\r\n");
            return new WaitingActivate(*m_pStateMachineInstance);
        }
        case STK_SERVICE_RUNNING:
        {
            RIL_LOG_INFO("UsatInitStateMachine::CreateNextEvent() - PROFILE ACTIVATION\r\n");
            return new ProfileActivation(*m_pStateMachineInstance);
        }
        default:
            return NULL;
     }
}

void UsatInitStateMachine::SendEvent(int event)
{
    // Access mutex
    CMutex::Lock(m_pUsatStateLock);

    ProcessNextEvent(*m_currState, event);

    // release mutex
    CMutex::Unlock(m_pUsatStateLock);
}

void UsatInitStateMachine::PassToNextEvent(int event)
{
    // WARNING: nothing should be done after calling PassToNextEvent() in a state
    delete m_currState;

    m_simEvent = event;
    m_currState = CreateNextEvent(event);
    if (NULL != m_currState)
    {
        m_currState->run();
    }
}

void UsatInitStateMachine::SetAddProfileSupport(int addProfSupport)
{
    m_addProfileSupport = addProfSupport;
}

int UsatInitStateMachine::GetAddProfileSupport()
{
    return m_addProfileSupport;
}

void UsatInitStateMachine::SetReadyToActivate(bool isReady)
{
    m_isReadyForActivation = isReady;
}

bool UsatInitStateMachine::GetReadyToActivate()
{
    return m_isReadyForActivation;
}

void UsatInitStateMachine::SetTeWriteNeeded(bool isWriteNeeded)
{
    m_isTeWriteNeeded = isWriteNeeded;
}

bool UsatInitStateMachine::GetTeWriteNeeded()
{
    return m_isTeWriteNeeded;
}

void UsatInitStateMachine::SetTeProfileToWrite(const char* pszTeProfile)
{
    free((void*) m_pszTeProfileToWrite);
    m_pszTeProfileToWrite = strdup(pszTeProfile);
}

const char* UsatInitStateMachine::GetTeProfileToWrite()
{
    return m_pszTeProfileToWrite;
}

void UsatInitStateMachine::SetProfileArray(char** ppszProf)
{
    FreeProfileArray();
    // taking the ownership of the memory
    m_ppszProfiles = ppszProf;
}

char** UsatInitStateMachine::GetProfileArray()
{
    return m_ppszProfiles;
}

void UsatInitStateMachine::SetStkServiceRunning(bool isRunning)
{
    m_isStkServiceRunning = isRunning;
}

bool UsatInitStateMachine::GetStkServiceRunning()
{
    return m_isStkServiceRunning;
}

void UsatInitStateMachine::SetUiccState(int state)
{
    m_uiccState = state;
}

int UsatInitStateMachine::GetUiccState()
{
    return m_uiccState;
}

CCatProfile* UsatInitStateMachine::GetCatProfile()
{
    return m_pCatProfile;
}

bool UsatInitStateMachine::IsSimResetPossible()
{
    RIL_LOG_INFO("UsatInitStateMachine::IsSimResetPossible() - uiccState: %d, simEvent: %d\r\n",
            m_uiccState, m_simEvent);

    return (m_uiccState != UICC_ACTIVE
            ||  (m_simEvent != SIM_READY_TO_ACTIVATE && m_simEvent != STK_SERVICE_RUNNING));
}
