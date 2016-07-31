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

#ifndef OPENSLMEDIAPLAYERPRESETREVERB_HPP_
#define OPENSLMEDIAPLAYERPRESETREVERB_HPP_

#include <oslmp/OpenSLMediaPlayerAPICommon.hpp>

namespace oslmp {

class OpenSLMediaPlayerContext;

class OpenSLMediaPlayerPresetReverb : public virtual android::RefBase {
public:
    enum {
        PRESET_NONE = 0,
        PRESET_SMALLROOM = 1,
        PRESET_MEDIUMROOM = 2,
        PRESET_LARGEROOM = 3,
        PRESET_MEDIUMHALL = 4,
        PRESET_LARGEHALL = 5,
        PRESET_PLATE = 6,
    };

    struct Settings {
        uint16_t preset;
    };

    OpenSLMediaPlayerPresetReverb(const android::sp<OpenSLMediaPlayerContext> &context) OSLMP_API_ABI;
    virtual ~OpenSLMediaPlayerPresetReverb() OSLMP_API_ABI;

    int setEnabled(bool enabled) noexcept OSLMP_API_ABI;
    int getEnabled(bool *enabled) noexcept OSLMP_API_ABI;
    int getId(int32_t *id) noexcept OSLMP_API_ABI;
    int hasControl(bool *hasControl) noexcept OSLMP_API_ABI;
    int getPreset(uint16_t *preset) noexcept OSLMP_API_ABI;
    int getProperties(Settings *settings) noexcept OSLMP_API_ABI;
    int setPreset(uint16_t preset) noexcept OSLMP_API_ABI;
    int setProperties(const Settings *settings) noexcept OSLMP_API_ABI;

private:
    class Impl;
    Impl *impl_; // NOTE: do not use unique_ptr to avoid cxxporthelper dependencies
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYERPRESETREVERB_HPP_
