/* DualSimModemAudioManager.h
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

#include "ATCommand.h"
#include <pthread.h>
#include <semaphore.h>
#include <list>

#include "ModemAudioManager.h"

using namespace std;

class CDualSimModemAudioManager : public CModemAudioManager {


public:
    CDualSimModemAudioManager(IModemStatusNotifier *observer);

    ~CDualSimModemAudioManager();

    // Inherited from CATModemManager: Start
    virtual AT_STATUS start();

    // Inherited from CATModemManager: Stop
    virtual void stop();

    // Inherited from CATModemManager: Get modem status and modem audio status
    virtual bool isModemAudioAvailable() const;

private:
    // Secondary SIM Audio AT Manager instance
    CAudioATManager* _pSecondaryAudioATManager;

    static const char* const _pcAtXSIMSEL;

    // Secondary Channel name property
    static const char* const _pcSecondaryChannelNameProperty;
};
