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

#include "oslmp/utils/pthread_utils.hpp"

namespace oslmp {
namespace utils {


// for perior to android 5.0 
static int local_pthread_cond_timedwait_monotonic_np(::pthread_cond_t*, ::pthread_mutex_t *, const ::timespec *)
    __attribute__((weakref("pthread_cond_timedwait_monotonic_np")));

static int local_pthread_cond_timedwait_relative_np(::pthread_cond_t*, ::pthread_mutex_t *, const ::timespec *)
    __attribute__((weakref("pthread_cond_timedwait_relative_np")));

// for android 5.0 or later
static int local_pthread_condattr_setclock(::pthread_condattr_t*, ::clockid_t)
    __attribute__((weakref("pthread_condattr_setclock")));

pt_condition_variable::pt_condition_variable()
{
    ::pthread_condattr_init(&attr_);
    if (local_pthread_condattr_setclock) {
        // Android 5.0+
        local_pthread_condattr_setclock(&attr_, CLOCK_MONOTONIC);
    }
    ::pthread_cond_init(&cv_, &attr_);
}

pt_condition_variable::~pt_condition_variable()
{
    ::pthread_cond_destroy(&cv_);
    ::pthread_condattr_destroy(&attr_);
}

int pt_condition_variable::timedwait_absolute(pt_unique_lock &lock, const timespec &timeout_abs) noexcept
{
    if (local_pthread_condattr_setclock) {
        // Android 5.0+
        return ::pthread_cond_timedwait(&cv_, &(lock.native_handle()), &timeout_abs);
    } else {
        // Android 4.x
        return local_pthread_cond_timedwait_monotonic_np(&cv_, &(lock.native_handle()), &timeout_abs);
    }
}

int pt_condition_variable::timedwait_relative(pt_unique_lock &lock, const timespec &offset) noexcept
{
    if (local_pthread_condattr_setclock) {
        // Android 5.0+
        ::timespec ts({ 0, 0 });
        timespec_utils::get_current_time(ts);
        ts = timespec_utils::add_timespec(ts, offset);
        return ::pthread_cond_timedwait(&cv_, &(lock.native_handle()), &ts);
    } else {
        // Android 4.x
        ::timespec ts({ 0, 0 });
        ts = timespec_utils::add_timespec(ts, offset);
        return local_pthread_cond_timedwait_relative_np(&cv_, &(lock.native_handle()), &ts);
    }
}

} // namespace utils
} // namespace oslmp
