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

#ifndef AUDIOMIXER_HPP_
#define AUDIOMIXER_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

#include "oslmp/impl/MixingUnit.hpp"

//
// forward declarations
//
namespace oslmp {
namespace impl {
class OpenSLMediaPlayerInternalContext;
class AudioDataPipeManager;
class AudioSourceDataPipe;
class AudioSinkDataPipe;
class AudioCaptureDataPipe;
class StereoVolumeDataPipe;
class MixedOutputAudioEffect;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

class AudioMixer {
public:
    enum mixing_start_cause_t {
        MIXING_START_CAUSE_EXPLICIT_OPERATION,
        MIXING_START_CAUSE_LOOP_TRIGGERED,
        MIXING_START_CAUSE_NO_LOOP_TRIGGERED,
    };

    enum mixing_stop_cause_t {
        MIXING_STOP_CAUSE_EXPLICIT_OPERATION,
        MIXING_STOP_CAUSE_FADED_OUT,
        MIXING_STOP_CAUSE_END_OF_DATA_NO_TRIGGERED,
        MIXING_STOP_CAUSE_END_OF_DATA_TRIGGERED_NO_LOOPING_SOURCE,
        MIXING_STOP_CAUSE_END_OF_DATA_TRIGGERED_LOOPING_SOURCE,
        MIXING_STOP_CAUSE_DETACHED,
    };

    class SourceClientEventHandler {
    public:
        virtual ~SourceClientEventHandler() {}

        virtual void onMixingStarted(AudioSourceDataPipe *pipe, mixing_start_cause_t cause) noexcept = 0;
        virtual void onMixingStopped(AudioSourceDataPipe *pipe, mixing_stop_cause_t cause) noexcept = 0;
    };

    typedef struct st_source_client_handle {
        uint32_t f1_;
        uint32_t f2_;
        uintptr_t f3_;

        st_source_client_handle() : f1_(0), f2_(0), f3_(0) {}

        bool operator==(const st_source_client_handle &h) const
        {
            return (h.f1_ == f1_) && (h.f2_ == f2_) && (h.f3_ == f3_);
        }

        bool operator!=(const st_source_client_handle &h) const { return !(*this == h); }
    } source_client_handle_t;

    enum {
        NUM_MAX_SOURCE_CLIENTS = 4,
        NUM_MAX_SOURCE_PIPES = (NUM_MAX_SOURCE_CLIENTS * 4),
        NUM_MAX_MIXOOUT_EFFECTS = 4,
    };

    enum mixing_mode_t {
        MIX_MODE_MUTE = MixingUnit::MODE_MUTE,
        MIX_MODE_ADD = MixingUnit::MODE_ADD,
        MIX_MODE_SHORT_FADE_IN = MixingUnit::MODE_SHORT_FADE_IN,
        MIX_MODE_SHORT_FADE_OUT = MixingUnit::MODE_SHORT_FADE_OUT,
        MIX_MODE_LONG_FADE_IN = MixingUnit::MODE_LONG_FADE_IN,
        MIX_MODE_LONG_FADE_OUT = MixingUnit::MODE_LONG_FADE_OUT,
    };

    enum trigger_mode_t { TRIGGER_NONE, TRIGGER_ON_END_OF_DATA, };

    // NOTE: stop_cond_t is bitmap
    enum stop_cond_t { STOP_NONE = 0, STOP_ON_PLAYBACK_END = 1, STOP_AFTER_FADE_OUT = 2, };

    enum operation_t {
        OPERATION_NONE,
        OPERATION_START,
        OPERATION_STOP,
        OPERATION_DETACH, // internal use only
    };

    enum state_t { MIXER_STATE_NOT_INITIALIZED, MIXER_STATE_STOPPED, MIXER_STATE_STARTED, MIXER_STATE_SUSPENDED, };

    struct initialize_args_t {
        OpenSLMediaPlayerInternalContext *context;
        AudioDataPipeManager *pipe_manager;
        AudioSinkDataPipe *sink_pipe;
        AudioCaptureDataPipe *capture_pipe;
        uint32_t sampling_rate;
        uint32_t short_fade_duration_ms;
        uint32_t long_fade_duration_ms;
        uint32_t num_sink_player_blocks;

        MixedOutputAudioEffect *mixout_effects[NUM_MAX_MIXOOUT_EFFECTS];

        initialize_args_t()
            : context(nullptr), pipe_manager(nullptr), sink_pipe(nullptr), capture_pipe(nullptr), sampling_rate(0),
              short_fade_duration_ms(0), long_fade_duration_ms(0), num_sink_player_blocks(0)
        {

            for (auto &e : mixout_effects) {
                e = nullptr;
            }
        }
    };

    struct attach_update_source_args_t {
        source_client_handle_t handle;
        uint32_t source_no;

        // operation
        operation_t operation;

        // source pipe
        AudioSourceDataPipe *source_pipe;

        // mixing
        mixing_mode_t mix_mode;
        float mix_phase;
        bool mix_phase_override;

        // trigger (for no looping)
        trigger_mode_t trigger_no_loop_mode;
        source_client_handle_t trigger_no_loop_target;
        uint32_t trigger_no_loop_source_no;

        // trigger (for looping)
        trigger_mode_t trigger_loop_mode;
        source_client_handle_t trigger_loop_target;
        uint32_t trigger_loop_source_no;

        // stop
        uint32_t stop_cond;

        attach_update_source_args_t()
            : handle(), operation(OPERATION_NONE), source_no(0), source_pipe(nullptr), mix_mode(MIX_MODE_MUTE),
              mix_phase(0.0f), mix_phase_override(false), trigger_no_loop_mode(TRIGGER_NONE), trigger_no_loop_target(),
              trigger_loop_mode(TRIGGER_NONE), trigger_loop_target(), stop_cond(STOP_NONE)
        {
        }
    };

    struct register_source_client_args_t {
        void *client;
        SourceClientEventHandler *event_handler;
        source_client_handle_t control_handle;

        register_source_client_args_t() : client(nullptr), event_handler(nullptr), control_handle() {}
    };

    class DeferredApplication {
    public:
        DeferredApplication(AudioMixer *mixer);
        ~DeferredApplication();

        bool isValid() const noexcept { return (mixer_ != nullptr); }

    private:
        AudioMixer *mixer_;
    };

    AudioMixer();
    ~AudioMixer();

    int initialize(const initialize_args_t &args) noexcept;

    int start(bool suspended = false) noexcept;
    int stop() noexcept;
    int suspend() noexcept;
    int resume() noexcept;

    int attachOrUpdateSourcePipe(const attach_update_source_args_t &args, DeferredApplication *da = nullptr) noexcept;
    int detachSourcePipe(AudioSourceDataPipe *pipe, DeferredApplication *da = nullptr) noexcept;

    int apply() noexcept;

    bool isPollingRequired() const noexcept;
    int poll() noexcept;

    state_t getState() const noexcept;

    bool canSuspend() const noexcept;

    AudioCaptureDataPipe *getCapturePipe() const noexcept;

    int registerSourceClient(register_source_client_args_t &args) noexcept;
    int unregisterSourceClient(const source_client_handle_t &control_handle) noexcept;
    int setVolume(const source_client_handle_t &control_handle, float leftVolume, float rightVolume) noexcept;
    int setLooping(const source_client_handle_t &control_handle, bool looping) noexcept;

    int setAudioCaptureEnabled(bool enabled) noexcept;

    int setGlobalPreMixVolumeLevel(float level) noexcept;

    int getSinkPullListenerCallback(void (**ppfunc)(void *), void **pargs) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOMIXER_HPP_
