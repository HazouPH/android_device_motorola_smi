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

#ifndef SOFT_ALAC_H_

#define SOFT_ALAC_H_

#include "SimpleSoftOMXComponent.h"
#include "OMX_Ext_Intel.h"
#include "ALACEngine.h"
#include "ALACBitUtilities.h"

namespace android {

struct SoftALAC : public SimpleSoftOMXComponent {
    SoftALAC(const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

protected:
    virtual ~SoftALAC();

    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);

private:
    enum {
        kNumBuffers = 4
    };

    ALACSpecificConfig mALACConfig;
    ALACEngine *mALACEngine;
    BitBuffer mAlacBitBuf;
    int32_t mFrameCount;
    int64_t mAnchorTimeUs;
    int64_t mNumSamplesOutput;
    int32_t mNumChannels;
    int32_t mSamplingRate;
    bool mSignalledError;

    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;

    void initPorts();
    void initDecoder();

    DISALLOW_EVIL_CONSTRUCTORS(SoftALAC);
};

}  // namespace android

#endif  // SOFT_ALAC_H_


