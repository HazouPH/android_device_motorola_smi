/*
 *
 *
 * Copyright (C) 2009 Intrinsyc Software International,
 * Inc.  All Rights Reserved
 *
 * Use of this code is subject to the terms of the
 * written agreement between you and Intrinsyc.
 *
 * UNLESS OTHERWISE AGREED IN WRITING, THIS WORK IS
 * DELIVERED ON AN AS IS BASIS WITHOUT WARRANTY,
 * REPRESENTATION OR CONDITION OF ANY KIND, ORAL OR
 * WRITTEN, EXPRESS OR IMPLIED, IN FACT OR IN LAW
 * INCLUDING WITHOUT LIMITATION, THE IMPLIED WARRANTIES
 * OR CONDITIONS OF MERCHANTABLE QUALITY
 * AND FITNESS FOR A PARTICULAR PURPOSE
 *
 * This work may be subject to patent protection in the
 *  United States and other jurisdictions
 *
 * Description:
 *    Implementation of Non-Volatile storage for Android
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "types.h"
#include "rril.h"
#include "repository.h"
#include "rillog.h"


//////////////////////////////////////////////////////////////////////////
// Standardized Non-Volatile Memory Access Strings

const char   g_szGroupRequestTimeouts[]        = "RequestTimeouts";

// See request_info.h for the request names used in setting request
// parameters (ie. timeouts, retries) in repository.txt

/////////////////////////////////////////////////

const char   g_szGroupInternalRequestTimeouts[]= "InternalRequestTimeouts";
const char   g_szGroupOtherTimeouts[]          = "OtherTimeouts";

const char   g_szTimeoutCmdInit[]              = "TimeoutCmdInit";
const char   g_szTimeoutAPIDefault[]           = "TimeoutAPIDefault";
const char   g_szTimeoutWaitForInit[]          = "TimeoutWaitForInit";
const char   g_szTimeoutWaitForXIREG[]         = "TimeoutWaitForXIREG";

/////////////////////////////////////////////////

const char   g_szGroupRILSettings[]            = "RILSettings";

const char   g_szTimeoutThresholdForRetry[]    = "TimeoutThresholdForRetry";
const char   g_szOpenPortRetries[]             = "OpenPortRetries";
const char   g_szOpenPortInterval[]            = "OpenPortInterval";
const char   g_szPinCacheMode[]                = "PinCacheMode";

/////////////////////////////////////////////////

const char   g_szGroupInitCmds[]               = "InitCmds";

const char* g_szPreInitCmds[] =
{
    "PreInitCmds",      // COM_INIT_INDEX
    "PreReinitCmds",    // COM_REINIT_INDEX
    "PreUnlockCmds",    // COM_UNLOCK_INDEX
    "PreSmsInitCmds",   // COM_SMSINIT_INDEX
};

const char* g_szPostInitCmds[] =
{
    "PostInitCmds",     // COM_INIT_INDEX
    "PostReinitCmds",   // COM_REINIT_INDEX
    "PostUnlockCmds",   // COM_UNLOCK_INDEX
    "PostSmsInitCmds",  // COM_SMSINIT_INDEX
};

/////////////////////////////////////////////////

const char   g_szGroupModem[]                   = "Modem";

const char   g_szNetworkInterfaceNamePrefix[]   = "NetworkInterfaceNamePrefix";
const char   g_szMTU[]                          = "MTU";
const char   g_szEnableCellInfo[]               = "EnableCellInfo";
const char   g_szEnableCipheringInd[]           = "EnableCipheringInd";
const char   g_szEnableModemOffInFlightMode[]   = "EnableModemOffInFlightMode";
const char   g_szFDDelayTimer[]                 = "FDDelayTimer";
const char   g_szSCRITimer[]                    = "SCRITimer";
const char   g_szFDMode[]                       = "FDMode";
const char   g_szTempOoSNotificationEnable[]    = "TempOoSNotificationEnable";
const char   g_szVoiceCapable[]                 = "VoiceCapable";
const char   g_szDataCapable[]                  = "DataCapable";
const char   g_szSmsOverCSCapable[]             = "SmsOverCSCapable";
const char   g_szSmsOverPSCapable[]             = "SmsOverPSCapable";
const char   g_szStkCapable[]                   = "StkCapable";
const char   g_szEnableXDATASTATURC[]           = "EnableXDATASTATReporting";
const char   g_szEnableSMSOverIP[]              = "EnableSMSOverIP";
const char   g_szSupportCGPIAF[]                = "SupportCGPIAF";
const char   g_szNwInitiatedContextAct[]        = "NwInitiatedContextAct";
const char   g_szEnableSignalStrengthURC[]      = "EnableSignalStrengthReporting";
const char   g_szImeiBlackList[]                = "ImeiBlackList";

// used for 7x60 and 6360 modems
const char   g_szModemResourceName[]    = "ModemDataChannelResourceName";
const char   g_szIpcDataChannelMin[]    = "IpcDataChannelMin";
const char   g_szHsiDataDirect[]        = "HsiDataDirect";
const char   g_szApnTypeDefault[]       = "ApnTypeDefault";
const char   g_szApnTypeDUN[]           = "ApnTypeDUN";
const char   g_szApnTypeIMS[]           = "ApnTypeIMS";
const char   g_szApnTypeMMS[]           = "ApnTypeMMS";
const char   g_szApnTypeCBS[]           = "ApnTypeCBS";
const char   g_szApnTypeFOTA[]          = "ApnTypeFOTA";
const char   g_szApnTypeSUPL[]          = "ApnTypeSUPL";
const char   g_szApnTypeEmergency[]     = "ApnTypeEmergency";

// used for SATK
const char   g_szTeProfile[]             = "TeProfile";
const char   g_szMtMask[]                = "MtMask";

// used for Signal Strength reporting configuration in bar mapping mode
const char g_sz2GParameters[]            = "BarMapping2GParameters";
const char g_sz3GParameters[]            = "BarMapping3GParameters";
const char g_szLteParameters[]           = "BarMappingLteParameters";

/////////////////////////////////////////////////

const char   g_szGroupLogging[]                 = "Logging";

const char   g_szCallDropReporting[]            = "CallDropReporting";
const char   g_szLogLevel[]                     = "LogLevel";

//////////////////////////////////////////////////////////////////////////

const char   g_szGroupChannelSilos[]    = "ChannelSiloConfiguration";

const char   g_szSilosATCmd[]           = "SilosATCmdChannel";
const char   g_szSilosDLC2[]            = "SilosDLC2Channel";
const char   g_szSilosDLC6[]            = "SilosDLC6Channel";
const char   g_szSilosDLC8[]            = "SilosDLC8Channel";
const char   g_szSilosDLC22[]           = "SilosDLC22Channel";
const char   g_szSilosDLC23[]           = "SilosDLC23Channel";
const char   g_szSilosSms[]             = "SilosSmsChannel";
const char   g_szSilosURC[]             = "SilosURCChannel";
const char   g_szSilosOEM[]             = "SilosOEMChannel";
const char   g_szSilosData[]            = "SilosDataChannel";

//////////////////////////////////////////////////////////////////////////
// Structs and Enums.

enum
{
    E_ERROR     = -1,
    E_OK        =  0,
    E_NOT_FOUND =  1,
    E_EOF       =  2
};


//////////////////////////////////////////////////////////////////////////
// Class-Specific Strings

static const char* REPO_DIR = "/system/etc/telephony";
static const char* GROUP_MARKER = "Group";
static const int   GROUP_MARKER_LEN = 5;


//////////////////////////////////////////////////////////////////////////
// Variable Initialization

struct CRepository::CRepLock CRepository::m_stLock = {{0}, {0}, 0, 0};
bool CRepository::m_bInitialized = FALSE;
char CRepository::m_cRepoPath[MAX_MODEM_NAME_LEN];


//////////////////////////////////////////////////////////////////////////
// CRepository Class Implementation

CRepository::CRepository() : m_iFd(-1)
{
}

CRepository::~CRepository()
{
}

BOOL CRepository::Init(const char* szConfigFile)
{
    m_bInitialized = FALSE;

    CRepository::m_stLock.iReaders = 0;
    CRepository::m_stLock.iReadWaiters = 0;

    if (0 != pthread_mutex_init(&m_stLock.stLock, NULL))
    {
        goto Error;
    }

    if (0 != pthread_cond_init(&m_stLock.stRead, NULL))
    {
        goto Error;
    }

    m_bInitialized = TRUE;

    snprintf(m_cRepoPath, MAX_MODEM_NAME_LEN, "%s/%s", REPO_DIR, szConfigFile);

Error:
    return m_bInitialized;
}

BOOL CRepository::Close()
{
    if (m_bInitialized)
    {
        pthread_mutex_destroy(&m_stLock.stLock);
        pthread_cond_destroy(&m_stLock.stRead);
        m_bInitialized = FALSE;
    }

    return TRUE;
}

BOOL CRepository::OpenRepositoryFile()
{
    BOOL fRetVal = FALSE;

    m_iLine = 1;
    m_iFd   = open(m_cRepoPath, O_RDONLY);

    if (m_iFd < 0)
    {
        RIL_LOG_CRITICAL("OpenRepositoryFile()-Could not open file \"%s\" - %s\r\n",
                m_cRepoPath, strerror(errno));
        goto Error;
    }

    // seek to beginning of file
    ResetFile();

    fRetVal = TRUE;

Error:
    return fRetVal;
}

void CRepository::CloseRepositoryFile()
{
    if (m_iFd >= 0)
    {
        close(m_iFd);
        m_iFd = -1;
    }
}

BOOL CRepository::Read(const char* szGroup, const char* szKey, int& iRes)
{
    BOOL fRetVal = FALSE;
    char szBuf[MAX_INT_LEN];

    fRetVal = Read(szGroup, szKey, szBuf, MAX_INT_LEN);
    if (fRetVal)
    {
        char* remaining;

        iRes = strtol(szBuf, &remaining, 0); // 0: base used is determined by format in sequence
        fRetVal = (szBuf != remaining);
    }

    return fRetVal;
}

BOOL CRepository::Read(const char* szGroup, const char* szKey, char* szRes, int iMaxLen)
{
    char  szBuf[MAX_LINE_LEN];
    char* pBuf = szBuf;
    int   iRetVal = E_ERROR;
    int   iRes;

    if (!m_bInitialized)
    {
        RIL_LOG_CRITICAL("CRepository::Read() - Repository has not been initialized.\r\n");
        goto Error;
    }

    // grab lock before accessing the NVM file
    ReaderLock();

    if (!OpenRepositoryFile())
    {
        RIL_LOG_CRITICAL("CRepository::Read() - Could not open the Repository file.\r\n");
        goto Error;
    }

    // look for group
    iRes = LocateGroup(szGroup);
    if (E_OK != iRes)
    {
        RIL_LOG_INFO("CRepository::Read() - Could not locate the \"%s\" group.\r\n", szGroup);
        goto Error;
    }

    // look for key in group
    iRes = LocateKey(szKey, pBuf);
    if (E_OK != iRes)
    {
        RIL_LOG_VERBOSE("CRepository::Read() - Could not locate the \"%s\" key.\r\n", szKey);
        goto Error;
    }

    // get value associated to key
    RemoveComment(pBuf);
    iRetVal = ExtractValue(pBuf, szRes, iMaxLen);

Error:
    CloseRepositoryFile();
    ReaderUnlock();

    return (E_OK == iRetVal);
}

// Read Fast Dormancy parameters from repository
// check parameter consistency according to modem range
// if value in undefined or out of range, parameter is replaced by empty string meaning "use value
//  stored in modem NVRAM"
BOOL CRepository::ReadFDParam(const char* szGroup, const char* szKey, char* szRes, int iMaxLen,
                                                                                   int iMinVal,
                                                                                   int iMaxVal)
{
    int param = 0;
    if (Read(szGroup, szKey, szRes, iMaxLen))
    {
        // Check param consistency
        sscanf(szRes, "%d", &param);
        // Replace szRes by empty string in case of value out of range
        if (param < iMinVal || param > iMaxVal)
        {
            szRes[0] = 0;
            RIL_LOG_WARNING("CRepository::ReadFDParam() -"
                    "FD Parameter \"%s\" out of range, use NVRAM modem value.\r\n", szKey);
        }
    }
    else
    {
        szRes[0] = 0;
        RIL_LOG_WARNING("CRepository::ReadFDParam() -"
                "FD Parameter \"%s\" not found, use NVRAM modem value.\r\n", szKey);
    }

    return (E_OK);
}

int CRepository::DumpLines(int iFd, int iFrom, int iTo)
{
    int iRetVal = E_ERROR;

    if (E_OK != GotoLine(iFrom))
        goto Error;

    while (m_iLine != iTo)
    {
        char szBuf[MAX_LINE_LEN];
        int  iSize;
        int  iRes;

        iRes = ReadLine(szBuf, FALSE);
        if (E_OK != iRes && E_EOF != iRes)
            goto Error;

        iSize = strlen(szBuf);
        if (write(iFd, szBuf, iSize) != iSize)
            goto Error;

        if (E_EOF == iRes)
            break;
    }

    iRetVal = E_OK;

Error:
    return iRetVal;
}

void CRepository::ResetFile()
{
    lseek(m_iFd, 0, SEEK_SET);
    m_iLine = 1;
}

int CRepository::GotoLine(int iLine)
{
    int iRetVal = E_ERROR;

    if (iLine < m_iLine)
        ResetFile();

    while (m_iLine != iLine)
    {
        if (E_OK != ReadLine())
            goto Error;
    }

    iRetVal = E_OK;

Error:
    return iRetVal;
}

int CRepository::ReadLine(char* szBuf, bool bRemoveCRAndLF)
{
    int  iRetVal = E_ERROR;
    int  iCount = 0;
    char cCh;

    // TODO: limit to 256 characters

    while(1)
    {
        switch(read(m_iFd, &cCh, 1))
        {
        case 1:
            if (cCh == '\n' || cCh == '\r')
            {
                if (szBuf)
                {
                    if (!bRemoveCRAndLF)
                        szBuf[iCount++] = cCh;

                    szBuf[iCount] = '\0';
                }
                iRetVal = E_OK;
                ++m_iLine;
                goto Done;
            }
            else if (szBuf)
            {
                szBuf[iCount++] = cCh;
            }
            break;

        case 0:
            // end of file
            if (szBuf)
                szBuf[iCount] = '\0';

            iRetVal = E_EOF;
            goto Done;
            break;  // unreachable

        default:
            // error
            if (szBuf)
                szBuf[iCount] = '\0';

            iRetVal = E_ERROR;
            goto Done;
            break;  // unreachable
        }
    }

Done:
    return iRetVal;
}


void CRepository::RemoveComment(char* szIn)
{
    // locate // marker
    const char* szMarker = "//";
    char* pSubStr = strstr(szIn, szMarker);
    if (pSubStr)
        *pSubStr= '\0';
}

char* CRepository::SkipSpace(char* szIn)
{
    if (szIn != NULL)
    {
        for (; *szIn != '\0' && isspace(*szIn); ++szIn);
    }
    return szIn;
}

void CRepository::RemoveTrailingSpaces(char* szIn)
{
    if (NULL != szIn)
    {
        int indexWalk = strlen(szIn);

        while (0 < indexWalk)
        {
            // If the current character is a space, replace it with NULL.
            // If the current character is NULL, skip it.
            // Otherwise, we are done.

            if (' ' == szIn[indexWalk])
            {
                szIn[indexWalk] = '\0';
            }
            else if ('\0' != szIn[indexWalk])
            {
                break;
            }

            indexWalk--;
        }
    }
}

char* CRepository::SkipAlphaNum(char* szIn)
{
    if (szIn != NULL)
    {
        for (; *szIn != '\0' && isalnum(*szIn); ++szIn);
    }
    return szIn;
}

int CRepository::LocateGroup(const char* szGroup)
{
    char  szBuf[MAX_LINE_LEN];
    char* pBuf;
    int   iCount = 0;
    int   iRetVal = E_ERROR;

    while (1)
    {
        int iRes = ReadLine(szBuf);
        switch(iRes)
        {
            case E_OK:
            case E_EOF:
                pBuf= SkipSpace(szBuf);
                if (strncmp(pBuf, GROUP_MARKER, GROUP_MARKER_LEN) == 0)
                {
                    // is this our group?
                    pBuf += GROUP_MARKER_LEN;
                    pBuf = SkipSpace(pBuf);
                    if (strncmp(pBuf, szGroup, strlen(szGroup)) == 0)
                    {
                        // group found
                        iRetVal = E_OK;
                        goto Done;
                    }
                }
                if (E_EOF == iRes)
                {
                    iRetVal = E_NOT_FOUND;
                    goto Done;
                }
                break;

            default:
                iRetVal = E_ERROR;
                goto Done;
                break;  // unreachable
        }
    }

Done:
    return iRetVal;
}


int CRepository::LocateKey(const char* szKey, char* szOut)
{
    char  szBuf[MAX_LINE_LEN];
    char* pBuf;
    int   iRetVal = E_ERROR;
    int   iKeyLen = strlen(szKey);

    while (1)
    {
        int iRes = ReadLine(szBuf);
        switch(iRes)
        {
            case E_OK:
            case E_EOF:
                pBuf= SkipSpace(szBuf);
                if (strncmp(pBuf, szKey, iKeyLen) == 0)
                {
                    if (szOut)
                    {
                        strcpy(szOut, szBuf);
                    }

                    iRetVal = E_OK;
                    goto Done;
                }
                else if (strncmp(pBuf, GROUP_MARKER, GROUP_MARKER_LEN) == 0)
                {
                    // into next group
                    iRetVal = E_NOT_FOUND;
                    goto Done;
                }

                if (E_EOF == iRes)
                {
                    iRetVal = E_NOT_FOUND;
                    goto Done;
                }
                break;

            default:
                iRetVal = E_ERROR;
                goto Done;
                break;  // unreachable
        }
    }

Done:
    return iRetVal;
}

int CRepository::ExtractValue(char* szIn, char* szRes, UINT32 iMaxLen)
{
    // skip key
    szIn = SkipSpace(szIn);
    szIn = SkipAlphaNum(szIn);
    szIn = SkipSpace(szIn);
    RemoveTrailingSpaces(szIn);

    // copy value
    if (strlen(szIn) >= iMaxLen)
    {
        strncpy(szRes, szIn, iMaxLen - 1);
        szRes[iMaxLen - 1] = '\0';
    }
    else
    {
        strcpy(szRes, szIn);
    }

    return E_OK;
}

int CRepository::ReaderLock()
{
    int iRetVal = E_ERROR;

    if (0 == pthread_mutex_lock(&m_stLock.stLock))
    {
        ++m_stLock.iReaders;
        iRetVal = (pthread_mutex_unlock(&m_stLock.stLock) == 0) ? E_OK : E_ERROR;
    }

Error:
    return iRetVal;
}

int CRepository::ReaderUnlock()
{
    int iRetVal = E_ERROR;

    if (0 != pthread_mutex_lock(&m_stLock.stLock))
        goto Error;

    --m_stLock.iReaders;
    iRetVal = (pthread_mutex_unlock(&m_stLock.stLock) == 0) ? E_OK : E_ERROR;

Error:
    return iRetVal;
}

