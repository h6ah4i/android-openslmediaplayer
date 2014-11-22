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

// #define LOG_TAG "AudioCaptureDataPipe"

#include "oslmp/impl/AudioCaptureDataPipe.hpp"

#include <algorithm>

#include <cxxporthelper/memory>
#include <cxxporthelper/aligned_memory.hpp>

#include <lockfree/lockfree_circulation_buffer.hpp>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/utils/timespec_utils.hpp"

#define ROUND_UP_TO_CACHE_LINE_SIZE(x)                                                                                 \
    (((x) + CXXPH_PLATFORM_CACHE_LINE_SIZE - 1) & ~(CXXPH_PLATFORM_CACHE_LINE_SIZE - 1))

namespace oslmp {
namespace impl {

struct AudioCaptureDataPipeItem {
    AudioCaptureDataPipe::data_type *buffer;
    uint32_t num_channels;
    uint32_t num_frames;
    timespec timestamp;

    AudioCaptureDataPipeItem()
        : buffer(nullptr), num_channels(0), num_frames(0), timestamp(utils::timespec_utils::ZERO())
    {
    }
};

typedef lockfree::lockfree_circulation_buffer<
    AudioCaptureDataPipeItem *, (AudioCaptureDataPipe::MAX_BUFFER_ITEM_COUNT + 1)> audio_capture_buffer_queue_t;

class AudioCaptureDataPipe::Impl {
public:
    Impl();
    ~Impl();

    int initialize(const initialize_args_t &args) noexcept;
    int reset() noexcept;
    int allocateBuffer() noexcept;
    int releaseBuffer() noexcept;

    bool lockWrite(AudioCaptureDataPipe::write_block_t &block, size_t min_remains) noexcept;
    bool unlockWrite(AudioCaptureDataPipe::write_block_t &block) noexcept;

    bool lockRead(AudioCaptureDataPipe::read_block_t &block, size_t min_remains) noexcept;
    bool unlockRead(AudioCaptureDataPipe::read_block_t &block) noexcept;

    bool isCapturedDataAvailable() const noexcept;

private:
    int setupQueues(const initialize_args_t &args, size_t cache_aligned_block_size, uint8_t *buffer_pool) noexcept;

private:
    bool initialized_;

    initialize_args_t init_args_;
    size_t cache_aligned_block_size_;

    AudioCaptureDataPipeItem items_pool_[AudioCaptureDataPipe::MAX_BUFFER_ITEM_COUNT];
    cxxporthelper::aligned_memory<uint8_t> buffer_pool_;
    audio_capture_buffer_queue_t producer_queue_;
    audio_capture_buffer_queue_t consumer_queue_;
};

//
// Utilities
//
static inline void clear(AudioCaptureDataPipe::write_block_t &block) noexcept
{
    block.dest = nullptr;
    block.num_channels = 0;
    block.num_frames = 0;
    block.lock = 0;
    block.timestamp = utils::timespec_utils::ZERO();
}

static inline void clear(AudioCaptureDataPipe::read_block_t &block) noexcept
{
    block.src = nullptr;
    block.num_channels = 0;
    block.num_frames = 0;
    block.lock = 0;
    block.timestamp = utils::timespec_utils::ZERO();
}

//
// AudioCaptureDataPipe
//

AudioCaptureDataPipe::AudioCaptureDataPipe() : impl_(new (std::nothrow) Impl()) {}

AudioCaptureDataPipe::~AudioCaptureDataPipe() {}

int AudioCaptureDataPipe::initialize(const AudioCaptureDataPipe::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioCaptureDataPipe::reset() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->reset();
}

int AudioCaptureDataPipe::allocateBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->allocateBuffer();
}

int AudioCaptureDataPipe::releaseBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->releaseBuffer();
}

bool AudioCaptureDataPipe::lockWrite(AudioCaptureDataPipe::write_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockWrite(block, min_remains);
}

bool AudioCaptureDataPipe::unlockWrite(AudioCaptureDataPipe::write_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockWrite(block);
}

bool AudioCaptureDataPipe::lockRead(AudioCaptureDataPipe::read_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockRead(block, min_remains);
}

bool AudioCaptureDataPipe::unlockRead(AudioCaptureDataPipe::read_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockRead(block);
}

bool AudioCaptureDataPipe::isCapturedDataAvailable() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isCapturedDataAvailable();
}

//
// AudioCaptureDataPipe::Impl
//

AudioCaptureDataPipe::Impl::Impl() : initialized_(false), cache_aligned_block_size_(0), buffer_pool_() {}

AudioCaptureDataPipe::Impl::~Impl() {}

int AudioCaptureDataPipe::Impl::initialize(const initialize_args_t &args) noexcept
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

    // allocate memory pool
    if (!args.deferred_buffer_alloc) {
        buffer_pool.allocate(cache_aligned_block_size * num_blocks, CXXPH_PLATFORM_CACHE_LINE_SIZE);

        if (!buffer_pool)
            return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;

        // setup queue
        int result = setupQueues(args, cache_aligned_block_size, &buffer_pool[0]);

        if (result != OSLMP_RESULT_SUCCESS)
            return result;
    }

    // update fields
    buffer_pool_ = std::move(buffer_pool);
    init_args_ = args;
    cache_aligned_block_size_ = cache_aligned_block_size;
    initialized_ = true;

    return OSLMP_RESULT_SUCCESS;
}

