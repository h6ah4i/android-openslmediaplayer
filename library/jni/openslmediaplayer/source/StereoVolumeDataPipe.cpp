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

// #define LOG_TAG "StereoVolumeDataPipe"

#include "oslmp/impl/StereoVolumeDataPipe.hpp"

#include <algorithm>

#include <cxxporthelper/memory>

#include <lockfree/lockfree_circulation_buffer.hpp>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

namespace oslmp {
namespace impl {

struct StereoVolumeDataPipeItem {
    float volume_left;
    float volume_right;

    StereoVolumeDataPipeItem() : volume_left(0.0f), volume_right(0.0f) {}
};

typedef lockfree::lockfree_circulation_buffer<StereoVolumeDataPipeItem *, (StereoVolumeDataPipe::PIPE_ITEM_COUNT + 1)>
stereo_volume_item_t;

class StereoVolumeDataPipe::Impl {
public:
    Impl();
    ~Impl();

    int initialize() noexcept;
    int reset() noexcept;

    bool lockWrite(StereoVolumeDataPipe::write_block_t &block) noexcept;
    bool unlockWrite(StereoVolumeDataPipe::write_block_t &block) noexcept;

    bool lockRead(StereoVolumeDataPipe::read_block_t &block) noexcept;
    bool unlockRead(StereoVolumeDataPipe::read_block_t &block) noexcept;

    bool getLastReadPositionMsec(int32_t &dest) noexcept;

private:
    int setupQueues() noexcept;

private:
    bool initialized_;

    StereoVolumeDataPipeItem items_pool_[StereoVolumeDataPipe::PIPE_ITEM_COUNT];
    stereo_volume_item_t producer_queue_;
    stereo_volume_item_t consumer_queue_;
};

//
// Utilities
//
static inline void clear(StereoVolumeDataPipe::write_block_t &block) noexcept
{
    block.volume_left = 0.0f;
    block.volume_right = 0.0f;
    block.lock = 0;
}

static inline void clear(StereoVolumeDataPipe::read_block_t &block) noexcept
{
    block.volume_left = 0.0f;
    block.volume_right = 0.0f;
    block.lock = 0;
}

//
// StereoVolumeDataPipe
//

StereoVolumeDataPipe::StereoVolumeDataPipe() : impl_(new (std::nothrow) Impl()) {}

StereoVolumeDataPipe::~StereoVolumeDataPipe() {}

int StereoVolumeDataPipe::initialize() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize();
}

int StereoVolumeDataPipe::reset() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->reset();
}

bool StereoVolumeDataPipe::lockWrite(StereoVolumeDataPipe::write_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockWrite(block);
}

bool StereoVolumeDataPipe::unlockWrite(StereoVolumeDataPipe::write_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockWrite(block);
}

bool StereoVolumeDataPipe::lockRead(StereoVolumeDataPipe::read_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->lockRead(block);
}

bool StereoVolumeDataPipe::unlockRead(StereoVolumeDataPipe::read_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->unlockRead(block);
}

//
// StereoVolumeDataPipe::Impl
//

StereoVolumeDataPipe::Impl::Impl() : initialized_(false) {}

StereoVolumeDataPipe::Impl::~Impl() {}

int StereoVolumeDataPipe::Impl::initialize() noexcept
{
    // check state
    if (CXXPH_UNLIKELY(initialized_))
        return OSLMP_RESULT_ILLEGAL_STATE;

    // setup queue
    int result = setupQueues();

    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    initialized_ = true;

    return OSLMP_RESULT_SUCCESS;
}

int StereoVolumeDataPipe::Impl::reset() noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return OSLMP_RESULT_ILLEGAL_STATE;

    return setupQueues();
}

int StereoVolumeDataPipe::Impl::setupQueues() noexcept
{
    producer_queue_.clear();
    consumer_queue_.clear();

    bool failed = false;
    for (size_t i = 0; i < PIPE_ITEM_COUNT; ++i) {
        stereo_volume_item_t::index_t index = stereo_volume_item_t::INVALID_INDEX;

        if (producer_queue_.lock_write(index)) {
            StereoVolumeDataPipeItem &item = items_pool_[i];
            producer_queue_.at(index) = &item;
            producer_queue_.unlock_write(index);
        } else {
            LOGE("initialize() - producer_queue_.lock_write() failed");
            failed = true;
            break;
        }
    }

    if (failed) {
        producer_queue_.clear();
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    return OSLMP_RESULT_SUCCESS;
}

bool StereoVolumeDataPipe::Impl::lockWrite(StereoVolumeDataPipe::write_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return false;

    // obtain free item from the producer_queue_
    stereo_volume_item_t::index_t index = stereo_volume_item_t::INVALID_INDEX;
    stereo_volume_item_t &queue = producer_queue_;

    if (queue.lock_read(index)) {
        StereoVolumeDataPipeItem *item = queue.at(index);

        queue.at(index) = nullptr;

        block.volume_left = item->volume_left;
        block.volume_right = item->volume_right;
        block.lock = reinterpret_cast<uintptr_t>(item);

        queue.unlock_read(index);

        return true;
    } else {
        clear(block);

        return false;
    }
}

bool StereoVolumeDataPipe::Impl::unlockWrite(StereoVolumeDataPipe::write_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return false;

    // push prepared item into the consumer_queue_
    stereo_volume_item_t::index_t index = stereo_volume_item_t::INVALID_INDEX;
    stereo_volume_item_t &queue = consumer_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        StereoVolumeDataPipeItem *item = reinterpret_cast<StereoVolumeDataPipeItem *>(block.lock);

        item->volume_left = block.volume_left;
        item->volume_right = block.volume_right;

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

bool StereoVolumeDataPipe::Impl::lockRead(StereoVolumeDataPipe::read_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return false;

    // obtain prepared item from the consumer_queue_
    stereo_volume_item_t::index_t index = stereo_volume_item_t::INVALID_INDEX;
    stereo_volume_item_t &queue = consumer_queue_;

    if (CXXPH_LIKELY(queue.lock_read(index))) {
        StereoVolumeDataPipeItem *item = queue.at(index);

        queue.at(index) = nullptr;

        block.volume_left = item->volume_left;
        block.volume_right = item->volume_right;
        block.lock = reinterpret_cast<uintptr_t>(item);

        queue.unlock_read(index);

        return true;
    } else {
        clear(block);

        return false;
    }
}

bool StereoVolumeDataPipe::Impl::unlockRead(StereoVolumeDataPipe::read_block_t &block) noexcept
{
    if (CXXPH_UNLIKELY(!initialized_))
        return false;

    // push free item into the producer_queue_
    stereo_volume_item_t::index_t index = stereo_volume_item_t::INVALID_INDEX;
    stereo_volume_item_t &queue = producer_queue_;
    bool result;

    if (CXXPH_LIKELY(queue.lock_write(index))) {
        StereoVolumeDataPipeItem *item = reinterpret_cast<StereoVolumeDataPipeItem *>(block.lock);
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

} // namespace impl
} // namespace oslmp
