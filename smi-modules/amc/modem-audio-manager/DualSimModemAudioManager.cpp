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

#define LOG_TAG "DUAL_SIM_MODEM_MANAGER"

#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <utils/Log.h>

#include "Property.h"
#include "DualSimModemAudioManager.h"

#define base    CModemAudioManager

const char* const CDualSimModemAudioManager::_pcAtXSIMSEL = "AT+XSIMSEL=";

const char* const CDualSimModemAudioManager::_pcSecondaryChannelNameProperty = "audiocomms.atm.secondaryChannel";

CDualSimModemAudioManager::CDualSimModemAudioManager(IModemStatusNotifier *observer) :
    base(observer), _pSecondaryAudioATManager(new CAudioATManager(this))
{
    LOGD("%s", __FUNCTION__);

    // Read primary AT Manager TTY name property
    TProperty<string> propStrTty(_pcSecondaryChannelNameProperty);

    _pSecondaryAudioATManager->setTtyName(propStrTty);

    // Tricks: use unsollicted to add the SIMSEL registration
    // It will be automatically sent if the modem is resetted
    // SIMSEL must be sent before starting the AudioATManager
    // hence before registering to modem audio messages

    // Primary ATManager: listening to SIM1
    _pPrimaryAudioATManager->addUnsollicitedATCommand(new CUnsollicitedATCommand(string(_pcAtXSIMSEL) + "0", "", ""));

    // Secondary ATManager: listening to SIM2
    _pSecondaryAudioATManager->addUnsollicitedATCommand(new CUnsollicitedATCommand(string(_pcAtXSIMSEL) + "1", "", ""));

}

CDualSimModemAudioManager::~CDualSimModemAudioManager()
{
    // AT parser
    delete _pSecondaryAudioATManager;
}


AT_STATUS CDualSimModemAudioManager::start()
{
    LOGD("%s", __FUNCTION__);
    AT_STATUS ret;

    ret = base::start();

    if (ret == AT_OK) {

        ret = _pSecondaryAudioATManager->start();
    }

    return ret;
}

// Stop
void CDualSimModemAudioManager::stop()
{
    LOGD("%s", __FUNCTION__);

    // Stop Audio AT Manager
    _pSecondaryAudioATManager->stop();

    base::stop();
}

bool CDualSimModemAudioManager::isModemAudioAvailable() const
{
    LOGD("%s", __FUNCTION__);

    // Check if Modem Audio Path is available
    // According to network, some can receive XCALLSTAT, some can receive XPROGRESS
    // so compute both information
    return _pSecondaryAudioATManager->isModemAudioAvailable() || base::isModemAudioAvailable();
}

