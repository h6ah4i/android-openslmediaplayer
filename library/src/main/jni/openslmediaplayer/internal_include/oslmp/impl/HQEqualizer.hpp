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

#ifndef HQEQUALIZER_HPP_
#define HQEQUALIZER_HPP_

#include <cxxporthelper/memory>

#include "oslmp/impl/MixedOutputAudioEffect.hpp"

namespace oslmp {
namespace impl {

class HQEqualizer : public MixedOutputAudioEffect {
public:
    enum { NUM_BANDS = 10, };

    enum impl_type_specifiler {
        kImplBasicPeakingFilter = 0, ///< Basic graphic equalizer using cascaded peaking filter
        kImplFlatGain = 1, ///< Flat gain response graphic equalizer  (using x2 amount of low/high shelf filters)
    };

    static constexpr uint16_t UNDEFINED = 0xffffU;

    struct initialize_args_t {
        uint32_t num_channels;
        uint32_t sampling_rate; // [millihertz]
        uint32_t block_size_in_frames;
        impl_type_specifiler impl_type;

        initialize_args_t()
            : num_channels(0), sampling_rate(0), block_size_in_frames(0), impl_type(kImplBasicPeakingFilter)
        {
        }
    };

    HQEqualizer();
    virtual ~HQEqualizer();

    bool initialize(const initialize_args_t &args) noexcept;

    int setEnabled(bool enabled) noexcept;
    int getEnabled(bool *enabled) const noexcept;
    int getBand(uint32_t frequency, uint16_t *band) const noexcept;
    int setBandLevel(uint16_t band, int16_t level) noexcept;
    int getBandLevel(uint16_t band, int16_t *level) const noexcept;
    int getBandFreqRange(uint16_t band, uint32_t *min_freq, uint32_t *max_freq) const noexcept;
    int getBandLevelRange(int16_t *min_level, int16_t *max_level) const noexcept;
    int getCenterFreq(uint16_t band, uint32_t *center_freq) const noexcept;
    int getNumberOfBands(uint16_t *num_bands) const noexcept;
    int setAllBandLevel(const int16_t *level, uint16_t num_bands) noexcept;
    int getAllBandLevel(int16_t *level, uint16_t num_bands) const noexcept;

    // implements MixedOutputAudioEffect
    virtual bool isPollingRequired() const noexcept;
    virtual int poll() noexcept;

    virtual void onAttachedToMixerThread() noexcept;
    virtual void onDetachedFromMixerThread() noexcept;

    virtual int pollFromMixerThread() noexcept;
    virtual int process(float *data, uint32_t num_channels, uint32_t num_frames) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // HQEQUALIZER_HPP_
