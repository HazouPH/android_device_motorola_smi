////////////////////////////////////////////////////////////////////////////
// nd_structs.h
//
// Copyright 2005-2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines blobs for use in Android to support expected return structures
//      of pointer arrays to valid data
//
/////////////////////////////////////////////////////////////////////////////

#ifndef ND_STRUCTS_H
#define ND_STRUCTS_H

#include "rril.h"

// Normally number size is around 15, set max size 50
const int MAX_NUMBER_SIZE = 50;

//
// Struct for reporting Current Call List to Android
//
typedef struct
{
    RIL_Call*           apRilCall    [RRIL_MAX_CALL_ID_COUNT];
    RIL_Call            aRilCall     [RRIL_MAX_CALL_ID_COUNT];
    char                aszNumber    [RRIL_MAX_CALL_ID_COUNT][MAX_NUMBER_SIZE];
    char                aszName      [RRIL_MAX_CALL_ID_COUNT][MAX_CNAP_NAME_SIZE];
} S_ND_CALL_LIST_DATA, *P_ND_CALL_LIST_DATA;


//
// Struct for reporting Current Call Forwarding to Android
//
typedef struct
{
    RIL_CallForwardInfo* apRilCallForwardInfo[RIL_MAX_CALLFWD_ENTRIES];
    RIL_CallForwardInfo  aRilCallForwardInfo[RIL_MAX_CALLFWD_ENTRIES];
    char                 aszCallForwardNumber[RIL_MAX_CALLFWD_ENTRIES][MAX_NUMBER_SIZE];
} S_ND_CALLFWD_DATA, *P_ND_CALLFWD_DATA;

//
// Structs for getting the available operator list to Android
//
typedef struct
{
    char* pszOpInfoLong;
    char* pszOpInfoShort;
    char* pszOpInfoNumeric;
    char* pszOpInfoStatus;
} S_ND_OPINFO_PTRS, *P_ND_OPINFO_PTRS;

// As peer 3GPP 27.007,"7.3 PLMN selection +COPS"
// long alphanumeric format can be upto 16 characters long
// still for some NW operators the long name can exceed 16 chars.
// For safety reason we shall use 40 + 1.
const int MAX_OP_NAME_LONG = 40 + 1;
// short format up to 8 characters
const int MAX_OP_NAME_SHORT = 8 + 1;
// three BCD digit country code plus a three BCD digit network code
const int MAX_OP_NAME_NUM = 6 + 1;
// longest string passed is "forbidden"
const int MAX_OP_NAME_STATUS = 9 + 1;

typedef struct
{
    char szOpInfoLong[MAX_OP_NAME_LONG];
    char szOpInfoShort[MAX_OP_NAME_SHORT];
    char szOpInfoNumeric[MAX_OP_NAME_NUM];
    char szOpInfoStatus[MAX_OP_NAME_STATUS];
} S_ND_OPINFO_DATA, *P_ND_OPINFO_DATA;


const int RIL_MAX_BROADCASTSMSCONFIGINFO_ENTRIES = 10;
//
// Struct for reporting Broadcast SMS config to Android
//
typedef struct
{
    RIL_GSM_BroadcastSmsConfigInfo*
            apRilGSMBroadcastSmsConfigInfo[RIL_MAX_BROADCASTSMSCONFIGINFO_ENTRIES];
    RIL_GSM_BroadcastSmsConfigInfo
            aRilGsmBroadcastSmsConfigInfo[RIL_MAX_BROADCASTSMSCONFIGINFO_ENTRIES];
} S_ND_BROADCASTSMSCONFIGINFO_DATA, *P_ND_BROADCASTSMSCONFIGINFO_DATA;

const int MAX_ACK_PDU_SIZE = 160;

typedef struct
{
    RIL_SMS_Response smsRsp;
    char szAckPDU[MAX_ACK_PDU_SIZE];
} S_ND_SEND_MSG, *P_ND_SEND_MSG;

