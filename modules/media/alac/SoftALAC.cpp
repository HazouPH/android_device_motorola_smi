/*
 * Portions Copyright (C) 2011 Intel
 */

/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "SoftALAC"
#define MAX_NUM_CHANNELS 8
#define MAX_OUT_PCM_BUFFER_SIZE (MAX_NUM_CHANNELS*4096*2)
#include <utils/Log.h>
#include "SoftALAC.h"
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <string.h>
#include <math.h>

namespace android {

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

// Convert 20/24/32-bit PCM audio buffer to 16-bit PCM audio buffer
// by truncating least significant bits
void convertToPcm16( uint8_t *pSrcDst, int32_t bitDepth, int32_t numChannels, int32_t len )
{
    uint8_t *ptr = pSrcDst + 1;
    int16_t *pIn, *pOut = (int16_t *) pSrcDst;
    int k=3;
    volatile int16_t tmp;

    // Set input buffer start position and increment
    // based on bit depth; default to 3 for 20/24
    if (bitDepth==32)
    {
        k=4;
        ptr++;
    }

    // Repack 20/24/32-bit PCM to 16-bit PCM
    // via truncation of LS 4/8/16 bits
    for(int j=0;j<len*numChannels;j++)
    {
        pIn = (int16_t *)ptr;
        tmp = *pIn;
        pOut[j] = tmp;
        ptr += k;
    }
}

SoftALAC::SoftALAC(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : SimpleSoftOMXComponent(name, callbacks, appData, component),
      mAnchorTimeUs(0),
      mNumSamplesOutput(0),
      mSignalledError(false),
      mFrameCount(0),
      mOutputPortSettingsChange(NONE)
{
    ALOGV("SoftALAC v0.1016 %s %s\n",__DATE__,__TIME__);
    initPorts();
    initDecoder();
    ALOGV("EXIT SoftALAC\n");
}

SoftALAC::~SoftALAC()
{
    ALOGV("ENTER ~SoftALAC\n");
    if ( mALACEngine != NULL )
    {
        ALOGV("Deleting SoftALAC\n");
        delete mALACEngine;
        mALACEngine = NULL;
        ALOGV("Done deleting SoftALAC\n");
    }
    ALOGV("EXIT ~SoftALAC\n");
}

void SoftALAC::initPorts()
{
    ALOGV("ENTER SoftALAC::initPorts\n");

    // Create ports
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    // Define input port
    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;

    // FIXME - make parametric on init (bps x numSamples, etc.)
    def.nBufferSize = 32768;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;
    def.format.audio.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_AUDIO_ALAC);
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = (OMX_AUDIO_CODINGTYPE) OMX_AUDIO_CodingALAC;
    addPort(def);

    // Define output port
    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;

    // FIXME - make parametric on init (as above)
    def.nBufferSize = MAX_OUT_PCM_BUFFER_SIZE;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;
    def.format.audio.cMIMEType = const_cast<char *>("audio/raw");
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    addPort(def);
    ALOGV("EXIT SoftALAC::initPorts\n");
}

void SoftALAC::initDecoder()
{
    ALOGV("ENTER SoftALAC::initDecoder\n");
    // Create alac decoder
    mALACEngine = new ALACEngine;
    memset(&mALACConfig,sizeof(ALACSpecificConfig),0);
    ALOGV("EXIT SoftALAC::initDecoder\n");
}

OMX_ERRORTYPE SoftALAC::internalGetParameter( OMX_INDEXTYPE index, OMX_PTR params )
{
    ALOGV("ENTER SoftALAC::internalGetParameter\n");
    switch (index)
    {
        case (OMX_INDEXTYPE) OMX_IndexParamAudioAlac:
        {
            OMX_AUDIO_PARAM_ALACTYPE_EXT_INTEL *alacParams = (OMX_AUDIO_PARAM_ALACTYPE_EXT_INTEL *) params;
            if ( alacParams->nPortIndex != 0 )
            {
                ALOGV("ERROR SoftALAC::internalGetParameter/portIndex\n");
                return OMX_ErrorUndefined;
            }
            alacParams->nFrameLength = mALACConfig.frameLength;
            alacParams->nCompatibleVersion = mALACConfig.compatibleVersion;
            alacParams->nBitDepth = mALACConfig.bitDepth;
            alacParams->nPb = mALACConfig.pb;
            alacParams->nMb = mALACConfig.mb;
            alacParams->nKb = mALACConfig.kb;
            alacParams->nChannels = mALACConfig.numChannels;
            alacParams->nMaxRun = mALACConfig.maxRun;
            alacParams->nMaxFrameBytes = mALACConfig.maxFrameBytes;
            alacParams->nAvgBitRate = mALACConfig.avgBitRate;
            alacParams->nSampleRate = mALACConfig.sampleRate;
            ALOGV("EXIT SoftALAC::internalGetParameter()/ALAC input port\n");
            return OMX_ErrorNone;
        }
        case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams = (OMX_AUDIO_PARAM_PCMMODETYPE *) params;
            if ( pcmParams->nPortIndex != 1 )
            {
                ALOGV("ERROR SoftALAC::internalGetParameter/portIndex\n");
                return OMX_ErrorUndefined;
            }
            pcmParams->eNumData = OMX_NumericalDataSigned;
            pcmParams->eEndian = OMX_EndianBig;
            pcmParams->bInterleaved = OMX_TRUE;
            pcmParams->nBitPerSample = 16;        // hardwired truncation for now
            pcmParams->ePCMMode = OMX_AUDIO_PCMModeLinear;
            pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            pcmParams->eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            pcmParams->nChannels = mALACConfig.numChannels;
            pcmParams->nSamplingRate = mALACConfig.sampleRate;
            ALOGV("EXIT SoftALAC::internalGetParameter()/PCM output port\n");
            return OMX_ErrorNone;
        }
        default:
            return SimpleSoftOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftALAC::internalSetParameter( OMX_INDEXTYPE index, const OMX_PTR params )
{
    ALOGV("ENTER SoftALAC::internalSetParameter\n");
    switch (index)
    {
        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams = (const OMX_PARAM_COMPONENTROLETYPE *) params;
            if ( strncmp((const char *)roleParams->cRole, "audio_decoder.alac", OMX_MAX_STRINGNAME_SIZE - 1) )
            {
                ALOGV("Error SoftALAC::internalSetParameter UNDEFINED ROLE REQUEST\n");
                return OMX_ErrorUndefined;
            }
            ALOGV("Exit SoftALAC::internalSetParameter ROLE REQUEST OK\n");
            return OMX_ErrorNone;
        }
        case (OMX_INDEXTYPE) OMX_IndexParamAudioAlac:
        {
            const OMX_AUDIO_PARAM_ALACTYPE_EXT_INTEL *alacParams = (const OMX_AUDIO_PARAM_ALACTYPE_EXT_INTEL *) params;
            if (alacParams->nPortIndex != 0)
            {
                ALOGV("EXIT SoftALAC::internalSetParameter()/input ALAC port index error\n");
                return OMX_ErrorUndefined;
            }
            mALACConfig.frameLength = alacParams->nFrameLength;
            mALACConfig.compatibleVersion = alacParams->nCompatibleVersion;
            mALACConfig.bitDepth = alacParams->nBitDepth;
            mALACConfig.pb = alacParams->nPb;
            mALACConfig.mb = alacParams->nMb;
            mALACConfig.kb = alacParams->nKb;
            mALACConfig.numChannels = alacParams->nChannels;
            mALACConfig.maxRun = alacParams->nMaxRun;
            mALACConfig.maxFrameBytes = alacParams->nMaxFrameBytes;
            mALACConfig.avgBitRate = alacParams->nAvgBitRate;
            mALACConfig.sampleRate = alacParams->nSampleRate;
            ALOGV("EXIT SoftALAC::internalSetParameter()/ALAC input port\n");

            // Create and initialize alac decoder
            // FIXME: use defaults, dynamic reconfig as needed
            if ( mALACEngine != NULL )
            {
                ALOGV("Deleting ALAC decoder\n");
                delete mALACEngine;
                mALACEngine = NULL;
                ALOGV("Done deleting ALAC decoder\n");
            }
            ALOGV("Creating new ALAC decoder\n");
            mALACEngine = new ALACEngine;
            ALOGV("Done creating new ALAC decoder\n");
            ALOGV("Initializing new ALAC decoder per SetParameter inputs\n");
            ALOGV("%d %d %d %d %d %d %d %d %d %d\n",mALACConfig.frameLength,mALACConfig.bitDepth,mALACConfig.pb,mALACConfig.mb,mALACConfig.kb,mALACConfig.numChannels,
                  mALACConfig.maxRun,mALACConfig.maxFrameBytes,mALACConfig.avgBitRate,mALACConfig.sampleRate);
            mALACEngine->Init(&mALACConfig);
            mFrameCount = 0;
            ALOGV("Done initializing new ALAC decoder\n");

            return OMX_ErrorNone;
        }
        default:
        {
            ALOGV("EXIT SoftALAC::internalSetParameter / default\n");
            return SimpleSoftOMXComponent::internalSetParameter(index, params);
        }
    }
}

void SoftALAC::onQueueFilled(OMX_U32 portIndex)
{
    uint32_t numSamplesDecoded;
    if ( mSignalledError || mOutputPortSettingsChange != NONE )
    {
        ALOGV("EXIT SoftALAC::onQueueFilled: NO DATA\n");
        return;
    }
    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!inQueue.empty() && !outQueue.empty())
    {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;
        if ( inHeader->nFlags & OMX_BUFFERFLAG_EOS )
        {
            ALOGV("onQueueFilled OMX_BUFFERFLAG_EOS\n");
            inQueue.erase(inQueue.begin());
            inInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(inHeader);
            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            outQueue.erase(outQueue.begin());
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
            ALOGV("EXIT SoftALAC::onQueueFilled EOS\n");
            return;
        }
        if (inHeader->nOffset == 0)
        {
            mAnchorTimeUs = inHeader->nTimeStamp;
            mNumSamplesOutput = 0;
        }
        uint8_t *p = inHeader->pBuffer + inHeader->nOffset;
        uint32_t inputBufferCurrentLength = inHeader->nFilledLen;
        int16_t *pOut = reinterpret_cast<int16_t *>(outHeader->pBuffer);
        int M=inHeader->nFilledLen;

        // Decode ALAC frame
        BitBufferInit( &mAlacBitBuf, p, inputBufferCurrentLength );
        mALACEngine->Decode( &mAlacBitBuf, (uint8_t *)pOut, mALACConfig.frameLength, mALACConfig.numChannels, &numSamplesDecoded );
        ALOGV("Frame %d %d %d\n", mFrameCount, inputBufferCurrentLength, numSamplesDecoded);
        mFrameCount++;

        // Convert output to 16-bit PCM
        if ( mALACConfig.bitDepth != 16 )
        {
            convertToPcm16( (uint8_t *)pOut, mALACConfig.bitDepth, mALACConfig.numChannels, numSamplesDecoded );
        }

        /* if decoder error
        {
                notify(OMX_EventError, OMX_ErrorUndefined, decoderErr, NULL);
                mSignalledError = true;
                return;
        } */

        // Update buf pointers and TS
        outHeader->nOffset = 0;
        outHeader->nFilledLen = mALACConfig.numChannels * numSamplesDecoded * sizeof(int16_t);
        outHeader->nTimeStamp = mAnchorTimeUs + (mNumSamplesOutput * 1000000ll) / mALACConfig.sampleRate;
        outHeader->nFlags = 0;
        //CHECK_GE(inHeader->nFilledLen, mConfig->inputBufferUsedLength);
        inHeader->nOffset += M;
        inHeader->nFilledLen -= M;
        mNumSamplesOutput += numSamplesDecoded;

        // Check for empty input buffer
        if ( inHeader->nFilledLen == 0 )
        {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }

        // Update output buffer status
        outInfo->mOwnedByUs = false;
        outQueue.erase(outQueue.begin());
        outInfo = NULL;
        notifyFillBufferDone(outHeader);
        outHeader = NULL;
    }
}

