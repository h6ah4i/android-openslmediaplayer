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

#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"

#include <string>
#include <vector>
#include <cstdlib>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <oslmp/OpenSLMediaPlayer.hpp>

namespace oslmp {
namespace impl {

static int decode_uri(const char *src, char *dest, size_t n) noexcept
{
    int dest_cnt = 0;
    int work_cnt = 0;
    char work[4] = { '\0', '\0', '\0', '\0' };

    for (size_t i = 0; (i < n) && (src[i] != '\0'); ++i) {
        if (work_cnt > 0) {
            work[work_cnt] = src[i];
            ++work_cnt;

            if (work_cnt == 3) {
                char *pe = nullptr;
                long lvalue = ::strtol(&work[1], &pe, 16);

                if (pe != &work[3])
                    return -1;

                dest[dest_cnt] = static_cast<char>(lvalue);
                ++dest_cnt;
                work_cnt = 0;
            }
        } else {
            if (src[i] == '%') {
                work[0] = src[i];
                work_cnt = 1;
            } else {
                dest[dest_cnt] = src[i];
                ++dest_cnt;
            }
        }
    }

    if (work_cnt != 0)
        return -1;

    return dest_cnt;
}

// Decode 'Percent-encoded' URI string
//   http://en.wikipedia.org/wiki/Percent-encoding
bool OpenSLMediaPlayerInternalUtils::sDecodeUri(const std::string &src, std::string &dest) noexcept
{
    dest.clear();

    try
    {
        std::vector<char> buff(src.size() + 1);
        int result = decode_uri(src.c_str(), &buff[0], src.size());

        if (result < 0) {
            return false;
        }

        dest.append(&buff[0], result);

        return true;
    }
    catch (const std::bad_alloc &) { return false; }

    return false;
}

int OpenSLMediaPlayerInternalUtils::sTranslateOpenSLErrorCode(SLresult result) noexcept
{
    switch (result) {
    case SL_RESULT_SUCCESS:
        return OSLMP_RESULT_SUCCESS;
    case SL_RESULT_PARAMETER_INVALID:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    case SL_RESULT_MEMORY_FAILURE:
        return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
    case SL_RESULT_RESOURCE_ERROR:
    case SL_RESULT_RESOURCE_LOST:
        return OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
    case SL_RESULT_IO_ERROR:
        return OSLMP_RESULT_IO_ERROR;
    case SL_RESULT_PERMISSION_DENIED:
        return OSLMP_RESULT_PERMISSION_DENIED;
    case SL_RESULT_CONTENT_CORRUPTED:
    case SL_RESULT_CONTENT_UNSUPPORTED:
        return OSLMP_RESULT_CONTENT_UNSUPPORTED;
    case SL_RESULT_CONTENT_NOT_FOUND:
        return OSLMP_RESULT_CONTENT_NOT_FOUND;
    case SL_RESULT_PRECONDITIONS_VIOLATED:
        return OSLMP_RESULT_ILLEGAL_STATE;
    case SL_RESULT_BUFFER_INSUFFICIENT:
    case SL_RESULT_FEATURE_UNSUPPORTED:
    case SL_RESULT_INTERNAL_ERROR:
    case SL_RESULT_OPERATION_ABORTED:
        return OSLMP_RESULT_INTERNAL_ERROR;
    case SL_RESULT_CONTROL_LOST:
        return OSLMP_RESULT_CONTROL_LOST;
    case SL_RESULT_UNKNOWN_ERROR:
    default:
        return OSLMP_RESULT_ERROR;
    }
}

bool OpenSLMediaPlayerInternalUtils::sTranslateToOpenSLStreamType(int streamType, SLint32 *slStreamType) noexcept
{
    switch (streamType) {
    case OSLMP_STREAM_VOICE:
        (*slStreamType) = SL_ANDROID_STREAM_VOICE;
        break;
    case OSLMP_STREAM_SYSTEM:
        (*slStreamType) = SL_ANDROID_STREAM_SYSTEM;
        break;
    case OSLMP_STREAM_RING:
        (*slStreamType) = SL_ANDROID_STREAM_RING;
        break;
    case OSLMP_STREAM_MUSIC:
        (*slStreamType) = SL_ANDROID_STREAM_MEDIA;
        break;
    case OSLMP_STREAM_ALARM:
        (*slStreamType) = SL_ANDROID_STREAM_ALARM;
        break;
    case OSLMP_STREAM_NOTIFICATION:
        (*slStreamType) = SL_ANDROID_STREAM_NOTIFICATION;
        break;
    default:
        (*slStreamType) = SL_ANDROID_STREAM_MEDIA;
        return false;
    }

    return true;
}

bool OpenSLMediaPlayerInternalUtils::sTranslateOpenSLStreamType(SLint32 slStreamType, int *streamType) noexcept
{
    switch (slStreamType) {
    case SL_ANDROID_STREAM_VOICE:
        (*streamType) = OSLMP_STREAM_VOICE;
        break;
    case SL_ANDROID_STREAM_SYSTEM:
        (*streamType) = OSLMP_STREAM_SYSTEM;
        break;
    case SL_ANDROID_STREAM_RING:
        (*streamType) = OSLMP_STREAM_RING;
        break;
    case SL_ANDROID_STREAM_MEDIA:
        (*streamType) = OSLMP_STREAM_MUSIC;
        break;
    case SL_ANDROID_STREAM_ALARM:
        (*streamType) = OSLMP_STREAM_ALARM;
        break;
    case SL_ANDROID_STREAM_NOTIFICATION:
        (*streamType) = OSLMP_STREAM_NOTIFICATION;
        break;
    default:
        (*streamType) = OSLMP_STREAM_MUSIC;
        return false;
    }

    return true;
}

const char *OpenSLMediaPlayerInternalUtils::sGetPlayerStateName(PlayerState state) noexcept
{
    switch (state) {
    case STATE_CREATED:
        return "STATE_CREATED";
    case STATE_IDLE:
        return "STATE_IDLE";
    case STATE_INITIALIZED:
        return "STATE_INITIALIZED";
    case STATE_PREPARING_SYNC:
        return "STATE_PREPARING_SYNC";
    case STATE_PREPARING_ASYNC:
        return "STATE_PREPARING_ASYNC";
    case STATE_PREPARED:
        return "STATE_PREPARED";
    case STATE_STARTED:
        return "STATE_STARTED";
    case STATE_PAUSED:
        return "STATE_PAUSED";
    case STATE_PLAYBACK_COMPLETED:
        return "STATE_PLAYBACK_COMPLETED";
    case STATE_STOPPED:
        return "STATE_STOPPED";
    case STATE_END:
        return "STATE_END";
    case STATE_ERROR:
        return "STATE_ERROR";
    default:
        return "Unknown";
    }
}

} // namespace impl
} // namespace oslmp
