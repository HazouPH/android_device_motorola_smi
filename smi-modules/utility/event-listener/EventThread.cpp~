/* EventThread.cpp
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
#include <unistd.h>
#include <assert.h>
#include <strings.h>
#include <string.h>

#define LOG_TAG "EVENT_THREAD"
#include <utils/Log.h>
#include "EventListener.h"

#include "EventThread.h"

CEventThread::CEventThread(IEventListener* pEventListener) :
    _pEventListener(pEventListener), _bIsStarted(false), _ulThreadId(0), _uiNbPollFds(0), _uiTimeoutMs(-1), _bThreadContext(false)
{
    assert(pEventListener);

    // Create inband pipe
    pipe(_aiInbandPipe);

    // Add to poll fds
    addOpenedFd(-1, _aiInbandPipe[0], true);
}

CEventThread::~CEventThread()
{
    // Make sure we're stopped
    stop();

    // Close inband pipe
    close(_aiInbandPipe[0]);
    close(_aiInbandPipe[1]);
}

// Add open FDs
void CEventThread::addOpenedFd(uint32_t uiFdClientId, int iFd, bool bToListenTo)
{
    assert(!_bIsStarted || inThreadContext());

    _sFdList.push_back(SFd(uiFdClientId, iFd, bToListenTo));

    if (bToListenTo) {

        // Keep track of number of polled Fd
        _uiNbPollFds++;
    }
}

// Remove and close FD
void CEventThread::closeAndRemoveFd(uint32_t uiClientFdId)
{
    assert(!_bIsStarted || inThreadContext());

    SFdListIterator it;

    for (it = _sFdList.begin(); it != _sFdList.end(); ++it) {

        const SFd* pFd = &(*it);

        if (pFd->_uiClientFdId == uiClientFdId) {

            // Close
            close(pFd->_iFd);

            if (pFd->_bToListenTo) {

                // Keep track of number of polled Fd
                _uiNbPollFds--;
            }

            // Remove element
            _sFdList.erase(it);

            // Done
            return;
        }
    }
}

// Get FD
int CEventThread::getFd(uint32_t uiClientFdId) const
{
    SFdListConstIterator it;

    for (it = _sFdList.begin(); it != _sFdList.end(); ++it) {

        const SFd* pFd = &(*it);

        if (pFd->_uiClientFdId == uiClientFdId) {

            return pFd->_iFd;
        }
    }

    LOGD("%s: Could not find File descriptor from List", __func__);

    return -1;
}

// Timeout (must be called from within thread when started or anywhere when not started)
void CEventThread::setTimeoutMs(uint32_t uiTimeoutMs)
{
    _uiTimeoutMs = uiTimeoutMs;
}

uint32_t CEventThread::getTimeoutMs() const
{
    return _uiTimeoutMs;
}

// Start
bool CEventThread::start()
{
    assert(!_bIsStarted);

    // Create thread
    pthread_create(&_ulThreadId, NULL, thread_func, this);

    // State
    _bIsStarted = true;

    return true;
}

// Stop
void CEventThread::stop()
{
    // Check state
    if (!_bIsStarted) {

        return;
    }

    // Cause exiting of the thread
    uint8_t ucData = EExit;
    ::write(_aiInbandPipe[1], &ucData, sizeof(ucData));

    // Join thread
    pthread_join(_ulThreadId, NULL);

    // State
    _bIsStarted = false;
}

// Trigger
void CEventThread::trig()
{
    LOGD("%s: in", __func__);

    assert(_bIsStarted);

    uint8_t ucData = EProcess;
    ::write(_aiInbandPipe[1], &ucData, sizeof(ucData));

    LOGD("%s: out", __func__);
}

// Context check
bool CEventThread::inThreadContext() const
{
    return pthread_self() == _ulThreadId;
}

// Thread
void* CEventThread::thread_func(void* pData)
{
    reinterpret_cast<CEventThread*>(pData)->run();

    return NULL;
}

void CEventThread::run()
{
    while (true) {

        // Rebuild polled FDs
        struct pollfd astPollFds[_uiNbPollFds];

        buildPollFds(astPollFds);

        // Poll
        int iPollRes = poll(astPollFds, _uiNbPollFds, _uiTimeoutMs);

        if (!iPollRes) {

            // Timeout case
            _pEventListener->onTimeout();

            continue;
        }
        if (iPollRes < 0) {

            // I/O error?
            _pEventListener->onPollError();

            continue;
        }

        // Exit request?
        if (astPollFds[0].revents & POLLIN) {

            // Consume request
            uint8_t ucData;
            ::read(_aiInbandPipe[0], &ucData, sizeof(ucData));

            if (ucData == EProcess) {
                _pEventListener->onProcess();

                continue;
            } else {
                assert(ucData == EExit);
                LOGD("%s exit", __func__);
                // Exit
                return ;
            }
        }

        {
            //bool bContinue = false;
            uint32_t uiIndex;

            // Check for read events
            for (uiIndex = 1; uiIndex < _uiNbPollFds; uiIndex++) {

                // Check for errors first
                if (astPollFds[uiIndex].revents & POLLERR) {
                    LOGD("%s POLLERR event on Fd (%d)", __func__, uiIndex);

                    // Process
                    if (_pEventListener->onError(astPollFds[uiIndex].fd)) {

                        // FD list has changed, bail out
                        break;
                    }
                }
                // Check for hang ups
                if (astPollFds[uiIndex].revents & POLLHUP) {
                    LOGD("%s POLLHUP event on Fd (%d)", __func__, uiIndex);

                    if (_pEventListener->onHangup(astPollFds[uiIndex].fd)) {

                        // FD list has changed, bail out
                        break;
                    }
                }
                // Check for read events
                if (astPollFds[uiIndex].revents & POLLIN) {
                    LOGD("%s POLLIN event on Fd (%d)", __func__, uiIndex);
                    // Process
                    if (_pEventListener->onEvent(astPollFds[uiIndex].fd)) {

                        // FD list has changed, bail out
                        break;
                    }
                }
            }
        }
    }
}

// Poll FD computation
void CEventThread::buildPollFds(struct pollfd* paPollFds) const
{
    // Reset memory
    bzero(paPollFds, sizeof(struct pollfd) * _uiNbPollFds);

    // Fill
    uint32_t uiFdIndex = 0;
    SFdListConstIterator it;

    for (it = _sFdList.begin(); it != _sFdList.end(); ++it) {

        const SFd* pFd = &(*it);

        if (pFd->_bToListenTo) {

            paPollFds[uiFdIndex].fd = pFd->_iFd;
            paPollFds[uiFdIndex++].events = POLLIN;
        }
    }
    // Consistency
    assert(uiFdIndex == _uiNbPollFds);
}
