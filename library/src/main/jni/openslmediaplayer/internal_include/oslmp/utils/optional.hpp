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

#ifndef OPTIONAL_HPP_
#define OPTIONAL_HPP_

namespace oslmp {
namespace utils {

template <typename T>
class optional {
public:
    optional() : available_(false), value_() {}

    optional(T value) : available_(true), value_(value) {}

    const T &get() const noexcept { return value_; }

    optional<T> &set(T value) noexcept
    {
        available_ = true;
        value_ = value;
        return (*this);
    }

    void clear() noexcept
    {
        available_ = false;
        value_ = T();
    }

    bool available() const noexcept { return available_; }

private:
    bool available_;
    T value_;
};

} // namespace utils
} // namespace oslmp

#endif // OPTIONAL_HPP_
