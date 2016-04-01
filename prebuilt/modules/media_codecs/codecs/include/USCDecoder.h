/*
Portions Copyright (c) 2011 Intel Corporation.
*/

/*
* Copyright (C) 2009 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>

#include "mc_version.h"
#include "eppdefs.h"
#include "usc.h"
#include "rtp_amr_payload.h"
#include "amr_common.h"
#include <media/stagefright/MediaSource.h>

#include "CIPAMRCommon.h"

#ifdef AUDIO_DUMP_ENABLE
#include "AudioDumpUtils.h"
#include <cutils/properties.h>
#endif

namespace android {

struct MediaBufferGroup;

template<size_t uInBufferSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, int32_t kSampleRate, AMRCodecType codecType>
class  USCDecoder : public MediaSource {
public:
    USCDecoder(const sp<MediaSource> &source);
#ifdef AUDIO_DUMP_ENABLE
    AudioDump *mParserAudioDump;
#endif

protected:
    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
        MediaBuffer **buffer, const ReadOptions *options);

    virtual ~USCDecoder();

private:
    sp<MediaSource> mSource;
    bool mStarted;

    MediaBufferGroup *mBufferGroup;

    int64_t mAnchorTimeUs;
    int64_t mNumSamplesOutput;

    MediaBuffer *mInputBuffer;
    unsigned char mInputSampleBuffer[uInBufferSize];
    int numSkipedFrames;
    int maxNumSkipedFrames;

    USC_Fxns &mrUSCAMRFxns;
    USC_MemBank *mBanksDec;
    int mNumBanksDec;
    USC_Handle mUSCDecoder;
    USC_CodecInfo mInfo;
    Epp32s mPrevBitrate;

    USCDecoder(const USCDecoder &);
    USCDecoder &operator=(const USCDecoder &);
};


template<size_t uInBufferSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, int32_t kSampleRate, AMRCodecType codecType>
USCDecoder<uInBufferSize, kNumSamplesPerFrame, kSampleRate, codecType>
::USCDecoder(const sp<MediaSource> &source)
    : mSource(source),
      mStarted(false),
      mBufferGroup(NULL),
      mAnchorTimeUs(0),
      mNumSamplesOutput(0),
      mInputBuffer(NULL),
      maxNumSkipedFrames(7),
      numSkipedFrames(7),
#ifdef AUDIO_DUMP_ENABLE
      mParserAudioDump(NULL),
#endif
      mrUSCAMRFxns (GetUSCFunctions<codecType>()) {

    if (NB == codecType) {
        mPrevBitrate =  AMRNBBitrate[0];
    } else {
        mPrevBitrate =  AMRWBBitrate[0];
    }

    LOGI("USC Decoder plugin created, Media Codecs version: %s", MediaCodecs_GetVersion() );
}

template<size_t uInBufferSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, int32_t kSampleRate, AMRCodecType codecType>
USCDecoder<uInBufferSize, kNumSamplesPerFrame, kSampleRate, codecType>
::~USCDecoder() {
    stop();

#ifdef AUDIO_DUMP_ENABLE
    if (mParserAudioDump) {
        delete mParserAudioDump;
        mParserAudioDump = NULL;
    }
#endif
    LOGI("USC Decoder plugin deleted");
}

template<size_t uInBufferSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, int32_t kSampleRate, AMRCodecType codecType>
status_t USCDecoder<uInBufferSize, kNumSamplesPerFrame, kSampleRate, codecType>
::start(MetaData *params) {

    if(mStarted){
        return ERROR_MALFORMED;
    }

    mBufferGroup = new MediaBufferGroup;
    MediaBuffer *pOutBuf = new MediaBuffer(kNumSamplesPerFrame * sizeof(int16_t));
    if (NULL == mBufferGroup || NULL == pOutBuf) {
        delete mBufferGroup;
        mBufferGroup = NULL;

        LOGE("Failed to allocate memory");
        return NO_MEMORY;
    }
    mBufferGroup->add_buffer(pOutBuf);

    if (mrUSCAMRFxns.std.GetInfo((USC_Handle)NULL, &mInfo) != USC_NoError) {
        LOGE("USC GetInfo failed");
        return INVALID_OPERATION;
    }

    mInfo.params.direction = USC_DECODE;

    mInfo.params.law = 0;// 0 - pcm

    status_t status = initResources(mrUSCAMRFxns, mInfo, mBanksDec, mNumBanksDec, mUSCDecoder);
    if (OK != status)
        return status;

    mSource->start();

    mAnchorTimeUs = 0;
    mNumSamplesOutput = 0;
    mStarted = true;

    return OK;
}

template<size_t uInBufferSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, int32_t kSampleRate, AMRCodecType codecType>
status_t USCDecoder<uInBufferSize, kNumSamplesPerFrame, kSampleRate, codecType>
::stop() {
    SafeRelease(mInputBuffer);

    freeResources(mBanksDec, mNumBanksDec);

    SafeDelete(mBufferGroup);

    if(mStarted){
        mSource->stop();
        mStarted = false;
    }

    return OK;
}

template<size_t uInBufferSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, int32_t kSampleRate, AMRCodecType codecType>
sp<MetaData> USCDecoder<uInBufferSize, kNumSamplesPerFrame, kSampleRate, codecType>
::getFormat() {
    sp<MetaData> meta;

    if (NULL != mSource.get()){
        sp<MetaData> srcFormat = mSource->getFormat();
        int32_t numChannels;
        int32_t sampleRate;
        if(NULL != srcFormat.get()
            && srcFormat->findInt32(kKeyChannelCount, &numChannels)
            && (1 == numChannels)
            && srcFormat->findInt32(kKeySampleRate, &sampleRate)
            && (kSampleRate == sampleRate)){

            meta = new MetaData;
            if(NULL != meta.get()){
                meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
                meta->setInt32(kKeyChannelCount, numChannels);
                meta->setInt32(kKeySampleRate, sampleRate);

                int64_t durationUs;
                if (srcFormat->findInt64(kKeyDuration, &durationUs)) {
                    meta->setInt64(kKeyDuration, durationUs);
                }

                meta->setCString(kKeyDecoderComponent, "LOG_TAG");
            }
        }
    }
    return meta;
}

template<size_t uInBufferSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, int32_t kSampleRate, AMRCodecType codecType>
status_t USCDecoder<uInBufferSize, kNumSamplesPerFrame, kSampleRate, codecType>
::read(MediaBuffer **out, const ReadOptions *options) {
    status_t err;

    if (NULL == out){
        return BAD_VALUE;
    }

    *out = NULL;
    int64_t seekTimeUs;
    ReadOptions::SeekMode seekMode;
    USC_Bitstream in;
    USC_PCMStream usc_out;
    Epp32s *pOrderMapTbl;
    Epp32s frameLenBits;
    Epp32s bitrate, frameType;
    Epp32s STI, usc_mode;
    Epp32s FrameLength;



    if (options && options->getSeekTo(&seekTimeUs, &seekMode)) {
        if(seekTimeUs < 0){
            return BAD_VALUE;
        }
        LOGV("Seek bitrate %d %d %d", mInfo.params.modes.bitrate, mInfo.params.modes.truncate,mInfo.params.modes.vad);
        mNumSamplesOutput = 0;
        numSkipedFrames = 0;
        if (mInputBuffer) {
            LOGV("Seek: mInputBuffer->release();");
            mInputBuffer->release();
            mInputBuffer = NULL;
        }
        USC_Modes modes;

        modes.bitrate = mInfo.params.modes.bitrate;
        modes.truncate = mInfo.params.modes.truncate;
        modes.vad = mInfo.params.modes.vad;
        modes.hpf = 0;
        modes.pf = 0;
        modes.outMode = USC_OUT_NO_CONTROL;

        LOGV("-Seek bitrate %d %d %d", modes.bitrate, modes.truncate, modes.vad);

        //mrUSCAMRFxns.std.Reinit(&modes, mUSCDecoder);
        //mrUSCAMRFxns.std.Init(&modes, mUSCDecoder);
    } else {
        seekTimeUs = -1;
    }

    if (mInputBuffer == NULL) {
        err = mSource->read(&mInputBuffer, options);

        if (err != OK) {
            return err;
        }

        int64_t timeUs;
        if (mInputBuffer->meta_data()->findInt64(kKeyTime, &timeUs)) {
            mAnchorTimeUs = timeUs;
            mNumSamplesOutput = 0;
        } else if (seekTimeUs >= 0){
            // We must have a new timestamp after seeking.
            return ERROR_MALFORMED;
        }
    }

    uint8_t *inputPtr =
        ( uint8_t *)mInputBuffer->data() + mInputBuffer->range_offset();
    Frame_Type_3GPP frame_mode =  GetFrameTypeLength(codecType, inputPtr, &FrameLength);
    FrameLength--;
    inputPtr++;
    LOGV("FrameLength =%d,frame_mode =%d,range_length =%u",FrameLength,frame_mode,mInputBuffer->range_length());

    bitrate = GetBitRate(codecType, frame_mode);
    if (bitrate == 0) {
        LOGE("Invalid bitrate, mode: %d", frame_mode);
        return ERROR_MALFORMED;
    }

    frameType = GetUSCFrameType(codecType, frame_mode);
    if (frameType == 0) {
        GetBitReordersTable(codecType, frame_mode, &pOrderMapTbl, &frameLenBits);
        RTP2USCActiveFrame(&inputPtr, mInputSampleBuffer, FrameLength, pOrderMapTbl, frameLenBits);
        in.pBuffer = (char *)mInputSampleBuffer;
    } else if (frameType == 1) {
        RTP2USCSIDFrame(&inputPtr, mInputSampleBuffer, FrameLength, &STI, &usc_mode, codecType);
        if (STI == 1) {
            frameType = 2;
        }
        bitrate = GetBitRate(codecType, usc_mode);
        in.pBuffer = (char *)mInputSampleBuffer;
    } else if (frameType == 3) {
        bitrate = mPrevBitrate;
        in.pBuffer = (char *)inputPtr;
    } else {
        LOGE("frame type = %d, invalid.", frameType);
        return ERROR_MALFORMED;
    }

    in.frametype = frameType;
    in.bitrate = bitrate;
    in.nbytes = FrameLength;

    MediaBuffer *buffer;
    if(OK != mBufferGroup->acquire_buffer(&buffer)){
        return ERROR_MALFORMED;
    }

    usc_out.pBuffer = (char *)(buffer->data());
    usc_out.pcmType.bitPerSample = 16;

#ifdef AUDIO_DUMP_ENABLE
    char value[PROPERTY_VALUE_MAX];
    if (property_get("audio.media_pb.parser.dump", value, "disable")) {
        if (!strcmp(value, "enable") && mParserAudioDump) {
            mParserAudioDump->dumpData((uint8_t*)mInputBuffer->data(), mInputBuffer->range_offset(), mInputBuffer->range_length());
        } else if (!strcmp(value, "enable") && !mParserAudioDump) {
            mParserAudioDump = new AudioDump(AudioDump::AUDIO_PARSER);
            if (mParserAudioDump) {
                mParserAudioDump->dumpData((uint8_t*)mInputBuffer->data(), mInputBuffer->range_offset(), mInputBuffer->range_length());
            }
        }
    }
#endif
    USC_Status decStatus;
    {
        AUTO_TIMER(LOG_TAG);
        decStatus = mrUSCAMRFxns.Decode(mUSCDecoder, &in, &usc_out);
    }

    LOGV("Decode status: %d usc_out.nbytes %d", decStatus, usc_out.nbytes);
    if (decStatus != USC_NoError) {
        LOGE("Decode Failed Status: %d", decStatus);
        buffer->release();
        buffer = NULL;
        return ERROR_MALFORMED;
    }

    if (numSkipedFrames < maxNumSkipedFrames)
    {
      memset(usc_out.pBuffer, 0, usc_out.nbytes);
      numSkipedFrames ++;
    }

    mPrevBitrate = bitrate;

    buffer->set_range(0, kNumSamplesPerFrame * sizeof(int16_t));

    mInputBuffer->set_range(
        mInputBuffer->range_offset() + FrameLength + 1,
        mInputBuffer->range_length() - FrameLength - 1);

    if (mInputBuffer->range_length() == 0) {
        mInputBuffer->release();
        mInputBuffer = NULL;
    }

    buffer->meta_data()->setInt64(
        kKeyTime,
        mAnchorTimeUs
        + (mNumSamplesOutput * US_PER_SECOND) / kSampleRate);

    mNumSamplesOutput += kNumSamplesPerFrame;

    *out = buffer;

    return OK;
}

}  // namespace android
