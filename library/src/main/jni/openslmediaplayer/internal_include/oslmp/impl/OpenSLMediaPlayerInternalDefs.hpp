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

#ifndef OPENSLMEDIAPLAYERINTERNALDEFS_HPP_
#define OPENSLMEDIAPLAYERINTERNALDEFS_HPP_

#include <cerrno>

//
// Error codes
//

// NOTE:
//  The error codes are same with Android platform ones.
//  (ref. platform/frameworks/base/include/utils/Errors.h)

#define ERROR_WHAT_NO_ERROR (0)
#define ERROR_WHAT_UNKNOWN_ERROR (0x80000000)
#define ERROR_WHAT_NO_MEMORY (-ENOMEM)
#define ERROR_WHAT_INVALID_OPERATION (-ENOSYS)
#define ERROR_WHAT_BAD_VALUE (-EINVAL)
#define ERROR_WHAT_BAD_TYPE (0x80000001)
#define ERROR_WHAT_PERMISSION_DENIED (-EPERM)
#define ERROR_WHAT_NO_INIT (-ENODEV)
#define ERROR_WHAT_DEAD_OBJECT (-EPIPE)
#define ERROR_WHAT_TIMED_OUT (-ETIMEDOUT)

namespace oslmp {
namespace impl {

//
// Player state
//
typedef enum {
    STATE_CREATED,
    STATE_IDLE,
    STATE_INITIALIZED,
    STATE_PREPARING_SYNC,
    STATE_PREPARING_ASYNC,
    STATE_PREPARED,
    STATE_STARTED,
    STATE_PAUSED,
    STATE_PLAYBACK_COMPLETED,
    STATE_STOPPED,
    STATE_END,
    STATE_ERROR,
} PlayerState;

} // namespace impl
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERINTERNALDEFS_HPP_
