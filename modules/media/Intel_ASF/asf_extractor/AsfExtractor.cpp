/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


//#define LOG_NDEBUG 0
#define LOG_TAG "AsfExtractor"
#include <utils/Log.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <utils/String8.h>

#include "MetaDataExt.h"
#include "MediaBufferPool.h"
#include "AsfStreamParser.h"
#include "AsfExtractor.h"


namespace android {

// The audio format tags that represent the input categories supported
// by the Windows Media Audio decoder, don't change it
enum WMAAudioFormats {
    WAVE_FORMAT_MSAUDIO1         = 0x160,
    WAVE_FORMAT_WMAUDIO2         = 0x161,
    WAVE_FORMAT_WMAUDIO3X        = 0x162,
    WAVE_FORMAT_WMAUDIO_LOSSLESS = 0x163,
    WAVE_FORMAT_WMAVOICE9        = 0x000A,
    WAVE_FORMAT_WMAVOICE10       = 0x000B,
};

class ASFSource : public MediaSource  {
public:
    ASFSource(const sp<AsfExtractor> &extractor, int trackIndex)
        : mExtractor(extractor),
          mTrackIndex(trackIndex) {
    }

    virtual status_t start(MetaData *params = NULL) {
        return OK;
    }

    virtual status_t stop() {
        return OK;
    }

    virtual sp<MetaData> getFormat() {
        return mExtractor->getTrackMetaData(mTrackIndex, 0);
    }

    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options = NULL) {
        return mExtractor->read(mTrackIndex, buffer, options);
    }

protected:
    virtual ~ASFSource() {
        mExtractor = NULL;
    }

private:
    sp<AsfExtractor> mExtractor;
    int mTrackIndex;

    ASFSource(const ASFSource &);
    ASFSource &operator=(const ASFSource &);
};


AsfExtractor::AsfExtractor(const sp<DataSource> &source)
    : mDataSource(source),
      mInitialized(false),
      mHasIndexObject(false),
      mFirstTrack(NULL),
      mLastTrack(NULL),
      mReadLock(),
      mFileMetaData(new MetaData),
      mParser(NULL),
      mHeaderObjectSize(0),
      mDataObjectSize(0),
      mDataPacketBeginOffset(0),
      mDataPacketEndOffset(0),
      mDataPacketCurrentOffset(0),
      mDataPacketSize(0),
      mNeedKeyFrame(false),
      mDataPacketData(NULL) {
    mParser = new AsfStreamParser;
}

AsfExtractor::~AsfExtractor() {
    uninitialize();
    mDataSource = NULL;
    mFileMetaData = NULL;
    delete mParser;
    mParser = NULL;
}

sp<MetaData> AsfExtractor::getMetaData() {
    status_t err = initialize();
    if (err != OK) {
        return new MetaData;
    }

    return mFileMetaData;
}

size_t AsfExtractor::countTracks() {
    status_t err = initialize();
    if (err != OK) {
        return 0;
    }

    size_t n = 0;
    Track *track = mFirstTrack;
    while (track) {
        ++n;
        track = track->next;
    }

    ALOGV("track count is %d", n);
    return n;
}

sp<MetaData> AsfExtractor::getTrackMetaData(size_t index, uint32_t flags) {
    status_t err = initialize();
    if (err != OK) {
        return NULL;
    }

    Track *track = getTrackByTrackIndex(index);
    if (track == NULL) {
        return NULL;
    }

    // There is no thumbnail data so ignore flags: kIncludeExtensiveMetaData
    return track->meta;
}

sp<MediaSource> AsfExtractor::getTrack(size_t index) {
    status_t err;
    if ((err = initialize()) != OK) {
        return NULL;
    }

    Track *track = getTrackByTrackIndex(index);
    if (track == NULL) {
        return NULL;
    }

    // Assume this track is active
    track->skipTrack = false;
    return new ASFSource(this, index);
}

