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




#ifndef ASF_DATA_PARSER_H_
#define ASF_DATA_PARSER_H_

#include "AsfParserDefs.h"

class AsfPayloadDataInfoPool {
public:
    AsfPayloadDataInfoPool();
    ~AsfPayloadDataInfoPool();

    // put payload data info to internal queue for reuse.
    void releasePayloadDataInfo(AsfPayloadDataInfo *info);
    inline AsfPayloadDataInfo* getPayloadDataInfo();

private:
    AsfPayloadDataInfo *mFirstDataInfo;
    AsfPayloadDataInfo *mLastDataInfo;
};


struct AsfErrorCorrectionData {
    int parse(uint8_t *buffer, uint32_t size);

    union {
        struct {
            uint8_t errorCorrectionDataLength :4;
            uint8_t opaqueDataPresent :1;
            uint8_t errorCorrectionLengthType :2;
            uint8_t errorCorrectionPresent :1;
        } bits;
        uint8_t value;
    } errorCorrectionFlags;

    // size of this data block, 0 if Error Correction Present is 0
    uint32_t blockSize;
};

struct AsfPayloadParsingInformation {
    int parse(uint8_t *buffer, uint32_t size);

    union {
        struct {
            uint8_t multiplePayloadsPresent :1;
            uint8_t sequenceType :2;
            uint8_t paddingLengthType :2;
            uint8_t packetLengthType :2;
            uint8_t errorCorrectionPresent :1;
        } bits;
        uint8_t value;
    } lengthTypeFlags;

    union {
        struct {
            uint8_t replicatedDataLengthType :2;
            uint8_t offsetIntoMediaObjectLengthType :2;
            uint8_t mediaObjectNumberLengthType :2;
            uint8_t streamNumberLengthType :2;
        } bits;
        uint8_t value;
    } propertyFlags;


    uint32_t packetLength;  // Varialbe length: 0, 8, 16, 32
    uint32_t sequence;      // Variable length: 0, 8, 16, 32
    uint32_t paddingLength; // Varialbe length: 0, 8, 16, 32
    uint32_t sendTime;
    uint16_t duration;

    // size of this data block
    uint32_t blockSize;
};


struct AsfSinglePayloadUncompressed {
    int parse(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out);

    union {
        struct {
            uint8_t streamNumber :7;
            uint8_t keyFrameBit :1;
        } bits;
        uint8_t value;
    } streamNumber;

    uint32_t mediaObjectNumber;       // Varialbe length: 0, 8, 16, 32
    uint32_t offsetIntoMediaObject;   // Varialbe length: 0, 8, 16, 32
    uint32_t replicatedDataLength;    // Varialbe length: 0, 8, 16, 32
    //BYTE replicatedData[];
    //BYTE payloadData[];

    // size of this data block including padding data
    uint32_t blockSize;
    AsfPayloadParsingInformation *ppi;
    AsfPayloadDataInfoPool *pool;
};


struct AsfSinglePayloadCompressed {
    int parse(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out);

    union {
        struct {
            uint8_t streamNumber :7;
            uint8_t keyFrameBit :1;
        } bits;
        uint8_t value;
    } streamNumber;

    uint32_t mediaObjectNumber;       // Varialbe length: 0, 8, 16, 32
    uint32_t presentationTime;        // Varialbe length: 0, 8, 16, 32
    uint32_t replicatedDataLength;    // Varialbe length: 0, 8, 16, 32
    uint8_t presentationTimeDelta;
    //BYTE subPayload #0 data length
    //BYTE subPayload #0 data
    //Byte subPayload #1 data length
    //Byte subPayload #1 data

    // size of this block including padding data
    uint32_t blockSize;
    AsfPayloadParsingInformation *ppi;
    AsfPayloadDataInfoPool *pool;
};


struct AsfMultiplePayloadsHeader {
    int parse(uint8_t *buffer, uint32_t size);

    union {
        struct {
            uint8_t numberOfPayloads :6;
            uint8_t payloadLengthType :2;
        } bits;
        uint8_t value;
    } payloadFlags;

    // BYTES payloads[];

    // size of this header block, must be 1
    uint32_t blockSize;
    AsfPayloadParsingInformation *ppi;
};


struct AsfMultiplePayloadsUncompressed {
    int parse(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out);

    union {
         struct {
             uint8_t streamNumber :7;
             uint8_t keyFrameBit :1;
         } bits;
         uint8_t value;
     } streamNumber;

     uint32_t mediaObjectNumber;       // Varialbe length: 0, 8, 16, 32
     uint32_t offsetIntoMediaObject;   // Varialbe length: 0, 8, 16, 32
     uint32_t replicatedDataLength;    // Varialbe length: 0, 8, 16, 32
     //BYTE replicatedData[];
     uint32_t payloadLength;           // Varialbe length: 8, 16, 32
     //BYTE payloadData[];

     // size of this single uncompressed payload block in the multiple payloads packet
     uint32_t blockSize;
     AsfPayloadParsingInformation *ppi;
     AsfMultiplePayloadsHeader *mpHeader;
     AsfPayloadDataInfoPool *pool;
};


struct AsfMultiplePayloadsCompressed {
    int parse(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out);

    union {
        struct {
            uint8_t streamNumber :7;
            uint8_t keyFrameBit :1;
        } bits;
        uint8_t value;
    } streamNumber;
    uint32_t mediaObjectNumber;       // Varialbe length: 0, 8, 16, 32
    uint32_t presentationTime;        // Varialbe length: 0, 8, 16, 32
    uint32_t replicatedDataLength;    // Varialbe length: 0, 8, 16, 32
    uint8_t presentationTimeDelta;
    uint32_t payloadLength;           // Varialbe length: 8, 16, 32
    //BYTE subPayload #0 data length
    //BYTE subPayload #0 data
    //Byte subPayload #1 data length
    //Byte subPayload #1 data

    // size of this single compressed payload block in the multiple payloads packet.
    uint32_t blockSize;
    AsfPayloadParsingInformation *ppi;
    AsfMultiplePayloadsHeader *mpHeader;
    AsfPayloadDataInfoPool *pool;
};

class AsfDataParser {
public:
    AsfDataParser(class AsfHeaderParser *hdrparser);
    ~AsfDataParser(void);

public:
    int parseHeader(uint8_t *buffer, uint32_t size);

    uint64_t getTotalDataPackets();
    // buffer must contain a complete data packet and only one packet
    int parsePacket(uint8_t *buffer, uint32_t size, AsfPayloadDataInfo **out);
    // put payload data info to internal queue for reuse.
    void releasePayloadDataInfo(AsfPayloadDataInfo *info);
    static class AsfHeaderParser *mHeaderParser;

private:
    inline AsfPayloadDataInfo* getPayloadDataInfo();

private:
    uint64_t mTotalDataPackets;
    AsfErrorCorrectionData mECD;
    AsfPayloadParsingInformation mPPI;
    AsfSinglePayloadUncompressed mSPUncompressed;
    AsfSinglePayloadCompressed   mSPCompressed;
    AsfMultiplePayloadsHeader mMPHeader;
    AsfMultiplePayloadsUncompressed mMPUncompressed;
    AsfMultiplePayloadsCompressed mMPCompressed;
    AsfPayloadDataInfoPool mPool;
};

#endif

