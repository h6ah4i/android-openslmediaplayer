//
//    Copyright (C) 2016 Haruki Hasegawa
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

#include "oslmp/impl/AudioSink.hpp"

#ifdef USE_OSLMP_DEBUG_FEATURES
#include "oslmp/impl/NonBlockingTraceLogger.hpp"
#endif

//
// forward declarations
//
namespace oslmp {
namespace impl {
class AudioTrackStream;
} // namespace impl
} // namespace oslmp


namespace oslmp {
namespace impl {

class AudioSinkAudioTrackBackend : public AudioSinkBackend {
public:
    AudioSinkAudioTrackBackend();
    virtual ~AudioSinkAudioTrackBackend() override;

    virtual int onInitialize(const AudioSink::initialize_args_t &args, void *pipe_user) noexcept override;
    virtual int onStart() noexcept override;
    virtual int onPause() noexcept override;
    virtual int onResume() noexcept override;
    virtual int onStop() noexcept override;
    virtual uint32_t onGetLatencyInFrames() const noexcept override;
    virtual int32_t onGetAudioSessionId() const noexcept override;
    virtual int onSelectActiveAuxEffect(int aux_effect_id) noexcept override;
    virtual int onSetAuxEffectSendLevel(float level) noexcept override;
    virtual int onSetAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept override;

    virtual SLresult onGetInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept override;
    virtual SLresult onGetInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept override;

    virtual int onSetNotifyPullCallback(void (*pfunc)(void *), void *args) noexcept override;

private:
    static int32_t audioTrackStreamCallback(void *buffer, sample_format_type format, uint32_t num_channels, uint32_t buffer_size_in_frames, void *args) noexcept;

private:
    int block_size_in_frames_;
    uint32_t num_player_blocks_;
    uint32_t num_pipe_blocks_;
    std::unique_ptr<AudioTrackStream> stream_;
    int32_t audio_session_id_;

    void (*notify_pull_callback_pfunc_)(void *);
    void *notify_pull_callback_args_;

#ifdef USE_OSLMP_DEBUG_FEATURES
    std::unique_ptr<NonBlockingTraceLoggerClient> nb_logger_;
#endif
};

} // namespace impl
} // namespace oslmp