status_t AsfExtractor::read(
        int trackIndex,
        MediaBuffer **buffer,
        const MediaSource::ReadOptions *options) {
    Track *track = getTrackByTrackIndex(trackIndex);
    if (track == NULL) {
        return BAD_VALUE;
    }

    int64_t seekTimeUs;
    MediaSource::ReadOptions::SeekMode mode;
    bool seekTo = false;

    if (options != NULL) {
        seekTo = options->getSeekTo(&seekTimeUs, &mode);
    }

    if (!mParser->hasVideo() || (mParser->hasVideo() && mHasIndexObject)) {
        if (seekTo) {
            status_t err = seek_l(track, seekTimeUs, mode);
            if (err != OK) {
                return err;
            }
        }
    } else if (mParser->hasVideo() && !mHasIndexObject){
        // Add support for auto looping of unseekable clip
        if (seekTo && seekTimeUs == 0) {
            // Start over by resetting the offset
            mDataPacketCurrentOffset = mDataPacketBeginOffset;
        }
        //ALOGW("No index object. Seek may not be supported!!!");
    }

    return read_l(track, buffer);
}

uint32_t AsfExtractor::flags() const {
    uint32_t flags = CAN_PAUSE;

    if (!mParser->hasVideo() || (mParser->hasVideo() && mHasIndexObject)) {
        flags |= CAN_SEEK_FORWARD | CAN_SEEK_BACKWARD | CAN_SEEK;
    }

    return flags;
}

static int compareTime(const int64_t *timeUs1, const int64_t *timeUs2) {

    if (*timeUs1 < *timeUs2)
        return -1;
    else if (*timeUs1 > *timeUs2)
        return 1;
    else
        return 0;
}
void AsfExtractor::establishAvgFrameRate(Track *dstTrack, int32_t *avgFrameRate) {
    *avgFrameRate = 0;
    if (dstTrack == NULL) {
        return;
    }
    Vector<int64_t> timeArray;
    timeArray.clear();
    int64_t dataOffSet = mDataPacketCurrentOffset;
    uint8_t *data = new uint8_t [mDataPacketSize];
    if (data == NULL) {
        return;
    }
    uint64_t lastTimeStamp = 0;
    while(timeArray.size() < 20) {
        if (mDataSource->readAt(dataOffSet, data, mDataPacketSize) != mDataPacketSize) {
            break;
        }
        dataOffSet += mDataPacketSize;
        AsfPayloadDataInfo *payloads = NULL;

        int status = mParser->parseDataPacket(data, mDataPacketSize, &payloads);
        if (status != ASF_PARSER_SUCCESS || payloads == NULL) {
            break;
        }
        AsfPayloadDataInfo* payload = payloads;
        while(payload) {
            Track* track = getTrackByStreamNumber(payload->streamNumber);
            if (track == NULL || track != dstTrack) {
                payload = payload->next;
                continue;
            }
            if (lastTimeStamp == 0) {
                lastTimeStamp = payload->presentationTime *1000;
                if (lastTimeStamp != 0) {
                    timeArray.push_back(lastTimeStamp);
                }
            } else {
                int64_t timestamp = payload->presentationTime *1000;
                if (lastTimeStamp != timestamp) {
                    timeArray.push_back(timestamp);
                    lastTimeStamp = timestamp;
                    ALOGV("Add timestamp = %lld", lastTimeStamp);
                }
            }
            payload = payload->next;
        }
        mParser->releasePayloadDataInfo(payloads);
    }
    if (data != NULL) {
        delete [] data;
        data  = NULL;
    }
    if (timeArray.empty()) {
        return;
    }
    timeArray.sort(compareTime); // sort by PTS
    int32_t count = timeArray.size();
    if (count <= 1)
        return;
    // remove the last 8 items.
    count = count > 9 ? (count - 8) : count;
    int64_t beginTimeUs = timeArray.itemAt(0);
    int64_t endTimeUs = timeArray.itemAt(count - 1);
    int64_t duration = endTimeUs - beginTimeUs;
    if (duration != 0) {
        *avgFrameRate = ((count - 1) * 1000000LL + (duration >> 1)) / duration;
    }
}

