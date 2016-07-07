////////////////////////////////////////////////////////////////////////////
// usat_state.cpp
//
// Copyright (C) Intel 2013.
//
//
// Description:
//    States of the USAT Init State Machine for modem Rel.10+ with the new 3GPP
//    USAT interface
//
//
/////////////////////////////////////////////////////////////////////////////

#include "usat_state.h"
#include "rillog.h"
#include "te.h"
#include "ccatprofile.h"
#include "util.h"
#include "usat_init_state_machine.h"

UsatState::UsatState(UsatInitStateMachine& usatInitStateMachine)
 : m_usatInitStateMachine(usatInitStateMachine)
{
}

UsatState::~UsatState()
{
}

////////////////////////////////////////////////////////////////

// InitState corresponds to the first state of the state machine.
// It waits for the SIM to be ready to start configuring profile download.

InitState::InitState(UsatInitStateMachine& usatInitStateMachine)
 : UsatState(usatInitStateMachine)
{
}

InitState::~InitState()
{
}

////////////////////////////////////////////////////////////////

// ProfileConfig state corresponds to the second state of the state machine.
// It configures the profile download by sending AT+CUSATD=X,X then AT+CUSATA to get
// AddProfSupport to know if a UICC reset is needed to take into account the new profiles written
// and the UICC state and finaly it reads the profiles in NVM with AT+CUSATR.
// When profiles are read, they can be processed.

ProfileConfig::ProfileConfig(UsatInitStateMachine& usatInitStateMachine)
 : UsatState(usatInitStateMachine)
{
}

ProfileConfig::~ProfileConfig()
{
}

////////////////////////////////////////////////////////////////

// ProfileProcess is the third state of the state machine.
// It computes the TE and MT profile to write according to the TE profile and MT mask values read
// in repository.txt and the values of the profiles already written.
// If profile writing is not needed it waits for STK service to be running.
// If profile is needed and over, it can either reset the UICC if needed or wait for STK service to
// be running.

ProfileProcess::ProfileProcess(UsatInitStateMachine& usatInitStateMachine)
 : UsatState(usatInitStateMachine)
{
}

ProfileProcess::~ProfileProcess()
{
}

////////////////////////////////////////////////////////////////

// In UiccReset state the UICC is reset by sending AT+CFUN=27 if UICC reset is needed.
// When the UICC is active, it waits for STK service to be running.

UiccReset::UiccReset(UsatInitStateMachine& usatInitStateMachine)
 : UsatState(usatInitStateMachine)
{
}

UiccReset::~UiccReset()
{
}

////////////////////////////////////////////////////////////////

// In WaitingActivate state, it waits for STK service to be running.

WaitingActivate::WaitingActivate(UsatInitStateMachine& usatInitStateMachine)
 : UsatState(usatInitStateMachine)
{
}

WaitingActivate::~WaitingActivate()
{
}

////////////////////////////////////////////////////////////////

//In ProfileActivation state, it sends the AT+CUSATA=X command to activate the profile

ProfileActivation::ProfileActivation(UsatInitStateMachine& usatInitStateMachine)
 : UsatState(usatInitStateMachine)
{
}

ProfileActivation::~ProfileActivation()
{
}

////////////////////////////////////////////////////////////////

void UsatState::run()
{
    RIL_LOG_INFO("UsatState::run() - Nothing to do\r\n");
}

void UsatState::SimReady()
{
    RIL_LOG_INFO("UsatState::SimReady() - Nothing to do\r\n");
}

void UsatState::ProfileRead()
{
    RIL_LOG_INFO("UsatState::ProfileRead() - Nothing to do\r\n");
}

void UsatState::SimUnavailable()
{
    RIL_LOG_INFO("UsatState::SimUnavailable() - Nothing to do\r\n");
}

void UsatState::SimReadyForReset()
{
    RIL_LOG_INFO("UsatState::SimReadyForReset() - Nothing to do\r\n");
}

