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
#define LOG_TAG "AUDIO_AT_MANAGER_CALLPROGRESS"

#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <utils/Log.h>
#include "Tokenizer.h"

#include "ModemAudioEvent.h"
#include "ProgressUnsollicitedATCommand.h"

#define AT_XPROGRESS "AT+XPROGRESS=1"
#define AT_XPROGRESS_PREFIX "+XPROGRESS:"
#define AT_XPROGRESS_NOTIFICATION_PREFIX "+XPROGRESS:"

#define base CUnsollicitedATCommand

CProgressUnsollicitedATCommand::CProgressUnsollicitedATCommand()
    : base(AT_XPROGRESS, AT_XPROGRESS_PREFIX, AT_XPROGRESS_NOTIFICATION_PREFIX, EModemAudioAvailabilibty), _bAudioPathAvailable(false)
{
    LOGD("%s", __FUNCTION__);
}

// Indicate if Modem Audio Path is available
bool CProgressUnsollicitedATCommand::isAudioPathAvailable() const
{
    LOGD("%s: avail = %d", __FUNCTION__, _bAudioPathAvailable);

    return _bAudioPathAvailable;
}

// Inherited from CUnsollicitedATCommand
//
// Answer is formated:
// +XPROGRESS: <callId>,<statusId>
// This code may be repeated so that for each call one line
// is displayed (up to 6)
//
void CProgressUnsollicitedATCommand::doProcessNotification()
{
    string strAnswer = getAnswer();

    // Assert the answer has the CallStat prefix...
    assert((strAnswer.find(getNotificationPrefix()) != string::npos));

    // Remove the prefix from the answer
    string strwoPrefix = strAnswer.substr(strAnswer.find(getNotificationPrefix()) + getNotificationPrefix().size());

    // Extract the xcallstat params using "," token
    Tokenizer tokenizer(strwoPrefix, ",");
    vector<string> astrItems = tokenizer.split();

    // Each line should only have 2 parameters...
    if (astrItems.size() != 2)
    {
         LOGD("%s wrong answer format...", __FUNCTION__);
         return ;
    }

    uint32_t uiCallIndex = strtoul(astrItems[0].c_str(), NULL, 0);
    uint32_t uiProgressStatus = strtoul(astrItems[1].c_str(), NULL, 0);

    LOGD("%s: CALL INDEX=(%d) PROGRES STATUS=(%d)", __FUNCTION__, uiCallIndex, uiProgressStatus);

    //
    // MT Call: audio path established on MTAcceptedTCHYetAvailable
    //                                 or TCHAvailableMTYetAccepted
    // MO Call: audio path established on AlertingInBandOrTCHNotYetAvailable
    //                                 or InBandToneAvailable
    //                                 or TCHAvailableInBandToneYetIndicatedAvailable
    //                                 or TCHAvailableInBandYetIndicatedNotAvailable
    //
    // MO / MT: audio path disconnected on LastSpeechCallEndedSpeechCanBeDisabled
    // (Do not have to care about the # of session)
    //
    if (uiProgressStatus == AlertingInBandOrTCHNotYetAvailable
            || uiProgressStatus == InBandToneAvailable
            || uiProgressStatus == MTAcceptedTCHYetAvailable
            || uiProgressStatus == TCHAvailableMTYetAccepted
            || uiProgressStatus == TCHAvailableInBandToneYetIndicatedAvailable
            || uiProgressStatus == TCHAvailableInBandYetIndicatedNotAvailable)
    {
        // If call is alerting (MT), active (MO) or on hold (to keep the path in case
        //  of multisession or call swap
        _bAudioPathAvailable = true;
        LOGD("%s AudioPath available =%d", __FUNCTION__, _bAudioPathAvailable);

    } else if (uiProgressStatus == LastSpeechCallEndedSpeechCanBeDisabled) {

        _bAudioPathAvailable = false;
    }

    // Clear the answer and the status
    clearStatus();
}
