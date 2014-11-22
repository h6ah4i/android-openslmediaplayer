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

#ifndef OPENSLMEDIAPLAYERVIRTUALIZER_HPP_
#define OPENSLMEDIAPLAYERVIRTUALIZER_HPP_

#include <oslmp/OpenSLMediaPlayerAPICommon.hpp>

namespace oslmp {

class OpenSLMediaPlayerContext;

class OpenSLMediaPlayerVirtualizer : public virtual android::RefBase {
public:
    struct Settings {
        int16_t strength;
    };

    OpenSLMediaPlayerVirtualizer(const android::sp<OpenSLMediaPlayerContext> &context) OSLMP_API_ABI;
    virtual ~OpenSLMediaPlayerVirtualizer() OSLMP_API_ABI;

    int setEnabled(bool enabled) noexcept OSLMP_API_ABI;
    int getEnabled(bool *enabled) noexcept OSLMP_API_ABI;
    int getId(int *id) noexcept OSLMP_API_ABI;
    int hasControl(bool *hasControl) noexcept OSLMP_API_ABI;
    int getStrengthSupported(bool *strengthSupported) noexcept OSLMP_API_ABI;
    int getRoundedStrength(int16_t *roundedStrength) noexcept OSLMP_API_ABI;
    int getProperties(Settings *settings) noexcept OSLMP_API_ABI;
    int setStrength(int16_t strength) noexcept OSLMP_API_ABI;
    int setProperties(const Settings *settings) noexcept OSLMP_API_ABI;

private:
    class Impl;
    Impl *impl_; // NOTE: do not use unique_ptr to avoid cxxporthelper dependencies
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYERVIRTUALIZER_HPP_
