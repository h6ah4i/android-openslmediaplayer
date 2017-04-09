//
//    Copyright (C) 2016 Haruki Hasegawa
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#ifndef OPENSLES_ANDROID_COMPLEMENTS_H_
#define OPENSLES_ANDROID_COMPLEMENTS_H_

#include <android/api-level.h>
#include <SLES/OpenSLES.h>

#if (__ANDROID_API__ < 21) && !defined(SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT)
extern "C" {

/* The following pcm representations and data formats map to those in OpenSLES 1.1 */
#define SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT ((SLuint32)0x00000001)
#define SL_ANDROID_PCM_REPRESENTATION_UNSIGNED_INT ((SLuint32)0x00000002)
#define SL_ANDROID_PCM_REPRESENTATION_FLOAT ((SLuint32)0x00000003)

#define SL_ANDROID_DATAFORMAT_PCM_EX ((SLuint32)0x00000004)

typedef struct SLAndroidDataFormat_PCM_EX_ {
    SLuint32 formatType;
    SLuint32 numChannels;
    SLuint32 sampleRate;
    SLuint32 bitsPerSample;
    SLuint32 containerSize;
    SLuint32 channelMask;
    SLuint32 endianness;
    SLuint32 representation;
} SLAndroidDataFormat_PCM_EX;

} // extern "C"
#endif

#endif // OPENSLES_ANDROID_COMPLEMENTS_H_

