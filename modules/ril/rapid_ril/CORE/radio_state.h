////////////////////////////////////////////////////////////////////////////
// radio_state.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Stores the current radio state and notifies upon state changes
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_RADIO_STATE_H
#define RRIL_RADIO_STATE_H

#include "rril.h"

typedef enum {
        RRIL_RADIO_STATE_UNAVAILABLE = 0,
        RRIL_RADIO_STATE_OFF = 1,
        RRIL_RADIO_STATE_ON = 2
} RRIL_Radio_State;

class CRadioState
{
public:
    CRadioState();
    ~CRadioState();

    RIL_RadioState GetRadioState();

    void SetRadioState(const RRIL_Radio_State eRadioState);

    void SetRadioStateAndNotify(const RRIL_Radio_State eRadioState);

private:
    RRIL_Radio_State m_eRadioState;

    const char* PrintState(const RRIL_Radio_State eRadioState);

};

#endif // RRIL_RADIO_STATE_H
