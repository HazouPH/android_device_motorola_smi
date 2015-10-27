/*++

   INTEL CONFIDENTIAL
   Copyright (c) 2013- Intel Corporation All Rights Reserved.

   The source code contained or described herein and all documents related to
   the source code ("Material") are owned by Intel Corporation or its
   suppliers or licensors. Title to the Material remains with Intel
   Corporation or its suppliers and licensors. The Material contains trade
   secrets and proprietary and confidential information of Intel or its
   suppliers and licensors. The Material is protected by worldwide copyright
   and trade secret laws and treaty provisions. No part of the Material may be
   used, copied, reproduced, modified, published, uploaded, posted, transmitted,
   distributed, or disclosed in any way without Intel's prior express written
   permission.

   No license under any patent, copyright, trade secret or other intellectual
   property right is granted to or conferred upon you by disclosure or delivery
   of the Materials, either expressly, by implication, inducement, estoppel or
   otherwise. Any license under such intellectual property rights must be
   express and approved by Intel in writing.

File Name:

   widevine.h

Abstract:



Author:

   Ruan, Xiaoyu
   Patel, Arpit A

Revision history:

   3/17/2013: WV SEC FW API v0.19
   3/21/2013: v0.30
   3/29/2013: v0.40
   4/4/2013:  v0.43
   4/8/2013:  v0.45
   4/11/2013: v0.50
   4/24/2013: v0.51

--*/

#ifndef __WIDEVINE_H
#define __WIDEVINE_H

#if 0
#include "CompMgmt.h"
#include "Heci.h"
#include "Crypto.h"
#include "Storage.h"
#include "Memory.h"
#include "MemDefs.h"
#include <string.h>

#include "PavpHeciApiCommonDefs.h"
#include "PavpHeciApi.h"
#include "PavpVideoKeyMgr.h"
#include "PavpBaseKeyMgr.h"
#include "GfxKeyMgrDefs.h"

#include "dbg.h"
#include "romapi.h"

#include "bitstream.h"
#include "decoder.h"
#include "nalu.h"
#include "sei.h"
#include "sps.h"
#include "pps.h"
#include "slice_header.h"
#include "flag.h"
#endif

#define WV_USE_SATT 1

#define WV_1KB 1024
#define WV_DEVICE_ID_SIZE 32
#define WV_DEVICE_KEY_SIZE 16
#define WV_KEY_DATA_SIZE 72
#define MV_KEYBOX_MAGIC_SIZE 4
#define WV_CRC_SIZE 4
#define WV_MAGIC_SIZE 4
#define WV_AES_IV_SIZE 16
#define WV_AES_KEY_SIZE 16
#define WV_AES_BLOCK_SIZE 16
#define WV_MAX_PACKETS_IN_FRAME 20 /* 20*64K=1.3M, max frame size */
#define WV_MAX_PACKET_SIZE (64*WV_1KB-1)
#define WV_PARSED_BUFFER_SIZE (4*WV_1KB)
#define WV_PER_DMA_SIZE (8*WV_1KB + 16 + 16) /* 16 is AES block size, for CTS last block. add 16 more for DMA alignment */
#define WV_PARSER_BUF_EDGE 64
#define WV_WORK_BUFFER_SIZE (WV_PER_DMA_SIZE+WV_PARSER_BUF_EDGE)
#define WV_ECM_SIZE 32
#define WV_FLAGS_SIZE 4
#define WV_RAND_SIZE 32
#define WV_API_VERSION 0x00010000
#define WV_FRAME_METADATA_SIZE (WV_MAX_PACKETS_IN_FRAME*sizeof(wv_frame_metadata))
#define WV_MALLOC_ALIGNMENT 32
#define WV_MALLOC_TIMEOUT 0
#define WV_DMA_ALIGNMENT 32
#define WV_XCRIPT_DMA_OUT_SWAP (CIPHER_DST_SWAP)
#define WV_OUT_MSG_FLAG 0x80000000
#define WV_SATT_BASE SATT_2

