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
#define LOG_TAG "AsfHeaderParser"
#include <wrs_omxil_core/log.h>

#include "AsfHeaderParser.h"
#include <string.h>

#include <media/stagefright/Utils.h>

#define PRFORMATTAG 0x5052
AsfHeaderParser::AsfHeaderParser(void)
    : mAudioInfo(NULL),
      mVideoInfo(NULL),
      mFileInfo(NULL),
      mNumObjectParsed(0),
      mIsProtected(false),
      mPlayreadyHeader(NULL),
      mPlayreadyHeaderLen(0),
      mNumberofHeaderObjects(0) {
    mFileInfo = new AsfFileMediaInfo;
    memset(mFileInfo, 0, sizeof(AsfFileMediaInfo));
}

AsfHeaderParser::~AsfHeaderParser(void) {
    delete mFileInfo;
    if (mPlayreadyHeader) {
        delete mPlayreadyHeader;
        mPlayreadyHeader = NULL;
    }

    // Deleting memory from mExtendedStreamPropertiesObj recursively
    for (vector<AsfExtendedStreamPropertiesExObject *>::iterator it = mExtendedStreamPropertiesObj.begin(); it != mExtendedStreamPropertiesObj.end(); ++it) {

        for (int i = 0; i < (*it)->streamNames.size(); i++) {
            if ((*it)->streamNames[i]->pStreamName != NULL) {
                delete (*it)->streamNames[i]->pStreamName;
            }
            delete (*it)->streamNames[i];
        }
        (*it)->streamNames.clear();

        for (int i = 0; i < (*it)->extensionSystems.size(); i++) {
            if ((*it)->extensionSystems[i]->extensionSystemInfo != NULL) {
                delete (*it)->extensionSystems[i]->extensionSystemInfo;
            }
            delete (*it)->extensionSystems[i];
        }
        (*it)->extensionSystems.clear();
        delete (*it);
    }
    mExtendedStreamPropertiesObj.clear();

    resetStreamInfo();
}

AsfAudioStreamInfo* AsfHeaderParser::getAudioInfo() const {
    return mAudioInfo;
}

AsfVideoStreamInfo* AsfHeaderParser::getVideoInfo() const {
    return mVideoInfo;
}

AsfFileMediaInfo* AsfHeaderParser::getFileInfo() const {
    return mFileInfo;
}

uint64_t AsfHeaderParser::getDuration() {
    return mFileInfo->duration - mFileInfo->preroll * ASF_SCALE_MS_TO_100NANOSEC;
}

uint32_t AsfHeaderParser::getDataPacketSize() {
    return mFileInfo->packetSize;
}

uint32_t AsfHeaderParser::getPreroll() {
    // in millisecond unit
    return mFileInfo->preroll;
}

uint64_t AsfHeaderParser::getTimeOffset() {
    // in 100-nanoseconds unit
    if (mAudioInfo) {
        return mAudioInfo->timeOffset;
    }

    if (mVideoInfo) {
        return mVideoInfo->timeOffset;
    }

    return 0;
}

bool AsfHeaderParser::hasVideo() {
    return mVideoInfo != NULL;
}

bool AsfHeaderParser::hasAudio() {
    return mAudioInfo != NULL;
}

bool AsfHeaderParser::isSeekable() {
    return mFileInfo->seekable;
}

