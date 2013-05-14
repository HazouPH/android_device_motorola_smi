/* EventThread.h
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
#pragma once

#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <list>

using namespace std;

class IEventListener;

class CEventThread
{
    enum PipeMsg {
        EProcess,
        EExit,

        ENbPipeMsg
    };

    struct SFd {
        SFd(uint32_t uiClientFdId, int iFd, bool bToListenTo) : _uiClientFdId(uiClientFdId), _iFd(iFd), _bToListenTo(bToListenTo) {}

        uint32_t _uiClientFdId;
        int _iFd;
        bool _bToListenTo;
    };

    typedef list<SFd>::iterator SFdListIterator;
    typedef list<SFd>::const_iterator SFdListConstIterator;

public:
    // Construction
    CEventThread(IEventListener* pEventListener);
    ~CEventThread();

    // Add open FDs
    void addOpenedFd(uint32_t uiFdClientId, int iFd, bool bToListenTo = false);

    // Get FD
    int getFd(uint32_t uiClientFdId) const;

    // Remove and close all FDs
    void closeAndRemoveFds();

    // Remove and close FD from index
    void closeAndRemoveFd(uint32_t uiClientFdId);

    // Timeout (must be called from within thread when started or anytime when not started)
    void setTimeoutMs(uint32_t uiTimeoutMs);
    uint32_t getTimeoutMs() const;

    // Thread start
    bool start();
    // Thread stop
    void stop();
    // trigger
    void trig();

    // Context check
    bool inThreadContext() const;

private:
    // Thread
    static void* thread_func(void* pData);
    // Run
    void run();
    // Add FD
    void addListenedFd(int iFd);
    // Remove FD
    void removeListenedFd(int iFd);
    // Poll FD computation
    void buildPollFds(struct pollfd* paPollFds) const;

    // Listener
    IEventListener* _pEventListener;
    // State
    bool _bIsStarted;
    // Thread id
    pthread_t _ulThreadId;
    // Inband pipe
    int _aiInbandPipe[2];
    // FD list
    list<SFd> _sFdList;
    // Polled FD count
    uint32_t _uiNbPollFds;
    // Wait timeout
    uint32_t _uiTimeoutMs;
    // Thread context
    bool _bThreadContext;
};

