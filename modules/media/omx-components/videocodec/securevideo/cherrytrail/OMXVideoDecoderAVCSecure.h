/*
* Copyright (c) 2009-2013 Intel Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef OMX_VIDEO_DECODER_AVC_SECURE_H_
#define OMX_VIDEO_DECODER_AVC_SECURE_H_

#include <utils/Mutex.h>
#include "OMXVideoDecoderBase.h"
#include "libpavp.h"
#include "widevine.h"
#include "secvideoparser.h"

//  Must match the same structs defined in WVCrypto.h
#pragma pack(push, 1)
typedef struct {
    uint32_t offset;
    uint32_t size;
} sec_partition_t;
typedef struct {
    sec_partition_t src;
    sec_partition_t dest;
    sec_partition_t metadata;
    sec_partition_t headers;
} video_partition_t;
typedef struct {
    pavp_lib_session* pLibInstance;
    uint8_t* base;
    uint32_t size;
    uint32_t base_offset;
    video_partition_t partitions;
    uint32_t frame_size;
    uint32_t src_fill;
    uint8_t pes_packet_count;
    uint8_t clear;
} SECVideoBuffer;
typedef struct {
    uint32_t index;
    SECVideoBuffer* secBuffer;
} OMXSecureBuffer;
#pragma pack(pop)


class OMXVideoDecoderAVCSecure : public OMXVideoDecoderBase {
public:
    OMXVideoDecoderAVCSecure();
    virtual ~OMXVideoDecoderAVCSecure();

protected:
    virtual OMX_ERRORTYPE InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput);;
    virtual OMX_ERRORTYPE ProcessorStop(void);
    virtual OMX_ERRORTYPE ProcessorProcess(
            OMX_BUFFERHEADERTYPE ***pBuffers,
            buffer_retain_t *retains,
            OMX_U32 numberBuffers);

   virtual OMX_ERRORTYPE PrepareConfigBuffer(VideoConfigBuffer *p);
   virtual OMX_ERRORTYPE PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p);
   virtual OMX_COLOR_FORMATTYPE GetOutputColorFormat(int width, int height);

   virtual OMX_ERRORTYPE BuildHandlerList(void);
   DECLARE_HANDLER(OMXVideoDecoderAVCSecure, ParamVideoAvc);
   DECLARE_HANDLER(OMXVideoDecoderAVCSecure, ParamVideoAVCProfileLevel);
   DECLARE_HANDLER(OMXVideoDecoderAVCSecure, NativeBufferMode);

    static OMX_U8* MemAllocSecure(OMX_U32 nSizeBytes, OMX_PTR pUserData);
    static void MemFreeSecure(OMX_U8 *pBuffer, OMX_PTR pUserData);
    OMX_U8* MemAllocSecure(OMX_U32 nSizeBytes);
    void  MemFreeSecure(OMX_U8 *pBuffer);

    OMX_ERRORTYPE ConstructFrameInfo(uint8_t* frame_data, uint32_t frame_size,
        pavp_info_t* pavp_info, uint8_t* nalu_data, uint32_t nalu_data_size,
        frame_info_t* frame_info);

private:

    enum {
        // WARNING: if these values are changed, please make corresponding changes
        // in the SEC memory region partitioning in the OEMCrypto for BYT (secregion.h)

        // OMX_PARAM_PORTDEFINITIONTYPE
        INPORT_MIN_BUFFER_COUNT = 1,
        INPORT_ACTUAL_BUFFER_COUNT = 5,
        INPORT_BUFFER_SIZE = 1382400,

        // for OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS
        // default number of reference frame
        NUM_REFERENCE_FRAME = 4,

        OUTPORT_NATIVE_BUFFER_COUNT = 20,
    };

    OMX_VIDEO_PARAM_AVCTYPE mParamAvc;

    OMXSecureBuffer* mOMXSecureBuffers[INPORT_ACTUAL_BUFFER_COUNT];

    struct SECParsedFrame {
        uint8_t* nalu_data;
        uint32_t nalu_data_size;
        pavp_info_t pavp_info;
        frame_info_t frame_info;
    };

    SECParsedFrame mParsedFrames[INPORT_ACTUAL_BUFFER_COUNT];

    pavp_lib_session *mpLibInstance;
    bool mDropUntilIDR;
};

#endif /* OMX_VIDEO_DECODER_AVC_SECURE_H_ */
