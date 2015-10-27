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
#define LOG_TAG "OMXVideoDecoder"
#include <utils/Log.h>
#include "OMXVideoDecoderAVCSecure.h"
#include <time.h>
#include <signal.h>
#include <pthread.h>

extern "C" {
#include <sepdrm.h>
#include <fcntl.h>
#include <linux/psb_drm.h>
#include "xf86drm.h"
#include "xf86drmMode.h"
}

// Be sure to have an equal string in VideoDecoderHost.cpp (libmix)
static const char* AVC_MIME_TYPE = "video/avc";
static const char* AVC_SECURE_MIME_TYPE = "video/avc-secure";

#define IMR_INITIAL_OFFSET      0 //1024
#define IMR_BUFFER_SIZE         (8 * 1024 * 1024)
#define KEEP_ALIVE_INTERVAL     5 // seconds
#define DRM_KEEP_ALIVE_TIMER    1000000
#define WV_SESSION_ID           0x00000011
#define NALU_BUFFER_SIZE        8192
#define FLUSH_WAIT_INTERVAL     (30 * 1000) //30 ms


#pragma pack(push, 1)
struct IMRDataBuffer {
    uint32_t offset;
    uint32_t size;
    uint8_t  *data;
    uint8_t  clear;  // 0 when IMR offset is valid, 1 when data is valid
};
#pragma pack(pop)

OMXVideoDecoderAVCSecure::OMXVideoDecoderAVCSecure()
    : mKeepAliveTimer(0),
      mSessionPaused(false),
      mDrmDevFd(-1) {
    LOGV("OMXVideoDecoderAVCSecure is constructed.");
    mVideoDecoder = createVideoDecoder(AVC_SECURE_MIME_TYPE);
    if (!mVideoDecoder) {
        LOGE("createVideoDecoder failed for \"%s\"", AVC_SECURE_MIME_TYPE);
    }
    // Override default native buffer count defined in the base class
    mNativeBufferCount = OUTPORT_NATIVE_BUFFER_COUNT;

    BuildHandlerList();

    mDrmDevFd = open("/dev/card0", O_RDWR, 0);
    if (mDrmDevFd < 0) {
        LOGE("Failed to open drm device.");
    }
}