void UsatState::SimReadyToActivate()
{
    RIL_LOG_INFO("UsatState::SimReadyToActivate() - Nothing to do\r\n");
}

void UsatState::ProfileToActivate()
{
    RIL_LOG_INFO("UsatState::ProfileToActivate() - Nothing to do\r\n");
}

void ProfileConfig::run()
{
    CTE::GetTE().ConfigureUsatProfileDownload(1, 1);
}

inline bool isValidString(const char* pszStr)
{
    return (pszStr != NULL && pszStr[0] != '\0');
}

void ProfileProcess::run()
{
    RIL_LOG_VERBOSE("ProfileProcess::run() - Enter\r\n");
    bool isTeWriteNeeded = true;
    bool isMtWriteNeeded = true;
    // Char array that holds MT profile read in NVM
    char szReadMtProfile[UsatInitStateMachine::MAX_SIZE_PROFILE * 2 + 1] = {'\0'};
    // Char array that holds TE profile to write read from repository.txt
    char szDefaultTeProfile[UsatInitStateMachine::MAX_SIZE_PROFILE * 2 + 1] = {'\0'};
    // Char array that holds calculated TE profile to write
    char szTeCalculatedProfile[UsatInitStateMachine::MAX_SIZE_PROFILE * 2 + 1] = {'\0'};
    // Char array that holds calculated MT profile to write
    char szMtCalculatedProfile[UsatInitStateMachine::MAX_SIZE_PROFILE * 2 + 1] = {'\0'};
    // Array of char array that holds all of the profiles read in NVM
    char** ppszProfiles = NULL;
    // Byte array that holds TE profile to write read in repository.txt
    const BYTE* achDefaultTeProfile;
    // Byte array that holds MT mask to apply read in repository.txt
    const BYTE* achDefaultMtMask;
    // Char array that holds TE profile read in NVM
    const char* pszReadTeProfile;
    // Char array that holds MT profile read in NVM
    const char* pszReadMtProfile;
    // Char array that holds MT only profile read in NVM
    const char* pszReadMtOnlyProfile;
    // Features that can be both supported by MT and TE : see 3GPP 31.111 Annex Q
    // Byte 3.8: REFRESH
    // Byte 5.1: SET_UP_EVENT_LIST
    // Byte 12.1: OPEN_CHANNEL
    // Byte 12.2: CLOSE_CHANNEL
    // Byte 12.3: RECEIVE_DATA
    // Byte 12.4: SEND_DATA
    // Byte 12.5: GET_CHANNEL_STATUS
    // Byte 12.6: SERVICE_SEARCH
    // Byte 12.7: GET_SERVICE_INFORMATION
    // Byte 12.8: DECLARE_SERVICE
    const char TE_MT_SUPPORTED[]
            = "0000800001000000000000FF0000000000000000000000000000000000000000";
    CCatProfile* pCatProfile = m_usatInitStateMachine.GetCatProfile();

    ppszProfiles = m_usatInitStateMachine.GetProfileArray();

    if (NULL == ppszProfiles)
    {
        RIL_LOG_CRITICAL("ProfileProcess::run() - Profile array is NULL\r\n");
        goto Out;
    }

    pszReadTeProfile = isValidString(ppszProfiles[TE_PROFILE_STORAGE])
            ? ppszProfiles[TE_PROFILE_STORAGE]
            : UsatInitStateMachine::DEFAULT_PROFILE;

    pszReadMtProfile = isValidString(ppszProfiles[MT_PROFILE_STORAGE])
            ? ppszProfiles[MT_PROFILE_STORAGE]
            : UsatInitStateMachine::DEFAULT_PROFILE;

    pszReadMtOnlyProfile = isValidString(ppszProfiles[MT_ONLY_PROFILE_STORAGE])
            ? ppszProfiles[MT_ONLY_PROFILE_STORAGE]
            : UsatInitStateMachine::DEFAULT_PROFILE;

    if (NULL == pCatProfile)
    {
        RIL_LOG_CRITICAL("ProfileProcess::run() - pCatProfile is NULL\r\n");
        goto Out;
    }

    // Set new TE profile to use to the value in NVM in case of an error
    pCatProfile->SetTeProfile(pszReadTeProfile, strlen(pszReadTeProfile));

    // Get TE profile read in repository.txt that should be written in NVM
    achDefaultTeProfile = pCatProfile->GetTeDefaultProfile();

    if (NULL == achDefaultTeProfile)
    {
        RIL_LOG_CRITICAL("ProfileProcess::run() - TE profile read is NULL\r\n");
        goto Out;
    }

    // Get MT mask read in repository.txt that should be apply to MT profile to disable
    // some modem features
    achDefaultMtMask = pCatProfile->GetMtMask();

    if (NULL == achDefaultMtMask)
    {
        RIL_LOG_CRITICAL("ProfileProcess::run() - MT mask read is NULL\r\n");
        goto Out;
    }

    if (!convertByteArrayIntoString(achDefaultTeProfile, UsatInitStateMachine::MAX_SIZE_PROFILE,
            szDefaultTeProfile))
    {
        RIL_LOG_CRITICAL("ProfileProcess::run() - Could not convert TE into string\r\n");
        goto Out;
    }

    // compare if TE read is equal to TE to write read in repository.txt
    if (CompareProfiles(pszReadTeProfile, szDefaultTeProfile))
    {
        RIL_LOG_VERBOSE("ProfileProcess::run() - Same TE\r\n");
        isTeWriteNeeded = false;
        // copy read TE to calculated TE
        memcpy(szTeCalculatedProfile, szDefaultTeProfile, strlen(szDefaultTeProfile) + 1);
    }
    else
    {
        // calculate new TE profile by doing Te = TeRepository AND NOT MtOnly
        if (!CalculateTeProfile(szDefaultTeProfile, pszReadMtOnlyProfile, szTeCalculatedProfile))
        {
            RIL_LOG_CRITICAL("ProfileProcess::run() - Cannot calculate TE profile\r\n");
            goto Out;
        }

        // Check if TE writing is needed by comparing calculated TE to the modem TE
        isTeWriteNeeded = !CompareProfiles(szTeCalculatedProfile, pszReadTeProfile);

        // Set new calculated TE profile
        pCatProfile->SetTeProfile(szTeCalculatedProfile, strlen(szTeCalculatedProfile));
    }

    // Calculate new MT profile by doing Mt = MtModem AND NOT Te
    // then the MT mask is applied to remove some modem features
    if (!CalculateMtProfile(pszReadMtProfile, szTeCalculatedProfile, TE_MT_SUPPORTED,
            achDefaultMtMask, szMtCalculatedProfile))
    {
        RIL_LOG_CRITICAL("ProfileProcess::run() - Cannot calculate MT profile\r\n");
        goto Out;
    }

    // Check if writing MT is needed by comparing it to MT read in NVM
    isMtWriteNeeded = !CompareProfiles(szMtCalculatedProfile, pszReadMtProfile);

    if (!isMtWriteNeeded && !isTeWriteNeeded)
    {
        RIL_LOG_INFO("ProfileProcess::run() - No need to write profiles\r\n");
        goto Out;
    }

    // Write profiles
    CTE::GetTE().WriteUsatProfiles(szTeCalculatedProfile, isTeWriteNeeded, szMtCalculatedProfile,
            isMtWriteNeeded);
    return;

Out:
    if (m_usatInitStateMachine.GetReadyToActivate()
            && m_usatInitStateMachine.GetUiccState() == UsatInitStateMachine::UICC_ACTIVE)
    {
        m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_READY_TO_ACTIVATE);
    }
}