status_t AsfExtractor::initialize() {
    if (mInitialized) {
        return OK;
    }

    status_t status = OK;
    // header object is the first mandatory object. The first 16 bytes
    // is GUID of object, the following 8 bytes is size of object
    if (mDataSource->readAt(16, &mHeaderObjectSize, 8) != 8) {
        return ERROR_IO;
    }

    uint8_t* headerObjectData = new uint8_t [mHeaderObjectSize];
    if (headerObjectData == NULL) {
        return NO_MEMORY;
    }

    if (mDataSource->readAt(0, headerObjectData, mHeaderObjectSize) != mHeaderObjectSize) {
        return ERROR_IO;
    }
    status = mParser->parseHeaderObject(headerObjectData, mHeaderObjectSize);
    if (status != ASF_PARSER_SUCCESS) {
        ALOGE("Failed to parse header object.");
        return ERROR_MALFORMED;
    }

    delete [] headerObjectData;
    headerObjectData = NULL;

    uint8_t dataObjectHeaderData[ASF_DATA_OBJECT_HEADER_SIZE];
    if (mDataSource->readAt(mHeaderObjectSize, dataObjectHeaderData, ASF_DATA_OBJECT_HEADER_SIZE)
        != ASF_DATA_OBJECT_HEADER_SIZE) {
        return ERROR_IO;
    }
    status = mParser->parseDataObjectHeader(dataObjectHeaderData, ASF_DATA_OBJECT_HEADER_SIZE);
    if (status != ASF_PARSER_SUCCESS) {
        ALOGE("Failed to parse data object header.");
        return ERROR_MALFORMED;
    }

    // first 16 bytes is GUID of data object
    mDataObjectSize = *(uint64_t*)(dataObjectHeaderData + 16);
    mDataPacketBeginOffset = mHeaderObjectSize + ASF_DATA_OBJECT_HEADER_SIZE;
    mDataPacketEndOffset = mHeaderObjectSize + mDataObjectSize;
    mDataPacketCurrentOffset = mDataPacketBeginOffset;

    // allocate memory for data packet
    mDataPacketSize = mParser->getDataPacketSize();
    mDataPacketData = new uint8_t [mDataPacketSize];
    if (mDataPacketData == NULL) {
        return NO_MEMORY;
    }

    const AsfFileMediaInfo *fileMediaInfo = mParser->getFileInfo();
    if (fileMediaInfo && fileMediaInfo->seekable) {
        uint64_t offset = mDataPacketEndOffset;

        // Find simple index object for time seeking.
        // object header include 16 bytes of object GUID and 8 bytes of object size.
        uint8_t objectHeader[24];
        uint64_t objectSize;
        for (;;) {
            if (mDataSource->readAt(offset, objectHeader, 24) != 24) {
                break;
            }

            objectSize = *(uint64_t *)(objectHeader + 16);
            if (!AsfStreamParser::isSimpleIndexObject(objectHeader)) {
                offset += objectSize;
                if (objectSize == 0) {
                    ALOGW("WARN: The file's objectSize is zero,ingore this header.");
                    offset += 24;
                }
                if (offset >= fileMediaInfo->fileSize) {
                    ALOGW("WARN: the file's simple index objectSize is illegal,break");
                    break;
                }
                continue;
            }
            mHasIndexObject = true;
            uint8_t* indexObjectData = new uint8_t [objectSize];
            if (indexObjectData == NULL) {
                // don't report as error, we just lose time seeking capability.
                break;
            }
            if (mDataSource->readAt(offset, indexObjectData, objectSize) == objectSize) {
                // Ignore return value
                mParser->parseSimpleIndexObject(indexObjectData, objectSize);
            }
            delete [] indexObjectData;
            break;
        }
    }

    if (mParser->hasVideo()) {
        ALOGV("MEDIA_MIMETYPE_CONTAINER_ASF");
        mFileMetaData->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_ASF);
    } else if (mParser->hasAudio() && mParser->getAudioInfo()->codecID >= WAVE_FORMAT_MSAUDIO1 &&
               mParser->getAudioInfo()->codecID <= WAVE_FORMAT_WMAUDIO_LOSSLESS) {
        ALOGV("MEDIA_MIMETYPE_AUDIO_WMA", mParser->getAudioInfo()->codecID);
        mFileMetaData->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_WMA);
    } else {
        ALOGE("Content does not have neither audio nor video.");
        return ERROR_UNSUPPORTED;
    }

    // duration returned from parser is in 100-nanosecond unit, converted it to microseconds (us)
    ALOGV("Duration is %.2f (sec)", mParser->getDuration()/1E7);
    mFileMetaData->setInt64(kKeyDuration, mParser->getDuration() / SCALE_100_NANOSEC_TO_USEC);

    setupTracks();
    mInitialized = true;
    return OK;
}

