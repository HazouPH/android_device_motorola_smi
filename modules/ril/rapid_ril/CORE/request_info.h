////////////////////////////////////////////////////////////////////////////
// request_info.h
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the request info structures.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_REQUEST_INFO_H
#define RRIL_REQUEST_INFO_H

#include "request_id.h"

// Struct used for standard Android requests defined in ril.h.
struct REQ_INFO
{
    const char* szName; // request name used for setting request params in repository.txt
    UINT32 uiChannel;
    UINT32 uiTimeout;
};

// Struct used for internal requests only. The values for internal request ids must
// not conflict with values in ril.h
struct REQ_INFO_INTERNAL
{
     REQ_INFO reqInfo;
     E_REQ_ID_INTERNAL reqId; // see E_REQ_ID_INTERNAL enum in request_id.h for possible values
};

extern REQ_INFO* g_pReqInfo;
extern const REQ_INFO g_ReqInfoDefault[];
extern REQ_INFO_INTERNAL g_ReqInternal[];

extern const int REQ_ID_TOTAL;
extern const int INTERNAL_REQ_ID_TOTAL;

#endif // RRIL_REQUEST_INFO_H