bool ProfileProcess::CalculateTeProfile(const char* pszTeProfile, const char* pszMtOnlyProfile,
        char* pszTeCalcProfile)
{
    BYTE achTeDefProfile[UsatInitStateMachine::MAX_SIZE_PROFILE] = { 0 };
    BYTE achMtOnlyProfile[UsatInitStateMachine::MAX_SIZE_PROFILE] = { 0 };
    BYTE achCalcTeProfile[UsatInitStateMachine::MAX_SIZE_PROFILE] = { 0 };

    if (strlen(pszTeProfile) > UsatInitStateMachine::MAX_SIZE_PROFILE * 2)
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateTeProfile() - "
                "TE profile size is too big.\r\n");
        return false;
    }

    if (strlen(pszMtOnlyProfile) > UsatInitStateMachine::MAX_SIZE_PROFILE * 2)
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateTeProfile() - "
                "MT only profile size is too big.\r\n");
        return false;
    }

    if (!extractByteArrayFromString(pszTeProfile, strlen(pszTeProfile), achTeDefProfile))
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateTeProfile() - "
                "Cannot extract byte array from repo TE profile string.\r\n");
        return false;
    }

    if (!extractByteArrayFromString(pszMtOnlyProfile, strlen(pszMtOnlyProfile),
            achMtOnlyProfile))
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateTeProfile() - "
                "Cannot extract byte array from MT only string.\r\n");
        return false;
    }

    // TE profile is calculated by doing Te = TeRepository AND NOT MtOnly
    for (int i = 0; i < UsatInitStateMachine::MAX_SIZE_PROFILE; i++)
    {
        achCalcTeProfile[i] = achTeDefProfile[i] & ~achMtOnlyProfile[i];
    }

    if (!convertByteArrayIntoString(achCalcTeProfile, UsatInitStateMachine::MAX_SIZE_PROFILE,
            pszTeCalcProfile))
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateTeProfile() - "
                "Cannot convert byte array from calculated TE profile into string\r\n");
        return false;
    }

    return true;
}

