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

#ifndef HQVISUALIZERCAPTUREDAUDIODATABUFFER_HPP_
#define HQVISUALIZERCAPTUREDAUDIODATABUFFER_HPP_

#include <cxxporthelper/cstdint>
#include <cxxporthelper/memory>
#include <cxxporthelper/time.hpp>
#include <cxxporthelper/aligned_memory.hpp>

#include "oslmp/utils/pthread_utils.hpp"

namespace oslmp {
namespace impl {

class HQVisualizerCapturedAudioDataBuffer {
public:
    HQVisualizerCapturedAudioDataBuffer(uint32_t buffer_size_in_frames, uint32_t max_read_size_in_frames);
    ~HQVisualizerCapturedAudioDataBuffer();

    void reset() noexcept;

    bool put_captured_data(uint32_t num_channels, uint32_t sampling_rate, const float *data, uint32_t num_frames,
                           const timespec *captured_time) noexcept;

    bool get_captured_data(uint32_t num_frames, uint32_t read_rate, uint32_t *num_channels, uint32_t *sampling_rate,
                           const float **data) noexcept;

    bool get_latest_captured_data(uint32_t num_frames, uint32_t *num_channels, uint32_t *sampling_rate,
                                  const float **data, timespec *updated_time) noexcept;

    bool get_last_updated_time(timespec *updated_time) const noexcept;

    void set_output_latency(uint32_t latency_in_frames) noexcept;

    uint32_t get_output_latency() const noexcept;

private:
    void initialize(uint32_t sampling_rate) noexcept;

    void copy_to_buffer(uint32_t num_channels, const float *src, uint32_t src_pos, uint32_t dest_pos,
                        uint32_t n) noexcept;

private:
    // MEMO:
    // << buffer >>
    //        buffer_size_in_frames           +-- max_read_size_in_frames
    //                 |                      |
    // +----------------------------------+-------+
    // |#######                           |#######|
    // +----------------------------------+-------+
    //     |                                  ^
    //     +---------- mirrored --------------+

    const uint32_t buffer_num_channels_;
    const uint32_t buffer_size_in_frames_;
    const uint32_t max_read_size_in_frames_;

    mutable utils::pt_mutex syncobj_;
    uint32_t sampling_rate_; // [milli hertz]
    uint32_t num_channels_;  // [channels]
    uint32_t read_pos_;      // [frames]
    uint32_t write_pos_;     // [frames]
    timespec read_time_;
    timespec write_time_;
    cxxporthelper::aligned_memory<float> buffer_;
    uint32_t output_latency_; // [frames]

    // for debug purposes
    uint32_t dbg_write_counter_;
    uint32_t dbg_read_counter_;
};

} // namespace impl
} // namespace oslmp

#endif // HQVISUALIZERCAPTUREDAUDIODATABUFFER_HPP_
