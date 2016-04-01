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
#include "usc2ietf.h"

#include "CIPAMRCommon.h"


namespace android {

struct MediaBufferGroup;
//NB - 61, NB_NUMSAMPLES_PER_FRAME, NB_SAMPLE_RATE
//template<size_t uInBufferSize, 32, AMR_NB_SID_RTP_FT, NB>
//template<size_t uInBufferSize, 64, AMR_WB_SID_RTP_FT, WD>
//size_t uInBufferSize, int32_t kNumSamplesPerFrame, int32_t kSampleRate

template<size_t uEncodedFrameSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, SAMPLE_RATE kSampleRate, int32_t kSidRtpFT, AMRCodecType codecType>
class USCEncoder : public MediaSource {
public:
    USCEncoder(const sp<MediaSource> &source, const sp<MetaData> &meta);
protected:
    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
        MediaBuffer **buffer, const ReadOptions *options);

protected:
    virtual ~USCEncoder();

private:

    sp<MediaSource> mSource;
    sp<MetaData>    mMeta;
    bool mStarted;

    MediaBufferGroup *mBufferGroup;

    void *mEncState;
    void *mSidState;
    int64_t mAnchorTimeUs;
    int64_t mNumFramesOutput;

    MediaBuffer *mInputBuffer;
    int mMode;

    /* AMRNB uses 8kHz sampling frequency and 160 samples per frame */
//    static const int32_t kNumSamplesPerFrame = NB_NUMSAMPLES_PER_FRAME;
//    static const int32_t kSampleRate = NB_SAMPLE_RATE;

    int16_t mInputFrame[kNumSamplesPerFrame];
    int32_t mNumInputSamples;

    USC_Fxns &mrUSCAMRFxns;
    USC_CodecInfo mInfo;
    USC_MemBank *mBanksEnc;
    int mNumBanksEnc;
    USC_Handle mUSCEncoder;

    char mTempOutputBuffer[uEncodedFrameSize];// uEncodedFrameSize is max encoded frame size for NB encoded frame

    USCEncoder(const USCEncoder &);
    USCEncoder &operator=(const USCEncoder &);
};



template<size_t uEncodedFrameSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, SAMPLE_RATE kSampleRate, int32_t kSidRtpFT, AMRCodecType codecType>
USCEncoder<uEncodedFrameSize, kNumSamplesPerFrame, kSampleRate, kSidRtpFT, codecType>
::USCEncoder(const sp<MediaSource> &source, const sp<MetaData> &meta)
    : mSource(source),
      mMeta(meta),
      mStarted(false),
      mBufferGroup(NULL),
      mEncState(NULL),
      mSidState(NULL),
      mAnchorTimeUs(0),
      mNumFramesOutput(0),
      mInputBuffer(NULL),
      mMode(0),
      mNumInputSamples(0),
      mrUSCAMRFxns(GetUSCFunctions<codecType>()),
      mBanksEnc (NULL),
      mNumBanksEnc(0),
      mUSCEncoder(NULL) {
    LOGI("USC Encoder plugin created, Media Codecs version: %s", MediaCodecs_GetVersion() );
}

template<size_t uEncodedFrameSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, SAMPLE_RATE kSampleRate, int32_t kSidRtpFT, AMRCodecType codecType>
USCEncoder<uEncodedFrameSize, kNumSamplesPerFrame, kSampleRate, kSidRtpFT, codecType>
::~USCEncoder() {
    if (mStarted) {
        stop();
    }

    freeResources(mBanksEnc, mNumBanksEnc);

    delete mBufferGroup;
    mBufferGroup = NULL;

    LOGI("USC Encoder plugin deleted");
}

