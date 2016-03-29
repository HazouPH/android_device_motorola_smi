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



//#define LOG_NDEBUG 0
#define LOG_TAG "AsfDataParser"
#include "AsfDataParser.h"
#include "AsfGuids.h"
#include "AsfObjects.h"
#include <string.h>
#include <wrs_omxil_core/log.h>
#include "AsfHeaderParser.h"

AsfHeaderParser *AsfDataParser::mHeaderParser = NULL;

using namespace std;
// Helper fucctions

static inline uint8_t lengthType2Bytes(uint8_t lengthType) {
    // lengthType: 0    1   2   3
    // bits:            0    8  16  32
    // bytes:         0    1   2   4
    return 4 >> (3 - (lengthType & 0x03));
}

static inline uint32_t getModuleValue(uint32_t value, uint8_t lengthType) {
    switch (lengthType) {
        case 0:
            return 0; // field does not exist
        case 1:
            return value % 0x100; // (BYTE)
        case 2:
            return value % 0x10000;  // (WORD)
        case 3:
            return value; //(DWORD)
    }
    return value;
}

static inline uint32_t getFieldValue(uint8_t *buffer, uint8_t lengthType) {
    switch (lengthType) {
        case 0:
            return 0; // field does not exist
        case 1:
            return *buffer;
        case 2:
            return *(uint16_t*)buffer;
        case 3:
            return *(uint32_t*)buffer;
    }
    // This line should not be reached
    return 0xffffffff ;
 }

static void freePayloadDataInfo(AsfPayloadDataInfo *header) {
    while (header) {
        AsfPayloadDataInfo *next = header->next;
        delete header;
        header = next;
    }
}
AsfPayloadDataInfoPool::AsfPayloadDataInfoPool()
    : mFirstDataInfo(NULL),
      mLastDataInfo(NULL) {
}

AsfPayloadDataInfoPool::~AsfPayloadDataInfoPool() {
    freePayloadDataInfo(mFirstDataInfo);
}

void AsfPayloadDataInfoPool::releasePayloadDataInfo(AsfPayloadDataInfo *info) {
    if (info == NULL) {
        return;
    }

    if (mFirstDataInfo == NULL) {
        mFirstDataInfo = info;
    } else {
        mLastDataInfo->next = info;
    }
    while (info->next != NULL) {
        info = info->next;
    }
    mLastDataInfo = info;
}

AsfPayloadDataInfo* AsfPayloadDataInfoPool::getPayloadDataInfo() {
    AsfPayloadDataInfo *entry;

    if (mFirstDataInfo == NULL) {
        entry =  new AsfPayloadDataInfo;
        if (entry == NULL) {
            return NULL;
        }
    } else {
        entry = mFirstDataInfo;
        mFirstDataInfo = mFirstDataInfo->next;
        if (mFirstDataInfo == NULL) {
            mLastDataInfo = NULL;
        }
    }
    memset(entry, 0, sizeof(AsfPayloadDataInfo));
    return entry;
}


int AsfErrorCorrectionData::parse(uint8_t *buffer, uint32_t size) {
    errorCorrectionFlags.value = *buffer;

    blockSize = 0;
    if (errorCorrectionFlags.bits.errorCorrectionPresent == 0) {
        return ASF_PARSER_SUCCESS;
    }

    blockSize = 1;
    // determine if Error Correction Data Length is valid
    if (errorCorrectionFlags.bits.errorCorrectionLengthType == 0) {
        // Error Correction Data Length is valid only if the value of the Error Correction Length Type is 00

        // Error Correction Data Length should be 0010
        // Opaque Data Present should be set to 0
        blockSize += errorCorrectionFlags.bits.errorCorrectionDataLength;
        return ASF_PARSER_SUCCESS;
    }

    // if Error Correction Length Type is different thant 00, Error Correction Data Length shall be zero.
    if (errorCorrectionFlags.bits.errorCorrectionDataLength == 0) {
        return ASF_PARSER_SUCCESS;
    }

    return ASF_PARSER_BAD_VALUE;
}


