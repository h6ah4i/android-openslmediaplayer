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

#include <SLESCXX/OpenSLES_CXX.hpp>

#include "oslmp/impl/AudioDataTypes.hpp"

//
// forward declarations
//
namespace oslmp {
namespace impl {
class OpenSLMediaPlayerInternalContext;
class AudioSinkDataPipe;
class AudioDataPipeManager;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {


//
// AudioSink
//
class AudioSink {
public:
    enum state_t { SINK_STATE_NOT_INITIALIZED, SINK_STATE_STOPPED, SINK_STATE_STARTED, SINK_STATE_PAUSED, };
    enum backend_t { BACKEND_OPENSL, BACKEND_AUDIO_TRACK, };

    struct initialize_args_t {
        OpenSLMediaPlayerInternalContext *context;
        backend_t backend;
        sample_format_type sample_format;
        uint32_t sampling_rate;
        int stream_type;
        uint32_t opts;
        AudioDataPipeManager *pipe_manager;
        AudioSinkDataPipe *pipe;
        uint32_t num_player_blocks;

        initialize_args_t()
            : context(nullptr), backend(BACKEND_OPENSL), 
              sample_format(kAudioSampleFormatType_Unknown), sampling_rate(0), stream_type(0),
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
    int32_t getAudioSessionId() const noexcept;

    int selectActiveAuxEffect(int aux_effect_id) noexcept;
    int setAuxEffectSendLevel(float level) noexcept;
    int setAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept;

    SLresult getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept;
    SLresult getInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept;

    enum {
        NUM_CHANNELS = 2
    };
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};


//
// AudioSinkBackend
//
class AudioSinkBackend {
public:
    AudioSinkBackend() : pipe_(nullptr) {
    }

    virtual ~AudioSinkBackend() {}

    virtual int onInitialize(const AudioSink::initialize_args_t &args, void *pipe_user) noexcept = 0;
    virtual int onStart() noexcept = 0;
    virtual int onPause() noexcept = 0;
    virtual int onResume() noexcept = 0;
    virtual int onStop() noexcept = 0;
    virtual uint32_t onGetLatencyInFrames() const noexcept = 0;
    virtual int32_t onGetAudioSessionId() const noexcept = 0;
    virtual int onSelectActiveAuxEffect(int aux_effect_id) noexcept = 0;
    virtual int onSetAuxEffectSendLevel(float level) noexcept = 0;
    virtual int onSetAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept = 0;

    virtual SLresult onGetInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept = 0;
    virtual SLresult onGetInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept = 0;

protected:
    AudioSinkDataPipe *pipe_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOSINK_HPP_