template<size_t uEncodedFrameSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, SAMPLE_RATE kSampleRate, int32_t kSidRtpFT, AMRCodecType codecType>
status_t USCEncoder<uEncodedFrameSize, kNumSamplesPerFrame, kSampleRate, kSidRtpFT, codecType>
::start(MetaData *params) {
    if (mStarted) {
        LOGW("Call start() when encoder already started");
        return OK;
    }

    if (mrUSCAMRFxns.std.GetInfo((USC_Handle)NULL, &mInfo) != USC_NoError) {
        LOGE("USC GetInfo failed");
        return INVALID_OPERATION;
    }

    mInfo.params.direction = USC_ENCODE;
    mInfo.params.law = 0;
    mInfo.params.modes.vad = 0;// suppress silence compression
    mInfo.params.pcmType.sample_frequency = kSampleRate;
    mInfo.params.pcmType.nChannels = 1;

    if(!mMeta->findInt32(kKeyBitRate, &mInfo.params.modes.bitrate)){
      LOGE("kKeyBitRate not found in MetaData...");
    }

    mMode = PickModeFromBitrate<codecType>(mInfo.params.modes.bitrate);
    if(-1==mMode){
        return ERROR_UNSUPPORTED;
    }

    LOGV("PCM IN bitrate =%d,samplerate=%d,channelcount=%d",mInfo.params.modes.bitrate,mInfo.params.pcmType.sample_frequency,mInfo.params.pcmType.nChannels);
    mInfo.params.pcmType.bitPerSample = 16;


    mBufferGroup = new MediaBufferGroup;
    MediaBuffer *pOutBuf = new MediaBuffer(uEncodedFrameSize);
    if (NULL == mBufferGroup || NULL == pOutBuf) {
        delete mBufferGroup;
        mBufferGroup = NULL;

        LOGE("Failed to allocate memory");
        return NO_MEMORY;
    }
    mBufferGroup->add_buffer(pOutBuf);

    status_t status = initResources(mrUSCAMRFxns, mInfo, mBanksEnc, mNumBanksEnc, mUSCEncoder);
    if (OK != status)
        return status;

    mAnchorTimeUs = 0;
    mNumFramesOutput = 0;
    mNumInputSamples = 0;

    status = mSource->start(params);
    if (OK != status) {
        LOGE("Source failed to start: %d", status);
        return status;
    }

    mStarted = true;

    return OK;
}

template<size_t uEncodedFrameSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, SAMPLE_RATE kSampleRate, int32_t kSidRtpFT, AMRCodecType codecType>
status_t USCEncoder<uEncodedFrameSize, kNumSamplesPerFrame, kSampleRate, kSidRtpFT, codecType>
::stop() {
    if (!mStarted) {
        LOGW("Call stop() when encoder has not started.");
        return OK;
    }

    if (mInputBuffer) {
        mInputBuffer->release();
        mInputBuffer = NULL;
    }

    delete mBufferGroup;
    mBufferGroup = NULL;

    mSource->stop();

    freeResources(mBanksEnc, mNumBanksEnc);


    mEncState = mSidState = NULL;

    mStarted = false;

    return OK;
}

template<size_t uEncodedFrameSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, SAMPLE_RATE kSampleRate, int32_t kSidRtpFT, AMRCodecType codecType>
sp<MetaData> USCEncoder<uEncodedFrameSize, kNumSamplesPerFrame, kSampleRate, kSidRtpFT, codecType>
::getFormat() {
    sp<MetaData> srcFormat = mSource->getFormat();

    mMeta->setCString(kKeyMIMEType, GetAMRMimeType<codecType>());

    int64_t durationUs;
    if (srcFormat->findInt64(kKeyDuration, &durationUs)) {
        mMeta->setInt64(kKeyDuration, durationUs);
    }

    mMeta->setCString(kKeyDecoderComponent, LOG_TAG);

    return mMeta;
}

