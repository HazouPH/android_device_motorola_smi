////////////////////////////////////////////////////////////////////////////
// port.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implements a com port interface
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_PORT_H
#define RRIL_PORT_H

#include "file_ops.h"
#include "rril.h"

class CPort
{
public:
    CPort();
    ~CPort();

private:
    //  Prevent assignment: Declared but not implemented.
    CPort(const CPort& rhs);  // Copy Constructor
    CPort& operator=(const CPort& rhs);  //  Assignment operator


public:
    BOOL Open(const char* pszFileName, BOOL fIsSocket);
    BOOL Init();

    BOOL Close();

    BOOL Read(char* pszReadBuf, UINT32 uiReadBufSize, UINT32& ruiBytesRead);
    BOOL Write(const char* pszWriteBuf, const UINT32 uiBytesToWrite, UINT32& ruiBytesWritten);

    BOOL WaitForAvailableData(UINT32 uiTimeout);

    BOOL IsOpen()   {   return m_fIsPortOpen;   };

    int  GetFD();

private:
    BOOL OpenPort(const char* pszFileName);
    BOOL OpenSocket(const char* pszSocketName);

    BOOL        m_fIsPortOpen;
    CFile*      m_pFile;
};

#endif
