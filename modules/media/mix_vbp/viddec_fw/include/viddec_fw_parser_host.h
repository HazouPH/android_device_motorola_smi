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

#ifndef VIDDEC_FW_PARSER_HOST_H
#define VIDDEC_FW_PARSER_HOST_H

#ifdef __cplusplus
extern "C" {
#endif
#include "viddec_fw_common_defs.h"

    /** @weakgroup viddec Fw Parser interface Functions */
    /** @ingroup viddec_fw_parser */
    /*@{*/

    /**
       This function returns the size required for loading fw.
       @retval  size : Required size.
    */
    uint32_t viddec_fw_parser_query_fwsize(void);

    /**
       This function loads Parser Firmware and initialises necessary state information.This a synchronous message to FW.
       @param[in] phys                : Physical address on where firmware should be loaded.
       @param[in] len                 : Length of data allocated at phys.
       @retval VIDDEC_FW_SUCCESS      : Successfully loaded firmware.
       @retval VIDDEC_FW_FAILURE      : Failed to communicate with firmware.
       @retval VIDDEC_FW_NORESOURCES  : Failed to allocate resources for Loading firmware.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
    */
    uint32_t viddec_fw_parser_loadfw(uint32_t phys, uint32_t len);

    /**
       This function returns the size required opening a stream. This a synchronous message to FW.
       @param[in] codec_type          : Type of codec that we want information about.
       @param[out] num_wklds          : Number of wklds required for initialisation.
       @param[out] size               : Size of memory required for opening a stream.
    */
    void viddec_fw_parser_query_streamsize(uint32_t codec_type, uint32_t *num_wklds, uint32_t *size);

    /**
       This function opens requested codec.This a synchronous message to FW.
       @param[in] codec_type          : Type of codec that we want to open.
       @param[in] phys                : Physical address of allocated memory for this codec.
       @param[in] prority             : Priority of stream. 1 for realtime and 0 for background.
       @param[out] strm_handle        : Handle of the opened stream.
       @retval VIDDEC_FW_SUCCESS      : Successfully Opened the stream.
       @retval VIDDEC_FW_FAILURE      : Failed to Open a stream.
       @retval VIDDEC_FW_NORESOURCES  : Failed to Open a stream as we are out of resources.
    */
    uint32_t viddec_fw_parser_openstream(uint32_t codec_type, uint32_t *strm_handle, uint32_t phys, uint32_t priority);

    /**
       This function closes stream.This a synchronous message to FW.
       For the close stream to be effective, host has to do flush with discard first and then close the stream.
       @param[in] strm_handle        : Handle of the stream to close.
    */
    void viddec_fw_parser_closestream(uint32_t strm_handle);

    /**
       This function flushes the current stream. This is a synchronous message to FW.
       Before calling this function the host has to make sure the output queue of the firmware
       is empty. After this function is executed the FW will read all entries in input
       es buffer queue into a free or partial workload and push it into output queue.
       After this operation the host has to read all entries in output queue again to
       finish the flush operation.
       @param[in] flush_type          : Type of flush we want to perform.ex:flush and discard.
       @param[in]  strm_handle        : Handle of the stream we want to flush.
       @retval VIDDEC_FW_SUCCESS      : Successfully flushed the stream.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
       @retval VIDDEC_FW_NEED_FREE_WKLD  : Failed to flush sice a free wkld was not available.
    */
    uint32_t viddec_fw_parser_flushstream(uint32_t strm_handle, uint32_t flush_type);

    /**
       This function sends an input es buffer.
       @param[in] strm_handle         : The handle of stream that we want to send es buffer to.
       @param[in] message             : The es buffer we want to send.
       @retval VIDDEC_FW_SUCCESS      : Successfully Sent the message.
       @retval VIDDEC_FW_PORT_FULL    : Port to fw full unsuccesful in sending message.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
    */
    uint32_t viddec_fw_parser_send(uint32_t strm_handle, ipc_msg_data *message);

    /**
       This function gets the next processed workload. The host is required to add free workloads
       to keep the parser busy. The FW will stall when it doesn't have enough workloads(2) to continue.
       @param[in] strm_handle         : The handle of stream that we want to read workload from.
       @param[out] message            : The workload descriptor.
       @retval VIDDEC_FW_SUCCESS      : Successfully Sent the message.
       @retval VIDDEC_FW_PORT_EMPTY   : Workload port is empty,unsuccesful in reading wkld.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
    */
    uint32_t viddec_fw_parser_recv(uint32_t strm_handle, ipc_msg_data *message);

    /**
       This function adds a free workload to current stream.
       @param[in] strm_handle         : The handle of stream that we want to write workload to.
       @param[out] message            : The workload descriptor.
       @retval VIDDEC_FW_SUCCESS      : Successfully Sent the message.
       @retval VIDDEC_FW_PORT_FULL    : Workload port is full,unsuccesful in writing wkld.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
    */
    uint32_t viddec_fw_parser_addwkld(uint32_t strm_handle, ipc_msg_data *message);

    /**
       This function enables or disables Interrupts for a stream. By default the FW will always enable interrupts.
       The driver can disable/enable Interrupts if it needs for this particular stream.

       @param[in] strm_handle         : The handle of stream that we want to get mask from
       @param[in] mask                : This is read as boolean variable, true to enable, false to disable.
       @retval VIDDEC_FW_SUCCESS      : Successfully set mask.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
    */
    uint32_t viddec_fw_parser_set_interruptmask(uint32_t strm_handle, uint32_t mask);
    /**
       This function gets the interrupt status for current stream.
       When the host gets Interrupted since its a global interrupt it's expected that host will look at all active streams,
       by calling this function. The status is what the FW thinks the current state of stream is. The status information that
       FW provides is complete information on all possible events that are defined. The host should only access this information
       in its ISR at which state FW doesn't modify this information.

       @param[in] strm_handle         : The handle of stream that we want to get mask from
       @param[out] status             : The status of the stream based on viddec_fw_parser_int_status_t enum.
       @retval VIDDEC_FW_SUCCESS      : Successfully in reading status.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
    */
    uint32_t viddec_fw_parser_getstatus(uint32_t strm_handle, uint32_t *status);

    /**
       This function allows to set stream attributes that are supported.
       @param[in] strm_handle         : The handle of stream that we want to set attribute on.
       @param[in] type                : The type of attribute we want to set, this should be one of items in viddec_fw_stream_attributes_t.
       @param[in] value               : The value of the type that we want to set.
       @retval VIDDEC_FW_SUCCESS      : Successfully Set the attribute.
       @retval VIDDEC_FW_INVALID_PARAM: The input parameters are not valid.
    */
    uint32_t viddec_fw_parser_set_stream_attributes(uint32_t strm_handle, uint32_t type, uint32_t value);

    /**
       This function allows to get current status of all the parser queues. If the current stream is active we return
       number of inout messages that can be written to input queue, no of messages in output queue and number of
       free available workloads the stream has.
       Normally this is called when Host receives an interrupt from parser, In which case before releasing the INT
       Host will try its best to keep the FW busy. We always get a interrupt if we passed the watermark on input or
       a workload was pushed into output and INT line is free. If host holds onto INT when firmware tries to send an INT
       FW would send the Interrupt after host releases INT. Since we have EDGE triggered interrupts we cannot guarantee
       one interrupt per frame, ex: If three frames are generated and after the first frame FW was able to provide an INT
       to host, but host held on to INT while the FW finished the next two frames, after host releases the INT the FW will
       give only one INT and host should try to empty output queue.
       @param[in] strm_handle         : The handle of stream that we want to get status of queues.
       @param[out] status             : The status of each queue gets updated in here.
       @retval VIDDEC_FW_SUCCESS      : Successfully Got the status information.
       @retval VIDDEC_FW_INVALID_PARAM: Invalid parameter in this case an inactive stream.
    */
    uint32_t viddec_fw_parser_get_queue_status(uint32_t strm_handle, viddec_fw_q_status_t *status);

    /**
       This function unloads Parser Firmware and free's the resources allocated in Load fw.
       If this function is called before load fw it will crash with a segmentation fault.
    */
    void viddec_fw_parser_deinit(void);

    /**
       This function gets the major and minor revison numbers of the loaded firmware.
       @param[out] major              : The major revision numner.
       @param[out] minor              : The minor revision number.
       @param[out] build              : The Internal Build number.
    */
    void viddec_fw_parser_get_version_number(unsigned int *major, unsigned int *minor, unsigned int *build);

    /**
       This function clears the global interrupt. This is the last thing host calls before exiting ISR.
    */
    void viddec_fw_parser_clear_global_interrupt(void);

    /*@}*/
#ifdef __cplusplus
}
#endif

#endif//#ifndef VIDDEC_FW_PARSER_HOST_H
