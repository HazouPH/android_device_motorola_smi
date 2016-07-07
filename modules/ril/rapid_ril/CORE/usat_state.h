////////////////////////////////////////////////////////////////////////////
// usat_state.h
//
// Copyright (C) Intel 2013.
//
//
// Description:
//    States of the USAT Init State Machine for modem Rel.10+ with the new 3GPP
//    USAT interface
//
/////////////////////////////////////////////////////////////////////////////

#ifndef USAT_STATE_H
#define USAT_STATE_H

#include "types.h"
#include "usat_init_state_machine.h"

class UsatState
{
public:
    enum
    {
        TE_PROFILE_STORAGE = 0,
        MT_PROFILE_STORAGE = 1,
        MT_ONLY_PROFILE_STORAGE = 5
    };

    virtual ~UsatState();

    virtual void run();
    virtual void SimReady();
    virtual void ProfileRead();
    virtual void SimUnavailable();
    virtual void SimReadyForReset();
    virtual void SimReadyToActivate();
    virtual void ProfileToActivate();

protected:

    UsatState(UsatInitStateMachine& usatInitStateMachine);

    UsatInitStateMachine& m_usatInitStateMachine;

};

///////////////////////////////////////////////////////////////////////////////
//
// State subclasses

class InitState : public UsatState
{
public:
    InitState(UsatInitStateMachine& usatInitStateMachine);
    virtual ~InitState();

    void SimReady();
};

class ProfileConfig : public UsatState
{
public:
    ProfileConfig(UsatInitStateMachine& usatInitStateMachine);
    virtual ~ProfileConfig();

    void run();
    void SimUnavailable();
    void ProfileRead();
};

class ProfileProcess : public UsatState
{
public:
    ProfileProcess(UsatInitStateMachine& usatInitStateMachine);
    virtual ~ProfileProcess();

    void run();
    void SimUnavailable();
    void SimReadyForReset();
    void SimReadyToActivate();

private:

    bool CalculateTeProfile(const char* pszTeProfile, const char* pszMtOnlyProfile,
            char* pszTeCalcProfile);
    bool CalculateMtProfile(const char* pszMtDefProfile, const char* pszTeProfile,
        const char* pszMtTeMask, const BYTE* achMask, char* pszMtProfile);
    bool CompareProfiles(const char* pszProfileRead, const char* pszDefaultProfile);
};

class UiccReset : public UsatState
{
public:
    UiccReset(UsatInitStateMachine& usatInitStateMachine);
    virtual ~UiccReset();

    void run();
    void SimReadyToActivate();
};

class WaitingActivate : public UsatState
{
public:
    WaitingActivate(UsatInitStateMachine& usatInitStateMachine);
    virtual ~WaitingActivate();

    void run();
    void SimUnavailable();
    void ProfileToActivate();
};

class ProfileActivation : public UsatState
{
public:
    ProfileActivation(UsatInitStateMachine& usatInitStateMachine);
    virtual ~ProfileActivation();

    void run();
    void SimUnavailable();
};

#endif // USAT_STATE_H