struct PdpData
{
    char* szRadioTechnology;
    char* szRILDataProfile;
    char* szApn;
    char* szUserName;
    char* szPassword;
    char* szPAPCHAP;
    char* szPDPType;
    char* szHandover;
};

//
// Struct for reporting Setup Data Call to Android
//
typedef struct
{
    RIL_Data_Call_Response_v6   sPDPData;
    char                        szPdpType[MAX_PDP_TYPE_SIZE];
    char                        szNetworkInterfaceName[MAX_INTERFACE_NAME_SIZE];
    char                        szIPAddress[MAX_BUFFER_SIZE];
    char                        szDNS[MAX_BUFFER_SIZE];
    char                        szGateway[MAX_BUFFER_SIZE];
} S_ND_SETUP_DATA_CALL, *P_ND_SETUP_DATA_CALL;

//
// Struct for reporting PDP Context List to Android
//
typedef struct
{
    RIL_Data_Call_Response_v6   aPDPData[MAX_PDP_CONTEXTS];
    char                        aszTypeBuffers[MAX_PDP_CONTEXTS][MAX_PDP_TYPE_SIZE];
    char                        aszIfnameBuffers[MAX_PDP_CONTEXTS][MAX_INTERFACE_NAME_SIZE];
    char                        aszAddressBuffers[MAX_PDP_CONTEXTS][MAX_BUFFER_SIZE];
    char                        aszDnsesBuffers[MAX_PDP_CONTEXTS][MAX_BUFFER_SIZE];
    char                        aszGatewaysBuffers[MAX_PDP_CONTEXTS][MAX_BUFFER_SIZE];
} S_ND_PDP_CONTEXT_DATA, *P_ND_PDP_CONTEXT_DATA;

//
// Struct for reporting Neighboring Cell List to Android
//
// Normally there are 32 neigh cells + 1 serving =33, set max number at 40.
const int RRIL_MAX_CELL_ID_COUNT = 40;
const int CELL_ID_ARRAY_LENGTH = 9;

typedef struct
{
    RIL_NeighboringCell*     apRilNeighboringCell [RRIL_MAX_CELL_ID_COUNT];
    RIL_NeighboringCell      aRilNeighboringCell  [RRIL_MAX_CELL_ID_COUNT];
    char                     aszCellCIDBuffers    [RRIL_MAX_CELL_ID_COUNT][CELL_ID_ARRAY_LENGTH];
} S_ND_N_CELL_DATA, *P_ND_N_CELL_DATA;

typedef struct
{
    RIL_CellInfo_v12      aRilCellInfo[RRIL_MAX_CELL_ID_COUNT];
} S_ND_N_CELL_INFO_DATA_V12, *P_ND_N_CELL_INFO_DATA_V12;

typedef struct
{
    RIL_CellInfo_v2   aRilCellInfo[RRIL_MAX_CELL_ID_COUNT];
} S_ND_N_CELL_INFO_DATA_V2, *P_ND_N_CELL_INFO_DATA_V2;


typedef struct
{
    char* pszStat;
    char* pszLAC;
    char* pszCID;
    char* pszNetworkType;
    char* pszBaseStationID;
    char* pszBaseStationLat;
    char* pszBaseStationLon;
    char* pszConcurrentServices;
    char* pszSystemID;
    char* pszNetworkID;
    char* pszTSB58;
    char* pszPRL;
    char* pszDefaultRoaming;
    char* pszReasonDenied;
    char* pszPrimaryScramblingCode;
} S_ND_REG_STATUS_POINTERS, *P_ND_REG_STATUS_POINTERS;

const int REG_STATUS_LENGTH = 8;

