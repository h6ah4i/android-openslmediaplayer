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

#ifndef AUDIOPIPEBUFFERQUEUEBINDER_HPP_
#define AUDIOPIPEBUFFERQUEUEBINDER_HPP_

#include "oslmp/impl/AudioSinkDataPipe.hpp"
#include "oslmp/impl/AudioSinkDataPipeReadBlockQueue.hpp"

#ifdef USE_OSLMP_DEBUG_FEATURES
#include "oslmp/impl/NonBlockingTraceLogger.hpp"
#endif

namespace opensles {
class CSLAndroidSimpleBufferQueueItf;
}

namespace oslmp {
namespace impl {

class OpenSLMediaPlayerInternalContext;

class AudioPipeBufferQueueBinder {
public:
    AudioPipeBufferQueueBinder();
    ~AudioPipeBufferQueueBinder();

    void release() noexcept;
    int beforeStart(opensles::CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept;
    int afterStop(opensles::CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept;
    int initialize(OpenSLMediaPlayerInternalContext *context, AudioSinkDataPipe *pipe,
                   opensles::CSLAndroidSimpleBufferQueueItf *buffer_queue, int num_blocks) noexcept;
    void handleCallback(AudioSinkDataPipe *pipe, opensles::CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept;

private:
    static bool getSilentReadBlock(AudioSinkDataPipe *pipe, AudioSinkDataPipe::read_block_t &rb) noexcept;

private:
    OpenSLMediaPlayerInternalContext *context_;

    int num_blocks_;
    AudioSinkDataPipe::read_block_t silent_block_;

    size_t wait_buffering_;
    AudioSinkDataPipeReadBlockQueue wait_completion_blocks_;
    AudioSinkDataPipeReadBlockQueue pooled_blocks_;

#ifdef USE_OSLMP_DEBUG_FEATURES
    std::unique_ptr<NonBlockingTraceLoggerClient> nb_logger_;
    int callback_trace_toggle_;
#endif
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOPIPEBUFFERQUEUEBINDER_HPP_
