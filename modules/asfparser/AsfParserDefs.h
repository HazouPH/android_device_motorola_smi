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



#ifndef ASF_PARSER_DEFS_H_
#define ASF_PARSER_DEFS_H_

#include "AsfObjects.h"
#include <stdint.h>
#include <vector>

#ifndef NULL
//#define NULL (void*) 0
#define NULL 0
#endif

// data object header size is 50 bytes
#define ASF_DATA_OBJECT_HEADER_SIZE             50
#define ASF_SIMPLE_INDEX_OBJECT_HEADER_SIZE     56

// 1 millisecond = 10,000 100-nano seconds
#define ASF_SCALE_MS_TO_100NANOSEC              10000

// ASF parser error codes
enum {
    ASF_PARSER_NULL_POINTER = -7,
    ASF_PARSER_INVALID_STATE = -6,
    ASF_PARSER_UNEXPECTED_VALUE = -5,
    ASF_PARSER_BAD_VALUE = -4,
    ASF_PARSER_BAD_DATA = -3,
    ASF_PARSER_NO_MEMORY = -2,
    ASF_PARSER_FAILED = -1,
    ASF_PARSER_COMPRESSED_PAYLOAD = 0,
    ASF_PARSER_SUCCESS = 1,
};

struct AsfAudioStreamInfo {
    uint8_t streamNumber;
    uint8_t encryptedContentFlag;
    uint64_t timeOffset; // in 100-nanosecond units
    uint32_t codecID;
    uint32_t numChannels;
    uint32_t sampleRate;
    uint32_t avgByteRate;
    uint32_t blockAlignment;
    uint32_t bitsPerSample;
    uint32_t codecDataSize;
    uint8_t *codecData;
    AsfAudioStreamInfo *next;
};

struct AsfVideoStreamInfo {
    uint8_t streamNumber;
    uint8_t encryptedContentFlag;
    uint64_t timeOffset; // in 100-nanosecond units
    uint32_t width;
    uint32_t height;
    uint32_t fourCC;
    uint32_t aspectX;
    uint32_t aspectY;
    uint32_t codecDataSize;
    uint8_t *codecData;
    AsfVideoStreamInfo *next;
};

struct AsfFileMediaInfo {
    uint64_t fileSize;
    uint64_t dataPacketsCount;
    uint64_t duration; 	// 100-nanosecond units
    uint64_t preroll;     // in millisecond units.
    uint32_t packetSize;
    bool seekable;
};

struct AsfSimpleIndexInfo {
    uint8_t *indexData;
    uint32_t indexSize;
    uint64_t indexEntryTimeInterval;  // in 100-nanosecond unit
    uint32_t maximumPacketCount;
    uint32_t indexEntriesCount;
};

struct AsfPayloadDataInfo {
    const uint8_t *payloadData;
    uint32_t payloadSize;
    uint8_t  replicatedDataLength;
    uint8_t  *replicatedData;
    uint8_t  *sampleID; // Sample Id is always 8 bytes to be used by playready
    uint32_t sampleIDLen;
    uint32_t presentationTime; // in milliseconds
    uint32_t offsetIntoMediaObject;
    uint32_t mediaObjectLength;
    uint8_t streamNumber;
    uint8_t mediaObjectNumber;
    bool keyframe;
    AsfPayloadDataInfo *next;
};

#endif  // ASF_PARSER_DEFS_H_

