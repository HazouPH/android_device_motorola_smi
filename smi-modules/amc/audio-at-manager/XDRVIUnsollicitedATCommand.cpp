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
#define LOG_TAG "ATMANAGER_XDRVI"
#include "XDRVIUnsollicitedATCommand.h"
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <utils/Log.h>
#include "ModemAudioEvent.h"
#include "Tokenizer.h"

#define AT_XDRV "AT+XDRV=10,8,1"
#define AT_XDRVI_PREFIX "+XDRV:"
#define AT_XDRVI_NOTIFICATION_PREFIX "+XDRVI:"

// Number of argument in the XDRVI notification AT command
#define XDRVI_NOTIFICATION_ARGC              6
// CODEC argument index in the XDRVI notification AT command
#define XDRVI_NOTIFICATION_CODEC_ARGI        3
// Sample rate argument index in the XDRVI notification AT command
#define XDRVI_NOTIFICATION_SAMPLE_RATE_ARGI  4


#define base CUnsollicitedATCommand

CXDRVIUnsollicitedATCommand::CXDRVIUnsollicitedATCommand()
    : base(AT_XDRV, AT_XDRVI_PREFIX, AT_XDRVI_NOTIFICATION_PREFIX, EModemAudioPCMChanged), _uiSpeechCodec(CODEC_TYPE_INVALID), _uiSampleRate(SAMPLE_RATE_INVALID)
{
    LOGD("%s", __FUNCTION__);
}

// Inherited from CUnsollicitedATCommand
//
// Answer is formated:
// +XDRVI:10,8,<xdrv_result>, <audio_codec>, <audio_sample_rate>, <call_type>
// Type of speech codec used during call
//
void CXDRVIUnsollicitedATCommand::doProcessNotification()
{
    LOGD("%s", __FUNCTION__);

    string _strCodecNames[] = {"FULL_RATE_SPEECH", "ENHANCED_FULL_RATE_SPEECH",
			       "HALF_RATE_SPEECH", "NB_AMR_SPEECH", "WB_AMR_SPEECH", "INVALID"};

    string _strSampleRate[] = {"8kHz", "11.025kHz", "12kHz", "16kHz", "22.05kHz",
			        "24kHz","32kHz", "44.1kHz", "48kHz", "INVALID"};
    string str = getAnswer();

    LOGD("%s: ans=(%s) %d", __FUNCTION__, str.c_str(), str.find(getPrefix()));

    // Assert the answer has the XDRVI prefix...
    assert((str.find(getNotificationPrefix()) != string::npos));

    // Remove the prefix from the answer
    string strwoPrefix = str.substr(str.find(getNotificationPrefix()) + getNotificationPrefix().size());

    // Extract the xdrvi params using "," token
    Tokenizer tokenizer(strwoPrefix, ",");
    vector<string> astrItems = tokenizer.split();

    // Test if answer format is OK
    if (astrItems.size() != XDRVI_NOTIFICATION_ARGC)
    {
         LOGE("%s wrong answer format...", __FUNCTION__);
         return ;
    }

    _uiSpeechCodec = (MODEM_CODEC) strtoul(astrItems[XDRVI_NOTIFICATION_CODEC_ARGI].c_str(), NULL, 0);
    if (_uiSpeechCodec > CODEC_TYPE_INVALID)
        _uiSpeechCodec = CODEC_TYPE_INVALID;
    _uiSampleRate = (MODEM_SAMPLE_RATE) strtoul(astrItems[XDRVI_NOTIFICATION_SAMPLE_RATE_ARGI].c_str(), NULL, 0);
    if (_uiSampleRate > SAMPLE_RATE_INVALID)
        _uiSampleRate = SAMPLE_RATE_INVALID;

    LOGD("%s: Speech codec =%d (%s); Sample rate=%d (%s)", __FUNCTION__, _uiSpeechCodec, _strCodecNames[_uiSpeechCodec].c_str(), _uiSampleRate, _strSampleRate[_uiSampleRate].c_str());

    // Clear the answer and the status
    clearStatus();
}

MODEM_CODEC CXDRVIUnsollicitedATCommand::getCodec(void) const
{
   return _uiSpeechCodec;
}

MODEM_SAMPLE_RATE CXDRVIUnsollicitedATCommand::getSampleRate(void) const
{
   return _uiSampleRate;
}

