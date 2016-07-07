////////////////////////////////////////////////////////////////////////////
// rril.h
//
// Copyright 2005-2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides RIL structures and constants.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef RRIL_RRIL_H
#define RRIL_RRIL_H

#include <telephony/ril.h>
#include <telephony/librilutils.h>
#include <cutils/properties.h>
///////////////////////////////////////////////////////////////////////////////
// Typedefs
//
typedef long                RIL_CMD_ID;
typedef long                RIL_RESULT_CODE;


////////////////////////////////////////////////////////////////////////////////
// Android / WM structs conversion
//

const UINT32 WAIT_FOREVER = 0xFFFFFFFF;

///////////////////////////////////////////////////////////////////////////////
// Number of Calls to Track
//
const UINT32 RRIL_MAX_CALL_ID_COUNT = 10;

///////////////////////////////////////////////////////////////////////////////
// Number of Call Forwarding Entries to support
//
const UINT32 RIL_MAX_CALLFWD_ENTRIES = 10;

///////////////////////////////////////////////////////////////////////////////
// Value to indicate no retry for RIL_REQUEST_SETUP_DATA_CALL request.
// value comes from request description which can be found in ril.h
//
const int MAX_INT = 0x7FFFFFFF;

///////////////////////////////////////////////////////////////////////////////
// Maximum length for various buffers and string parameters
//
const int MAX_BUFFER_SIZE = 1024;
const int MAX_PIN_SIZE = 9;
const int MAX_FACILITY_CODE = 5;
const int MAX_PDP_CONTEXTS = 5;
const int MAX_INTERFACE_NAME_SIZE = 50;
const int MAX_PDP_TYPE_SIZE = 20;
const int MAX_MDM_RESOURCE_NAME_SIZE = 20;
const int MAX_FDTIMER_SIZE = 5;
const int MODEM_STATE_UNKNOWN = -1;
const int MAX_APN_SIZE = 101;
const int AUTN_LENGTH = 32;
const int RAND_LENGTH = 32;

// size includes the C-string terminal zero
const int MAX_IPADDR_SIZE = 64 + 1;
const int MAX_CNAP_NAME_SIZE = 80 + 1;

///////////////////////////////////////////////////////////////////////////////
// SIM related constants
//

const int EF_FDN = 0x6F3B;
const int EF_EXT2 = 0x6F4B;

const int SIM_COMMAND_READ_BINARY = 176;
const int SIM_COMMAND_READ_RECORD = 178;
const int SIM_COMMAND_GET_RESPONSE = 192;
const int SIM_COMMAND_UPDATE_BINARY = 214;
const int SIM_COMMAND_UPDATE_RECORD = 220;
const int SIM_COMMAND_STATUS = 242;
const int SIM_COMMAND_RETRIEVE_DATA = 203;
const int SIM_COMMAND_SET_DATA = 219;

const int SIM_USIM_APP_INDEX = 0;
const int ISIM_APP_INDEX = 1;

const UINT32 MAX_APP_LABEL_SIZE = 33; // including null termination
const UINT32 MAX_AID_SIZE = 33; // Hex string length including null termination

///////////////////////////////////////////////////////////////////////////////
// Value to indicate profile Id in RIL_REQUEST_SETUP_DATA_CALL request.
// value comes from frameworks/base/telephony/java/com/android/internal/telephony/RILConstants.java
//
const int DATA_PROFILE_DEFAULT = 0;
const int DATA_PROFILE_TETHERED = 1;
const int DATA_PROFILE_IMS = 2;
const int DATA_PROFILE_FOTA = 3;
const int DATA_PROFILE_CBS = 4;
// Next profiles are OEM ids (above RILConstants.DATA_PROFILE_OEM_BASE = d1000)
const int DATA_PROFILE_OEM_RCS = 0x400;
const int DATA_PROFILE_OEM_EIMS = 0x800;
const int DATA_PROFILE_OEM_XCAP = 0x1000;
// Constant to mask bits potentially usable by AOSP profiles (<= d1023)
const int DATA_PROFILE_AOSP_MASK = 0x3FF;

#if !defined (USE_PATCHED_AOSP)

//////////////////////////////////////////////////////////////////////////////
// ril.h definitions used to support vanilla aosp

