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




#ifndef ASF_OBJECTS_H_
#define ASF_OBJECTS_H_

#include "AsfParserDefs.h"
#include "AsfGuids.h"
#include <vector>

#define UUIDSIZE 16

#pragma pack(push, 1)

using namespace std;
struct AsfObject {
    GUID objectID;
    uint64_t objectSize;
};

struct AsfHeaderObject : AsfObject {
    uint32_t numberofHeaderObjects;
    uint8_t reserved1;
    uint8_t reserved2;
};

struct AsfProtectionSystemIdObj : AsfObject {
    uint8_t sysId[UUIDSIZE];
    uint32_t sysVer;
    uint32_t dataSize;
    uint32_t lenRecords;
    uint16_t countRecords;
    // records
};

struct AsfFilePropertiesObject : AsfObject {
    GUID fileID;
    uint64_t fileSize;
    uint64_t creationDate;
    uint64_t dataPacketsCount;
    uint64_t playDuration;
    uint64_t sendDuration;
    uint64_t preroll;
    union {
        struct {
            uint32_t broadcastFlag :1;
            uint32_t seekableFlag :1;
            uint32_t reserved :30;
        } bits;
        uint32_t value;
    } flags;
    uint32_t minimumDataPacketSize;
    uint32_t maximumDataPacketSize;
    uint32_t maximumBitrate;
};

struct AsfStreamPropertiesObject : AsfObject {
    GUID streamType;
    GUID errorCorrectionType;
    uint64_t timeOffset;
    uint32_t typeSpecificDataLength;
    uint32_t errorCorrectionDataLength;
    union {
        struct {
            uint16_t streamNumber :7;
            uint16_t reserved :8;
            uint16_t encryptedContentFlag :1;
        } bits;
        uint16_t value;
    } flags;
    uint32_t reserved;
    //type-Specific Data;
    //error Correction Data;
};

struct AsfHeaderExtensionObject : AsfObject {
    GUID clockType;  // Reserved Field 1
    uint16_t clockSize; // Reserved Field 2
    uint32_t headerExtensionDataSize;
    //header Extension Data;
};


struct AsfCodecListObject  : AsfObject {
    // TODO:
};

struct AsfScriptCommandObject : AsfObject {
    // TODO:
};

struct AsfMarkerObject : AsfObject {
    // TODO:
};

struct AsfBitrateMutualExclusionObject : AsfObject {
    // TODO:
};

struct AsfErrorCorrectionObject : AsfObject {
    // TODO:
};

struct AsfContentDescriptionObject : AsfObject {
    // TODO:
};

struct AsfExtendedContentDescriptionObject : AsfObject {
    // TODO:
};

struct AsfStreamBitratePropertiesObject : AsfObject {
    // TODO:
};

struct AsfContentBrandingObject : AsfObject {
    // TODO:
};

struct AsfContentEncryptionObject : AsfObject {
    // TODO:
};

struct AsfExtendedContentEncryptionObject : AsfObject {
    // TODO:
};

struct AsfDigitalSignatureObject : AsfObject {
    // TODO:
};

struct AsfPaddingObject : AsfObject {
    // TODO:
};

struct AsfStreamName {
    uint16_t  languageIDIndex;
    uint16_t  streamNameLength;
    uint8_t  *pStreamName;
};



// objects in the ASF Header Extension object
struct AsfPayloadExtensionSystem {
    GUID      extensionSystemId;
    uint16_t  extensionDataSize;
    uint32_t  extensionSystemInfoLength;
    uint8_t  *extensionSystemInfo;
};