bool ProfileProcess::CalculateMtProfile(const char* pszMtDefProfile, const char* pszTeProfile,
        const char* pszMtTeMask, const BYTE* achMask, char* pszMtProfile)
{
    BYTE achMtDefProfile[UsatInitStateMachine::MAX_SIZE_PROFILE] = { 0 };
    // Byte array that holds TE profile read in NVM
    BYTE achTeProfile[UsatInitStateMachine::MAX_SIZE_PROFILE] = { 0 };
    // Byte array that holds MT profile read in NVM
    BYTE achMtProfile[UsatInitStateMachine::MAX_SIZE_PROFILE] = { 0 };
    // Byte array that holds MT and TE supported mask
    BYTE achMtTeMask[UsatInitStateMachine::MAX_SIZE_PROFILE] = { 0 };

    if (strlen(pszMtDefProfile) > UsatInitStateMachine::MAX_SIZE_PROFILE * 2)
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateTeProfile() - "
                "MT profile size is too big.\r\n");
        return false;
    }

    if (strlen(pszTeProfile) > UsatInitStateMachine::MAX_SIZE_PROFILE * 2)
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateTeProfile() - "
                "TE profile size is too big.\r\n");
        return false;
    }

    if (strlen(pszMtTeMask) > UsatInitStateMachine::MAX_SIZE_PROFILE * 2)
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateTeProfile() - "
                "MT TE mask size is too big.\r\n");
        return false;
    }

    if (!extractByteArrayFromString(pszMtDefProfile, strlen(pszMtDefProfile), achMtDefProfile))
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateMtProfile() - "
                "Cannot extract byte array from MT profile string\r\n");
        return false;
    }

    if (!extractByteArrayFromString(pszTeProfile, strlen(pszTeProfile), achTeProfile))
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateMtProfile() - "
                "Cannot extract byte array from modem TE profile string\r\n");
        return false;
    }

    if (!extractByteArrayFromString(pszMtTeMask, strlen(pszMtTeMask), achMtTeMask))
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateMtProfile() - "
                "Cannot extract byte array from modem TE MT mask string\r\n");
        return false;
    }

    // As some features can be supported by both MT and TE, a temporary TE is calculated to prevent
    // from removing those features in MT profile. This is done by applying a mask.
    // MT profile is calculated by removing all of the features handle by TE.
    // then MT mask is applied to remove some unwanted modem features.
    for (int i = 0; i < UsatInitStateMachine::MAX_SIZE_PROFILE; i++)
    {
        achTeProfile[i] = achTeProfile[i] & ~achMtTeMask[i];
        achMtProfile[i] = achMtDefProfile[i] & ~achTeProfile[i];
        achMtProfile[i] = achMtProfile[i] & ~achMask[i];
    }

    if (!convertByteArrayIntoString(achMtProfile, UsatInitStateMachine::MAX_SIZE_PROFILE,
            pszMtProfile))
    {
        RIL_LOG_CRITICAL("ProfileProcess::CalculateMtProfile() - "
                "Cannot convert byte array from calculated MT profile into string\r\n");
        return false;
    }

    return true;
}

