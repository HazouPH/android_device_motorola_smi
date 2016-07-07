////////////////////////////////////////////////////////////////////////////
// bertlv_util.cpp
//
// Copyright (C) Intel 2013.
//
//
// Description:
//     Defines BERTLV parsing function
//
/////////////////////////////////////////////////////////////////////////////

#include "rillog.h"
#include "bertlv_util.h"

BOOL BerTlv::Parse(const UINT8* pRawData, UINT32 cbSize)
{
    if (2 > cbSize)
    {
        // Not enough data for a TLV.
        return FALSE;
    }

    // Tag at index 0.
    BYTE bTag = pRawData[0];

    if (0x00 == bTag || 0xFF == bTag)
    {
        // Invalid Tag
        return FALSE;
    }


    // Encoded length starts at index 1
    UINT8 bValuePos = 0;
    UINT32 uLen = 0;

    if (0x80 == (0x80 & pRawData[1]))
    {
        UINT8 bLenBytes = 0x7F & pRawData[1];

        if (1 < bLenBytes || 3 > cbSize)
        {
            // Currently only support 1 extra length byte
            return FALSE;
        }

        uLen = pRawData[2];
        bValuePos = 3;
    }
    else
    {
        uLen = pRawData[1];
        bValuePos = 2;
    }

    // Verify there is enough data available for the value
    if (uLen + bValuePos > cbSize)
    {
        return FALSE;
    }

    // Verify length and value size are consistent.
    if (cbSize - bValuePos < uLen)
    {
        // Try and recover using the minimum value.
        uLen = cbSize - bValuePos;
    }

    m_bTag = bTag;
    m_uLen = uLen;
    m_pbValue = pRawData + bValuePos;
    m_uTotalSize = uLen + bValuePos;

    return TRUE;
}