int AsfPayloadParsingInformation::parse(uint8_t *buffer, uint32_t size) {
    lengthTypeFlags.value = *buffer;
    propertyFlags.value = *(buffer  + 1);

    // lengthTypeFlags:
    // sequence type should be set to 00
    // packet length type should be set to 00 when creating content
    // propertyFlags:
    // replicated data length type should be set to 01 (BYTE)
    // offset into media object shall be set to 11 (DWORD)
    // media object number length type shall be set to 01 (BYTE)
    // stream number length type shalll be set to 01 (BYTE)

    blockSize = 2;
    packetLength = getFieldValue(buffer + blockSize, lengthTypeFlags.bits.packetLengthType);
    blockSize += lengthType2Bytes(lengthTypeFlags.bits.packetLengthType);

    sequence = getFieldValue(buffer + blockSize, lengthTypeFlags.bits.sequenceType);
    blockSize += lengthType2Bytes(lengthTypeFlags.bits.sequenceType);

    paddingLength = getFieldValue(buffer + blockSize, lengthTypeFlags.bits.paddingLengthType);
    blockSize += lengthType2Bytes(lengthTypeFlags.bits.paddingLengthType);

    sendTime = *(uint32_t*)(buffer + blockSize);
    blockSize += 4;

    duration = *(uint16_t*)(buffer + blockSize);
    blockSize += 2;

    return ASF_PARSER_SUCCESS;
}


int AsfSinglePayloadUncompressed::parse(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out) {
    // initialize output
    *out = NULL;
    streamNumber.value = *buffer;
    blockSize = 1;

    mediaObjectNumber = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.mediaObjectNumberLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.mediaObjectNumberLengthType);

    offsetIntoMediaObject = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.offsetIntoMediaObjectLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.offsetIntoMediaObjectLengthType);

    replicatedDataLength = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.replicatedDataLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.replicatedDataLengthType);

    if (replicatedDataLength == 1) {
        // compressed payload
        blockSize == 0;
        return ASF_PARSER_COMPRESSED_PAYLOAD;
    }

    if (replicatedDataLength == 0) {
        // TODO:
        return ASF_PARSER_UNEXPECTED_VALUE;
    }

    if (replicatedDataLength < 8) {
        return ASF_PARSER_BAD_VALUE;
    }

    AsfPayloadDataInfo *obj = pool->getPayloadDataInfo();
    if (obj == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }

    // point to replicated data into object's buffer. Yet to be interpreted.
    obj->replicatedDataLength = replicatedDataLength;
    obj->replicatedData = buffer + blockSize;

    // Replicated data, at least 8 bytes
    obj->mediaObjectLength = *(uint32_t*)(buffer + blockSize);
    obj->presentationTime = *(uint32_t*)(buffer + blockSize + 4);

    blockSize += replicatedDataLength;

    uint8_t streamNum = streamNumber.bits.streamNumber;

    // Extension Systems are required if replicatedDataLength is greater than 8.
    if (replicatedDataLength > 8) {
        // For protected content, get SampleID which is used as IV.
        int returnStatus = AsfDataParser::mHeaderParser->parseSampleIDFromReplicatedData(obj,streamNum);
        if (returnStatus == ASF_PARSER_UNEXPECTED_VALUE) {
            ALOGD("Only Encryption_Sample_ID extension system is supported");
        } else if (returnStatus != ASF_PARSER_SUCCESS) {
            return ASF_PARSER_FAILED;
        }
    }

    obj->payloadData = buffer + blockSize;

    // size = packet length - packet header length
    // payload size = size - payload header size (blockSize) - padding length
    obj->payloadSize = size - blockSize - ppi->paddingLength;
    if ((int)obj->payloadSize <= 0) {
        delete obj;
        return ASF_PARSER_BAD_VALUE;
    }
    obj->offsetIntoMediaObject = offsetIntoMediaObject;
    obj->streamNumber = streamNumber.bits.streamNumber;
    obj->mediaObjectNumber = mediaObjectNumber;
    obj->keyframe = streamNumber.bits.keyFrameBit;
    obj->next = NULL;

    // skip padding data
    blockSize += ppi->paddingLength;
    *out = obj;
    return ASF_PARSER_SUCCESS;
}