void SoftALAC::onPortFlushCompleted(OMX_U32 portIndex)
{
    ALOGV("ENTER SoftALAC::onPortFlushCompleted\n");
    if (portIndex == 0) {
        // Make sure that the next buffer output does not still
        // depend on fragments from the last one decoded.
        // FIXME: re-init decoder
    }
    ALOGV("EXIT SoftALAC::onPortFlushCompleted\n");
}

void SoftALAC::onPortEnableCompleted(OMX_U32 portIndex, bool enabled)
{
    ALOGV("ENTER SoftALAC::onPortEnableCompleted\n");
    if (portIndex != 1)
    {
        return;
    }

    switch (mOutputPortSettingsChange) {
        case NONE:
            break;

        case AWAITING_DISABLED:
        {
            CHECK(!enabled);
            mOutputPortSettingsChange = AWAITING_ENABLED;
            break;
        }

        default:
        {
            CHECK_EQ((int)mOutputPortSettingsChange, (int)AWAITING_ENABLED);
            CHECK(enabled);
            mOutputPortSettingsChange = NONE;
            break;
        }
    }
    ALOGV("EXIT SoftALAC::onPortEnableCompleted\n");
}

}  // namespace android

android::SoftOMXComponent *createSoftOMXComponent( const char *name, const OMX_CALLBACKTYPE *callbacks, OMX_PTR appData, OMX_COMPONENTTYPE **component)
{
    ALOGV("ENTER/EXIT SoftALAC SoftOMXComponent - createSoftOMXComponent\n");
    return new android::SoftALAC(name, callbacks, appData, component);
}
