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

// #define LOG_TAG "AudioSourceDataPipe"

#include "oslmp/impl/AudioSourceDataPipe.hpp"

#include <algorithm>

#include <cxxporthelper/memory>
#include <cxxporthelper/aligned_memory.hpp>

#include <lockfree/lockfree_circulation_buffer.hpp>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#define ROUND_UP_TO_CACHE_LINE_SIZE(x)                                                                                 \
    (((x) + CXXPH_PLATFORM_CACHE_LINE_SIZE - 1) & ~(CXXPH_PLATFORM_CACHE_LINE_SIZE - 1))

namespace oslmp {
namespace impl {

struct AudioSourceDataPipeItem {
    AudioSourceDataPipe::data_type *buffer;
    uint32_t num_channels;
    uint32_t num_frames;
    uint32_t tag;
    int32_t position_msec;

    AudioSourceDataPipeItem()
        : buffer(nullptr), num_channels(0), num_frames(0), tag(AudioSourceDataPipe::TAG_NONE), position_msec(0)
    {
    }

    void clearInfo() noexcept
    {
        tag = AudioSourceDataPipe::TAG_NONE;
        position_msec = 0;
    }
};

typedef lockfree::lockfree_circulation_buffer<
    AudioSourceDataPipeItem *, (AudioSourceDataPipe::MAX_BUFFER_ITEM_COUNT + 1)> audio_source_buffer_queue_t;

class AudioSourceDataPipe::Impl {
public:
    Impl();
    ~Impl();

    int initialize(const initialize_args_t &args) noexcept;
    int reset() noexcept;
    int allocateBuffer() noexcept;
    int releaseBuffer() noexcept;

    bool lockProduce(AudioSourceDataPipe::produce_block_t &block, size_t min_remains) noexcept;
    bool unlockProduce(AudioSourceDataPipe::produce_block_t &block) noexcept;

    bool lockConsume(AudioSourceDataPipe::consume_block_t &block, size_t min_remains,
                     uint32_t tag_mask_bitmap) noexcept;
    bool unlockConsume(AudioSourceDataPipe::consume_block_t &block) noexcept;

    bool lockRecycle(AudioSourceDataPipe::recycle_block_t &block, size_t min_remains) noexcept;
    bool unlockRecycle(AudioSourceDataPipe::recycle_block_t &block) noexcept;

    uint32_t getCapacity() const noexcept;

    uint32_t consumerGetLastBlockTag() const noexcept;

private:
    int setupQueues(const initialize_args_t &args, size_t block_size, uint8_t *buffer_pool) noexcept;

private:
    bool initialized_;

    initialize_args_t init_args_;
    size_t cache_aligned_block_size_;

    AudioSourceDataPipeItem items_pool_[AudioSourceDataPipe::MAX_BUFFER_ITEM_COUNT];
    cxxporthelper::aligned_memory<uint8_t> buffer_pool_;
    audio_source_buffer_queue_t producer_queue_;
    audio_source_buffer_queue_t consumer_queue_;
    audio_source_buffer_queue_t recycler_queue_;

    uint32_t last_consumer_tag_;
};

//
// Utilities
//
static inline void clear(AudioSourceDataPipe::produce_block_t &block) noexcept
{
    block.dest = nullptr;
    block.num_channels = 0;
    block.num_frames = 0;
    block.tag = AudioSourceDataPipe::TAG_NONE;
    block.position_msec = 0;
    block.lock = 0;
}

static inline void clear(AudioSourceDataPipe::consume_block_t &block) noexcept
{
    block.src = nullptr;
    block.num_channels = 0;
    block.num_frames = 0;
    block.tag = AudioSourceDataPipe::TAG_NONE;
    block.position_msec = 0;
    block.lock = 0;
}

static inline void clear(AudioSourceDataPipe::recycle_block_t &block) noexcept
{
    block.src = nullptr;
    block.num_channels = 0;
    block.num_frames = 0;
    block.tag = AudioSourceDataPipe::TAG_NONE;
    block.position_msec = 0;
    block.lock = 0;
}

//
// AudioSourceDataPipe
//

AudioSourceDataPipe::AudioSourceDataPipe() : impl_(new (std::nothrow) Impl()) {}

AudioSourceDataPipe::~AudioSourceDataPipe() {}

int AudioSourceDataPipe::initialize(const AudioSourceDataPipe::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioSourceDataPipe::reset() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->reset();
}

int AudioSourceDataPipe::allocateBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->allocateBuffer();
}

int AudioSourceDataPipe::releaseBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->releaseBuffer();
}

bool AudioSourceDataPipe::lockProduce(AudioSourceDataPipe::produce_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockProduce(block, min_remains);
}

bool AudioSourceDataPipe::unlockProduce(AudioSourceDataPipe::produce_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockProduce(block);
}

bool AudioSourceDataPipe::lockConsume(AudioSourceDataPipe::consume_block_t &block, size_t min_remains,
                                      uint32_t tag_mask_bitmap) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockConsume(block, min_remains, tag_mask_bitmap);
}

bool AudioSourceDataPipe::unlockConsume(AudioSourceDataPipe::consume_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockConsume(block);
}

