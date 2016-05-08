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

#ifndef MESSAGEHANDLERTHREAD_HPP_
#define MESSAGEHANDLERTHREAD_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstddef>
#include <cxxporthelper/cstdint>

namespace oslmp {
namespace impl {

class MessageHandlerThread {
    MessageHandlerThread(const MessageHandlerThread &) = delete;
    MessageHandlerThread &operator=(const MessageHandlerThread &) = delete;

public:
    enum { MESSAGE_SIZE = 32 + (sizeof(uintptr_t) * 4), TAG_SIZE = 16 };
    class EventHandler;

    MessageHandlerThread();
    virtual ~MessageHandlerThread();

    bool start(EventHandler *handler) noexcept;
    bool join(void **retval) noexcept;

    bool post(const void *msg, size_t msg_size, const void *tag, size_t tag_size) noexcept;

    bool checkIsMessagePending() const noexcept;
    bool checkIsMessageOrStopRequestPending() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class MessageHandlerThread::EventHandler {
public:
    virtual ~EventHandler() {}

    virtual void onEnterHandlerThread() noexcept = 0;
    virtual bool onHandleMessage(const void *msg, const void *tag) noexcept = 0;
    virtual bool onReceiveMessageTimeout() noexcept = 0;
    virtual void *onLeaveHandlerThread(bool stop_requested) noexcept = 0;

    // return timeout duration in milliseconds
    // <  0: infinity
    // == 0: timeout immediately
    // >  0: specified time
    virtual int onDetermineWaitTimeout() noexcept = 0;
};

} // namespace impl
} // namespace oslmp

#endif // MESSAGEHANDLERTHREAD_HPP_
