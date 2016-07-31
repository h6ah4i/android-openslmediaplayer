//
//    Copyright (C) 2016 Haruki Hasegawa
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

#include "oslmp/impl/AudioSinkDataPipeReadBlockQueue.hpp"

#include <cxxporthelper/compiler.hpp>

#include <SLESCXX/OpenSLES_CXX.hpp>

#include "oslmp/OpenSLMediaPlayerResultCodes.hpp"

namespace oslmp {
namespace impl {

using namespace ::opensles;

//
// AudioSinkDataPipeReqdBlockQueue
//

AudioSinkDataPipeReadBlockQueue::AudioSinkDataPipeReadBlockQueue() : capacity_(0), count_(0), rp_(0), wp_(0) {}

AudioSinkDataPipeReadBlockQueue::~AudioSinkDataPipeReadBlockQueue() {}

int AudioSinkDataPipeReadBlockQueue::initialize(int capacity) noexcept
{
    if (CXXPH_UNLIKELY(!(capacity >= 2 && capacity <= MAX_BLOCK_COUNT)))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    capacity_ = capacity;
    clear();

    return OSLMP_RESULT_SUCCESS;
}

int AudioSinkDataPipeReadBlockQueue::capacity() const noexcept { return capacity_; }

int AudioSinkDataPipeReadBlockQueue::count() const noexcept { return count_; }

void AudioSinkDataPipeReadBlockQueue::clear() noexcept
{
    count_ = 0;
    rp_ = 0;
    wp_ = 0;
}

bool AudioSinkDataPipeReadBlockQueue::enqueue(const AudioSinkDataPipe::read_block_t &src) noexcept
{
    if (CXXPH_UNLIKELY(count_ >= capacity_))
        return false;

    rb_queue_[wp_] = src;
    wp_ += 1;
    if (CXXPH_UNLIKELY(wp_ >= capacity_)) {
        wp_ = 0;
    }
    ++count_;

    return true;
}

bool AudioSinkDataPipeReadBlockQueue::dequeue(AudioSinkDataPipe::read_block_t &dest) noexcept
{
    if (CXXPH_UNLIKELY(count_ <= 0))
        return false;

    dest = rb_queue_[rp_];
    rp_ += 1;
    if (CXXPH_UNLIKELY(rp_ >= capacity_)) {
        rp_ = 0;
    }
    --count_;

    return true;
}

} // namespace impl
} // namespace oslmp
