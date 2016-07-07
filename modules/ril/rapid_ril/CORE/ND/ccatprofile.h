////////////////////////////////////////////////////////////////////////////
// ccatprofile.h
//
// Copyright (C) Intel 2013.
//
//
// Description:
//     CCatProfile handles the TE and MT profiles to be sent to modem for USAT facilities
//
/////////////////////////////////////////////////////////////////////////////

#ifndef CCATPROFILE_H
#define CCATPROFILE_H

#include "types.h"
#include "usat_init_state_machine.h"

class CCatProfile
{
public:

    enum
    {
        PROACTIVE_UICC_TAG = 0xD0,      /** Proactive UICC command tag. see ETSI 102 223 */
        COMMAND_DETAILS_TAG = 0x81      /** Command details tag. See ETSI 101220 section 7.2 */
    };

// This is the list of Proactive command IDs and their coded values (in Hexa)
// NOTE from ETSI 102 223 Section 9.4: The IDs with value 0 are not used currently, but they will
//       become useful when managing Terminal profiles (Dynamic)
    enum Tag
    {
        REFRESH = 0x01,
        MORE_TIME = 0x02,
        POLL_INTERVAL = 0x03,
        POLLING_OFF = 0x04,
        SET_UP_EVENT_LIST = 0x05,
        SET_UP_CALL = 0x10,
        SET_UP_CALL_VALID = 0x00,
        CALL_CONTROL_VALID = 0x00,
        SEND_SS = 0x11,
        SEND_USSD = 0x12,
        SEND_SHORT_MESSAGE = 0x13,
        SEND_DTMF = 0x14,
        LAUNCH_BROWSER = 0x15,
        GEO_LOCATION_REQUEST = 0x16,
        PLAY_TONE = 0x20,
        DISPLAY_TEXT = 0x21,
        DISPLAY_TEXT_VAR_TIME_OUT = 0x00,
        GET_INKEY = 0x22,
        GET_INKEY_HELP_SUPPORTED = 0x00,
        GET_INKEY_VAR_TIME_OUT = 0x00,
        GET_INKEY_VALID = 0x00,
        GET_INPUT = 0x23,
        SELECT_ITEM = 0x24,
        SET_UP_MENU = 0x25,
        PROVIDE_LOCAL_INFORMATION = 0x26,
        PROVIDE_LOCAL_INFORMATION_NMR = 0x00,
        PROVIDE_LOCAL_INFORMATION_DATE = 0x00,
        PROVIDE_LOCAL_INFORMATION_NMR_VALID = 0x00,
        PROVIDE_LOCAL_INFORMATION_LANG = 0x00,
        PROVIDE_LOCAL_INFORMATION_TIMING = 0x00,
        PROVIDE_LOCAL_INFORMATION_ACCESS = 0x00,
        PROVIDE_LOCAL_INFORMATION_ESN = 0x00,
        PROVIDE_LOCAL_INFORMATION_IMEISV = 0x00,
        PROVIDE_LOCAL_INFORMATION_SEARCH_MODE_CHANGE = 0x00,
        PROVIDE_LOCAL_INFORMATION_BATT_STATE = 0x00,
        PROVIDE_LOCAL_INFORMATION_MEID = 0x00,
        PROVIDE_LOCAL_INFORMATION_NMR_UTRAN = 0x00,
        PROVIDE_LOCAL_INFORMATION_WSID = 0x00,
        PROVIDE_LOCAL_INFORMATION_BROADCAST = 0x00,
        TIMER_MANAGEMENT_START_STOP = 0x27,
        TIMER_MANAGEMENT_GET_CURRENT = 0x00,
        SET_UP_IDLE_MODE_TEXT = 0x28,
        PERFORM_CARD_APDU = 0x30,
        POWER_ON_CARD = 0x31,
        POWER_OFF_CARD = 0x32,
        GET_READER_STATUS_STATUS = 0x33,
        GET_READER_STATUS_IDENTIFIER = 0x00,
        RUN_AT_COMMAND = 0x34,
        LANGUAGE_NOTIFICATION = 0x35,
        OPEN_CHANNEL = 0x40,
        CLOSE_CHANNEL = 0x41,
        RECEIVE_DATA = 0x42,
        SEND_DATA = 0x43,
        GET_CHANNEL_STATUS = 0x44,
        SERVICE_SEARCH = 0x45,
        GET_SERVICE_INFORMATION = 0x46,
        DECLARE_SERVICE = 0x47,
        SET_FRAMES = 0x50,
        GET_FRAMES_STATUS = 0x51,
        PLAY_TONE_MELODY = 0x00,
        RETRIEVE_MULTIMEDIA_MESSAGE = 0x60,
        SUBMIT_MULTIMEDIA_MESSAGE = 0x61,
        DISPLAY_MULTIMEDIA_MESSAGE = 0x62,
        ACTIVATE_CLASS_L = 0x70,
        CONTACTLESS_STATE_CHANGED = 0x71,
        COMMAND_CONTAINER = 0x72,
        IMS_SUPPORT = 0x73,
        END_PROACTIVE_SESSION = 0x81,
        EVENT_MT_CALL = 0x00,
        EVENT_CALL_CONNECTED = 0x00,
        EVENT_CALL_DISCONNECTED = 0x00,
        EVENT_LOCATION_STATUS = 0x00,
        EVENT_USER_ACTIVITY = 0x00,
        EVENT_IDLE_SCREEN_AVAILABLE = 0x00,
        EVENT_CARD_READER_STATUS = 0x00,
        EVENT_LANGUAGE_SELECTION = 0x00,
        EVENT_BROWSER_TERMINATION = 0x00,
        EVENT_DATA_AVAILABLE = 0x00,
        EVENT_CHANNEL_STATUS = 0x00,
        EVENT_ACCESS_TECH_CHANGE = 0x00,
        EVENT_DISPLAY_PARAMS_CHANGED = 0x00,
        EVENT_LOCAL_CONNECTION = 0x00,
        EVENT_NETWORK_SEARCH_MODE_CHANGE = 0x00
    };

    struct ProactiveCommandInfo
    {
        UINT8      uiCommandId;
        BOOL       isProactiveCmd;
    };

    CCatProfile();
    BOOL SetTeProfile(const char* pszProfile, const UINT32 uiLength);
    BOOL SetMtMask(const char* szMask, const UINT32 uiLength);
    const BYTE* GetTeProfile();
    const BYTE* GetTeDefaultProfile();
    const BYTE* GetMtMask();
    BOOL ExtractPduInfo(const char* pszUrc, const UINT32 uiLength, ProactiveCommandInfo* pPduInfo);

private:
    // TE profile
    BYTE m_achTeProfile[UsatInitStateMachine::MAX_SIZE_PROFILE];
    // TE default profile
    BYTE m_achTeDefaultProfile[UsatInitStateMachine::MAX_SIZE_PROFILE];
    // MT Mask
    BYTE m_achMtMask[UsatInitStateMachine::MAX_SIZE_PROFILE];

    struct ProfileItem
    {
        UINT8      uiByteId;
        UINT8      uiCmdId;
        UINT8      uiBitMask;
    };

    // USAT command ID table
    static const ProfileItem s_proactiveUICCTable[];

    BOOL SetByteArray(const char* pszProfile, const UINT32 uiLength, BYTE* achByteArray);
    BOOL SetTeDefaultProfile(const char* pszProfile, const UINT32 uiLength);
    BOOL ReadTeDefaultProfile();
    void InitTeProfile();
    void InitMtMask();
};

#endif // CCATPROFILE_H
