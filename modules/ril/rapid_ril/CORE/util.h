////////////////////////////////////////////////////////////////////////////
// util.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides function prototypes for the RIL Utility functions.
//    Also includes related class, constant, and structure definitions.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "rillog.h"
#include "rril.h"
#include "sync_ops.h"

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))


// Copies as much of input string into output as possible and *ALWAYS* appends a NULL
// character at the end. If the output buffer cannot hold all of the input string, a
// NULL will be placed in the last byte of the output buffer. The BOOL return FALSE
// indicates that the output does not contain the entire input string. cbOut is the size
// of the output buffer in bytes.
BOOL CopyStringNullTerminate(char* const pszOut, const char* pszIn, const UINT32 cbOut);

// Prints out the given arguments as per pszFormat into the output buffer. If the resulting
// string is too large, it will be truncated and NULL terminated. The return will be false
// for truncation and any error condition. If an error occurs, pszOut will contain a zero
// length NULL terminated string.
BOOL PrintStringNullTerminate(char* const pszOut, const UINT32 cbOut, const char* pszFormat, ... );

// Adds pszIn to the end of pszOut overwritting its NULL terminating character. Adds a NULL
// character at the end of the string regardless of truncation. If truncation or some other
// error occurs, the function will return FALSE.
BOOL ConcatenateStringNullTerminate(char* const pszOut,
        const size_t cbOut, const char* const pszIn);

class CRLFExpandedString
{
public:
    CRLFExpandedString(const char* const pszIn, const int nInLen);
    ~CRLFExpandedString();

private:
    //  Prevent assignment: Declared but not implemented
    CRLFExpandedString(const CRLFExpandedString& rhs);  // Copy Constructor
    CRLFExpandedString& operator=(const CRLFExpandedString& rhs);  //  Assignment operator


public:
    const char* GetString() { return (NULL != m_pszString) ? m_pszString : "NULL"; };

private:
    char* m_pszString;

};

//
// Function declarations
//

// conversion functions
BYTE SemiByteCharsToByte(const char chHigh, const char chLow);
BOOL GSMHexToGSM(const char* sIn, const UINT32 cbIn, BYTE* sOut,
              const UINT32 cbOut, UINT32& rcbUsed);
BOOL GSMToGSMHex(const BYTE* sIn, const UINT32 cbIn, char* sOut,
              const UINT32 cbOut, UINT32& rcbUsed);
char* ConvertUCS2ToUTF8(const char* pHexBuffer, const UINT32 hexBufferLength);

BOOL IsElementarySimFile(UINT32 dwFileID);

void Sleep(UINT32 dwTimeInMS);
UINT32 GetTickCount();


//
// Class declarations
//


// Doubly-linked list element
class CListElem
{
public:
                CListElem() : m_pNext(NULL), m_pPrev(NULL) {};
    virtual     ~CListElem() {};

    CListElem*  GetNext() const         { return m_pNext; };
    CListElem*  GetPrev() const         { return m_pPrev; };

    void        SetNext(CListElem* p)   { m_pNext = p; };
    void        SetPrev(CListElem* p)   { m_pPrev = p; };

private:
    CListElem*  m_pNext;
    CListElem*  m_pPrev;
};

// Function to be passed to CQueue::Enum().
//    This function should return TRUE for enumeration to stop.
typedef BOOL (*PFN_QUEUE_ENUM)(void* pItem, UINT32 uiData);

// Function to be passed to CQueue::ConditionalGet().
//    This function should return TRUE for for the item to be removed from the queue.
typedef BOOL (*PFN_QUEUE_TEST)(void* pItem, UINT32 uiData);


// Generic queue
template <class Type, UINT32 Size>
class CQueue
{
public:
                    CQueue(const BOOL fDontCallDestructors = FALSE);
    virtual         ~CQueue();