int AsfHeaderParser::parse(uint8_t *buffer, uint64_t size) {
    int status = ASF_PARSER_SUCCESS;

    // reset parser's status
    mNumObjectParsed = 0;
    resetStreamInfo();
    memset(mFileInfo, 0, sizeof(AsfFileMediaInfo));

    do {
        if (size < sizeof(AsfObject)) {
            return ASF_PARSER_BAD_DATA;
        }

        AsfObject *obj = (AsfObject*)buffer;
        if (obj->objectSize > size) {
            return ASF_PARSER_BAD_VALUE;
        }

        if (obj->objectID == ASF_Header_Object) {
            if (size < sizeof(AsfHeaderObject)) {
                return ASF_PARSER_BAD_DATA;
            }
            AsfHeaderObject *headerObj = (AsfHeaderObject*)buffer;
            mNumberofHeaderObjects = headerObj->numberofHeaderObjects;
            size -= sizeof(AsfHeaderObject);
            buffer += sizeof(AsfHeaderObject);
        } else {
            if(obj->objectID == ASF_File_Properties_Object) {
                status = onFilePropertiesObject(buffer, size);
            } else if(obj->objectID == ASF_Stream_Properties_Object) {
                status = onStreamPropertiesObject(buffer, size);
            } else if(obj->objectID == ASF_Header_Extension_Object) {
                //AsfHeaderExtensionObject *headerExtObj = (AsfHeaderExtensionObject*)buffer;
                if (size < sizeof(AsfHeaderExtensionObject)) {
                    return ASF_PARSER_BAD_DATA;
                }
                status = parseHeaderExtensionObject(
                        buffer + sizeof(AsfHeaderExtensionObject),
                        size - sizeof(AsfHeaderExtensionObject));
            } else if(obj->objectID == ASF_Codec_List_Object) {
            } else if(obj->objectID == ASF_Script_Command_Object) {
            } else if(obj->objectID == ASF_Marker_Object) {
            } else if(obj->objectID == ASF_Bitrate_Mutual_Exclusion_Object) {
            } else if(obj->objectID == ASF_Error_Correction_Object) {
            } else if(obj->objectID == ASF_Content_Description_Object) {
            } else if(obj->objectID == ASF_Extended_Content_Description_Object) {
            } else if(obj->objectID == ASF_Stream_Bitrate_Properties_Object) {
            } else if(obj->objectID == ASF_Content_Branding_Object) {
            } else if(obj->objectID == ASF_Content_Encryption_Object) {
            } else if(obj->objectID == ASF_Extended_Content_Encryption_Object) {
            } else if(obj->objectID == ASF_Digital_Signature_Object) {
            } else if(obj->objectID == ASF_Padding_Object) {
            } else if(obj->objectID == ASF_Protection_System_Identifier_Object) {
                mIsProtected = true;
                LOGV("ASF_Protection_System_Identifier_Object");
                if (obj->objectSize < sizeof(AsfProtectionSystemIdObj)) {
                    LOGE("Unsupported Protection System Object");
                    return ASF_PARSER_BAD_DATA;
                }
                AsfProtectionSystemIdObj *protectionSysObj = (AsfProtectionSystemIdObj*)buffer;

                uint8_t* playreadyObjBuf = NULL;
                memcpy(mPlayreadyUuid, protectionSysObj->sysId, UUIDSIZE);

                // Rights Management Header - Record Type = 0x0001
                // Traverse till field containing number of records
                playreadyObjBuf = buffer + sizeof(AsfProtectionSystemIdObj);
                uint32_t sizeLeft = obj->objectSize - sizeof(AsfProtectionSystemIdObj); // For Maintaining and checking the left memory in the object

                for (int i = 0; i < protectionSysObj->countRecords; i++) {
                    if (sizeLeft > 4) {
                        uint16_t recordType = *((uint16_t*)playreadyObjBuf);
                        playreadyObjBuf += sizeof(uint16_t);
                        sizeLeft -= sizeof (recordType);

                        uint16_t recordLen = *((uint16_t*)playreadyObjBuf);
                        playreadyObjBuf += sizeof(uint16_t);
                        sizeLeft -= sizeof (recordLen);
                        if (sizeLeft < recordLen) {
                            LOGE("Invalid Rec Protection Identifier Object");
                            status = ASF_PARSER_BAD_DATA;
                            break;
                        }

                        if (recordType == 0x01)  {// Rights management Header
                            mPlayreadyHeaderLen = recordLen;

                            if (mPlayreadyHeaderLen == 0) {
                                LOGE("Invalid Protection System Record Length Value");
                                return ASF_PARSER_BAD_DATA;
                            }
                            mPlayreadyHeader = new uint8_t [mPlayreadyHeaderLen];
                            if (mPlayreadyHeader == NULL) {
                                return ASF_PARSER_NO_MEMORY;
                            }

                            memcpy(mPlayreadyHeader, playreadyObjBuf, mPlayreadyHeaderLen);
                            break;
                        }
                        playreadyObjBuf += recordLen;
                        sizeLeft -= recordLen;
                    } else {
                        LOGE("Invalid sizeLeft");
                        return ASF_PARSER_BAD_DATA;
                    }
                }
            }
            if (status != ASF_PARSER_SUCCESS) {
                return status;
            }
            size -= (uint32_t)obj->objectSize;
            buffer += obj->objectSize;
            mNumObjectParsed++;
            if (mNumObjectParsed == mNumberofHeaderObjects) {
                return ASF_PARSER_SUCCESS;
            }
        }
    }
    while (status == ASF_PARSER_SUCCESS);

    return status;
}

