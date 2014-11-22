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

#ifndef AUDIOSINKDATAPIPE_HPP_
#define AUDIOSINKDATAPIPE_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

#include "oslmp/impl/AudioDataTypes.hpp"

namespace oslmp {
namespace impl {

class AudioSinkDataPipe {
public:
    enum { MAX_BUFFER_ITEM_COUNT = 63 };

    struct initialize_args_t {
        sample_format_type sample_format;
        uint32_t num_buffer_items;
        uint32_t num_channels;
        uint32_t num_frames;
        bool deferred_buffer_alloc;
    };

    struct write_block_t {
        sample_format_type sample_format;
        void *dest;
        uint32_t num_channels;
        uint32_t num_frames;
        uintptr_t lock;
#if USE_OSLMP_DEBUG_FEATURES
        uint16_t dbg_lock_index;
#endif
    };

    struct read_block_t {
        sample_format_type sample_format;
        const void *src;
        uint32_t num_channels;
        uint32_t num_frames;
        uintptr_t lock;
#if USE_OSLMP_DEBUG_FEATURES
        uint16_t dbg_lock_index;
#endif
    };

    AudioSinkDataPipe();
    ~AudioSinkDataPipe();

    int initialize(const initialize_args_t &args) noexcept;
    int reset() noexcept;
    int allocateBuffer() noexcept;
    int releaseBuffer() noexcept;

    bool lockWrite(write_block_t &block, size_t min_remains = 0) noexcept;
    bool unlockWrite(write_block_t &block) noexcept;

    bool lockRead(read_block_t &block, size_t min_remains = 0) noexcept;
    bool unlockRead(read_block_t &block) noexcept;

    uint32_t getNumberOfBufferItems() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOSINKDATAPIPE_HPP_
