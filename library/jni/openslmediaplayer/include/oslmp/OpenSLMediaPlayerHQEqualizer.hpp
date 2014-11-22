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

#ifndef OPENSLMEDIAPLAYERHQEQUALIZER_HPP_
#define OPENSLMEDIAPLAYERHQEQUALIZER_HPP_

#include <oslmp/OpenSLMediaPlayerAPICommon.hpp>

namespace oslmp {

class OpenSLMediaPlayerContext;

class OpenSLMediaPlayerHQEqualizer : public virtual android::RefBase {
public:
    enum { NUM_BANDS = 10 };
    struct Settings {
        uint16_t curPreset;
        uint16_t numBands;
        int16_t bandLevels[NUM_BANDS];
    };

    OpenSLMediaPlayerHQEqualizer(const android::sp<OpenSLMediaPlayerContext> &context) OSLMP_API_ABI;
    virtual ~OpenSLMediaPlayerHQEqualizer() OSLMP_API_ABI;

    int setEnabled(bool enabled) noexcept OSLMP_API_ABI;
    int getEnabled(bool *enabled) noexcept OSLMP_API_ABI;
    int getId(int *id) noexcept OSLMP_API_ABI;
    int hasControl(bool *hasControl) noexcept OSLMP_API_ABI;
    int getBand(uint32_t frequency, uint16_t *band) noexcept OSLMP_API_ABI;
    int getBandFreqRange(uint16_t band, uint32_t *range) noexcept OSLMP_API_ABI;
    int getBandLevel(uint16_t band, int16_t *level) noexcept OSLMP_API_ABI;
    int getBandLevelRange(int16_t *range) noexcept OSLMP_API_ABI;
    int getCenterFreq(uint16_t band, uint32_t *center_freq) noexcept OSLMP_API_ABI;
    int getCurrentPreset(uint16_t *preset) noexcept OSLMP_API_ABI;
    int getNumberOfBands(uint16_t *num_bands) noexcept OSLMP_API_ABI;
    int getNumberOfPresets(uint16_t *num_presets) noexcept OSLMP_API_ABI;
    int getPresetName(uint16_t preset, const char **name) noexcept OSLMP_API_ABI;
    int getProperties(Settings *settings) noexcept OSLMP_API_ABI;
    int setBandLevel(uint16_t band, int16_t level) noexcept OSLMP_API_ABI;
    int usePreset(uint16_t preset) noexcept OSLMP_API_ABI;
    int setProperties(const Settings *settings) noexcept OSLMP_API_ABI;

private:
    class Impl;
    Impl *impl_; // NOTE: do not use unique_ptr to avoid cxxporthelper dependencies
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYERHQEQUALIZER_HPP_
