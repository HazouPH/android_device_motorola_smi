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

#ifndef _PSB_MDS_H_
#define _PSB_MDS_H_

#ifdef TARGET_HAS_MULTIPLE_DISPLAY
#ifdef USE_MDS_LEGACY
#include <display/MultiDisplayClient.h>
#else
#include <display/MultiDisplayService.h>
#ifdef PSBVIDEO_MRFL_VPP
#include <VPPSetting.h>
#endif

#endif
#endif

#ifdef TARGET_HAS_MULTIPLE_DISPLAY
enum {
    MDS_INIT_VALUE = 0,
    MDS_HDMI_VIDEO_ISPLAYING,
    MDS_WIDI_VIDEO_ISPLAYING,
};
#endif

namespace android {
#ifndef USE_MDS_LEGACY
namespace intel {
#endif

#ifndef USE_MDS_LEGACY
class MDSListener : public BnMultiDisplayListener {
private:
    int     mMode;
    bool    mVppState;
    int32_t mDecoderConfigWidth;
    int32_t mDecoderConfigHeight;
public:
    MDSListener(int, bool, int32_t, int32_t);
    ~MDSListener();

    status_t onMdsMessage(int msg, void* value, int size);
    int  getMode();
    bool getDecoderOutputResolution(int32_t* width, int32_t* height);
    bool getVppState();
};
#endif

class psbMultiDisplayListener {
private:
#ifndef USE_MDS_LEGACY
    sp<IMDService>  mMds;
#if 0
    sp<MDSListener> mListener;
    int32_t         mListenerId;
#else
    sp<IMultiDisplayInfoProvider> mListener;
#endif
#else
    MultiDisplayClient* mListener;
#endif
public:
    psbMultiDisplayListener();
    ~psbMultiDisplayListener();
    inline bool checkMode(int value, int bit) {
        return (value & bit) == bit ? true : false;
    }

    int  getMode();
    // only for WIDI video playback
    bool getDecoderOutputResolution(int32_t* width, int32_t* height);
    bool getVppState();
};

}; // namespace android
#ifndef USE_MDS_LEGACY
}; // namespace intel
#endif

#endif
