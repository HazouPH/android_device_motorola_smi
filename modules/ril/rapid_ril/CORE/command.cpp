////////////////////////////////////////////////////////////////////////////
// command.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CCommand class which stores details required to execute
//    and return the result of a specific RIL API
//
/////////////////////////////////////////////////////////////////////////////

#include "util.h"
#include "sync_ops.h"
#include "cmdcontext.h"
#include "command.h"

CCommand::CCommand( UINT32 uiChannel,
                    RIL_Token token,
                    int reqId,
                    const char* pszATCmd,
                    PFN_TE_PARSE pParseFcn,
                    PFN_TE_POSTCMDHANDLER pHandlerFcn) :
    m_uiChannel(RIL_CHANNEL_ATCMD),
    m_token(token),
    m_reqId(reqId),
    m_pszATCmd1(NULL),
    m_pszATCmd2(NULL),
    m_pParseFcn(pParseFcn),
    m_pPostCmdHandlerFcn(pHandlerFcn),
    m_uiTimeout(0),
    m_fAlwaysParse(FALSE),
    m_fHighPriority(FALSE),
    m_fIsInitCommand(FALSE),
    m_pContext(NULL),
    m_pContextData(NULL),
    m_cbContextData(0),
    m_pContextData2(NULL),
    m_cbContextData2(0),
    m_callId(-1)
{
    if (uiChannel < g_uiRilChannelCurMax)
    {
        m_uiChannel = uiChannel;
    }
    else
    {
        RIL_LOG_CRITICAL("CCommand::CCommand() - Using default channel as given argument"
                " is invalid [%d]\r\n", uiChannel);
    }

    if ((NULL == pszATCmd) || ('\0' == pszATCmd[0]))
    {
        m_pszATCmd1 = NULL;
    }
    else
    {
        UINT32 uiCmdLen = strlen(pszATCmd) + 1;
        m_pszATCmd1 = new char[uiCmdLen];
        memset(m_pszATCmd1, 0, uiCmdLen);
        CopyStringNullTerminate(m_pszATCmd1, pszATCmd, uiCmdLen);
    }
}

CCommand::CCommand( UINT32 uiChannel,
                    RIL_Token token,
                    int reqId,
                    const char* pszATCmd1,
                    const char* pszATCmd2,
                    PFN_TE_PARSE pParseFcn,
                    PFN_TE_POSTCMDHANDLER pHandlerFcn) :
    m_uiChannel(RIL_CHANNEL_ATCMD),
    m_token(token),
    m_reqId(reqId),
    m_pszATCmd1(NULL),
    m_pszATCmd2(NULL),
    m_pParseFcn(pParseFcn),
    m_pPostCmdHandlerFcn(pHandlerFcn),
    m_uiTimeout(0),
    m_fAlwaysParse(FALSE),
    m_fHighPriority(FALSE),
    m_fIsInitCommand(FALSE),
    m_pContext(NULL),
    m_pContextData(NULL),
    m_cbContextData(0),
    m_pContextData2(NULL),
    m_cbContextData2(0),
    m_callId(-1)
{
    if (uiChannel < g_uiRilChannelCurMax)
    {
        m_uiChannel = uiChannel;
    }
    else
    {
        RIL_LOG_CRITICAL("CCommand::CCommand() - Using default channel as given argument is"
                " invalid [%d]\r\n", uiChannel);
    }

    if ((NULL == pszATCmd1) || ('\0' == pszATCmd1[0]))
    {
        m_pszATCmd1 = NULL;
    }
    else
    {
        UINT32 uiCmdLen = strlen(pszATCmd1) + 1;
        m_pszATCmd1 = new char[uiCmdLen];
        memset(m_pszATCmd1, 0, uiCmdLen);
        CopyStringNullTerminate(m_pszATCmd1, pszATCmd1, uiCmdLen);
    }

    if ((NULL == pszATCmd2) || ('\0' == pszATCmd2[0]))
    {
        m_pszATCmd2 = NULL;
    }
    else
    {
        UINT32 uiCmdLen = strlen(pszATCmd2) + 1;
        m_pszATCmd2 = new char[uiCmdLen];
        memset(m_pszATCmd2, 0, uiCmdLen);
        CopyStringNullTerminate(m_pszATCmd2, pszATCmd2, uiCmdLen);
    }
}

