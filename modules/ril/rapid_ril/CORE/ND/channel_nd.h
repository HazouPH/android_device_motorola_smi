////////////////////////////////////////////////////////////////////////////
// channel_nd.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines channel-related classes, constants, and structures.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef RRIL_CHANNEL_ND_H
#define RRIL_CHANNEL_ND_H

#include "channelbase.h"

typedef enum {
    FLUSH_CHANNEL_NO_FLUSH,
    FLUSH_CHANNEL_FLUSH_BUFFER,
    FLUSH_CHANNEL_FLUSH_TTY,
    FLUSH_CHANNEL_FLUSH_ALL,
} FLUSH_CHANNEL;

typedef enum {
    BLOCK_CHANNEL_NO_BLOCK,
    BLOCK_CHANNEL_BLOCK_THREAD,
    BLOCK_CHANNEL_BLOCK_TTY,
    BLOCK_CHANNEL_BLOCK_ALL,
} BLOCK_CHANNEL;

typedef enum {
    UNBLOCK_CHANNEL_NO_UNBLOCK,
    UNBLOCK_CHANNEL_UNBLOCK_THREAD,
    UNBLOCK_CHANNEL_UNBLOCK_TTY,
    UNBLOCK_CHANNEL_UNBLOCK_ALL,
} UNBLOCK_CHANNEL;

class CCommand;
class CResponse;

class CChannel : public CChannelBase
{
public:
    CChannel(UINT32 uiChannel);
    virtual ~CChannel();

private:
    //  Prevent assignment: Declared but not implemented.
    CChannel(const CChannel& rhs);  // Copy Constructor
    CChannel& operator=(const CChannel& rhs);  //  Assignment operator

public:
    //  Init functions
    virtual UINT32 CommandThread()  { return CChannelBase::CommandThread(); }
    virtual UINT32 ResponseThread() { return CChannelBase::ResponseThread(); }

    virtual void FlushResponse();

    // Public Flush functions
    virtual BOOL BlockAndFlushChannel(BLOCK_CHANNEL blockLevel, FLUSH_CHANNEL flushLevel);
    virtual BOOL FlushAndUnblockChannel(UNBLOCK_CHANNEL blockLevel, FLUSH_CHANNEL flushLevel);

    /*
     * Goes through Tx queue, finds identical request IDs and completes
     * ril request with the provided result code and response.
     */
    virtual int FindIdenticalRequestsAndSendResponses(int reqID,
                                                        UINT32 uiResultCode,
                                                        void* pResponse,
                                                        size_t responseLen,
                                                        int callId = -1);

protected:
    //  Init functions
    virtual BOOL FinishInit() = 0;

    //  Framework functions
    virtual BOOL SendCommand(CCommand*& rpCmd);
    virtual RIL_RESULT_CODE GetResponse(CCommand*& rpCmd, CResponse*& rpRsp);
    virtual BOOL ParseResponse(CCommand*& rpCmd, CResponse*& rpRsp);

    // Called at end of ResponseThread()
    // Give GPRS response thread a chance to handle Rx data in Data mode
    virtual BOOL ProcessModemData(char* szData, UINT32 uiRead);

    //  Handle the timeout scenario (ABORT command, PING)
    virtual BOOL HandleTimeout(CCommand*& rpCmd, CResponse*& rpRsp,
            UINT32 uiCmdIndex);

    //  Helper function to determine whether to send phase 2 of a command
    BOOL SendCommandPhase2(const UINT32 uiResCode, const int reqID) const;

private:
    // Helper functions
    RIL_RESULT_CODE ReadQueue(CResponse*& rpRsp, UINT32 uiTimeout);
    BOOL ProcessResponse(CResponse*& rpRsp);
    BOOL ProcessNoop(CResponse*& rpRsp);
    void WaitForResponse(CCommand*& rpCmd, CResponse*& pResponse);

    //  helper function to request modem restart due to command time-out
    void RequestCleanUpOnCommandTimeout(CCommand* rpCmd, UINT32 uiCmdIndex);

protected:
    CResponse* m_pResponse;
};


#endif // RRIL_CHANNEL_ND_H

