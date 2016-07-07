////////////////////////////////////////////////////////////////////////////
// request_id.h
//
// Copyright 2009-2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines internal request ids used by RRIL.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_REQUEST_ID_H
#define RRIL_REQUEST_ID_H

const int INTERNAL_REQ_ID_START = 500;

// Used to index into the internal request info array g_ReqInternal[] in request_info.cpp.
enum REQ_IDX
{
    E_REQ_IDX_SILENT_PIN_ENTRY,
    E_REQ_IDX_QUERY_SIM_SMS_STORE_STATUS,
    E_REQ_IDX_SET_PROFILE_DOWNLOAD_FOR_NEXT_UICC_STARTUP,
    E_REQ_IDX_CONFIGURE_USAT_PROFILE_DOWNLOAD,
    E_REQ_IDX_QUERY_UICC_STATE,
    E_REQ_IDX_READ_USAT_PROFILES,
    E_REQ_IDX_WRITE_USAT_PROFILE,
    E_REQ_IDX_RESET_UICC,
    E_REQ_IDX_ENABLE_PROFILE_FACILITY_HANDLING
};

// For internal request ids, we start at 500 as not to conflict with values in ril.h
typedef enum
{
     E_REQ_ID_INTERNAL_SILENT_PIN_ENTRY = INTERNAL_REQ_ID_START,
     E_REQ_ID_INTERNAL_QUERY_SIM_SMS_STORE_STATUS,
     E_REQ_ID_INTERNAL_SET_PROFILE_DOWNLOAD_FOR_NEXT_UICC_STARTUP,
     E_REQ_ID_INTERNAL_CONFIGURE_USAT_PROFILE_DOWNLOAD,
     E_REQ_ID_INTERNAL_QUERY_UICC_STATE,
     E_REQ_ID_INTERNAL_READ_USAT_PROFILES,
     E_REQ_ID_INTERNAL_WRITE_USAT_PROFILE,
     E_REQ_ID_INTERNAL_RESET_UICC,
     E_REQ_ID_INTERNAL_ENABLE_PROFILE_FACILITY_HANDLING
} E_REQ_ID_INTERNAL;

const int REQ_ID_NONE = -1;

#endif // RRIL_REQUEST_ID_H
