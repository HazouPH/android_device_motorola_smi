////////////////////////////////////////////////////////////////////////////
// file_ops.h
//
// Copyright 2005-2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Handles file opening, closing, reading, and writing.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __file_ops_h__
#define __file_ops_h__

#include "rril.h"

// Access flags (choose one)
const unsigned int FILE_ACCESS_READ_ONLY = 0x0001;
const unsigned int FILE_ACCESS_WRITE_ONLY = 0x0002;
const unsigned int FILE_ACCESS_READ_WRITE = 0x0003;

// Open flags (bitmask)
const unsigned int FILE_OPEN_NON_BLOCK = 0x0001;
const unsigned int FILE_OPEN_APPEND = 0x0002;
const unsigned int FILE_OPEN_CREATE = 0x0004;
const unsigned int FILE_OPEN_TRUNCATE = 0x0008;
const unsigned int FILE_OPEN_EXCL = 0x0010;
const unsigned int FILE_OPEN_EXIST = 0x0020;
const unsigned int FILE_OPEN_NO_CTTY = 0x0030;

// File attribute flags
const unsigned int FILE_ATTRIB_REG = 0x00000001;
const unsigned int FILE_ATTRIB_DIR = 0x00000002;
const unsigned int FILE_ATTRIB_SOCK = 0x00000004;
const unsigned int FILE_ATTRIB_RO = 0x00000008;
const unsigned int FILE_ATTRIB_BLK = 0x00000010;
const unsigned int FILE_ATTRIB_CHR = 0x00000020;
const unsigned int FILE_ATTRIB_DOESNT_EXIST = 0xFFFFFFFF;

// Event codes (bitmask)
const unsigned int FILE_EVENT_RXCHAR = 0x00000001;
const unsigned int FILE_EVENT_BREAK = 0x00000002;
const unsigned int FILE_EVENT_ERROR = 0x00000004;

class CFile
{
public:
    CFile();
    ~CFile();

    static BOOL Open(CFile* pFile, const char* pszFileName, UINT32 dwAccessFlags,
                                                              UINT32 dwOpenFlags,
                                                              BOOL fIsSocket = FALSE);
    static BOOL Close(CFile* pFile);

    static BOOL Read(CFile* pFile, void* pBuffer, UINT32 dwBytesToRead,
                                                 UINT32& rdwBytesRead);
    static BOOL Write(CFile* pFile, const void* pBuffer,
                                  UINT32 dwBytesToWrite,
                                  UINT32& rdwBytesWritten);

    static BOOL WaitForEvent(CFile* pFile, UINT32& rdwFlags, UINT32 dwTimeoutInMS = WAIT_FOREVER);

    static int GetFD(CFile* pFile);

    static inline char* GetName(CFile* pFile) { return pFile->m_pszFileName; }

private:

    BOOL  Open(const char* pszFileName, UINT32 dwAccessFlags, UINT32 dwOpenFlags,
                                                               BOOL fIsSocket = FALSE);
    BOOL  Close();

    BOOL  Read(void* pBuffer, UINT32 dwBytesToRead, UINT32& rdwBytesRead);
    BOOL  Write(const void* pBuffer, UINT32 dwBytesToWrite, UINT32& rdwBytesWritten);

    BOOL  WaitForEvent(UINT32& rdwFlags, UINT32 dwTimeoutInMS = WAIT_FOREVER);

    UINT32 GetFileAttributes(const char* pszFileName);

    BOOL    OpenSocket(const char* pszSocketName);

    static const char* const NULL_FILENAME;
    inline const char* GetPrintName()
            { return m_pszFileName == NULL ? NULL_FILENAME : m_pszFileName; }

    int    m_file;

    BOOL   m_fInitialized;

    char*  m_pszFileName;
};

#endif // __file_ops_h__
