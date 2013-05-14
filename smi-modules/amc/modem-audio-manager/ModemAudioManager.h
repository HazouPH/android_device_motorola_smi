/* ModemAudioManager.h
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

#include "AudioATManager.h"
#include "AudioATModemTypes.h"
#include "ModemStatusNotifier.h"

class CAudioATManager;


using namespace std;

class CModemAudioManager : public IEventNotifier {


public:
    CModemAudioManager(IModemStatusNotifier* observer);

    ~CModemAudioManager();

    // Start
    virtual AT_STATUS start();

    // Stop
    virtual void stop();

    // Get modem audio status
    virtual bool isModemAudioAvailable() const;

    // Get modem audio PCM codec
    virtual MODEM_CODEC getModemCodec() const;

    // Get modem status
    bool isModemAlive() const;

    // Get the modem status (from STMD definition)
    int getModemStatus() const;

    // Send - Asynchronous or synchronous function
    // When used asynchronously, take care pATCommand shall not be destroyed
    // until answer is received
    AT_STATUS sendCommand(CATCommand* pATCommand, bool bSynchronous);

private:
    // Inherited from IEventNotifier: Event processing
    virtual void onEvent(uint32_t uiEventId);

    virtual void onModemStateChanged();

protected:
    // Primary SIM Audio AT Manager instance
    CAudioATManager* _pPrimaryAudioATManager;

private:
    IModemStatusNotifier* _pObserver;

    // Primary Channel name property
    static const char* const _pcPrimaryChannelNameProperty;
};