    BOOL            Init(CEvent* const pEvent);
    BOOL            Put(Type* const pItem, const UINT32 dwTimeout);
    RIL_RESULT_CODE Get(Type*& rpItem, const UINT32 dwTimeout);
    BOOL            Peek(Type*& rpItem);
    RIL_RESULT_CODE ConditionalGet(const PFN_QUEUE_TEST pfnTest,
                                            const UINT32 uiData,
                                            Type*& rpItem,
                                            const UINT32 dwTimeout);
    RIL_RESULT_CODE WaitForNextItem(const UINT32 dwTimeout);
    CEvent*         GetPutEvent() { return m_pPutEvent; }
    void            Enum(const PFN_QUEUE_ENUM pfnEnum, const UINT32 uiData, const BOOL fClear);
    BOOL            FEmpty();
    UINT32            GetSize() { return m_uiUsed; }

protected:
    RIL_RESULT_CODE GetInternal(Type*& rpItem, const UINT32 dwTimeout);
    BOOL            PeekInternal(Type*& rpItem);
    BOOL            WaitForEmptySpaceInternal(const UINT32 dwTimeout);
    RIL_RESULT_CODE WaitForNextItemInternal(const UINT32 dwTimeout);

    Type*               m_rgpItems[Size];
    UINT32                m_uiUsed;
    CEvent*             m_pGetEvent;
    CEvent*             m_pPutEvent;
    CEvent*             m_pCancelEvent;
    CMutex*             m_pQueueMutex;
    BOOL                m_fDontCallDestructors;
    BOOL                m_fInited;
};


// Priority queue
template <class Type, UINT32 Size>
class CPriorityQueue : public CQueue<Type, Size>
{
public:
                CPriorityQueue();
                ~CPriorityQueue();

    BOOL        Put(Type* const pItem, const UINT32 dwTimeout);
};


// Function to be passed to CDblList::Enum().
//    This function should return TRUE for enumeration to stop.
typedef BOOL (*PFN_LIST_ENUM)(void* pItem, UINT32 uiData);


// Generic doubly-linked list
template <class Type>
class CDblList
{
public:
            CDblList();
            ~CDblList();

    BOOL    Add(Type* const pAdd);
    BOOL    Remove(const Type* const pRemove);
    void    Enum(const PFN_LIST_ENUM pfnEnum, const UINT32 uiData);

private:
    Type*               m_pElems;
    CMutex*            m_pDblListMutex;
};



//
// Template definitions
//

//
// Queue ctor
//
template <class Type, UINT32 Size>
CQueue<Type, Size>::CQueue(const BOOL fDontCallDestructors /* = FALSE */)
: m_uiUsed(0),
  m_pGetEvent(NULL),
  m_pPutEvent(NULL),
  m_pCancelEvent(NULL),
  m_fDontCallDestructors(fDontCallDestructors),
  m_fInited(FALSE)
{
    // Initialize the critical section
    m_pQueueMutex = new CMutex();

    for (UINT32 i = 0; i < Size; i++)
    {
        m_rgpItems[i] = NULL;
    }
}


//
// Queue dtor
//
template <class Type, UINT32 Size>
CQueue<Type, Size>::~CQueue()
{
    // Get rid of the events
    if (m_pPutEvent)
    {
        delete m_pPutEvent;
        m_pPutEvent = NULL;
    }

    if (m_pGetEvent)
    {
        delete m_pGetEvent;
        m_pGetEvent = NULL;
    }

    // Delete the elements still in the queue
    for (UINT32 i = 0; i < m_uiUsed; i++)
    {
        if (m_fDontCallDestructors)
        {
            // NOTE: We use this to avoid useless asserts when this destructor
            // (called from RIL driver) frees memory allocated by RIL proxy
            FreeBlob(m_rgpItems[i]);
        }
        else
        {
            delete m_rgpItems[i];
            m_rgpItems[i] = NULL;
        }
    }

    // Get rid of the critical section
    delete m_pQueueMutex;
    m_pQueueMutex = NULL;
}


//
// Initalize the queue
//
template <class Type, UINT32 Size>
BOOL CQueue<Type, Size>::Init(CEvent* const pEvent)
{
    if (NULL == pEvent)
    {
        goto Error;
    }

    // If the queue is already initialized, skip this
    if (m_fInited)
    {
        goto Success;
    }

    // Create the events
    m_pGetEvent = new CEvent();
    if (!m_pGetEvent)
    {
        goto Error;
    }

    m_pPutEvent = new CEvent();
    if (!m_pPutEvent)
    {
        goto Error;
    }

    m_pCancelEvent = pEvent;
    m_fInited = TRUE;

Success:
    return TRUE;

Error:
    if (m_pGetEvent)
    {
        delete m_pGetEvent;
        m_pGetEvent = NULL;
    }
    return FALSE;
}


