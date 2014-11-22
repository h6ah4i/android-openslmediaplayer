//
//    Copyright (C) 2014 Haruki Hasegawa
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

#ifndef AUDIODATATYPES_HPP_
#define AUDIODATATYPES_HPP_

#include <cstddef>

namespace oslmp {
namespace impl {

enum sample_format_type { kAudioSampleFormatType_Unknown, kAudioSampleFormatType_S16, kAudioSampleFormatType_F32, };

inline size_t getBytesPerSample(sample_format_type fmt)
{
    switch (fmt) {
    case kAudioSampleFormatType_S16:
        return 2;
    case kAudioSampleFormatType_F32:
        return 4;
    default:
        return 0;
    }
}

} // namespace impl
} // namespace oslmp

#endif // AUDIODATATYPES_HPP_
