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




#ifndef ASF_HEADER_PARSER_H_
#define ASF_HEADER_PARSER_H_

#include "AsfParserDefs.h"
#include "AsfObjects.h"
#include "AsfGuids.h"
#include <vector>

using namespace std;

class AsfHeaderParser {
public:
    AsfHeaderParser(void);
    ~AsfHeaderParser(void);

public:
    // buffer must contain a complete header object
    int parse(uint8_t* buffer, uint64_t size);

    AsfAudioStreamInfo* getAudioInfo() const;
    AsfVideoStreamInfo* getVideoInfo() const;
    AsfFileMediaInfo* getFileInfo() const;
    // return duration in 100-nanosecond unit , readable when header object is parsed
    uint64_t getDuration();
    // return data packet size, readable when header object is parsed
    uint32_t getDataPacketSize();
    // return preroll in milliseconds
    uint32_t getPreroll();
    // return Time Offset in any stream properties in 100-nanosecond unit
    // Time Offset must be equal for all the stream properties object
    uint64_t getTimeOffset();
    bool hasVideo();
    bool hasAudio();
    bool isSeekable();
    int getPlayreadyUuid(uint8_t playreadyUuid[], uint16_t len);
    int getPlayreadyHeaderXml(uint8_t* header, uint32_t* len);
    int parseSampleIDFromReplicatedData(AsfPayloadDataInfo *obj, uint8_t streamNumber);
private:
    int onFilePropertiesObject(uint8_t *buffer, uint32_t size);
    int onStreamPropertiesObject(uint8_t *buffer, uint32_t size);
    int onVideoSpecificData(AsfStreamPropertiesObject *obj, uint8_t *data);
    int onAudioSpecificData(AsfStreamPropertiesObject *obj, uint8_t *data);
    int onExtendedStreamPropertiesObject(uint8_t *buffer, uint32_t size);
    int parseHeaderExtensionObject(uint8_t *buffer, uint32_t size);
    void resetStreamInfo();

private:
    AsfAudioStreamInfo *mAudioInfo;
    AsfVideoStreamInfo *mVideoInfo;
    AsfFileMediaInfo  *mFileInfo;
    uint32_t mNumObjectParsed;
    uint8_t mPlayreadyUuid[UUIDSIZE];
    bool mIsProtected;
    uint8_t *mPlayreadyHeader;
    uint16_t mPlayreadyHeaderLen;
    uint32_t mNumberofHeaderObjects;
    vector<AsfExtendedStreamPropertiesExObject *> mExtendedStreamPropertiesObj;
};

#endif

