/* AudioATManager.h
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

#include <pthread.h>
#include <semaphore.h>
#include <list>

#include "ATManager.h"
#include "AudioATModemTypes.h"

class CProgressUnsollicitedATCommand;
class CCallStatUnsollicitedATCommand;
class CXDRVIUnsollicitedATCommand;

using namespace std;

class CAudioATManager : public CATManager {

public:
    CAudioATManager(IEventNotifier *observer);
    ~CAudioATManager();

    // Start
    AT_STATUS start();

    // Stop
    void stop();

    // Set the tty name to use
    // MUST BE CALLED BEFORE STARTING THE MANAGER
    void setTtyName(const string& pcModemTty);

    // Get modem audio status
    bool isModemAudioAvailable() const;

    // Get Modem PCM Codec
    MODEM_CODEC getModemCodec() const;

private:
    CProgressUnsollicitedATCommand* _pXProgressCmd;
    CCallStatUnsollicitedATCommand* _pXCallstatCmd;
    CXDRVIUnsollicitedATCommand*    _pXDRVICmd;

    // Modem Call state
    bool _bModemCallActive;

    // TTY used by this instance
    string _strTtyName;
};