int AsfSinglePayloadCompressed::parse(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out) {
    // initialize output
    *out = NULL;
    streamNumber.value = *buffer;
    blockSize = 1;

    mediaObjectNumber = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.mediaObjectNumberLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.mediaObjectNumberLengthType);

    // presentation time is coded using the value of Offset Into Media Object Length Type
    presentationTime= getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.offsetIntoMediaObjectLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.offsetIntoMediaObjectLengthType);

    // must be 1
    replicatedDataLength = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.replicatedDataLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.replicatedDataLengthType);

    presentationTimeDelta = *(buffer + blockSize);
    blockSize++;

    int payloadLenRemaining = size - blockSize - ppi->paddingLength;
    if (payloadLenRemaining <= 0) {
        return ASF_PARSER_BAD_VALUE;
    }

    uint32_t pts = presentationTime;
    uint32_t objNumber = mediaObjectNumber;

    uint8_t subPayloadDataLength;
    AsfPayloadDataInfo *first = NULL, *next = NULL, *last = NULL;

    while (payloadLenRemaining > 0) {
        subPayloadDataLength = *(buffer + blockSize);
        blockSize++;
        payloadLenRemaining -= 1;

        next = pool->getPayloadDataInfo();
        if (next == NULL) {
            freePayloadDataInfo(first);
            return ASF_PARSER_NO_MEMORY;
        }

        next->payloadData = buffer + blockSize;
        next->payloadSize = subPayloadDataLength;
        next->presentationTime = pts;
        next->offsetIntoMediaObject = 0;
        next->mediaObjectLength = subPayloadDataLength;
        next->streamNumber = streamNumber.bits.streamNumber;
        next->mediaObjectNumber = getModuleValue(objNumber, ppi->propertyFlags.bits.mediaObjectNumberLengthType);
        next->keyframe = streamNumber.bits.keyFrameBit;
        next->next = NULL;

        if (first == NULL) {
            first = next;
            last = next;
        } else {
            last->next = next;
            last = next;
        }

        pts += presentationTimeDelta;
        objNumber++;
        blockSize += subPayloadDataLength;
        payloadLenRemaining -= subPayloadDataLength;
    }

    if (payloadLenRemaining != 0) {
        // TODO:
        freePayloadDataInfo(first);
        return ASF_PARSER_BAD_VALUE;
    }

    // skip padding data
    blockSize += ppi->paddingLength;
    *out = first;
    return ASF_PARSER_SUCCESS;
}


int AsfMultiplePayloadsHeader::parse(uint8_t *buffer, uint32_t size) {
    payloadFlags.value = *buffer;
    blockSize = 1;

    // number of payloads must not be 0
    if (payloadFlags.bits.numberOfPayloads == 0) {
        return ASF_PARSER_BAD_VALUE;
    }

    // payload length type should be set to 10 (WORD)
    return ASF_PARSER_SUCCESS;
}


