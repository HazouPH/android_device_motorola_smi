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

#define LOG_TAG "AUDIO_AT_MANAGER"


#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "stmd.h"
#include <errno.h>
#include <utils/Log.h>
#include <sys/types.h>

#include "AudioATManager.h"
#include "CallStatUnsollicitedATCommand.h"
#include "ProgressUnsollicitedATCommand.h"
#include "XDRVIUnsollicitedATCommand.h"

#define MAX_WAIT_ACK_SECONDS    2

#define base CATManager

CAudioATManager::CAudioATManager(IEventNotifier *observer) :
    _pXProgressCmd(NULL),
    _pXCallstatCmd(NULL),
    _pXDRVICmd(NULL),
    _bModemCallActive(false),
    _strTtyName()
{
    base::addEventNotifier(observer);
}

CAudioATManager::~CAudioATManager()
{
    // Stop AT Manager
    base::stop();
}

void CAudioATManager::setTtyName(const string& ttyName)
{
    LOGD("%s: working on %s", __FUNCTION__, ttyName.c_str());

    // set TTY used by this instance
    _strTtyName = ttyName;
}

bool CAudioATManager::isModemAudioAvailable() const
{
    // Check if Modem Audio Path is available
    // According to network, some can receive XCALLSTAT, some can receive XPROGRESS
    // so compute both information
    return _pXCallstatCmd->isAudioPathAvailable() || _pXProgressCmd->isAudioPathAvailable();
}


MODEM_CODEC CAudioATManager::getModemCodec() const
{
    return _pXDRVICmd->getCodec();
}

AT_STATUS CAudioATManager::start()
{
    LOGD("on %s: %s", _strTtyName.c_str(), __FUNCTION__);

    // Add XProgress and XCallStat commands to Unsollicited commands list of the ATManager
    // (it will be automatically resent after reset of the modem)
    base::addUnsollicitedATCommand(_pXProgressCmd = new CProgressUnsollicitedATCommand());
    base::addUnsollicitedATCommand(_pXCallstatCmd = new CCallStatUnsollicitedATCommand());
    // Add 'XDRVI PCM change' commands to Unsollicited command list of the ATManager
    base::addUnsollicitedATCommand(_pXDRVICmd = new CXDRVIUnsollicitedATCommand());

    // Check TTY name was set correctly
    if (_strTtyName.empty()) {

        return AT_UNINITIALIZED;
    }

    return base::start(_strTtyName.c_str());
}

// Stop
void CAudioATManager::stop()
{
    LOGD("on %s: %s", _strTtyName.c_str(), __FUNCTION__);
    base::stop();
}
