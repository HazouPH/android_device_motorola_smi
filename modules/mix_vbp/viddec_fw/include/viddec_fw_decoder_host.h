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

#ifndef VIDDEC_FW_DECODER_HOST_H
#define VIDDEC_FW_DECODER_HOST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "viddec_fw_common_defs.h"

    /** @weakgroup viddec Fw Decoder interface Functions */
    /** @ingroup viddec_fw_decoder */
    /*@{*/

    /**
       This function returns the size required for loading fw.
       @retval  size : Required size.
    */
    uint32_t viddec_fw_decoder_query_fwsize(void);

    /**
       This function loads Decoder Firmware and initialises necessary state information.
       @param[in] phys                : Physical address on where firmware should be loaded.
       @param[in] len                 : Length of data allocated at phys.
       @retval VIDDEC_FW_SUCCESS      : Successfully loaded firmware.
       @retval VIDDEC_FW_FAILURE      : Failed to communicate with firmware.
       @retval VIDDEC_FW_NORESOURCES  : Failed to allocate resources for Loading firmware.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
    */
    uint32_t viddec_fw_decoder_loadfw(uint32_t phys, uint32_t len);

    /**
       This function returns required size for global memory for all supported decoders. This is a synchronous message to FW.
       @param[out] size               : returns the size required.
       @retval VIDDEC_FW_SUCCESS      : Successfuly got required information from FW.
       @retval VIDDEC_FW_FAILURE      : Failed to communicate with firmware.
    */
    uint32_t viddec_fw_decoder_query_fwsize_scratchmem(uint32_t *size);

    /**
       This function sets global memory for the firmware to use.This is a synchronous message to FW.
       @param[in] phys                : Physical address on where global memory starts.
       @param[in] len                 : Length of data allocated at phys.
       @retval VIDDEC_FW_SUCCESS      : Successfully setup global memory.
       @retval VIDDEC_FW_FAILURE      : Failed to communicate with firmware.
    */
    uint32_t viddec_fw_decoder_set_fw_scratchmem(uint32_t phys, uint32_t len);

    /**
       This function returns the size required opening a stream. This a synchronous message to FW.
       @param[in] codec_type          : Type of codec that we want information about.
       @param[out] size               : Size of memory required for opening a stream.
       @retval VIDDEC_FW_SUCCESS      : Successfuly talked to FW and got required size.
       @retval VIDDEC_FW_FAILURE      : Failed to communicate with firmware.
    */
    uint32_t viddec_fw_decoder_query_streamsize(uint32_t codec_type, uint32_t *size);

    /**
       This function opens requested codec.This a synchronous message to FW.
       @param[in] codec_type          : Type of codec that we want to open.
       @param[in] phys                : Physical address of allocated memory for this codec.
       @param[in] prority             : Priority of stream. 1 for realtime and 0 for background.
       @param[out] strm_handle        : Handle of the opened stream.
       @retval VIDDEC_FW_SUCCESS      : Successfully Opened the stream.
       @retval VIDDEC_FW_FAILURE      : Failed to Open a stream.
    */
    uint32_t viddec_fw_decoder_openstream(uint32_t codec_type, uint32_t *strm_handle, uint32_t phys, uint32_t priority);


    /**
       This function closes stream.This a synchronous message to FW.
       @param[in] strm_handle        : Handle of the stream to close.
    */
    void viddec_fw_decoder_closestream(uint32_t strm_handle);

    /**
       This function allows to get current status of the decoder workload queues. If the current stream is active we return
       number of input messages that can be written to input queue and the number of messages in output queue of the stream.

       Normally this is called when Host receives an interrupt from decoder, In which case before releasing the INT
       Host will try its best to keep the FW busy. Normally when a interrupt is received it means at least one workload is
       written into output queue of a stream.
       @param[in] strm_handle         : The handle of stream that we want to get status of queues.
       @param[out] status             : The status of each queue gets updated in here.
       @retval VIDDEC_FW_SUCCESS      : Successfully Got the status information.
       @retval VIDDEC_FW_INVALID_PARAM: Invalid parameter in this case an inactive stream.
    */
    uint32_t viddec_fw_decoder_get_queue_status(uint32_t strm_handle, viddec_fw_decoder_q_status_t *status);

    /**
       This function flushes the current stream. This is a synchronous message to FW.
       Before calling this function the host has to make sure the output queue of the firmware
       is empty. After this function is executed the FW will read all entries in input
       wkld buffer queue into output queue. After this operation the host has to read all entries
       in output queue again to finish the flush operation.
       @param[in] flush_type          : Type of flush we want to perform.ex:flush and discard.
       @param[in]  strm_handle        : Handle of the stream we want to flush.
       @retval VIDDEC_FW_SUCCESS      : Successfully flushed the stream.
       @retval VIDDEC_FW_FAILURE      : Failed to flush a stream.
    */
    uint32_t viddec_fw_decoder_flushstream(uint32_t strm_handle, uint32_t flush_type);

    /**
       This function sends an input workload buffer. The host should provide required frame buffers in this workload before
       sending it to fw.
       @param[in] strm_handle         : The handle of stream that we want to send workload buffer to.
       @param[in] cur_wkld            : The workload buffer we want to send.
       @retval VIDDEC_FW_SUCCESS      : Successfully Sent the message.
       @retval VIDDEC_FW_PORT_FULL    : Port to fw full unsuccesful in sending message.
    */
    uint32_t viddec_fw_decoder_send(uint32_t strm_handle, ipc_msg_data *cur_wkld);

    /**
       This function gets the decoded workload from fw.
       @param[in] strm_handle         : The handle of stream that we want to read workload from.
       @param[out] cur_wkld           : The workload descriptor.
       @retval VIDDEC_FW_SUCCESS      : Successfully Sent the message.
       @retval VIDDEC_FW_PORT_EMPTY   : Workload port is empty,unsuccesful in reading wkld.
    */
    uint32_t viddec_fw_decoder_recv(uint32_t strm_handle, ipc_msg_data *cur_wkld);

    /**
       This function unloads Decoder Firmware and free's the resources allocated in Load fw.
       If this function is called before load fw it will crash with a segmentation fault.
    */
    void viddec_fw_decoder_deinit(void);

    /**
       This function gets the major and minor revison numbers of the loaded firmware.
       @param[out] major              : The major revision number.
       @param[out] minor              : The minor revision number.
       @param[out] build              : The Internal Build number.
    */
    void viddec_fw_decoder_get_version_number(unsigned int *major, unsigned int *minor, unsigned int *build);

    /**
       This function returns the interrupt status of all streams which need to be processed. A value of zero
       means no active streams which generated this interrupt.
    */
    uint32_t viddec_fw_decoder_active_pending_interrupts(void);

    /**
       This function clears the interrupts for all active streams represented by status input parameter.
       The status should always be a value that was returned by viddec_fw_decoder_active_pending_interrupts().
       @param[in] status              : The status value that was returned by viddec_fw_decoder_active_pending_interrupts().
    */
    void viddec_fw_decoder_clear_all_pending_interrupts(uint32_t status);

    /**
       This function enables/disables interrupt for the stream specified.
       @param[in] strm_handle         : The handle of stream that we want enable or disable interrupts for.
       @param[in] enable              : Boolean value if ==0 means disable Interrupts else enable.
       @retval VIDDEC_FW_SUCCESS      : Successfully Sent the message.
       @retval VIDDEC_FW_INVALID_PARAM: Invalid stream handle was passed.
    */
    uint32_t viddec_fw_decoder_set_stream_interrupt_mask(uint32_t stream_handle, uint32_t enable);

    /**
       This function returns which stream interrupted in the past based on status, which is a snapshot of
       interrupt status that was cleared in the past. The host has to call clear with status information
       before calling this function again with status value. The Host should do this operation until this function
       returns 0, which means all the streams that generated interrupt have been processed.
       @param[out]strm_handle         : The handle of a stream that generated interrupt.
       @param[in] status              : Snapshot of Interrupt status which was returned by viddec_fw_decoder_active_pending_interrupts().
       @retval 1                      : A valid stream handle was found.
       @retval 0                      : No more streams from the status which caused interrupt.
    */
    uint32_t viddec_fw_decoder_get_next_stream_from_interrupt_status(uint32_t status, uint32_t *stream_handle);

    /**
       This function clears the stream_handle from the status snapshot that we got from viddec_fw_decoder_active_pending_interrupts(),
       This should be called after host performs all necessary actions for the stream.
       @param[in] strm_handle         : The handle of a stream that we want to clear to indicate we handled it.
       @param[in] status              : Snapshot of Interrupt status which was returned by viddec_fw_decoder_active_pending_interrupts().
       @retval 1                      : Operation was sucessful.
       @retval 0                      : Invalid stream handle was passed.
    */
    uint32_t viddec_fw_decoder_clear_stream_from_interrupt_status(uint32_t *status, uint32_t stream_handle);

    /*@}*/
#ifdef __cplusplus
}
#endif

#endif//#ifndef VIDDEC_FW_DECODER_HOST_H
