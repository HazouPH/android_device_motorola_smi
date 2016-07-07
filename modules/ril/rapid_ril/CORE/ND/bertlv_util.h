////////////////////////////////////////////////////////////////////////////
// bertlv_util.h
//
// Copyright (C) Intel 2013.
//
//
// Description:
//    Defines BERTLV parsing function
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BERTLV_UTIL_H
#define BERTLV_UTIL_H

#include "types.h"
#include "nd_structs.h"

class BerTlv
{
public:
    BerTlv() : m_bTag(0), m_uLen(0), m_pbValue(NULL), m_uTotalSize(0) {}
    ~BerTlv() {};

    BYTE GetTag() {return m_bTag;}
    UINT32 GetLength() {return m_uLen;}
    const UINT8* GetValue() {return m_pbValue;}
    UINT32 GetTotalSize() {return m_uTotalSize;}

    BOOL Parse(const UINT8* pRawData, UINT32 cbSize);


private:
    BYTE m_bTag;
    UINT32 m_uLen;
    const UINT8* m_pbValue;
    UINT32 m_uTotalSize;
};
#endif // BERTLV_UTIL_H