int AsfMultiplePayloadsUncompressed::parse(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out) {
    // initialize output
    *out = NULL;
    streamNumber.value = *buffer;
    blockSize = 1;

    mediaObjectNumber = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.mediaObjectNumberLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.mediaObjectNumberLengthType);

    offsetIntoMediaObject = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.offsetIntoMediaObjectLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.offsetIntoMediaObjectLengthType);

    replicatedDataLength = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.replicatedDataLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.replicatedDataLengthType);

    if (replicatedDataLength == 1) {
        // compressed payload
        blockSize == 0;
        return ASF_PARSER_COMPRESSED_PAYLOAD;
    }

    if (replicatedDataLength == 0) {
        // TODO:
        return ASF_PARSER_UNEXPECTED_VALUE;
    }

    if (replicatedDataLength < 8) {
        return ASF_PARSER_BAD_VALUE;
    }

    AsfPayloadDataInfo *obj = pool->getPayloadDataInfo();
    if (obj == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }

    // point to replicated data into object's buffer. Yet to be interpreted.
    obj->replicatedDataLength = replicatedDataLength;
    obj->replicatedData = buffer + blockSize;

    // at least 8 bytes replicated data
    obj->mediaObjectLength = *(uint32_t *)(buffer + blockSize);
    obj->presentationTime = *(uint32_t *)(buffer + blockSize + 4);

    blockSize += replicatedDataLength;

    uint8_t streamNum = streamNumber.bits.streamNumber;

    // Extension Systems are required if replicatedDataLength is greater than 8.
    if (replicatedDataLength > 8) {
        // For protected content, get SampleID which is used as IV.
        int returnStatus = AsfDataParser::mHeaderParser->parseSampleIDFromReplicatedData(obj,streamNum);

        if (returnStatus == ASF_PARSER_UNEXPECTED_VALUE) {
            ALOGD("Only Encryption_Sample_ID extension system is supported");
        } else if (returnStatus != ASF_PARSER_SUCCESS) {
            return ASF_PARSER_FAILED;
        }
    }

    // payload length must not be 0
    payloadLength = getFieldValue(buffer + blockSize, mpHeader->payloadFlags.bits.payloadLengthType);
    blockSize += lengthType2Bytes(mpHeader->payloadFlags.bits.payloadLengthType);

    if (payloadLength == 0 || payloadLength + blockSize > size) {
        delete obj;
        return ASF_PARSER_BAD_VALUE;
    }

    obj->payloadData = buffer + blockSize;
    obj->payloadSize = payloadLength;

    obj->offsetIntoMediaObject = offsetIntoMediaObject;
    obj->streamNumber = streamNumber.bits.streamNumber;
    obj->mediaObjectNumber = mediaObjectNumber;
    obj->keyframe = streamNumber.bits.keyFrameBit;
    obj->next = NULL;

    // skip payload data
    blockSize += payloadLength;
    *out = obj;
    return ASF_PARSER_SUCCESS;
}


int AsfMultiplePayloadsCompressed::parse(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out) {
    // initialize output
    *out = NULL;
    streamNumber.value = *buffer;
    blockSize = 1;

    mediaObjectNumber = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.mediaObjectNumberLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.mediaObjectNumberLengthType);

    // presentation time is coded using the value of Offset Into Media Object Length Type
    presentationTime= getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.offsetIntoMediaObjectLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.offsetIntoMediaObjectLengthType);

    // must be 1
    replicatedDataLength = getFieldValue(buffer + blockSize, ppi->propertyFlags.bits.replicatedDataLengthType);
    blockSize += lengthType2Bytes(ppi->propertyFlags.bits.replicatedDataLengthType);

    presentationTimeDelta = *(buffer + blockSize);
    blockSize++;

    // payload length must not be 0
    payloadLength = getFieldValue(buffer + blockSize, mpHeader->payloadFlags.bits.payloadLengthType);
    blockSize += lengthType2Bytes(mpHeader->payloadFlags.bits.payloadLengthType);
    if (payloadLength == 0 || blockSize + payloadLength > size) {
        return ASF_PARSER_BAD_VALUE;
    }

    // safe to case from uint32_t to int.
    int payloadLenRemaining = (int)payloadLength;
    uint32_t pts = presentationTime;
    uint32_t objNumber = mediaObjectNumber;
    uint8_t subPayloadDataLength;
    AsfPayloadDataInfo *first = NULL, *next = NULL, *last = NULL;

    while (payloadLenRemaining > 0) {
        subPayloadDataLength = *(buffer + blockSize);
        blockSize++;
        payloadLenRemaining -= 1;

        next = pool->getPayloadDataInfo();
        if (next == NULL) {
            freePayloadDataInfo(first);
            return ASF_PARSER_NO_MEMORY;
        }

        next->payloadData = buffer + blockSize;
        next->payloadSize = subPayloadDataLength;
        next->presentationTime = pts;
        next->offsetIntoMediaObject = 0;
        next->mediaObjectLength = subPayloadDataLength;
        next->streamNumber = streamNumber.bits.streamNumber;
        next->mediaObjectNumber = getModuleValue(objNumber, ppi->propertyFlags.bits.mediaObjectNumberLengthType);
        next->keyframe = streamNumber.bits.keyFrameBit;
        next->next = NULL;

        if (first == NULL) {
            first = next;
            last = next;
        } else {
            last->next = next;
            last = next;
        }

        pts += presentationTimeDelta;
        objNumber++;
        blockSize += subPayloadDataLength;
        payloadLenRemaining -= subPayloadDataLength;
    }

    if (payloadLenRemaining < 0) {
        // TODO:
        freePayloadDataInfo(first);
        return ASF_PARSER_BAD_VALUE;
    }

    // blockSize stays as it is
    *out = first;
    return ASF_PARSER_SUCCESS;
}


