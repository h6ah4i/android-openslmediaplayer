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

// #define LOG_TAG "AudioSinkDataPipe"

#include "oslmp/impl/AudioSinkDataPipe.hpp"

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

struct AudioSinkDataPipeItem {
    sample_format_type sample_format;
    void *buffer;
    uint32_t num_channels;
    uint32_t num_frames;

    AudioSinkDataPipeItem()
        : sample_format(kAudioSampleFormatType_Unknown), buffer(nullptr), num_channels(0), num_frames(0)
    {
    }
};

typedef lockfree::lockfree_circulation_buffer<AudioSinkDataPipeItem *, (AudioSinkDataPipe::MAX_BUFFER_ITEM_COUNT + 1)>
audio_sink_buffer_queue_t;

class AudioSinkDataPipe::Impl {
public:
    Impl();
    ~Impl();

    int initialize(const initialize_args_t &args) noexcept;
    int reset() noexcept;
    int allocateBuffer() noexcept;
    int releaseBuffer() noexcept;

    bool lockWrite(AudioSinkDataPipe::write_block_t &block, size_t min_remains) noexcept;
    bool unlockWrite(AudioSinkDataPipe::write_block_t &block) noexcept;

    bool lockRead(AudioSinkDataPipe::read_block_t &block, size_t min_remains) noexcept;
    bool unlockRead(AudioSinkDataPipe::read_block_t &block) noexcept;

    uint32_t getNumberOfBufferItems() const noexcept;

private:
    int setupQueues(const initialize_args_t &args, size_t block_size, uint8_t *buffer_pool) noexcept;

private:
    bool initialized_;

    initialize_args_t init_args_;
    size_t cache_aligned_block_size_;

    AudioSinkDataPipeItem items_pool_[AudioSinkDataPipe::MAX_BUFFER_ITEM_COUNT];
    cxxporthelper::aligned_memory<uint8_t> buffer_pool_;
    audio_sink_buffer_queue_t producer_queue_;
    audio_sink_buffer_queue_t consumer_queue_;
};

//
// Utilities
//
static inline void clear(AudioSinkDataPipe::write_block_t &block) noexcept
{
    block.sample_format = kAudioSampleFormatType_Unknown;
    block.dest = nullptr;
    block.num_channels = 0;
    block.num_frames = 0;
    block.lock = 0;
#if USE_OSLMP_DEBUG_FEATURES
    block.dbg_lock_index = audio_sink_buffer_queue_t::INVALID_INDEX;
#endif
}

static inline void clear(AudioSinkDataPipe::read_block_t &block) noexcept
{
    block.sample_format = kAudioSampleFormatType_Unknown;
    block.src = nullptr;
    block.num_channels = 0;
    block.num_frames = 0;
    block.lock = 0;
#if USE_OSLMP_DEBUG_FEATURES
    block.dbg_lock_index = audio_sink_buffer_queue_t::INVALID_INDEX;
#endif
}

//
// AudioSinkDataPipe
//

AudioSinkDataPipe::AudioSinkDataPipe() : impl_(new (std::nothrow) Impl()) {}

AudioSinkDataPipe::~AudioSinkDataPipe() {}

int AudioSinkDataPipe::initialize(const AudioSinkDataPipe::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioSinkDataPipe::reset() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->reset();
}

int AudioSinkDataPipe::allocateBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->allocateBuffer();
}

int AudioSinkDataPipe::releaseBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->releaseBuffer();
}

bool AudioSinkDataPipe::lockWrite(AudioSinkDataPipe::write_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockWrite(block, min_remains);
}

bool AudioSinkDataPipe::unlockWrite(AudioSinkDataPipe::write_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockWrite(block);
}

bool AudioSinkDataPipe::lockRead(AudioSinkDataPipe::read_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockRead(block, min_remains);
}

bool AudioSinkDataPipe::unlockRead(AudioSinkDataPipe::read_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockRead(block);
}

uint32_t AudioSinkDataPipe::getNumberOfBufferItems() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0;
    return impl_->getNumberOfBufferItems();
}

//
// AudioSinkDataPipe::Impl
//

AudioSinkDataPipe::Impl::Impl() : initialized_(false), init_args_(), cache_aligned_block_size_(0), buffer_pool_() {}

AudioSinkDataPipe::Impl::~Impl() {}

