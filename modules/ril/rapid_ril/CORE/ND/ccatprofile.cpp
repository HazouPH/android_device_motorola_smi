////////////////////////////////////////////////////////////////////////////
// ccatprofile.cpp
//
// Copyright (C) Intel 2013.
//
//
// Description:
//     CCatProfile handles the TE and MT profiles to be sent to modem for USAT facilities
//
/////////////////////////////////////////////////////////////////////////////

#include "rillog.h"
#include "bertlv_util.h"
#include "ccatprofile.h"
#include "extract.h"
#include "util.h"
#include "repository.h"

// Aligned on 3GPP 31.111 version: v10.11 and ETSI 102223 version: v10.11
// PROVIDE_LOCAL_INFORMATION_XX are considered as proactive command so should be commented
const CCatProfile::ProfileItem CCatProfile::s_proactiveUICCTable[] =
{
    // FIRST BYTE
    // SECOND BYTE
    // THIRD BYTE
    { 3, DISPLAY_TEXT , 1 },                // 0000 0001
    { 3, GET_INKEY , 2 },                   // 0000 0010
    { 3, GET_INPUT , 4 },                   // 0000 0100
    { 3, MORE_TIME , 8 },                   // 0000 1000
    { 3, PLAY_TONE , 16 },                   // 0001 0000
    { 3, POLL_INTERVAL , 32 },               // 0010 0000
    { 3, POLLING_OFF , 64 },                 // 0100 0000
    { 3, REFRESH , 128 },                    // 1000 0000
    // FOURTH BYTE
    { 4, SELECT_ITEM , 1 },
    { 4, SEND_SHORT_MESSAGE , 2 },
    { 4, SEND_SS , 4 },
    { 4, SEND_USSD , 8 },
    { 4, SET_UP_CALL , 16 },
    { 4, SET_UP_MENU , 32 },
    { 4, PROVIDE_LOCAL_INFORMATION , 64 },
    // { 4, PROVIDE_LOCAL_INFORMATION_NMR , 128 },
    // FIFTH BYTE
    { 5, SET_UP_EVENT_LIST , 1 },
    // { 5, EVENT_MT_CALL , 2 }, // event sent
    // { 5, EVENT_CALL_CONNECTED , 4 }, // event sent
    // { 5, EVENT_CALL_DISCONNECTED , 8 }, // event sent
    // { 5, EVENT_LOCATION_STATUS , 16 }, // event sent
    // { 5, EVENT_USER_ACTIVITY , 32 }, // event sent
    // { 5, EVENT_IDLE_SCREEN_AVAILABLE , 64 }, // event sent
    // { 5, EVENT_CARD_READER_STATUS , 128 }, // event sent
    // SIXTH BYTE
    // { 6, EVENT_LANGUAGE_SELECTION , 1 }, // event sent
    // { 6, EVENT_BROWSER_TERMINATION , 2 }, // event sent
    // { 6, EVENT_DATA_AVAILABLE , 4 }, // event sent
    // { 6, EVENT_CHANNEL_STATUS , 8 }, // event sent
    // { 6, EVENT_ACCESS_TECH_CHANGE , 16 }, // event sent
    // { 6, EVENT_DISPLAY_PARAMS_CHANGED , 32 }, // event sent
    // { 6, EVENT_LOCAL_CONNECTION , 64 }, // event sent
    // { 6, EVENT_NETWORK_SEARCH_MODE_CHANGE , 128 }, // event sent
    // SEVENTH BYTE
    // { 7, POWER_ON_CARD , 1 }, // NOT SUPPORTED
    // { 7, POWER_OFF_CARD , 2 }, // NOT SUPPORTED
    // { 7, PERFORM_CARD_APDU , 4 }, // NOT SUPPORTED
    // { 7, GET_READER_STATUS_STATUS , 8 }, // NOT SUPPORTED
    // { 7, GET_READER_STATUS_IDENTIFIER , 16 }, // NOT SUPPORTED
    // EIGHTH BYTE
    { 8, TIMER_MANAGEMENT_START_STOP , 1 },
    { 8, TIMER_MANAGEMENT_GET_CURRENT , 2 },
    // { 8, PROVIDE_LOCAL_INFORMATION_DATE , 4 },
    // { 8, GET_INKEY_VALID , 8 }, // Redundant with 3.2
    { 8, SET_UP_IDLE_MODE_TEXT , 16 },
    { 8, RUN_AT_COMMAND , 32 },
    // { 8, SET_UP_CALL_VALID , 64 }, // Redundant with 4.16
    // { 8, CALL_CONTROL_VALID , 128 }, // Redundant with ??
    // NINTH BYTE
    // 9.1: DISPLAY_TEXT, redundant with above 3.1
    { 9, SEND_DTMF , 2 },
    // { 9, PROVIDE_LOCAL_INFORMATION_NMR_VALID , 4 }, // redundant with 4.8
    // { 9, PROVIDE_LOCAL_INFORMATION_LANG , 8 },
    // { 9, PROVIDE_LOCAL_INFORMATION_TIMING , 16 },
    { 9, LANGUAGE_NOTIFICATION , 32 },
    { 9, LAUNCH_BROWSER , 64 },
    // { 9, PROVIDE_LOCAL_INFORMATION_ACCESS , 128 },
    // TENTH BYTE
    // ELEVENTH BYTE
    // TWELFTH BYTE (class "e": all commands are for the moment considered as only AP handled,
    // or not supported.)
    // TODO: refine filtering on bearer type and Transport control type as some bearers could be
    // handled by modem in a close future (so seen as notifications and not as proactive commands),
    // such as default bearer or packet data service bearer
    { 12, OPEN_CHANNEL , 1 },
    { 12, CLOSE_CHANNEL , 2 },
    { 12, RECEIVE_DATA , 4 },
    { 12, SEND_DATA , 8 },
    { 12, GET_CHANNEL_STATUS , 16 },
    { 12, SERVICE_SEARCH , 32 },
    { 12, GET_SERVICE_INFORMATION , 64 },
    { 12, DECLARE_SERVICE , 128 },
    // THIRTEENTH BYTE
    // FOURTEENTH BYTE
    // FIFTEENTH BYTE
    // SIXTEENTH BYTE
    // SEVENTEENTH BYTE
    // EIGHTEENTH BYTE
    // { 18, DISPLAY_TEXT_VAR_TIME_OUT , 1 }, // OPTION
    // { 18, GET_INKEY_HELP_SUPPORTED , 2 }, // OPTION
    // { 18, GET_INKEY_VAR_TIME_OUT , 8 }, // OPTION
    // { 18, PROVIDE_LOCAL_INFORMATION_ESN , 16 },
    // 18.6: CALL CONTROL GPS
    // { 18, PROVIDE_LOCAL_INFORMATION_IMEISV , 64 },
    // { 18, PROVIDE_LOCAL_INFORMATION_SEARCH_MODE_CHANGE , 128 },
    // NINETEENTH BYTE
    // TWENTIETH BYTE
    // TWENTY-FIRST BYTE
    // TWENTY-SECOND BYTE
    // 22.1: Support of UTRAN PS with extended parameters (BIP UMTS)
    // { 22, PROVIDE_LOCAL_INFORMATION_BATT_STATE , 2 },
    // { 22, PLAY_TONE_MELODY , 4 }, // redundant with 3.16
    // { 22, RETRIEVE_MULTIMEDIA_MESSAGE , 32}, // NOT SUPPORTED
    // { 22, SUBMIT_MULTIMEDIA_MESSAGE , 64}, // NOT SUPPORTED
    // { 22, DISPLAY_MULTIMEDIA_MESSAGE , 128 }, // NOT SUPPORTED
    // TWENTY-THIRD BYTE
    // { 23, SET_FRAMES , 1 }, // NOT SUPPORTED
    // { 23, GET_FRAMES_STATUS , 2 }, // NOT SUPPORTED
    // { 23, PROVIDE_LOCAL_INFORMATION_MEID , 32 },
    // { 23, PROVIDE_LOCAL_INFORMATION_NMR_UTRAN , 64 },
    // TWENTY-FOURTH BYTE
    // TWENTY-FIFTH BYTE
    // TWENTY-SIXTH BYTE
    // TWENTY-SEVENTH BYTE
    // TWENTY-EIGHTH BYTE
    // TWENTY-NINTH
    // THIRTIETH BYTE
    // { 30, PROVIDE_LOCAL_INFORMATION_WSID , 2 },
    // 30.4: REFRESH steering of roaming, done by modem
    { 30, ACTIVATE_CLASS_L , 16 }, // NFC
    // { 30, GEO_LOCATION_REQUEST , 32 }, // NOT USED
    // { 30, PROVIDE_LOCAL_INFORMATION_BROADCAST , 64 },
    // THIRTY-FIRST BYTE
    // { 31, CONTACTLESS_STATE_CHANGED , 1 }, // NOT USED, class "r"
    // 31.4: Communication Control for IMS
    // { 31, COMMAND_CONTAINER , 128 }, // NOT USED
    // THIRTY-SECOND BYTE
    // { 32, IMS_SUPPORT , 64 }, // NOT USED
    // END
    { 0, 0 , 0 }
};