//
// Enqueue an element
//
template <class Type, UINT32 Size>
BOOL CQueue<Type, Size>::Put(Type* const pNew, const UINT32 dwTimeout)
{
    CMutex::Lock(m_pQueueMutex);

    BOOL fRet = FALSE;

    if (FALSE == m_fInited)
    {
        RIL_LOG_CRITICAL("CQueue<Type, Size>::Put() : m_fInited was FALSE\r\n");
        goto Error;
    }

    if (pNew == NULL)
    {
        RIL_LOG_CRITICAL("CQueue<Type, Size>::Put() : pNew was NULL\r\n");
        goto Error;
    }

    if (m_fInited)
    {
        if (!this->WaitForEmptySpaceInternal(dwTimeout))
        {
            goto Error;
        }

        if (m_uiUsed >= Size)
        {
            RIL_LOG_CRITICAL("CQueue<Type, Size>::Put() : No space in the queue\r\n");
            goto Error;
        }

        // We have space in the queue
        m_rgpItems[m_uiUsed] = pNew;
        m_uiUsed++;

        // Signal the Put event
        CEvent::Signal(m_pPutEvent);
    }

    fRet = TRUE;

Error:
    CMutex::Unlock(m_pQueueMutex);
    return fRet;
}


//
// Dequeue an element
//
template <class Type, UINT32 Size>
RIL_RESULT_CODE CQueue<Type, Size>::Get(Type*& rpItem, const UINT32 dwTimeout)
{
    CMutex::Lock(m_pQueueMutex);

    RIL_RESULT_CODE resCode = GetInternal(rpItem, dwTimeout);

    CMutex::Unlock(m_pQueueMutex);
    return resCode;
}


//
// Dequeue an element (internal version)
//
template <class Type, UINT32 Size>
RIL_RESULT_CODE CQueue<Type, Size>::GetInternal(Type*& rpItem, const UINT32 dwTimeout)
{
    RIL_RESULT_CODE retCode = RRIL_E_NO_ERROR;

    // Initialize the returned pointer in case we fail
    rpItem = NULL;

    if (m_fInited)
    {
        retCode = WaitForNextItemInternal(dwTimeout);
        if (RRIL_E_NO_ERROR != retCode)
        {
            goto Error;
        }

        // Got an item in the queue
        rpItem = m_rgpItems[0];
        m_uiUsed--;
        if ( (sizeof(m_rgpItems)/sizeof(m_rgpItems[0])) < m_uiUsed )
        {
            retCode = RRIL_E_ABORT;
            goto Error;
        }
        memmove(m_rgpItems, (BYTE*)m_rgpItems + sizeof(Type*), sizeof(Type*) * m_uiUsed);
        m_rgpItems[m_uiUsed] = NULL;

        // Signal the Get event
        CEvent::Signal(m_pGetEvent);
    }
    else
    {
        retCode = RRIL_E_UNKNOWN_ERROR;
    }

Error:
    return retCode;
}


//
// Retrieve a pointer to the first element in the queue
//
template <class Type, UINT32 Size>
BOOL CQueue<Type, Size>::Peek(Type*& rpItem)
{
    CMutex::Lock(m_pQueueMutex);

    BOOL fRet = PeekInternal(rpItem);

    CMutex::Unlock(m_pQueueMutex);

    return fRet;
}


//
// Retrieve a pointer to the first element in the queue (internal version)
//
template <class Type, UINT32 Size>
BOOL CQueue<Type, Size>::PeekInternal(Type*& rpItem)
{
    BOOL fRet = FALSE;

    if (FALSE == m_fInited)
    {
        RIL_LOG_CRITICAL("CQueue<Type, Size>::PeekInternal() - m_fInited was FALSE\r\n");
        goto Error;
    }

    rpItem = NULL;

    if (!m_uiUsed)
    {
        RIL_LOG_CRITICAL("CQueue<Type, Size>::PeekInternal() - m_uiUsed was 0\r\n");
        goto Error;
    }

    rpItem = m_rgpItems[0];
    if (NULL == rpItem)
    {
        RIL_LOG_CRITICAL("CQueue<Type, Size>::PeekInternal() - rpItem was NULL\r\n");
        goto Error;
    }

    fRet = TRUE;

Error:
    return fRet;
}


