/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2005-2008 Intel Corporation. All rights reserved.

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
    Intel Corporation
    2200 Mission College Blvd.
    Santa Clara, CA  97052

  BSD LICENSE

  Copyright(c) 2005-2008 Intel Corporation. All rights reserved.

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
#ifndef VIDDEC_FW_PARSER_FW_IPC_H
#define VIDDEC_FW_PARSER_FW_IPC_H 1

#include "viddec_fw_parser_ipclib.h"

/** Generic Firmware-to-host Message Send Queue */
typedef struct
{
    struct IPC_MsgQueue         mq;     /* local MSGQueue handle */
} FW_IPC_SendQue;

/** Generic Host-to-Firmware Message Receive Queue */
typedef struct
{
    struct IPC_MsgQueue         mq;     /* local MSGQueue handle */
} FW_IPC_ReceiveQue;

typedef struct
{
    unsigned int state;
    unsigned int priority;
} FW_IPC_stream_info;

/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

typedef struct
{
    /** Synchronous Message Buffer, shared between host and firmware */
    volatile char                  *sync_msg_buf;

    /** WARNING: EACH OF THESE STRUCTS MUST BE 8 BYTE ALIGNED */
    FW_IPC_SendQue         snd_q[CONFIG_IPC_HOST_MAX_RX_QUEUES];

    /** WARNING: EACH OF THESE STRUCTS MUST BE 8 BYTE ALIGNED */
    FW_IPC_ReceiveQue      rcv_q[CONFIG_IPC_FW_MAX_RX_QUEUES];
    /** WARNING: EACH OF THESE STRUCTS MUST BE 8 BYTE ALIGNED */
    FW_IPC_ReceiveQue      wkld_q[CONFIG_IPC_FW_MAX_RX_QUEUES];

    /** FIRMWARE_TO_HOST Message Queues (outbound) */
    struct _IPC_QueueHeader        *snd_q_shared[CONFIG_IPC_HOST_MAX_RX_QUEUES];
    /** HOST_TO_FIRMWARE Message Queues (inbbound) */
    struct _IPC_QueueHeader        *rcv_q_shared[CONFIG_IPC_FW_MAX_RX_QUEUES];
    /** HOST_TO_FIRMWARE Message Queues (inbbound) */
    struct _IPC_QueueHeader        *wkld_q_shared[CONFIG_IPC_FW_MAX_RX_QUEUES];
    /** Actual qheaders allocated in FW memory */
    struct _IPC_QueueHeader         snd_qh[CONFIG_IPC_HOST_MAX_RX_QUEUES];
    struct _IPC_QueueHeader         rcv_qh[CONFIG_IPC_FW_MAX_RX_QUEUES];
    struct _IPC_QueueHeader         wkld_qh[CONFIG_IPC_FW_MAX_RX_QUEUES];

    /** Stream releated info like priority */
    FW_IPC_stream_info              strm_info[CONFIG_IPC_FW_MAX_RX_QUEUES];

    unsigned int                    one_msg_size;
    unsigned char                   one_msg[CONFIG_IPC_MESSAGE_MAX_SIZE];
} FW_IPC_Handle;

/*@}*/

/** @weakgroup Host IPC Functions */
/** @ingroup fw_ipc */
/*@{*/

/**
This function allows us to check and see if there's space available on the send queue(output) of fw
for the message of size(message_size). It also provides us the amount of space available.
@param[in] fwipc          : Ipc handle.
@param[in] message_size  : size of message that we want to write.
@param[out] bytes        : returns the amount of space available for writing.
@retval 0                : if space is not available for current message.
@retval 1                : if space is available for current message.
*/
int FwIPC_SpaceAvailForMessage(FW_IPC_Handle *fwipc, FW_IPC_SendQue *snd_q, unsigned int message_size, unsigned int *bytes);

/**
This function writes the message of message_size into queue(host_rx_queue).
@param[in] fwipc          : Ipc handle.
@param[in] host_rx_queue : id of the queue that needs to be written.
@param[in] message       : Message that we want to write.
@param[in] message_size  : size of message that we want to write.
@retval 0                : if write fails.
@retval 1                : if write succeeds.
*/
int FwIPC_SendMessage(FW_IPC_Handle *fwipc, unsigned int host_rx_queue, const char *message, unsigned int message_size );

/**
This function reads a message(which is <= max_message_size) from rcv_queue of firmware into input parameter message.
@param[in] fwipc             : Ipc handle.
@param[in] rcv_q            : Receive queue to read from.
@param[out] message          : Message that we want to read.
@param[in] max_message_size : max possible size of the message.
@retval                     : The size of message that was read.
*/
int FwIPC_ReadMessage(FW_IPC_Handle *fwipc, FW_IPC_ReceiveQue *rcv_q, char *message, unsigned int max_message_size );

/**
This function Initialises shared queue headers and sync command buffer for IPC.
@param[in] fwipc                       : Ipc handle.
@param[in] synchronous_command_buffer : update handle with pointer to shared memory
                                        between host and FW.
@retval 0                             : if write succeeds.
*/
int FwIPC_Initialize(FW_IPC_Handle *fwipc, volatile char *synchronous_command_buffer );

/**
This function Initialises Sendqueue with circular buffer which has actual data.
@param[in] fwipc                       : Ipc handle.
@param[in] snd_q                       : Send queue that needs to be initialized.
@param[in] snd_circbuf                 : Address of circular buffer.
*/
void FWIPC_SendQueue_Init(FW_IPC_Handle *fwipc, FW_IPC_SendQue *snd_q, void *snd_circbuf );

/**
This function Initialises Recvqueue with circular buffer which has actual data.
@param[in] fwipc                       : Ipc handle.
@param[in] rcv_q                       : Receive queue that needs to be initialized.
@param[in] rcv_circbuf                 : Address of circular buffer.
*/
void FwIPC_ReceiveQueue_Init(FW_IPC_Handle *fwipc, FW_IPC_ReceiveQue *rcv_q, void *rcv_circbuf );

/**
This function reads the nth(index) message(which is <= max_message_size ) from rcv_queue of firmware into input parameter message
by peeking the queue.
@param[in] fwipc             : Ipc handle.
@param[in] rcv_q            : Send queue to read from.
@param[out] message          : Message that we want to read.
@param[in] max_message_size : max possible size of the message.
@param[in] index            : nth message(index >=0).
@retval                     : The size of message that was read.
*/
int FwIPC_PeekReadMessage(FW_IPC_Handle *fwipc,  FW_IPC_ReceiveQue *rcv_q, char *message, unsigned int max_message_size, unsigned int index );

/*@}*/
#endif /* FW_IPC_H */
