////////////////////////////////////////////////////////////////////////////
// types.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Basic types for RapidRIL
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(__RIL_TYPES_H__)
#define __RIL_TYPES_H__

#include <cstdint>
#include <cstring>
#include <cerrno>

typedef bool                BOOL;
typedef uint8_t             BYTE;
typedef uint8_t             UINT8;
typedef int8_t              INT8;
typedef uint16_t            UINT16;
typedef int16_t             INT16;
typedef uint32_t            UINT32;
typedef int32_t             INT32;
typedef long int            LONG;
typedef unsigned long int   ULONG;

const bool FALSE = false;
const bool TRUE = true;

#endif  // __RIL_TYPES_H__