int AudioSinkDataPipe::Impl::initialize(const initialize_args_t &args) noexcept
{
    // check arguments
    if (!(args.num_buffer_items >= 2 && args.num_buffer_items <= MAX_BUFFER_ITEM_COUNT))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (args.num_channels == 0)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (args.num_frames == 0)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!(args.sample_format == kAudioSampleFormatType_S16 || args.sample_format == kAudioSampleFormatType_F32)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    // check state
    if (initialized_)
        return OSLMP_RESULT_ILLEGAL_STATE;

    const size_t block_size = getBytesPerSample(args.sample_format) * args.num_channels * args.num_frames;
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

int AudioSinkDataPipe::Impl::reset() noexcept
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

int AudioSinkDataPipe::Impl::allocateBuffer() noexcept
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

int AudioSinkDataPipe::Impl::releaseBuffer() noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (!buffer_pool_)
        return OSLMP_RESULT_SUCCESS;

    buffer_pool_.free();

    return OSLMP_RESULT_SUCCESS;
}

int AudioSinkDataPipe::Impl::setupQueues(const AudioSinkDataPipe::initialize_args_t &args, size_t block_size,
                                         uint8_t *buffer_pool) noexcept
{
    const size_t num_blocks = args.num_buffer_items;

    producer_queue_.clear();
    consumer_queue_.clear();

    bool failed = false;
    for (size_t i = 0; i < num_blocks; ++i) {
        audio_sink_buffer_queue_t::index_t index = audio_sink_buffer_queue_t::INVALID_INDEX;

        if (producer_queue_.lock_write(index)) {
            AudioSinkDataPipeItem &item = items_pool_[i];

            item.sample_format = args.sample_format;
            item.buffer = static_cast<void *>(&(buffer_pool[block_size * i]));
            item.num_channels = args.num_channels;
            item.num_frames = args.num_frames;

            producer_queue_.at(index) = &item;

            producer_queue_.unlock_write(index);
        } else {
            LOGE("initialize() - producer_queue_.lock_write() failed");
            failed = true;
        }
    }

    if (failed) {
        producer_queue_.clear();
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    return OSLMP_RESULT_SUCCESS;
}

bool AudioSinkDataPipe::Impl::lockWrite(AudioSinkDataPipe::write_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // obtain free item from the producer_queue_
    audio_sink_buffer_queue_t::index_t index = audio_sink_buffer_queue_t::INVALID_INDEX;
    audio_sink_buffer_queue_t &queue = producer_queue_;

    if (CXXPH_LIKELY(queue.lock_read(index, min_remains))) {
        AudioSinkDataPipeItem *item = queue.at(index);

        queue.at(index) = nullptr;

        block.sample_format = item->sample_format;
        block.dest = item->buffer;
        block.num_channels = item->num_channels;
        block.num_frames = item->num_frames;
        block.lock = reinterpret_cast<uintptr_t>(item);

#if USE_OSLMP_DEBUG_FEATURES
        block.dbg_lock_index = index;
#endif

        queue.unlock_read(index);

        return true;
    } else {
        clear(block);

        return false;
    }
}

bool AudioSinkDataPipe::Impl::unlockWrite(AudioSinkDataPipe::write_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // push prepared item into the consumer_queue_
    audio_sink_buffer_queue_t::index_t index = audio_sink_buffer_queue_t::INVALID_INDEX;
    audio_sink_buffer_queue_t &queue = consumer_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        AudioSinkDataPipeItem *item = reinterpret_cast<AudioSinkDataPipeItem *>(block.lock);
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

bool AudioSinkDataPipe::Impl::lockRead(AudioSinkDataPipe::read_block_t &block, size_t min_remains) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // obtain prepared item from the consumer_queue_
    audio_sink_buffer_queue_t::index_t index = audio_sink_buffer_queue_t::INVALID_INDEX;
    audio_sink_buffer_queue_t &queue = consumer_queue_;

    if (CXXPH_LIKELY(queue.lock_read(index, min_remains))) {
        AudioSinkDataPipeItem *item = queue.at(index);

        queue.at(index) = nullptr;

        block.sample_format = item->sample_format;
        block.src = item->buffer;
        block.num_channels = item->num_channels;
        block.num_frames = item->num_frames;
        block.lock = reinterpret_cast<uintptr_t>(item);

#if USE_OSLMP_DEBUG_FEATURES
        block.dbg_lock_index = index;
#endif

        queue.unlock_read(index);

        return true;
    } else {
        clear(block);

        return false;
    }
}

bool AudioSinkDataPipe::Impl::unlockRead(AudioSinkDataPipe::read_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!(initialized_ && buffer_pool_)))
        return false;

    // push free item into the producer_queue_
    audio_sink_buffer_queue_t::index_t index = audio_sink_buffer_queue_t::INVALID_INDEX;
    audio_sink_buffer_queue_t &queue = producer_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        AudioSinkDataPipeItem *item = reinterpret_cast<AudioSinkDataPipeItem *>(block.lock);
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

uint32_t AudioSinkDataPipe::Impl::getNumberOfBufferItems() const noexcept { return init_args_.num_buffer_items; }

} // namespace impl
} // namespace oslmp