int AudioCaptureDataPipe::Impl::reset() noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return OSLMP_RESULT_ILLEGAL_STATE;

    int result;
    if (buffer_pool_) {
        result = setupQueues(init_args_, cache_aligned_block_size_, &buffer_pool_[0]);
    } else {
        result = OSLMP_RESULT_SUCCESS;
    }

    return result;
}

int AudioCaptureDataPipe::Impl::allocateBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (buffer_pool_)
        return OSLMP_RESULT_SUCCESS;

    const size_t cache_aligned_block_size = cache_aligned_block_size_;
    const size_t num_blocks = init_args_.num_buffer_items;

    // allocate memory pool
    cxxporthelper::aligned_memory<uint8_t> buffer_pool;

    buffer_pool.allocate(cache_aligned_block_size * num_blocks, CXXPH_PLATFORM_CACHE_LINE_SIZE);

    if (CXXPH_UNLIKELY(!buffer_pool))
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

int AudioCaptureDataPipe::Impl::releaseBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (!buffer_pool_)
        return OSLMP_RESULT_SUCCESS;

    buffer_pool_.free();

    return OSLMP_RESULT_SUCCESS;
}

int AudioCaptureDataPipe::Impl::setupQueues(const AudioCaptureDataPipe::initialize_args_t &args, size_t block_size,
                                            uint8_t *buffer_pool) noexcept
{
    const size_t num_blocks = args.num_buffer_items;

    producer_queue_.clear();
    consumer_queue_.clear();

    bool failed = false;
    for (size_t i = 0; i < num_blocks; ++i) {
        audio_capture_buffer_queue_t::index_t index = audio_capture_buffer_queue_t::INVALID_INDEX;

        if (CXXPH_LIKELY(producer_queue_.lock_write(index))) {
            AudioCaptureDataPipeItem &item = items_pool_[i];

            item.buffer = reinterpret_cast<data_type *>(&(buffer_pool[block_size * i]));
            item.num_channels = args.num_channels;
            item.num_frames = args.num_frames;

            producer_queue_.at(index) = &item;

            producer_queue_.unlock_write(index);
        } else {
            LOGE("initialize() - producer_queue_.lock_write() failed");
            failed = true;
        }
    }

    if (CXXPH_UNLIKELY(failed)) {
        producer_queue_.clear();
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    return OSLMP_RESULT_SUCCESS;
}

bool AudioCaptureDataPipe::Impl::lockWrite(AudioCaptureDataPipe::write_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // obtain free item from the producer_queue_
    audio_capture_buffer_queue_t::index_t index = audio_capture_buffer_queue_t::INVALID_INDEX;
    audio_capture_buffer_queue_t &queue = producer_queue_;

    if (CXXPH_LIKELY(queue.lock_read(index, min_remains))) {
        AudioCaptureDataPipeItem *item = queue.at(index);

        queue.at(index) = nullptr;

        block.dest = item->buffer;
        block.num_channels = item->num_channels;
        block.num_frames = item->num_frames;
        item->timestamp = utils::timespec_utils::ZERO();
        block.lock = reinterpret_cast<uintptr_t>(item);

        queue.unlock_read(index);

        return true;
    } else {
        clear(block);

        return false;
    }
}

bool AudioCaptureDataPipe::Impl::unlockWrite(AudioCaptureDataPipe::write_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // push prepared item into the consumer_queue_
    audio_capture_buffer_queue_t::index_t index = audio_capture_buffer_queue_t::INVALID_INDEX;
    audio_capture_buffer_queue_t &queue = consumer_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        AudioCaptureDataPipeItem *item = reinterpret_cast<AudioCaptureDataPipeItem *>(block.lock);

        // update item
        item->timestamp = block.timestamp;

        queue.at(index) = item;
        queue.unlock_write(index);

        // clear block
        clear(block);

        result = true;
    } else {
        LOGE("unlockWrite() - consumer_queue_.lock_write() failed");
        result = false;
    }

    return result;
}

bool AudioCaptureDataPipe::Impl::lockRead(AudioCaptureDataPipe::read_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // obtain prepared item from the consumer_queue_
    audio_capture_buffer_queue_t::index_t index = audio_capture_buffer_queue_t::INVALID_INDEX;
    audio_capture_buffer_queue_t &queue = consumer_queue_;

    if (CXXPH_LIKELY(queue.lock_read(index, min_remains))) {
        AudioCaptureDataPipeItem *item = queue.at(index);

        queue.at(index) = nullptr;

        block.src = item->buffer;
        block.num_channels = item->num_channels;
        block.num_frames = item->num_frames;
        block.timestamp = item->timestamp;
        block.lock = reinterpret_cast<uintptr_t>(item);

        queue.unlock_read(index);

        return true;
    } else {
        clear(block);

        return false;
    }
}

bool AudioCaptureDataPipe::Impl::unlockRead(AudioCaptureDataPipe::read_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // push free item into the producer_queue_
    audio_capture_buffer_queue_t::index_t index = audio_capture_buffer_queue_t::INVALID_INDEX;
    audio_capture_buffer_queue_t &queue = producer_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        AudioCaptureDataPipeItem *item = reinterpret_cast<AudioCaptureDataPipeItem *>(block.lock);
        queue.at(index) = item;
        queue.unlock_write(index);

        // clear block
        clear(block);

        result = true;
    } else {
        LOGE("unlockRead() - producer_queue_.lock_write() failed");
        result = false;
    }

    return result;
}

bool AudioCaptureDataPipe::Impl::isCapturedDataAvailable() const noexcept { return !(consumer_queue_.empty()); }

} // namespace impl
} // namespace oslmp
