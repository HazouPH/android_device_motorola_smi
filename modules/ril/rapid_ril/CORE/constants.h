////////////////////////////////////////////////////////////////////////////
// constants.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
// Description:
//    Constants for RapidRIL
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(__RIL_CONSTANTS_H__)
#define __RIL_CONSTANTS_H__

static const UINT32 MAX_MODEM_NAME_LEN = 64;

static const char* szXMM6260 = "6260";
static const char* szXMM6360 = "6360";
static const char* szXMM7160 = "7160";
static const char* szXMM7260 = "7260";
static const char* szXMM2230 = "2230";

enum {
    MODEM_TYPE_UNKNOWN,
    MODEM_TYPE_XMM6260,
    MODEM_TYPE_XMM6360,
    MODEM_TYPE_XMM7160,
    MODEM_TYPE_XMM7260,
    MODEM_TYPE_XMM2230
};

enum {
    MODE_PS_DATA_CENTRIC,
    MODE_CS_PS_VOICE_CENTRIC,
    MODE_CS_PS_DATA_CENTRIC,
    MODE_PS_VOICE_CENTRIC
};

static const int MAX_NETWORK_DATA_SIZE = 50;

enum LAST_NETWORK_DATA_ID {
    LAST_NETWORK_XCSQ = 0,
    LAST_NETWORK_CREG,
    LAST_NETWORK_CGREG,
    LAST_NETWORK_XREG,
    LAST_NETWORK_OP_NAME_NUMERIC,
    LAST_NETWORK_OP_NAME_SHORT,
    LAST_NETWORK_LAC,
    LAST_NETWORK_CID,
    LAST_NETWORK_DATA_COUNT
};

const int RSSI_UNKNOWN = 99;
const int BER_UNKNOWN = 99;
const int RSCP_UNKNOWN = 255;
const int ECNO_UNKNOWN = 255;
const int RSRQ_UNKNOWN = 255;
const int RSRP_UNKNOWN = 255;
const int RSSNR_UNKNOWN = 255;
const int RXLEV_UNKNOWN = 99;

const UINT32 IMS_NOT_REGISTERED = 0;
const UINT32 IMS_REGISTERED = 1;

const int STATE_SET_NETWORK_SELECTION_MODE = 0;
const int STATE_SET_INITIAL_ATTACH_APN = 1;
const int STATE_ATTACH = 2;

const int SERVING_CELL = 1;
const int NEIGHBOURING_CELL = 0;

#endif  // __RIL_CONSTANTS_H__
