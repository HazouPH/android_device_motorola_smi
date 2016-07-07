////////////////////////////////////////////////////////////////////////////
// radio_state.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Stores the current radio state and notifies upon state changes
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rril.h"
#include "rillog.h"
#include "rildmain.h"
#include "radio_state.h"


///////////////////////////////////////////////////////////////////////////////
CRadioState::CRadioState() : m_eRadioState(RRIL_RADIO_STATE_UNAVAILABLE)
{
    RIL_LOG_VERBOSE("CRadioState::CRadioState() - Enter / Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////
CRadioState::~CRadioState()
{
    RIL_LOG_VERBOSE("CRadioState::~CRadioState() - Enter / Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////
RIL_RadioState CRadioState::GetRadioState()
{
    RIL_RadioState radioState;
    switch (m_eRadioState)
    {
    case RRIL_RADIO_STATE_UNAVAILABLE:
        radioState =  RADIO_STATE_UNAVAILABLE;
        break;
    case RRIL_RADIO_STATE_OFF:
        radioState = RADIO_STATE_OFF;
        break;
    case RRIL_RADIO_STATE_ON:
        radioState = RADIO_STATE_ON;
        break;
    default:
        radioState =  RADIO_STATE_UNAVAILABLE;
    }

    return radioState;
}

///////////////////////////////////////////////////////////////////////////////
void CRadioState::SetRadioState(const RRIL_Radio_State eRadioState)
{
    m_eRadioState = eRadioState;
    RIL_LOG_INFO("CRadioState::SetRadioState() - RADIO STATE = %s\r\n", PrintState(m_eRadioState));
}

///////////////////////////////////////////////////////////////////////////////
void CRadioState::SetRadioStateAndNotify(const RRIL_Radio_State eRadioState)
{
    if (m_eRadioState != eRadioState) {
        m_eRadioState = eRadioState;
        RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED, NULL, 0);
        RIL_LOG_INFO("CRadioState::SetRadioStateAndNotify() - RADIO STATE = %s\r\n",
                PrintState(m_eRadioState));
    }
}

///////////////////////////////////////////////////////////////////////////////
const char* CRadioState::PrintState(const RRIL_Radio_State eRadioState)
{
    switch (eRadioState)
    {
        case RRIL_RADIO_STATE_OFF:
            return "RADIO_STATE_OFF";
        case RRIL_RADIO_STATE_ON:
            return "RADIO_STATE_ON";
        case RRIL_RADIO_STATE_UNAVAILABLE:
            return "RADIO_STATE_UNAVAILABLE";
        default:
            return "UNKNOWN";
    }
}

