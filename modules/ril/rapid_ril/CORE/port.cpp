////////////////////////////////////////////////////////////////////////////
// port.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implements a com port interface
//
/////////////////////////////////////////////////////////////////////////////

// system include
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

// local includes
#include "types.h"
#include "rril.h"
#include "sync_ops.h"
#include "util.h"
#include "rillog.h"
#include "port.h"
#include "rildmain.h"
#include "reset.h"
#include "repository.h"

CPort::CPort() :
    m_fIsPortOpen(FALSE),
    m_pFile(NULL)
{
}

CPort::~CPort()
{
    if (m_fIsPortOpen)
    {
        if (!Close())
        {
            RIL_LOG_CRITICAL("CPort::~CPort() - Failed to close port!\r\n");
        }
    }

    delete m_pFile;
    m_pFile = NULL;
}

int CPort::GetFD()
{
    int fd = -1;
    if (m_fIsPortOpen)
    {
        fd = CFile::GetFD(m_pFile);
    }
    return fd;
}

BOOL CPort::Open(const char* pszFileName, BOOL fIsSocket)
{
    RIL_LOG_VERBOSE("CPort::Open() - Enter  fIsSocket=[%d]\r\n", fIsSocket);
    BOOL fRet = FALSE;

    if (NULL == pszFileName)
    {
        RIL_LOG_CRITICAL("CPort::Open() - pszFileName is NULL!\r\n");
        return FALSE;
    }

    if (!m_fIsPortOpen)
    {
        if (NULL == m_pFile)
        {
            m_pFile = new CFile();
        }

        if (fIsSocket)
        {
            fRet = OpenSocket(pszFileName);
        }
        else
        {
            fRet = OpenPort(pszFileName);
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CPort::Open() - Port is already open!\r\n");
    }

    RIL_LOG_VERBOSE("CPort::Open() - Exit\r\n");
    return fRet;
}

BOOL CPort::Init()
{
    RIL_LOG_VERBOSE("CPort::Init() - Enter\r\n");
    BOOL fRet = FALSE;

    int fd = 0;
    int bit = 0;
    int flag;
    struct termios oldtio;
    struct termios newtio;

    if (m_fIsPortOpen)
    {
        fd = CFile::GetFD(m_pFile);

        if (fd >= 0)
        {
            // save current port settings
            tcgetattr(fd,&oldtio);

            // switch TTY to NON BLOCKING mode for Rd/Wr RRIL operations
            flag = fcntl(fd, F_GETFL, 0);
            flag |= O_NONBLOCK;

            if (fcntl(fd, F_SETFL, flag) < 0)
                perror("fcntl()");

            bzero(&newtio, sizeof(newtio));
            newtio.c_cflag = B115200;

            newtio.c_cflag |= CS8 | CLOCAL | CREAD;
            newtio.c_iflag &= ~(INPCK | IGNPAR | PARMRK | ISTRIP | IXANY | ICRNL);
            newtio.c_oflag &= ~OPOST;
            newtio.c_cc[VMIN]  = 1;
            newtio.c_cc[VTIME] = 10;

            // set input mode (non-canonical, no echo,...)
            newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
            //newtio.c_lflag &= ~(ICANON | ISIG);

            //newtio.c_cc[VTIME]    = 1000;   /* inter-character timer unused */
            //newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

            tcflush(fd, TCIFLUSH);
            tcsetattr(fd,TCSANOW,&newtio);

            bit = TIOCM_DTR;
            /*
            if(ioctl(fd, TIOCMBIS, &bit))
            {
                RIL_LOG_CRITICAL("CPort::Init() - ioctl(%d, 0x%x, 0x%x) failed with error="
                        " %d\r\n", errno);
            }
            else
            {
                fRet = TRUE;
            }
            */
            fRet = TRUE;
        }
        else
        {
            RIL_LOG_CRITICAL("CPort::Init() - File descriptor was negative!\r\n");
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CPort::Init() - Port is not open!\r\n");
    }

    RIL_LOG_VERBOSE("CPort::Init() - Exit\r\n");
    return fRet;
}

BOOL CPort::Close()
{
    RIL_LOG_VERBOSE("CPort::Close() - Enter\r\n");
    BOOL fRet = FALSE;

    if (!m_fIsPortOpen)
    {
        RIL_LOG_CRITICAL("CPort::Close() - Port is already closed!\r\n");
    }
    else
    {
        m_fIsPortOpen = FALSE;
        fRet = CFile::Close(m_pFile);

        if (!fRet)
        {
            RIL_LOG_CRITICAL("CPort::Close() - Unable to properly close port!\r\n");
        }
    }

    delete m_pFile;
    m_pFile = NULL;

    RIL_LOG_VERBOSE("CPort::Close() - Exit\r\n");
    return fRet;
}

BOOL CPort::Read(char* pszReadBuf, UINT32 uiReadBufSize, UINT32& ruiBytesRead)
{
    RIL_LOG_VERBOSE("CPort::Read() - Enter\r\n");
    BOOL fRet = FALSE;

    ruiBytesRead = 0;

    if (m_fIsPortOpen)
    {
        if (CFile::Read(m_pFile, pszReadBuf, uiReadBufSize, ruiBytesRead))
        {
            fRet = TRUE;
        }
        else
        {
            RIL_LOG_CRITICAL("CPort::Read() - Unable to read from port\r\n");
        }
    }
    else
    {
        CModemRestart::SaveRequestReason(3, "Port read error", "port is not open",
                CFile::GetName(m_pFile));

        RIL_LOG_CRITICAL("CPort::Read() - Port is not open!\r\n");
    }

    RIL_LOG_VERBOSE("CPort::Read() - Exit\r\n");
    return fRet;
}

BOOL CPort::Write(const char* pszWriteBuf, const UINT32 uiBytesToWrite, UINT32& ruiBytesWritten)
{
    RIL_LOG_VERBOSE("CPort::Write() - Enter\r\n");
    BOOL fRet = FALSE;

    ruiBytesWritten = 0;

    if (m_fIsPortOpen)
    {
        if (CFile::Write(m_pFile, pszWriteBuf, uiBytesToWrite, ruiBytesWritten))
        {
            fRet = TRUE;
        }
        else
        {
            RIL_LOG_CRITICAL("CPort::Write() - Unable to write to port\r\n");
        }
    }
    else
    {
        CModemRestart::SaveRequestReason(3, "Port write error", "port is not open",
                CFile::GetName(m_pFile));

        RIL_LOG_CRITICAL("CPort::Write() - Port is not open!\r\n");
    }

    RIL_LOG_VERBOSE("CPort::Write() - Exit\r\n");
    return fRet;
}

BOOL CPort::OpenPort(const char* pszFileName)
{
    RIL_LOG_VERBOSE("CPort::OpenPort() - Enter\r\n");
    BOOL fRet = FALSE;

    if (NULL == m_pFile)
    {
        RIL_LOG_CRITICAL("CPort::OpenPort() - m_pFile is NULL");
        return fRet;
    }

    fRet = CFile::Open(m_pFile, pszFileName, FILE_ACCESS_READ_WRITE,
            FILE_OPEN_EXIST);

    if (fRet)
    {
        m_fIsPortOpen = TRUE;
    }
    else
    {
        RIL_LOG_CRITICAL("CPort::OpenPort()  CANNOT OPEN PORT\r\n");
    }

    RIL_LOG_VERBOSE("CPort::OpenPort() - Exit\r\n");
    return fRet;
}

BOOL CPort::OpenSocket(const char* pszSocketName)
{
    RIL_LOG_VERBOSE("CPort::OpenSocket() - Enter\r\n");

    // TODO : Pull this from repository
    const char szSocketInit[] = "gsm";

    UINT32 uiBytesWritten = 0;
    UINT32 uiBytesRead = 0;
    char szResponse[10] = {0};

    BOOL fRet = FALSE;

    const UINT32 uiRetries = 30;
    const UINT32 uiInterval = 2000;

    for (UINT32 uiAttempts = 0; uiAttempts < uiRetries; uiAttempts++)
    {
        fRet = CFile::Open(m_pFile, pszSocketName, 0, 0, TRUE);

        if (fRet)
        {
            m_fIsPortOpen = TRUE;
            RIL_LOG_INFO("CPort::OpenSocket() - Port is open!!\r\n");
            break;
        }

        Sleep(uiInterval);
    }

    if (fRet)
    {
        if (Write(szSocketInit, strlen(szSocketInit), uiBytesWritten))
        {
            if (Read(szResponse, sizeof(szResponse), uiBytesRead))
            {
                m_fIsPortOpen = TRUE;
            }
            else
            {
                RIL_LOG_CRITICAL("CPort::OpenSocket() - Unable to read response from socket\r\n");
                fRet = FALSE;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CPort::OpenSocket() - Unable to write \"%s\" to socket\r\n",
                    szSocketInit);
            fRet = FALSE;
        }
    }

    RIL_LOG_VERBOSE("CPort::OpenSocket() - Exit\r\n");
    return fRet;
}

BOOL CPort::WaitForAvailableData(UINT32 uiTimeout)
{
    RIL_LOG_VERBOSE("CPort::WaitForAvailableData() - Enter\r\n");
    BOOL fRet = FALSE;
    UINT32 uiMask = 0;

    if (m_fIsPortOpen)
    {
        fRet = CFile::WaitForEvent(m_pFile, uiMask, uiTimeout);

        if(fRet)
        {
            if (uiMask & FILE_EVENT_ERROR)
            {
                RIL_LOG_CRITICAL("CPort::WaitForAvailableData() - FILE_EVENT_ERROR received"
                        " on port\r\n");
            }

            if (uiMask & FILE_EVENT_BREAK)
            {
                RIL_LOG_INFO("CPort::WaitForAvailableData() - FILE_EVENT_BREAK received on"
                        " port\r\n");
            }

            if (uiMask & FILE_EVENT_RXCHAR)
            {
                fRet = TRUE;
            }
        }
        else
        {
            RIL_LOG_CRITICAL("CPort::WaitForAvailableData() - Returning failure\r\n");
        }
    }
    else
    {
        CModemRestart::SaveRequestReason(3, "Port is not open", "", CFile::GetName(m_pFile));
        RIL_LOG_CRITICAL("CPort::WaitForAvailableData() - Port is not open!\r\n");
    }

Error:
    RIL_LOG_VERBOSE("CPort::WaitForAvailableData() - Exit\r\n");
    return fRet;
}
