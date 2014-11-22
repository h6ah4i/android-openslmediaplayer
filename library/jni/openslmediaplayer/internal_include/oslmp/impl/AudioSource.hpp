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

#ifndef AUDIOSOURCE_HPP_
#define AUDIOSOURCE_HPP_

#include <string>

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

//
// forward declarations
//
namespace oslmp {
namespace impl {
class OpenSLMediaPlayerInternalContext;
class AudioSourceDataPipe;
class AudioDataPipeManager;
class OpenSLMediaPlayerMetadata;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

class AudioSource {
public:
    enum data_source_type_t { DATA_SOURCE_NONE, DATA_SOURCE_PATH, DATA_SOURCE_URI, DATA_SOURCE_FD, };

    enum playback_completion_type_t { PLAYBACK_NOT_COMPLETED, PLAYBACK_COMPLETED, PLAYBACK_COMPLETED_WITH_LOOP_POINT, };

    struct data_source_info_t {
        data_source_type_t type;
        std::string path_uri;
        int fd;
        int64_t offset;
        int64_t length;

        data_source_info_t() : type(DATA_SOURCE_NONE), path_uri(), fd(0), offset(0), length(0) {}
    };

    struct initialize_args_t {
        OpenSLMediaPlayerInternalContext *context;
        uint32_t sampling_rate;
        AudioDataPipeManager *pipe_manager;
        AudioSourceDataPipe *pipe;

        initialize_args_t() : context(nullptr), sampling_rate(0), pipe_manager(nullptr), pipe(nullptr) {}
    };

    struct prepare_args_t {
        data_source_info_t *data_source;
        int32_t initial_seek_position_msec;

        prepare_args_t() : data_source(nullptr), initial_seek_position_msec(0) {}
    };

    struct prepare_poll_args_t {
        bool completed;
        bool need_retry;

        prepare_poll_args_t() : completed(false), need_retry(false) {}
    };

    AudioSource();
    ~AudioSource();

    int initialize(const initialize_args_t &args) noexcept;

    int startPreparing(const prepare_args_t &args) noexcept;
    int pollPreparing(prepare_poll_args_t &args) noexcept;

    int start() noexcept;
    int pause() noexcept;
    int stopDecoder() noexcept;

    bool isPreparing() const noexcept;
    bool isPrepared() const noexcept;
    bool isStarted() const noexcept;
    playback_completion_type_t getPlaybackCompletionType() const noexcept;

    AudioSourceDataPipe *getPipe() const noexcept;
    uint32_t getId() const noexcept;

    int getMetaData(OpenSLMediaPlayerMetadata *metadata) const noexcept;
    int getCurrentPosition(int32_t *position) const noexcept;
    int getBufferedPosition(int32_t *position) const noexcept;

    int32_t getInitialSeekPosition() const noexcept;

    bool isNetworkSource() const noexcept;
    bool isDecoderReachedToEndOfData() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOSOURCE_HPP_
