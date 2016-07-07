////////////////////////////////////////////////////////////////////////////
// usat_init_state_machine.h
//
// Copyright (C) Intel 2013.
//
//
// Description:
//    USAT Init State Machine for modem Rel.10+ with the new 3GPP USAT interface
//
/////////////////////////////////////////////////////////////////////////////

#ifndef USAT_INIT_STATE_MACHINE_H
#define USAT_INIT_STATE_MACHINE_H

class UsatState;
class CCatProfile;
class CMutex;

// This is the state machine that handles USAT init for modem Rel.10+ with the
// new 3GPP USAT interface
class UsatInitStateMachine
{
public:

    enum SimEvent
    {
        SIM_READY = 1,
        SIM_UNAVAILABLE = 2,
        PROFILE_READ = 3,
        SIM_READY_FOR_RESET = 4,
        SIM_READY_TO_ACTIVATE = 5,
        STK_SERVICE_RUNNING = 6
    };

    enum
    {
        MAX_SIZE_PROFILE = 32, // profile is 32 bytes length in 3GPP 31.111 v10.11.0
        PROFILE_READ_NUMBER = 6,
        PROFILE_DOWNLOAD_COMPLETED = 2,
        UICC_ACTIVE = 4
    };

    static const char* DEFAULT_PROFILE;
    static const char* DEFAULT_MASK;

    ~UsatInitStateMachine();
    static UsatInitStateMachine& GetStateMachine();

private:

    static void CreateStateMachine();

    UsatInitStateMachine();

    //  Prevent assignment: Declared but not implemented.
    UsatInitStateMachine(const UsatInitStateMachine& rhs);  // Copy Constructor
    UsatInitStateMachine& operator=(const UsatInitStateMachine& rhs);  //  Assignment operator

    void FreeProfileArray();

    void ProcessNextEvent(UsatState& state, const int event);
    UsatState* CreateNextEvent(int event);

public:

    void SendEvent(const int event);
    void PassToNextEvent(int event);

    void SetAddProfileSupport(const int addProfSupport);
    int GetAddProfileSupport();
    void SetReadyToActivate(const bool isReady);
    bool GetReadyToActivate();
    void SetTeWriteNeeded(bool isWriteNeeded);
    bool GetTeWriteNeeded();
    void SetTeProfileToWrite(const char* pszTeProfile);
    const char* GetTeProfileToWrite();
    void SetProfileArray(char** ppszProf);
    char** GetProfileArray();
    void SetStkServiceRunning(bool isRunning);
    bool GetStkServiceRunning();
    void SetUiccState(const int state);
    int GetUiccState();
    CCatProfile* GetCatProfile();
    bool IsSimResetPossible();

private:

    static UsatInitStateMachine* m_pStateMachineInstance;
    UsatState* m_currState;
    CCatProfile* m_pCatProfile;

    int m_addProfileSupport;
    bool m_isReadyForActivation;
    bool m_isTeWriteNeeded;
    // m_pszTeProfileToWrite memory has been allocated with strdup so needs to be freed with free
    const char* m_pszTeProfileToWrite;
    // m_ppszProfiles memory has been allocated with malloc so needs to be freed with free
    char** m_ppszProfiles;
    bool m_isStkServiceRunning;
    int m_uiccState;
    CMutex* m_pUsatStateLock;
    int m_simEvent;
};

#endif // STATE_MACHINE_H
