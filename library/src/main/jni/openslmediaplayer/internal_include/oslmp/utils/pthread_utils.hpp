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

#ifndef PTHREAD_UTILS_HPP_
#define PTHREAD_UTILS_HPP_

#include <cerrno>
#include <pthread.h>
#include <android/api-level.h>

#include <cxxporthelper/compiler.hpp>

#include "oslmp/utils/timespec_utils.hpp"

namespace oslmp {
namespace utils {

class pt_mutex;
class pt_lock_guard;
class pt_unique_lock;
class pt_condition_variable;

class pt_mutex {
public:
    pt_mutex()
    {
        int pr;
        do {
            pr = ::pthread_mutex_init(&m_, nullptr);
        } while (pr == EAGAIN);
    }

    ~pt_mutex() { ::pthread_mutex_destroy(&m_); }

    pthread_mutex_t &native_handle() noexcept { return m_; }

private:
    pthread_mutex_t m_;
};

class pt_lock_guard {
public:
    pt_lock_guard(pt_mutex &m) : m_(m), locked_(false)
    {
        int pr;
        do {
            pr = ::pthread_mutex_lock(&(m_.native_handle()));
        } while (CXXPH_UNLIKELY(pr == EAGAIN));

        locked_ = (pr == 0);
    }

    ~pt_lock_guard()
    {
        if (CXXPH_LIKELY(locked_)) {
            ::pthread_mutex_unlock(&(m_.native_handle()));
            locked_ = false;
        }
    }

private:
    pt_mutex &m_;
    bool locked_;
};

class pt_unique_lock {
    friend class pt_condition_variable;

public:
    pt_unique_lock(pt_mutex &m, bool defer_lock = false) : m_(m), locked_(false)
    {
        if (!defer_lock) {
            lock();
        }
    }

    ~pt_unique_lock() { unlock(); }

    int lock() noexcept
    {
        if (locked_) {
            return 0;
        }

        int pr;
        do {
            pr = ::pthread_mutex_lock(&(m_.native_handle()));
        } while (CXXPH_UNLIKELY(pr == EAGAIN));

        locked_ = (pr == 0);

        return pr;
    }

    int try_lock() noexcept
    {
        if (locked_) {
            return 0;
        }

        int pr;
        do {
            pr = ::pthread_mutex_trylock(&(m_.native_handle()));
        } while (CXXPH_UNLIKELY(pr == EAGAIN));

        locked_ = (pr == 0);

        return pr;
    }

    void unlock() noexcept
    {
        if (CXXPH_LIKELY(locked_)) {
            ::pthread_mutex_unlock(&(m_.native_handle()));
            locked_ = false;
        }
    }

    bool owns_lock() const noexcept { return locked_; }

private:
    pt_mutex &m_;
    bool locked_;

    pthread_mutex_t &native_handle() { return m_.native_handle(); }
};

class pt_condition_variable {
public:
    pt_condition_variable()
    {
#if __ANDROID_API__ <= 19
        ::pthread_cond_init(&cv_, nullptr);
#else
        ::pthread_condattr_t attr;
        ::pthread_condattr_init(&attr);
        ::pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
        ::pthread_cond_init(&cv_, &attr);
#endif
    }

    ~pt_condition_variable() { ::pthread_cond_destroy(&cv_); }

    void notify_one() noexcept { (void)::pthread_cond_signal(&cv_); }

    void notify_all() noexcept { (void)::pthread_cond_broadcast(&cv_); }

    int wait(pt_unique_lock &lock) noexcept { return ::pthread_cond_wait(&cv_, &(lock.native_handle())); }

    int wait_absolute(pt_unique_lock &lock, const timespec &timeout_abs) noexcept
    {
        return timedwait_absolute(lock, timeout_abs);
    }

    int wait_relative_ms(pt_unique_lock &lock, int timeout_ms) noexcept
    {
        timespec ts({ 0, 0 });
        ts = timespec_utils::add_ms(ts, timeout_ms);
        return timedwait_relative(lock, ts);
    }

    int wait_relative_us(pt_unique_lock &lock, int timeout_us) noexcept
    {
        timespec ts({ 0, 0 });
        ts = timespec_utils::add_us(ts, timeout_us);
        return timedwait_relative(lock, ts);
    }

    int wait_relative_ns(pt_unique_lock &lock, int timeout_ns) noexcept
    {
        timespec ts({ 0, timeout_ns });
        return timedwait_relative(lock, ts);
    }

    int wait_relative(pt_unique_lock &lock, const timespec &timeout_ts) noexcept
    {
        return timedwait_relative(lock, timeout_ts);
    }

    pthread_cond_t &native_handle() noexcept { return cv_; }

private:
    int timedwait_absolute(pt_unique_lock &lock, const timespec &timeout_abs) noexcept
    {
#if __ANDROID_API__ <= 19
        return ::pthread_cond_timedwait_monotonic_np(&cv_, &(lock.native_handle()), &timeout_abs);
#else
        return ::pthread_cond_timedwait(&cv_, &(lock.native_handle()), &timeout_abs);
#endif
    }

    int timedwait_relative(pt_unique_lock &lock, const timespec &offset) noexcept
    {
#if __ANDROID_API__ <= 19
        timespec ts({ 0, 0 });

        static_assert(HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE, "HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE is requred");

        ts = timespec_utils::add_timespec(ts, offset);

        return ::pthread_cond_timedwait_relative_np(&cv_, &(lock.native_handle()), &ts);
#else
        timespec ts({ 0, 0 });

        timespec_utils::get_current_time(ts);
        ts = timespec_utils::add_timespec(ts, offset);

        return ::pthread_cond_timedwait(&cv_, &(lock.native_handle()), &ts);
#endif
    }

    pthread_cond_t cv_;
};

} // namespace utils
} // namespace oslmp

#endif // PTHREAD_UTILS_HPP_
