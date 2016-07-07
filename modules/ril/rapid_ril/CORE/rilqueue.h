////////////////////////////////////////////////////////////////////////////
// rilqueue.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    A simple priority queue
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __RIL_QUEUE__
#define __RIL_QUEUE__

#include "rillog.h"
#include "sync_ops.h"

template <class Object>
class CRilQueue
{
  public:
    CRilQueue(BOOL bDestroy = false);
    ~CRilQueue( );

    BOOL IsEmpty( );
    BOOL GetFront(Object& rObj);

    void MakeEmpty( );
    BOOL Dequeue(Object& rObj);
    BOOL Enqueue(const Object& rObj, UINT32 uiPriority = m_uiMinPriority, BOOL bFront = false);

    void GetAllQueuedObjects(Object*& rpObjArray, int& rnNumOfObjects);
    BOOL DequeueByObj(Object& rObj);

  private:
    // disallow copy constructor and assignment operator
    CRilQueue(const CRilQueue& rQueue);
    const CRilQueue& operator= (const CRilQueue& rQueue);

    struct ListNode
    {
        Object      m_Element;
        UINT32      m_uiPriority;
        ListNode*   m_pNext;

        ListNode(const Object& rObject,
                UINT32 uiPriority = m_uiMinPriority,
                ListNode* pNext = NULL)
            : m_Element(rObject), m_uiPriority(uiPriority), m_pNext(pNext) { }
    };

    static const UINT32 m_uiMinPriority = 0;
    ListNode* m_pFront;
    ListNode* m_pBack;
    BOOL      m_bDestroy;
    CMutex    m_cMutex;
};

// Construct the queue.
template <class Object>
CRilQueue<Object>::CRilQueue(BOOL bDestroy /* = false */)
{
    m_pFront = NULL;
    m_pBack = NULL;
    m_bDestroy = bDestroy;
}

// Destructor.
template <class Object>
CRilQueue<Object>::~CRilQueue()
{
    MakeEmpty();
}

// Test if the queue is logically empty.
// Return true if empty, false, otherwise.
template <class Object>
BOOL CRilQueue<Object>::IsEmpty()
{
    CMutex::Lock(&m_cMutex);

    BOOL bIsEmpty = (m_pFront == NULL);

    CMutex::Unlock(&m_cMutex);
    return bIsEmpty;
}

// Make the queue logically empty.
template <class Object>
void CRilQueue<Object>::MakeEmpty()
{
    CMutex::Lock(&m_cMutex);

    while (!IsEmpty())
    {
        Object obj;
        BOOL isOk = Dequeue(obj);
        //RIL_LOG_CRITICAL("CRilQueue::MakeEmpty() - REMOVING OBJECT\r\n");
        if (isOk && m_bDestroy) delete obj;
    }

    CMutex::Unlock(&m_cMutex);
}

// Return the least recently inserted item in the queue
template <class Object>
BOOL CRilQueue<Object>::GetFront(Object& rObj)
{
    BOOL bRetVal = true;

    CMutex::Lock(&m_cMutex);

    if (IsEmpty())
    {
        bRetVal = false;
    }
    else
    {
        rObj = m_pFront->m_Element;
        bRetVal = true;
    }

    CMutex::Unlock(&m_cMutex);

    return bRetVal;
}


// Return and remove the least recently inserted item from
// the queue.
template <class Object>
BOOL CRilQueue<Object>::Dequeue(Object& rObj)
{
    BOOL bRetVal = true;

    CMutex::Lock(&m_cMutex);

    if (false == GetFront(rObj))
    {
        bRetVal = false;
    }
    else
    {
        rObj = m_pFront->m_Element;
        ListNode* pOld = m_pFront;
        if (m_pFront == m_pBack)
        {
            // One item in queue.  Handle that case.
            m_pFront = NULL;
            m_pBack = NULL;
        }
        else
        {
            m_pFront = m_pFront->m_pNext;
        }
        delete pOld;
    }

    CMutex::Unlock(&m_cMutex);

    return bRetVal;
}

// Insert object into the queue.
template <class Object>
BOOL CRilQueue<Object>::Enqueue(const Object& rObj, UINT32 uiPriority,
                                                          BOOL bFront)
{
    BOOL bRetVal = false;
    CMutex::Lock(&m_cMutex);

    if (IsEmpty())
    {
        ListNode* newNode = new ListNode(rObj, uiPriority);
        if (NULL == newNode)
        {
            RIL_LOG_CRITICAL("CRilQueue::Enqueue() - Cannot allocate memory new ListNode"
                    " - empty\r\n");
            goto Error;
        }
        m_pFront = newNode;
        m_pBack = newNode;
    }
    else
    {
        ListNode* node;
        // check for existing object before inserting
        for (node = m_pFront; node != NULL; node = node->m_pNext)
        {
            if (rObj == node->m_Element)
            {
                RIL_LOG_WARNING("CRilQueue::Enqueue() - Existing object found in queue\r\n");
                return true;
            }
        }

        if (bFront)
        {
            ListNode* temp = m_pFront;
            ListNode* newNode = new ListNode(rObj, uiPriority);
            if (NULL == newNode)
            {
                RIL_LOG_CRITICAL("CRilQueue::Enqueue() - Cannot allocate memory new ListNode"
                        " - front\r\n");
                goto Error;
            }
            newNode->m_pNext = temp;
            m_pFront = newNode;
        }
        else
        {
            if (m_uiMinPriority == uiPriority)
            {
                ListNode* newNode = new ListNode(rObj, uiPriority);
                if (NULL == newNode)
                {
                    RIL_LOG_CRITICAL("CRilQueue::Enqueue() - Cannot allocate memory new ListNode"
                            " - min priority\r\n");
                    goto Error;
                }
                m_pBack->m_pNext = newNode;
                m_pBack = newNode;
            }
            else
            {
                ListNode* node;
                ListNode* previous;
                for (previous = node = m_pFront;
                     node != NULL && node->m_uiPriority >= uiPriority;
                     previous = node, node = node->m_pNext);
                if (NULL == node)
                {
                    //  Insert at end of queue.
                    ListNode* newNode = new ListNode(rObj, uiPriority);
                    if (NULL == newNode)
                    {
                        RIL_LOG_CRITICAL("CRilQueue::Enqueue() - Cannot allocate memory new "
                                "ListNode - end\r\n");
                        goto Error;
                    }
                    m_pBack->m_pNext = newNode;
                    m_pBack = newNode;
                }
                else if (m_pFront == node)
                {
                    //  Insert in front of queue.
                    ListNode* newNode = new ListNode(rObj, uiPriority, m_pFront);
                    if (NULL == newNode)
                    {
                        RIL_LOG_CRITICAL("CRilQueue::Enqueue() - Cannot allocate memory new "
                                "ListNode - front\r\n");
                        goto Error;
                    }
                    m_pFront = newNode;
                }
                else
                {
                    //  Insert in middle of queue.
                    ListNode* newNode = new ListNode(rObj, uiPriority, node);
                    if (NULL == newNode)
                    {
                        RIL_LOG_CRITICAL("CRilQueue::Enqueue() - Cannot allocate memory new "
                                "ListNode - middle\r\n");
                        goto Error;
                    }
                    previous->m_pNext = newNode;
                }
            }
        }
    }

    bRetVal = true;
Error:
    CMutex::Unlock(&m_cMutex);
    return bRetVal;
}


