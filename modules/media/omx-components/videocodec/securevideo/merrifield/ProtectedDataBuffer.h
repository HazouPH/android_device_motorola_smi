/*
* Copyright (c) 2014 Intel Corporation.  All rights reserved.
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

#ifndef PROTECTED_DATA_BUFFER_H
#define PROTECTED_DATA_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: this size takes into account the space used by DRM
// schemes with full sample encryption (e.g., WV Classic) or
// subsample encryption (e.g., WV Modular, which uses 2KB for
// frame info data).
#define NALU_BUFFER_SIZE        (4 * 1024)

// Either start code + type (00 00 00 01 <type byte>) or 4 byte length + type.
#define NALU_HEADER_SIZE        5

// This should be able to fit compressed 1080p video I-frame, use half
// of NV12 1080p frame, which on average uses 12 bits per pixel.
#define MAX_COMPRESSED_FRAME_SIZE   (1920 * 1080 * 3 / 4)

#define MAX_PROT_BUFFER_DATA_SIZE  (MAX_COMPRESSED_FRAME_SIZE + NALU_BUFFER_SIZE)

#define MAX_PES_BUFFER_SIZE     (64*1024)

// TODO: it's not clear, how to calculate this value, since PES packet may contain
// less than 64KB worth of data.
#define MAX_PES_PACKETS_PER_FRAME   64

// Video decoder defines maximum number of NALUs per frame as 16.
// (At least, as of June of 2014.) Use the same value here.
#define MAX_NALUS_IN_FRAME      16

// Integer, which "PDBF", but no 0 terminator
#define PROTECTED_DATA_BUFFER_MAGIC   (0UL | ('F' << 24) | ('B' << 16) | ('D' << 8) | 'P')

#define DRM_SCHEME_NONE         0
#define DRM_SCHEME_WV_CLASSIC   1
#define DRM_SCHEME_WV_MODULAR   2
#define DRM_SCHEME_MCAST_SINK   3

// Flags to indicate if we need IED Transcription (needed for inplace encryption)
// Currently Widi Sink uses it to handle OMX Port reconfig scenario
#define PDB_FLAG_NEED_TRANSCRIPTION   0x10000000

#pragma pack(push, 4)

typedef struct ProtectedPESBuffer_tag {

    // AES CTR stream counter, needed for HDCP decryption.
    // If ProtectedDataBuffer::clear is 1, streamCounter is ignored.
    uint32_t streamCounter ;

    // AES CTR input counter, needed for HDCP decryption
    // If ProtectedDataBuffer::clear is 1, inputCounter is ignored.
    uint64_t inputCounter ;

    // Offset within ProtectedDataBuffer::data buffer, to the start
    // of this PES packet's data.
    //
    // IMPORTANT: for protected content (ProtectedDataBuffer::clear is 0),
    // this offset must be divisible by 16 (AES block size).  This is to allow
    // for in-place transcryption from AES CTR to IED (AES ECB).  OMX will
    // check that the offset is divisible by 16, and will abort
    // playback, if the offset is NOT divisible by 16.  For this reason,
    // the offset is used and not a byte pointer.
    uint32_t pesDataOffset ;

    // Size of the PES data, pointed to by pesData
    uint32_t pesSize ;
}
ProtectedPESBuffer ;

typedef struct ProtectedDataBuffer_tag {

    // Must be set to PROTECTED_DATA_BUFFER_MAGIC.  Must be the first
    // member of ProtectedDataBuffer structure.
    uint32_t magic;

    // See DRM_SCHEME_* defines above
    uint32_t drmScheme;

    // 1 if clear, 0 if encrypted
    uint32_t  clear;

    // Session ID, used by some DRM schemes (e.g. PlayReady)
    uint32_t session_id ;

    // Flags, used by some DRM schemes (e.g., PlayReady)
    uint32_t flags ;

    // Information about the PES data buffers.  Used for DRM_SCHEME_MCAST_SINK.
    // Reserve space for one more PES data buffer for sentinel value, for
    // ease of implementation.
    //
    ProtectedPESBuffer pesBuffers[MAX_PES_PACKETS_PER_FRAME + 1] ;

    // Number of filled-out entries in pesBuffers array.
    // Used for DRM_SCHEME_MCAST_SINK.  If data buffer is not partitioned
    // into PES packet buffers, set numPesBuffers must be 0.
    //
    uint32_t numPesBuffers ;

    // Size of the data buffer.
    uint32_t size ;

    // For clear content, this is the space for clear data.
    // For encrypted content, this space is occupied by IED encrypted
    // data or HDCP encrypted data (payloads only, no PES headers),
    // depending on the DRM scheme.
    //
    // A space is made at the end of encrypted data for
    // decrypted SPS/PPS headers.
    //
    // NOTE: data must be last, to allow for flexibility not
    // to copy the whole ProtectedDataBuffer, if not whole data
    // buffer is filled.
    //
    uint8_t data[MAX_PROT_BUFFER_DATA_SIZE];
}
ProtectedDataBuffer;

#pragma pack(pop)

#define PDBUFFER_DATA_OFFSET     offsetof(ProtectedDataBuffer, data)

static inline void Init_ProtectedDataBuffer(ProtectedDataBuffer* buf)
{
    // This is internal helper function.  If you pass invalid (e.g. NULL)
    // pointer to it, you deserve to crash.

    // Perform initialization of certain members, ignore the data
    // areas, which will be overwritten in the course of the
    // normal usage.

    buf->magic = PROTECTED_DATA_BUFFER_MAGIC ;
    buf->drmScheme = DRM_SCHEME_NONE ;
    buf->clear = 0 ;
    buf->size = 0 ;
    buf->numPesBuffers = 0 ;
    buf->session_id = 0 ;
    buf->flags = 0 ;
}
// End of Init_ProtectedDataBuffer()

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PROTECTED_DATA_BUFFER_H