CCatProfile::CCatProfile()
{
    RIL_LOG_VERBOSE("CCatProfile::CCatProfile() - Enter\r\n");
    InitTeProfile();
    InitMtMask();
    // Mapping of a profile string.
    // Focused on proactive commands.
}

/**
 * Specify the TE profile to use.
 *
 * @param pszProfile : An allocated string of hexadecimal characters.
 * @param uiLength : The length of the profile/mask string.
 * @param achByteArray : The byte array to fill in.
 * @return false it is not possible to set the profile/mask
 */
BOOL CCatProfile::SetByteArray(const char* pszProfile, const UINT32 uiLength, BYTE* achByteArray)
{
    RIL_LOG_VERBOSE("CCatProfile::SetByteArray() - Enter\r\n");
    BOOL bRet = FALSE;

    if (!pszProfile)
    {
        RIL_LOG_CRITICAL("CCatProfile::SetByteArray() - Profile/Mask is NULL\r\n");
        return bRet;
    }

    if (uiLength > UsatInitStateMachine::MAX_SIZE_PROFILE * 2)
    {
        RIL_LOG_INFO("CCatProfile::SetByteArray() - Profile/Mask size is too long. Exit\r\n");
        return bRet;
    }

    bRet = extractByteArrayFromString(pszProfile, uiLength, achByteArray);

    RIL_LOG_VERBOSE("CCatProfile::SetByteArray() - Profile/Mask set: %s - Exit\r\n", pszProfile);

    return bRet;
}