typedef struct
{
    S_ND_REG_STATUS_POINTERS sStatusPointers;
    char szStat[REG_STATUS_LENGTH];
    char szLAC[REG_STATUS_LENGTH];
    char szCID[REG_STATUS_LENGTH];
    char szNetworkType[REG_STATUS_LENGTH];
    char szBaseStationID[REG_STATUS_LENGTH];
    char szBaseStationLat[REG_STATUS_LENGTH];
    char szBaseStationLon[REG_STATUS_LENGTH];
    char szConcurrentServices[REG_STATUS_LENGTH];
    char szSystemID[REG_STATUS_LENGTH];
    char szNetworkID[REG_STATUS_LENGTH];
    char szTSB58[REG_STATUS_LENGTH];
    char szPRL[REG_STATUS_LENGTH];
    char szDefaultRoaming[REG_STATUS_LENGTH];
    char szReasonDenied[REG_STATUS_LENGTH];
    char szPrimaryScramblingCode[REG_STATUS_LENGTH];
} S_ND_REG_STATUS, *P_ND_REG_STATUS;

typedef struct
{
    char* pszStat;
    char* pszLAC;
    char* pszCID;
    char* pszNetworkType;
    char* pszReasonDenied;
    char* pszNumDataCalls;
} S_ND_GPRS_REG_STATUS_POINTERS, *P_ND_GPRS_REG_STATUS_POINTERS;

typedef struct
{
    S_ND_GPRS_REG_STATUS_POINTERS sStatusPointers;
    char szStat[REG_STATUS_LENGTH];
    char szLAC[REG_STATUS_LENGTH];
    char szCID[REG_STATUS_LENGTH];
    char szNetworkType[REG_STATUS_LENGTH];
    char szReasonDenied[REG_STATUS_LENGTH];
    char szNumDataCalls[REG_STATUS_LENGTH];
} S_ND_GPRS_REG_STATUS, *P_ND_GPRS_REG_STATUS;


typedef struct
{
    char* pszOpNameLong;
    char* pszOpNameShort;
    char* pszOpNameNumeric;
} S_ND_OP_NAME_POINTERS, *P_ND_OP_NAME_POINTERS;

typedef struct
{
    S_ND_OP_NAME_POINTERS sOpNamePtrs;
    char szOpNameLong[MAX_OP_NAME_LONG];
    char szOpNameShort[MAX_OP_NAME_SHORT];
    char szOpNameNumeric[MAX_OP_NAME_NUM];
} S_ND_OP_NAMES, *P_ND_OP_NAMES;

typedef struct
{
    char* pszType;
    char* pszMessage;
} S_ND_USSD_POINTERS, *P_ND_USSD_POINTERS;

// As peer 3GPP 27.007, "7.15 Unstructured supplementary service data +CUSD"
// URC is +CUSD: <m>[,<str>,<dcs>], where <m> is of integer type, and between 0 and 5
const int MAX_USSD_TYPE_SIZE = 1 + 1;
// USSD messages are up to 182 alphanumeric characters in length
const int MAX_USSD_MESSAGE_SIZE = 182 + 1;

typedef struct
{
    S_ND_USSD_POINTERS sStatusPointers;
    char szType[MAX_USSD_TYPE_SIZE];
    char szMessage[MAX_USSD_MESSAGE_SIZE];
} S_ND_USSD_STATUS, *P_ND_USSD_STATUS;

const UINT32 MAX_ATR_SIZE = 80;

typedef struct
{
    char* pszATR;
} S_ND_GET_ATR_POINTER, *P_ND_GET_ATR_POINTER;

typedef struct
{
    S_ND_GET_ATR_POINTER sResponsePointer;
    char szATR[MAX_ATR_SIZE];
} S_ND_GET_ATR, *P_ND_GET_ATR;

enum PDP_TYPE
{
    PDP_TYPE_UNKNOWN,
    PDP_TYPE_IPV4,
    PDP_TYPE_IPV6,
    PDP_TYPE_IPV4V6,
};

typedef struct
{
    int nUsed;
    int nTotal;
} S_ND_SIM_SMS_STORAGE, *P_ND_SIM_SMS_STORAGE;

const UINT32 MAX_TEMP_SIZE = 50;
const UINT32 MAX_API_VERSION_SIZE = 3;

//
// Structs for reporting thermal sensor API version to Android
//
typedef struct
{
    char* pszVersion;
}  S_ND_OEM_VERSION_PTR, *P_ND_OEM_VERSION_PTR;

