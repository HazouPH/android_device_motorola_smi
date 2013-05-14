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
#include "ATCommand.h"
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <utils/Log.h>

CATCommand::CATCommand(const string& strCommand, const string& strRespPrefix) :
    _strCommand(strCommand),
    _strRespPrefix(strRespPrefix),
    _eStatus(StatusUnset)
{
}

CATCommand::~CATCommand()
{

}

// Clear
void CATCommand::clearStatus()
{
    LOGD("%s", __func__);

    // Answer status
    _eStatus = StatusUnset;

    // Answer
    _strAnswer.clear();
}

// Command
const string& CATCommand::getCommand() const
{
    return _strCommand;
}

// Answer fragment
void CATCommand::addAnswerFragment(const string& strAnswerFragment)
{
    _strAnswer += strAnswerFragment + '\n';
}

// Answer
const string& CATCommand::getAnswer() const
{
    return _strAnswer;
}

// Set Status
void CATCommand::setAnswerOK(bool bIsOK)
{
    _eStatus = bIsOK ? StatusOK : StatusError;
}

// Process response
void CATCommand::doProcessAnswer()
{
}

// Get Status
bool CATCommand::isAnswerOK() const
{
    return _eStatus == StatusOK;
}

// Get reception completion
bool CATCommand::isComplete() const
{
    return _eStatus != StatusUnset;
}

// Prefix
const string& CATCommand::getPrefix() const
{
    return _strRespPrefix;
}

// Has prefix
bool CATCommand::hasPrefix() const
{
    return !_strRespPrefix.empty();
}