/* If SIM IMEI locked */
const int RIL_APPSTATE_IMEI = 6;

// Must be the same as CellInfo.TYPE_XXX
const int RIL_CELL_INFO_TYPE_GSM_V2 = 11;
const int RIL_CELL_INFO_TYPE_CDMA_V2 = 12;
const int RIL_CELL_INFO_TYPE_LTE_V2 = 13;
const int RIL_CELL_INFO_TYPE_WCDMA_V2 = 14;

typedef struct {
    int signalStrength; /* Valid values are (0-31, 99) as defined in TS 27.007 8.5 */
    int bitErrorRate; /* bit error rate (0-7, 99) as defined in TS 27.007 8.5 */
    int rxLev; /* Valid values are (0-63, 99) as defined in TS 27.007 8.69 */
    int timingAdvance; /* Timing Advance, INT_MAX if unknown */
} RIL_GW_SignalStrength_v2;

typedef struct {
    int signalStrength; /* Valid values are (0-31, 99) as defined in TS 27.007 8.5 */
    int bitErrorRate; /* bit error rate (0-7, 99) as defined in TS 27.007 8.5 */
    int rscp; /* Received signal code power, Valid values are (0-96, 255) as defined in
               * 3GPPTS 27.007 8.69.
               */
    int ecNo; /* Valid values are (0-49, 255) as defined in TS 27.007 8.69. Ratio of the received
               * energy per PN chip to the total received power spectral density.
               */
} RIL_SignalStrengthWcdma_v2;

/** RIL_CellIdentityGsm_v2 */
typedef struct {
    int mcc; /* 3-digit Mobile Country Code, 0..999, INT_MAX if unknown */
    int mnc; /* 2 or 3-digit Mobile Network Code, 0..999, INT_MAX if unknown */
    int lac; /* 16-bit Location Area Code, 0..65535, INT_MAX if unknown */
    int cid; /* 16-bit GSM Cell Identity described in TS 27.007, 0..65535, INT_MAX if unknown */
    int basestationId; /* Base station identification code, INT_MAX if unknown */
    int arfcn; /* 16-bit Absolute Radio Frequency Channel Number, INT_MAX if unknown */
} RIL_CellIdentityGsm_v2;

/** RIL_CellIdentityWcdma_v2 */
typedef struct {
    int mcc; /* 3-digit Mobile Country Code, 0..999, INT_MAX if unknown */
    int mnc; /* 2 or 3-digit Mobile Network Code, 0..999, INT_MAX if unknown */
    int lac; /* 16-bit Location Area Code, 0..65535, INT_MAX if unknown */
    int cid; /* 28-bit UMTS Cell Identity described in TS 25.331, 0..268435455,
              * INT_MAX if unknown */
    int psc; /* 9-bit UMTS Primary Scrambling Code described in TS 25.331, 0..511,
              * INT_MAX if unknown */
    int dluarfcn; /* Downlink UTRA Absolute Radio Frequency Channel Number, INT_MAX if unknown */
    int uluarfcn; /* Uplink UTRA Absolute Radio Frequency Channel Number, INT_MAX if unknown */
    int pathloss; /* Pathloss described in TS 25.331 sec 10.3.7.3, 46..158, INT_MAX if unknown */
} RIL_CellIdentityWcdma_v2;

/** RIL_CellIdentityLte_v2 */
typedef struct {
    int mcc; /* 3-digit Mobile Country Code, 0..999, INT_MAX if unknown */
    int mnc; /* 2 or 3-digit Mobile Network Code, 0..999, INT_MAX if unknown */
    int ci; /* 28-bit Cell Identity described in TS ???, INT_MAX if unknown */
    int pci; /* physical cell id 0..503, INT_MAX if unknown */
    int tac; /* 16-bit tracking area code, INT_MAX if unknown */
    int dlearfcn; /* Downlink UTRA Absolute Radio Frequency Channel Number, INT_MAX if unknown */
    int ulearfcn; /* Uplink UTRA Absolute Radio Frequency Channel Number, INT_MAX if unknown */
    int pathloss; /* Pathloss described in TS 36.213 sec 5.1.1.1, 0..2400, INT_MAX if unknown */
} RIL_CellIdentityLte_v2;

