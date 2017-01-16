/* ////////////////////////////////////////////////////////////////////////////// */
/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright (c) 2005-2011 Intel Corporation. All Rights Reserved.
//
//
*/

/*
* Copyright (C) 2009 The Android Open Source Project
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

#ifndef UMC_MACRO_H_
#define UMC_MACRO_H_

#include "UMCPerfTracing.h"

// Switch IPP to linux code path (defined in Andoird.mk)
// #define LINUX32

// Logging control define (comment to disable logging):
// #define LOG_NDEBUG 0

// Factory declaration:
#define FACTORY_CREATE_DECL(name) \
sp<MediaSource> Make##name(const sp<MediaSource> &source);

#define FACTORY_CREATE_ENCODER_DECL(name) \
sp<MediaSource> Make##name(const sp<MediaSource> &source, const sp<MetaData> &meta);

// Factory implementation:
#define FACTORY_CREATE_IMPL(name) \
sp<MediaSource> Make##name(const sp<MediaSource> &source){\
    return new name(source); \
}

// Factory implementation(+threaded source):
// ThreadedSource.h should be included in the macro is used
#define FACTORY_CREATE_IMPL_THREADED(name) \
sp<MediaSource> Make##name(const sp<MediaSource> &source){\
    return new ThreadedSource(new name(source)); \
}

#define FACTORY_CREATE_ENCODER_IMPL(name)\
sp<MediaSource> Make##name(const sp<MediaSource> &source, const sp<MetaData> &meta){\
    return new name(source, meta); \
}

#define US_PER_SECOND 1000000LL
namespace android{

template <class T>
inline void SafeDelete(T &ptr){
    delete ptr;
    ptr = NULL;
}

template <class T>
inline void SafeRelease(T &ptr){
    if(NULL != ptr) {
        ptr->release();
        ptr= NULL;
    }
}

// Meta data keys, specific to MDP codec plugins:
enum {
    kKeyMultichannelSupport = 'mchS', // key for Int32 flag to enable multi-channel support in MDP plugins
    kKeyMp3DecoderLfeFilterOff = 'lfeO', // key for Int32 flag to turn off LFE filter in MP3 decoder
    kKeyAacParametricStereoModeOff = 'apsO', // key for Int32 flag to turn off Parmetric Stereo mode in AAC decoder
};

}
#endif //UMC_MACRO_H_
