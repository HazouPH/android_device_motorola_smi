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
#define LOG_TAG "AT_MANAGER_UNSOLLICITED"
#include "UnsollicitedATCommand.h"
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <utils/Log.h>

#define base CATCommand

CUnsollicitedATCommand::CUnsollicitedATCommand(const string& strCommand, const string& strRespPrefix, const string& strNotifPrefix, uint32_t eventId) :
    base(strCommand, strRespPrefix),
    _uiEventId(eventId),
    _strNotifPrefix(strNotifPrefix)
{
    LOGD("%s", __FUNCTION__);
}

// Clear
void CUnsollicitedATCommand::addAnswerFragment(const string& strAnswerFragment)
{
    base::addAnswerFragment(strAnswerFragment);
}

// Notification prefix
const string& CUnsollicitedATCommand::getNotificationPrefix() const
{
    return _strNotifPrefix;
}

// Has notification prefix
bool CUnsollicitedATCommand::hasNotificationPrefix() const
{
    return !_strNotifPrefix.empty();
}

uint32_t CUnsollicitedATCommand::getEventId() const
{
    return _uiEventId;
}

void CUnsollicitedATCommand::doProcessNotification()
{
}
