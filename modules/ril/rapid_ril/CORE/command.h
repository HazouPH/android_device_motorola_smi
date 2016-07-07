////////////////////////////////////////////////////////////////////////////
// command.h
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

#ifndef RRIL_COMMAND_H
#define RRIL_COMMAND_H

#include "rril.h"
#include "types.h"
#include "rilchannels.h"
#include "systemmanager.h"
#include "rril_OEM.h"

class CContext;
class CTE;

typedef RIL_RESULT_CODE (CTE::*PFN_TE_PARSE) (RESPONSE_DATA& rRspData);
typedef void (CTE::*PFN_TE_POSTCMDHANDLER) (POST_CMD_HANDLER_DATA& rRspData);

class CCommand
{
public:
    CCommand(   UINT32 uiChannel,
                RIL_Token token,
                int reqId,
                const char* pszATCmd,
                PFN_TE_PARSE pParseFcn = NULL,
                PFN_TE_POSTCMDHANDLER pHandlerFcn = NULL);

    CCommand(   UINT32 uiChannel,
                RIL_Token token,
                int reqId,
                const char* pszATCmd1,
                const char* pszATCmd2,
                PFN_TE_PARSE pParseFcn = NULL,
                PFN_TE_POSTCMDHANDLER pHandlerFcn = NULL);

    CCommand(   UINT32 uiChannel,
                RIL_Token token,
                int reqId,
                REQUEST_DATA reqData,
                PFN_TE_PARSE pParseFcn = NULL,
                PFN_TE_POSTCMDHANDLER pHandlerFcn = NULL);

    ~CCommand();

private:
    //  Prevent assignment: Declared but not implemented.
    CCommand(const CCommand& rhs);  // Copy Constructor
    CCommand& operator=(const CCommand& rhs);  //  Assignment operator


public:
    UINT32              GetChannel()        { return m_uiChannel;   };
    RIL_Token           GetToken()          { return m_token;       };
    int                 GetRequestID()      { return m_reqId;     };
    char*               GetATCmd1()         { return m_pszATCmd1;   };
    char*               GetATCmd2()         { return m_pszATCmd2;   };
    PFN_TE_PARSE        GetParseFcn()       { return m_pParseFcn;   };
    PFN_TE_POSTCMDHANDLER  GetPostCmdHandlerFcn() { return m_pPostCmdHandlerFcn; };

    UINT32              GetTimeout()        { return m_uiTimeout;   };
    CContext*           GetContext()        { return m_pContext;    };
    void*               GetContextData()    { return m_pContextData;};
    UINT32              GetContextDataSize(){ return m_cbContextData;};
    void*               GetContextData2()    { return m_pContextData2;};
    UINT32              GetContextDataSize2(){ return m_cbContextData2;};

    int                 GetCallId()         { return m_callId; }
    void                SetCallId(int id)   { m_callId = id; }

    BOOL                IsAlwaysParse()     { return m_fAlwaysParse; };
    BOOL                IsHighPriority()    { return m_fHighPriority; };
    BOOL                IsInitCommand()     { return m_fIsInitCommand; };

    void SetTimeout(UINT32 uiTimeout)       { m_uiTimeout = uiTimeout;  };
    void SetAlwaysParse()                   { m_fAlwaysParse = TRUE;    };
    void SetHighPriority()                  { m_fHighPriority = TRUE; };
    void SetInitCommand()                   { m_fIsInitCommand = TRUE; };
    void SetContext(CContext*& pContext)    { m_pContext = pContext; pContext = NULL; };
    void SetContextData(void* pData)        { m_pContextData = pData; };
    void SetContextDataSize(UINT32 nSize)   { m_cbContextData = nSize; };
    void SetContextData2(void* pData)       { m_pContextData2 = pData; };
    void SetContextDataSize2(UINT32 nSize)  { m_cbContextData2 = nSize; };

    void FreeContextData();

    static BOOL AddCmdToQueue(CCommand*& pCmd, BOOL bFront = false);

private:

    UINT32              m_uiChannel;
    RIL_Token           m_token;
    int                 m_reqId;
    char*               m_pszATCmd1;
    char*               m_pszATCmd2;
    PFN_TE_PARSE        m_pParseFcn;
    PFN_TE_POSTCMDHANDLER  m_pPostCmdHandlerFcn;
    UINT32              m_uiTimeout;
    BOOL                m_fAlwaysParse;
    BOOL                m_fHighPriority;
    BOOL                m_fIsInitCommand;
    CContext*           m_pContext;
    void*               m_pContextData;
    UINT32              m_cbContextData;
    void*               m_pContextData2;
    UINT32              m_cbContextData2;
    int                 m_callId;
};

#endif
