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

#include "oslmp/impl/NonBlockingTraceLogger.hpp"

#ifdef USE_OSLMP_DEBUG_FEATURES

#include <lockfree/lockfree_circulation_buffer.hpp>

#include <cassert>
#include <cstdio>

#include <cxxporthelper/atomic>
#include <cxxporthelper/time.hpp>

#include <android/log.h>

#include "oslmp/impl/AndroidHelper.hpp"

#include "oslmp/utils/timespec_utils.hpp"
#include "oslmp/utils/pthread_utils.hpp"
#include "oslmp/utils/bitmap_looper.hpp"

#define OUTPUT_LOG_TIME

#define INVALID_CLIENT_ID 0xffffffffUL

namespace oslmp {
namespace impl {

typedef unsigned int log_seq_no_t;

struct LogEntry {
    log_seq_no_t sequence_no;
    NonBlockingTraceLogger::LogLevel log_level;
    const char *tag;
    char message[NonBlockingTraceLogger::MESSAGE_BUFFER_SIZE];

#ifdef OUTPUT_LOG_TIME
    timespec timestamp;
#endif

    LogEntry() : sequence_no(0), log_level(NonBlockingTraceLogger::LOG_LEVEL_UNKNOWN), tag(nullptr) {}

    void clear() noexcept
    {
        sequence_no = 0;
        log_level = NonBlockingTraceLogger::LOG_LEVEL_UNKNOWN;
        tag = nullptr;

#ifdef OUTPUT_LOG_TIME
        timestamp.tv_sec = 0;
        timestamp.tv_nsec = 0;
#endif
    }
};

class NonBlockingTraceLogger::WorkerImpl {
public:
    WorkerImpl();
    ~WorkerImpl();

    bool putf(uint32_t client_id, LogLevel log_level, const char *tag, const char *format, va_list ap) noexcept;

    NonBlockingTraceLoggerClient *create_new_client() noexcept;
    void detach_client(uint32_t client_id) noexcept;

    bool start_output_worker(uint32_t polling_period_ms) noexcept;
    bool stop_output_worker() noexcept;

private:
    bool is_initialized() const noexcept;
    bool validate_putf_params(uint32_t client_id, LogLevel log_level, const char *tag, const char *format) const
        noexcept;

    static void *outputWorkerThreadEntry(void *args) noexcept;
    void outputWorkerThreadProcess(void) noexcept;
    int workerPollLogEntries(void) noexcept;
    void outputLog(const LogEntry &entry) noexcept;

private:
    typedef lockfree::lockfree_circulation_buffer<LogEntry, LOG_ENTRY_BUFFER_NUM_PER_CLIENT> lf_log_entry_queue_t;
    uint32_t polling_period_ms_;

    std::unique_ptr<lf_log_entry_queue_t[]> log_entry_queue_pool_;
    std::atomic<log_seq_no_t> producer_sequence_no_counter_;
    log_seq_no_t consumer_sequence_no_counter_;

    pthread_t worker_thread_;
    utils::pt_mutex worker_mutex_;
    utils::pt_condition_variable worker_cv_;
    std::atomic_bool stop_request_worker_tread_;
    uint32_t active_client_map_;
    static_assert(NUM_CLIENTS <= 32, "NUM_CLIENTS cannot be greater than 32");
};

class NonBlockingTraceLogger::ClientImpl {
public:
    ClientImpl(WorkerImpl *worker_impl);
    ~ClientImpl();

    bool putf(LogLevel log_level, const char *tag, const char *format, va_list ap) noexcept;
    bool is_attached() const noexcept;