//  Return all objects in the queue.
//  rpObjArray = returned array of Objects.
//  rnNumOfObjects = number of pointers in the returned array.
//  Note that the caller must free the allocated array.
template <class Object>
void CRilQueue<Object>::GetAllQueuedObjects(Object*& rpObjArray, int& rnNumOfObjects)
{
    RIL_LOG_VERBOSE("CRilQueue::GetAllQueuedObjects() - ENTER\r\n");

    CMutex::Lock(&m_cMutex);

    ListNode* node  = NULL;
    int nCount = 0;

    //  Start count at zero.
    rnNumOfObjects = 0;
    rpObjArray = NULL;

    if (IsEmpty())
    {
        RIL_LOG_VERBOSE("CRilQueue::GetAllQueuedObjects() - Empty!  EXIT\r\n");
        goto Error;
    }

    //  Iterate through queue to get count.
    for (node = m_pFront; node != NULL; node = node->m_pNext)
    {
        rnNumOfObjects++;
    }

    RIL_LOG_VERBOSE("CRilQueue::GetAllQueuedObjects() - count = [%d]\r\n", rnNumOfObjects);

    //  Allocate pointer array
    rpObjArray = new Object[rnNumOfObjects];
    if (NULL == rpObjArray)
    {
        RIL_LOG_CRITICAL("CRilQueue::GetAllQueuedObjects() - Cannot allocate memory for %d"
                " object pointers\r\n", rnNumOfObjects);
        goto Error;
    }

    //  Iterate through queue, copy Object.
    for (node = m_pFront; node != NULL; node = node->m_pNext)
    {
        rpObjArray[nCount] = node->m_Element;
        nCount++;
    }


Error:
    CMutex::Unlock(&m_cMutex);

    RIL_LOG_VERBOSE("CRilQueue::GetAllQueuedObjects() - EXIT\r\n");
}

//  Remove item with matching object from the queue.
template <class Object>
BOOL CRilQueue<Object>::DequeueByObj(Object& rObj)
{
    BOOL ret = false;
    RIL_LOG_VERBOSE("CRilQueue::DequeueByObj() - ENTER  rObj=[0x%08x]\r\n", (unsigned long)rObj);

    CMutex::Lock(&m_cMutex);

    if (IsEmpty())
    {
        RIL_LOG_VERBOSE("CRilQueue::DequeueByObj() - Empty!  EXIT\r\n");
        goto Done;
    }

    ListNode* node;
    ListNode* previous;
    for (previous = node = m_pFront;
         node != NULL;
         previous = node, node = node->m_pNext)
    {
        if (rObj == node->m_Element)
        {
            ret = true;
            //  Found a match.  Determine where in the queue it is.
            RIL_LOG_VERBOSE("CRilQueue::DequeueByObj() - Found a match\r\n");
            if (m_pFront == node)
            {
                //  In front.  Make sure handle case with 1 item.
                RIL_LOG_VERBOSE("CRilQueue::DequeueByObj() - Found a match in front\r\n");
                if (m_pBack == node)
                {
                    RIL_LOG_VERBOSE("CRilQueue::DequeueByObj() -"
                            " Found a match in front, was one so remove\r\n");
                    m_pFront = NULL;
                    m_pBack = NULL;
                }
                else
                {
                    m_pFront = node->m_pNext;
                }
            }
            else if (m_pBack == node)
            {
                //  Dequeue back node.
                RIL_LOG_VERBOSE("CRilQueue::DequeueByObj() - Found a match in back\r\n");
                m_pBack = previous;
                previous->m_pNext = NULL;
            }
            else
            {
                //  In middle.
                RIL_LOG_VERBOSE("CRilQueue::DequeueByObj() - Found a match in middle\r\n");
                previous->m_pNext = node->m_pNext;
            }

            delete node;
            break;
        }
    }

Done:
    CMutex::Unlock(&m_cMutex);

    RIL_LOG_VERBOSE("CRilQueue::DequeueByObj() - EXIT\r\n");
    return ret;
}


#endif  // __RIL_QUEUE__