/** RIL_CellInfoGsm_v2 */
typedef struct {
  RIL_CellIdentityGsm_v2 cellIdentityGsm;
  RIL_GW_SignalStrength_v2 signalStrengthGsm;
} RIL_CellInfoGsm_v2;

/** RIL_CellInfoWcdma_v2 */
typedef struct {
  RIL_CellIdentityWcdma_v2 cellIdentityWcdma;
  RIL_SignalStrengthWcdma_v2 signalStrengthWcdma;
} RIL_CellInfoWcdma_v2;

/** RIL_CellInfoLte_v2 */
typedef struct {
  RIL_CellIdentityLte_v2 cellIdentityLte;
  RIL_LTE_SignalStrength_v8  signalStrengthLte;
} RIL_CellInfoLte_v2;

typedef struct {
  RIL_CellInfoType  cellInfoType;   /* cell type for selecting from union CellInfo */
  int               registered;     /* !0 if this cell is registered 0 if not registered */
  RIL_TimeStampType timeStampType;  /* type of time stamp represented by timeStamp */
  uint64_t          timeStamp;      /* Time in nanos as returned by ril_nano_time */
  union {
    RIL_CellInfoGsm_v2   gsm;
    RIL_CellInfoCdma     cdma;
    RIL_CellInfoLte_v2   lte;
    RIL_CellInfoWcdma_v2 wcdma;
  } CellInfo;
} RIL_CellInfo_v2;

typedef RIL_SignalStrength_v6 RIL_SignalStrength;
#else // USE_PATCHED_AOSP
typedef RIL_SignalStrength_v11 RIL_SignalStrength;
#endif
/////////////////////////////////////////////////////////////////////////////
// Radio off reasons
//
enum
{
    E_RADIO_OFF_REASON_NONE,
    E_RADIO_OFF_REASON_SHUTDOWN,
    E_RADIO_OFF_REASON_AIRPLANE_MODE
};

///////////////////////////////////////////////////////////////////////////////
// Registration type constants. This is used in determining the parsing
// function to be called, cache variable to be used etc.
//
enum
{
    E_REGISTRATION_TYPE_CREG = 0,
    E_REGISTRATION_TYPE_CGREG,
    E_REGISTRATION_TYPE_CEREG,
    E_REGISTRATION_TYPE_XREG
};

///////////////////////////////////////////////////////////////////////////////
// PIN cache mode constants
//
enum
{
    E_PIN_CACHE_MODE_FS = 1,
    E_PIN_CACHE_MODE_NVRAM = 2
};

///////////////////////////////////////////////////////////////////////////////
// Network selection mode constants
//
enum
{
    E_NETWORK_SELECTION_MODE_AUTOMATIC = 0,
    E_NETWORK_SELECTION_MODE_MANUAL = 1
};

///////////////////////////////////////////////////////////////////////////////
// Device configuration
//
enum
{
    CONFIG_GENERAL = -1,
    CONFIG_ATT = 1
};

///////////////////////////////////////////////////////////////////////////////
// radio power constants
//
const int RADIO_POWER_UNKNOWN = -1;
const int RADIO_POWER_OFF = 0;
const int RADIO_POWER_ON = 1;

///////////////////////////////////////////////////////////////////////////////
// screen state constants
//

const int SCREEN_STATE_UNKNOWN = -1;
const int SCREEN_STATE_OFF = 0;
const int SCREEN_STATE_ON = 1;

///////////////////////////////////////////////////////////////////////////////
// Selectable SIM application list information
//

typedef struct
{
    int appType;
    char szAid[MAX_AID_SIZE];
    char szAppLabel[MAX_APP_LABEL_SIZE];
    int sessionId;
} S_ND_SIM_APP_INFO;

typedef struct
{
    int nApplications;
    S_ND_SIM_APP_INFO aAppInfo[RIL_CARD_MAX_APPS];
} S_ND_SIM_APP_LIST_DATA, *P_ND_SIM_APP_LIST_DATA;


///////////////////////////////////////////////////////////////////////////////
// Retry count information
//
typedef struct
{
    int pin;
    int pin2;
    int puk;
    int puk2;
} S_PIN_RETRY_COUNT;

