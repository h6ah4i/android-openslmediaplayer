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

#include "oslmp/impl/HQEqualizerPresets.hpp"

#include <cxxporthelper/compiler.hpp>

#include "oslmp/impl/HQEqualizer.hpp"

namespace oslmp {
namespace impl {

#define NUM_BANDS HQEqualizer::NUM_BANDS

static const int16_t gPreset_Normal_BandLevel[NUM_BANDS] = { 300, 300, 200, 0, 0, 0, 0, 100, 300, 300 };
static const HQEqualizerPresets::PresetData gPreset_Normal = { "Normal", NUM_BANDS, gPreset_Normal_BandLevel };

static const int16_t gPreset_Classical_BandLevel[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, -100, -100, -200, -300 };
static const HQEqualizerPresets::PresetData gPreset_Classical = { "Classical", NUM_BANDS, gPreset_Classical_BandLevel };

static const int16_t gPreset_Dance_BandLevel[NUM_BANDS] = { 500, 400, 300, 0, 0, -100, 0, 100, 100, 100 };
static const HQEqualizerPresets::PresetData gPreset_Dance = { "Dance", NUM_BANDS, gPreset_Dance_BandLevel };

static const int16_t gPreset_Flat_BandLevel[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const HQEqualizerPresets::PresetData gPreset_Flat = { "Flat", NUM_BANDS, gPreset_Flat_BandLevel };

static const int16_t gPreset_Folk_BandLevel[NUM_BANDS] = { 200, 300, 300, 100, 0, 0, 0, 200, -100, -100 };
static const HQEqualizerPresets::PresetData gPreset_Folk = { "Folk", NUM_BANDS, gPreset_Folk_BandLevel };

static const int16_t gPreset_Heavy_Metal_BandLevel[NUM_BANDS] = { 400, 400, 200, 100, 0, 300, 500, 100, 200, 400 };
static const HQEqualizerPresets::PresetData gPreset_Heavy_Metal = { "Heavy Metal", NUM_BANDS,
                                                                    gPreset_Heavy_Metal_BandLevel };

static const int16_t gPreset_Hip_Hop_BandLevel[NUM_BANDS] = { 400, 500, 400, 300, 0, 0, 100, 100, 200, 300 };
static const HQEqualizerPresets::PresetData gPreset_Hip_Hop = { "Hip Hop", NUM_BANDS, gPreset_Hip_Hop_BandLevel };

static const int16_t gPreset_Jazz_BandLevel[NUM_BANDS] = { 300, 400, 400, 200, 0, -200, 0, 200, 300, 500 };
static const HQEqualizerPresets::PresetData gPreset_Jazz = { "Jazz", NUM_BANDS, gPreset_Jazz_BandLevel };

static const int16_t gPreset_Pop_BandLevel[NUM_BANDS] = { -100, -100, 0, 200, 300, 500, 200, 100, -100, -200 };
static const HQEqualizerPresets::PresetData gPreset_Pop = { "Pop", NUM_BANDS, gPreset_Pop_BandLevel };

static const int16_t gPreset_Rock_BandLevel[NUM_BANDS] = { 400, 500, 400, 300, 0, -100, 0, 300, 400, 500 };
static const HQEqualizerPresets::PresetData gPreset_Rock = { "Rock", NUM_BANDS, gPreset_Rock_BandLevel };

static HQEqualizerPresets::PresetData const *const gHQEqualizerPresets[] = {
    &gPreset_Normal,      &gPreset_Classical, &gPreset_Dance, &gPreset_Flat, &gPreset_Folk,
    &gPreset_Heavy_Metal, &gPreset_Hip_Hop,   &gPreset_Jazz,  &gPreset_Pop,  &gPreset_Rock,
};

int16_t HQEqualizerPresets::sGetNumPresets() noexcept
{
    return sizeof(gHQEqualizerPresets) / sizeof(gHQEqualizerPresets[0]);
}

const HQEqualizerPresets::PresetData *HQEqualizerPresets::sGetPresetData(uint16_t preset_no) noexcept
{
    if (CXXPH_UNLIKELY(!(preset_no < sGetNumPresets()))) {
        return nullptr;
    }

    return gHQEqualizerPresets[preset_no];
}

} // namespace impl
} // namespace oslmp
