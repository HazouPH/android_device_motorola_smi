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



#ifndef ASF_STREAM_PARSER_H_
#define ASF_STREAM_PARSER_H_

#include "AsfParserDefs.h"

class AsfStreamParser {
public:
    AsfStreamParser(void);
    ~AsfStreamParser(void);

public:
    static bool isSimpleIndexObject(uint8_t *guid);
    static bool isHeaderObject(uint8_t *guid);

    // buffer must contain a complete header object
    int parseHeaderObject(uint8_t *buffer, uint64_t size);
    AsfAudioStreamInfo* getAudioInfo() const;
    AsfVideoStreamInfo* getVideoInfo() const;
    AsfFileMediaInfo* getFileInfo() const;
    int getDrmUuid(uint8_t playreadyUuid[], uint16_t len);
    int getDrmHeaderXml(uint8_t* playreadyHeader, uint32_t* playreadyHeaderLen);

    // return duration in 100-nanosecond unit , readable when header object is parsed
    uint64_t getDuration();
    // return data packet size, readable when header object is parsed
    uint32_t getDataPacketSize();
    bool hasVideo();
    bool hasAudio();
    // buffer must contain a complete data object header
    int parseDataObjectHeader(uint8_t *buffer, uint32_t size);
    // buffer must contain a complete data packet and only a packet
    int parseDataPacket(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out);

    // caller must release AsfPayloadDataInfo using this method
    void releasePayloadDataInfo(AsfPayloadDataInfo *info);

    // buffer must contain a complete simple index object
    int parseSimpleIndexObject(uint8_t *buffer, uint32_t size);

    AsfSimpleIndexInfo* getIndexInfo() const;

    // seek to the closest previous or next sync packet. time stamp is in 100-nanosecond units
    int seek(uint64_t seekTime, bool nextSync, uint32_t& packetNumber, uint64_t& targetTime);

    // return maximum video object size, readable when simple index object is parsed.
    // If simple index object is not parsed or  is not available, 0 is returned
    uint32_t getMaxObjectSize();

private:
    // fixed data packet size
    uint32_t mDataPacketSize;
    // offset of PTS in milliseconds (converted from original 100-nanoseconds unit) due to cut/edit.
    // all stream properties must have same "Time Offset".
    // If value is zero, it will be set to "Preroll"  value in the File Properties object.
    // Preroll value is used for data buffering.
    uint32_t mTimeOffsetMs;
    bool mHeaderParsed;
    class AsfHeaderParser *mHeaderParser;
    class AsfDataParser *mDataParser;
    class AsfSimpleIndexParser *mSimpleIndexParser;
};

#endif

