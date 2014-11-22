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

#ifndef OPENSLMEDIAPLAYERINTERNALUTILS_HPP_
#define OPENSLMEDIAPLAYERINTERNALUTILS_HPP_

#include <string>
#include <SLES/OpenSLES.h>
#include "oslmp/impl/OpenSLMediaPlayerInternalDefs.hpp"

namespace oslmp {
namespace impl {

class OpenSLMediaPlayerInternalUtils {

    OpenSLMediaPlayerInternalUtils() = delete;

public:
    static bool sDecodeUri(const std::string &src, std::string &dest) noexcept;
    static int sTranslateOpenSLErrorCode(SLresult result) noexcept;
    static bool sTranslateToOpenSLStreamType(int streamType, SLint32 *slStreamType) noexcept;
    static bool sTranslateOpenSLStreamType(SLint32 slStreamType, int *streamType) noexcept;
    static const char *sGetPlayerStateName(PlayerState state) noexcept;
};

} // namespace impl
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERINTERNALUTILS_HPP_
