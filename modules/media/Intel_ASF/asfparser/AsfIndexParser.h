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




#ifndef ASF_INDEX_PARSER_H_
#define ASF_INDEX_PARSER_H_

#include "AsfParserDefs.h"

class AsfSimpleIndexParser {
public:
    AsfSimpleIndexParser(void);
    ~AsfSimpleIndexParser(void);

public:
    AsfSimpleIndexInfo* getIndexInfo() const;

    // buffer must contain a complete simple index object
    int parse(uint8_t *buffer, uint32_t size);
    // seek to the closest previous or next sync packet. time stamp is in 100-nanosecond units
    int seek(uint64_t seekTime, bool nextSync, uint32_t& packetNumber, uint64_t& targetTime);
    // return maximum video packet count per object, readable when simple index object is parsed.
    // If simple index object is not parsed or is not available, 0 is returned
    uint32_t getMaximumPacketCount();

private:
    void resetIndexInfo();

private:
    enum {
        // 4 bytes of "packet number" plus 2 bytes of "packet count"
        INDEX_ENTRY_SIZE = 6,
    };
    AsfSimpleIndexInfo *mIndexInfo;
};

#endif

