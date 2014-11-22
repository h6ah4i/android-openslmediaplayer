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

#ifndef OPENSLMEDIAPLAYERTHREADMESSAGE_HPP_
#define OPENSLMEDIAPLAYERTHREADMESSAGE_HPP_

#include "oslmp/impl/MessageHandlerThread.hpp"

#define MESSAGE_BLOB_SIZE 32

namespace oslmp {
namespace impl {

//
// Message object
//
struct OpenSLMediaPlayerThreadMessage {
    int category;
    int what;
    volatile bool *processed;
    volatile int *result;
    uint8_t blob[MESSAGE_BLOB_SIZE];

    OpenSLMediaPlayerThreadMessage() : category(0), what(0), processed(nullptr), result(nullptr)
    {
        static_assert(sizeof(OpenSLMediaPlayerThreadMessage) <= MessageHandlerThread::MESSAGE_SIZE,
                      "message size check");
    }

    OpenSLMediaPlayerThreadMessage(int category, int what)
        : category(category), what(what), processed(nullptr), result(nullptr)
    {
    }

    bool needNotification() const noexcept { return (processed && result); }
};

static_assert(sizeof(OpenSLMediaPlayerThreadMessage) <= MessageHandlerThread::MESSAGE_SIZE, "Check message size");

//
// Helper methods & macros
//
template <typename Tmsg, typename Tblob>
static Tblob &get_msg_blob(Tmsg &msg)
{
    return *(reinterpret_cast<Tblob *>(&msg.blob[0]));
}

template <typename Tmsg, typename Tblob>
static const Tblob &get_msg_blob(const Tmsg &msg)
{
    return *(reinterpret_cast<const Tblob *>(&msg.blob[0]));
}

#define GET_MSG_BLOB(msg) get_msg_blob<Message, blob_t>(msg)

} // namespace impl
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERTHREADMESSAGE_HPP_