#define WV_CEILING(a,b) ((a)%(b)==0?(a):((a)/(b)+1)*(b))
#define PREPRODUCTION   (RomData.gBringupDataPtr->FuseMap.DevModeEnabled || RomData.gBringupDataPtr->FuseMap.JtagEnabled)
#define WV_SWAP_DWORD(dword)  (((dword) & 0x000000ff) << 24 | ((dword) & 0x0000ff00) << 8 | ((dword) & 0x00ff0000) >> 8 | ((dword) & 0xff000000) >> 24)
#define WV_SWAP_SHORT(sho)  (((sho) & (0x00ff)) << 8 | ((sho) & (0xff00)) >> 8)

/**
 * @brief PAVP HECI message header.
 */
typedef struct _PAVP_CMD_HEADER
{
   UINT32         ApiVersion;
   UINT32         CommandId;
   UINT32         Status;
   UINT32         BufferLength;
} PAVP_CMD_HEADER;

typedef struct _PAVP_CMD_NODATA
{
   PAVP_CMD_HEADER Header;
   // no data follows header   
} PAVP_CMD_NODATA;

typedef int BOOL;

typedef struct {
    uint8_t device_id[WV_DEVICE_ID_SIZE];
    uint8_t device_key[WV_DEVICE_KEY_SIZE];
    uint8_t key_data[WV_KEY_DATA_SIZE];
    uint8_t magic[WV_MAGIC_SIZE];
    uint8_t key_data_crc[WV_CRC_SIZE];
} wv_keybox;

typedef struct {
    uint32_t packet_byte_size; // number of bytes in this PES packet, same for input and output
    uint8_t  packet_iv[WV_AES_IV_SIZE]; // IV used for CBC-CTS decryption, if the PES packet is encrypted
} sec_wv_packet_metadata;

typedef struct {
    sec_wv_packet_metadata  *metadata_buffer;//WV_MAX_PACKETS_IN_FRAME * sizeof(wv_packet_metadata)
	uint32_t title_video_size;
} wv_frame_metadata;

/* wv_nalu and wv_nalu_headers are for host only.  FW does not use */
typedef struct {
    uint32_t imr_offset;
    uint32_t nalu_size;
    uint32_t data_size;
    uint8_t  data[1]; //place holder.  actual size is data_size
} wv_nalu;

/* wv_nalu and wv_nalu_headers are for host only.  FW does not use */
typedef struct {
    uint32_t num_nalu;
    wv_nalu  drm_nalu[1];// place holder.  actual size is num_nalu
} wv_nalu_headers;

#if 0
typedef struct {
	wv_frame_metadata  frame_metadata;
    uint32_t *frame_header; //WV_PARSED_BUFFER_SIZE, dynamic allocation from locked memory
    uint8_t  *frame_proc_buffer;// size = WV_PER_DMA_SIZE, dynamic allocation from locked memory
    uint8_t  *work_buffer;// size = WV_WORK_BUFFER_SIZE, dynamic allocation from locked memory
    uint8_t  *parser_work_buffer;// size = WV_WORK_BUFFER_SIZE, dynamic allocation from locked memory
    uint8_t   asset_key[WV_AES_KEY_SIZE];
    BOOL      asset_key_valid;
    uint8_t   title_key[WV_AES_KEY_SIZE];
    uint8_t   title_decrypt_key[WV_AES_KEY_SIZE];
    BOOL      title_key_valid;
    uint8_t   pavp_key[WV_AES_KEY_SIZE];
    BOOL      pavp_key_valid;
    uint8_t   pavp_counter[WV_AES_IV_SIZE];
    decoder_ctx_t decoder;
    uint32_t  video_frame_count;
    BOOL      bypass_transcryption;
    uint32_t  audio_frame_count;
    uint32_t  time_start; /*timer starts at first frame of a video frame*/
    uint32_t  time_end; /*timer ends at title_complete*/
    uint32_t  time_start_heci;
    uint32_t  time_heci;
    uint32_t  time_dma_in;
    uint32_t  time_decrypt;
    uint32_t  time_parse;
    uint32_t  time_encrypt_dma_out;
} wv_title_context;