OMXVideoDecoderAVCSecure::~OMXVideoDecoderAVCSecure() {
    LOGV("OMXVideoDecoderAVCSecure is destructed.");

    if (mDrmDevFd) {
        close(mDrmDevFd);
        mDrmDevFd = 0;
    }
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::InitInputPortFormatSpecific(OMX_PARAM_PORTDEFINITIONTYPE *paramPortDefinitionInput) {
    // OMX_PARAM_PORTDEFINITIONTYPE
    paramPortDefinitionInput->nBufferCountActual = INPORT_ACTUAL_BUFFER_COUNT;
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

    this->ports[INPORT_INDEX]->SetMemAllocator(MemAllocIMR, MemFreeIMR, this);

    for (int i = 0; i < INPORT_ACTUAL_BUFFER_COUNT; i++) {
        mIMRSlot[i].offset = IMR_INITIAL_OFFSET + i * INPORT_BUFFER_SIZE;
        mIMRSlot[i].owner = NULL;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorInit(void) {
    mSessionPaused = false;
    return OMXVideoDecoderBase::ProcessorInit();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorDeinit(void) {
    // Session should be torn down in ProcessorStop, delayed to ProcessorDeinit
    // to allow remaining frames completely rendered.
    LOGI("Calling Drm_DestroySession.");
    uint32_t sepres = drm_destroy_session(WV_SESSION_ID);
    if (sepres != 0) {
        LOGW("Drm_DestroySession returns %#x", sepres);
    }
    EnableIEDSession(false);

    return OMXVideoDecoderBase::ProcessorDeinit();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorStart(void) {
    uint32_t imrOffset = 0;
    uint32_t imrBufferSize = IMR_BUFFER_SIZE;
    uint32_t sessionID;

    EnableIEDSession(true);
    uint32_t sepres = drm_wv_create_session( &sessionID);
    if (sepres != 0) {
        LOGW("Drm_WV_CreateSession failed. Result = %#x", sepres);
        // Returning error will cause OMX client to crash.
        //return OMX_ErrorHardware;
    }
    if (sessionID != WV_SESSION_ID) {
        LOGE("Invalid session ID %#x created", sessionID);
        //return OMX_ErrorHardware;
    }
    LOGI("drm_wv_create_session: sessionID = %#x", sessionID);

    //drmCommandNone(mDrmDevFd, DRM_PSB_HDCP_DISPLAY_IED_OFF);

    int ret;
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = this;
    sev.sigev_notify_function = KeepAliveTimerCallback;

    ret = timer_create(CLOCK_REALTIME, &sev, &mKeepAliveTimer);
    if (ret != 0) {
        LOGE("Failed to create timer.");
    } else {
        struct itimerspec its;
        its.it_value.tv_sec = -1; // never expire
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = KEEP_ALIVE_INTERVAL;
        its.it_interval.tv_nsec = 0;

        ret = timer_settime(mKeepAliveTimer, TIMER_ABSTIME, &its, NULL);
        if (ret != 0) {
            LOGE("Failed to set timer.");
        }
    }
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

    OMX_BUFFERHEADERTYPE *pInput = *pBuffers[INPORT_INDEX];
    IMRDataBuffer *imrBuffer = (IMRDataBuffer *)pInput->pBuffer;
    if (imrBuffer->size == 0) {
        // error occurs during decryption.
        LOGW("size of returned IMR buffer is 0, decryption fails.");
        mVideoDecoder->flush();
        usleep(FLUSH_WAIT_INTERVAL);
        OMX_BUFFERHEADERTYPE *pOutput = *pBuffers[OUTPORT_INDEX];
        pOutput->nFilledLen = 0;
        // reset IMR buffer size
        imrBuffer->size = INPORT_BUFFER_SIZE;
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
#if 0
    uint32_t sepres = Drm_Playback_Pause(WV_SESSION_ID);
    if (sepres != 0) {
        LOGE("Drm_Playback_Pause failed. Result = %#x", sepres);
    }
#endif
    return OMXVideoDecoderBase::ProcessorPause();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::ProcessorResume(void) {
#if 0
    uint32_t sepres = Drm_Playback_Resume(WV_SESSION_ID);
    if (sepres != 0) {
        LOGE("Drm_Playback_Resume failed. Result = %#x", sepres);
    }
#endif
    return OMXVideoDecoderBase::ProcessorResume();
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareConfigBuffer(VideoConfigBuffer *p) {
    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::PrepareConfigBuffer(p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareConfigBuffer");
    p->flag |=  WANT_SURFACE_PROTECTION;
    return ret;
}

OMX_ERRORTYPE OMXVideoDecoderAVCSecure::PrepareDecodeBuffer(OMX_BUFFERHEADERTYPE *buffer, buffer_retain_t *retain, VideoDecodeBuffer *p) {
    OMX_ERRORTYPE ret;
    ret = OMXVideoDecoderBase::PrepareDecodeBuffer(buffer, retain, p);
    CHECK_RETURN_VALUE("OMXVideoDecoderBase::PrepareDecodeBuffer");

    if (buffer->nFilledLen == 0) {
        return OMX_ErrorNone;
    }
    // OMX_BUFFERFLAG_CODECCONFIG is an optional flag
    // if flag is set, buffer will only contain codec data.
    if (buffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
        LOGV("Received AVC codec data.");
        return ret;
    }
    p->flag |= HAS_COMPLETE_FRAME;

    if (buffer->nOffset != 0) {
        LOGW("buffer offset %lu is not zero!!!", buffer->nOffset);
    }

    IMRDataBuffer *imrBuffer = (IMRDataBuffer *)buffer->pBuffer;
    if (imrBuffer->clear) {
        p->data = imrBuffer->data + buffer->nOffset;
        p->size = buffer->nFilledLen;
    } else {
        imrBuffer->size = NALU_BUFFER_SIZE;
        struct drm_wv_nalu_headers nalu_headers;
        nalu_headers.p_enc_ciphertext = imrBuffer->data;

        // TODO: NALU Buffer is supposed to be 4k but using 1k, fix it once chaabi fix is there

        nalu_headers.hdrs_buf_len = 1024;
        nalu_headers.frame_size = buffer->nFilledLen;
        // Make sure that NALU header frame size is 16 bytes aligned
        nalu_headers.frame_size = (nalu_headers.frame_size + 0xF) & (~0xF);
        // Use same video buffer to fill NALU headers returned by chaabi,
        // Adding 4 because the first 4 bytes after databuffer will be used to store length of NALU headers
        nalu_headers.p_hdrs_buf = (uint8_t *)(imrBuffer->data + nalu_headers.frame_size + 4);
        nalu_headers.parse_size = buffer->nFilledLen;

        uint32_t res = drm_wv_return_naluheaders(WV_SESSION_ID, &nalu_headers);
        if (res == DRM_FAIL_FW_SESSION || !nalu_headers.hdrs_buf_len) {
            LOGW("Drm_WV_ReturnNALUHeaders failed. Session is disabled.");
            mSessionPaused = true;
            ret =  OMX_ErrorNotReady;
        } else if (res != 0) {
            mSessionPaused = false;
            LOGE("Drm_WV_ReturnNALUHeaders failed. Error = %#x, frame_size: %d, len = %lu", res, nalu_headers.frame_size, buffer->nFilledLen);
            ret = OMX_ErrorHardware;
        } else {
            mSessionPaused = false;
            // NALU headers are appended to encrypted video bitstream
            // |...encrypted video bitstream (16 bytes aligned)...| 4 bytes of header size |...NALU headers..|
            uint32_t *ptr = (uint32_t*)(imrBuffer->data + nalu_headers.frame_size);
            *ptr = nalu_headers.hdrs_buf_len;
            p->data = imrBuffer->data;
            p->size = nalu_headers.frame_size;
            p->flag |= IS_SECURE_DATA;
        }
    }

    // reset IMR size
    imrBuffer->size = NALU_BUFFER_SIZE;
    return ret;
}

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

OMX_U8* OMXVideoDecoderAVCSecure::MemAllocIMR(OMX_U32 nSizeBytes, OMX_PTR pUserData) {
    OMXVideoDecoderAVCSecure* p = (OMXVideoDecoderAVCSecure *)pUserData;
    if (p) {
        return p->MemAllocIMR(nSizeBytes);
    }
    LOGE("NULL pUserData.");
    return NULL;
}

void OMXVideoDecoderAVCSecure::MemFreeIMR(OMX_U8 *pBuffer, OMX_PTR pUserData) {
    OMXVideoDecoderAVCSecure* p = (OMXVideoDecoderAVCSecure *)pUserData;
    if (p) {
        p->MemFreeIMR(pBuffer);
        return;
    }
    LOGE("NULL pUserData.");
}

OMX_U8* OMXVideoDecoderAVCSecure::MemAllocIMR(OMX_U32 nSizeBytes) {
    // Ignore passed nSizeBytes, use INPORT_BUFFER_SIZE instead

    for (int i = 0; i < INPORT_ACTUAL_BUFFER_COUNT; i++) {
        if (mIMRSlot[i].owner == NULL) {
            IMRDataBuffer *pBuffer = new IMRDataBuffer;
            if (pBuffer == NULL) {
                LOGE("Failed to allocate memory.");
                return NULL;
            }

            pBuffer->data = new uint8_t [INPORT_BUFFER_SIZE];
            if (pBuffer->data == NULL) {
                delete pBuffer;
                LOGE("Failed to allocate memory, size to allocate %d.", INPORT_BUFFER_SIZE);
                return NULL;
            }

            pBuffer->offset = mIMRSlot[i].offset;
            pBuffer->size = INPORT_BUFFER_SIZE;
            mIMRSlot[i].owner = (OMX_U8 *)pBuffer;

            LOGV("Allocating buffer = %#x, IMR offset = %#x, data = %#x",  (uint32_t)pBuffer, mIMRSlot[i].offset, (uint32_t)pBuffer->data);
            return (OMX_U8 *) pBuffer;
        }
    }
    LOGE("IMR slot is not available.");
    return NULL;
}

void OMXVideoDecoderAVCSecure::MemFreeIMR(OMX_U8 *pBuffer) {
    IMRDataBuffer *p = (IMRDataBuffer*) pBuffer;
    if (p == NULL) {
        return;
    }
    for (int i = 0; i < INPORT_ACTUAL_BUFFER_COUNT; i++) {
        if (pBuffer == mIMRSlot[i].owner) {
            LOGV("Freeing IMR offset = %d, data = %#x", mIMRSlot[i].offset, (uint32_t)p->data);
            delete [] p->data;
            delete p;
            mIMRSlot[i].owner = NULL;
            return;
        }
    }
    LOGE("Invalid buffer %#x to de-allocate", (uint32_t)pBuffer);
}

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


bool OMXVideoDecoderAVCSecure::EnableIEDSession(bool enable)
{
    if (mDrmDevFd < 0) {
        return false;
    }
    int request = enable ?  DRM_PSB_ENABLE_IED_SESSION : DRM_PSB_DISABLE_IED_SESSION;
    int ret = drmCommandNone(mDrmDevFd, request);
    return ret == 0;
}

DECLARE_OMX_COMPONENT("OMX.Intel.VideoDecoder.AVC.secure", "video_decoder.avc", OMXVideoDecoderAVCSecure);
