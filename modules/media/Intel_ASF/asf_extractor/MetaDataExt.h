/*
* Copyright (C) 2011 The Android Open Source Project
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


#ifndef META_DATA_EXT_H_
#define META_DATA_EXT_H_

#include <media/stagefright/MetaData.h>

namespace android {

/*#define MEDIA_MIMETYPE_AUDIO_WMA        "audio/x-ms-wma"
#ifndef DOLBY_UDC
#define MEDIA_MIMETYPE_AUDIO_AC3        "audio/ac3"
#endif
#define MEDIA_MIMETYPE_VIDEO_WMV        "video/x-ms-wmv"
#define MEDIA_MIMETYPE_CONTAINER_ASF    "video/x-ms-asf"
#define MEDIA_MIMETYPE_VIDEO_VA         "video/x-va"
#define MEDIA_MIMETYPE_AUDIO_WMA_VOICE  "audio/wma-voice"
*/ // Already defined in CM source

enum
{
    // value by default takes int32_t unless specified
    kKeyConfigData              = 'kcfg',  // raw data
    kKeyProtected               = 'prot',  // int32_t (bool)
    kKeyCropLeft                = 'clft',
    kKeyCropRight               = 'crit',
    kKeyCropTop                 = 'ctop',
    kKeyCropBottom              = 'cbtm',
    kKeySuggestedBufferSize     = 'sgbz',
    kKeyWantRawOutput           = 'rawo'
};

enum
{
    kTypeConfigData             = 'tcfg',
};

}  // namespace android

#endif  // META_DATA_EXT_H_