    void attach(uint32_t client_id) noexcept;

private:
    uint32_t client_id_;
    WorkerImpl *worker_impl_;
};

static inline log_seq_no_t update_increment_nonzero(std::atomic<log_seq_no_t> &x) noexcept
{
    log_seq_no_t y;

    do {
        y = (++x);
    } while (y == 0); // retry if zero

    return y;
}

static inline log_seq_no_t get_increment_nonzero(log_seq_no_t x) noexcept
{
    log_seq_no_t y;

    do {
        y = (++x);
    } while (y == 0); // retry if zero

    return y;
}

static android_LogPriority to_android_log_priority(NonBlockingTraceLogger::LogLevel log_level) noexcept
{
    switch (log_level) {
    case NonBlockingTraceLogger::LOG_LEVEL_VERBOSE:
        return ANDROID_LOG_VERBOSE;
    case NonBlockingTraceLogger::LOG_LEVEL_DEBUG:
        return ANDROID_LOG_DEBUG;
    case NonBlockingTraceLogger::LOG_LEVEL_INFO:
        return ANDROID_LOG_INFO;
    case NonBlockingTraceLogger::LOG_LEVEL_WARNING:
        return ANDROID_LOG_WARN;
    case NonBlockingTraceLogger::LOG_LEVEL_ERROR:
        return ANDROID_LOG_ERROR;
    case NonBlockingTraceLogger::LOG_LEVEL_FATAL:
        return ANDROID_LOG_FATAL;
    default:
        return ANDROID_LOG_DEBUG;
    }
}

//
// NonBlockingTraceLogger
//
NonBlockingTraceLogger::NonBlockingTraceLogger() : impl_(new (std::nothrow) WorkerImpl()) {}

NonBlockingTraceLogger::~NonBlockingTraceLogger() {}

NonBlockingTraceLoggerClient *NonBlockingTraceLogger::create_new_client() noexcept
{
    if (!impl_)
        return nullptr;
    return impl_->create_new_client();
}

bool NonBlockingTraceLogger::start_output_worker(uint32_t polling_period_ms) noexcept
{
    if (!impl_)
        return false;
    return impl_->start_output_worker(polling_period_ms);
}

bool NonBlockingTraceLogger::stop_output_worker() noexcept
{
    if (!impl_)
        return false;
    return impl_->stop_output_worker();
}

//
// NonBlockingTraceLogger::WorkerImpl
//
NonBlockingTraceLogger::WorkerImpl::WorkerImpl()
    : polling_period_ms_(0), log_entry_queue_pool_(), producer_sequence_no_counter_(0),
      consumer_sequence_no_counter_(0), worker_thread_(0), worker_mutex_(), worker_cv_(), active_client_map_(0)
{
    const size_t pool_size = NUM_CLIENTS * LOG_ENTRY_BUFFER_NUM_PER_CLIENT;
    log_entry_queue_pool_.reset(new (std::nothrow) lf_log_entry_queue_t[pool_size]);
}

NonBlockingTraceLogger::WorkerImpl::~WorkerImpl() {}

bool NonBlockingTraceLogger::WorkerImpl::putf(uint32_t client_id, LogLevel log_level, const char *tag,
                                              const char *format, va_list ap) noexcept
{
    // check is initialized
    if (CXXPH_UNLIKELY(!is_initialized())) {
        return false;
    }

    // verivy parameters
    if (CXXPH_UNLIKELY(!validate_putf_params(client_id, log_level, tag, format))) {
        assert(false);
        return false;
    }

    // lock log entry
    lf_log_entry_queue_t::index_t lock_index = lf_log_entry_queue_t::INVALID_INDEX;
    lf_log_entry_queue_t &entry_queue = log_entry_queue_pool_[client_id];
    if (CXXPH_UNLIKELY(!entry_queue.lock_write(lock_index))) {
        return false;
    }

    // fill log entry infos
    LogEntry &entry = entry_queue.at(lock_index);

    entry.sequence_no = update_increment_nonzero(producer_sequence_no_counter_);
    entry.log_level = log_level;
    entry.tag = tag;

#ifdef OUTPUT_LOG_TIME
    utils::timespec_utils::get_current_time(entry.timestamp);
#endif

    {
        va_list ap2;
        va_copy(ap2, ap);
        ::vsnprintf(entry.message, sizeof(entry.message), format, ap);
        va_end(ap2);
    }

    // unlock log entry
    entry_queue.unlock_write(lock_index);

    return true;
}

NonBlockingTraceLoggerClient *NonBlockingTraceLogger::WorkerImpl::create_new_client() noexcept
{
    std::unique_ptr<NonBlockingTraceLoggerClient> client(new (std::nothrow) NonBlockingTraceLoggerClient());
    std::unique_ptr<ClientImpl> client_impl(new (std::nothrow) ClientImpl(this));

    if (CXXPH_UNLIKELY(!(client && client_impl))) {
        return nullptr;
    }

    client->impl_ = (std::move)(client_impl);

    {
        utils::pt_unique_lock lock(worker_mutex_);

        utils::bitmap_looper free_checker(~active_client_map_);

        if (free_checker.loop()) {
            // found a free slot
            client->impl_->attach(free_checker.index());
            active_client_map_ |= (1U << free_checker.index());
        } else {
            // no free slot found
        }
    }

    return client.release();
}

void NonBlockingTraceLogger::WorkerImpl::detach_client(uint32_t client_id) noexcept
{
    if (CXXPH_UNLIKELY(client_id == INVALID_CLIENT_ID)) {
        return;
    }

    {
        utils::pt_unique_lock lock(worker_mutex_);
        active_client_map_ &= ~(1U << client_id);
    }
}

// NOTE: this method is not thread safe
bool NonBlockingTraceLogger::WorkerImpl::start_output_worker(uint32_t polling_period_ms) noexcept
{
    // check is initialized
    if (CXXPH_UNLIKELY(!is_initialized())) {
        return false;
    }

    if (CXXPH_UNLIKELY(worker_thread_)) {
        return true; // already started
    }

    // set polling_period_ms to the field
    polling_period_ms_ = polling_period_ms;

    // clear stop request
    stop_request_worker_tread_ = false;

    // start thread
    int s = ::pthread_create(&worker_thread_, nullptr, outputWorkerThreadEntry, this);
    if (s != 0) {
        return false;
    }

    return true;
}

// NOTE: this method is not thread safe
bool NonBlockingTraceLogger::WorkerImpl::stop_output_worker() noexcept
{
    // check is initialized
    if (CXXPH_UNLIKELY(!is_initialized())) {
        return false;
    }

    if (worker_thread_) {
        void *thread_retval = nullptr;
        stop_request_worker_tread_ = true;
        (void)::pthread_join(worker_thread_, &thread_retval);
        worker_thread_ = 0;
    }

    return true;
}

void *NonBlockingTraceLogger::WorkerImpl::outputWorkerThreadEntry(void *args) noexcept
{
    WorkerImpl *impl = static_cast<WorkerImpl *>(args);

    // set thread name
    AndroidHelper::setCurrentThreadName("NBTraceLogger");

    impl->outputWorkerThreadProcess();
    return nullptr;
}

void NonBlockingTraceLogger::WorkerImpl::outputWorkerThreadProcess(void) noexcept
{
    while (!stop_request_worker_tread_) {
        const int num_processed = workerPollLogEntries();

        if (num_processed == 0) {
            utils::pt_unique_lock lock(worker_mutex_);
            worker_cv_.wait_relative_ms(lock, polling_period_ms_);
        }
    }
}

int NonBlockingTraceLogger::WorkerImpl::workerPollLogEntries() noexcept
{
    bool processed = false;
    const int expected_sequence_no = get_increment_nonzero(consumer_sequence_no_counter_);

    // find expected sequence number log entry from all clients
    for (int client_id = 0; client_id < NUM_CLIENTS; ++client_id) {
        lf_log_entry_queue_t::index_t lock_index = lf_log_entry_queue_t::INVALID_INDEX;
        lf_log_entry_queue_t &entry_queue = log_entry_queue_pool_[client_id];

        if (entry_queue.lock_read(lock_index)) {
            LogEntry &entry = entry_queue.at(lock_index);

            if (entry.sequence_no == expected_sequence_no) {
                // found!
                outputLog(entry);
                entry.clear();
                entry_queue.unlock_read(lock_index, true);
                processed = true;
                break;
            } else {
                entry_queue.unlock_read(lock_index, false);
            }
        }
    }

    // update fields
    if (processed) {
        consumer_sequence_no_counter_ = expected_sequence_no;
    }

    return (processed) ? 1 : 0;
}

void NonBlockingTraceLogger::WorkerImpl::outputLog(const LogEntry &entry) noexcept
{
#ifdef OUTPUT_LOG_TIME
    ::__android_log_print(to_android_log_priority(entry.log_level), entry.tag, "[%ld.%06ld] %s", entry.timestamp.tv_sec,
                          (entry.timestamp.tv_nsec / 1000), entry.message);
#else
    ::__android_log_write(to_android_log_priority(entry.log_level), entry.tag, entry.message);
#endif
}

bool NonBlockingTraceLogger::WorkerImpl::is_initialized() const noexcept
{
    if (CXXPH_UNLIKELY(!log_entry_queue_pool_)) {
        return false;
    }

    return true;
}

bool NonBlockingTraceLogger::WorkerImpl::validate_putf_params(uint32_t client_id, LogLevel log_level, const char *tag,
                                                              const char *format) const noexcept
{
    if (!(client_id < NUM_CLIENTS))
        return false;
    if (!(log_level >= LOG_LEVEL_VERBOSE && log_level <= LOG_LEVEL_FATAL))
        return false;
    if (!tag)
        return false; // NOTE: cannot verify, but the tag requires a literal string.
    if (!format)
        return false;
    return true;
}

//
// NonBlockingTraceLoggerClient
//
NonBlockingTraceLoggerClient::NonBlockingTraceLoggerClient() : impl_() {}

NonBlockingTraceLoggerClient::~NonBlockingTraceLoggerClient() {}

bool NonBlockingTraceLoggerClient::putf(NonBlockingTraceLogger::LogLevel log_level, const char *tag, const char *format,
                                        ...) noexcept
{
    bool result;

    if (CXXPH_UNLIKELY(!impl_))
        return false;

    va_list ap;
    va_start(ap, format);
    result = impl_->putf(log_level, tag, format, ap);
    va_end(ap);

    return result;
}

bool NonBlockingTraceLoggerClient::is_attached() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->is_attached();
}

//
// NonBlockingTraceLogger::ClientImpl
//
NonBlockingTraceLogger::ClientImpl::ClientImpl(WorkerImpl *worker_impl)
    : client_id_(INVALID_CLIENT_ID), worker_impl_(worker_impl)
{
}

NonBlockingTraceLogger::ClientImpl::~ClientImpl()
{
    if (worker_impl_ && (is_attached())) {
        worker_impl_->detach_client(client_id_);
        client_id_ = INVALID_CLIENT_ID;
    }
}

bool NonBlockingTraceLogger::ClientImpl::putf(LogLevel log_level, const char *tag, const char *format,
                                              va_list ap) noexcept
{
    bool result;

    if (CXXPH_UNLIKELY(!is_attached()))
        return false;

    {
        va_list ap2;
        va_copy(ap2, ap);
        result = worker_impl_->putf(client_id_, log_level, tag, format, ap);
        va_end(ap2);
    }

    return result;
}

bool NonBlockingTraceLogger::ClientImpl::is_attached() const noexcept { return (client_id_ != INVALID_CLIENT_ID); }

void NonBlockingTraceLogger::ClientImpl::attach(uint32_t client_id) noexcept { client_id_ = client_id; }

} // namespace impl
} // namespace oslmp

#endif // USE_OSLMP_DEBUG_FEATURES