void AsfExtractor::uninitialize() {
    if (mDataPacketData) {
        delete [] mDataPacketData;
        mDataPacketData = NULL;
    }
    mDataPacketSize = 0;

    Track* track = mFirstTrack;
    MediaBuffer* p;
    while (track != NULL) {
        track->meta = NULL;
        if (track->bufferActive) {
            track->bufferActive->release();
            track->bufferActive = NULL;
        }

        int size = track->bufferQueue.size();
        for (int i = 0; i < size; i++) {
            p = track->bufferQueue.editItemAt(i);
            p->release();
        }

        track->bufferQueue.clear();
        delete track->bufferPool;

        track->meta = NULL;
        mFirstTrack = track->next;
        delete track;
        track = mFirstTrack;
    }
    mFirstTrack = NULL;
    mLastTrack = NULL;
}

static const char* FourCC2MIME(uint32_t fourcc) {
    // The first charater of FOURCC characters appears in the least-significant byte
    // WVC1 => 0x31435657
    switch (fourcc) {
        //case FOURCC('W', 'M', 'V', '1'):
        //case FOURCC('W', 'M', 'V', '2'):
        //case FOURCC('W', 'M', 'V', 'A'):
        case FOURCC('1', 'V', 'M', 'W'):
            ALOGW("WMV1 format is not supported.");
            return "video/wmv1";
        case FOURCC('2', 'V', 'M', 'W'):
            ALOGW("WMV2 format is not supported.");
            return "video/wmv2";
        case FOURCC('A', 'V', 'M', 'W'):
            ALOGW("WMV Advanced profile, assuming as WVC1 for now");
            return MEDIA_MIMETYPE_VIDEO_WMV;
        //case FOURCC('W', 'M', 'V', '3'):
        //case FOURCC('W', 'V', 'C', '1'):
        case FOURCC('3', 'V', 'M', 'W'):
        case FOURCC('1', 'C', 'V', 'W'):
            return MEDIA_MIMETYPE_VIDEO_WMV;
        default:
            ALOGE("Unknown video format.");
            return "video/unknown-type";
    }
}

static const char* CodecID2MIME(uint32_t codecID) {
    switch (codecID) {
        // WMA version 1
        case WAVE_FORMAT_MSAUDIO1:
        // WMA version 2 (7, 8, 9 series)
        case WAVE_FORMAT_WMAUDIO2:
            return MEDIA_MIMETYPE_AUDIO_WMA;
        // WMA 9 lossless
        case WAVE_FORMAT_WMAUDIO_LOSSLESS:
            //return MEDIA_MIMETYPE_AUDIO_WMA_LOSSLESS;
            return MEDIA_MIMETYPE_AUDIO_WMA;
        // WMA voice 9
        case WAVE_FORMAT_WMAVOICE9:
        // WMA voice 10
        case WAVE_FORMAT_WMAVOICE10:
            ALOGW("WMA voice 9/10 is not supported.");
            return "audio/wma-voice";
        default:
            ALOGE("Unsupported Audio codec ID: %#x", codecID);
            return "audio/unknown-type";
    }
}


status_t AsfExtractor::setupTracks() {
    AsfAudioStreamInfo* audioInfo = mParser->getAudioInfo();
    AsfVideoStreamInfo* videoInfo = mParser->getVideoInfo();
    Track* track;
    while (audioInfo || videoInfo) {
        track = new Track;
        if (mLastTrack == NULL) {
            mFirstTrack = track;
            mLastTrack = track;
        } else {
            mLastTrack->next = track;
            mLastTrack = track;
        }

        // this flag will be set to false within getTrack
        track->skipTrack = true;
        track->seekCompleted = false;
        track->next = NULL;
        track->meta = new MetaData;
        track->bufferActive = NULL;
        track->bufferPool = new MediaBufferPool;

        if (audioInfo) {
            ALOGV("streamNumber = %d\n, encryptedContentFlag= %d\n, timeOffset = %lld\n,"
                  "codecID = %d\n, numChannels=%d\n, sampleRate=%d\n, avgBitRate = %d\n,"
                  "blockAlignment =%d\n, bitsPerSample=%d\n, codecDataSize=%d\n",
                  audioInfo->streamNumber, audioInfo->encryptedContentFlag,
                  audioInfo->timeOffset, audioInfo->codecID, audioInfo->numChannels,
                  audioInfo->sampleRate, audioInfo->avgByteRate*8, audioInfo->blockAlignment,
                  audioInfo->bitsPerSample, audioInfo->codecDataSize);

            track->streamNumber = audioInfo->streamNumber;
            track->encrypted = audioInfo->encryptedContentFlag;
            track->meta->setInt32(kKeyChannelCount, audioInfo->numChannels);
            track->meta->setInt32(kKeySampleRate, audioInfo->sampleRate);
            track->meta->setInt32(kKeyWmaBlockAlign, audioInfo->blockAlignment);
            track->meta->setInt32(kKeyBitPerSample, audioInfo->bitsPerSample);
            track->meta->setInt32(kKeyBitRate, audioInfo->avgByteRate*8);
            track->meta->setInt32(kKeyWmaFormatTag, audioInfo->codecID);

            if (audioInfo->codecDataSize) {
                track->meta->setData(
                    kKeyConfigData,
                    kTypeConfigData,
                    audioInfo->codecData,
                    audioInfo->codecDataSize);
            }
            // duration returned is in 100-nanosecond unit
            track->meta->setInt64(kKeyDuration, mParser->getDuration() / SCALE_100_NANOSEC_TO_USEC);
            track->meta->setCString(kKeyMIMEType, CodecID2MIME(audioInfo->codecID));
            track->meta->setInt32(kKeySuggestedBufferSize, mParser->getDataPacketSize());
            audioInfo = audioInfo->next;
        } else {
            track->streamNumber = videoInfo->streamNumber;
            track->encrypted = videoInfo->encryptedContentFlag;
            track->meta->setInt32(kKeyWidth, videoInfo->width);
            track->meta->setInt32(kKeyHeight, videoInfo->height);
            if (videoInfo->codecDataSize) {
                track->meta->setData(
                    kKeyConfigData,
                    kTypeConfigData,
                    videoInfo->codecData,
                    videoInfo->codecDataSize);
            }
            // duration returned is in 100-nanosecond unit
            track->meta->setInt64(kKeyDuration, mParser->getDuration() / SCALE_100_NANOSEC_TO_USEC);
            track->meta->setCString(kKeyMIMEType, FourCC2MIME(videoInfo->fourCC));
            int maxSize = mParser->getMaxObjectSize();
            if (maxSize == 0) {
                // estimated maximum packet size.
                maxSize = 10 * mParser->getDataPacketSize();
            }
            track->meta->setInt32(kKeySuggestedBufferSize, maxSize);
            if (mHasIndexObject) {
                // set arbitary thumbnail time
                track->meta->setInt64(kKeyThumbnailTime, mParser->getDuration() / (SCALE_100_NANOSEC_TO_USEC * 2));
            } else {
                track->meta->setInt64(kKeyThumbnailTime, 0);
            }
            int32_t avgFrameRate = 0;
            establishAvgFrameRate(track, &avgFrameRate);
            if (avgFrameRate > 0) {
                track->meta->setInt32(kKeyFrameRate, avgFrameRate);
                ALOGV("get avf frame rate = %d", avgFrameRate);
            }
            videoInfo = videoInfo->next;
        }
    }

    return OK;
}

status_t AsfExtractor::seek_l(Track* track, int64_t seekTimeUs, MediaSource::ReadOptions::SeekMode mode) {
    Mutex::Autolock lockSeek(mReadLock);

    // It is expected seeking will happen on all the tracks with the same seeking options.
    // Only the first track receiving the seeking command will perform seeking and all other
    // tracks just siliently ignore it.

    // TODO: potential problems in the following case:
    // audio seek
    // video read
    // video seek
    // video read

    if (track->seekCompleted) {
        // seeking is completed through a different track
        track->seekCompleted = false;
        return OK;
    }

    uint64_t targetSampleTimeUs = 0;

    // seek to next sync sample or previous sync sample
    bool nextSync = false;
    switch (mode) {
        case MediaSource::ReadOptions::SEEK_NEXT_SYNC:
        nextSync = true;
        break;
        // Always seek to the closest previous sync frame
        case MediaSource::ReadOptions::SEEK_PREVIOUS_SYNC:
        case MediaSource::ReadOptions::SEEK_CLOSEST_SYNC:

        // Not supported, already seek to sync frame, so will  not set kKeyTargetTime on bufferActive.
        case MediaSource::ReadOptions::SEEK_CLOSEST:
        default:
        break;
    }

    uint32_t packetNumber;
    uint64_t targetTime;
    // parser takes seek time in 100-nanosecond unit and returns target time in 100-nanosecond as well.
    if (!mParser->seek(seekTimeUs * SCALE_100_NANOSEC_TO_USEC, nextSync, packetNumber, targetTime)) {
        ALOGV("Seeking failed.");
        return ERROR_END_OF_STREAM;
    }
    ALOGV("seek time = %.2f secs, actual time = %.2f secs", seekTimeUs/1E6, targetTime / 1E7);

    // convert to microseconds
    targetSampleTimeUs = targetTime / SCALE_100_NANOSEC_TO_USEC;
    mDataPacketCurrentOffset = mDataPacketBeginOffset + packetNumber * mDataPacketSize;
    ALOGV("data packet offset = %lld", mDataPacketCurrentOffset);

    // flush all pending buffers on all the tracks
    Track* temp = mFirstTrack;
    while (temp != NULL) {
        Mutex::Autolock lockTrack(temp->lock);
        if (temp->bufferActive) {
            temp->bufferActive->release();
            temp->bufferActive = NULL;
        }

        int size = temp->bufferQueue.size();
        for (int i = 0; i < size; i++) {
            MediaBuffer* buffer = temp->bufferQueue.editItemAt(i);
            buffer->release();
        }
        temp->bufferQueue.clear();

        if (temp != track) {
            // notify all other tracks seeking is completed.
            // this flag is reset when seeking request is made on each track.
            // don't set this flag on the driving track so a new seek can be made.
            temp->seekCompleted = true;
        }
        temp = temp->next;
    }
    if (mParser->hasVideo()) {
        mNeedKeyFrame =true;
    }
    return OK;
}

status_t AsfExtractor::read_l(Track *track, MediaBuffer **buffer) {
    status_t err = OK;
    while (err == OK) {
        Mutex::Autolock lock(track->lock);
        if (track->bufferQueue.size() != 0) {
            *buffer = track->bufferQueue[0];
            track->bufferQueue.removeAt(0);
            return OK;
        }
        track->lock.unlock();

        err = readPacket();
    }
    ALOGE("read_l failed.");
    return err;
}

