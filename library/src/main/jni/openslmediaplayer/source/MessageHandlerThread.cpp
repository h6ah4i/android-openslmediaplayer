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

// #define LOG_TAG "MessageHandlerThread"

#include "oslmp/impl/MessageHandlerThread.hpp"

#include <deque>
#include <cerrno>
#include <cassert>
#include <cstring>

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>
#include <cxxporthelper/atomic>

#include <pthread.h>

#include <loghelper/loghelper.h>

#include "oslmp/utils/pthread_utils.hpp"
#include "oslmp/utils/timespec_utils.hpp"

namespace oslmp {
namespace impl {

struct MessageBlock {
    CXXPH_ALIGNAS(4) uint8_t data[MessageHandlerThread::MESSAGE_SIZE];
    CXXPH_ALIGNAS(4) uint8_t tag[MessageHandlerThread::TAG_SIZE];

    MessageBlock() {}

    MessageBlock(const void *data, size_t data_size, const void *tag, size_t tag_size)
    {
        ::memcpy(&(this->data[0]), data, data_size);
        ::memset(&(this->data[data_size]), 0, (sizeof(this->data) - data_size));

        if (tag != 0) {
            ::memcpy(&(this->tag[0]), tag, tag_size);
        } else {
            tag_size = 0;
        }
        ::memset(&(this->tag[tag_size]), 0, (sizeof(this->tag) - tag_size));
    }
};

class MessageHandlerThread::Impl {
public:
    Impl(MessageHandlerThread *holder);
    ~Impl();

    bool start(EventHandler *handler) noexcept;
    bool join(void **retval) noexcept;

    bool post(const void *msg, size_t msg_size, const void *tag, size_t tag_size) noexcept;

    bool checkIsMessagePending() const noexcept;
    bool checkIsMessageOrStopRequestPending() noexcept;

private:
    static void *threadEntryFunc(void *args) noexcept;
    void *threadMainLoop() noexcept;

private:
    EventHandler *handler_;

    pthread_t pthread_;
    mutable utils::pt_mutex mutex_;
    utils::pt_condition_variable cond_;
    std::deque<MessageBlock> msg_queue_;
    std::atomic<bool> stop_req_;
};

MessageHandlerThread::MessageHandlerThread() : impl_(new (std::nothrow) Impl(this)) {}

MessageHandlerThread::~MessageHandlerThread() {}

bool MessageHandlerThread::start(EventHandler *handler) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->start(handler);
}

bool MessageHandlerThread::join(void **retval) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->join(retval);
}

bool MessageHandlerThread::post(const void *msg, size_t msg_size, const void *tag, size_t tag_size) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->post(msg, msg_size, tag, tag_size);
}

bool MessageHandlerThread::checkIsMessagePending() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->checkIsMessagePending();
}

bool MessageHandlerThread::checkIsMessageOrStopRequestPending() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->checkIsMessageOrStopRequestPending();
}

MessageHandlerThread::Impl::Impl(MessageHandlerThread *holder)
    : handler_(nullptr), pthread_(0), msg_queue_(), stop_req_(false)
{
}

MessageHandlerThread::Impl::~Impl() {}