AsfDataParser::AsfDataParser(AsfHeaderParser *hdrparser)
    : mTotalDataPackets(0) {
    mSPUncompressed.ppi = &mPPI;
    mSPCompressed.ppi = &mPPI;
    mMPHeader.ppi = &mPPI;
    mMPUncompressed.ppi = &mPPI;
    mMPCompressed.ppi = &mPPI;
    mMPUncompressed.mpHeader = &mMPHeader;
    mMPCompressed.mpHeader = &mMPHeader;

    mSPUncompressed.pool = &mPool;
    mSPCompressed.pool = &mPool;
    mMPUncompressed.pool = &mPool;
    mMPCompressed.pool = &mPool;
    if (hdrparser) {
        AsfDataParser::mHeaderParser = hdrparser;
    }
}


AsfDataParser::~AsfDataParser(void) {
}

int AsfDataParser::parseHeader(uint8_t *buffer, uint32_t size) {
    if (size < sizeof(AsfDataObject)) {
        return ASF_PARSER_BAD_DATA;
    }
    AsfDataObject *obj = (AsfDataObject*)buffer;
    mTotalDataPackets = obj->totalDataPackets;
    return ASF_PARSER_SUCCESS;
}

uint64_t AsfDataParser::getTotalDataPackets() {
    return mTotalDataPackets;
}

int AsfDataParser::parsePacket(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out) {
    int status;
    AsfPayloadDataInfo *first = NULL;

    status = mECD.parse(buffer, size);
    if (status != ASF_PARSER_SUCCESS) {
        return status;
    }

    buffer += mECD.blockSize;
    size -= mECD.blockSize;
    status = mPPI.parse(buffer, size);
    if (status != ASF_PARSER_SUCCESS) {
        return status;
    }

    buffer += mPPI.blockSize;
    size -= mPPI.blockSize;

    if (mPPI.lengthTypeFlags.bits.multiplePayloadsPresent) {
        status = mMPHeader.parse(buffer, size);
        if (status != ASF_PARSER_SUCCESS) {
            return status;
        }
        buffer += mMPHeader.blockSize;
        size -= mMPHeader.blockSize;

        AsfPayloadDataInfo *last = NULL, *next = NULL;
        for (int i = 0; i < mMPHeader.payloadFlags.bits.numberOfPayloads; i++) {
            status = mMPUncompressed.parse(buffer, size, &next);

            if (status == ASF_PARSER_SUCCESS) {
                buffer += mMPUncompressed.blockSize;
                size -= mMPUncompressed.blockSize;
            } else if (status == ASF_PARSER_COMPRESSED_PAYLOAD) {
                status = mMPCompressed.parse(buffer, size, &next);
                if (status != ASF_PARSER_SUCCESS) {
                    break;
                }
                buffer += mMPCompressed.blockSize;
                size -= mMPCompressed.blockSize;
            }
            else {
                break;
            }

            if ((int)size < 0) {
                status = ASF_PARSER_BAD_VALUE;
                break;
            }
            // concatenate the payloads.
            if (first == NULL) {
                first = next;
                last = next;
            }
            else {
                while (last->next != NULL) {
                    last = last->next;
                }
                last->next = next;
                last = next;
            }
        }
    }
    else {
        status = mSPUncompressed.parse(buffer, size, &first);

         if (status == ASF_PARSER_COMPRESSED_PAYLOAD) {
            status = mSPCompressed.parse(buffer, size, &first);
        }
    }

    if (status != ASF_PARSER_SUCCESS) {
        freePayloadDataInfo(first);
        return status;
    }

    *out = first;
    return ASF_PARSER_SUCCESS;
}

void AsfDataParser::releasePayloadDataInfo(AsfPayloadDataInfo *info) {
    mPool.releasePayloadDataInfo(info);
}
