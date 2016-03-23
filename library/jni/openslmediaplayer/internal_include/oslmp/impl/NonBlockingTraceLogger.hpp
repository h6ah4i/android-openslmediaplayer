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

#ifndef NONBLOCKINGTRACELOGGER_HPP_
#define NONBLOCKINGTRACELOGGER_HPP_

#ifdef USE_OSLMP_DEBUG_FEATURES

#include <jni.h>
#include <cstdarg>
#include <cxxporthelper/cstdint>
#include <cxxporthelper/memory>

namespace oslmp {
namespace impl {

class NonBlockingTraceLoggerClient;

class NonBlockingTraceLogger {
    friend class NonBlockingTraceLoggerClient;

    NonBlockingTraceLogger(const NonBlockingTraceLogger &) = delete;
    NonBlockingTraceLogger &operator=(const NonBlockingTraceLogger &) = delete;

public:
    enum { MESSAGE_BUFFER_SIZE = 256, LOG_ENTRY_BUFFER_NUM_PER_CLIENT = 256, NUM_CLIENTS = 24, };

    enum LogLevel {
        LOG_LEVEL_UNKNOWN = 0, // for internal use only
        LOG_LEVEL_VERBOSE,
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_FATAL,
    };

    NonBlockingTraceLogger();
    ~NonBlockingTraceLogger();

    NonBlockingTraceLoggerClient *create_new_client() noexcept;

    bool start_output_worker(JavaVM *jvm, uint32_t polling_period_ms) noexcept;
    bool stop_output_worker() noexcept;

    void trigger_periodic_process() noexcept;

private:
    class WorkerImpl;
    class ClientImpl;
    std::unique_ptr<WorkerImpl> impl_;
};

// NOTE:
// - Instance of the logger is needed per thread.
class NonBlockingTraceLoggerClient {
    friend class NonBlockingTraceLogger::WorkerImpl;

    NonBlockingTraceLoggerClient(const NonBlockingTraceLoggerClient &) = delete;
    NonBlockingTraceLoggerClient &operator=(const NonBlockingTraceLoggerClient &) = delete;

private:
    NonBlockingTraceLoggerClient();

public:
    ~NonBlockingTraceLoggerClient();

    // NOTEs:
    // tag: specify literal string
    // format: non-null, this argument accepts non-literal string
    bool putf(NonBlockingTraceLogger::LogLevel log_level, const char *tag, const char *format, ...) noexcept;

    bool is_attached() const noexcept;

private:
    std::unique_ptr<NonBlockingTraceLogger::ClientImpl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // USE_OSLMP_DEBUG_FEATURES

#endif // NONBLOCKINGTRACELOGGER_HPP_
