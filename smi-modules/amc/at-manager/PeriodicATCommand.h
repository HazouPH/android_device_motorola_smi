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
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>
#include <string>

#include "ATCommand.h"

using namespace std;

class CPeriodicATCommand : public CATCommand
{

public:
    CPeriodicATCommand(const string& strCommand, const string& strRespPrefix = "");

    // Inherited from CATCommand
    virtual void setAnswerOK(bool bIsOK);

private:
    // Period time in seconds
    uint32_t _uiDelaySeconds;

    // Send Deadline
    struct timespec _tsSendDeadline;
};

