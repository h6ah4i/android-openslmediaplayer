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

#ifndef AUDIODATAADAPETR_HPP_
#define AUDIODATAADAPETR_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>
#include <cxxporthelper/compiler.hpp>

namespace oslmp {
namespace impl {

class AudioDataAdapter {
public:
    enum resampler_quality_spec_t {
        RESAMPLER_QUALITY_LOW = 0,
        RESAMPLER_QUALITY_MIDDLE = 1,
        RESAMPLER_QUALITY_HIGH = 2,
    };

    struct initialize_args_t {
        uint32_t in_num_channels;
        uint32_t in_sampling_rate;
        uint32_t in_block_size;

        uint32_t out_num_channels;
        uint32_t out_sampling_rate;
        uint32_t out_block_size;

        resampler_quality_spec_t resampler_quality_spec;

        initialize_args_t()
            : in_num_channels(0), in_sampling_rate(0), in_block_size(0), out_num_channels(0), out_sampling_rate(0),
              out_block_size(0), resampler_quality_spec(RESAMPLER_QUALITY_MIDDLE)
        {
        }
    };

    AudioDataAdapter();
    ~AudioDataAdapter();

    bool init(const initialize_args_t &args) noexcept;

    bool is_output_data_ready() const noexcept;

    bool put_input_data(const int16_t *data, uint32_t num_channels, uint32_t num_frames) noexcept;
    bool get_output_data(float *data, uint32_t num_channels, uint32_t num_frames) noexcept;

    bool flush() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIODATAADAPETR_HPP_
