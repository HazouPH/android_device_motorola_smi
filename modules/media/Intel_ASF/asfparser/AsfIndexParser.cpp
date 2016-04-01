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



#include "AsfIndexParser.h"
#include "AsfObjects.h"
#include <string.h>


AsfSimpleIndexParser::AsfSimpleIndexParser(void)
    : mIndexInfo(NULL) {
}

AsfSimpleIndexParser::~AsfSimpleIndexParser(void) {
    resetIndexInfo();
}

AsfSimpleIndexInfo* AsfSimpleIndexParser::getIndexInfo() const {
    return mIndexInfo;
}

int AsfSimpleIndexParser::parse(uint8_t *buffer, uint32_t size) {
    // reset parser status
    resetIndexInfo();

    if (size <= sizeof(AsfSimpleIndexObject)) {
        return ASF_PARSER_BAD_DATA;
    }
    AsfSimpleIndexObject *obj = (AsfSimpleIndexObject*)buffer;
    if (obj->objectSize != size) {
        return ASF_PARSER_BAD_VALUE;
    }

    if (obj->indexEntryTimeInterval == 0) {
       return ASF_PARSER_BAD_VALUE;
    }

    mIndexInfo = new AsfSimpleIndexInfo;
    if (mIndexInfo == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }

    mIndexInfo->indexSize = size - sizeof(AsfSimpleIndexObject);
    mIndexInfo->indexData = new uint8_t [mIndexInfo->indexSize];

    if (mIndexInfo->indexData == NULL) {
        delete mIndexInfo;
        return ASF_PARSER_NO_MEMORY;
    }

    memcpy(mIndexInfo->indexData,
        buffer + sizeof(AsfSimpleIndexObject),
        size - sizeof(AsfSimpleIndexObject));

    mIndexInfo->indexEntryTimeInterval = obj->indexEntryTimeInterval;
    mIndexInfo->maximumPacketCount = obj->maximumPacketCount;
    mIndexInfo->indexEntriesCount = obj->indexEntriesCount;
    return ASF_PARSER_SUCCESS;
}


int AsfSimpleIndexParser::seek(
        uint64_t seekTime,
        bool nextSync,
        uint32_t& packetNumber,
        uint64_t& targetTime) {
    if (mIndexInfo == NULL) {
         return ASF_PARSER_INVALID_STATE;
    }

    // calculate offset of index entry in 6-byte unit
    uint32_t offset;
    if (nextSync) {
        offset = (seekTime + mIndexInfo->indexEntryTimeInterval - 1)/mIndexInfo->indexEntryTimeInterval;
    } else {
        offset = seekTime/mIndexInfo->indexEntryTimeInterval;
    }

    if (offset >= mIndexInfo->indexEntriesCount) {
        offset = mIndexInfo->indexEntriesCount;
    }

    if (INDEX_ENTRY_SIZE * offset > mIndexInfo->indexSize - INDEX_ENTRY_SIZE) {
        offset = mIndexInfo->indexSize/INDEX_ENTRY_SIZE - 1;
    }

    targetTime = offset * mIndexInfo->indexEntryTimeInterval;
    uint8_t *data = mIndexInfo->indexData + INDEX_ENTRY_SIZE * offset;
    // packet number 4 bytes
    // packet count 2 bytes
    packetNumber = *(uint32_t*)data;

    return ASF_PARSER_SUCCESS;
}

uint32_t AsfSimpleIndexParser::getMaximumPacketCount() {
    if (mIndexInfo == NULL)
        return 0;

    return mIndexInfo->maximumPacketCount;
}

void AsfSimpleIndexParser::resetIndexInfo() {
    if (mIndexInfo) {
        if (mIndexInfo->indexData) {
            delete [] mIndexInfo->indexData;
        }
        delete mIndexInfo;
    }

    mIndexInfo = NULL;
}