typedef struct {
#if !WV_USE_SATT
    uint32_t  host_phy_mem_lo;
    uint32_t  host_phy_mem_hi;
    uint32_t  host_phy_mem_size;
    BOOL      host_phy_mem_valid;
#endif
    wv_keybox keybox;
    BOOL      keybox_valid;
    wv_title_context   title;
    PAVP_STREAM_ID     wvVideoStreamSlot;
} wv_context;
#endif

/*******************
 * HECI APIs
 *******************/

typedef enum {
    WV_SUCCESS = 0,
    WV_SUCCESS_PRIV_DATA = 0x80000000,
    WV_FAIL_INVALID_PARAMS = 0x000F0001,
    WV_FAIL_INVALID_INPUT_HEADER,
    WV_FAIL_NOT_PROVISIONED,
    WV_FAIL_BAD_KEYBOX_CRC,
    WV_FAIL_UNKNOWN_ERROR,
    WV_FAIL_HDCP_OFF,
    WV_FAIL_NO_HOST_MEM,
    WV_FAIL_NOT_EXPECTED,
    WV_FAIL_INVALID_AUDIO_FRAME,
    WV_FAIL_KEYBOX_INVALID_BAD_PROVISIONING,
    WV_FAIL_KEYBOX_INVALID_BAD_MAGIC,
    WV_FAIL_WV_NO_ASSET_KEY,
    WV_FAIL_WV_NO_CEK,
    WV_FAIL_REACHED_HOST_MEM_LIMIT,
    WV_FAIL_WV_SESSION_NALU_PARSE_FAILURE,
    WV_FAIL_WV_SESSION_NALU_PARSE_TOO_MANY_HEADERS,
    WV_FAIL_GENERATE_RANDOM_NUMBER_FAILURE,
    WV_FAIL_AES_CBC_FAILURE,
    WV_FAIL_AES_XCRIPT_FAILURE,
    WV_FAIL_AES_ECB_FAILURE,
    WV_FAIL_BLOB_ERROR,
    WV_FAIL_BAD_AUDIO_CLEAR_PKT,
    WV_FAIL_NO_TITLE_KEY,
    WV_FAIL_OUT_OF_MEMORY,
    WV_FAIL_PAVP_INJECT_KEY_ERROR,
    WV_FAIL_MSG_NOT_FROM_RING_0,
    WV_FAIL_DMA_READ,
    WV_FAIL_DMA_WRITE,
    WV_FAIL_DMA_WRITE_HEADER,
    WV_FAIL_INVALID_TITLE_KEY,
    WV_FAIL_INVALID_PAVP_KEY,
    WV_FAIL_INVALID_NUM_OF_PACKETS,
    WV_FAIL_PAVP_INIT_NOT_COMPLETE,
    WV_FAIL_STATUS_CHAIN_NOT_INITIALIZED,
    WV_FAIL_OUTPUT_HOST_MEM_OVERLAP,
    WV_FAIL_NOT_SUPPORTED
} wv_heci_status;

typedef enum {
    wv_init_dma = 0x000A0002,
    wv_heci_begin = wv_init_dma,
    wv_get_random,
    wv_get_keybox_data,
    wv_set_xcript_key,
    wv_set_entitlement_key,
    wv_derive_control_word,
    wv_process_video_frame,
    wv_process_audio_frame,
    wv_title_completed,
    wv_uninit_dma,
    wv_heci_end = wv_uninit_dma,
    wv_dbg_get_title_key = 0x000A00F0,
    wv_heci_dbg_begin = wv_dbg_get_title_key,
    wv_dbg_get_xcript_key,
    wv_dbg_dis_xcript_enc,
    wv_dbg_en_xcript_enc,
    wv_dbg_get_diag,
    wv_dbg_set_keybox,
    wv_dbg_reset_keybox,
    wv_dbg_set_xcript_key_wo_pavp,
    wv_dbg_reset_xcript_key_wo_pavp,
    wv_heci_dbg_end = wv_dbg_reset_xcript_key_wo_pavp,

    wv_init_dma_rsp = (WV_OUT_MSG_FLAG | wv_init_dma),
    wv_get_random_rsp,
    wv_get_keybox_data_rsp,
    wv_set_xcript_key_rsp,
    wv_set_entitlement_key_rsp,
    wv_derive_control_word_rsp,
    wv_process_video_frame_rsp,
    wv_process_audio_frame_rsp,
    wv_title_completed_rsp,
    wv_uninit_dma_rsp,
    wv_dbg_get_title_key_rsp = (WV_OUT_MSG_FLAG | wv_dbg_get_title_key),
    wv_dbg_get_xcript_key_rsp,
    wv_dbg_dis_xcript_enc_rsp,
    wv_dbg_en_xcript_enc_rsp,
    wv_dbg_get_diag_rsp,
    wv_dbg_set_keybox_rsp,
    wv_dbg_reset_keybox_rsp,
    wv_dbg_set_xcript_key_wo_pavp_rsp,
    wv_dbg_reset_xcript_key_wo_pavp_rsp,
} wv_heci_command_id;