//
// Dequeue an element from the queue, if it satisfies a condition
//    (will wait for the next element for a specified timeout)
//
template <class Type, UINT32 Size>
RIL_RESULT_CODE CQueue<Type, Size>::ConditionalGet(const PFN_QUEUE_TEST pfnTest,
                                                            const UINT32 uiData,
                                                            Type*& rpItem,
                                                            const UINT32 dwTimeout)
{
    CMutex::Lock(m_pQueueMutex);

    RIL_RESULT_CODE retCode = RRIL_E_NO_ERROR;

    // Wait for an element to appear in the queue
    retCode = WaitForNextItemInternal(dwTimeout);
    if (RRIL_E_NO_ERROR != retCode)
    {
        goto Error;
    }

    // Peek the first element
    if (!PeekInternal(rpItem))
    {
        retCode = RRIL_E_UNKNOWN_ERROR;
        goto Error;
    }

    // See if the first element satisfies the condition
    if (!pfnTest(rpItem, uiData))
    {
        rpItem = NULL;
        retCode = RRIL_E_UNKNOWN_ERROR;
        goto Error;
    }

    // Retrieve the first element
    if (RRIL_E_NO_ERROR != GetInternal(rpItem, WAIT_FOREVER))
    {
        rpItem = NULL;
        retCode = RRIL_E_UNKNOWN_ERROR;
        goto Error;
    }

Error:
    CMutex::Unlock(m_pQueueMutex);
    return retCode;
}


//
// Wait until an element appears in the queue
//
template <class Type, UINT32 Size>
RIL_RESULT_CODE CQueue<Type, Size>::WaitForNextItem(const UINT32 dwTimeout)
{
    CMutex::Lock(m_pQueueMutex);

    RIL_RESULT_CODE resCode = WaitForNextItemInternal(dwTimeout);

    CMutex::Unlock(m_pQueueMutex);

    return resCode;
}


//
// Wait until an item appears in the queue (internal version)
//
template <class Type, UINT32 Size>
RIL_RESULT_CODE CQueue<Type, Size>::WaitForNextItemInternal(const UINT32 dwTimeout)
{
    CEvent* rgpEvents[2] = { m_pPutEvent, m_pCancelEvent };
    UINT32 dwWait;
    RIL_RESULT_CODE retCode = RRIL_E_NO_ERROR;

    if (m_fInited)
    {
        while(1)
        {
            // Are there any items in the queue?
            if (m_uiUsed)
            {
                // Yes -- proceed
                break;
            }
            else
            {
                // No - need to wait for Put to happen
                CMutex::Unlock(m_pQueueMutex);

                dwWait = CEvent::WaitForAnyEvent(2, rgpEvents, dwTimeout);

                if (WAIT_EVENT_0_SIGNALED + 1 == dwWait)
                {
                    // We hit the terminate event -- quit
                    retCode = RRIL_E_CANCELLED;
                    goto Error;
                }
                else if (WAIT_TIMEDOUT == dwWait)
                {
                    // We timed out-- quit
                    retCode = RRIL_E_TIMED_OUT;
                    goto Error;
                }
                else
                {
                    if (WAIT_EVENT_0_SIGNALED != dwWait)
                    {
                        retCode = RRIL_E_UNKNOWN_ERROR;
                        goto Error;
                    }
                }

                CMutex::Lock(m_pQueueMutex);
            }
        }

        if (m_uiUsed == 0)
        {
            retCode = RRIL_E_UNKNOWN_ERROR;
        }
    }
    else
    {
        retCode = RRIL_E_UNKNOWN_ERROR;
    }

Error:
    return retCode;
}


