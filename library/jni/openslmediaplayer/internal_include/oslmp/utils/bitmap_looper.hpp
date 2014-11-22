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

#ifndef BITMAP_LOOPER_HPP_
#define BITMAP_LOOPER_HPP_

#include <cxxporthelper/cstdint>
#include <cxxporthelper/compiler.hpp>

namespace oslmp {
namespace utils {

// [Usage]
//
//   ---
//   bitmap_looper looper(0xA);
//   while (looper.loop()) {
//       printf("%d\n", looper.index());
//   }
//   ---
//   > 1
//   > 3
//   ---
class bitmap_looper {
public:
    bitmap_looper(uint32_t bm) : bm_(bm), index_(-1) {}

    ~bitmap_looper() {}

    bool loop() noexcept
    {
        if (CXXPH_LIKELY(bm_ != 0UL)) {
            index_ = ::__builtin_ctz(bm_);
            bm_ ^= static_cast<unsigned int>(1 << index_);
            return true;
        } else {
            index_ = -1;
            return false;
        }
    }

    int index() const noexcept { return index_; }

private:
    uint32_t bm_;
    int index_;
};

} // namespace utils
} // namespace oslmp

#endif // BITMAP_LOOPER_HPP_
