////////////////////////////////////////////////////////////////////////////
// callbacks.h
//
// Copyright 2005-2008 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Headers for all functions provided to RIL_requestTimedCallback
//
/////////////////////////////////////////////////////////////////////////////

#ifndef CALLBACKS_H
#define CALLBACKS_H

//
// Used to determine frequency of callstate polling
//
static const struct timeval CallStateSlowPoll = {0, 5000000};
static const struct timeval CallStateHyperPoll = {0, 500000};

//
// Callback to trigger network state change
//
void notifyNetworkStateChanged(void* param);

//
// Callback to trigger call state update
//
void notifyChangedCallState(void* param);

//
// Callback to trigger radio off state change indication
// This is done to force the framework to trigger RADIO_POWER on
// request again
//
void triggerRadioOffInd(void* param);

//
// Callback to trigger data resumed notification
//
void triggerDataResumedInd(void* param);

//
// Callback to trigger data suspended notification
//
void triggerDataSuspendInd(void* param);

//
// Callback to trigger hangup request to modem
//
void triggerHangup(UINT32 uiCallId);

//
// Callback to send incoming SMS acknowledgement
//
void triggerSMSAck(void* param);

//
// Callback to send USSD notification
//
void triggerUSSDNotification(void* param);

//
// Callback to trigger deactivate Data call
//
void triggerDeactivateDataCall(void* param);

//
// Callback to trigger manual network search
//
void triggerManualNetworkSearch(void* param);

//
// Callback to trigger extended error report on data call disconnect
//
void triggerQueryCEER(void* param);

//
// Callback to query default PDN context parameters
//
void triggerQueryDefaultPDNContextParams(void* param);

//
// Callback to query bearer parameters
//
void triggerQueryBearerParams(void* param);

//
// Callback to trigger drop call data to crashtool
//
void triggerDropCallEvent(void* param);

void triggerCellInfoList (void* param);

//
// Callback to trigger SIM application error
//
void triggerSIMAppError(const void* param);

//
// Callback to trigger cell broadcast activation request
//
void triggerCellBroadcastActivation(void* param);

//
// Callback to query Uicc information
//
void triggerQueryUiccInfo(void* param);

#endif
