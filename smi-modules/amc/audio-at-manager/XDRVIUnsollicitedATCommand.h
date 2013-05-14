/*
 **
 ** Copyright 2012 Intel Corporation
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
#pragma once
#include "UnsollicitedATCommand.h"
#include "AudioATModemTypes.h"
#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>
#include <string>


using namespace std;

class CXDRVIUnsollicitedATCommand : public CUnsollicitedATCommand
{
public:
    CXDRVIUnsollicitedATCommand();


    // Get codec type
    MODEM_CODEC getCodec(void) const;

    // Get sample rate
    MODEM_SAMPLE_RATE getSampleRate(void) const;

private:
    // Inherited from CUnsollicitedATCommand
    virtual void doProcessNotification();

    MODEM_CODEC       _uiSpeechCodec;
    MODEM_SAMPLE_RATE _uiSampleRate;
};

