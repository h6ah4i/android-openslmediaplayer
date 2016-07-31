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

#ifndef MIXINGUNIT_HPP_
#define MIXINGUNIT_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>
#include <cxxporthelper/compiler.hpp>

#include "oslmp/impl/AudioDataTypes.hpp"

//
// forward declarations
//
namespace oslmp {
namespace impl {
class MixedOutputAudioEffect;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

class MixingUnit {
public:
    enum mode_t {
        MODE_MUTE,
        MODE_ADD,
        MODE_SHORT_FADE_IN,
        MODE_SHORT_FADE_OUT,
        MODE_LONG_FADE_IN,
        MODE_LONG_FADE_OUT,
    };

    struct Context {
        mode_t mode;
        float phase;
        float volume[2];

        Context() : mode(MODE_MUTE), phase(0.0f)
        {
            for (auto &v : volume) {
                v = 0.0f;
            }
        }
    };

    struct initialize_args_t {
        uint32_t sampling_rate; // [millihertz]
        uint32_t block_size_in_frames;
        uint32_t short_fade_duration_ms;
        uint32_t long_fade_duration_ms;

        initialize_args_t()
            : sampling_rate(0), block_size_in_frames(0), short_fade_duration_ms(0), long_fade_duration_ms(0)
        {
        }
    };

    typedef float in_data_type;
    typedef float capture_data_type;

    MixingUnit();
    ~MixingUnit();

    bool initialize(const initialize_args_t &args) noexcept;

    uint32_t blockSizeInFrames() const noexcept;

    bool begin(void *CXXPH_RESTRICT dest, sample_format_type dest_sample_format,
               capture_data_type *CXXPH_RESTRICT capture, uint32_t size_in_frames,
               MixedOutputAudioEffect *mixout_effects[], uint32_t num_mixout_effects) noexcept;
    bool end() noexcept;

    bool mix(Context *context, const in_data_type *src, uint32_t size_in_frames) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // MIXINGUNIT_HPP_
