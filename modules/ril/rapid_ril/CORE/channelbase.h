////////////////////////////////////////////////////////////////////////////
// channelbase.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines channel-related classes, constants, and structures.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_CHANNEL_H
#define RRIL_CHANNEL_H

#include "com_init_index.h"
#include "port.h"
#include "command.h"
#include "initializer.h"

// forward declarations
class CSilo;
class CThread;

// forward declarations
class CSilo;
class CThread;

const int MAX_COM_PORT_NAME_LENGTH  = 64;


//  Structure used for specifying init strings
struct INITSTRING_DATA
{
    const char* szCmd;
};

// Structure to hold silos
struct SILO_CONTAINER
{
    CSilo* rgpSilos[SILO_MAX];
    UINT32 nSilos;
};

class CChannelBase
{
public:
    CChannelBase(UINT32 uiChannel);
    virtual ~CChannelBase();

private:
    //  Prevent assignment: Declared but not implemented.
    CChannelBase(const CChannelBase& rhs);  // Copy Constructor
    CChannelBase& operator=(const CChannelBase& rhs);  //  Assignment operator

public:
    //  Init functions
    BOOL Initialize();
    BOOL StartChannelThreads();
    BOOL StopChannelThreads();
    virtual UINT32 CommandThread();
    virtual UINT32 ResponseThread();

    BOOL AddSilo(CSilo *pSilo);

    // Channel init command string functions
    BOOL SendModemConfigurationCommands(eComInitIndex eInitIndex);
    char* GetBasicInitCmd() { return m_szChannelBasicInitCmd; }
    char* GetUnlockInitCmd() { return m_szChannelUnlockInitCmd; }

    // Public port interface
    virtual BOOL OpenPort() = 0;
    BOOL InitPort();
    BOOL ClosePort();
    int GetFD() { return m_Port.GetFD(); }

    UINT32 GetRilChannel() const { return m_uiRilChannel; }

    //  Public framework functions
    void ClearCmdThreadBlockedOnRxQueue() { m_bCmdThreadBlockedOnRxQueue = FALSE; }

    BOOL BlockReadThread();
    BOOL UnblockReadThread();

    //  Public interfaces to notify all silos.
    BOOL ParseUnsolicitedResponse(CResponse*
                                const pResponse,
                                const char*& rszPointer,
                                BOOL& fGotoError);

    //  General public functions
    BOOL IsCmdThreadBlockedOnRxQueue() const { return m_bCmdThreadBlockedOnRxQueue; }
//    BOOL FWaitingForRsp() const  { return m_fWaitingForRsp; }

    /*
     * Goes through Tx queue, finds identical request IDs and completes
     * ril request with the provided result code and response.
     */
    virtual int FindIdenticalRequestsAndSendResponses(int reqID,
                                                UINT32 uiResultCode,
                                                void* pResponse,
                                                size_t responseLen,
                                                int callId = -1) = 0;

    // Clear the command queue on initiailization not successful.
    void ClearCommandQueue();

protected:
    //  Init functions
    virtual BOOL FinishInit() = 0;

    // Protected port interface for inside of channel class
    BOOL WriteToPort(const char* pData, UINT32 uiBytesToWrite, UINT32& ruiBytesWritten);
    BOOL ReadFromPort(char* pszReadBuf, UINT32 uiReadBufSize, UINT32& ruiBytesRead);
    BOOL IsPortOpen();
    BOOL WaitForAvailableData(UINT32 uiTimeout);

    //  Framework functions
    virtual BOOL SendCommand(CCommand*& rpCmd) = 0;
    virtual RIL_RESULT_CODE GetResponse(CCommand*& rpCmd, CResponse*& rpRsp) = 0;
    virtual BOOL ParseResponse(CCommand*& rpCmd, CResponse*& rpRsp) = 0;

    // Called at end of ResponseThread()
    // Give GPRS response thread a chance to handle Rx data in Data mode
    virtual BOOL ProcessModemData(char* szData, UINT32 uiRead) = 0;

    //  Framework helper functions
    void SetCmdThreadBlockedOnRxQueue() { m_bCmdThreadBlockedOnRxQueue = TRUE; }
    BOOL WaitForCommand();

    char* GetTESpecificInitCommands(eComInitIndex eInitIndex);

protected:
    //  Member variables
    UINT32 m_uiRilChannel;

    UINT32 m_bCmdThreadBlockedOnRxQueue : 1;
    UINT32 m_bTimeoutWaitingForResponse : 1;
    UINT32 m_fWaitingForRsp : 1;
    UINT32 m_fLastCommandTimedOut : 1;
    UINT32 m_fFinalInitOK : 1;

    CThread* m_pCmdThread;
    CThread* m_pReadThread;

    CEvent* m_pBlockReadThreadEvent;
    BOOL m_bReadThreadBlocked;

    UINT32 m_uiLockCommandQueue;
    UINT32 m_uiLockCommandQueueTimeout;

    INITSTRING_DATA* m_paInitCmdStrings;

    SILO_CONTAINER m_SiloContainer;

    CPort m_Port;

    //  When closing and opening the port (in case of AT command timeout),
    //  need to ignore possible invalid file descriptor errors while port is
    //  temporarily closed.
    BOOL m_bPossibleInvalidFD;
    CMutex* m_pPossibleInvalidFDMutex;
    CMutex* m_pResponseObjectAccessMutex;

    // channel init command strings
    char m_szChannelBasicInitCmd[MAX_BUFFER_SIZE];
    char m_szChannelUnlockInitCmd[MAX_BUFFER_SIZE];
};

#endif //RRIL_CHANNEL_H