bool AudioSourceDataPipe::lockRecycle(AudioSourceDataPipe::recycle_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockRecycle(block, min_remains);
}

bool AudioSourceDataPipe::unlockRecycle(AudioSourceDataPipe::recycle_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockRecycle(block);
}

uint32_t AudioSourceDataPipe::getCapacity() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0;
    return impl_->getCapacity();
}

uint32_t AudioSourceDataPipe::consumerGetLastBlockTag() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return TAG_NONE;
    return impl_->consumerGetLastBlockTag();
}

//
// AudioSourceDataPipe::Impl
//

AudioSourceDataPipe::Impl::Impl()
    : initialized_(false), cache_aligned_block_size_(0), buffer_pool_(),
      last_consumer_tag_(AudioSourceDataPipe::TAG_NONE)
{
}

AudioSourceDataPipe::Impl::~Impl() {}

int AudioSourceDataPipe::Impl::initialize(const initialize_args_t &args) noexcept
{
    // check arguments
    if (!(args.num_buffer_items >= 2 && args.num_buffer_items <= MAX_BUFFER_ITEM_COUNT))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (args.num_channels == 0)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (args.num_frames == 0)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    // check state
    if (initialized_)
        return OSLMP_RESULT_ILLEGAL_STATE;

    const size_t block_size = sizeof(float) * args.num_channels * args.num_frames;
    const size_t cache_aligned_block_size = ROUND_UP_TO_CACHE_LINE_SIZE(block_size);
    const size_t num_blocks = args.num_buffer_items;

    cxxporthelper::aligned_memory<uint8_t> buffer_pool;

    if (!args.deferred_buffer_alloc) {
        // allocate memory pool
        buffer_pool.allocate(cache_aligned_block_size * num_blocks, CXXPH_PLATFORM_CACHE_LINE_SIZE);

        if (!buffer_pool)
            return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;

        // setup queue
        int result = setupQueues(args, cache_aligned_block_size, &buffer_pool[0]);

        if (result != OSLMP_RESULT_SUCCESS)
            return result;
    }

    // update field
    buffer_pool_ = std::move(buffer_pool);
    init_args_ = args;
    cache_aligned_block_size_ = cache_aligned_block_size;
    initialized_ = true;
    last_consumer_tag_ = TAG_NONE;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSourceDataPipe::Impl::reset() noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return OSLMP_RESULT_ILLEGAL_STATE;

    last_consumer_tag_ = TAG_NONE;

    int result;
    if (buffer_pool_) {
        result = setupQueues(init_args_, cache_aligned_block_size_, &buffer_pool_[0]);
    } else {
        result = OSLMP_RESULT_SUCCESS;
    }

    return result;
}

int AudioSourceDataPipe::Impl::allocateBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (buffer_pool_)
        return OSLMP_RESULT_SUCCESS;

    LOGD("allocateBuffer()");

    const size_t cache_aligned_block_size = cache_aligned_block_size_;
    const size_t num_blocks = init_args_.num_buffer_items;

    // allocate memory pool
    cxxporthelper::aligned_memory<uint8_t> buffer_pool;

    buffer_pool.allocate(cache_aligned_block_size * num_blocks, CXXPH_PLATFORM_CACHE_LINE_SIZE);

    if (!buffer_pool)
        return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;

    // setup queue
    int result = setupQueues(init_args_, cache_aligned_block_size, &buffer_pool[0]);

    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS)) {
        return result;
    }

    // update field
    buffer_pool_ = std::move(buffer_pool);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSourceDataPipe::Impl::releaseBuffer() noexcept
{
    if (!initialized_)
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (!buffer_pool_)
        return OSLMP_RESULT_SUCCESS;

    LOGD("releaseBuffer()");

    buffer_pool_.free();

    return OSLMP_RESULT_SUCCESS;
}

