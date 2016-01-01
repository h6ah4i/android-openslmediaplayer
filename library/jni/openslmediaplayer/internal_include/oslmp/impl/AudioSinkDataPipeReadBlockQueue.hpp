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

#ifndef AUDIOSINKDATAPIPEREADBLOCKQUEUE_HPP_
#define AUDIOSINKDATAPIPEREADBLOCKQUEUE_HPP_

#include "oslmp/impl/AudioSinkDataPipe.hpp"

namespace opensles {
class CSLAndroidSimpleBufferQueueItf;
}

namespace oslmp {
namespace impl {

class AudioSinkDataPipeReadBlockQueue {
public:
    AudioSinkDataPipeReadBlockQueue();
    ~AudioSinkDataPipeReadBlockQueue();

    int initialize(int capacity) noexcept;
    int capacity() const noexcept;
    int count() const noexcept;
    void clear() noexcept;
    bool enqueue(const AudioSinkDataPipe::read_block_t &src) noexcept;
    bool dequeue(AudioSinkDataPipe::read_block_t &dest) noexcept;

    enum {
        MAX_BLOCK_COUNT = 32
    };

private:
    AudioSinkDataPipe::read_block_t rb_queue_[MAX_BLOCK_COUNT];
    int capacity_;
    int count_;
    int rp_;
    int wp_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOSINKDATAPIPEREADBLOCKQUEUE_HPP_
