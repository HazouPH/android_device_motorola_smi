/*
* Copyright (c) 2009-2014 Intel Corporation.  All rights reserved.
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

#define SEC_DMA_ALIGN(x)            (((x)+0x1F)&(~0x1F))
#define DRM_TYPE_CLASSIC_WV 0x0
#define DRM_TYPE_MDRM 0x1 
#define MDRM_API_VERSION  0x00010005
#define MDRM_OMX_VERSION  2.0001


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
    uint8_t config[64];
    uint8_t config_len;
    uint32_t config_frame_offset;
    uint8_t key_id[16];
    uint32_t key_id_len;
    uint8_t session_id;
} mdrm_meta;
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
    mdrm_meta mdrm_info;
    uint8_t drm_type; //0 -> Classic, 1 -> MDRM
    uint8_t iv[16];
} SECVideoBuffer;
typedef struct {
    uint32_t index;
    SECVideoBuffer* secBuffer;
} OMXSecureBuffer;
#pragma pack(pop)

//Function Codes
typedef enum {
      wv2_begin = 0x000C0001,
    wv2_open_session = wv2_begin,
      wv2_generate_nonce,
      wv2_generate_derived_keys,
      wv2_generate_hmac_signature,
      wv2_load_keys,
      wv2_refresh_keys,
      wv2_select_key,
      wv2_inject_key,
      wv2_rewrap_device_RSA_key,
      wv2_load_device_RSA_key,
      wv2_generate_RSA_signature,
      wv2_derived_sessionkeys,
      wv2_process_video_frame,
      wv2_decrypt_ctr,
      wv2_generic_encrypt,
      wv2_generic_decrypt,
      wv2_generic_sign,
      wv2_generic_verify_signature,
      wv2_close_session,
      wv2_dbg_get_keys,
      wv2_delete_nonce,
} wv2_heci_command_id;


typedef struct {
	uint32_t	dest_encrypt_mode_25_24 : 2;
	uint32_t	reserved_31_26 : 6;
	uint32_t	src_encrypt_mode_17_16 : 2;
	uint32_t	reserved_23_18 : 6;
	uint32_t	num_headers_or_packets_15_8 : 8;
	uint32_t	drm_type_1_0 : 2;
	uint32_t	reserved_7_2 : 6;
} transcript_conf;
/*processvidoe frame*/
typedef struct {
	PAVP_CMD_HEADER			Header;
	transcript_conf			conf;
	uint32_t				key_index;
	uint32_t				frame_offset;
	uint32_t				metadata_offset;
	uint32_t				header_offset;
	uint32_t				dest_offset;
        uint8_t             key_id[16];
} process_video_frame_in;
typedef struct {
	PAVP_CMD_HEADER			Header;
	uint32_t				parsed_data_size;
        uint8_t             key_id[16];
        uint8_t             key[16];
	uint8_t					iv[16];
} process_video_frame_out;

/*wv2_inject_key*/
typedef struct {
      PAVP_CMD_HEADER               Header;
    uint32_t                        session_id;
      uint32_t                      StreamId;
      transcript_conf               conf;
      uint32_t                      key_id_len;
      uint8_t                             key_id[16];
} wv2_inject_key_in;

typedef struct {
      PAVP_CMD_HEADER               Header;
} wv2_inject_key_out;


/*wv2_process_video_frame*/
typedef struct {
    PAVP_CMD_HEADER  Header;
    transcript_conf  conf;
    uint32_t         session_id;
    uint32_t         frame_offset;
    uint32_t         metadata_offset;
    uint32_t         header_offset;
    uint32_t         dest_offset;
    uint32_t         key_id_len;
    uint8_t          key_id[16];
} wv2_process_video_frame_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    uint32_t         parsed_data_size;;
    uint8_t          iv[16];
} wv2_process_video_frame_out;

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
    uint32_t mPAVPAppID;
    OMX_ERRORTYPE CreatePavpSession(void);
    OMX_ERRORTYPE SecPassThrough(uint8_t*, uint32_t, uint8_t*, uint32_t);
    OMX_ERRORTYPE MdrmInjectKey(uint8_t, uint8_t*);
    OMX_ERRORTYPE WvSetTranscriptKey(void);
    OMX_ERRORTYPE ManagePAVPSession(bool);
    OMX_ERRORTYPE ClassicProcessVideoFrame(SECVideoBuffer *secBuffer, uint32_t*);
    OMX_ERRORTYPE ModularProcessVideoFrame(SECVideoBuffer *secBuffer, uint32_t*);
};

#endif /* OMX_VIDEO_DECODER_AVC_SECURE_H_ */