BOOL CCatProfile::SetTeProfile(const char* pszMask, const UINT32 uiLength)
{
    return SetByteArray(pszMask, uiLength, m_achTeProfile);
}

BOOL CCatProfile::SetTeDefaultProfile(const char* pszMask, const UINT32 uiLength)
{
    return SetByteArray(pszMask, uiLength, m_achTeDefaultProfile);
}

BOOL CCatProfile::SetMtMask(const char* pszMask, const UINT32 uiLength)
{
    return SetByteArray(pszMask, uiLength, m_achMtMask);
}

const BYTE* CCatProfile::GetTeProfile()
{
    return m_achTeProfile;
}

const BYTE* CCatProfile::GetTeDefaultProfile()
{
    return m_achTeDefaultProfile;
}

const BYTE* CCatProfile::GetMtMask()
{
    return m_achMtMask;
}

/**
 * This method parses the given PDU and extract info into ProactiveCommandInfo given object.
 *
 * @param pszPdu : String of hexadecimal characters.
 * @param uiLength : Length of PDU.
 * @param pPduInfo : Allocated pointer to a ProactiveCommandInfo object.
 */
BOOL CCatProfile::ExtractPduInfo(const char* pszPdu, const UINT32 uiLength,
        ProactiveCommandInfo* pPduInfo)
{
    RIL_LOG_VERBOSE("CCatProfile::ExtractPduInfo() - Enter");
    BOOL bRet = FALSE;
    BerTlv tlvPdu;
    BerTlv tlvCmdDetails;
    UINT8* pPduBytes = NULL;
    const UINT8* pbCmdData = NULL;
    UINT32 cbCmdDataSize = 0;
    BOOL bFound = FALSE;
    UINT8 uiCmd = 0;

    if (!pPduInfo)
    {
        RIL_LOG_CRITICAL("CCatProfile::ExtractPduInfo() - ERROR BAD PARAMETER\r\n");
        goto Error;
    }

    // Init returned struct
    pPduInfo->uiCommandId = 0;
    pPduInfo->isProactiveCmd = FALSE;

    // NEED TO CONVERT chars string onto bytes array
    pPduBytes = (UINT8*)malloc(uiLength / 2);

    if (NULL == pPduBytes)
    {
        RIL_LOG_CRITICAL("CCatProfile::ExtractPduInfo() -"
                " Could not allocate memory for PDU.\r\n");
        goto Error;
    }

    if (!extractByteArrayFromString(pszPdu, uiLength, pPduBytes))
    {
        RIL_LOG_CRITICAL("CCatProfile::ExtractPduInfo() -"
                "Unable to extract byte array from string");
        goto Error;
    }

    RIL_LOG_INFO("CCatProfile::ExtractPduInfo() - ENTER:Length:%d, PDU:[0x%X][0x%X]\r\n",
            uiLength, pPduBytes[0], pPduBytes[1]);

    if (tlvPdu.Parse(pPduBytes, uiLength))
    {
        RIL_LOG_INFO("CCatProfile::ExtractPduInfo() : First Tag:0x%X, Length:%d\r\n",
                tlvPdu.GetTag(), tlvPdu.GetLength());

        pbCmdData = tlvPdu.GetValue();
        cbCmdDataSize = tlvPdu.GetLength();

        if (tlvCmdDetails.Parse(pbCmdData, cbCmdDataSize))
        {
            uiCmd = tlvCmdDetails.GetTag();
            RIL_LOG_INFO("CCatProfile::ExtractPduInfo() : Command DETAILS Tag:0x%X\r\n", uiCmd);
            if (uiCmd == COMMAND_DETAILS_TAG)
            {
                pbCmdData = tlvCmdDetails.GetValue();
                // Format is (see ETSI 102223 - Annex B and C):
                // pbCmdData[0] = Command number
                // pbCmdData[1] and pbCmdData[2] = Command identifier
                cbCmdDataSize = tlvCmdDetails.GetLength();
                if (cbCmdDataSize > 1)
                {
                    uiCmd = pbCmdData[1];
                }
                else
                {
                    RIL_LOG_CRITICAL("CCatProfile::ExtractPduInfo() -"
                            "ERROR COMMAND DETAILS WRONG SIZE:%d!\r\n", cbCmdDataSize);
                    goto Error;
                }
            }
            else
            {
                RIL_LOG_CRITICAL("CCatProfile::ExtractPduInfo() -"
                        "ERROR CmdId:0x%X NOT A COMMAND DETAILS\r\n", uiCmd);
                goto Error;
            }
            pPduInfo->uiCommandId = uiCmd;
            RIL_LOG_INFO("CCatProfile::ExtractPduInfo() -"
                    "Command Tag:0x%X, Length:%d\r\n", uiCmd, cbCmdDataSize);

            int indexFound = -1;
            for (int i = 0; s_proactiveUICCTable[i].uiByteId != 0; ++i)
            {
                if (s_proactiveUICCTable[i].uiCmdId == uiCmd)
                {
                    indexFound = i;
                    break;
                }
            }

            if (indexFound > -1)
            {
                UINT8 byteId = s_proactiveUICCTable[indexFound].uiByteId;
                bFound = (m_achTeProfile[byteId - 1]
                        & s_proactiveUICCTable[indexFound].uiBitMask) != 0;
                pPduInfo->isProactiveCmd = bFound;
                bRet = TRUE;
                RIL_LOG_INFO("CCatProfile::ExtractPduInfo() -"
                        " indexFound:%d, CmdId:0x%X, uiByteId:0x%X,"
                        " uiCmdId:0x%X, uiBitMask:0x%X, Found:%d\r\n", indexFound, uiCmd,
                        s_proactiveUICCTable[indexFound].uiByteId,
                        s_proactiveUICCTable[indexFound].uiCmdId,
                        s_proactiveUICCTable[indexFound].uiBitMask, bFound);
            }
            else
            {
                RIL_LOG_INFO("CCatProfile::ExtractPduInfo() - "
                        "Index not found, setting as event\r\n");
            }
        }
    }

    // Return TRUE if CMD was known and found in ProactiveUICC table.

Error:
    free(pPduBytes);
    pPduBytes = NULL;

    RIL_LOG_VERBOSE("CCatProfile::ExtractPduInfo() : Return:%d\r\n - Exit", bRet);
    return bRet;
}

