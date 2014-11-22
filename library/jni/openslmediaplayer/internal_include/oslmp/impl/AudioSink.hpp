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

#ifndef AUDIOSINK_HPP_
#define AUDIOSINK_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

#include "oslmp/impl/AudioDataTypes.hpp"

//
// forward declarations
//
namespace opensles {
class CSLObjectItf;
}

namespace oslmp {
namespace impl {
class OpenSLMediaPlayerInternalContext;
class AudioSinkDataPipe;
class AudioDataPipeManager;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

class AudioSink {
public:
    enum state_t { SINK_STATE_NOT_INITIALIZED, SINK_STATE_STOPPED, SINK_STATE_STARTED, SINK_STATE_PAUSED, };

    struct initialize_args_t {
        OpenSLMediaPlayerInternalContext *context;
        sample_format_type sample_format;
        uint32_t sampling_rate;
        int stream_type;
        uint32_t opts;
        AudioDataPipeManager *pipe_manager;
        AudioSinkDataPipe *pipe;
        uint32_t num_player_blocks;

        initialize_args_t()
            : context(nullptr), sample_format(kAudioSampleFormatType_Unknown), sampling_rate(0), stream_type(0),
              opts(0), pipe_manager(nullptr), pipe(nullptr), num_player_blocks(0)
        {
        }
    };

    AudioSink();
    ~AudioSink();

    int initialize(const AudioSink::initialize_args_t &args) noexcept;
    int start() noexcept;
    int pause() noexcept;
    int resume() noexcept;
    int stop() noexcept;
    uint32_t getLatencyInFrames() const noexcept;
    state_t getState() const noexcept;
    AudioSinkDataPipe *getPipe() const noexcept;
    opensles::CSLObjectItf *getPlayerObj() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOSINK_HPP_
