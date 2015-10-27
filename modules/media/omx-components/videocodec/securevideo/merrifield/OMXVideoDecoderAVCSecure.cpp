/*
* Copyright (c) 2009-2012 Intel Corporation.  All rights reserved.
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


//#define LOG_NDEBUG 0
#define LOG_TAG "OMXVideoDecoderAVCSecure"
#include <utils/Log.h>

#include <cutils/properties.h>

#include <time.h>
#include <signal.h>
#include <pthread.h>

extern "C" {
#include <sepdrm.h>
#include <hdcp2x_api.h>
#include <hdcp2x_error.h>
#include <fcntl.h>
#include <linux/psb_drm.h>
#include "xf86drm.h"
#include "xf86drmMode.h"
}

#include "OMXVideoDecoderAVCSecure.h"

#include "VideoFrameInfo.h"
#include "ProtectedDataBuffer.h"

#define INPORT_BUFFER_SIZE  sizeof(ProtectedDataBuffer)

#define ALIGN_16_BYTES(x) (((x) + 0xF) & (~0xF))

#define AES_BLOCK_SIZE  16

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif // min

// Be sure to have an equal string in VideoDecoderHost.cpp (libmix)
static const char* AVC_MIME_TYPE = "video/avc";
static const char* AVC_SECURE_MIME_TYPE = "video/avc-secure";

#define KEEP_ALIVE_INTERVAL             5 // seconds
#define DRM_KEEP_ALIVE_TIMER            1000000
#define WV_SESSION_ID                   0x00000011
#define FLUSH_WAIT_INTERVAL             (30 * 1000) //30 ms

#if LOG_NDEBUG == 0

#ifdef ANDROID
#define DUMP_EOL    ""
#else
#define LOGV    printf
#define LOGE    printf
#define DUMP_EOL    "\n"
#endif // ANDROID

static inline void Copy4Bytes(void* dst, const void* src)
{
    // Don't check input pointers for NULL: this is internal function,
    // and if you pass NULL to it, your code deserves to crash.

    uint8_t* bdst = (uint8_t*) dst ;
    const uint8_t* bsrc = (const uint8_t*) src ;

    *bdst++ = *bsrc++ ;
    *bdst++ = *bsrc++ ;
    *bdst++ = *bsrc++ ;
    *bdst = *bsrc ;
}
// End of Copy4Bytes()

static void DumpBufferToString(char* str, uint32_t strSize, const uint8_t* start, uint32_t size)
{
    char* s = str ;
    char* send = str + strSize ;

    const uint8_t* byte = start ;
    const uint8_t* end = start + size ;

    while (byte < end && s < send)
    {
        s += snprintf(s, strSize - (s - str), "%02x ", *byte) ;
        ++byte ;
    }
}
// End of DumpBufferToString()

static void DumpNaluDataBuffer(uint32_t nalu, const uint8_t* start, uint32_t size)
{
    const uint32_t STR_SIZE = 1024 ;
    char str[STR_SIZE] = {0} ;

    DumpBufferToString(str, STR_SIZE, start, size) ;

    LOGV("NALU-dump(nalu %u): data: %s" DUMP_EOL, nalu, str) ;
}
// End of DumpNaluDataBuffer()

static void DumpBuffer(const char* prefix, const uint8_t* start, uint32_t size)
{
    const uint32_t STR_SIZE = 1024 ;
    char str[STR_SIZE] = {0} ;

    DumpBufferToString(str, STR_SIZE, start, size) ;

    if (prefix == NULL)
    {
        prefix = "" ;
    }

    LOGV("%s: ptr=%p, size=%u, data=%s" DUMP_EOL, prefix, start, size, str) ;
}
// End of DumpBuffer()

static void DumpNaluHeaderBuffer(const uint8_t* const start, uint32_t size)
{
    if (start == NULL || size == 0)
    {
        return ;
    }

    const uint8_t* current = start ;

    uint32_t numNALUs = 0 ;
    Copy4Bytes(&numNALUs, current) ;
    current += sizeof(numNALUs) ;

    LOGV("NALU-dump: num NALUs = %u\n", numNALUs) ;

    if (numNALUs > MAX_NALUS_IN_FRAME)
    {
        LOGE("NALU-dump: ERROR, num NALUs is too big (%u)" DUMP_EOL, numNALUs) ;
    }

    for (uint32_t nalu = 0; nalu < numNALUs; ++nalu)
    {
        uint32_t imr_offset = 0 ;
        Copy4Bytes(&imr_offset, current) ;
        current += sizeof(imr_offset) ;

        uint32_t nalu_size = 0 ;
        Copy4Bytes(&nalu_size, current) ;
        current += sizeof(nalu_size) ;

        uint32_t data_size = 0 ;
        Copy4Bytes(&data_size, current) ;
        current += sizeof(data_size) ;

        LOGV("NALU-dump(nalu %u): imr_offset = %u, nalu_size = %u, data_size = %u" DUMP_EOL,
            nalu, imr_offset, nalu_size, data_size) ;

        DumpNaluDataBuffer(nalu, current, data_size) ;

        // Skip past the data
        current += data_size ;
    }
    // End of for
}
// End of DumpNaluHeaderBuffer()

const char* DrmSchemeToString(uint32_t drmScheme)
{
    switch(drmScheme)
    {
        case DRM_SCHEME_NONE:
            return "None" ;

        case DRM_SCHEME_WV_CLASSIC:
            return "WV Classic" ;

        case DRM_SCHEME_WV_MODULAR:
            return "WV Modular" ;

        case DRM_SCHEME_MCAST_SINK:
            return "MCast Sink" ;

        default:
            return "unknown" ;
    }
}
// End of DrmSchemeToString()

static void DumpBuffer2(const char* prefix, const uint8_t* start, uint32_t size)
{
    const uint32_t STR_SIZE = 1024 ;
    char str[STR_SIZE] = {0} ;

    DumpBufferToString(str, STR_SIZE, start, size) ;

    if (prefix == NULL)
    {
        prefix = "" ;
    }

    LOGV("%s%s" DUMP_EOL, prefix, str) ;
}
// End of DumpBuffer2()

static void DumpProtectedDataBuffer(const char* prefix, ProtectedDataBuffer* buf)
{
    const uint32_t MAX_BUFFER_DUMP_LENGTH = 32 ;

    if (buf == NULL)
    {
        return ;
    }

    if (prefix == NULL) { prefix = "" ; }

    if (buf->magic != PROTECTED_DATA_BUFFER_MAGIC)
    {
        const uint8_t* p = (uint8_t*) &buf->magic ;
        LOGV("%sWrong magic: %02x %02x %02x %02x" DUMP_EOL, prefix, p[0], p[1], p[2], p[3]) ;
        return ;
    }

    LOGV("%smagic: ok, drmScheme: %u (%s), clear: %u, size: %u, num PES: %u" DUMP_EOL, prefix,
        buf->drmScheme, DrmSchemeToString(buf->drmScheme), buf->clear, buf->size, buf->numPesBuffers) ;

    if (buf->numPesBuffers == 0)
    {
        uint32_t dumpLength = min(buf->size, MAX_BUFFER_DUMP_LENGTH) ;
        DumpBuffer2("data: ", buf->data, dumpLength) ;
    }
    else
    {
        for (uint32_t i = 0; i < buf->numPesBuffers; ++i)
        {
            const uint32_t STR_SIZE = 1024 ;
            char str[STR_SIZE] = {0} ;

            uint32_t dumpLength = min(buf->pesBuffers[i].pesSize, MAX_BUFFER_DUMP_LENGTH) ;

            DumpBufferToString(str, STR_SIZE,
                buf->data + buf->pesBuffers[i].pesDataOffset, dumpLength) ;

            LOGV("PES %u: streamCounter: %u, inputCounter: %llu, offset: %u, size: %u, PES data: %s" DUMP_EOL,
                i, buf->pesBuffers[i].streamCounter, buf->pesBuffers[i].inputCounter,
                buf->pesBuffers[i].pesDataOffset, buf->pesBuffers[i].pesSize, str) ;
        }
    }
}
// End of DumpProtectedDataBuffer

#else

// Avoid #ifdef around the dump code

#define DumpBuffer(...)
#define DumpBuffer2(...)
#define DumpNaluHeaderBuffer(...)
#define DumpProtectedDataBuffer(...)

#define DUMP_EOL

#endif // LOG_NDEBUG == 0

OMXVideoDecoderAVCSecure::OMXVideoDecoderAVCSecure()
    : mNumInportBuffers(0)
    , mKeepAliveTimer(0)
    , mSessionPaused(false)
{
    LOGV("constructor");

    // Override default native buffer count defined in the base class
    mNativeBufferCount = OUTPORT_NATIVE_BUFFER_COUNT;

    // If we are loaded for Miracast Sink support, adjust the buffer count
    char prop[PROPERTY_VALUE_MAX];
    if ((property_get("media.widi.sink.enabled", prop, NULL) > 0) &&
        (!strcmp(prop, "1") || !strcasecmp(prop, "true")))
    {
        LOGI("%s: DRM_SCHEME_MCAST_SINK is enabled, increase Output port(+2)", __FUNCTION__);
        mNativeBufferCount = OUTPORT_NATIVE_BUFFER_COUNT + 2;
    }

    BuildHandlerList();
}

OMXVideoDecoderAVCSecure::~OMXVideoDecoderAVCSecure()
{
    LOGV("destructor");
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput)
{
    if (paramPortDefinitionInput == NULL)
    {
        return OMX_ErrorBadParameter ;
    }

    const char* mime = AVC_SECURE_MIME_TYPE ;
    mVideoDecoder = createVideoDecoder(mime);
    if (!mVideoDecoder) {
        LOGE("%s: createVideoDecoder failed for \"%s\"", __FUNCTION__, mime);
        return OMX_ErrorNotReady ;
    }
    LOGV("%s: created video decoder for protected content", __FUNCTION__) ;

    // OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionInput->nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
    char prop[PROPERTY_VALUE_MAX];
    if ((property_get("media.widi.sink.enabled", prop, NULL) > 0) &&
        (!strcmp(prop, "1") || !strcasecmp(prop, "true")))
    {
        LOGI("%s: DRM_SCHEME_MCAST_SINK is enabled", __FUNCTION__);
        paramPortDefinitionInput->nBufferCountActual = INPORT_MIN_BUFFER_COUNT;
    }

    LOGI("%s: nBufferCountActual = %lu", __FUNCTION__, paramPortDefinitionInput->nBufferCountActual) ;

    paramPortDefinitionInput->nBufferCountMin = INPORT_MIN_BUFFER_COUNT;
    paramPortDefinitionInput->nBufferSize = INPORT_BUFFER_SIZE;
    paramPortDefinitionInput->format.video.cMIMEType = (OMX_STRING)AVC_MIME_TYPE;
    paramPortDefinitionInput->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;

    // OMX_VIDEO_PARAM_AVCTYPE
    memset(&mParamAvc, 0, sizeof(mParamAvc));
    SetTypeHeader(&mParamAvc, sizeof(mParamAvc));
    mParamAvc.nPortIndex = INPORT_INDEX;
    // TODO: check eProfile/eLevel
    mParamAvc.eProfile = OMX_VIDEO_AVCProfileHigh; //OMX_VIDEO_AVCProfileBaseline;
    mParamAvc.eLevel = OMX_VIDEO_AVCLevel41; //OMX_VIDEO_AVCLevel1;

    this->ports[INPORT_INDEX]->SetMemAllocator(MemAllocDataBuffer, MemFreeDataBuffer, this);

    return OMX_ErrorNone;
}
// End of OMXVideoDecoderAVCSecure::InitInputPortFormatSpecific()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorInit(void)
{
    mSessionPaused = false;
    return OMXVideoDecoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorDeinit(void)
{
    // Session should be torn down in ProcessorStop, delayed to ProcessorDeinit
    // to allow remaining frames completely rendered.
    switch (mDrmScheme)
    {
        case DRM_SCHEME_WV_CLASSIC:
        {
            uint32_t sepres = drm_destroy_session(WV_SESSION_ID);
            LOGW_IF(sepres != 0,
                "%s: WV Classic: Drm_DestroySession returned %#x", __FUNCTION__, sepres);
        }
        break ;

        case DRM_SCHEME_WV_MODULAR:
        {
            // TODO: WV_SESSION_ID is not appropriate for WV Modular DRM.
            // Need to use real session ID.
            uint32_t ret = drm_wv_mod_stop_playback(WV_SESSION_ID);
            LOGW_IF(ret != DRM_WV_MOD_SUCCESS,
                "%s: WV Modular: drm_wv_mod_stop_playback returned %#x", __FUNCTION__, ret);
        }
        break;

        case DRM_SCHEME_MCAST_SINK:
        {
            // Note: IED session is descroyed by HDCP authentication code in
            // WiDi stack.
        }
        break ;

        default:
            LOGW("%s: trying to deinit unknown DRM scheme %u", __FUNCTION__,  mDrmScheme) ;
    }
    // End of switch

    mDrmScheme = DRM_SCHEME_NONE ;

    return OMXVideoDecoderBase::ProcessorDeinit();
}
// End of OMXVideoDecoderAVCSecure::ProcessorDeinitf()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorStart(void) {

    mSessionPaused = false;
    return OMXVideoDecoderBase::ProcessorStart();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorStop(void) {
    if (mKeepAliveTimer != 0) {
        timer_delete(mKeepAliveTimer);
        mKeepAliveTimer = 0;
    }

    return OMXVideoDecoderBase::ProcessorStop();
}


OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorFlush(OMX_U32 portIndex) {
    return OMXVideoDecoderBase::ProcessorFlush(portIndex);
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorProcess(
        OMX_BUFFERHEADERTYPE ***pBuffers,
        buffer_retain_t *retains,
        OMX_U32 numberBuffers) {

    int ret_value;

    OMX_BUFFERHEADERTYPE *pInput = *pBuffers[INPORT_INDEX];

    // TODO: check magic
    ProtectedDataBuffer *dataBuffer = (ProtectedDataBuffer *)pInput->pBuffer;

    if((dataBuffer->drmScheme == DRM_SCHEME_WV_CLASSIC) && (!mKeepAliveTimer)){
        struct sigevent sev;
        memset(&sev, 0, sizeof(sev));
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_value.sival_ptr = this;
        sev.sigev_notify_function = KeepAliveTimerCallback;

        ret_value = timer_create(CLOCK_REALTIME, &sev, &mKeepAliveTimer);
        if (ret_value != 0) {
            LOGE("Failed to create timer.");
        } else {
            struct itimerspec its;
            its.it_value.tv_sec = -1; // never expire
            its.it_value.tv_nsec = 0;
            its.it_interval.tv_sec = KEEP_ALIVE_INTERVAL;
            its.it_interval.tv_nsec = 0;

            ret_value = timer_settime(mKeepAliveTimer, TIMER_ABSTIME, &its, NULL);
            if (ret_value != 0) {
                LOGE("Failed to set timer.");
            }
        }
    }

    if (dataBuffer->size == 0) {
        // error occurs during decryption.
        LOGW("%s: size of returned data buffer is 0, decryption fails, flushing all ports.", __FUNCTION__);
        mVideoDecoder->flush();
        usleep(FLUSH_WAIT_INTERVAL);
        OMX_BUFFERHEADERTYPE *pOutput = *pBuffers[OUTPORT_INDEX];
        pOutput->nFilledLen = 0;
        // reset Data buffer size
        dataBuffer->size = INPORT_BUFFER_SIZE;
        this->ports[INPORT_INDEX]->FlushPort();
        this->ports[OUTPORT_INDEX]->FlushPort();
        return OMX_ErrorNone;
    }

    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::ProcessorProcess(pBuffers, retains, numberBuffers);
    if (ret != OMX_ErrorNone) {
        LOGE("OMXVideoDecoderBase::ProcessorProcess failed. Result: %#x", ret);
        return ret;
    }

    if (mSessionPaused && (retains[OUTPORT_INDEX] == BUFFER_RETAIN_GETAGAIN)) {
        retains[OUTPORT_INDEX] = BUFFER_RETAIN_NOT_RETAIN;
        OMX_BUFFERHEADERTYPE *pOutput = *pBuffers[OUTPORT_INDEX];
        pOutput->nFilledLen = 0;
        this->ports[INPORT_INDEX]->FlushPort();
        this->ports[OUTPORT_INDEX]->FlushPort();
    }

    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorPause(void) {
    return OMXVideoDecoderBase::ProcessorPause();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorResume(void) {
    return OMXVideoDecoderBase::ProcessorResume();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareConfigBuffer(VideoConfigBuffer *p) {
    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::PrepareConfigBuffer(p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareConfigBuffer");

    // Since using protected video decoder, always enable surface
    // protection flag
    p->flag |=  WANT_SURFACE_PROTECTION;

    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareClassicWVDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p){

    if (buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
    {
        // For this DRM scheme, the config data comes encrypted in the stram.
        // We should ignore anything else that claims to be codec data.
        LOGW("%s: ignoring codec data buffer", __FUNCTION__) ;
        return OMX_ErrorNone ;
    }

   OMX_ERRORTYPE ret = OMX_ErrorNone;

   p->flag |= HAS_COMPLETE_FRAME;

   if (buffer->nOffset != 0) {
       LOGW("buffer offset %lu is not zero!!!", buffer->nOffset);
   }

   ProtectedDataBuffer *dataBuffer = (ProtectedDataBuffer *)buffer->pBuffer;
   if (dataBuffer->clear) {
       p->data = dataBuffer->data + buffer->nOffset;
       p->size = buffer->nFilledLen;
   } else {
       dataBuffer->size = NALU_BUFFER_SIZE;
       struct drm_wv_nalu_headers nalu_headers;
       nalu_headers.p_enc_ciphertext = dataBuffer->data;

       nalu_headers.hdrs_buf_len = NALU_BUFFER_SIZE;
       nalu_headers.frame_size = buffer->nFilledLen;
       // Make sure that NALU header frame size is 16 bytes aligned
       nalu_headers.frame_size = (nalu_headers.frame_size + 0xF) & (~0xF);

       // Use same video buffer to fill NALU headers returned by chaabi,
       // Adding 4 because the first 4 bytes after databuffer will be used to store length of NALU headers
       if((nalu_headers.frame_size + NALU_BUFFER_SIZE) > INPORT_BUFFER_SIZE){
           LOGE("Not enough buffer for NALU headers");
           return OMX_ErrorOverflow;
       }

       nalu_headers.p_hdrs_buf = (uint8_t *)(dataBuffer->data + nalu_headers.frame_size + 4);
       nalu_headers.parse_size = buffer->nFilledLen;

       uint32_t res = drm_wv_return_naluheaders(WV_SESSION_ID, &nalu_headers);
       if (res == DRM_FAIL_FW_SESSION) {
           LOGW("Drm_WV_ReturnNALUHeaders failed. Session is disabled.");
           mSessionPaused = true;
           ret =  OMX_ErrorNotReady;
       } else if (res != 0) {
           mSessionPaused = false;
           LOGE("Drm_WV_ReturnNALUHeaders failed. Error = %#x, frame_size: %d, len = %lu", res, nalu_headers.frame_size, buffer->nFilledLen);
           ret = OMX_ErrorHardware;
       } else {
           mSessionPaused = false;

           // If chaabi returns 0 NALU headers fill the frame size to zero.
           if (!nalu_headers.hdrs_buf_len) {
               p->size = 0;
               return ret;
           }
           else{
               DumpBuffer2("OMX: WV Classic NALU header data: ",
                dataBuffer->data + nalu_headers.frame_size + 4, nalu_headers.hdrs_buf_len) ;
               // NALU headers are appended to encrypted video bitstream
               // |...encrypted video bitstream (16 bytes aligned)...| 4 bytes of header size |...NALU headers..|
               uint32_t *ptr = (uint32_t*)(dataBuffer->data + nalu_headers.frame_size);
               *ptr = nalu_headers.hdrs_buf_len;
               p->data = dataBuffer->data;
               p->size = nalu_headers.frame_size;
               p->flag |= IS_SECURE_DATA;
           }
       }
   }

   // reset Data size
   dataBuffer->size = NALU_BUFFER_SIZE;
   return ret;
}
// End of OMXVideoDecoderAVCSecure::PrepareClassicWVDecodeBuffer()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareModularWVDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p){
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    // OMX_BUFFERFLAG_CODECCONFIG is an optional flag
    // if flag is set, buffer will only contain codec data.
    // For WV Modular, we must not ignore the codec data.
    if (buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        LOGI("WV Modular: received AVC codec data.");
    }

    p->flag |= HAS_COMPLETE_FRAME | IS_SUBSAMPLE_ENCRYPTION;

    if (buffer->nOffset != 0) {
        LOGW("buffer offset %lu is not zero!!!", buffer->nOffset);
    }

    ProtectedDataBuffer *dataBuffer = (ProtectedDataBuffer *)buffer->pBuffer;
    p->data = dataBuffer->data;
    p->size = sizeof(frame_info_t);
    p->flag |= IS_SECURE_DATA;
    return ret;
}
// End of OMXVideoDecoderAVCSecure::PrepareModularWVDecodeBuffer()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareMCastSinkWVDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p)
{
    if (buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
    {
        // For this DRM scheme, the config data comes encrypted in the stram.
        // We should ignore anything else that claims to be codec data.
        LOGW("%s: ignoring codec data buffer", __FUNCTION__) ;
        return OMX_ErrorNone ;
    }

    if (buffer->nOffset != 0)
    {
        LOGE("%s: error: buffer offset %lu is not zero", __FUNCTION__, buffer->nOffset);
        return OMX_ErrorBadParameter ;
    }

    ProtectedDataBuffer *dataBuffer = (ProtectedDataBuffer *)buffer->pBuffer;
    // NOTE: magic was already checked by the caller

    if (dataBuffer->numPesBuffers > MAX_PES_PACKETS_PER_FRAME)
    {
        LOGE("%s: numPesBuffers = %u, but maximum is %u" DUMP_EOL,
            __FUNCTION__, dataBuffer->numPesBuffers, MAX_PES_PACKETS_PER_FRAME) ;
        return OMX_ErrorBadParameter ;
    }

    LOGV("%s: dataBuffer->clear = %u, dataBuffer->size = %u",
        __FUNCTION__, dataBuffer->clear, dataBuffer->size) ;

    if (dataBuffer->clear)
    {
        // TODO: simplify this
        LOGV("%s: encrypting clear content to IED", __FUNCTION__) ;

        const uint32_t alignedDataSize = ALIGN_16_BYTES(dataBuffer->size) ;

        uint32_t dumpLength = (dataBuffer->size > 64 ? 64 : dataBuffer->size) ;
        DumpBuffer2("OMX: clear data, up to 64 bytes: ", dataBuffer->data, dumpLength) ;

        // The data must be padded with 0, otherwise the video decoder may
        // choke on this data.
        memset(dataBuffer->data + dataBuffer->size, 0, alignedDataSize - dataBuffer->size);

        LOGV("%s: zeroed %u bytes at address %p",
            __FUNCTION__, alignedDataSize - dataBuffer->size,
            dataBuffer->data + dataBuffer->size) ;

        uint32_t result = 0;

        // Condition to address the Format change where OMX retains the input buffer
        // and after re-allocation tries to process the buffer again.
        // Since it is inplace IED decryption we dont have original data.
        // This flag is set in WiDi HDCP component while filing the buffer
        if (dataBuffer->flags & PDB_FLAG_NEED_TRANSCRIPTION) {
            result = hdcp2x_rx_decrypt_pes(
                    HDCP_CLEAR_CONTENT_IED_ENCRYPT_FLAG, 0, 0,
                    dataBuffer->data, alignedDataSize,
                    dataBuffer->data, alignedDataSize) ;
            if (result != HDCP_SUCCESS)
            {
                LOGE("%s: error: hdcp2x_rx_decrypt_pes(HDCP_CLEAR_CONTENT_IED_ENCRYPT_FLAG"
                        "streamCounter = 0, inputCounter = 0, "
                        "data = %p, alignedDataSize = %u) returned %#x",
                        __FUNCTION__, dataBuffer->data, alignedDataSize, result) ;
                return OMX_ErrorHardware ;
            }
            // Update the flags to indicate IED transcription is done
            dataBuffer->flags &= ~PDB_FLAG_NEED_TRANSCRIPTION;
        } else {
            LOGI("Skipping IED encryption as frame is already IED transcripted...\n");
        }

        DumpBuffer2("OMX: IED encrypted data, up to 64 bytes: ", dataBuffer->data, dumpLength) ;

        // Do we have space in the data buffer for headers?  Account
        // for the aligned data size, maximum NALU buffer size, and
        // 4 bytes used to store the length of the data in NALU buffer.
        uint32_t maxDataBufferSize = alignedDataSize + sizeof(uint32_t)
            + NALU_BUFFER_SIZE ;
        if (maxDataBufferSize > MAX_PROT_BUFFER_DATA_SIZE)
        {
            LOGE("%s: maximum required buffer space %u exceeds data buffer capacity %u",
                __FUNCTION__, maxDataBufferSize, MAX_PROT_BUFFER_DATA_SIZE) ;
            return OMX_ErrorOverflow ;
        }

        uint8_t* encryptedData = dataBuffer->data ;
        uint32_t* headerSizeLocation = (uint32_t*) (encryptedData + alignedDataSize) ;
        uint32_t encryptedDataSize = alignedDataSize ;

        // Header data starts after aligned encrypted data, skipping 4 bytes
        // for the location to receive header data size.
        uint8_t* headerData = encryptedData + alignedDataSize + sizeof(uint32_t) ;

        LOGV("%s: encryptedData = %p, encryptedDataSize = %u, "
            "alignedDataSize = %u, headerSizeLocation = %p, "
            "headerData = %p", __FUNCTION__,
            encryptedData, encryptedDataSize,
            alignedDataSize, headerSizeLocation, headerData) ;

        uint32_t headerDataSize = min(NALU_BUFFER_SIZE, alignedDataSize) ;

        result = hdcp2x_rx_return_nalu(encryptedData, alignedDataSize,
            headerDataSize, headerData, &headerDataSize) ;
        if (result != HDCP_SUCCESS)
        {
            LOGE("%s: error: hdcp2x_rx_return_nalu(encryptedData = %p, "
                "alignedDataSize = %u, parse_size = %u, "
                "headerData = %p, headerDataSize = %u), returned %#x",
                __FUNCTION__, encryptedData, alignedDataSize, headerDataSize,
                headerData, headerDataSize, result) ;
            return OMX_ErrorHardware ;
        }

        DumpBuffer2("OMX: SPS/PPS header data: ", headerData, headerDataSize) ;
        LOGV("header data size = %u", headerDataSize) ;

        *headerSizeLocation = headerDataSize ;

        p->data = encryptedData ;
        p->size = alignedDataSize ;
        p->flag |= HAS_COMPLETE_FRAME;
        p->flag |= IS_SECURE_DATA;

        // End of handling clear data
    }
    else
    {
        if (dataBuffer->numPesBuffers == 0)
        {
            LOGE("%s: numPesBuffers is 0, which is incorrect for encrypted buffer" DUMP_EOL,
                __FUNCTION__) ;
            return OMX_ErrorBadParameter ;
        }
        else if (dataBuffer->numPesBuffers == 1)
        {
            LOGV("%s: numPesBuffers is 1, use combined Chaabi API for transcryption and parsing" DUMP_EOL,
                __FUNCTION__) ;

            const uint32_t pesDataOffset = dataBuffer->pesBuffers[0].pesDataOffset ;

            // PES data must start on AES block size aligned offset
            if (pesDataOffset % AES_BLOCK_SIZE != 0)
            {
                LOGE("%s: PES buffer start offset %u is not AES block aligned",
                    __FUNCTION__, pesDataOffset) ;
                return OMX_ErrorBadParameter ;
            }

            const uint32_t encryptedDataSize = dataBuffer->pesBuffers[0].pesSize ;
            const uint32_t alignedDataSize = ALIGN_16_BYTES(encryptedDataSize) ;

            // Do we have space in the data buffer for headers?  Account
            // for the aligned data size, maximum NALU buffer size, and
            // 4 bytes used to store the length of the data in NALU buffer.
            uint32_t maxDataBufferSize = alignedDataSize + sizeof(uint32_t)
                + NALU_BUFFER_SIZE ;
            if (maxDataBufferSize > MAX_PROT_BUFFER_DATA_SIZE)
            {
                LOGE("%s: maximum required buffer space %u exceeds data buffer capacity %u",
                    __FUNCTION__, maxDataBufferSize, MAX_PROT_BUFFER_DATA_SIZE) ;
                return OMX_ErrorOverflow ;
            }

            uint8_t* encryptedData = dataBuffer->data + pesDataOffset ;
            uint32_t* headerSizeLocation = (uint32_t*) (encryptedData + alignedDataSize) ;

            // Header data starts after aligned encrypted data, skipping 4 bytes
            // for the location to receive header data size.
            uint8_t* headerData = encryptedData + alignedDataSize + sizeof(uint32_t) ;

            LOGV("%s: encryptedData = %p, encryptedDataSize = %u, "
                "alignedDataSize = %u, headerSizeLocation = %p, "
                "headerData = %p", __FUNCTION__,
                encryptedData, encryptedDataSize,
                alignedDataSize, headerSizeLocation, headerData) ;

            uint32_t headerDataSize = NALU_BUFFER_SIZE ;

            uint32_t dumpLength = (encryptedDataSize > 64 ? 64 : encryptedDataSize) ;
            DumpBuffer2("OMX: HDCP encrypted data, up to 64 bytes: ", encryptedData, dumpLength) ;

            // The data must be padded with 0, otherwise the video decoder may
            // choke on this data.
            memset(encryptedData + encryptedDataSize, 0, alignedDataSize - encryptedDataSize);

            LOGV("%s: zeroed %u bytes at address %p",
                __FUNCTION__, alignedDataSize - encryptedDataSize,
                encryptedData + encryptedDataSize) ;

            // Condition to address the Format change where OMX retains the input buffer
            // and after re-allocation tries to process the buffer again.
            // Since it is inplace IED decryption we dont have original data.
            // This flag is set in WiDi HDCP component while filing the buffer
            if (dataBuffer->flags & PDB_FLAG_NEED_TRANSCRIPTION) {
                uint32_t result = hdcp2x_rx_decrypt(
                        dataBuffer->pesBuffers[0].streamCounter,
                        dataBuffer->pesBuffers[0].inputCounter,

                        // Input data, encrypted with HDCP
                        encryptedData, encryptedDataSize,

                        // Output data, encrypted with IED (in-place decryption)
                        encryptedData, alignedDataSize,

                        headerData, &headerDataSize) ;
                if (result != HDCP_SUCCESS)
                {
                    LOGE("%s: error: hdcp2x_rc_decrypt(streamCounter = %u, inputCounter = %llu, "
                            "encryptedData = %p, encryptedDataSize = %u, alignedDataSize = %u, "
                            "headerData = %p) returned %x",
                            __FUNCTION__, dataBuffer->pesBuffers[0].streamCounter,
                            dataBuffer->pesBuffers[0].inputCounter,
                            encryptedData, encryptedDataSize, alignedDataSize,
                            headerData, result) ;
                    return OMX_ErrorHardware ;
                }

                // Update the flags to indicate IED transcription is done
                dataBuffer->flags &= ~PDB_FLAG_NEED_TRANSCRIPTION;
            } else {
                LOGI("Skipping HDCP Decryption as frame is already IED transcripted...\n");
            }
            DumpBuffer2("OMX: IED encrypted data, up to 64 bytes: ", encryptedData, dumpLength) ;
            DumpBuffer2("OMX: SPS/PPS header data: ", headerData, headerDataSize) ;
            LOGV("header data size = %u", headerDataSize) ;

            *headerSizeLocation = headerDataSize ;

            p->data = encryptedData ;
            p->size = alignedDataSize ;
            p->flag |= HAS_COMPLETE_FRAME;
        }
        else
        {
            LOGE("%s: numPesBuffers is %u, this support is not yet implemented" DUMP_EOL,
                __FUNCTION__, dataBuffer->numPesBuffers) ;
            return OMX_ErrorNotImplemented ;
        }

        p->flag |= IS_SECURE_DATA;
    }
    // End of "if clear or protected"

    return OMX_ErrorNone ;
}
// End of OMXVideoDecoderAVCSecure::PrepareMCastSinkWVDecodeBuffer()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p)
{
    OMX_ERRORTYPE ret = OMXVideoDecoderBase::PrepareDecodeBuffer(buffer, retain, p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareDecodeBuffer");

    if (buffer->nFilledLen == 0)
    {
        LOGI("%s: nFilledLen is 0, skipping further processing of this buffer", __FUNCTION__) ;
        return OMX_ErrorNone;
    }

    ProtectedDataBuffer *dataBuffer = (ProtectedDataBuffer *)buffer->pBuffer;

    // Check that we are dealing with the right buffer
    if (dataBuffer->magic != PROTECTED_DATA_BUFFER_MAGIC)
    {
        if (buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
        {
            // Processing codec data, which is not in ProtectedDataBuffer format
            LOGV("%s: received AVC codec data (%lu bytes).", __FUNCTION__, buffer->nFilledLen);
            DumpBuffer2("OMX: AVC codec data: ", buffer->pBuffer, buffer->nFilledLen) ;
            return OMX_ErrorNone ;
        }
        else
        {
            // Processing non-codec data, but this buffer is not in ProtectedDataBuffer format
            LOGE("%s: protected data buffer pointer %p doesn't have the right magic", __FUNCTION__, dataBuffer) ;
            return OMX_ErrorBadParameter ;
        }
    }
    // End of magic check

    // TODO: "Getting awful crowded in my sky."  Restructure in similar way, how
    // CTP OMX code was restructured.
    switch (dataBuffer->drmScheme)
    {
        case DRM_SCHEME_WV_CLASSIC:
        {
            mDrmScheme = DRM_SCHEME_WV_CLASSIC;
            return PrepareClassicWVDecodeBuffer(buffer, retain, p);
        }
        break ;

        case DRM_SCHEME_WV_MODULAR:
        {
            mDrmScheme = DRM_SCHEME_WV_MODULAR;
            return PrepareModularWVDecodeBuffer(buffer, retain, p);
        }
        break ;

        case DRM_SCHEME_MCAST_SINK:
        {
            mDrmScheme = DRM_SCHEME_MCAST_SINK;
            return PrepareMCastSinkWVDecodeBuffer(buffer, retain, p) ;
        }
        break ;

        default:
        {
            mDrmScheme = DRM_SCHEME_NONE ;
            LOGE("%s: invalid DRM scheme %u", __FUNCTION__, dataBuffer->drmScheme) ;
            return OMX_ErrorBadParameter ;
        }
    }
    // End of switch

    return OMX_ErrorNone;
}
// End of OMXVideoDecoderAVCSecure::PrepareDecodeBuffer()

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::BuildHandlerList(void) {
    OMXVideoDecoderBase::BuildHandlerList();
    AddHandler(OMX_IndexParamVideoAvc, GetParamVideoAvc, SetParamVideoAvc);
    AddHandler(OMX_IndexParamVideoProfileLevelQuerySupported, GetParamVideoAVCProfileLevel, SetParamVideoAVCProfileLevel);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::GetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);

    memcpy(p, &mParamAvc, sizeof(*p));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::SetParamVideoAvc(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_AVCTYPE *p = (OMX_VIDEO_PARAM_AVCTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);
    CHECK_SET_PARAM_STATE();

    // TODO: do we need to check if port is enabled?
    // TODO: see SetPortAvcParam implementation - Can we make simple copy????
    memcpy(&mParamAvc, p, sizeof(mParamAvc));
    return OMX_ErrorNone;
}


OMX_ERRORTYPE OMXVideoDecoderAVCSecure::GetParamVideoAVCProfileLevel(OMX_PTR pStructure) {
    OMX_ERRORTYPE ret;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *p = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pStructure;
    CHECK_TYPE_HEADER(p);
    CHECK_PORT_INDEX(p, INPORT_INDEX);
    CHECK_ENUMERATION_RANGE(p->nProfileIndex,1);

    p->eProfile = mParamAvc.eProfile;
    p->eLevel = mParamAvc.eLevel;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::SetParamVideoAVCProfileLevel(OMX_PTR pStructure) {
    LOGW("SetParamVideoAVCProfileLevel is not supported.");
    return OMX_ErrorUnsupportedSetting;
}

OMX_U8* OMXVideoDecoderAVCSecure::MemAllocDataBuffer(OMX_U32 nSizeBytes, OMX_PTR pUserData) {
    OMXVideoDecoderAVCSecure* p = (OMXVideoDecoderAVCSecure *)pUserData;
    if (p) {
        return p->MemAllocDataBuffer(nSizeBytes);
    }
    LOGE("%s: NULL pUserData.", __FUNCTION__);
    return NULL;
}

void OMXVideoDecoderAVCSecure::MemFreeDataBuffer(OMX_U8 *pBuffer, OMX_PTR pUserData) {
    OMXVideoDecoderAVCSecure* p = (OMXVideoDecoderAVCSecure *)pUserData;
    if (p) {
        p->MemFreeDataBuffer(pBuffer);
        return;
    }
    LOGE("%s: NULL pUserData.", __FUNCTION__);
}

OMX_U8* OMXVideoDecoderAVCSecure::MemAllocDataBuffer(OMX_U32 nSizeBytes)
{
    LOGW_IF(nSizeBytes != INPORT_BUFFER_SIZE,
        "%s: size of memory to allocate is %lu, but will allocate %u",
        __FUNCTION__, nSizeBytes, sizeof(ProtectedDataBuffer));

    if (mNumInportBuffers >= INPORT_ACTUAL_BUFFER_COUNT)
    {
        LOGE("%s: cannot allocate buffer: number of inport buffers is %u, which is already at maximum",
            __FUNCTION__, mNumInportBuffers) ;
        return NULL ;
    }

    ProtectedDataBuffer *pBuffer = new ProtectedDataBuffer;
    if (pBuffer == NULL)
    {
        LOGE("%s: failed to allocate memory.", __FUNCTION__);
        return NULL;
    }

    ++mNumInportBuffers ;

    Init_ProtectedDataBuffer(pBuffer) ;

    pBuffer->size = INPORT_BUFFER_SIZE;

    LOGV("Allocating buffer = %#x, data = %#x",  (uint32_t)pBuffer, (uint32_t)pBuffer->data);
    return (OMX_U8 *) pBuffer;
}
// End of OMXVideoDecoderAVCSecure::MemAllocDataBuffer()

void OMXVideoDecoderAVCSecure::MemFreeDataBuffer(OMX_U8 *pBuffer)
{
    if (pBuffer == NULL)
    {
        LOGE("%s: trying to free NULL pointer", __FUNCTION__) ;
        return;
    }

    if (mNumInportBuffers == 0)
    {
        LOGE("%s: allocated inport buffer count is already 0, cannot delete buffer %p",
            __FUNCTION__, pBuffer) ;
        return ;
    }

    ProtectedDataBuffer *p = (ProtectedDataBuffer*) pBuffer;
    if (p->magic != PROTECTED_DATA_BUFFER_MAGIC)
    {
        LOGE("%s: attempting to free buffer with a wrong magic 0x%08x", __FUNCTION__, p->magic) ;
        return ;
    }

    LOGV("Freeing Data buffer %p with data = %p", p, p->data);
    delete p;
    --mNumInportBuffers ;
}
// End of OMXVideoDecoderAVCSecure::MemFreeDataBuffer()

void OMXVideoDecoderAVCSecure::KeepAliveTimerCallback(sigval v) {
    OMXVideoDecoderAVCSecure *p = (OMXVideoDecoderAVCSecure *)v.sival_ptr;
    if (p) {
        p->KeepAliveTimerCallback();
    }
}

void OMXVideoDecoderAVCSecure::KeepAliveTimerCallback() {
    uint32_t timeout = DRM_KEEP_ALIVE_TIMER;
    uint32_t sepres =  drm_keep_alive(WV_SESSION_ID, &timeout);
    if (sepres != 0) {
        LOGE("Drm_KeepAlive failed. Result = %#x", sepres);
    }
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoDecoder.AVC.secure", "video_decoder.avc", OMXVideoDecoderAVCSecure);