template<size_t uEncodedFrameSize, SAMPLES_PER_FRAME kNumSamplesPerFrame, SAMPLE_RATE kSampleRate, int32_t kSidRtpFT, AMRCodecType codecType>
status_t USCEncoder<uEncodedFrameSize, kNumSamplesPerFrame, kSampleRate, kSidRtpFT, codecType>
::read(MediaBuffer **out, const ReadOptions *options) {

    if(NULL == out){
        return BAD_VALUE;
    }

    status_t err;
    USC_PCMStream usc_in;
    USC_Bitstream usc_out;
    Epp32s  size;
    bool readFromSource = false;
    int64_t wallClockTimeUs = -1;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    CHECK(options == NULL || !options->getSeekTo(&seekTimeUs, &mode));
    *out = NULL;

    while (mNumInputSamples < kNumSamplesPerFrame) {
        if (mInputBuffer == NULL) {
            err = mSource->read(&mInputBuffer, options);

            if (err != OK) {
                if (mNumInputSamples == 0) {
                    return ERROR_END_OF_STREAM;
                }
                memset(&mInputFrame[mNumInputSamples],
                       0,
                       sizeof(int16_t)
                            * (kNumSamplesPerFrame - mNumInputSamples));
                mNumInputSamples = kNumSamplesPerFrame;
                break;
            }

            size_t align = mInputBuffer->range_length() % sizeof(int16_t);
            CHECK_EQ(align, 0);
            readFromSource = true;

            int64_t timeUs;
            if (mInputBuffer->meta_data()->findInt64(kKeyDriftTime, &timeUs)) {
                wallClockTimeUs = timeUs;
            }
            if (mInputBuffer->meta_data()->findInt64(kKeyAnchorTime, &timeUs)) {
                mAnchorTimeUs = timeUs;
            }
        } else {
            readFromSource = false;
        }

        size_t copy =
            (kNumSamplesPerFrame - mNumInputSamples) * sizeof(int16_t);

        if (copy > mInputBuffer->range_length()) {
            copy = mInputBuffer->range_length();
        }

        memcpy(&mInputFrame[mNumInputSamples],
               (const uint8_t *)mInputBuffer->data()
                    + mInputBuffer->range_offset(),
               copy);

        mNumInputSamples += copy / sizeof(int16_t);

        mInputBuffer->set_range(
                mInputBuffer->range_offset() + copy,
                mInputBuffer->range_length() - copy);

        if (mInputBuffer->range_length() == 0) {
            mInputBuffer->release();
            mInputBuffer = NULL;
        }
    }

    MediaBuffer *buffer;
    CHECK_EQ(mBufferGroup->acquire_buffer(&buffer), (status_t)OK);
    uint8_t *outPtr = (uint8_t *)buffer->data();

    usc_in.bitrate = mInfo.params.modes.bitrate;
    usc_in.nbytes = kNumSamplesPerFrame*sizeof(int16_t);
    usc_in.pBuffer = (char*)mInputFrame;
    usc_in.pcmType.bitPerSample = mInfo.params.pcmType.bitPerSample;
    usc_in.pcmType.nChannels = mInfo.params.pcmType.nChannels;
    usc_in.pcmType.sample_frequency = mInfo.params.pcmType.sample_frequency;

    usc_out.pBuffer = mTempOutputBuffer;


    USC_Status encStatus;
    {
      AUTO_TIMER(LOG_TAG);
      encStatus = mrUSCAMRFxns.Encode(mUSCEncoder, &usc_in, &usc_out);
    }
    if (encStatus != USC_NoError) {
        LOGE("Encode Failed Status: %d", encStatus);
        return ERROR_UNSUPPORTED;
    }
    if (usc_out.frametype == 0) {
            USCToIETF((Epp8u *)usc_out.pBuffer, outPtr, usc_out.nbytes, &size, mMode, codecType);
    } else if (usc_out.frametype == 1) {
            USCToIETF((Epp8u *)usc_out.pBuffer, outPtr, usc_out.nbytes, &size, kSidRtpFT, codecType);
    } else if (usc_out.frametype == 2) {
            USCToIETF((Epp8u *)usc_out.pBuffer, outPtr, usc_out.nbytes, &size, kSidRtpFT, codecType, 1);
    } else {
        USCToIETF((Epp8u *)usc_out.pBuffer, outPtr, 0, &size, AMR_UNTR_RTP_FT, codecType);
    }


    buffer->set_range(0, size);

    // Each frame of 160/320 samples is 20ms long.
    int64_t mediaTimeUs = mNumFramesOutput * 20000LL;
    buffer->meta_data()->setInt64(
            kKeyTime, mAnchorTimeUs + mediaTimeUs);

    if (readFromSource && wallClockTimeUs != -1) {
        buffer->meta_data()->setInt64(kKeyDriftTime,
            mediaTimeUs - wallClockTimeUs);
    }

    ++mNumFramesOutput;

    *out = buffer;

    mNumInputSamples = 0;

    return OK;
}

}  // namespace android
