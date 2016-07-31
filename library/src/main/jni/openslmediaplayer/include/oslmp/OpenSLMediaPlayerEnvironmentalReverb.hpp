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

#ifndef OPENSLMEDIAPLAYERENVIRONMENTALREVERB_HPP_
#define OPENSLMEDIAPLAYERENVIRONMENTALREVERB_HPP_

#include <oslmp/OpenSLMediaPlayerAPICommon.hpp>

namespace oslmp {

class OpenSLMediaPlayerContext;

class OpenSLMediaPlayerEnvironmentalReverb : public virtual android::RefBase {
public:
    struct Settings {
        int16_t roomLevel;         // [millibel]
        int16_t roomHFLevel;       // [millibel]
        uint32_t decayTime;        // [milliseconds]
        int16_t decayHFRatio;      // [permille]
        int16_t reflectionsLevel;  // [millibel]
        uint32_t reflectionsDelay; // [milliseconds]
        int16_t reverbLevel;       // [millibel]
        uint32_t reverbDelay;      // [milliseconds]
        int16_t diffusion;         // [permille]
        int16_t density;           // [permille]
    };

    OpenSLMediaPlayerEnvironmentalReverb(const android::sp<OpenSLMediaPlayerContext> &context) OSLMP_API_ABI;
    virtual ~OpenSLMediaPlayerEnvironmentalReverb() OSLMP_API_ABI;

    int setEnabled(bool enabled) noexcept OSLMP_API_ABI;
    int getEnabled(bool *enabled) noexcept OSLMP_API_ABI;
    int getId(int32_t *id) noexcept OSLMP_API_ABI;
    int hasControl(bool *hasControl) noexcept OSLMP_API_ABI;

    int getDecayHFRatio(int16_t *decayHFRatio) noexcept OSLMP_API_ABI;
    int getDecayTime(uint32_t *decayTime) noexcept OSLMP_API_ABI;
    int getDensity(int16_t *density) noexcept OSLMP_API_ABI;
    int getDiffusion(int16_t *diffusion) noexcept OSLMP_API_ABI;
    int getReflectionsDelay(uint32_t *reflectionsDelay) noexcept OSLMP_API_ABI;
    int getReflectionsLevel(int16_t *reflectionsLevel) noexcept OSLMP_API_ABI;
    int getReverbDelay(uint32_t *reverbDelay) noexcept OSLMP_API_ABI;
    int getReverbLevel(int16_t *reverbLevel) noexcept OSLMP_API_ABI;
    int getRoomHFLevel(int16_t *roomHF) noexcept OSLMP_API_ABI;
    int getRoomLevel(int16_t *room) noexcept OSLMP_API_ABI;
    int getProperties(Settings *settings) noexcept OSLMP_API_ABI;
    int setDecayHFRatio(int16_t decayHFRatio) noexcept OSLMP_API_ABI;
    int setDecayTime(uint32_t decayTime) noexcept OSLMP_API_ABI;
    int setDensity(int16_t density) noexcept OSLMP_API_ABI;
    int setDiffusion(int16_t diffusion) noexcept OSLMP_API_ABI;
    int setReflectionsDelay(uint32_t reflectionsDelay) noexcept OSLMP_API_ABI;
    int setReflectionsLevel(int16_t reflectionsLevel) noexcept OSLMP_API_ABI;
    int setReverbDelay(uint32_t reverbDelay) noexcept OSLMP_API_ABI;
    int setReverbLevel(int16_t reverbLevel) noexcept OSLMP_API_ABI;
    int setRoomHFLevel(int16_t roomHF) noexcept OSLMP_API_ABI;
    int setRoomLevel(int16_t room) noexcept OSLMP_API_ABI;
    int setProperties(const Settings *settings) noexcept OSLMP_API_ABI;

private:
    class Impl;
    Impl *impl_; // NOTE: do not use unique_ptr to avoid cxxporthelper dependencies
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYERENVIRONMENTALREVERB_HPP_
