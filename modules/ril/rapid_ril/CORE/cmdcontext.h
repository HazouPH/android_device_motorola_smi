////////////////////////////////////////////////////////////////////////////
// cmccontext.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Interface for command context used to execute command
//    specific actions
//
/////////////////////////////////////////////////////////////////////////////

#if !defined (__RIL_CMD_CONTEXT__)
#define __RIL_CMD_CONTEXT__

#include "sync_ops.h"
#include "com_init_index.h"
#include "rilchannels.h"
#include "rildmain.h"

// forward declaration
class CCommand;

class CContext
{
public:
    CContext() {}
    virtual ~CContext() {}

    // interface
    virtual void Execute(BOOL, UINT32) = 0;

private:
    // copy constructor and assigment operator disallowed
    CContext(const CContext& rhs);
    const CContext& operator= (const CContext& rhs);
};

class CContextContainer : public CContext
{
public:
    CContextContainer() { m_pFront = m_pBack = NULL;}
    virtual ~CContextContainer();

    virtual void Execute(BOOL, UINT32);
    void Add(CContext*);

private:
    struct ListNode
    {
        CContext*   m_pContext;
        ListNode*   m_pNext;

        ListNode(CContext* pContext, ListNode* pNext = NULL)
          : m_pContext(pContext), m_pNext(pNext) { }
    };

    ListNode* m_pFront;
    ListNode* m_pBack;
};

class CContextEvent : public CContext
{
public:
    CContextEvent(CEvent& pEvent) : m_pEvent(pEvent) {}
    virtual ~CContextEvent() {}

    virtual void Execute(BOOL, UINT32);

private:
    CEvent&     m_pEvent;
};

class CContextInitString : public CContext
{
public:
    CContextInitString(eComInitIndex eInitIndex, UINT32 uiChannel, BOOL bFinalCmd)
        : m_eInitIndex(eInitIndex), m_uiChannel(uiChannel), m_bFinalCmd(bFinalCmd) {}
    virtual ~CContextInitString() {}

    virtual void Execute(BOOL, UINT32);

private:
    eComInitIndex m_eInitIndex;
    UINT32 m_uiChannel;
    BOOL m_bFinalCmd;
};


#endif  // __RIL_CMD_CONTEXT__
