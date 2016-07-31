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

#ifndef PREAMP_HPP_
#define PREAMP_HPP_

#include <cxxporthelper/memory>

//
// forward declarations
//
namespace oslmp {
namespace impl {
class AudioMixer;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

class PreAmp {
public:
    struct initialize_args_t {
        AudioMixer *mixer;

        initialize_args_t() : mixer(nullptr) {}
    };

    PreAmp();
    ~PreAmp();

    bool initialize(const initialize_args_t &args) noexcept;

    int setEnabled(bool enabled) noexcept;
    int getEnabled(bool *enabled) const noexcept;
    int setLevel(float level) noexcept;
    int getLevel(float *level) const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // PREAMP_HPP_