int AsfHeaderParser::getPlayreadyUuid(uint8_t playreadyUuid[], uint16_t len) {
    if (playreadyUuid == NULL || (!mIsProtected))
        return ASF_PARSER_FAILED;
    if (len < UUIDSIZE) {
        LOGE("Invalid length ");
        return ASF_PARSER_FAILED;
    }

    memcpy(playreadyUuid, mPlayreadyUuid, UUIDSIZE);
    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::getPlayreadyHeaderXml(uint8_t *header, uint32_t *len) {

    if (header == NULL) {
        *len = mPlayreadyHeaderLen;
        return ASF_PARSER_NULL_POINTER;
    }
    memcpy(header, mPlayreadyHeader, mPlayreadyHeaderLen);
    *len = mPlayreadyHeaderLen;

    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::onFilePropertiesObject(uint8_t *buffer, uint32_t size) {
    if (size < sizeof(AsfFilePropertiesObject))  {
        return ASF_PARSER_BAD_DATA;
    }

    AsfFilePropertiesObject *obj = (AsfFilePropertiesObject*)buffer;
    mFileInfo->dataPacketsCount = obj->dataPacketsCount;
    mFileInfo->duration = obj->playDuration;
    mFileInfo->fileSize = obj->fileSize;
    mFileInfo->packetSize = obj->maximumDataPacketSize;
    if (mFileInfo->packetSize != obj->minimumDataPacketSize) {
        return ASF_PARSER_BAD_VALUE;
    }
    mFileInfo->preroll = obj->preroll;
    mFileInfo->seekable = obj->flags.bits.seekableFlag;
    if (obj->flags.bits.broadcastFlag) {
        // turn off seeking
        mFileInfo->seekable = false;
    }
    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::onStreamPropertiesObject(uint8_t *buffer, uint32_t size) {
    int status;
    if (size < sizeof(AsfStreamPropertiesObject)) {
        return ASF_PARSER_BAD_DATA;
    }

    AsfStreamPropertiesObject *obj = (AsfStreamPropertiesObject*)buffer;
    if (obj->typeSpecificDataLength + obj->errorCorrectionDataLength >
        size - sizeof(AsfStreamPropertiesObject)) {
        return ASF_PARSER_BAD_VALUE;
    }

    uint8_t *typeSpecificData = buffer + sizeof(AsfStreamPropertiesObject);
    if (obj->streamType == ASF_Video_Media) {
        status = onVideoSpecificData(obj, typeSpecificData);
    } else if (obj->streamType == ASF_Audio_Media) {
        status = onAudioSpecificData(obj, typeSpecificData);
    } else {
        // ignore other media specific data
        status = ASF_PARSER_SUCCESS;
    }
    return status;
}

int AsfHeaderParser::onVideoSpecificData(AsfStreamPropertiesObject *obj, uint8_t *data) {
    // size of codec specific data is obj->typeSpecificDataLength
    uint32_t headerLen = sizeof(AsfVideoInfoHeader) + sizeof(AsfBitmapInfoHeader);
    if (obj->typeSpecificDataLength < headerLen) {
        return ASF_PARSER_BAD_DATA;
    }
    AsfVideoInfoHeader *info = (AsfVideoInfoHeader*)data;
    AsfBitmapInfoHeader *bmp = (AsfBitmapInfoHeader*)(data + sizeof(AsfVideoInfoHeader));

    if (info->formatDataSize < sizeof(AsfBitmapInfoHeader)) {
        return ASF_PARSER_BAD_VALUE;
    }

    if (bmp->formatDataSize - sizeof(AsfBitmapInfoHeader) >
        obj->typeSpecificDataLength - headerLen) {

        // codec specific data is invalid
        return ASF_PARSER_BAD_VALUE;
    }

    AsfVideoStreamInfo *videoInfo = new AsfVideoStreamInfo;
    if (videoInfo == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }
    videoInfo->streamNumber = obj->flags.bits.streamNumber;
    videoInfo->encryptedContentFlag = obj->flags.bits.encryptedContentFlag;
    videoInfo->timeOffset = obj->timeOffset;
    videoInfo->width = info->encodedImageWidth;
    videoInfo->height = info->encodedImageHeight;

    // Following condition taken from Section 2.4.2.2 - Video Media Type, of Playready documentation
    if (bmp->compressionID == FOURCC('Y', 'D', 'R', 'P')) {
        // That means PYV content, for which Compression Id is
        // the last 4 bytes of the codec specific data following the Video format data
        uint32_t* ptrActCompId = (uint32_t*)((data + sizeof(AsfVideoInfoHeader) + bmp->formatDataSize - sizeof(uint32_t)));
        videoInfo->fourCC = (*ptrActCompId);
    } else {
        videoInfo->fourCC = bmp->compressionID;
    }
    LOGV("onVideoSpecificData() with videoInfo->fourCC = %x", videoInfo->fourCC);

    // TODO: get aspect ratio from video meta data
    videoInfo->aspectX = 1;
    videoInfo->aspectY = 1;

    videoInfo->codecDataSize = bmp->formatDataSize - sizeof(AsfBitmapInfoHeader);
    if (videoInfo->codecDataSize) {
        videoInfo->codecData = new uint8_t [videoInfo->codecDataSize];
        if (videoInfo->codecData == NULL) {
            delete videoInfo;
            return ASF_PARSER_NO_MEMORY;
        }
        memcpy(videoInfo->codecData,
        data + headerLen,
        videoInfo->codecDataSize);
    } else {
        videoInfo->codecData = NULL;
    }

    videoInfo->next = NULL;
    if (mVideoInfo == NULL) {
        mVideoInfo = videoInfo;
    } else {
        AsfVideoStreamInfo *last = mVideoInfo;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = videoInfo;
    }

    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::onAudioSpecificData(AsfStreamPropertiesObject *obj, uint8_t *data) {
    if (obj->typeSpecificDataLength < sizeof(AsfWaveFormatEx)) {
        return ASF_PARSER_BAD_DATA;
    }

    AsfWaveFormatEx *format = (AsfWaveFormatEx*)data;
    if (format->codecSpecificDataSize >
        obj->typeSpecificDataLength - sizeof(AsfWaveFormatEx)) {
        return ASF_PARSER_BAD_VALUE;
    }

    AsfAudioStreamInfo *audioInfo = new AsfAudioStreamInfo;
    if (audioInfo == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }
    audioInfo->streamNumber = obj->flags.bits.streamNumber;
    audioInfo->encryptedContentFlag = obj->flags.bits.encryptedContentFlag;
    audioInfo->timeOffset = obj->timeOffset;

    // Codec Id is 0x5052 i.e. ASCII value of 'P', 'R' for Playready -
    // [Refer Section 2.4.2.1 - Audio Media Type, of Playready documentation]
    if (format->codecIDFormatTag == PRFORMATTAG) {
    // That means protected content, for which Codec Id is
    // the last 2 bytes of the codec specific data following the Audio format data
        uint32_t* ptrActCodecId = (uint32_t*)((data + sizeof(AsfWaveFormatEx) + format->codecSpecificDataSize - sizeof(format->codecIDFormatTag)));
        audioInfo->codecID = (*ptrActCodecId);
    } else {
        audioInfo->codecID = format->codecIDFormatTag;
    }
    LOGV("onAudioSpecificData => format->codecIDFormatTag = %x",format->codecIDFormatTag);

    audioInfo->numChannels = format->numberOfChannels;
    audioInfo->sampleRate= format->samplesPerSecond;
    audioInfo->avgByteRate = format->averageNumberOfBytesPerSecond;
    audioInfo->blockAlignment = format->blockAlignment;
    audioInfo->bitsPerSample = format->bitsPerSample;
    audioInfo->codecDataSize = format->codecSpecificDataSize;
    if (audioInfo->codecDataSize) {
        audioInfo->codecData = new uint8_t [audioInfo->codecDataSize];
        if (audioInfo->codecData == NULL) {
            delete audioInfo;
            return ASF_PARSER_NO_MEMORY;
        }
        memcpy(audioInfo->codecData,
            data + sizeof(AsfWaveFormatEx),
            audioInfo->codecDataSize);
    } else {
        audioInfo->codecData = NULL;
    }

    audioInfo->next = NULL;

    if (mAudioInfo == NULL) {
        mAudioInfo = audioInfo;
    } else {
        AsfAudioStreamInfo *last = mAudioInfo;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = audioInfo;
    }

    return ASF_PARSER_SUCCESS;
}


int AsfHeaderParser::onExtendedStreamPropertiesObject(uint8_t *buffer, uint32_t size) {
    int status = ASF_PARSER_SUCCESS;

    if (size < sizeof(AsfObject)) {
        return ASF_PARSER_BAD_DATA;
    }

    AsfExtendedStreamPropertiesObject *fixedLenExtStrPropsObj = (AsfExtendedStreamPropertiesObject *)buffer;
    if (fixedLenExtStrPropsObj->objectSize > size) {
        ALOGE("Invalid ASF Extended Stream Prop Object size");
        return ASF_PARSER_BAD_VALUE;
    }

    AsfExtendedStreamPropertiesExObject *extStrObj = new AsfExtendedStreamPropertiesExObject;
    if (extStrObj == NULL) {
        return ASF_PARSER_NO_MEMORY;
    }

    //copy all fixed length fields first.
    memcpy(&(extStrObj->propObj), fixedLenExtStrPropsObj, sizeof(AsfExtendedStreamPropertiesObject));

    ALOGD("Stream number = 0x%08X", fixedLenExtStrPropsObj->streamNumber);
    ALOGD("PayloadExtensionSystemCount = 0x%08X", fixedLenExtStrPropsObj->payloadExtensionSystemCount);

    // Get pointer to buffer where variable length fields might start
    buffer += sizeof(AsfExtendedStreamPropertiesObject);

    uint32_t streamNameOffset =  sizeof(uint16_t) * 2; // languageIDIndex + streamNameLength
    //StramNames might start here. depends on streamNameCount.
    for (int i = 0; i < fixedLenExtStrPropsObj->streamNameCount; i++) {
         AsfStreamName *streamNameObj = new AsfStreamName;
         AsfStreamName *StreamNameObjBuffer = (AsfStreamName *)buffer;

         // populate the StreamName object from the buffer
         streamNameObj->languageIDIndex = StreamNameObjBuffer->languageIDIndex;
         streamNameObj->streamNameLength = StreamNameObjBuffer->streamNameLength;

         // Allocate space to store StreamName(3rd field) for the StreamName structure
        if (streamNameObj->streamNameLength > 0) {
            streamNameObj->pStreamName = new uint8_t [StreamNameObjBuffer->streamNameLength];
            if (streamNameObj->pStreamName == NULL) {
               delete streamNameObj;
               delete extStrObj;
               return ASF_PARSER_NO_MEMORY;
            }

            memcpy(streamNameObj->pStreamName, buffer + streamNameOffset, StreamNameObjBuffer->streamNameLength);
        } else {
            // no streamName
            streamNameObj->pStreamName = NULL;
        }

        // calculate the length of current StreamName entry.
        // if there are multiple StreamNames  then increment
        // buffer by  4 + streamNameLength  to point to next StreamName entry
        buffer += streamNameOffset + StreamNameObjBuffer->streamNameLength;

        extStrObj->streamNames.push_back(streamNameObj);
    }

    uint32_t systemInfoOffset = sizeof(GUID) + sizeof(uint16_t) + sizeof(uint32_t);
    //buffer points to extension systems here depending on payloadExtensionSystemCount in previous Fixed field.
    for (int i = 0; i < fixedLenExtStrPropsObj->payloadExtensionSystemCount; i++) {
        AsfPayloadExtensionSystem *extensionObj = new  AsfPayloadExtensionSystem;
        AsfPayloadExtensionSystem *extObjData = (AsfPayloadExtensionSystem *)buffer;

        // populate the extension object from the buffer
        extensionObj->extensionSystemId = extObjData->extensionSystemId;
        extensionObj->extensionDataSize = extObjData->extensionDataSize;
        extensionObj->extensionSystemInfoLength = extObjData->extensionSystemInfoLength;

        // Allocate space to store extensionSystemInfo
        if (extensionObj->extensionSystemInfoLength > 0) {
            extensionObj->extensionSystemInfo = new uint8_t [extObjData->extensionSystemInfoLength];
            if (extensionObj->extensionSystemInfo == NULL) {
               delete extensionObj;
               delete extStrObj;
               return ASF_PARSER_NO_MEMORY;
            }
            memcpy(extensionObj->extensionSystemInfo, buffer + systemInfoOffset, extObjData->extensionSystemInfoLength);
        } else {
            // no extension system info
            extensionObj->extensionSystemInfo = NULL;
        }

        // calculate the length of current extension system.
        // if there are multiple extension systems then increment
        // buffer by  22 + extensionSystemInfoLength  to point to
        // next extension object
        buffer += systemInfoOffset + extensionObj->extensionSystemInfoLength;

        // add the extension object to the extended stream object
        extStrObj->extensionSystems.push_back(extensionObj);
    }

    mExtendedStreamPropertiesObj.push_back(extStrObj);
    return ASF_PARSER_SUCCESS;
}

int AsfHeaderParser::parseHeaderExtensionObject(uint8_t* buffer, uint32_t size) {
    // No empty space, padding, leading, or trailing bytes are allowed in the extention data

    int status = ASF_PARSER_SUCCESS;
    do {
        if (size < sizeof(AsfObject)) {
            return ASF_PARSER_BAD_DATA;
        }

        AsfObject *obj = (AsfObject *)buffer;
        if (obj->objectSize > size) {
            return ASF_PARSER_BAD_VALUE;
        }

        if(obj->objectID == ASF_Extended_Stream_Properties_Object) {
            status = onExtendedStreamPropertiesObject(buffer, size);
        } else if(obj->objectID == ASF_Advanced_Mutual_Exclusion_Object) {
        } else if(obj->objectID == ASF_Group_Mutual_Exclusion_Object) {
        } else if(obj->objectID == ASF_Stream_Prioritization_Object) {
        } else if(obj->objectID == ASF_Bandwidth_Sharing_Object) {
        } else if(obj->objectID == ASF_Language_List_Object) {
        } else if(obj->objectID == ASF_Metadata_Object) {
        } else if(obj->objectID == ASF_Metadata_Library_Object) {
        } else if(obj->objectID == ASF_Index_Parameters_Object) {
        } else if(obj->objectID == ASF_Media_Object_Index_Parameters_Object) {
        } else if(obj->objectID == ASF_Timecode_Index_Parameters_Object) {
        } else if(obj->objectID == ASF_Compatibility_Object) {
        } else if(obj->objectID == ASF_Advanced_Content_Encryption_Object) {
        } else {
        }

        if (status != ASF_PARSER_SUCCESS) {
            break;
        }

        size -= (uint32_t)obj->objectSize;
        buffer += obj->objectSize;

        if (size == 0) {
            break;
        }
    }
    while (status == ASF_PARSER_SUCCESS);

    return status;
}


int AsfHeaderParser::parseSampleIDFromReplicatedData(AsfPayloadDataInfo *obj, uint8_t streamNumber) {

    vector<AsfPayloadExtensionSystem *> *extSystems = NULL;

    // Get handle to extension systems in that stream's Extension Stream Properties object.
    for (unsigned int i = 0; i < mExtendedStreamPropertiesObj.size(); i++) {
        if (streamNumber == mExtendedStreamPropertiesObj[i]->propObj.streamNumber) {
            extSystems = &(mExtendedStreamPropertiesObj[i]->extensionSystems);
            break;
        }
    }

    if (extSystems == NULL) {
        return ASF_PARSER_FAILED;
    }

    int replicatedDataOffset = 0;
    // Find data correspodning to ASF_Payload_Extension_System_Encryption_Sample_ID which is SampleID
    for (int i = 0; i < extSystems->size(); i++) {
        // Point to ext system's data in replicated data buffer.
        if ((extSystems->at(i)->extensionSystemId) == ASF_Payload_Extension_System_Encryption_Sample_ID) {
            if (extSystems->at(i)->extensionDataSize == 0xFFFF) {
                uint16_t extensionDataSize = *((uint16_t*) (obj->replicatedData + 8 + replicatedDataOffset));
                obj->sampleIDLen = extensionDataSize;
                obj->sampleID = obj->replicatedData + 8 + replicatedDataOffset + sizeof(uint16_t);  // 2 bytes denote size of this extSystem
            } else {
                obj->sampleID = obj->replicatedData + 8 + replicatedDataOffset;
                obj->sampleIDLen = extSystems->at(i)->extensionDataSize;
            }
            return ASF_PARSER_SUCCESS;
        }
        // Some other extension system. Modify the replicatedData offset accordingly to point to next extension system.
        if (extSystems->at(i)->extensionDataSize == 0xFFFF) {
            uint16_t nSize = *((uint16_t*) (obj->replicatedData + 8 + replicatedDataOffset));
            replicatedDataOffset += (sizeof(uint16_t) + nSize);
        } else {
            replicatedDataOffset += extSystems->at(i)->extensionDataSize;
        }
    }
    return ASF_PARSER_UNEXPECTED_VALUE; // Other extension systems.
}

void AsfHeaderParser::resetStreamInfo() {
    while (mAudioInfo) {
         AsfAudioStreamInfo *next = mAudioInfo->next;
         delete [] mAudioInfo->codecData;
         delete mAudioInfo;
         mAudioInfo = next;
     }

     while (mVideoInfo) {
         AsfVideoStreamInfo *next = mVideoInfo->next;
         delete [] mVideoInfo->codecData;
         delete mVideoInfo;
         mVideoInfo = next;
     }
}