void *MessageHandlerThread::Impl::threadMainLoop() noexcept
{
    const int kMaxContinuousMessageReceive = 4;

    int pr;
    bool stop_requested = false;
    int continuous_received_count = 0;

    // call onEnterHandlerThread()
    try { handler_->onEnterHandlerThread(); }
    catch (const std::exception &e)
    {
        LOGE("An exception occurred in %s while calling onEnterHandlerThread(); %s", __func__, e.what());
    }

    while (true) {
        bool message_received = false;
        MessageBlock msg;

        stop_requested = stop_req_;
        if (stop_requested)
            break;

        //
        // Wait for message
        //
        {
            utils::pt_unique_lock lock(mutex_);

            while (true) {
                stop_requested = stop_req_;
                if (CXXPH_UNLIKELY(stop_requested))
                    break;

                if (CXXPH_UNLIKELY(continuous_received_count >= kMaxContinuousMessageReceive)) {
                    // break here to force call polling method
                    continuous_received_count = 0;
                    break;
                }

                if (CXXPH_UNLIKELY(!msg_queue_.empty())) {
                    msg = msg_queue_.front();
                    msg_queue_.pop_front();
                    message_received = true;
                    continuous_received_count += 1;
                    break;
                } else {
                    continuous_received_count = 0;
                }

                const int timeout_ms = handler_->onDetermineWaitTimeout();

                if (timeout_ms == 0) {
                    break;
                } else if (timeout_ms < 0) {
                    if (cond_.wait(lock) != 0) {
                        break;
                    }
                } else {
                    if (cond_.wait_relative_ms(lock, timeout_ms) != 0) {
                        break;
                    }
                }
            }
        }

        if (stop_requested)
            break;

        // call message handler
        if (message_received) {
            try
            {
                if (!(handler_->onHandleMessage(msg.data, msg.tag))) {
                    break;
                }
            }
            catch (const std::exception &e)
            {
                LOGE("An exception occurred in %s while calling onHandleMessage(); %s", __func__, e.what());
            }
        } else {
            try
            {
                if (!(handler_->onReceiveMessageTimeout())) {
                    break;
                }
            }
            catch (const std::exception &e)
            {
                LOGE("An exception occurred in %s while calling onReceiveMessageTimeout(); %s", __func__, e.what());
            }
        }
    }

    void *retval = nullptr;

    // call onLeaveHandlerThread()
    try { retval = handler_->onLeaveHandlerThread(stop_requested); }
    catch (const std::exception &e)
    {
        LOGE("An exception occurred in %s while calling onLeaveHandlerThread(); %s", __func__, e.what());
    }

    return retval;
}

bool MessageHandlerThread::Impl::start(MessageHandlerThread::EventHandler *handler) noexcept
{
    int s;

    this->handler_ = handler;

    s = ::pthread_create(&pthread_, nullptr, threadEntryFunc, this);

    return (s == 0);
}

bool MessageHandlerThread::Impl::join(void **retval) noexcept
{
    int s;
    void *tmp_retval = nullptr;

    {
        utils::pt_unique_lock lock(mutex_);
        stop_req_ = true;
        cond_.notify_one();
    }

    if (pthread_) {
        s = ::pthread_join(pthread_, &tmp_retval);
    } else {
        s = EINVAL;
    }

    if (s != 0) {
        LOGE("%s - pthread_join() failed; %d", __func__, s);
    }

    if (retval) {
        *retval = tmp_retval;
    }

    return (s == 0);
}

bool MessageHandlerThread::Impl::post(const void *msg, size_t msg_size, const void *tag, size_t tag_size) noexcept
{
    if (CXXPH_UNLIKELY((!msg) || (msg_size > MESSAGE_SIZE) || (tag_size > TAG_SIZE))) {
        assert(false);
        return false;
    }

    utils::pt_unique_lock lock(mutex_);

    bool result = false;

    if (pthread_) {
        try
        {
            MessageBlock blk(msg, msg_size, tag, tag_size);
            msg_queue_.push_back(blk);
            result = true;
        }
        catch (const std::bad_alloc &e) {}
    }

    if (result) {
        cond_.notify_one();
    }

    return result;
}

bool MessageHandlerThread::Impl::checkIsMessagePending() const noexcept
{
    utils::pt_unique_lock lock(mutex_);
    return !msg_queue_.empty();
}

bool MessageHandlerThread::Impl::checkIsMessageOrStopRequestPending() noexcept
{
    utils::pt_unique_lock lock(mutex_);

    bool result = false;

    result |= !msg_queue_.empty();
    result |= stop_req_.load(std::memory_order_relaxed); // NOTE: expecting the mutex effect

    return result;
}

void *MessageHandlerThread::Impl::threadEntryFunc(void *args) noexcept
{
    Impl *impl = static_cast<Impl *>(args);
    return impl->threadMainLoop();
}

} // namespace impl
} // namespace oslmp
