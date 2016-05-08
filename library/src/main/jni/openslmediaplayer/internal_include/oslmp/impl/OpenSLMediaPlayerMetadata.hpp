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

#ifndef OPENSLMEDIAPLAYERMETADATA_HPP_
#define OPENSLMEDIAPLAYERMETADATA_HPP_

#include <SLES/OpenSLES.h>
#include "oslmp/utils/optional.hpp"

namespace oslmp {
namespace impl {

struct OpenSLMediaPlayerMetadata {
    utils::optional<SLuint32> numChannels;       // [ch]
    utils::optional<SLmilliHertz> samplesPerSec; // [milliHz]
    utils::optional<SLuint32> bitsPerSample;     // [bits]
    utils::optional<SLuint32> containerSize;     // [bytes]
    utils::optional<SLuint32> channelMask;
    utils::optional<SLuint32> endianness;
    utils::optional<SLmillisecond> duration; // [ms]

    void clear() noexcept
    {
        numChannels.clear();
        samplesPerSec.clear();
        bitsPerSample.clear();
        containerSize.clear();
        channelMask.clear();
        endianness.clear();
        duration.clear();
    }

    bool isValid() const noexcept
    {
        return numChannels.available() && samplesPerSec.available() && bitsPerSample.available() &&
               containerSize.available() && channelMask.available() && endianness.available() && duration.available();
    }
};

} // namespace impl
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERMETADATA_HPP_