///////////////////////////////////////////////////////////////////////////////
// Network selection mode parameters
//
const int MAX_OPERATOR_NUMERIC_SIZE = 10;
typedef struct
{
    int mode;
    char szOperatorNumeric[MAX_OPERATOR_NUMERIC_SIZE];
} S_NETWORK_SELECTION_MODE_PARAMS;

///////////////////////////////////////////////////////////////////////////////
// Initial attach apn params
//
typedef struct
{
    char szApn[MAX_APN_SIZE];
    char szPdpType[MAX_PDP_TYPE_SIZE];
} S_INITIAL_ATTACH_APN_PARAMS;

///////////////////////////////////////////////////////////////////////////////
// Data profile information
//
typedef struct
{
    int profileId;
    char szApn[MAX_APN_SIZE];
} S_DATA_PROFILE_INFO;


///////////////////////////////////////////////////////////////////////////////
// Facility Lock Context data
//
typedef struct
{
    UINT32 uiResultCode;
    char szFacilityLock[MAX_FACILITY_CODE];
} S_SET_FACILITY_LOCK_CONTEXT_DATA;

///////////////////////////////////////////////////////////////////////////////
// SIM IO Context data
//
typedef struct
{
    int command;
    int fileId;
} S_SIM_IO_CONTEXT_DATA;

///////////////////////////////////////////////////////////////////////////////
// Setup data call Context data
//
typedef struct
{
    UINT32 uiCID;
} S_SETUP_DATA_CALL_CONTEXT_DATA;

///////////////////////////////////////////////////////////////////////////////
// Data call Info
//
typedef struct
{
    int failCause;
    UINT32 uiCID;
    int state;
    char szPdpType[MAX_PDP_TYPE_SIZE];
    char szInterfaceName[MAX_INTERFACE_NAME_SIZE];
    char szIpAddr1[MAX_IPADDR_SIZE];
    char szIpAddr2[MAX_IPADDR_SIZE];
    char szDNS1[MAX_IPADDR_SIZE];
    char szDNS2[MAX_IPADDR_SIZE];
    char szIpV6DNS1[MAX_IPADDR_SIZE];
    char szIpV6DNS2[MAX_IPADDR_SIZE];
    char szGateways[MAX_IPADDR_SIZE];
} S_DATA_CALL_INFO;

///////////////////////////////////////////////////////////////////////////////
// registration status information used internally
//

const int MAX_REG_STATUS_LENGTH = 8;

typedef struct
{
    char szState[MAX_REG_STATUS_LENGTH];
    char szAcT[MAX_REG_STATUS_LENGTH];
    char szLAC[MAX_REG_STATUS_LENGTH];
    char szCID[MAX_REG_STATUS_LENGTH];
} S_REG_INFO;

typedef struct
{
    int csRegState;
    int psRegState;
    int epsRegState;
} S_NETWORK_REG_STATE_INFO;

// Pref network type request information cache.
typedef struct
{
    RIL_Token token;
    RIL_PreferredNetworkType type;
    size_t datalen;
} PREF_NET_TYPE_REQ_INFO;

///////////////////////////////////////////////////////////////////////////////
// DTMF states
//
enum
{
    E_DTMF_STATE_START = 1,
    E_DTMF_STATE_STOP,
};

///////////////////////////////////////////////////////////////////////////////
// Voice call state information
//
typedef struct
{
    int id;
    int state;
    BOOL bDtmfAllowed;
} S_VOICECALL_STATE_INFO;

///////////////////////////////////////////////////////////////////////////////
// Data call states
//
enum
{
    E_DATA_STATE_IDLE = 0,
    E_DATA_STATE_INITING,
    E_DATA_STATE_ACTIVATING,
    E_DATA_STATE_ACTIVE,
    E_DATA_STATE_DEACTIVATING,
    E_DATA_STATE_DEACTIVATED
};

///////////////////////////////////////////////////////////////////////////////
// Registration States
//
// Check 3GPP 27.007 R11 section 7.2
enum
{
    E_REGISTRATION_NOT_REGISTERED_NOT_SEARCHING = 0,
    E_REGISTRATION_REGISTERED_HOME_NETWORK = 1,
    E_REGISTRATION_NOT_REGISTERED_SEARCHING = 2,
    E_REGISTRATION_DENIED = 3,
    E_REGISTRATION_UNKNOWN = 4,
    E_REGISTRATION_REGISTERED_ROAMING = 5,
    E_REGISTRATION_REGISTERED_FOR_SMS_ONLY_HOME_NETWORK = 6,
    E_REGISTRATION_REGISTERED_FOR_SMS_ONLY_ROAMING = 7,
    E_REGISTRATION_EMERGENCY_SERVICES_ONLY = 8,
    E_REGISTRATION_REGISTERED_FOR_CSFB_NP_HOME_NETWORK = 9,
    E_REGISTRATION_REGISTERED_FOR_CSFB_NP_ROAMING = 10
};