/*wv_init_dma*/
typedef struct {
    PAVP_CMD_HEADER  Header;
    uint32_t         phy_mem_lo;
    uint32_t         phy_mem_hi;
    uint32_t         phy_mem_size;
} wv_init_dma_in;

typedef PAVP_CMD_NODATA wv_init_dma_out;

/*wv_heci_get_random*/
typedef struct {
    PAVP_CMD_HEADER  Header;
    uint32_t         size_in_bytes;
} wv_heci_get_random_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    uint8_t          random_bytes[WV_RAND_SIZE];
} wv_heci_get_random_out;

/*wv_heci_get_keybox_data*/
typedef PAVP_CMD_NODATA wv_heci_get_keybox_data_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    uint8_t key_data[WV_KEY_DATA_SIZE];
    uint8_t device_id[WV_DEVICE_ID_SIZE];
} wv_heci_get_keybox_data_out;

/*wv_set_xcript_key*/
typedef PAVP_CMD_NODATA wv_set_xcript_key_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    /* private data for driver.  Size excluded in Header.Length
     * app caller: do not include this field*/
    //uint8_t          wrapped_xcript_key[WV_AES_KEY_SIZE];// libpcp will take care of this. Remove this from OMX.
} wv_set_xcript_key_out;

/*wv_heci_set_entitlement_key*/
typedef struct {
    PAVP_CMD_HEADER  Header;
    uint8_t          entitlement_key[WV_AES_KEY_SIZE];
} wv_heci_set_entitlement_key_in;

typedef PAVP_CMD_NODATA wv_heci_set_entitlement_key_out;

/*wv_heci_derive_control_word*/
typedef struct {
    PAVP_CMD_HEADER  Header;
    uint8_t          ecm[WV_ECM_SIZE];
} wv_heci_derive_control_word_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    uint8_t          flags[WV_FLAGS_SIZE];
} wv_heci_derive_control_word_out;

/*wv_heci_process_video_frame*/
typedef struct {
    PAVP_CMD_HEADER  Header;
    uint32_t         num_of_packets;//<=20
    BOOL             is_frame_not_encrypted;
    uint32_t         src_offset;
    uint32_t         dest_offset;
    uint32_t         metadata_offset;
    uint32_t         header_offset;
} wv_heci_process_video_frame_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    uint32_t         parsed_data_size;
    /* private data for driver.  Size excluded in Header.Length
     * app caller: do not include this field*/ //Remove comment
    uint8_t          iv[WV_AES_IV_SIZE];
} wv_heci_process_video_frame_out;

/*wv_heci_process_audio_frame*/
typedef struct {
    PAVP_CMD_HEADER  Header;
    uint32_t         num_of_packets;//<=20
    BOOL             is_frame_not_encrypted;
    uint32_t         src_offset;
    uint32_t         dest_offset;
    uint32_t         metadata_offset;
} wv_heci_process_audio_frame_in;

typedef PAVP_CMD_NODATA wv_heci_process_audio_frame_out;

/*wv_title_completed*/
typedef PAVP_CMD_NODATA wv_title_completed_in;
typedef PAVP_CMD_NODATA wv_title_completed_out;

/*wv_dbg_get_title_key*/
typedef PAVP_CMD_NODATA wv_dbg_get_title_key_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    BOOL             title_key_valid;
    uint8_t          title_key[WV_AES_KEY_SIZE];
} wv_dbg_get_title_key_out;

