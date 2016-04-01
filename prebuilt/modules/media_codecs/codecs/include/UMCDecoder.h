/* ////////////////////////////////////////////////////////////////////////////// */
/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright (c) 2005-2011 Intel Corporation. All Rights Reserved.
//
//
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


#ifndef UMC_DECODER_H_

#define UMC_DECODER_H_

// max no. of consecutive frames with no sync-word to be allowed
#define  MAX_NUM_SYNC_MISS 29

#include "UMCPerfTracing.h"
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/SkipCutBuffer.h>
#include <utils/threads.h>

#include "mc_version.h"
#include "umc_audio_codec.h"

#ifdef AUDIO_DUMP_ENABLE
#include "AudioDumpUtils.h"
#include <cutils/properties.h>
#endif

namespace UMC{
    class AudioData;
    class AudioCodec;
    typedef int Status;
};
typedef UMC::AudioCodec* (&FNCreateDecoder)();

namespace android {

    template<FNCreateDecoder fnFactory>
    struct UMCAudioDecoder : public MediaSource {
        UMCAudioDecoder (const sp<MediaSource> &source);

        virtual status_t start(MetaData *params);
        virtual status_t stop();

        virtual sp<MetaData> getFormat();

        virtual status_t read(MediaBuffer **pBuffer, const ReadOptions *options);

    protected:
        virtual ~UMCAudioDecoder();
        virtual UMC::Status InitDecoder();
        virtual status_t CheckFormatChange(bool &isResetReadFromBeginning);
        virtual int32_t getDecoderDelay();                        // In samples

        sp<MediaSource> mSource;
        sp<MetaData> mMeta;

        sp<SkipCutBuffer> mSkipCutBuffer;
        bool mIsFirstBuffer;

        bool mStarted;
        bool mmultiChannelSupport;
        bool mAllowSyncWordMissing;

        MediaBufferGroup *mBufferGroup;

        int64_t mAnchorTimeUs;
        int32_t mNumDecodedBuffers;
        int32_t mNumSamplesOutput;
        int32_t mNumSamplesLeftInFrame;
        int32_t mSyncWordMissCnt;

        Mutex mLock;
        MediaBuffer *mInputBuffer;
        void init();

        UMC::AudioCodec      *mpAudioUMCDecoder;
        UMC::AudioData        mInData;
        UMC::AudioData        mOutData;
#ifdef AUDIO_DUMP_ENABLE
        AudioDump *mParserAudioDump;
#endif

    private:
        UMCAudioDecoder(const UMCAudioDecoder &);//no copy
        UMCAudioDecoder &operator=(const UMCAudioDecoder &);//no copy
    };

    template<FNCreateDecoder fnFactory>
    UMCAudioDecoder<fnFactory>::UMCAudioDecoder(const sp<MediaSource> &source)
        :mSource(source)
        ,mMeta(new MetaData)
        ,mStarted(false)
        ,mAllowSyncWordMissing(true)
        ,mmultiChannelSupport(false)
        ,mBufferGroup(NULL)
        ,mSkipCutBuffer(NULL)
        ,mIsFirstBuffer(false)
        ,mAnchorTimeUs(0)
        ,mNumDecodedBuffers(0)
        ,mNumSamplesOutput(0)
        ,mNumSamplesLeftInFrame(-1)
        ,mSyncWordMissCnt(0)
        ,mInputBuffer(NULL)
        ,mpAudioUMCDecoder(NULL)
#ifdef AUDIO_DUMP_ENABLE
        ,mParserAudioDump(NULL)
#endif
    {
        if(NULL != mMeta.get()){
            sp<MetaData> srcFormat = mSource->getFormat();
            if(NULL != srcFormat.get()){
                int32_t SampleRate;
                int32_t NumChannels;

                mMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);

                int32_t iMultiChannelSupport = 0;

                if (srcFormat->findInt32(kKeyMultichannelSupport, &iMultiChannelSupport)) {
                    mmultiChannelSupport = (iMultiChannelSupport == 1);
                }

                if (mmultiChannelSupport) {
                    LOGV("Multi-channel support enabled");
                } else {
                    LOGV("Multi-channel support disabled");
                }

                if(srcFormat->findInt32(kKeyChannelCount, &NumChannels)){
                    LOGV("UMC: audio decoder srcFormat mNumChannels %d", NumChannels);
                    mMeta->setInt32(kKeyChannelCount, NumChannels);
                    if(srcFormat->findInt32(kKeySampleRate, &SampleRate)){
                        LOGV("UMC: audio decoder srcFormat SampleRate %d", SampleRate);
                        mMeta->setInt32(kKeySampleRate, SampleRate);

                        int64_t durationUs;
                        if (srcFormat->findInt64(kKeyDuration, &durationUs)) {
                            mMeta->setInt64(kKeyDuration, durationUs);
                        }

                        mMeta->setCString(kKeyDecoderComponent, LOG_TAG);

                        mInData.SetDataSize(0);
                        mOutData.SetDataSize(0);

                        mpAudioUMCDecoder = (fnFactory)();
                    }
                }
            }
        }