///////////////////////////////////////////////////////////////////////////////
// Call status
//
// Check 3GPP 27.007 section 7.18, 6 and 7 are IMC specific
enum
{
    E_CALL_STATUS_ACTIVE =          0,
    E_CALL_STATUS_HELD =            1,
    E_CALL_STATUS_DIALING =         2,
    E_CALL_STATUS_ALERTING =        3,
    E_CALL_STATUS_INCOMING =        4,
    E_CALL_STATUS_WAITING =         5,
    E_CALL_STATUS_DISCONNECTED =    6,
    E_CALL_STATUS_CONNECTED =       7
};

///////////////////////////////////////////////////////////////////////////////
// SIM States
//
enum
{
    E_SIM_READY = 1,
    E_SIM_PIN,
    E_SIM_PIN2,
    E_SIM_PUK,
    E_SIM_PUK2,
    E_SIM_PH_NETWORK,
    E_SIM_ABSENT
};

///////////////////////////////////////////////////////////////////////////////
// Fast Dormancy modes
//
enum
{
    E_FD_MODE_OEM_MANAGED = 1,
    E_FD_MODE_DISPLAY_DRIVEN,
    E_FD_MODE_ALWAYS_ON
};

///////////////////////////////////////////////////////////////////////////////
// MT CLASS
//
enum
{
    E_MT_CLASS_A = 1,
    E_MT_CLASS_B = 2,
    E_MT_CLASS_CG = 3,
    E_MT_CLASS_CC = 4,
};

///////////////////////////////////////////////////////////////////////////////
// mode used in +XCGEDPAGE
//
enum
{
    E_MODE_ONE_SHOT_DUMP = 0,
    E_MODE_PERIODIC_REFRESHED_DUMP = 1,
    E_MODE_STOP_PERIODIC_DUMP = 2,
    E_MODE_RESET_STATISTICS = 3,
    E_MODE_STOP_EM = 4
};

///////////////////////////////////////////////////////////////////////////////
// XICFG parmameters
//
enum
{
    E_XICFG_IMS_APN = 51,
    E_XICFG_PCSCF_ADDRESS = 102,
    E_XICFG_PCSCF_PORT = 101,
    E_XICFG_IMS_AUTH_MODE = 150,
    E_XICFG_PHONE_CONTEXT = 10,
    E_XICFG_LOCAL_BREAKOUT = 52,
    E_XICFG_XCAP_APN = 200,
    E_XICFG_XCAP_ROOT_URI = 201,
    E_XICFG_XCAP_USER_NAME = 202,
    E_XICFG_XCAP_USER_PASSWORD = 203
};
///////////////////////////////////////////////////////////////////////////////
// XICFG_SET : XICFG write command
// XICFG_GET : XICFG read  command
// XICFG number of parameters used in rril
const int   XICFG_SET = 0;
const int   XICFG_GET  = 1;
const int   XICFG_N_PARAMS  = 10;

///////////////////////////////////////////////////////////////////////////////
// Notify / Result Codes (m_dwCode)
//

const RIL_Errno RRIL_RESULT_OK = RIL_E_SUCCESS;                              // 0x00000000
const RIL_Errno RRIL_RESULT_RADIOOFF = RIL_E_RADIO_NOT_AVAILABLE;            // 0x00000001
const RIL_Errno RRIL_RESULT_ERROR = RIL_E_GENERIC_FAILURE;                   // 0x00000002
const RIL_Errno RRIL_RESULT_PASSWORDINCORRECT = RIL_E_PASSWORD_INCORRECT;    // 0x00000003
const RIL_Errno RRIL_RESULT_SIMPIN2 = RIL_E_SIM_PIN2;                        // 0x00000004
const RIL_Errno RRIL_RESULT_SIMPUK2 = RIL_E_SIM_PUK2;                        // 0x00000005
const RIL_Errno RRIL_RESULT_NOTSUPPORTED = RIL_E_REQUEST_NOT_SUPPORTED;      // 0x00000006
const RIL_Errno RRIL_RESULT_CALLABORTED = RIL_E_CANCELLED;                   // 0x00000007
const RIL_Errno RRIL_RESULT_FDN_FAILURE = RIL_E_FDN_CHECK_FAILURE;           // 0x0000000E
const int RRIL_RESULT_OK_IMMEDIATE = 0x00000010;