bool ProfileProcess::CompareProfiles(const char* pszProfileRead, const char* pszDefaultProfile)
{
    return (strcmp(pszProfileRead, pszDefaultProfile) == 0);
}

void UiccReset::run()
{
    if (m_usatInitStateMachine.GetAddProfileSupport() == 0)
    {
        m_usatInitStateMachine.SetReadyToActivate(false);
        m_usatInitStateMachine.SetStkServiceRunning(false);
        CTE::GetTE().ResetUicc();
    }
    else
    {
        if (m_usatInitStateMachine.GetReadyToActivate()
                && m_usatInitStateMachine.GetUiccState() == UsatInitStateMachine::UICC_ACTIVE)
        {
            m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_READY_TO_ACTIVATE);
        }
    }
}

void WaitingActivate::run()
{
    if (m_usatInitStateMachine.GetStkServiceRunning())
    {
        m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::STK_SERVICE_RUNNING);
    }
    else
    {
        CTE::GetTE().NotifyUiccReady();
    }
}

void ProfileActivation::run()
{
    if (m_usatInitStateMachine.GetUiccState() == UsatInitStateMachine::UICC_ACTIVE)
    {
        CTE::GetTE().NotifyUiccReady();
        CTE::GetTE().EnableProfileFacilityHandling();
    }
}

void InitState::SimReady()
{
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_READY);
}

void ProfileConfig::SimUnavailable()
{
    m_usatInitStateMachine.SetReadyToActivate(false);
    m_usatInitStateMachine.SetStkServiceRunning(false);
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_UNAVAILABLE);
}

void ProfileConfig::ProfileRead()
{
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::PROFILE_READ);
}

void ProfileProcess::SimUnavailable()
{
    m_usatInitStateMachine.SetReadyToActivate(false);
    m_usatInitStateMachine.SetStkServiceRunning(false);
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_UNAVAILABLE);
}

void ProfileProcess::SimReadyToActivate()
{
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_READY_TO_ACTIVATE);
}

void ProfileProcess::SimReadyForReset()
{
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_READY_FOR_RESET);
}

void UiccReset::SimReadyToActivate()
{
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_READY_TO_ACTIVATE);
}

void WaitingActivate::SimUnavailable()
{
    m_usatInitStateMachine.SetReadyToActivate(false);
    m_usatInitStateMachine.SetStkServiceRunning(false);
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_UNAVAILABLE);
}

void WaitingActivate::ProfileToActivate()
{
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::STK_SERVICE_RUNNING);
}

void ProfileActivation::SimUnavailable()
{
    m_usatInitStateMachine.SetReadyToActivate(false);
    m_usatInitStateMachine.SetStkServiceRunning(false);
    m_usatInitStateMachine.PassToNextEvent(UsatInitStateMachine::SIM_UNAVAILABLE);
}

