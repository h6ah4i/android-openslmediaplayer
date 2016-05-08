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

#ifndef AUDIOCAPTUREDATAPIPE_HPP_
#define AUDIOCAPTUREDATAPIPE_HPP_

#include <time.h>

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

namespace oslmp {
namespace impl {

class AudioCaptureDataPipe {
public:
    enum { MAX_BUFFER_ITEM_COUNT = 31 };

    typedef float data_type;

    struct initialize_args_t {
        uint32_t num_buffer_items;
        uint32_t num_channels;
        uint32_t num_frames;
        bool deferred_buffer_alloc;
    };

    struct write_block_t {
        data_type *dest;
        uint32_t num_channels;
        uint32_t num_frames;
        timespec timestamp;
        uintptr_t lock;
    };

    struct read_block_t {
        const data_type *src;
        uint32_t num_channels;
        uint32_t num_frames;
        timespec timestamp;
        uintptr_t lock;
    };

    AudioCaptureDataPipe();
    ~AudioCaptureDataPipe();

    int initialize(const initialize_args_t &args) noexcept;
    int reset() noexcept;
    int allocateBuffer() noexcept;
    int releaseBuffer() noexcept;

    bool lockWrite(write_block_t &block, size_t min_remains = 0) noexcept;
    bool unlockWrite(write_block_t &block) noexcept;

    bool lockRead(read_block_t &block, size_t min_remains = 0) noexcept;
    bool unlockRead(read_block_t &block) noexcept;

    bool isCapturedDataAvailable() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOCAPTUREDATAPIPE_HPP_
