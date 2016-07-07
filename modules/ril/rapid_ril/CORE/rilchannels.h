////////////////////////////////////////////////////////////////////////////
// rilchannels.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides function prototypes for
//    channel-related helper functions.
//    Also enumerates all of the channels that
//    can be used when sending AT Commands.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

//  List channels here (one per COM port)
const int RIL_CHANNEL_MAX = 14;

// Upper limit on number of RIL channels to create
extern UINT32 g_uiRilChannelUpperLimit;

// Current RIL channel index maximum (depends on number of data channels created)
extern UINT32 g_uiRilChannelCurMax;

// Number of apn profile
// THIS SHOULD MATCH DATA_PROFILE_XXX + 1 in rril.h
const int NUMBER_OF_APN_PROFILE = 6;

// Number of channels able to carry data.
const int RIL_MAX_NUM_IPC_CHANNEL = 5;

// HSI: channel 0 and 1 are not used for data.
const int RIL_DEFAULT_IPC_CHANNEL_MIN = 2;

// HSI: default configuration for 7x60.
const char RIL_DEFAULT_IPC_RESOURCE_NAME[] = "mipi_ipc";

//  Call control commands, misc commands
const int RIL_CHANNEL_ATCMD = 0;

//  GPRS/UMTS management (GPRS attach/detach), network commands
const int RIL_CHANNEL_DLC2 = 1;

//  Call settings, supplementary services
const int RIL_CHANNEL_DLC6 = 2;

//  SIM related functions, SIM toolkit
const int RIL_CHANNEL_DLC8 = 3;

//  URC channel
const int RIL_CHANNEL_URC = 4;

//  OEM channel (diagnostic commands)
const int RIL_CHANNEL_OEM = 5;

//  COPS=x commands
const int RIL_CHANNEL_DLC22 = 6;

//  Reserved for RF coexistence
const int RIL_CHANNEL_DLC23 = 7;

//  SMS channel
const int RIL_CHANNEL_SMS = 8;

//  Note: data channels must always stay the last defined
//  GPRS/UMTS data (1st primary context)
const int RIL_CHANNEL_DATA1 = 9;

//  GPRS/UMTS data (2nd primary context)
const int RIL_CHANNEL_DATA2 = 10;

//  GPRS/UMTS data (3rd primary context)
const int RIL_CHANNEL_DATA3 = 11;

//  GPRS/UMTS data (4th primary context)
const int RIL_CHANNEL_DATA4 = 12;

//  GPRS/UMTS data (5th primary context)
const int RIL_CHANNEL_DATA5 = 13;

// TODO - currently allow up to one reserved channel (e.g., for direct audio use).
// Set this to -1 or a number greater than RIL_CHANNEL_MAX if there is no reserved channel
const int RIL_CHANNEL_RESERVED = 555;