typedef struct
{
    S_ND_OEM_VERSION_PTR sResponsePointer;
    char szVersion[MAX_API_VERSION_SIZE];
} S_ND_OEM_VERSION, *P_ND_OEM_VERSION;

//
// Structs for reporting thermal sensor temperatures to Android
//
typedef struct
{
    char* pszTemperature;
}  S_ND_THERMAL_SENSOR_VALUE_PTR, *P_ND_THERMAL_SENSOR_VALUE_PTR;

typedef struct
{
    S_ND_THERMAL_SENSOR_VALUE_PTR sResponsePointer;
    char pszTemperature[MAX_TEMP_SIZE];
} S_ND_THERMAL_SENSOR_VALUE, *P_ND_THERMAL_SENSOR_VALUE;

//
// Structs for reporting PCSCF
//
typedef struct
{
    char* pszPcscf;
} S_ND_GET_PCSCF_RESPONSE_PTR, *P_ND_GET_PCSCF_RESPONSE_PTR;

typedef struct
{
    S_ND_GET_PCSCF_RESPONSE_PTR sResponsePointer;
    char szPcscf[MAX_BUFFER_SIZE];
} S_ND_GET_PCSCF_RESPONSE, *P_ND_GET_PCSCF_RESPONSE;

//
// Structs for OEM diagnostic commands
//
typedef struct
{
    char* pszGprsCellEnv;
}  S_ND_GPRS_CELL_ENV_PTR, *P_ND_GPRS_CELL_ENV_PTR;

typedef struct
{
    S_ND_GPRS_CELL_ENV_PTR sResponsePointer;
    char pszGprsCellEnv[MAX_BUFFER_SIZE*2];
} S_ND_GPRS_CELL_ENV, *P_ND_GPRS_CELL_ENV;

typedef struct
{
    char* pszDebugScreen;
}  S_ND_DEBUG_SCREEN_PTR, *P_ND_DEBUG_SCREEN_PTR;

typedef struct
{
    S_ND_DEBUG_SCREEN_PTR sResponsePointer;
    char pszDebugScreen[MAX_BUFFER_SIZE*2];
} S_ND_DEBUG_SCREEN, *P_ND_DEBUG_SCREEN;

const UINT32 MAX_SMS_TRANSPORT_MODE_SIZE = 5;
const UINT32 MAX_RF_POWER_CUTBACK_TABLE_SIZE = 15;
const UINT32 MAX_DVP_CONFIG_SIZE = 2;

typedef struct
{
    char* pszService;
}  S_ND_SMS_TRANSPORT_MODE_PTR, *P_ND_SMS_TRANSPORT_MODE_PTR;

typedef struct
{
    S_ND_SMS_TRANSPORT_MODE_PTR sResponsePointer;
    char szService[MAX_SMS_TRANSPORT_MODE_SIZE];
} S_ND_SMS_TRANSPORT_MODE, *P_ND_SMS_TRANSPORT_MODE;

typedef struct
{
    char* pszResponse;
}  S_ND_SEND_AT_RESPONSE_PTR, *P_ND_SEND_AT_RESPONSE_PTR;

const int MAX_AT_RESP_SIZE = 1024;

typedef struct
{
    S_ND_SEND_AT_RESPONSE_PTR sResponsePointer;
    char szResponse[MAX_AT_RESP_SIZE];
} S_ND_SEND_AT_RESPONSE, *P_ND_SEND_AT_RESPONSE;

//
// Structs for retrieving the RF Power Cutback Table
//
typedef struct
{
    char* pszRFPowerCutbackTable;
}  S_ND_RF_POWER_CUTBACK_TABLE_PTR, *P_ND_RF_POWER_CUTBACK_TABLE_PTR;

typedef struct
{
    S_ND_RF_POWER_CUTBACK_TABLE_PTR sResponsePointer;
    char szRFPowerCutbackTable[MAX_RF_POWER_CUTBACK_TABLE_SIZE];
} S_ND_RF_POWER_CUTBACK_TABLE, *P_ND_RF_POWER_CUTBACK_TABLE;