// class AsfHeaderParser;
// Fixed Length fields of AsfExtendedStreamPropertiesObject
struct AsfExtendedStreamPropertiesObject : AsfObject {
    uint64_t startTime;
    uint64_t endTime;
    uint32_t dataBitrate;
    uint32_t bufferSize;
    uint32_t initialBufferFullness;
    uint32_t alternateDataBitrate;
    uint32_t alternateBufferSize;
    uint32_t alternateInitialBufferFullness;
    uint32_t maximumObjectSize;
    union {
        struct {
            uint32_t reliableFlag :1;
            uint32_t seekableFlag :1;
            uint32_t noCleanpointsFlag :1;
            uint32_t resendLiveCleanpointsFlag :1;
            uint32_t reservedFlags :28;
        } bits;
        uint32_t value;
    } flags;
    uint16_t streamNumber;
    uint16_t streamLanguageIDIndex;
    uint64_t averageTimePerFrame;
    uint16_t streamNameCount;
    uint16_t payloadExtensionSystemCount;
    //Stream Names - variable length
    //Payload Extension Systems - variable length
    //Stream Properties Object - variable length
};

// AsfExtendedStreamPropertiesObjectFixed + variable length extension systems.
struct AsfExtendedStreamPropertiesExObject {
    struct AsfExtendedStreamPropertiesObject  propObj;
    vector<AsfStreamName *> streamNames;
    vector<AsfPayloadExtensionSystem *> extensionSystems;
    //Stream Properties Object - variable length
};



struct AsfAdvancedMutualExclusionObject : AsfObject {
    // TODO:
};

struct AsfGroupMutualExclusionObject : AsfObject {
    // TODO:
};

struct AsfStreamPrioritizationObject : AsfObject {
    // TODO:
};

struct AsfBandwidthSharingObject : AsfObject {
    // TODO:
};

struct AsfLanguageListObject : AsfObject {
    // TODO:
};

struct AsfMetadataObject : AsfObject {
    // TODO:
};

struct AsfMetadataLibraryObject : AsfObject {
    // TODO:
};

struct AsfIndexParametersObject : AsfObject {
    // TODO:
};

struct AsfMediaObjectIndexParametersObject : AsfObject {
    // TODO:
};

struct AsfTimeCodeIndexParametersObject : AsfObject {
    // TODO:
};

struct AsfCompatibilityObject : AsfObject {
    // TODO:
};

struct AsfAdvancedContentEncryptionObject : AsfObject {
    // TODO:
};


// ASF top-level data object

struct AsfDataObject : AsfObject {
    GUID fileID;
    uint64_t totalDataPackets;
    uint16_t reserved;
    //Data Packets;
};


// ASF top-level index objects

struct AsfSimpleIndexObject : AsfObject {
    GUID fileID;
    // in 100-nanosecond units
    uint64_t indexEntryTimeInterval;
    uint32_t maximumPacketCount;
    uint32_t indexEntriesCount;
    //packet number for entry #0  (4 bytes)
    //packet count for entry #0 (2 bytes)
    //packet number for entry #1
    //packet count for entry #1
};

struct AsfIndexObject : AsfObject {
    // TODO:
};

struct AsfMediaObjectIndexObject : AsfObject {
    // TODO:
};


struct AsfTimecodeIndexObject : AsfObject {
    // TODO:
};


// media specific data structure

struct AsfWaveFormatEx {
    uint16_t codecIDFormatTag;
    uint16_t numberOfChannels;
    uint32_t samplesPerSecond;
    uint32_t averageNumberOfBytesPerSecond;
    uint16_t blockAlignment;
    uint16_t bitsPerSample;
    uint16_t codecSpecificDataSize;
    //uint8_t codecSpecificData[];
};

struct AsfVideoInfoHeader {
    uint32_t encodedImageWidth;
    uint32_t encodedImageHeight;
    uint8_t reservedFlags;
    uint16_t formatDataSize;
    //FormatData formatData[];
};

struct AsfBitmapInfoHeader {
    uint32_t formatDataSize;
    int32_t imageWidth;
    int32_t imageHeight;
    uint16_t reserved;
    uint16_t bitsPerPixelCount;
    uint32_t compressionID;
    uint32_t imageSize;
    int32_t horizontalPixelsPerMeter;
    int32_t verticalPixelsPerMeter;
    uint32_t colorsUsedCount;
    uint32_t importantColorsCount;
    //uint8_t codecSpecificData[];
};

#pragma pack(pop)

#endif