//
// Wait until an empty slot appears in the queue (internal version)
//
template <class Type, UINT32 Size>
BOOL CQueue<Type, Size>::WaitForEmptySpaceInternal(const UINT32 dwTimeout)
{
    CEvent* rgpEvents[2] = { m_pGetEvent, m_pCancelEvent };
    UINT32 dwWait;
    BOOL fRet = FALSE;

    if (m_fInited)
    {
        while (1)
        {
            // Is there space in the queue?
            if (m_uiUsed < Size)
            {
                // Yes -- proceed
                break;
            }
            else
            {
                if (Size != m_uiUsed)
                {
                    goto Error;
                }

                // No -- need to wait for Get to happen

                CMutex::Unlock(m_pQueueMutex);

                dwWait = CEvent::WaitForAnyEvent(2, rgpEvents, dwTimeout);
                if (WAIT_EVENT_0_SIGNALED != dwWait)
                {
                    // We hit the terminate event or timed out
                    goto Error;
                }

                CMutex::Lock(m_pQueueMutex);
            }
        }
    }

    if (m_uiUsed >= Size)
    {
        goto Error;
    }

    fRet = TRUE;

Error:
    return fRet;
}


//
// Enum all queue elements, calling the provided routine for each item
//
template <class Type, UINT32 Size>
void CQueue<Type, Size>::Enum(const PFN_QUEUE_ENUM pfnEnum, const UINT32 uiData, const BOOL fClear)
{
    CMutex::Lock(m_pQueueMutex);

    if (m_fInited)
    {
        for (UINT32 i = 0; i < m_uiUsed; i++)
        {
            if (pfnEnum((void*)m_rgpItems[i], uiData))
            {
                break;
            }
        }

        if (fClear)
        {
            // We also need to clear the queue
            for (UINT32 i = 0; i < m_uiUsed; i++)
            {
                delete m_rgpItems[i];
                m_rgpItems[i] = NULL;
            }
            m_uiUsed = 0;
        }
    }

    CMutex::Unlock(m_pQueueMutex);
}


//
// Determine if the queue is empty
//
template <class Type, UINT32 Size>
BOOL CQueue<Type, Size>::FEmpty()
{
    CMutex::Lock(m_pQueueMutex);

    BOOL fRet = !m_uiUsed;

    CMutex::Unlock(m_pQueueMutex);
    return fRet;
}


//
// Priority queue ctor
//
template <class Type, UINT32 Size>
CPriorityQueue<Type, Size>::CPriorityQueue()
: CQueue<Type, Size>(FALSE)
{
}


//
// Priority queue dtor
//
template <class Type, UINT32 Size>
CPriorityQueue<Type, Size>::~CPriorityQueue()
{
}


//
// Enqueue an element
//
template <class Type, UINT32 Size>
BOOL CPriorityQueue<Type, Size>::Put(Type* const pNew, const UINT32 dwTimeout)
{
    CMutex::Lock(this->m_pQueueMutex);

    UINT32 i;
    BOOL fRet = FALSE;

    if (FALSE == this->m_fInited)
    {
        RIL_LOG_CRITICAL("CPriorityQueue<Type, Size>::Put() : m_fInited was FALSE\r\n");
        goto Error;
    }

    if (pNew == NULL)
    {
        RIL_LOG_CRITICAL("CPriorityQueue<Type, Size>::Put() : pNew was NULL\r\n");
        goto Error;
    }

    if (this->m_fInited)
    {
        if (!this->WaitForEmptySpaceInternal(dwTimeout))
        {
            goto Error;
        }

        if (this->m_uiUsed >= Size)
        {
            RIL_LOG_CRITICAL("CPriorityQueue<Type, Size>::Put() : m_uiUsed >= Size!\r\n");
            goto Error;
        }

        // We have space in the queue -- find the correct spot for this item
        for (i = 0; i < this->m_uiUsed; i++)
        {
            if (pNew->GetPriority() < this->m_rgpItems[i]->GetPriority())
            {
                // We found the first item whose pri is lower (i.e., greater)
                // than the one for the new item --
                // shift other items in the queue
                memmove(&this->m_rgpItems[i + 1], &this->m_rgpItems[i],
                        sizeof(Type*) * (this->m_uiUsed - i));
                break;
            }
        }

        // Insert the new item
        this->m_rgpItems[i] = pNew;
        this->m_uiUsed++;

        // Signal the Put event
        CEvent::Signal(this->m_pPutEvent);
    }

    fRet = TRUE;

Error:
    CMutex::Unlock(this->m_pQueueMutex);
    return fRet;
}


