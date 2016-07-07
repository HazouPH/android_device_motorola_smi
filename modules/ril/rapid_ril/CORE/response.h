////////////////////////////////////////////////////////////////////////////
// response.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines classes, constants, and structures used by generic helper
//    functions used to process modem responses to AT Commands.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_RESPONSE_H
#define RRIL_RESPONSE_H

#include "command.h"
#include "util.h"

///////////////////////////////////////////////////////////////////////////////
class CChannel;

///////////////////////////////////////////////////////////////////////////////
// Class handling the response to an AT command
//

class CResponse : public CSelfExpandBuffer
{
public:
    CResponse(CChannel* pChannel);
    ~CResponse();

private:
    //  Prevent assignment: Declared but not implemented.
    CResponse(const CResponse& rhs);  // Copy Constructor
    CResponse& operator=(const CResponse& rhs);  //  Assignment operator

public:
    static BOOL TransferData(CResponse*& rpRspIn, CResponse*& rpRspOut);

    // For unsolicited responses, set fCpyMem to FALSE. This ensures our internal string pointers
    // are correct in the memory we return to upper layers. The framework already ensures this
    // memory will be freed.
    BOOL SetData(void* pData, const UINT32 nSize, const BOOL fCpyMem = TRUE);
    void GetData(void*& rpData, UINT32& rnDataSize) const
    {
        rpData = m_pData; rnDataSize = m_uiDataSize;
    }
    void GetData(RESPONSE_DATA& responseData) const
    {
        responseData.pData = m_pData;
        responseData.uiDataSize = m_uiDataSize;
    }
    void FreeData()
    {
            free(m_pData);
            m_pData = NULL;
            m_uiDataSize = 0;
            m_uiResponseEndMarker = 0;
    }

    BOOL IsCompleteResponse();
    BOOL ParseResponse(CCommand*& rpCmd);

    UINT32 GetResultCode() const                           { return m_uiResultCode; };
    void SetResultCode(const UINT32 uiResultCode)         { m_uiResultCode = uiResultCode; };

    UINT32 GetErrorCode() const                            { return m_uiErrorCode; };
    void SetErrorCode(const UINT32 uiErrorCode)           { m_uiErrorCode = uiErrorCode; };

    BOOL IsUnsolicitedFlag() const
    {
        return ( (m_uiFlags & E_RSP_FLAG_UNSOLICITED) ? TRUE : FALSE);
    }
    void SetUnsolicitedFlag(const BOOL bUnsolicited)
    {
        (bUnsolicited ? (m_uiFlags |= E_RSP_FLAG_UNSOLICITED) :
                (m_uiFlags &= ~E_RSP_FLAG_UNSOLICITED));
    }

    BOOL IsUnrecognizedFlag() const
    {
        return ( (m_uiFlags & E_RSP_FLAG_UNRECOGNIZED) ? TRUE : FALSE);
    }
    void SetUnrecognizedFlag(const BOOL bUnrecognized)
    {
        (bUnrecognized ? (m_uiFlags |= E_RSP_FLAG_UNRECOGNIZED) :
                (m_uiFlags &= ~E_RSP_FLAG_UNRECOGNIZED));
    }

    BOOL IsCorruptFlag() const
    {
        return ( (m_uiFlags & E_RSP_FLAG_CORRUPT) ? TRUE : FALSE);
    }
    void SetCorruptFlag(const BOOL bCorrupt)
    {
        (bCorrupt ? (m_uiFlags |= E_RSP_FLAG_CORRUPT) : (m_uiFlags &= ~E_RSP_FLAG_CORRUPT));
    }

    BOOL IsTimedOutFlag() const
    {
        return ( (m_uiFlags & E_RSP_FLAG_TIMEDOUT) ? TRUE : FALSE);
    }
    void SetTimedOutFlag(const BOOL bTimedOut)
    {
        (bTimedOut ? (m_uiFlags |= E_RSP_FLAG_TIMEDOUT) : (m_uiFlags &= ~E_RSP_FLAG_TIMEDOUT));
    }

    BOOL IsIgnoreFlag() const
    {
        return (m_uiFlags & E_RSP_FLAG_IGNORE) ? TRUE : FALSE;
    }
    void SetIgnoreFlag(const BOOL bIgnore)
    {
        bIgnore ? (m_uiFlags |= E_RSP_FLAG_IGNORE) : (m_uiFlags &= ~E_RSP_FLAG_IGNORE);
    }

private:
    enum
    {
        E_RSP_FLAG_UNSOLICITED  = 0x00000001,
        E_RSP_FLAG_UNRECOGNIZED = 0x00000002,
        E_RSP_FLAG_CORRUPT      = 0x00000004,
        E_RSP_FLAG_TIMEDOUT     = 0x00000008,
        E_RSP_FLAG_IGNORE       = 0x00000010
    };

    BOOL IsUnsolicitedResponse();
    BOOL IsExtendedError(const char* pszToken);
    BOOL IsCorruptResponse();
    BOOL IsOkResponse();
    BOOL IsErrorResponse();
    BOOL RetrieveErrorCode(const char*& rszPointer,  UINT32& nCode, const char* pszToken);
    BOOL IsConnectResponse();
    BOOL IsAbortedResponse();

    char      m_szNewLine[3];
    UINT32    m_uiResultCode;
    UINT32    m_uiErrorCode;
    void*     m_pData;
    UINT32    m_uiDataSize;
    CChannel* m_pChannel;
    UINT32    m_uiResponseEndMarker;

    //  internal flags.
    UINT32     m_uiFlags;
};

#endif //RRIL_RESPONSE_H
