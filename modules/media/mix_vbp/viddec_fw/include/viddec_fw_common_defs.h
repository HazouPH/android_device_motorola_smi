/*
    This file is provided under a dual BSD/GPLv2 license.  When using or
    redistributing this file, you may do so under either license.

    GPL LICENSE SUMMARY

    Copyright(c) 2007-2009 Intel Corporation. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of version 2 of the GNU General Public License as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
    The full GNU General Public License is included in this distribution
    in the file called LICENSE.GPL.

    Contact Information:

    BSD LICENSE

    Copyright(c) 2007-2009 Intel Corporation. All rights reserved.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
    * Neither the name of Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef VIDDEC_FW_COMMON_DEFS_H
#define VIDDEC_FW_COMMON_DEFS_H

#define VIDDEC_FW_PARSER_IPC_HOST_INT    0x87654321
#define EMITTER_WORKLOAD_ENTRIES    2048

/* Maximum supported dependent views for H264 MVC. Based on spec this can be 1023 */
#define MVC_MAX_SUPPORTED_VIEWS  1

/* This enum defines priority level for opening a stream */
enum viddec_stream_priority
{
    viddec_stream_priority_BACKGROUND, /* Lowest priority stream */
    viddec_stream_priority_REALTIME,   /* Real time highest priority stream */
    viddec_stream_priority_INVALID,
};

/* This enum defines supported flush types */
enum viddec_stream_flushtype
{
    VIDDEC_STREAM_FLUSH_DISCARD, /* Reinitialise to start state */
    VIDDEC_STREAM_FLUSH_PRESERVE, /* Reinitialise to start state  by preserving sequence info*/
};

enum viddec_stream_inband_flags
{
    VIDDEC_STREAM_DEFAULT_FLAG=0, /* Default value for flags */
    VIDDEC_STREAM_EOS,          /* End of stream message */
    VIDDEC_STREAM_DISCONTINUITY,  /* new segment which forces flush and preserve */
};

/* Message descriptor for Parser's Input and output queues. needs to be 8 byte aligned */
typedef struct viddec_input_buffer
{
    unsigned int             flags; /* Flags for Inband messages like EOS, valid range defined in viddec_stream_inband_flags */
    unsigned int             phys;/* DDR addr of where ES/WKLD is at. */
    unsigned int             len;/* size of buffer at phys_addr */
    unsigned int             id;/* A id for the buffer which is not used or modified by the FW. */
#ifdef HOST_ONLY
    unsigned char           *buf; /* virt pointer to buffer. This is a don't care for FW */
#endif
} ipc_msg_data;

typedef ipc_msg_data viddec_input_buffer_t;
typedef ipc_msg_data viddec_ipc_msg_data;

/* Return types for interface functions */
typedef enum
{
    VIDDEC_FW_SUCCESS, /* succesful with current operation */
    VIDDEC_FW_NORESOURCES, /* No resources to execute the requested functionality */
    VIDDEC_FW_FAILURE,    /* Failed for Uknown reason */
    VIDDEC_FW_INVALID_PARAM, /* The parameters that were passed are Invalid */
    VIDDEC_FW_PORT_FULL,     /* The operation failed since queue is full */
    VIDDEC_FW_PORT_EMPTY,   /* The operation failed since queue is empty */
    VIDDEC_FW_NEED_FREE_WKLD, /* The operation failed since a free wkld is not available */
} viddec_fw_return_types_t;

/* Defines for Interrupt mask and status */
typedef enum
{
    VIDDEC_FW_WKLD_DATA_AVAIL=1, /* A processed workload is available */
    VIDDEC_FW_INPUT_WATERMARK_REACHED=2,     /* The input path is below the set watermark for current stream */
} viddec_fw_parser_int_status_t;

/* Defines for attributes on stream, If not set explicitly will be default values */
typedef enum
{
    VIDDEC_FW_INPUT_Q_WATERMARK, /* Define for setting Input queue watermarks */
    VIDDEC_FW_STREAM_PRIORITY,    /* Define for setting stream priority */
} viddec_fw_stream_attributes_t;

typedef struct
{
    unsigned int input_q_space; /* Num of messages that can be written to input queue */
    unsigned int output_q_data; /* Num of messages in output queue */
    unsigned int workload_q_status; /* Number of free wklds available to parser */
} viddec_fw_q_status_t;

typedef struct
{
    unsigned int to_fw_q_space;     /* Num of messages that can be written to input queue */
    unsigned int from_fw_q_data;    /* Num of messages in output queue */
} viddec_fw_decoder_q_status_t;

enum viddec_fw_decoder_int_status
{
    VIDDEC_FW_DECODER_INT_STATUS_STREAM_0       = (1<< 0), /* Decoder Stream 0 Requires Service */
    VIDDEC_FW_DECODER_INT_STATUS_STREAM_1       = (1<< 1), /* Decoder Stream 1 Requires Service */
    VIDDEC_FW_DECODER_INT_STATUS_STREAM_2       = (1<< 2), /* Decoder Stream 2 Requires Service */


