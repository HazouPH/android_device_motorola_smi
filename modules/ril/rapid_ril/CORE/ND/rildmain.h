////////////////////////////////////////////////////////////////////////////
// rilmain.h
//
// Copyright 2005-2008 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for top-level RIL functions.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_RILDMAIN_H
#define RRIL_RILDMAIN_H

#include "types.h"
#include <telephony/ril.h>

class CThread;

// The device path to use for the AT command channel
extern char* g_szCmdPort;
// The device path to use for the GPRS attach/detatch, network channel
extern char* g_szDLC2Port;
// The device path to use for the Supplementary services channel
extern char* g_szDLC6Port;
// The device path to use for the SIM / SIM Toolkit channel
extern char* g_szDLC8Port;
// The device path to use for commands that can block
extern char* g_szDLC22Port;
// The device path to use for RF coexistence
extern char* g_szDLC23Port;
// The device path to use for the SMS channel
extern char* g_szSmsPort;
// The device path to use for the notification channel
extern char* g_szURCPort;
// The device path to use for the OEM channel
extern char* g_szOEMPort;
// The device path to use for the data channel1
extern char* g_szDataPort1;
// The device path to use for the data channel2
extern char* g_szDataPort2;
// The device path to use for the data channel3
extern char* g_szDataPort3;
// The device path to use for the data channel4
extern char* g_szDataPort4;
// The device path to use for the data channel5
extern char* g_szDataPort5;

extern BOOL  g_bIsSocket;


void RIL_onRequestComplete(RIL_Token tRIL, RIL_Errno eErrNo, void* pResponse, size_t responseLen);

void RIL_onUnsolicitedResponse(int unsolResponseID, const void* pData, size_t dataSize);

void RIL_requestTimedCallback(RIL_TimedCallback callback,
                                            void* pParam,
                                            const struct timeval* pRelativeTime);

void RIL_requestTimedCallback(RIL_TimedCallback callback,
                                            void* pParam,
                                            const unsigned long seconds,
                                            const unsigned long microSeconds);
#endif // RRIL_RILDMAIN_H