/*wv_uninit_dma*/
typedef PAVP_CMD_NODATA wv_uninit_dma_in;
typedef PAVP_CMD_NODATA wv_uninit_dma_out;

/*wv_dbg_get_xcript_key*/
typedef PAVP_CMD_NODATA wv_dbg_get_xcript_key_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    BOOL             xcript_key_valid;
    uint8_t          xcript_key[WV_AES_KEY_SIZE];
    uint8_t          xcript_counter[WV_AES_IV_SIZE];
} wv_dbg_get_xcript_key_out;

/*wv_dbg_dis_xcript_enc*/
typedef PAVP_CMD_NODATA wv_dbg_dis_xcript_enc_in;
typedef PAVP_CMD_NODATA wv_dbg_dis_xcript_enc_out;

/*wv_dbg_en_xcript_enc*/
typedef PAVP_CMD_NODATA wv_dbg_en_xcript_enc_in;
typedef PAVP_CMD_NODATA wv_dbg_en_xcript_enc_out;

/*wv_dbg_get_diag*/
typedef PAVP_CMD_NODATA wv_dbg_get_diag_in;

typedef struct {
    PAVP_CMD_HEADER  Header;
    uint16_t         fw_major_ver;
    uint16_t         fw_minor_ver;
    uint16_t         fw_hotfix;
    uint16_t         fw_build;
    BOOL             production_part;
#if !WV_USE_SATT
    uint32_t         host_phy_mem_lo;
    uint32_t         host_phy_mem_hi;
    uint32_t         host_phy_mem_size;
    BOOL             host_phy_mem_valid;
#endif
    uint32_t         video_frame_count;
    uint32_t         audio_frame_count;
    uint32_t         slice_count_for_this_frame;
    uint32_t         total_slice_count_for_this_title;
    uint32_t         bypass_transcryption : 1;
    uint32_t         frame_header_valid : 1;
    uint32_t         frame_proc_buffer_valid : 1;
    uint32_t         work_buffer_valid : 1;
    uint32_t         parser_work_buffer_valid : 1;
    uint32_t         asset_key_valid_valid : 1;
    uint32_t         title_key_valid_valid : 1;
    uint32_t         pavp_key_valid : 1;
    uint32_t         keybox_valid : 1;
    uint32_t         reserved1 : 7;
    uint32_t         wvVideoStreamSlot : 8;
    uint32_t         reserved2 : 8;
    uint32_t         time_start; /*timer starts at first frame of a video frame*/
    uint32_t         time_end; /*timer ends at title_complete*/
    uint32_t         time_elapsed; /*time_end - time_start, unit is microsecond*/
    uint32_t         video_size;
    uint32_t         throughput; /*bits/microsecond*/
    uint32_t         time_heci;
    uint32_t         time_dma_in;
    uint32_t         time_decrypt;
    uint32_t         time_parse;
    uint32_t         time_encrypt_dma_out;
} wv_dbg_get_diag_out;
//C_ASSERT(sizeof(wv_dbg_get_diag_out) == WV_USE_SATT ? 64 : 48);

/*wv_dbg_set_keybox*/
typedef struct {
    PAVP_CMD_HEADER  Header;
    wv_keybox        keybox;
} wv_dbg_set_keybox_in;

typedef PAVP_CMD_NODATA wv_dbg_set_keybox_out;

/*wv_dbg_reset_keybox*/
typedef PAVP_CMD_NODATA wv_dbg_reset_keybox_in;
typedef PAVP_CMD_NODATA wv_dbg_reset_keybox_out;

/*wv_dbg_set_xcript_key_wo_pavp*/
typedef PAVP_CMD_NODATA wv_dbg_set_xcript_key_wo_pavp_in;
typedef PAVP_CMD_NODATA wv_dbg_set_xcript_key_wo_pavp_out;

/*wv_dbg_reset_xcript_key_wo_pavp*/
typedef PAVP_CMD_NODATA wv_dbg_reset_xcript_key_wo_pavp_in;
typedef PAVP_CMD_NODATA wv_dbg_reset_xcript_key_wo_pavp_out;

#endif