int AudioSourceDataPipe::Impl::setupQueues(const AudioSourceDataPipe::initialize_args_t &args, size_t block_size,
                                           uint8_t *buffer_pool) noexcept
{
    const size_t num_blocks = args.num_buffer_items;

    producer_queue_.clear();
    consumer_queue_.clear();
    recycler_queue_.clear();

    bool failed = false;
    for (size_t i = 0; i < num_blocks; ++i) {
        audio_source_buffer_queue_t::index_t index = audio_source_buffer_queue_t::INVALID_INDEX;

        if (producer_queue_.lock_write(index)) {
            AudioSourceDataPipeItem &item = items_pool_[i];

            item.buffer = reinterpret_cast<data_type *>(&(buffer_pool[block_size * i]));
            item.num_channels = args.num_channels;
            item.num_frames = args.num_frames;
            item.clearInfo();

            producer_queue_.at(index) = &item;

            producer_queue_.unlock_write(index);
        } else {
            LOGE("reset() - producer_queue_.lock_write() failed");
            failed = true;
        }
    }

    if (failed) {
        producer_queue_.clear();
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    return OSLMP_RESULT_SUCCESS;
}

bool AudioSourceDataPipe::Impl::lockProduce(AudioSourceDataPipe::produce_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // obtain free item from the producer_queue_
    audio_source_buffer_queue_t::index_t index = audio_source_buffer_queue_t::INVALID_INDEX;
    audio_source_buffer_queue_t &queue = producer_queue_;

    if (CXXPH_LIKELY(queue.lock_read(index, min_remains))) {
        AudioSourceDataPipeItem *item = queue.at(index);

        queue.at(index) = nullptr;

        block.dest = item->buffer;
        block.num_channels = item->num_channels;
        block.num_frames = item->num_frames;
        block.tag = 0;
        block.position_msec = -1;
        block.lock = reinterpret_cast<uintptr_t>(item);

        queue.unlock_read(index);

        return true;
    } else {
        clear(block);

        return false;
    }
}

bool AudioSourceDataPipe::Impl::unlockProduce(AudioSourceDataPipe::produce_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // push prepared item into the consumer_queue_
    audio_source_buffer_queue_t::index_t index = audio_source_buffer_queue_t::INVALID_INDEX;
    audio_source_buffer_queue_t &queue = consumer_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        AudioSourceDataPipeItem *item = reinterpret_cast<AudioSourceDataPipeItem *>(block.lock);
        item->tag = block.tag;
        item->position_msec = block.position_msec;
        queue.at(index) = item;
        queue.unlock_write(index);

        // clear block
        clear(block);

        result = true;
    } else {
        LOGE("unlockProduce() - consumer_queue_.lock_write() failed");
        result = false;
    }

    return result;
}

bool AudioSourceDataPipe::Impl::lockConsume(AudioSourceDataPipe::consume_block_t &block, size_t min_remains,
                                            uint32_t tag_mask_bitmap) noexcept
{

    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // obtain prepared item from the consumer_queue_
    audio_source_buffer_queue_t::index_t index = audio_source_buffer_queue_t::INVALID_INDEX;
    audio_source_buffer_queue_t &queue = consumer_queue_;

    if (CXXPH_LIKELY(queue.lock_read(index, min_remains))) {
        AudioSourceDataPipeItem *item = queue.at(index);
        bool commit;

        if (tag_mask_bitmap & (1UL << item->tag)) {
            queue.at(index) = nullptr;

            block.src = item->buffer;
            block.num_channels = item->num_channels;
            block.num_frames = item->num_frames;
            block.tag = item->tag;
            block.position_msec = item->position_msec;
            block.lock = reinterpret_cast<uintptr_t>(item);

            commit = true;
        } else {
            commit = false;
        }

        queue.unlock_read(index, commit);

        return commit;
    } else {
        clear(block);

        return false;
    }
}

bool AudioSourceDataPipe::Impl::unlockConsume(AudioSourceDataPipe::consume_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // push used item into the recycler_queue_
    audio_source_buffer_queue_t::index_t index = audio_source_buffer_queue_t::INVALID_INDEX;
    audio_source_buffer_queue_t &queue = recycler_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        AudioSourceDataPipeItem *item = reinterpret_cast<AudioSourceDataPipeItem *>(block.lock);

        last_consumer_tag_ = block.tag;

        // override tag
        item->tag = block.tag;

        queue.at(index) = item;
        queue.unlock_write(index);

        // clear block
        clear(block);

        result = true;
    } else {
        LOGE("unlockConsume() - producer_queue_.lock_write() failed");
        result = false;
    }

    return result;
}

bool AudioSourceDataPipe::Impl::lockRecycle(AudioSourceDataPipe::recycle_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // obtain recycled item from the recycler_queue_
    audio_source_buffer_queue_t::index_t index = audio_source_buffer_queue_t::INVALID_INDEX;
    audio_source_buffer_queue_t &queue = recycler_queue_;

    if (CXXPH_LIKELY(queue.lock_read(index, min_remains))) {
        AudioSourceDataPipeItem *item = queue.at(index);

        queue.at(index) = nullptr;

        block.src = item->buffer;
        block.num_channels = item->num_channels;
        block.num_frames = item->num_frames;
        block.tag = item->tag;
        block.position_msec = item->position_msec;
        block.lock = reinterpret_cast<uintptr_t>(item);

        queue.unlock_read(index);

        return true;
    } else {
        clear(block);

        return false;
    }
}

bool AudioSourceDataPipe::Impl::unlockRecycle(AudioSourceDataPipe::recycle_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // push free item into the producer_queue_
    audio_source_buffer_queue_t::index_t index = audio_source_buffer_queue_t::INVALID_INDEX;
    audio_source_buffer_queue_t &queue = producer_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        AudioSourceDataPipeItem *item = reinterpret_cast<AudioSourceDataPipeItem *>(block.lock);

        item->clearInfo();

        queue.at(index) = item;
        queue.unlock_write(index);

        // clear block
        clear(block);

        result = true;
    } else {
        LOGE("unlockRecycle() - producer_queue_.lock_write() failed");
        result = false;
    }

    return result;
}

uint32_t AudioSourceDataPipe::Impl::getCapacity() const noexcept { return init_args_.num_buffer_items; }

uint32_t AudioSourceDataPipe::Impl::consumerGetLastBlockTag() const noexcept { return last_consumer_tag_; }

} // namespace impl
} // namespace oslmp