status_t AsfExtractor::readPacket() {
    Mutex::Autolock lock(mReadLock);
    if (mDataPacketCurrentOffset + mDataPacketSize > mDataPacketEndOffset) {
        ALOGI("readPacket hits end of stream.");
        return ERROR_END_OF_STREAM;
    }

    if (mDataSource->readAt(mDataPacketCurrentOffset, mDataPacketData, mDataPacketSize) !=
        mDataPacketSize) {
        return ERROR_END_OF_STREAM;
    }

    // update next read position
    mDataPacketCurrentOffset += mDataPacketSize;
    AsfPayloadDataInfo *payloads = NULL;
    int status = mParser->parseDataPacket(mDataPacketData, mDataPacketSize, &payloads);
    if (status != ASF_PARSER_SUCCESS || payloads == NULL) {
        ALOGE("Failed to parse data packet. status = %d", status);
        return ERROR_END_OF_STREAM;
    }

    AsfPayloadDataInfo* payload = payloads;
    while (payload) {
        Track* track = getTrackByStreamNumber(payload->streamNumber);
        if (track == NULL || track->skipTrack) {
            payload = payload->next;
            continue;
        }
        if (payload->mediaObjectLength == payload->payloadSize ||
            payload->offsetIntoMediaObject == 0) {
            const char *mime;
            CHECK(track->meta->findCString(kKeyMIMEType, &mime));
            if (mNeedKeyFrame && (!strncasecmp(mime, "video/", 6))) {
                if (!payload->keyframe) {
                    ALOGW("Non-keyframe received during seek mode!");
                    payload = payload->next;
                    continue;
                } else {
                    mNeedKeyFrame = false;
                }
            }
            // a comple object or the first payload of fragmented object
            MediaBuffer *buffer = NULL;
            status = track->bufferPool->acquire_buffer(
                payload->mediaObjectLength, &buffer);
            if (status != OK) {
                ALOGE("Failed to acquire buffer.");
                mParser->releasePayloadDataInfo(payloads);
                return status;
            }
            memcpy(buffer->data(),
                payload->payloadData,
                payload->payloadSize);

            buffer->set_range(0, payload->mediaObjectLength);
            // kKeyTime is in microsecond unit (usecs)
            // presentationTime is in mililsecond unit (ms)
            buffer->meta_data()->setInt64(kKeyTime,(uint64_t) payload->presentationTime * 1000);

            if (payload->keyframe) {
                buffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);
            }

            if (payload->mediaObjectLength == payload->payloadSize) {
                Mutex::Autolock lockTrack(track->lock);
                // a complete object
                track->bufferQueue.push(buffer);
            } else {
                // the first payload of a fragmented object
                track->bufferActive = buffer;
                if (track->encrypted) {
                    Mutex::Autolock lockTrack(track->lock);
                    MediaBuffer* copy = NULL;
                    track->bufferPool->acquire_buffer(payload->payloadSize, &copy);
                    copy->meta_data()->setInt64(kKeyTime,(uint64_t) payload->presentationTime * 1000);
                    memcpy(copy->data(), payload->payloadData, payload->payloadSize);
                    copy->set_range(0, payload->payloadSize);
                    track->bufferQueue.push(copy);
                }
            }
        } else {
            if (track->bufferActive == NULL) {
                ALOGE("Receiving corrupt or discontinuous data packet.");
                payload = payload->next;
                continue;
            }
            // TODO: check object number and buffer size!!!!!!!!!!!!!!
            // the last payload or the middle payload of a fragmented object
            memcpy(
                (uint8_t*)track->bufferActive->data() + payload->offsetIntoMediaObject,
                payload->payloadData,
                payload->payloadSize);

            if (payload->offsetIntoMediaObject + payload->payloadSize ==
                payload->mediaObjectLength) {
                // the last payload of a fragmented object
                // for encrypted content, push a cloned media buffer to vector instead.
                if (!track->encrypted)
                {
                    Mutex::Autolock lockTrack(track->lock);
                    track->bufferQueue.push(track->bufferActive);
                    track->bufferActive = NULL;
                } else {
                    Mutex::Autolock lockTrack(track->lock);
                    track->bufferActive->set_range(payload->offsetIntoMediaObject, payload->payloadSize);
                    track->bufferQueue.push(track->bufferActive);
                    track->bufferActive = NULL;
                }
            } else {
                // middle payload of a fragmented object
                if (track->encrypted) {
                    Mutex::Autolock lockTrack(track->lock);
                    MediaBuffer* copy = NULL;
                    int64_t keytime;
                    track->bufferPool->acquire_buffer(payload->payloadSize, &copy);
                    track->bufferActive->meta_data()->findInt64(kKeyTime, &keytime);
                    copy->meta_data()->setInt64(kKeyTime, keytime);
                    memcpy(copy->data(), payload->payloadData, payload->payloadSize);
                    copy->set_range(0, payload->payloadSize);
                    track->bufferQueue.push(copy);
                }
            }
        }
        payload = payload->next;
    };

    mParser->releasePayloadDataInfo(payloads);
    return OK;
}

AsfExtractor::Track* AsfExtractor::getTrackByTrackIndex(int index) {
    Track *track = mFirstTrack;
    while (index > 0) {
        if (track == NULL) {
            return NULL;
        }

        track = track->next;
        --index;
    }
    return track;
}

AsfExtractor::Track* AsfExtractor::getTrackByStreamNumber(int stream) {
    Track *track = mFirstTrack;
    while (track != NULL) {
        if (track->streamNumber == stream) {
            return track;
        }
        track = track->next;
    }
    return NULL;
}

bool SniffAsf(
        const sp<DataSource> &source,
        String8 *mimeType,
        float *confidence,
        sp<AMessage> *) {
    uint8_t guid[16];
    if (source->readAt(0, guid, 16) != 16) {
        return false;
    }
    if (!AsfStreamParser::isHeaderObject(guid)) {
        return false;
    }

    *mimeType = MEDIA_MIMETYPE_CONTAINER_ASF;
    *confidence = 0.4f;
    return true;
}

}  // namespace android

