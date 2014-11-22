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

#ifndef TIMESPEC_UTILS_HPP_
#define TIMESPEC_UTILS_HPP_

#include <cxxporthelper/cstdint>
#include <cxxporthelper/time.hpp>
#include <cxxporthelper/compiler.hpp>

namespace oslmp {
namespace utils {

class timespec_utils {
private:
    timespec_utils();

public:
    const static constexpr timespec ZERO() noexcept
    {
        return timespec({ 0, 0 });
    }

    static bool is_zero(const timespec &t) noexcept { return (t.tv_sec == 0 && t.tv_nsec == 0); }

    static bool compare(const timespec &t1, const timespec &t2) noexcept { return compare_equals(t1, t2); }

    // t1 == t2
    static bool compare_equals(const timespec &t1, const timespec &t2) noexcept
    {
        return (t1.tv_sec == t2.tv_sec) && (t1.tv_nsec == t2.tv_nsec);
    }

    // t1 >= t2
    static bool compare_greater_than_or_equals(const timespec &t1, const timespec &t2) noexcept
    {
        return (t1.tv_sec > t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_nsec >= t2.tv_nsec));
    }

    // t1 > t2
    static bool compare_greater_than(const timespec &t1, const timespec &t2) noexcept
    {
        return (t1.tv_sec > t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_nsec > t2.tv_nsec));
    }

    // t1 <= t2
    static bool compare_less_than_or_equals(const timespec &t1, const timespec &t2) noexcept
    {
        return (t1.tv_sec < t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_nsec <= t2.tv_nsec));
    }

    // t1 < t2
    static bool compare_less_than(const timespec &t1, const timespec &t2) noexcept
    {
        return (t1.tv_sec < t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_nsec < t2.tv_nsec));
    }

    static int64_t sub_ret_ms(const timespec &t1, const timespec &t2) noexcept
    {
        // this function returns (t1 - t2)
        int64_t msec;

        msec = (t1.tv_sec - t2.tv_sec) * 1000LL;
        msec += (t1.tv_nsec - t2.tv_nsec) / 1000000LL;

        return msec;
    }

    static int64_t sub_ret_us(const timespec &t1, const timespec &t2) noexcept
    {
        // this function returns (t1 - t2)
        int64_t usec;

        usec = (t1.tv_sec - t2.tv_sec) * 1000000LL;
        usec += (t1.tv_nsec - t2.tv_nsec) / 1000LL;

        return usec;
    }

    static int64_t sub_ret_ns(const timespec &t1, const timespec &t2) noexcept
    {
        // this function returns (t1 - t2)
        int64_t usec;

        usec = (t1.tv_sec - t2.tv_sec) * 100000000LL;
        usec += (t1.tv_nsec - t2.tv_nsec);

        return usec;
    }

    static timespec &set_zero(timespec &t) noexcept
    {
        t.tv_sec = 0;
        t.tv_nsec = 0;
        return t;
    }

    static timespec &normalize(timespec &t) noexcept
    {
        const time_t TIME_T_MAX = static_cast<time_t>((1ULL << (sizeof(time_t) * 8 - 1)) - 1ULL);

        if (CXXPH_UNLIKELY(t.tv_nsec >= 1000000000L)) {
            if (CXXPH_LIKELY(t.tv_sec < TIME_T_MAX)) {
                t.tv_sec += 1L;
                t.tv_nsec -= 1000000000L;
            } else {
                // clip to upper limit
                t.tv_sec = TIME_T_MAX;
                t.tv_nsec = 1000000000L;
            }
        } else if (CXXPH_UNLIKELY(t.tv_nsec < 0L)) {
            if (CXXPH_LIKELY(t.tv_sec > 0L)) {
                t.tv_sec -= 1L;
                t.tv_nsec += 1000000000L;
            } else {
                // clip to lower limit
                t.tv_sec = 0;
                t.tv_nsec = 0;
            }
        }

        return t;
    }

    static timespec add_ms(const timespec &t1, uint32_t ms) noexcept
    {
        timespec result;

        result.tv_sec = t1.tv_sec + (ms / 1000L);
        result.tv_nsec = t1.tv_nsec + (ms % 1000L) * 1000000L;

        normalize(result);

        return result;
    }

    static timespec add_us(const timespec &t1, uint32_t us) noexcept
    {
        timespec result;

        result.tv_sec = t1.tv_sec + (us / 1000000L);
        result.tv_nsec = t1.tv_nsec + (us % 1000000L) * 1000L;

        normalize(result);

        return result;
    }

    static timespec add_ns(const timespec &t1, uint32_t ns) noexcept
    {
        timespec result;

        result.tv_sec = t1.tv_sec + (ns / 1000000000L);
        result.tv_nsec = t1.tv_nsec + (ns % 1000000000L);

        normalize(result);

        return result;
    }

    static timespec add_timespec(const timespec &t1, const timespec &t2) noexcept
    {
        timespec result;

        result.tv_sec = t1.tv_sec + t2.tv_sec;
        result.tv_nsec = t1.tv_nsec + t2.tv_nsec;

        normalize(result);

        return result;
    }

    static bool get_current_time(timespec &t) noexcept
    {
#if 0
        return (::clock_gettime(CLOCK_MONOTONIC, &t) == 0);
#else
        // NOTE:
        // I noticed that Nexus 5 (4.4.3; KTU84M) has a issue on clock_gettime() function.
        // The tv_sec and tv_nsec fields are NOT handled as atomically.

        int num_retries = 5;

        while (num_retries--) {
            timespec t1, t2;

            int r1 = ::clock_gettime(CLOCK_MONOTONIC, &t1);
            int r2 = ::clock_gettime(CLOCK_MONOTONIC, &t2);

            if (CXXPH_UNLIKELY(r1 != 0))
                return false;
            if (CXXPH_UNLIKELY(r2 != 0))
                return false;

            // check t2 >= t1
            if (CXXPH_LIKELY(t2.tv_sec == t1.tv_sec)) {
                if (CXXPH_LIKELY(t2.tv_nsec >= t1.tv_nsec)) {
                    t = t2;
                    return true;
                } else {
                    // retry (!!!)
                    // asm("nop");
                }
            } else {
                // retry
            }
        }

        return false;
#endif
    }
};

} // namespace utils
} // namespace oslmp

#endif // TIMESPEC_UTILS_HPP_
