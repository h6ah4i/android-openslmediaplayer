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

// #define LOG_TAG "VisualizerCapturedAudioDataBuffer"

#include "oslmp/impl/VisualizerCapturedAudioDataBuffer.hpp"

#include <cassert>
#include <cstring>
#include <algorithm>

#include <cxxporthelper/memory>
#include <cxxporthelper/compiler.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/utils/timespec_utils.hpp"

#define NUM_CHANNELS 2

namespace oslmp {
namespace impl {

static void copy_audio_data(uint32_t num_channels, const int16_t *src, uint32_t src_pos, int16_t *dest,
                            uint32_t dest_pos, uint32_t n) noexcept;

static void copy_audio_data_stereo(const int16_t *CXXPH_RESTRICT src, uint32_t src_pos, int16_t *CXXPH_RESTRICT dest,
                                   uint32_t dest_pos, uint32_t n) noexcept;

VisualizerCapturedAudioDataBuffer::VisualizerCapturedAudioDataBuffer(uint32_t buffer_size_in_frames,
                                                                     uint32_t max_read_size_in_frames)
    : buffer_num_channels_(NUM_CHANNELS), buffer_size_in_frames_(buffer_size_in_frames),
      max_read_size_in_frames_(max_read_size_in_frames), syncobj_(), sampling_rate_(0), read_pos_(0), write_pos_(0),
      read_time_(utils::timespec_utils::ZERO()), write_time_(utils::timespec_utils::ZERO()), buffer_(),
      output_latency_(0), dbg_write_counter_(0), dbg_read_counter_(0)
{
    // allocate buffer (with zero filled)
    buffer_.allocate(buffer_num_channels_ * (buffer_size_in_frames_ + max_read_size_in_frames_));
}

VisualizerCapturedAudioDataBuffer::~VisualizerCapturedAudioDataBuffer() {}

void VisualizerCapturedAudioDataBuffer::reset() noexcept
{
    utils::pt_lock_guard lock(syncobj_);
    initialize(0);
}

bool VisualizerCapturedAudioDataBuffer::put_captured_data(uint32_t num_channels, uint32_t sampling_rate,
                                                          const int16_t *data, uint32_t num_frames,
                                                          const timespec *captured_time) noexcept
{

    if (CXXPH_UNLIKELY(num_channels != NUM_CHANNELS))
        return false;

    if (CXXPH_UNLIKELY(sampling_rate == 0))
        return false;

    if (CXXPH_UNLIKELY(!buffer_))
        return false;

    if (CXXPH_UNLIKELY(num_frames > buffer_size_in_frames_))
        return false;

    if (CXXPH_UNLIKELY(!captured_time))
        return false;

    uint32_t prev_write_pos;
    timespec prev_write_time;

    {
        utils::pt_lock_guard lock(syncobj_);

        if (CXXPH_UNLIKELY(!(sampling_rate_ == sampling_rate))) {
            initialize(sampling_rate);
        }

        prev_write_pos = write_pos_;
        prev_write_time = write_time_;
    }

    // NOTE:
    // The mutex is unlocked here, it's intended to minimize critical section.
    // (data corruption may be occurred, but that's not serious
    //  problem for visualization purpose)

    // copy audio data
    const uint32_t src_pos1 = 0;
    const uint32_t dest_pos1 = prev_write_pos;
    const uint32_t n1 = (std::min)(num_frames, (buffer_size_in_frames_ - dest_pos1));

    const uint32_t src_pos2 = n1;
    const uint32_t dest_pos2 = 0;
    const uint32_t n2 = (num_frames - n1);

    uint32_t new_write_pos = prev_write_pos;

    if (CXXPH_LIKELY(n1 > 0)) {
        copy_to_buffer(num_channels, data, src_pos1, dest_pos1, n1);
        new_write_pos = dest_pos1 + n1;
    }

    if (CXXPH_UNLIKELY(n2 > 0)) {
        copy_to_buffer(num_channels, data, src_pos2, dest_pos2, n2);
        new_write_pos = dest_pos2 + n2;
    }

    if (CXXPH_UNLIKELY(new_write_pos == buffer_size_in_frames_)) {
        new_write_pos = 0;
    }

    // update write_pos_ and write_time_
    const timespec new_write_time = *captured_time;

    {
        utils::pt_lock_guard lock(syncobj_);

        dbg_write_counter_ += 1;

        if (prev_write_pos == write_pos_ && utils::timespec_utils::compare(prev_write_time, write_time_)) {

            assert(new_write_pos >= 0 && new_write_pos < buffer_size_in_frames_);

            write_pos_ = new_write_pos;
            write_time_ = new_write_time;

#ifdef LOG_TAG
            timespec now;
            utils::timespec_utils::get_current_time(now);
            LOGV("[W] count = %u, pos = %d, time = (%ld, %ld), datatime = (%ld, %ld)", dbg_write_counter_, write_pos_,
                 now.tv_sec, now.tv_nsec, write_time_.tv_sec, write_time_.tv_nsec);
#endif
        } else {
// data corruption occurred !
#ifdef LOG_TAG
            timespec now;
            utils::timespec_utils::get_current_time(now);
            LOGD("[W!] count = %u pos = %d, time = (%ld, %ld), datatime = (%ld, %ld)", dbg_write_counter_, write_pos_,
                 now.tv_sec, now.tv_nsec, write_time_.tv_sec, write_time_.tv_nsec);
#endif
        }
    }

    return true;
}

bool VisualizerCapturedAudioDataBuffer::get_captured_data(uint32_t num_frames, uint32_t read_rate,
                                                          uint32_t *num_channels, uint32_t *sampling_rate,
                                                          const int16_t **data) noexcept
{

    if (CXXPH_UNLIKELY(!(num_frames <= max_read_size_in_frames_)))
        return false;

    if (CXXPH_UNLIKELY(read_rate == 0))
        return false;

    if (CXXPH_UNLIKELY(!(num_channels && sampling_rate && data)))
        return false;

    (*num_channels) = 0;
    (*sampling_rate) = 0;
    (*data) = nullptr;

    if (CXXPH_UNLIKELY(!buffer_))
        return false;

    {
        utils::pt_lock_guard lock(syncobj_);

        if (CXXPH_UNLIKELY(sampling_rate_ == 0))
            return false;

        timespec now;

        if (CXXPH_UNLIKELY(!utils::timespec_utils::get_current_time(now)))
            return false;

        const double diff_write_time = utils::timespec_utils::sub_ret_us(now, write_time_) * (1.0 / 1000000);

        if (CXXPH_UNLIKELY(diff_write_time > 0.5 /* [sec.] */)) {
            // captured data is too old
            LOGD("[R!] too old data");

            return false;
        }

        const double sampling_rate_hz = sampling_rate_ * (1 / 1000.);

        const double read_interval = 1000. / read_rate;
        const int32_t read_interval_frames = static_cast<int32_t>(read_interval * sampling_rate_hz);
        const int32_t num_available_frames = (write_pos_ >= read_pos_)
                                                 ? (write_pos_ - read_pos_)
                                                 : (buffer_size_in_frames_ + write_pos_ - read_pos_ - 1);

        // use read pointer based position (with adjustment)
        const int32_t offset = std::max(num_frames, (num_frames / 2) + output_latency_);
        const int32_t adj = static_cast<int32_t>(0.01 * ((num_available_frames - offset)));
        const int32_t skip = (std::min)((read_interval_frames + adj), (num_available_frames - offset));
        int32_t next_read_pos = static_cast<int32_t>(read_pos_) + skip;

        // correct read pointer range
        next_read_pos = (next_read_pos % buffer_size_in_frames_);
        if (next_read_pos < 0)
            next_read_pos += buffer_size_in_frames_;

        assert(next_read_pos >= 0 && next_read_pos < buffer_size_in_frames_);

        // update fields
        read_pos_ = next_read_pos;
        read_time_ = now;

        // store results
        (*num_channels) = buffer_num_channels_;
        (*sampling_rate) = sampling_rate_;
        (*data) = &buffer_[buffer_num_channels_ * read_pos_];

        dbg_read_counter_ += 1;

        LOGV("[R] count = %u, pos = %d, time = (%ld, %ld)", dbg_read_counter_, read_pos_, read_time_.tv_sec,
             read_time_.tv_nsec);
    }

    // NOTE:
    // The mutex is unlocked here, so accessing the buffer through the '*data'
    // pointer is not protected, and may causes data corruption. However the captured
    // data buffer is enough large and data corruption may not occurs easily.

    return true;
}

bool VisualizerCapturedAudioDataBuffer::get_latest_captured_data(uint32_t num_frames, uint32_t *num_channels,
                                                                 uint32_t *sampling_rate, const int16_t **data,
                                                                 timespec *updated_time) noexcept
{
    if (CXXPH_UNLIKELY(!(num_frames <= max_read_size_in_frames_)))
        return false;

    if (CXXPH_UNLIKELY(!(num_channels && sampling_rate && data)))
        return false;

    (*num_channels) = 0;
    (*sampling_rate) = 0;
    (*data) = nullptr;
    (*updated_time) = utils::timespec_utils::ZERO();

    if (CXXPH_UNLIKELY(!buffer_))
        return false;

    {
        utils::pt_lock_guard lock(syncobj_);

        if (CXXPH_UNLIKELY(sampling_rate_ == 0))
            return false;

        int32_t read_pos = static_cast<int32_t>(write_pos_) - num_frames;

        // correct read position
        if (read_pos < 0)
            read_pos += buffer_size_in_frames_;

        (*num_channels) = buffer_num_channels_;
        (*sampling_rate) = sampling_rate_;
        (*data) = &buffer_[read_pos];
        (*updated_time) = write_time_;
    }

    return true;
}

bool VisualizerCapturedAudioDataBuffer::get_last_updated_time(timespec *updated_time) const noexcept
{
    if (CXXPH_UNLIKELY(!updated_time))
        return false;

    {
        utils::pt_lock_guard lock(syncobj_);

        (*updated_time) = write_time_;
    }

    return true;
}

void VisualizerCapturedAudioDataBuffer::initialize(uint32_t sampling_rate) noexcept
{
    sampling_rate_ = sampling_rate;
    read_pos_ = 0;
    read_time_ = utils::timespec_utils::ZERO();
    write_pos_ = 0;
    write_time_ = utils::timespec_utils::ZERO();
}

void VisualizerCapturedAudioDataBuffer::copy_to_buffer(uint32_t num_channels, const int16_t *src, uint32_t src_pos,
                                                       uint32_t dest_pos, uint32_t n) noexcept
{
    copy_audio_data(num_channels, src, src_pos, &buffer_[0], dest_pos, n);

    // mirroring
    if (dest_pos < max_read_size_in_frames_) {
        copy_audio_data(num_channels, src, src_pos, &buffer_[buffer_size_in_frames_ * buffer_num_channels_], dest_pos,
                        (std::min)(n, (max_read_size_in_frames_ - dest_pos)));
    }
}

void VisualizerCapturedAudioDataBuffer::set_output_latency(uint32_t latency_in_frames) noexcept
{
    utils::pt_lock_guard lock(syncobj_);

    // max.: (buffer size) / 2
    output_latency_ = (std::min)(latency_in_frames, (buffer_size_in_frames_ / 2));
}

uint32_t VisualizerCapturedAudioDataBuffer::get_output_latency() const noexcept
{
    utils::pt_lock_guard lock(syncobj_);
    return output_latency_;
}

static void copy_audio_data(uint32_t num_channels, const int16_t *src, uint32_t src_pos, int16_t *dest,
                            uint32_t dest_pos, uint32_t n) noexcept
{

    switch (num_channels) {
    case 2:
        copy_audio_data_stereo(src, src_pos, dest, dest_pos, n);
        break;
    }
}

static void copy_audio_data_stereo(const int16_t *CXXPH_RESTRICT src, uint32_t src_pos, int16_t *CXXPH_RESTRICT dest,
                                   uint32_t dest_pos, uint32_t n) noexcept
{
    ::memcpy(&dest[dest_pos * 2], &src[src_pos * 2], (n * 2 * sizeof(int16_t)));
}

} // namespace impl
} // namespace oslmp