// V25 Results
const RIL_Errno RRIL_RESULT_NOCARRIER = RIL_E_GENERIC_FAILURE;
const RIL_Errno RRIL_RESULT_NODIALTONE = RIL_E_GENERIC_FAILURE;
const RIL_Errno RRIL_RESULT_BUSY = RIL_E_GENERIC_FAILURE;
const RIL_Errno RRIL_RESULT_NOANSWER = RIL_E_GENERIC_FAILURE;
const int RRIL_NOTIFY_CONNECT = 0x00010000;


///////////////////////////////////////////////////////////////////////////////
// Error codes (in pBlobs) - used to trigger actions during response handling
//

const int RRIL_E_NO_ERROR = 0x00000000;
const int RRIL_E_UNKNOWN_ERROR = 0x00001000;
const int RRIL_E_ABORT = 0x00001002;
const int RRIL_E_DIALSTRING_TOO_LONG = 0x00001003;
const int RRIL_E_MODEM_NOT_READY = 0x00001004;
const int RRIL_E_TIMED_OUT = 0x00001005;
const int RRIL_E_CANCELLED = 0x00001006;
const int RRIL_E_RADIOOFF = 0x00001007;
const int RRIL_E_MODEM_RESET = 0x00001008;


///////////////////////////////////////////////////////////////////////////////
// Information Classes
//
const int RRIL_INFO_CLASS_NONE = 0x00000000;                       // None
const int RRIL_INFO_CLASS_VOICE = 0x00000001;                      // Voice
const int RRIL_INFO_CLASS_DATA = 0x00000002;                       // Data
const int RRIL_INFO_CLASS_FAX = 0x00000004;                        // Fax
const int RRIL_INFO_CLASS_SMS = 0x00000008;                        // Short Message Service
const int RRIL_INFO_CLASS_DATA_CIRCUIT_SYNC = 0x00000010;          // Data Circuit Sync
const int RRIL_INFO_CLASS_DATA_CIRCUIT_ASYNC = 0x00000020;         // Data Circuit Async
const int RRIL_INFO_CLASS_DEDICATED_PACKET_ACCESS = 0x00000040;    // Dedicated packet access
const int RRIL_INFO_CLASS_DEDICATED_PAD_ACCESS = 0x00000080;       // Dedicated PAD access
const int RRIL_INFO_CLASS_ALL = 0x000000FF;                        // All Infoclasses

///////////////////////////////////////////////////////////////////////////////
// Call Forwarding constants
//
const int RRIL_CALL_FWD_UNCONDITIONAL = 0x00000000;
const int RRIL_CALL_FWD_MOBILE_BUSY = 0x00000001;
const int RRIL_CALL_FWD_NO_REPLY = 0x00000002;
const int RRIL_CALL_FWD_NOT_REACHABLE = 0x00000003;
const int RRIL_CALL_FWD_ALL_CALL_FWD = 0x00000004;
const int RRIL_CALL_FWD_ALL_COND_CALL_FWD = 0x00000005;

///////////////////////////////////////////////////////////////////////////////
// Call Waiting Status constants
//
const int RRIL_CALL_WAIT_DISABLE = 0x00000000;
const int RRIL_CALL_WAIT_ENABLE = 0x00000001;
const int RRIL_CALL_WAIT_QUERY_STATUS = 0x00000002;

///////////////////////////////////////////////////////////////////////////////
// Caller ID Restriction constants
//
const int RRIL_CLIR_NETWORK = 0x00000000;
const int RRIL_CLIR_INVOCATION = 0x00000001;
const int RRIL_CLIR_SUPPRESSION = 0x00000002;

#endif // RRIL_RRIL_H