void CCatProfile::InitTeProfile()
{
    UINT32 uiProfileLength = strlen(UsatInitStateMachine::DEFAULT_PROFILE);
    SetTeProfile(UsatInitStateMachine::DEFAULT_PROFILE, uiProfileLength);

    ReadTeDefaultProfile();
}

BOOL CCatProfile::ReadTeDefaultProfile()
{
    RIL_LOG_VERBOSE("CCatProfile::ReadTeDefaultProfile() - Enter\r\n");
    CRepository repository;
    UINT32 uiProfileLength = strlen(UsatInitStateMachine::DEFAULT_PROFILE);
    char szTeDefaultProfile[MAX_BUFFER_SIZE] = {'\0'};
    BOOL bRet = FALSE;

    // Read the Te profile from repository.txt
    if (!repository.Read(g_szGroupModem, g_szTeProfile, szTeDefaultProfile, MAX_BUFFER_SIZE))
    {
        bRet = SetTeDefaultProfile(UsatInitStateMachine::DEFAULT_PROFILE, uiProfileLength);
        RIL_LOG_INFO("CCatProfile::ReadTeDefaultProfile() - "
                "No TE profile found in repository, default TE is used\r\n");
    }
    else
    {
        bRet = SetTeDefaultProfile(szTeDefaultProfile, strlen(szTeDefaultProfile));
    }

    RIL_LOG_VERBOSE("CCatProfile::ReadTeDefaultProfile() - Exit\r\n");
    return bRet;
}

void CCatProfile::InitMtMask()
{
    RIL_LOG_VERBOSE("CCatProfile::InitMtMask() - Enter\r\n");
    CRepository repository;
    UINT32 uiProfileLength = strlen(UsatInitStateMachine::DEFAULT_MASK);
    char szMtMask[MAX_BUFFER_SIZE] = {'\0'};

    // Read the Mt mask from repository.txt
    if (!repository.Read(g_szGroupModem, g_szMtMask, szMtMask, MAX_BUFFER_SIZE))
    {
        SetMtMask(UsatInitStateMachine::DEFAULT_MASK, uiProfileLength);
        RIL_LOG_INFO("CCatProfile::InitMtMask() - "
                "No MT Mask found in repository, default MT mask is used\r\n");
    }
    else
    {
        SetMtMask(szMtMask, strlen(szMtMask));
    }

    RIL_LOG_VERBOSE("CCatProfile::InitMtMask() - Exit\r\n");
}