        LOGI("UMC Decoder plugin created, Media Codecs version: %s", MediaCodecs_GetVersion() );
    }

    template<FNCreateDecoder fnFactory>
    status_t UMCAudioDecoder<fnFactory>::InitDecoder()
    {
        UMC::AudioCodecParams acParams;
        acParams.m_lpMemoryAllocator=NULL;
        UMC::Status initStatus = mpAudioUMCDecoder->Init(&acParams);
        switch (initStatus) {
            case UMC::UMC_OK:
                return OK;

            case UMC::UMC_ERR_ALLOC:
                LOGE("UMCAudioDecoder::InitDecoder(): UMC_ERR_ALLOC \n");
                return NO_MEMORY;

            case UMC::UMC_ERR_INIT:
                LOGE("UMCAudioDecoder::InitDecoder(): UMC_ERR_INIT \n");
                return NO_INIT;

            default:
                LOGE("UMCAudioDecoder::InitDecoder(): error %d\n", initStatus);
                return NO_INIT;
        }
    }

    template<FNCreateDecoder fnFactory>
    UMCAudioDecoder<fnFactory>::~UMCAudioDecoder() {
        LOGV("~UMCAudioDecoder()  {");
        stop();

        mOutData.Close();
        mInData.Close();
#ifdef AUDIO_DUMP_ENABLE
        if (mParserAudioDump) {
            delete mParserAudioDump;
            mParserAudioDump = NULL;
        }
#endif
        LOGV("delete mpAudioUMCDecoder; {");
        delete mpAudioUMCDecoder;
        mpAudioUMCDecoder = NULL;
        LOGV("~UMCAudioDecoder()  }");
        LOGI("UMC Decoder plugin deleted");
    }

    template<FNCreateDecoder fnFactory>
    status_t UMCAudioDecoder<fnFactory>::start(MetaData *params) {
        Mutex::Autolock autoLock(mLock);
        sp<MetaData> meta = mSource->getFormat();
        int32_t delay = 0;
        int32_t padding = 0;
        int32_t numchannels = 0;

        if (mStarted) {
            LOGE("UMCDecoder::start ...UNKNOWN_ERROR - decoder already started  {%d}", __LINE__);
            return UNKNOWN_ERROR;
        }
        LOGV("UMCDecoder::start %d", __LINE__);
        //checking if constructor initialized correctly
        if (NULL == mpAudioUMCDecoder) {
            LOGE("UMCDecoder::start 'NULL==mMeta && mpAudioUMCDecoder' - NO_MEMORY %d", __LINE__);
            return NO_MEMORY;
        }
        MediaBuffer *pBuffer;
        mBufferGroup = new MediaBufferGroup;
        if (NULL == mBufferGroup) {
            LOGE("UMCDecoder::start 'new MediaBufferGroup' - NO_MEMORY %d", __LINE__);
            return NO_MEMORY;
        }

        UMC::AudioCodecParams acParams;
        status_t result = InitDecoder();
        LOGV("UMCAudioDecoder::InitDecoder() returned '%d' {%d}", result, __LINE__);
        if (OK != result) {
            goto deleteMediaBufferGroup_exit;
        }

        mpAudioUMCDecoder->GetInfo(&acParams);

        if ((pBuffer = new MediaBuffer(acParams.m_SuggestedOutputSize)) == NULL) {
            LOGE("UMCDecoder::start 'new MediaBuffer(%d)' NO_MEMORY {%d}", acParams.m_SuggestedOutputSize,  __LINE__);
            result = NO_MEMORY;
            goto deleteMediaBufferGroup_exit;
        }

        mBufferGroup->add_buffer(pBuffer);
        LOGV("UMCDecoder::start{buffer size%d} %d", acParams.m_SuggestedOutputSize, __LINE__);

        if (!meta->findInt32(kKeyEncoderDelay, &delay)) {
            delay = 0;
        }
        if (!meta->findInt32(kKeyEncoderPadding, &padding)) {
            padding = 0;
        }
        if (delay + padding) {
            if (mMeta->findInt32(kKeyChannelCount, &numchannels)) {
                size_t frameSize = numchannels * sizeof(int16_t);
                // Decoder delay has to be accomodated inside padding+delay itself.
                if (padding > getDecoderDelay()) {
                     padding -= getDecoderDelay();
                }
                LOGV("delay %d frameSize %d padding %d", delay, frameSize, padding);
                mSkipCutBuffer = new SkipCutBuffer(delay * frameSize, padding * frameSize);
            }
        }

        result = mSource->start();
        LOGV("UMCAudioDecoder::start mSource->start returned '%d' {%d}", result, __LINE__);
        if (OK != result) {
            goto releaseMediaBuffer_exit;
        }
        // If the source never limits the number of valid samples contained
        // in the input data, we'll assume that all of the decoded samples are valid.
        mNumSamplesLeftInFrame = -1;

        mAnchorTimeUs = 0;
        mNumDecodedBuffers = 0;
        mStarted = true;
        mIsFirstBuffer = true;

        LOGV("UMCAudioDecoder::start OK...exiting {%d}", __LINE__);
        return OK;

releaseMediaBuffer_exit:
        if (mSkipCutBuffer != NULL) {
            mSkipCutBuffer->clear();
        }
        SafeRelease(pBuffer);

deleteMediaBufferGroup_exit:
        SafeDelete(mBufferGroup);
        return result;

    }

    template<FNCreateDecoder fnFactory>
    status_t UMCAudioDecoder<fnFactory>::stop() {
        Mutex::Autolock autoLock(mLock);

        LOGV("UMCAudioDecoder::stop Locked { {%d}", __LINE__);

        if(mBufferGroup != NULL) {
            MediaBuffer *pBuffer;
            LOGV("UMCAudioDecoder::stop acquire_buffer {");
            mBufferGroup->acquire_buffer(&pBuffer);
            LOGV("UMCAudioDecoder::stop acquire_buffer } buffer->refcount()  %d",pBuffer->refcount() );
            SafeRelease(pBuffer);
            LOGV("UMCAudioDecoder::stop SafeDelete(mBufferGroup); {");
            SafeDelete(mBufferGroup);
            LOGV("UMCAudioDecoder::stop SafeDelete(mBufferGroup); }");
        }
        LOGV("UMCAudioDecoder::stop SafeRelease(mInputBuffer); {");
        SafeRelease(mInputBuffer);
        LOGV("UMCAudioDecoder::stop SafeRelease(mInputBuffer); }");

        status_t result(OK);
        if(mStarted){
            LOGV("UMCAudioDecoder::stop mSource->stop(); {");

            if (mSkipCutBuffer != NULL) {
                mSkipCutBuffer->clear();
            }

            status_t result = mSource->stop();
            LOGV("UMCAudioDecoder::stop mSource->stop(); }");
            if(mpAudioUMCDecoder){
                mpAudioUMCDecoder->Reset();
            }
        }

        mStarted = false;
        LOGV("UMCAudioDecoder::stop Locked } {%d}", __LINE__);

        return result;
    }

    template<FNCreateDecoder fnFactory>
    sp<MetaData> UMCAudioDecoder<fnFactory>::getFormat() {
        Mutex::Autolock autoLock(mLock);
        LOGV("getFormat");
        return mMeta;
    }

    template<FNCreateDecoder fnFactory>
    status_t UMCAudioDecoder<fnFactory>::read(MediaBuffer **out, const ReadOptions *options) {
        Mutex::Autolock autoLock(mLock);

        LOGV("-----------------------\nUMCAudioDecoder::read Locked {%d}", __LINE__);

        if (mStarted != true) {
             LOGV("UMCAudioDecoder::read called before calling start", __LINE__);
             return UNKNOWN_ERROR;
        }
        status_t err = 0;
        UMC::Status decoderStatus = OK;

        *out = NULL;
        int64_t seekTimeUs;
        int32_t channels = 0;
        ReadOptions optionsNoSeek;
        MediaBuffer *pBuffer;
        int64_t deltaTime = 0;
        int32_t numSamplesDecoded = 0;
        int32_t numChannels;

        status_t status = mBufferGroup->acquire_buffer(&pBuffer);
        if (OK != status ) {
            LOGE("mBufferGroup->acquire_buffer(&pBuffer) returned %d {%d}", status, __LINE__);
            return status;
        }
        bool isDecodingSucceed = true; // assume that decoding will be sucessfull
        bool isNeedSeekOptions = false;

        ReadOptions::SeekMode mode;
        if (options && options->getSeekTo(&seekTimeUs, &mode)) {
            if (seekTimeUs < 0) {
                LOGE("seekTimeUs = %lli {%d}", seekTimeUs, __LINE__);
                SafeRelease(pBuffer);
                return BAD_VALUE;
            }

            isNeedSeekOptions = true;
            mNumSamplesOutput = 0;

            SafeRelease(mInputBuffer);
            // Make sure that the next buffer output does not still
            // depend on fragments from the last one decoded.
            LOGV("seeking... mpAudioUMCDecoder->Reset() {%d}", __LINE__);
            mpAudioUMCDecoder->Reset();
        } else {
            seekTimeUs = -1;
        }

        do {
            LOGV("UMCAudioDecoder::read mInputBuffer == %p {%d}", mInputBuffer, __LINE__);
            if (NULL == mInputBuffer) {
                if(isNeedSeekOptions)
                {
                    isNeedSeekOptions = false;
                    err = mSource->read(&mInputBuffer, options);
                }
                else
                {
                    err = mSource->read(&mInputBuffer, &optionsNoSeek);
                }
                LOGV("UMCAudioDecoder::read mSource->read returned(%d) {%d}", err, __LINE__);

                if (OK != err) {
                    if (ERROR_IO == err) {
                        LOGD("UMCAudioDecoder::input data read (mSource->read) returned %d, considering ERROR_IO\n", err);
                        SafeRelease(pBuffer);
                        return ERROR_IO;
                    } else {
                        LOGD("UMCAudioDecoder::input data read (mSource->read) returned %d, considering end of stream.\n", err);
                        SafeRelease(pBuffer);
                        return ERROR_END_OF_STREAM;
                    }
                }
                int64_t timeUs;
                sp<MetaData> inputFormat = mInputBuffer->meta_data();
                if (inputFormat.get() && inputFormat->findInt64(kKeyTime, &timeUs)) {
                    if (mAnchorTimeUs != timeUs) {
                        LOGV("Anchor time changed on %d frame! Before %lld, after %lld", mNumDecodedBuffers, mAnchorTimeUs, timeUs);
                    }

                    mAnchorTimeUs = timeUs;
                    mNumSamplesOutput = 0;
                    LOGV("UMCAudioDecoder::read mAnchorTimeUs->%lld, mNumSamplesOutput->0 {%d}", mAnchorTimeUs, __LINE__);
                } else {
                    // We must have a new timestamp after seeking.
                    if (seekTimeUs >= 0){
                        LOGE("seekTimeUs = %lli {%d}", seekTimeUs, __LINE__);
                        SafeRelease(pBuffer);
                        return BAD_VALUE;
                    }
                }
                LOGV("UMCAudioDecoder::read {%d}", __LINE__);
            }

            sp<MetaData> inputFormat = mInputBuffer->meta_data();
            int32_t numFrameSamples;
            if (inputFormat.get() && inputFormat->findInt32(kKeyValidSamples, &numFrameSamples)) {
                if (numFrameSamples < 0) {
                    LOGE("seekTimeUs = %lli {%d}", seekTimeUs, __LINE__);
                    SafeRelease(pBuffer);
                    return BAD_VALUE;
                }
                mNumSamplesLeftInFrame = numFrameSamples;
                LOGV("Number of valid samples in frame is limited to %d {%d}", mNumSamplesLeftInFrame, __LINE__);
            }

            LOGV("mInputBuffer data pointer = %p,range_offset = %d, range_length = %d", mInputBuffer->data(), mInputBuffer->range_offset(), mInputBuffer->range_length());
            LOGV("buffer data pointer = %p, size = %d, mOutData ptr = %p", pBuffer->data(), pBuffer->size(), mOutData.GetBufferPointer());

            mInData.SetBufferPointer((uint8_t *)mInputBuffer->data() + mInputBuffer->range_offset(),mInputBuffer->range_length());
            mInData.SetDataSize( mInputBuffer->range_length());
            size_t dataSizeBeforeDecode = mInputBuffer->range_length();
            size_t decodedDataSize = 0;

#ifdef AUDIO_DUMP_ENABLE
            char value[PROPERTY_VALUE_MAX];
            if (property_get("audio.media_pb.parser.dump", value, "disable")) {
                if (!strcmp(value, "enable") && mParserAudioDump) {
                    mParserAudioDump->dumpData((uint8_t *)mInputBuffer->data(), mInputBuffer->range_offset(), mInputBuffer->range_length());
                } else if (!strcmp(value, "enable") && !mParserAudioDump) {
                    mParserAudioDump = new AudioDump(AudioDump::AUDIO_PARSER);
                    if (mParserAudioDump) {
                        mParserAudioDump->dumpData((uint8_t *)mInputBuffer->data(), mInputBuffer->range_offset(), mInputBuffer->range_length());
                    }
                }
            }
#endif
            // Decode frame
            do {
                mOutData.SetBufferPointer( static_cast<uint8_t *>(pBuffer->data()), pBuffer->size() );
                mOutData.MoveDataPointer(decodedDataSize);
                mOutData.SetDataSize(0);

                AUTO_TIMER(LOG_TAG);
                LOGV("before getframe mInData & mOutData size: %d & %d", mInData.GetDataSize(), mOutData.GetDataSize());
                decoderStatus = mpAudioUMCDecoder->GetFrame( &mInData, &mOutData );
                LOGV(" after getframe mInData & mOutData size: %d & %d", mInData.GetDataSize(), mOutData.GetDataSize());

                if (decoderStatus == UMC::UMC_OK) {
                    mSyncWordMissCnt = 0;
                }
                // Increment decoded data size:
                size_t decodedDataSizeOnIteration = mOutData.GetDataSize();
                decodedDataSize += decodedDataSizeOnIteration;
                LOGV("Decoded %d bytes of data on current iteration. Total bytes per frame equals %d bytes so far.",
                     decodedDataSizeOnIteration, decodedDataSize);

                if (decoderStatus == UMC::UMC_ERR_NOT_ENOUGH_BUFFER) {
                    size_t sizeBefore = mOutData.GetBufferSize();
                    LOGW("Not enough output buffer size %d! Requesting new buffer size...", sizeBefore);

                    UMC::AudioCodecParams params;
                    UMC::Status sts = mpAudioUMCDecoder->GetInfo(&params);
                    if (sts != UMC::UMC_OK) {
                        LOGW("Failed to get decoder parameters! Error %d", sts);
                        decoderStatus = sts;
                        break;
                    }

                    if (params.m_SuggestedOutputSize > sizeBefore) {
                        LOGV("Trying to reallocate output buffer with new suggested size: %d", params.m_SuggestedOutputSize);

                        MediaBufferGroup* pNewBufferGroup = new MediaBufferGroup;

                        LOGV("Create new buffer group.");
                        if(NULL==pNewBufferGroup){
                            LOGE("UMCDecoder::start 'new MediaBufferGroup' NO_MEMORY {%d}",  __LINE__);
                            return NO_MEMORY;
                        }

                        LOGV("Create new media buffer.");
                        MediaBuffer *pNewBuffer = new MediaBuffer(params.m_SuggestedOutputSize);
                        if (NULL == pNewBuffer) {
                            SafeDelete(pNewBufferGroup);
                            LOGE("UMCDecoder::start 'new MediaBuffer(%d)' NO_MEMORY {%d}", params.m_SuggestedOutputSize,  __LINE__);
                            return NO_MEMORY;
                        }

                        // Copy decoded output data to new buffer:
                        LOGV("Copy %d bytes of decoded data to the new buffer.", decodedDataSize);
                        memcpy(pNewBuffer->data(), pBuffer->data(), decodedDataSize );

                        LOGV("Add new buffer to the new buffer group.");
                        pNewBufferGroup->add_buffer(pNewBuffer);
                        status_t status = pNewBufferGroup->acquire_buffer(&pNewBuffer);
                        if (OK != status ) {
                            SafeRelease(pBuffer);
                            SafeDelete(pNewBufferGroup);
                            LOGE("pNewBufferGroup->acquire_buffer(&pNewBuffer) returned %d {%d}", status, __LINE__);
                            return status;
                        }

                        LOGV("Release old buffer & group and replace them with a new buffer & group.");
                        SafeRelease(pBuffer);
                        SafeDelete(mBufferGroup);

                        pBuffer      = pNewBuffer;
                        mBufferGroup = pNewBufferGroup;

                    } else {
                        LOGE("New suggested output buffer size is equal or smaller than before: %d", params.m_SuggestedOutputSize);
                        decoderStatus = ERROR_BUFFER_TOO_SMALL;
                    }
                }
            }
            while(decoderStatus == UMC::UMC_ERR_NOT_ENOUGH_BUFFER);

            // Set output data pointer to beginnging of buffer and adjust data size appropriately:
            LOGV("Reset output data buffer pointer to beginning and set data size to the total decoded bytes per frame %d.", decodedDataSize);
            mOutData.Reset();
            mOutData.SetDataSize(decodedDataSize);

            if(decoderStatus == UMC::UMC_OK && mOutData.GetDataSize() != 0)
            {
               mNumDecodedBuffers++;
            }

            if ((mNumDecodedBuffers == 1) && (decoderStatus == UMC::UMC_OK)) {

                bool isResetReadFromBeginning = false;
                status_t checkStatus = CheckFormatChange(isResetReadFromBeginning);
                if (checkStatus != OK) {
                  LOGW("Format changed, releasing buffers.");
                  SafeRelease(pBuffer);
                  SafeRelease(mInputBuffer);
                  if (checkStatus == INFO_FORMAT_CHANGED) {
                      if (isResetReadFromBeginning)
                      {
                          LOGW("Reset to read from beginning of input buffer. mNumDecodedBuffers->0");
                          mInData.SetBufferPointer((uint8_t *)mInputBuffer->data() + mInputBuffer->range_offset(),mInputBuffer->range_length());
                          mInData.SetDataSize( mInputBuffer->range_length());
                          mOutData.Reset();
                          mNumDecodedBuffers = 0;
                      }

                      return INFO_FORMAT_CHANGED;
                  } else {
                      LOGW("Check format changed returned unexpected error: %d", checkStatus);
                      return checkStatus;
                  }
                }

            }

            //size_t inputBufferUsedLength = mInputBuffer->range_length() - mInData.GetDataSize();
            if ( UMC::UMC_OK != decoderStatus) {
                bool isRecoverable = UMC::UMC_ERR_FAILED != decoderStatus;
                if (mAllowSyncWordMissing == false) {
                    // Not recoverable if sync word not found within the max no. of allowed frames.
                    if (decoderStatus == UMC::UMC_ERR_SYNC && mSyncWordMissCnt++ == MAX_NUM_SYNC_MISS) {
                        isRecoverable = false;
                    }
                }
                LOGV("UMC decoder returned error %d...%srecoverable, dataSizeBeforeDecode=%d", decoderStatus, isRecoverable?"":"UN", dataSizeBeforeDecode);

                if (!isRecoverable) {
                    SafeRelease(pBuffer);
                    SafeRelease(mInputBuffer);

                    return UNKNOWN_ERROR;
                }
                // This is recoverable, just ignore the current frame and
                // play silence instead.
                LOGW("substituting %d bytes of silence", pBuffer->size());
                memset(pBuffer->data(), 0, pBuffer->size());
                mInputBuffer->set_range(
                    mInputBuffer->range_offset() + dataSizeBeforeDecode,
                    mInputBuffer->range_length() - dataSizeBeforeDecode);

            } else {
                    LOGV("mInputBuffer( range_offset=%d, range_length=%d) dataSizeBeforeDecode=%d mInData.GetDataSize=%d ",
                            mInputBuffer->range_offset(), mInputBuffer->range_length(), dataSizeBeforeDecode, mInData.GetDataSize());
                    LOGV("mInputBuffer->set_range(%d, %d)", mInputBuffer->range_offset() + (dataSizeBeforeDecode - mInData.GetDataSize()),
                            mInputBuffer->range_length() - (dataSizeBeforeDecode - mInData.GetDataSize()));

                    mInputBuffer->set_range(
                            mInputBuffer->range_offset() + (dataSizeBeforeDecode - mInData.GetDataSize()),
                            mInputBuffer->range_length() - (dataSizeBeforeDecode - mInData.GetDataSize()));
            }
            channels = mOutData.m_info.iChannels;
            numSamplesDecoded = mOutData.GetDataSize() / (sizeof(int16_t) * (channels?channels:1));

            // Adjust number of decoded samples to skip invalid samples
            int32_t outputBufferRange = mOutData.GetDataSize();
            if (mNumSamplesLeftInFrame >= 0) {
                if (numSamplesDecoded > mNumSamplesLeftInFrame) {
                    LOGV("Discarding %d samples at end of frame", numSamplesDecoded - mNumSamplesLeftInFrame);
                    numSamplesDecoded = mNumSamplesLeftInFrame;
                    outputBufferRange = numSamplesDecoded * sizeof(int16_t) * mOutData.m_info.iChannels;
                }
                mNumSamplesLeftInFrame -= numSamplesDecoded;
            }

           LOGV("buffer->set_range(0, %d)", outputBufferRange);
            pBuffer->set_range(0, outputBufferRange);

            if (mInputBuffer->range_length() == 0) {
                SafeRelease(mInputBuffer);
                LOGV("mInputBuffer->range_length() == 0, mInputBuffer released");
            }

            deltaTime = 0;
            if (0 == mNumSamplesOutput) {
                //Output Port Parameters:
               LOGV("UMC decoder OutData sample frequency: %d", mOutData.m_info.iSampleFrequency);
               LOGV("UMC decoder OutData channels number: %d", mOutData.m_info.iChannels);
            } else {
                // Calculate key delta time:
                if (mOutData.m_info.iSampleFrequency > 0) {
                    deltaTime = (mNumSamplesOutput * US_PER_SECOND ) / (mOutData.m_info.iSampleFrequency);
                    LOGV("Frame delta time: %lld",  deltaTime);
                }
                else {
                    LOGW("Failed to calculate key time! Sampling frequency of output audio data is lower or equal to zero: %d", mOutData.m_info.iSampleFrequency);
                }
            }
        isDecodingSucceed = ((UMC::UMC_OK == decoderStatus) & (mOutData.GetDataSize() != 0));
        }
        while(isDecodingSucceed != true);
        // Set frame key time:
        pBuffer->meta_data()->setInt64( kKeyTime, mAnchorTimeUs + deltaTime);

        mNumSamplesOutput += numSamplesDecoded;
        LOGV("sample_frequency=%d, mNumSamplesOutput=%d, mAnchorTimeUs=%lld, mNumDecodedBuffers=%d",
                mOutData.m_info.iSampleFrequency, mNumSamplesOutput, mAnchorTimeUs, mNumDecodedBuffers);


        if (mIsFirstBuffer)
        {
            if (getDecoderDelay())
            {
                int32_t delayInSamles = getDecoderDelay();
                int32_t delayInBytes = delayInSamles * mOutData.m_info.iChannels
                        * sizeof(int16_t);
                pBuffer->set_range(delayInBytes, pBuffer->range_length() - delayInBytes);
            }
            mIsFirstBuffer = false;
        }
        if (mSkipCutBuffer != NULL) {
            mSkipCutBuffer->submit(pBuffer);
        }

        *out = pBuffer;

        LOGV("output size: %d", mOutData.GetDataSize());

        return OK;
    }

    template<FNCreateDecoder fnFactory>
    status_t UMCAudioDecoder<fnFactory>::CheckFormatChange(bool &isResetReadFromBeginning)
    {
        int32_t sampleRate;
        int32_t numChannels;
        bool formatchange = false;

        isResetReadFromBeginning = false;

        // mMeta should be valid - initialized in the constructor
        if (!mMeta->findInt32(kKeySampleRate, &sampleRate)) {
            LOGE("kKeySampleRate was not found in mSource metadata!");
            return NAME_NOT_FOUND;
        }

        if (!mMeta->findInt32(kKeyChannelCount, &numChannels)) {
            LOGE("kKeyChannelCount was not found in mSource metadata!");
            return NAME_NOT_FOUND;
        }

        if (mOutData.m_info.iSampleFrequency != (uint32_t)sampleRate) {
            mMeta->setInt32(kKeySampleRate, mOutData.m_info.iSampleFrequency);
            LOGW("Sample rate was %d Hz, but is now %d Hz.", sampleRate, mOutData.m_info.iSampleFrequency);
            formatchange = true;
        }

        if (mOutData.m_info.iChannels != (uint32_t)numChannels) {
            mMeta->setInt32(kKeyChannelCount, mOutData.m_info.iChannels);
            LOGW("Channel count was %d, but is now %d.", numChannels, mOutData.m_info.iChannels);
            // We don't need to release buffers because data is already correctly decoded.
        }

        numChannels = mOutData.m_info.iChannels;
        int32_t iMultiChannelSupport = 0;

        if (mMeta->findInt32(kKeyMultichannelSupport, &iMultiChannelSupport)) {
            mmultiChannelSupport = (iMultiChannelSupport == 1);
        }

        if (mmultiChannelSupport) {
            LOGV("Multi-channel support enabled");
        } else {
            LOGV("Multi-channel support disabled");
        }

        if (!mmultiChannelSupport && (numChannels < 0 || numChannels > 2) && numChannels != 6) {
            LOGE("Unsupported channel count %d", numChannels);
            return ERROR_UNSUPPORTED;
        }

        if (formatchange) {
            return INFO_FORMAT_CHANGED;
        } else {
            return OK;
        }
    }

    template<FNCreateDecoder fnFactory>
    int32_t UMCAudioDecoder<fnFactory>::getDecoderDelay()
    {
        return 0;
    }
}//namespace android

#endif //UMC_DECODER_H_
