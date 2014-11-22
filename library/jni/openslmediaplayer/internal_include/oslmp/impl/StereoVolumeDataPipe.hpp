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

#ifndef STEREOVOLUMEDATAPIPE_HPP_
#define STEREOVOLUMEDATAPIPE_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

namespace oslmp {
namespace impl {

class StereoVolumeDataPipe {
public:
    enum { PIPE_ITEM_COUNT = 3 };

    struct write_block_t {
        float volume_left;
        float volume_right;
        uintptr_t lock;
    };

    struct read_block_t {
        float volume_left;
        float volume_right;
        uintptr_t lock;
    };

    StereoVolumeDataPipe();
    ~StereoVolumeDataPipe();

    int initialize() noexcept;
    int reset() noexcept;

    bool lockWrite(write_block_t &block) noexcept;
    bool unlockWrite(write_block_t &block) noexcept;

    bool lockRead(read_block_t &block) noexcept;
    bool unlockRead(read_block_t &block) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // STEREOVOLUMEDATAPIPE_HPP_
