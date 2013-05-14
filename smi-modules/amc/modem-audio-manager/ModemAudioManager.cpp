/*
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 ** http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "MODEM_AUDIO_MANAGER"

#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <utils/Log.h>

#include "Property.h"
#include "ModemAudioEvent.h"

#include "ModemAudioManager.h"


const char* const CModemAudioManager::_pcPrimaryChannelNameProperty = "audiocomms.atm.primaryChannel";

CModemAudioManager::CModemAudioManager(IModemStatusNotifier *observer) :
    _pPrimaryAudioATManager(new CAudioATManager(this)),
    _pObserver(observer)
{
    // Read primary AT Manager TTY name property
    TProperty<string> propStrTty(_pcPrimaryChannelNameProperty);

    _pPrimaryAudioATManager->setTtyName(propStrTty);
}

CModemAudioManager::~CModemAudioManager()
{
    // Stop
    stop();

    // AT parser
    delete _pPrimaryAudioATManager;
}


AT_STATUS CModemAudioManager::start()
{
    LOGD("%s", __FUNCTION__);

    return _pPrimaryAudioATManager->start();
}

void CModemAudioManager::onEvent(uint32_t uiEventId)
{

    LOGD("%s: eventId=%d", __FUNCTION__, uiEventId);

    if (_pObserver != NULL) {
        if (uiEventId == EModemAudioAvailabilibty) {

            _pObserver->onModemAudioStatusChanged();
        }
        else if (uiEventId == EModemAudioPCMChanged) {

            _pObserver->onModemAudioPCMChanged();
        }
        else {

            LOGW("%s: unhandled event %d", __FUNCTION__, uiEventId);
        }
    } else{

        LOGW("%s: No listener for event %d", __FUNCTION__, uiEventId);
    }
}

void CModemAudioManager::onModemStateChanged()
{
    LOGD("%s", __FUNCTION__);

    if (_pObserver) {

        _pObserver->onModemStateChanged();
    }
}

// Stop
void CModemAudioManager::stop()
{
    LOGD("%s", __FUNCTION__);

    // Stop Audio AT Manager
    _pPrimaryAudioATManager->stop();
}

bool CModemAudioManager::isModemAlive() const
{
    LOGD("%s", __FUNCTION__);

    return _pPrimaryAudioATManager->isModemAlive();
}

bool CModemAudioManager::isModemAudioAvailable() const
{
    LOGD("%s", __FUNCTION__);

    // Check if Modem Audio Path is available
    // According to network, some can receive XCALLSTAT, some can receive XPROGRESS
    // so compute both information
    return _pPrimaryAudioATManager->isModemAudioAvailable();
}

MODEM_CODEC CModemAudioManager::getModemCodec() const
{
    LOGD("%s", __FUNCTION__);

    return _pPrimaryAudioATManager->getModemCodec();
}

int CModemAudioManager::getModemStatus() const
{
    LOGD("%s", __FUNCTION__);

    return _pPrimaryAudioATManager->getModemStatus();
}

AT_STATUS CModemAudioManager::sendCommand(CATCommand* pATCommand, bool bSynchronous)
{
    LOGD("%s", __FUNCTION__);

    // All AT cmd send request will be sent on Primary AT Manager
    return _pPrimaryAudioATManager->sendCommand(pATCommand, bSynchronous);
}



