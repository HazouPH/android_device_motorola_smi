/* AudioATModemTypes.h
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


typedef enum  {
    CODEC_TYPE_FULL_RATE_SPEECH = 0,
    CODEC_TYPE_ENHANCED_FULL_RATE_SPEECH,
    CODEC_TYPE_HALF_RATE_SPEECH,
    CODEC_TYPE_NB_AMR_SPEECH,
    CODEC_TYPE_WB_AMR_SPEECH,
    CODEC_TYPE_INVALID
} MODEM_CODEC;

typedef enum {
    SAMPLE_RATE_8000Hz = 0,
    SAMPLE_RATE_11025Hz,
    SAMPLE_RATE_12000Hz,
    SAMPLE_RATE_16000Hz,
    SAMPLE_RATE_22050Hz,
    SAMPLE_RATE_24000Hz,
    SAMPLE_RATE_32000Hz,
    SAMPLE_RATE_44100Hz,
    SAMPLE_RATE_48000Hz,
    SAMPLE_RATE_INVALID
} MODEM_SAMPLE_RATE;