typedef struct
{
    /* it will contain all the pairs (CallId, TransferResult) concatenated */
    char* pszSrvccPairs;
} S_ND_SRVCC_RESPONSE_PTR, *P_ND_SRVCC_RESPONSE_PTR;

// szSrvccPairs contains 2 integers: "<call_id> and <transfer_result>"
// separated by a comma AND a space, set max size 10.
const int MAX_SRVCC_RSP_SIZE = 10;

typedef struct
{
    S_ND_SRVCC_RESPONSE_PTR sResponsePointer;
    char szSrvccPairs[MAX_SRVCC_RSP_SIZE];
} S_ND_SRVCC_RESPONSE_VALUE, *P_ND_SRVCC_RESPONSE_VALUE;

//
// Structs for retrieving the DVP Config
//
typedef struct
{
    char* pszDVPConfig;
} S_ND_GET_DVP_RESPONSE_PTR, *P_ND_GET_DVP_RESPONSE_PTR;

typedef struct
{
    S_ND_GET_DVP_RESPONSE_PTR sResponsePointer;
    char szDVPConfig[MAX_DVP_CONFIG_SIZE];
} S_ND_GET_DVP_RESPONSE, *P_ND_GET_DVP_RESPONSE;

//
// Structs for retrieving current CSG state
//
typedef struct
{
    char* pszCsgCurrentState;
}  S_ND_CSG_CURRENT_STATE_PTR, *P_ND_CSG_CURRENT_STATE_PTR;

// CSG state is an integer, can be 0 or 1
const int MAX_CSG_STATE_SIZE = 1 + 1;

typedef struct
{
    S_ND_CSG_CURRENT_STATE_PTR sResponsePointer;
    char szCsgCurrentState[MAX_CSG_STATE_SIZE];
} S_ND_CSG_CURRENT_STATE, *P_ND_CSG_CURRENT_STATE;

//
// Structs for retrieving current CNAP state
//
// szCnapCurrentState: contains the result of the AT+CNAP?
//                     command after parsing of the response.
//
// values: '0' for service deactivated
//         '1' for service activated
//         '2' for service state unknown
//
typedef struct
{
    char* pszCnapCurrentState;
}  S_ND_CNAP_CURRENT_STATE_PTR, *P_ND_CNAP_CURRENT_STATE_PTR;

typedef struct
{
    S_ND_CNAP_CURRENT_STATE_PTR sResponsePointer;
    char szCnapCurrentState[MAX_BUFFER_SIZE];
} S_ND_CNAP_CURRENT_STATE, *P_ND_CNAP_CURRENT_STATE;

//
// Adaptive Clock Frequency Info  structures
//
typedef struct
{
    /* it will contain all the pairs (centFreq, freqSpread, noisePower) concatenated */
    char* pszAdaptiveClockFrequencyInfo;
}  S_ND_ADPCLK_FREQ_INFO_PTR, *P_ND_ADPCLK_FREQ_INFO_PTR;

typedef struct
{
    S_ND_ADPCLK_FREQ_INFO_PTR sResponsePointer;
    char szAdaptiveClockFrequencyInfo[MAX_BUFFER_SIZE];
} S_ND_ADPCLK_FREQ_INFO, *P_ND_ADPCLK_FREQ_INFO;

// RIL_REQUEST_SIM_AUTHENTICATION authContext values:
// See P2 parameter from 3GPP 31.102 7.1.2 (EVEN INS)
const int P2_AUTH_GSM_CONTEXT = 0x80;
const int P2_AUTH_3G_CONTEXT = 0x81;
// See P2 parameter from 3GPP 31.103 7.1.2 (EVEN INS)
const int P2_AUTH_IMS_AKA = 0x81; // Identical to 3G but not the same P2 table

// +XAUTH <Auth_context_type> values
const int XAUTH_CONTEXT_TYPE_3G = 1;
const int XAUTH_CONTEXT_TYPE_GSM = 2;
const int XAUTH_CONTEXT_TYPE_IMS = 6;

#endif