    VIDDEC_FW_DECODER_INT_STATUS_STREAM_HIGH    = (1<<30), /* Any Decoder Stream >= 30 Requires Service */
    VIDDEC_FW_DECODER_INT_STATUS_AUTO_API       = (1<<31)  /* An Auto-API Function has completed */
};

/** Hardware Accelerated stream formats */
typedef enum viddec_stream_format
{
    MFD_STREAM_FORMAT_MPEG=1,
    MFD_STREAM_FORMAT_H264,
    MFD_STREAM_FORMAT_VC1,
    MFD_STREAM_FORMAT_MPEG42,

    MFD_STREAM_FORMAT_MAX,   /* must be last  */
    MFD_STREAM_FORMAT_INVALID
} viddec_stream_format;

/* Workload specific error codes */
enum viddec_fw_workload_error_codes
{
    VIDDEC_FW_WORKLOAD_SUCCESS               = 0,
    VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE      = (1 << 0),/* Parser/Decoder detected a non decodable error with this workload */
    VIDDEC_FW_WORKLOAD_ERR_BUFFERS_OVERFLOW  = (1 << 1),/* Parser Detected more than 64 buffers between two start codes */
    VIDDEC_FW_WORKLOAD_ERR_ITEMS_OVERFLOW    = (1 << 2),/* Parser Detected overflow of currently allocated workload memory */
    VIDDEC_FW_WORKLOAD_ERR_FLUSHED_FRAME     = (1 << 3),/* This is impartial or empty frame which was flushed by Parser/Decoder */
    VIDDEC_FW_WORKLOAD_ERR_MISSING_DMEM      = (1 << 4),/* This is impartial or empty frame from Parser/Decoder */
    VIDDEC_FW_WORKLOAD_ERR_UNSUPPORTED       = (1 << 5),/* Parser Detected unsupported feature in the stream */
    /* First 8 bits reserved for Non Decodable errors */
    VIDDEC_FW_WORKLOAD_ERR_CONCEALED         = (1 << 9),/* The decoder concealed some errors in this frame */
    VIDDEC_FW_WORKLOAD_ERR_MISSING_REFERENCE = (1 << 10),/* Deocder/parser detected at least one of the required reference frames is missing */
    VIDDEC_FW_WORKLOAD_ERR_IN_REFERENCE      = (1 << 11),/* Deocder/parser detected at least one of the reference frames has errors in it */
    VIDDEC_FW_WORKLOAD_ERR_DANGLING_FLD      = (1 << 12),/* Parser detected at least one of the fields are missing */
    VIDDEC_FW_WORKLOAD_ERR_PARTIAL_SLICE     = (1 << 13),/* Deocder detected at least one of the fields are missing */
    VIDDEC_FW_WORKLOAD_ERR_MACROBLOCK        = (1 << 14),/* Deocder detected macroblock errors */
    VIDDEC_FW_WORKLOAD_ERR_MISSING_SEQ_INFO  = (1 << 16),/* Parser detected sequence information is missing */

    VIDDEC_FW_WORKLOAD_ERR_TOPFIELD          = (1 << 17),/* Decoder/Parser detected  errors in "top field" or "frame"*/
    VIDDEC_FW_WORKLOAD_ERR_BOTTOMFIELD       = (1 << 18),/* Decoder/Parser detected  errors in "bottom field" or "frame" */
    VIDDEC_FW_WORKLOAD_ERR_BITSTREAM_ERROR   = (1 << 19),/* Parser detected errors */

};

enum viddec_fw_mpeg2_error_codes
{
    VIDDEC_FW_MPEG2_ERR_CORRUPTED_SEQ_HDR       = (1 << 24),/* Parser detected corruption in sequence header. Will use the previous good sequence info, if found. */
    VIDDEC_FW_MPEG2_ERR_CORRUPTED_SEQ_EXT       = (1 << 25),/* Parser detected corruption in seqeunce extension. */
    VIDDEC_FW_MPEG2_ERR_CORRUPTED_SEQ_DISP_EXT  = (1 << 26),/* Parser detected corruption in sequence display extension. */
    VIDDEC_FW_MPEG2_ERR_CORRUPTED_GOP_HDR       = (1 << 27),/* Parser detected corruption in GOP header. */
    VIDDEC_FW_MPEG2_ERR_CORRUPTED_PIC_HDR       = (1 << 26),/* Parser detected corruption in picture header. */
    VIDDEC_FW_MPEG2_ERR_CORRUPTED_PIC_COD_EXT   = (1 << 27),/* Parser detected corruption in picture coding extension. */
    VIDDEC_FW_MPEG2_ERR_CORRUPTED_PIC_DISP_EXT  = (1 << 28),/* Parser detected corruption in picture display extension. */
    VIDDEC_FW_MPEG2_ERR_CORRUPTED_QMAT_EXT      = (1 << 29),/* Parser detected corruption in quantization matrix extension. */
};

#ifdef VBP

#ifndef NULL
#define NULL (void*)0x0
#endif

#ifndef true
#define true 1
#define false 0
#endif

#ifndef __cplusplus
#ifndef bool
typedef int bool;
#endif
#endif

#endif
/* end of #ifdef VBP */

#endif
