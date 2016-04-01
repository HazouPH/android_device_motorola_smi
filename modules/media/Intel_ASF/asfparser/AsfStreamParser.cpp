/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/


#define LOG_NDEBUG 0
#define LOG_TAG "AsfStreamParser"
#include <utils/Log.h>

#include "AsfHeaderParser.h"
#include "AsfDataParser.h"
#include "AsfIndexParser.h"
#include "AsfStreamParser.h"
#include <string.h>


AsfStreamParser::AsfStreamParser(void)
    : mDataPacketSize(0),
      mTimeOffsetMs(0),
      mHeaderParsed(false) {
    mHeaderParser = new AsfHeaderParser;
    mDataParser = new AsfDataParser(mHeaderParser);
    mSimpleIndexParser = NULL;
}

AsfStreamParser::~AsfStreamParser(void) {
    delete mHeaderParser;
    delete mDataParser;
    delete mSimpleIndexParser;
}

bool AsfStreamParser::isSimpleIndexObject(uint8_t *guid) {
    GUID *id = (GUID *)guid;
    return (*id == ASF_Simple_Index_Object);
}

bool AsfStreamParser::isHeaderObject(uint8_t *guid) {
    GUID *id = (GUID *)guid;
    return (*id == ASF_Header_Object);
}

int AsfStreamParser::parseHeaderObject(uint8_t *buffer, uint64_t size) {
    int status = mHeaderParser->parse(buffer, size);
    if (status != ASF_PARSER_SUCCESS) {
        return status;
    }

    mDataPacketSize = mHeaderParser->getDataPacketSize();
    mTimeOffsetMs = mHeaderParser->getTimeOffset() / ASF_SCALE_MS_TO_100NANOSEC;

    if (mTimeOffsetMs == 0) {
        // offset of PTS in milliseconds due to buffering
        mTimeOffsetMs = mHeaderParser->getPreroll();
    }
    mHeaderParsed = true;
    return ASF_PARSER_SUCCESS;
}

int AsfStreamParser::getDrmUuid(uint8_t playreadyUuid[], uint16_t len) {
    return mHeaderParser->getPlayreadyUuid(playreadyUuid, len);
}

int AsfStreamParser::getDrmHeaderXml(uint8_t *playreadyHeader, uint32_t *playreadyHeaderLen) {
    return mHeaderParser->getPlayreadyHeaderXml(playreadyHeader, playreadyHeaderLen);
}
AsfAudioStreamInfo* AsfStreamParser::getAudioInfo() const {
    return mHeaderParser->getAudioInfo();
}

AsfVideoStreamInfo* AsfStreamParser::getVideoInfo() const {
    return mHeaderParser->getVideoInfo();
}

AsfFileMediaInfo* AsfStreamParser::getFileInfo() const {
    return mHeaderParser->getFileInfo();
}

uint64_t AsfStreamParser::getDuration() {
    return mHeaderParser->getDuration();
}

uint32_t AsfStreamParser::getDataPacketSize() {
    return mHeaderParser->getDataPacketSize();
}

bool AsfStreamParser::hasVideo() {
    return mHeaderParser->hasVideo();
}

bool AsfStreamParser::hasAudio() {
    return mHeaderParser->hasAudio();
}

int AsfStreamParser::parseDataObjectHeader(uint8_t *buffer, uint32_t size) {
    if (mHeaderParsed == false) {
        return ASF_PARSER_INVALID_STATE;
    }
    return mDataParser->parseHeader(buffer, size);
}

int AsfStreamParser::parseDataPacket(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out) {
    if (mHeaderParsed == false) {
        return ASF_PARSER_INVALID_STATE;
    }

    if (size != mDataPacketSize) {
        return ASF_PARSER_BAD_DATA;
    }

    if (out == NULL) {
        return ASF_PARSER_NULL_POINTER;
    }

    int status = mDataParser->parsePacket(buffer, size, out);
    if (status != ASF_PARSER_SUCCESS) {
        return status;
    }

    if (mTimeOffsetMs == 0) {
        return ASF_PARSER_SUCCESS;
    }

    // update presentation time stamp
    AsfPayloadDataInfo *next = *out;
    while (next) {
        if (next->presentationTime >= mTimeOffsetMs) {
            next->presentationTime -= mTimeOffsetMs;
        }
        else {
            // TODO:
            next->presentationTime = 0;
            // return ASF_PARSER_BAD_VALUE;
        }
        next = next->next;
    }
    return status;
}

void AsfStreamParser::releasePayloadDataInfo(AsfPayloadDataInfo *info) {
    mDataParser->releasePayloadDataInfo(info);
}


int AsfStreamParser::parseSimpleIndexObject(uint8_t *buffer, uint32_t size) {
    if (mHeaderParsed == false) {
        return ASF_PARSER_INVALID_STATE;
    }

    if (mHeaderParser->isSeekable() == false) {
        return ASF_PARSER_FAILED;
    }

    if (mSimpleIndexParser) {
        delete mSimpleIndexParser;
        mSimpleIndexParser = NULL;
    }

    mSimpleIndexParser = new AsfSimpleIndexParser;

    if (mSimpleIndexParser == NULL) return ASF_PARSER_FAILED;

    if (ASF_PARSER_SUCCESS != mSimpleIndexParser->parse(buffer, size)) {
        delete mSimpleIndexParser;
        mSimpleIndexParser = NULL;
        return ASF_PARSER_FAILED;
    }

    return ASF_PARSER_SUCCESS;
}

AsfSimpleIndexInfo* AsfStreamParser::getIndexInfo() const {
    if (!mSimpleIndexParser) return NULL;

    return mSimpleIndexParser->getIndexInfo();
}

int AsfStreamParser::seek(
        uint64_t seekTime,
        bool nextSync,
        uint32_t& packetNumber,
        uint64_t& targetTime) {
    if (mHeaderParsed == false) {
        return ASF_PARSER_INVALID_STATE;
    }

    if (mHeaderParser->isSeekable() == false) {
        return ASF_PARSER_FAILED;
    }

    if (mSimpleIndexParser) {
        seekTime += mHeaderParser->getPreroll()*ASF_SCALE_MS_TO_100NANOSEC;  //add preroll start time
        return mSimpleIndexParser->seek(seekTime, nextSync, packetNumber, targetTime);
    }
    else {
        // no index object, need to seek using average bitrate method

        if (mHeaderParser->hasVideo()){
            return ASF_PARSER_FAILED;
        }

        if (!mHeaderParser->hasAudio()) {
            return ASF_PARSER_FAILED;
        }

        int totalByteRate=0;
        AsfAudioStreamInfo* audioInfo = mHeaderParser->getAudioInfo();
        while (audioInfo != NULL) {
            totalByteRate += audioInfo->avgByteRate;
            audioInfo = audioInfo->next;
        }

        if (totalByteRate == 0) {
            return ASF_PARSER_FAILED;
        }

        uint32_t packetSize = mHeaderParser->getDataPacketSize();
        if (packetSize <= 0) {
            return ASF_PARSER_FAILED;
        }

        packetNumber = seekTime/10000000 * totalByteRate / packetSize;
        targetTime = seekTime;
        return ASF_PARSER_SUCCESS;
    }
}

uint32_t AsfStreamParser::getMaxObjectSize() {
    if (!mSimpleIndexParser) return NULL;
    return mSimpleIndexParser->getMaximumPacketCount() * mDataPacketSize;
}