CCommand::CCommand( UINT32 uiChannel,
                    RIL_Token token,
                    int reqId,
                    REQUEST_DATA reqData,
                    PFN_TE_PARSE pParseFcn,
                    PFN_TE_POSTCMDHANDLER pHandlerFcn) :
    m_uiChannel(RIL_CHANNEL_ATCMD),
    m_token(token),
    m_reqId(reqId),
    m_pszATCmd1(NULL),
    m_pszATCmd2(NULL),
    m_pParseFcn(pParseFcn),
    m_pPostCmdHandlerFcn(pHandlerFcn),
    m_uiTimeout(reqData.uiTimeout),
    m_fAlwaysParse(reqData.fForceParse),
    m_fHighPriority(FALSE),
    m_fIsInitCommand(FALSE),
    m_pContext(NULL),
    m_pContextData(reqData.pContextData),
    m_cbContextData(reqData.cbContextData),
    m_pContextData2(reqData.pContextData2),
    m_cbContextData2(reqData.cbContextData2),
    m_callId(-1)
{
    if (uiChannel < g_uiRilChannelCurMax)
    {
        m_uiChannel = uiChannel;
    }
    else
    {
        RIL_LOG_CRITICAL("CCommand::CCommand() - Using default channel as given argument is"
                " invalid [%d]\r\n", uiChannel);
    }

    if ('\0' == reqData.szCmd1[0])
    {
        m_pszATCmd1 = NULL;
    }
    else
    {
        UINT32 uiCmdLen = strlen(reqData.szCmd1) + 1;
        m_pszATCmd1 = new char[uiCmdLen];
        memset(m_pszATCmd1, 0, uiCmdLen);
        CopyStringNullTerminate(m_pszATCmd1, reqData.szCmd1, uiCmdLen);
    }

    if ('\0' == reqData.szCmd2[0])
    {
        m_pszATCmd2 = NULL;
    }
    else
    {
        UINT32 uiCmdLen = strlen(reqData.szCmd2) + 1;
        m_pszATCmd2 = new char[uiCmdLen];
        memset(m_pszATCmd2, 0, uiCmdLen);
        CopyStringNullTerminate(m_pszATCmd2, reqData.szCmd2, uiCmdLen);
    }
}

CCommand::~CCommand()
{
    delete[] m_pszATCmd1;
    m_pszATCmd1 = NULL;
    delete[] m_pszATCmd2;
    m_pszATCmd2 = NULL;
    delete m_pContext;
    m_pContext = NULL;
}

void CCommand::FreeContextData()
{
    if (m_cbContextData > 0)
    {
        free(m_pContextData);
        m_pContextData = NULL;
    }

    if (m_cbContextData2 > 0)
    {
        free(m_pContextData2);
        m_pContextData2 = NULL;
    }
}

BOOL CCommand::AddCmdToQueue(CCommand*& rpCmd, BOOL bFront /*=false*/)
{
    RIL_LOG_VERBOSE("CCommand::AddCmdToQueue() - Enter\r\n");

    if (NULL != rpCmd && CSystemManager::GetInstance().IsInitializationSuccessful())
    {
        REQ_INFO reqInfo;
        memset(&reqInfo, 0, sizeof(reqInfo));

        // Get the info about this API
        CSystemManager::GetInstance().GetRequestInfo(rpCmd->GetRequestID(), reqInfo);

        //  A value of "0" for uiTimeout will use the retrieved request info from the registry.
        if (0 == rpCmd->GetTimeout())
        {
            rpCmd->SetTimeout(reqInfo.uiTimeout);
        }

        UINT32 nChannel = rpCmd->GetChannel();
        if (g_pTxQueue[nChannel]->Enqueue(rpCmd, (UINT32)(rpCmd->IsHighPriority()), bFront ))
        {
            // signal Tx thread
            (void) CEvent::Signal(g_TxQueueEvent[nChannel]);

            // The queue owns the command now
            rpCmd = NULL;

            RIL_LOG_VERBOSE("CCommand::AddCmdToQueue() - Exit\r\n");
            return TRUE;
        }
        else
        {
            RIL_LOG_CRITICAL("CCommand::AddCmdToQueue() - Unable to queue command on channel"
                    " [%d]\r\n", rpCmd->m_uiChannel);
        }
    }
    else
    {
        RIL_LOG_CRITICAL("CCommand::AddCmdToQueue() -"
                " Command pointer was NULL or Init Not Complete\r\n");
    }

    RIL_LOG_VERBOSE("CCommand::AddCmdToQueue() - Exit\r\n");
    return FALSE;
}

