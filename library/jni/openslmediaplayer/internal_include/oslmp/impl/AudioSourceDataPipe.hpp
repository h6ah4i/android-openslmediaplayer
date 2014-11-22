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

#ifndef AUDIOSOURCEDATAPIPE_HPP_
#define AUDIOSOURCEDATAPIPE_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

namespace oslmp {
namespace impl {

class AudioSourceDataPipe {
public:
    typedef float data_type;

    enum { MAX_BUFFER_ITEM_COUNT = 1023 };

    enum { TAG_NONE, TAG_AUDIO_DATA, TAG_EVENT_END_OF_DATA, TAG_EVENT_END_OF_DATA_WITH_LOOP_POINT, };

    struct initialize_args_t {
        uint32_t num_buffer_items;
        uint32_t num_channels;
        uint32_t num_frames;
        bool deferred_buffer_alloc;
    };

    struct produce_block_t {
        data_type *dest;
        uint32_t num_channels;
        uint32_t num_frames;
        uint32_t tag;
        int32_t position_msec;
        uintptr_t lock;
    };

    struct consume_block_t {
        const data_type *src;
        uint32_t num_channels;
        uint32_t num_frames;
        uint32_t tag;
        int32_t position_msec;
        uintptr_t lock;
    };

    struct recycle_block_t {
        const data_type *src;
        uint32_t num_channels;
        uint32_t num_frames;
        uint32_t tag;
        int32_t position_msec;
        uintptr_t lock;
    };

    AudioSourceDataPipe();
    ~AudioSourceDataPipe();

    int initialize(const initialize_args_t &args) noexcept;
    int reset() noexcept;
    int allocateBuffer() noexcept;
    int releaseBuffer() noexcept;

    bool lockProduce(produce_block_t &block, size_t min_remains = 0) noexcept;
    bool unlockProduce(produce_block_t &block) noexcept;

    bool lockConsume(consume_block_t &block, size_t min_remains = 0, uint32_t tag_mask_bitmap = 0xFFFFFFFFUL) noexcept;
    bool unlockConsume(consume_block_t &block) noexcept;

    bool lockRecycle(recycle_block_t &block, size_t min_remains = 0) noexcept;
    bool unlockRecycle(recycle_block_t &block) noexcept;

    uint32_t getCapacity() const noexcept;

    uint32_t consumerGetLastBlockTag() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOSOURCEDATAPIPE_HPP_
