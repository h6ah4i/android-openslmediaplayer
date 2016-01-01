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

// #define LOG_TAG "APBQueueBinder"
// #define NB_LOG_TAG "NB APBQueueBinder"

#include "oslmp/impl/AudioPipeBufferQueueBinder.hpp"

#include <cstring>
#include <SLESCXX/OpenSLES_CXX.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"

#define TRANSLATE_RESULT(result) InternalUtils::sTranslateOpenSLErrorCode(result)

#define IS_SL_RESULT_SUCCESS(slresult) (CXXPH_LIKELY((slresult) == SL_RESULT_SUCCESS))

namespace oslmp {
namespace impl {

using namespace ::opensles;

typedef OpenSLMediaPlayerInternalUtils InternalUtils;

static inline size_t getSize(const AudioSinkDataPipe::read_block_t &rb) noexcept
{
    return getBytesPerSample(rb.sample_format) * rb.num_channels * rb.num_frames;
}

static inline size_t getSize(const AudioSinkDataPipe::write_block_t &wb) noexcept
{
    return getBytesPerSample(wb.sample_format) * wb.num_channels * wb.num_frames;
}

//
//
// AudioPipeBufferQueueBinder
//

AudioPipeBufferQueueBinder::AudioPipeBufferQueueBinder() : context_(nullptr), num_blocks_(0), wait_buffering_(0) {}

AudioPipeBufferQueueBinder::~AudioPipeBufferQueueBinder() {}


bool AudioPipeBufferQueueBinder::getSilentReadBlock(AudioSinkDataPipe *pipe,
                                                    AudioSinkDataPipe::read_block_t &rb) noexcept
{
    {
        AudioSinkDataPipe::write_block_t wb;

        if (CXXPH_UNLIKELY(!pipe->lockWrite(wb))) {
            return false;
        }
        (void)::memset(wb.dest, 0, getSize(wb));
        pipe->unlockWrite(wb);
    }

    return pipe->lockRead(rb);
}

void AudioPipeBufferQueueBinder::release() noexcept
{
    wait_buffering_ = 0;
    context_ = nullptr;
}

int AudioPipeBufferQueueBinder::beforeStart(CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept
{
    SLresult slResult;

    buffer_queue->Clear();
    wait_completion_blocks_.clear();

    // fill silent block
    for (int i = 0; i < num_blocks_; ++i) {
        wait_completion_blocks_.enqueue(silent_block_);

        slResult = buffer_queue->Enqueue(silent_block_.src, getSize(silent_block_));

        if (!IS_SL_RESULT_SUCCESS(slResult)) {
            return TRANSLATE_RESULT(slResult);
        }
    }

    wait_buffering_ = (pooled_blocks_.count() == 0) ? 1 : 0;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPipeBufferQueueBinder::afterStop(CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept
{
    buffer_queue->Clear();

    pooled_blocks_.clear();

    {
        AudioSinkDataPipe::read_block_t rb;
        while (wait_completion_blocks_.dequeue(rb)) {
            if (rb.src != silent_block_.src) {
                pooled_blocks_.enqueue(rb);
            }
        }
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioPipeBufferQueueBinder::initialize(OpenSLMediaPlayerInternalContext *context, AudioSinkDataPipe *pipe,
                                           CSLAndroidSimpleBufferQueueItf *buffer_queue, int num_blocks) noexcept
{
    if (CXXPH_UNLIKELY(!getSilentReadBlock(pipe, silent_block_))) {
        return OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
    }

    if (CXXPH_UNLIKELY(!(num_blocks >= 2))) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    wait_completion_blocks_.initialize(num_blocks);
    pooled_blocks_.initialize(num_blocks);
    num_blocks_ = num_blocks;
    context_ = context;

#if USE_OSLMP_DEBUG_FEATURES
    nb_logger_.reset(context_->getNonBlockingTraceLogger().create_new_client());
#endif

    return OSLMP_RESULT_SUCCESS;
}

void AudioPipeBufferQueueBinder::handleCallback(AudioSinkDataPipe *pipe,
                                                CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept
{
    REF_NB_LOGGER_CLIENT(nb_logger_);

    // return used block
    {
        AudioSinkDataPipe::read_block_t rb;

        if (CXXPH_LIKELY(wait_completion_blocks_.dequeue(rb))) {
            if (rb.src != silent_block_.src) {
                pipe->unlockRead(rb);
            }
        } else {
            LOGE("AudioPipeBufferQueueBinder::handleCallback  wait_completion_blocks_.dequeue() returns an error");
        }
    }

    // obtain new block

    AudioSinkDataPipe::read_block_t rb;

    const int num_pooled_blocks = pooled_blocks_.count();
    if (CXXPH_UNLIKELY(num_pooled_blocks > 0)) {
        pooled_blocks_.dequeue(rb);
        wait_buffering_ = 0;
        NB_LOGV("use pooled audio block");
    } else {
        if (CXXPH_LIKELY(pipe->lockRead(rb, wait_buffering_))) {
            wait_buffering_ = 0;
            NB_LOGV("valid audio block (index = %d)", rb.dbg_lock_index);
        } else {
            wait_buffering_ = 1;
            rb = silent_block_;
            NB_LOGI("silent block is used");
        }
    }

    SLresult slResult = buffer_queue->Enqueue(rb.src, getSize(rb));

    if (IS_SL_RESULT_SUCCESS(slResult)) {
        if (CXXPH_LIKELY(wait_completion_blocks_.enqueue(rb))) {
        } else {
            LOGE("AudioPipeBufferQueueBinder::handleCallback  wait_completion_blocks_.enqueue() returns an error");
        }
    } else {
        LOGW("AudioPipeBufferQueueBinder::handleCallback  buffer_queue->Enqueue() returns an error");
        NB_LOGW("AudioPipeBufferQueueBinder::handleCallback  buffer_queue->Enqueue() returns an error");
    }
}

} // namespace impl
} // namespace oslmp