//
// List ctor
//
template <class Type>
CDblList<Type>::CDblList()
: m_pElems(NULL)
{
    m_pDblListMutex = new CMutex();
}


//
// List dtor
//
template <class Type>
CDblList<Type>::~CDblList()
{
    delete m_pDblListMutex;
    m_pDblListMutex = NULL;
}


//
// Add an item to the list
//
template <class Type>
BOOL CDblList<Type>::Add(Type* const pAdd)
{
    CMutex::Lock(m_pDblListMutex);

    BOOL fRet = FALSE;

    // Check that the new element exists
    if (!pAdd)
    {
        goto Error;
    }

    // Add the new element at the front
    pAdd->SetNext(m_pElems);
    pAdd->SetPrev(NULL);

    if (pAdd->GetNext())
    {
        pAdd->GetNext()->SetPrev(pAdd);
    }

    m_pElems = pAdd;
    fRet = TRUE;

Error:
    CMutex::Unlock(m_pDblListMutex);
    return fRet;
}


//
// Remove an item from the list
//
template <class Type>
BOOL CDblList<Type>::Remove(const Type* const pRemove)
{
    CMutex::Lock(m_pDblListMutex);

    BOOL fRet = FALSE;

    // Check that the element to be removed exists
    if (!pRemove)
    {
        goto Error;
    }

    // Is this element head of the list?
    if (pRemove == m_pElems)
    {
        // Yes
        m_pElems = (Type*)pRemove->GetNext();
    }
    else
    {
        // No
        pRemove->GetPrev()->SetNext(pRemove->GetNext());
    }

    // Re-route the links
    if (pRemove->GetNext())
    {
        pRemove->GetNext()->SetPrev(pRemove->GetPrev());
    }

    fRet = TRUE;

Error:
    CMutex::Unlock(m_pDblListMutex);
    return fRet;
}


//
// Enum all list elements, calling the provided routine for each item
//
template <class Type>
void CDblList<Type>::Enum(const PFN_LIST_ENUM pfnEnum, const UINT32 uiData)
{
    CMutex::Lock(m_pDblListMutex);

    Type* pNext;
    for (Type* pWalk = m_pElems; pWalk; pWalk = pNext)
    {
        pNext = (Type*)pWalk->GetNext();
        if (pfnEnum((void*)pWalk, uiData))
        {
            break;
        }
    }

    CMutex::Unlock(m_pDblListMutex);
}

//
// Class declarations
//

// self-expanding buffer
class CSelfExpandBuffer
{
public:
    CSelfExpandBuffer();
    virtual ~CSelfExpandBuffer();

private:
    //  Prevent assignment: Declared but not implemented.
    CSelfExpandBuffer(const CSelfExpandBuffer& rhs);  // Copy Constructor
    CSelfExpandBuffer& operator=(const CSelfExpandBuffer& rhs);  //  Assignment operator

public:
    BOOL            Append(const char* szIn, UINT32 nLength);
    const char*     Data() const    { return m_szBuffer; };
    UINT32          Size() const    { return m_uiUsed; };
    void            Flush()         { delete[] m_szBuffer; m_szBuffer = NULL; m_uiUsed = 0; };

private:
    static const UINT32 m_nChunkSize = 1024;

protected:
    char*   m_szBuffer;
    UINT32  m_uiUsed;
    UINT32  m_nCapacity;
};

BOOL convertGsmToUtf8HexString(BYTE* pAlphaBuffer, int offset, const int length,
        char* pszUtf8HexString, const int maxUtf8HexStringLength);

// convert an Integer into a byte array in Big Endian format
void convertIntToByteArray(unsigned char* outByteArray, int value);

// convert an Integer into a byte array in Big Endian format starting at 'position'
void convertIntToByteArrayAt(unsigned char* outByteArray, int value, int position);

// Utility function to translate Hexadecimal array (characters) onto byte array (numeric values)
BOOL extractByteArrayFromString(const char* szHexArray, const UINT32 uiLength, UINT8* szByteArray);

// Utility function to translate byte array (numeric values) onto Hexadecimal array (characters)
BOOL convertByteArrayIntoString(const UINT8* szByteArray, const UINT32 uiLength, char* pszHexArray);

