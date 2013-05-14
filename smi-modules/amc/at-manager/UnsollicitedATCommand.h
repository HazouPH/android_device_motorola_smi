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

class CUnsollicitedATCommand : public CATCommand
{

public:
    CUnsollicitedATCommand(const string& strCommand, const string& strRespPrefix, const string& strNotifPrefix, uint32_t eventId = 0);

    // Inherited from CATCommand
    virtual void addAnswerFragment(const string& strAnswerFragment);

    // Process notification
    virtual void doProcessNotification();

    // Notification prefix
    const string& getNotificationPrefix() const;

    // Has notification prefix
    bool hasNotificationPrefix() const;

    // Event ID
    uint32_t getEventId() const;

private:
    uint32_t _uiEventId;
    // Expected notification prefix
    string _strNotifPrefix;
};

