/*
 * Copyright (c) 2011 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    tianyang.zhu <tianyang.zhu@intel.com>
 */

//#define LOG_NDEBUG 0

#define LOG_TAG "MultiDisplay"

#include <utils/Log.h>
#include "psb_mds.h"

namespace android {
#ifndef USE_MDS_LEGACY
namespace intel {
#endif

#ifndef USE_MDS_LEGACY
MDSListener::MDSListener(int mode, bool vpp, int32_t width, int32_t height) {
    ALOGV("A new Mds listener 0x%x,%d,%d is created", mode, width, height);
    mMode = mode;
    mVppState = vpp;
    mDecoderConfigWidth  = width;
    mDecoderConfigHeight = height;
}

MDSListener::~MDSListener() {
    ALOGV("A Mds listener %p is distroyed", this);
    mMode = MDS_MODE_NONE;
    mVppState = false;
    mDecoderConfigWidth  = 0;
    mDecoderConfigHeight = 0;
}

status_t MDSListener::onMdsMessage(int msg, void* value, int size) {
    //ALOGV("Video driver receives a Mds message %d", msg);
    if ((msg & MDS_MSG_MODE_CHANGE) && (size == sizeof(int))) {
        mMode = *((int*)(value));
        ALOGI("A new mode change message: 0x%x", mMode);
    }
    return NO_ERROR;
}

int MDSListener::getMode() {
    //ALOGV("Mds mode 0x%x", mMode);
    return mMode;
}

bool MDSListener::getDecoderOutputResolution(int32_t* width, int32_t* height) {
    // only for WIDI video playback
    if (width == NULL || height == NULL)
        return false;
    *width  = mDecoderConfigWidth;
    *height = mDecoderConfigHeight;
    ALOGV("%dx%d", *width, *height);
    return true;
}

bool MDSListener::getVppState() {
    ALOGV("MDS Vpp state %d", mVppState);
    return mVppState;
}
#endif

psbMultiDisplayListener::psbMultiDisplayListener() {
#ifndef USE_MDS_LEGACY
    // get mds service and register listener
    sp<IServiceManager> sm = defaultServiceManager();
    if (sm == NULL) {
        LOGE("%s: Fail to get service manager", __func__);
        return;
    }
    mMds = interface_cast<IMDService>(sm->getService(String16(INTEL_MDS_SERVICE_NAME)));
    if (mMds == NULL) {
        LOGE("%s: Failed to get Mds service", __func__);
        return;
    }
#if 0
    // for initialization
    int mode = MDS_MODE_NONE;
    int32_t width  = 0;
    int32_t height = 0;
    bool vppState = false;
    sp<IMultiDisplayInfoProvider> infoProvider = mMds->getInfoProvider();
    if (infoProvider != NULL) {
        mode = infoProvider->getDisplayMode(true);
        //TODO: use default video session: 0
        //MDS will select a right video session ID
        infoProvider->getDecoderOutputResolution(-1, &width, &height);
        vppState = infoProvider->getVppState();
    }
    mListener = new MDSListener(mode, vppState, height, width);
    sp<IMultiDisplaySinkRegistrar> sinkRegistrar = NULL;
    if ((sinkRegistrar = mMds->getSinkRegistrar()) == NULL)
        return;
    mListenerId = sinkRegistrar->registerListener(
            mListener, "VideoDriver", MDS_MSG_MODE_CHANGE);
    ALOGV("Create a Video driver listener %d, %p", mListenerId, mListener.get());
#else
    mListener = mMds->getInfoProvider();
#endif
#else
    mListener = new MultiDisplayClient();
    if (mListener == NULL)
        return;
#endif
    return;
}

psbMultiDisplayListener::~psbMultiDisplayListener() {
#ifndef USE_MDS_LEGACY
#if 0
    ALOGV("Destroy Video driver listener %d,  %p", mListenerId, mListener.get());
    sp<IMultiDisplaySinkRegistrar> sinkRegistrar = NULL;
    if (mMds == NULL || mListenerId < 0 ||
            mListener.get() == NULL) {
        ALOGE("Failed to get Mds service");
        return;
    }
    if ((sinkRegistrar = mMds->getSinkRegistrar()) == NULL) {
        ALOGE("Failed to get Mds Sink registrar");
        return;
    }
    sinkRegistrar->unregisterListener(mListenerId);
    mListenerId = -1;
#endif
#else
    if (mListener != NULL)
        delete mListener;
#endif
    mListener = NULL;
    return;
}

int psbMultiDisplayListener::getMode() {
    int mode = MDS_MODE_NONE;
#ifndef USE_MDS_LEGACY
    if (mListener.get() == NULL) return MDS_INIT_VALUE;
#if 0
    mode = mListener->getMode();
#else
    mode = mListener->getDisplayMode(false);
#endif
    if (checkMode(mode, (MDS_VIDEO_ON | MDS_HDMI_CONNECTED)))
        mode = MDS_HDMI_VIDEO_ISPLAYING;
    else if (checkMode(mode, (MDS_VIDEO_ON | MDS_WIDI_ON)))
        mode = MDS_WIDI_VIDEO_ISPLAYING;
    else
        mode = MDS_INIT_VALUE;
#else
    if (mListener == NULL) return MDS_MODE_NONE;
    mode = mListener->getDisplayMode(false);
    if (checkMode(mode, MDS_HDMI_VIDEO_EXT))
        mode = MDS_HDMI_VIDEO_ISPLAYING;
    else if (checkMode(mode,MDS_WIDI_ON))
        mode = MDS_WIDI_VIDEO_ISPLAYING;
    else
        mode = MDS_INIT_VALUE;
#endif
    //ALOGV("mds mode is %d", mode);
    return mode;
}

bool psbMultiDisplayListener::getDecoderOutputResolution(int32_t* width, int32_t* height) {
#ifndef USE_MDS_LEGACY
    if (mListener.get() == NULL ||
            width == NULL || height == NULL)
        return false;
    // only for WIDI video playback,
    // TODO: HWC doesn't set the bit "MDS_WIDI_ON" rightly now
#if 0
    int mode = mListener->getMode();
#else
    int mode = mListener->getDisplayMode(false);
#endif
    if (!checkMode(mode, (MDS_VIDEO_ON | MDS_WIDI_ON)))
        return false;
#if 0
    return mListener->getDecoderOutputResolution(width, height);
#else
    return mListener->getDecoderOutputResolution(-1, width, height);
#endif
#else
    return false;
#endif
}

bool psbMultiDisplayListener::getVppState() {
#ifndef USE_MDS_LEGACY
    if (mListener.get() == NULL) {
        ALOGE("MDS listener is null");
        return false;
    }
    return mListener->getVppState();
#else
    return false;
#endif
}

}; // namespace android
#ifndef USE_MDS_LEGACY
}; // namespace intel
#endif
