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

// #define LOG_TAG "AudioMixer"
// #define NB_LOG_TAG "NB AudioMixer"

#include "oslmp/impl/AudioMixer.hpp"

#include <cassert>

#ifdef LOG_TAG
#include <string>
#endif

#include <algorithm>

#include <cxxporthelper/memory>
#include <cxxporthelper/time.hpp>

#include <lockfree/lockfree_circulation_buffer.hpp>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerContext.hpp>
#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/AudioDataPipeManager.hpp"
#include "oslmp/impl/MixingUnit.hpp"
#include "oslmp/impl/StereoVolumeDataPipe.hpp"
#include "oslmp/impl/AndroidHelper.hpp"
#include "oslmp/impl/MixedOutputAudioEffect.hpp"
#include "oslmp/utils/pthread_utils.hpp"
#include "oslmp/utils/bitmap_looper.hpp"
#include "oslmp/utils/timespec_utils.hpp"

#define AUDIO_SOURCE_SET_QUEUE_SIZE 64
#define NUM_AUDIO_SOURCE_SET_QUEUE_ITEMS_PER_THREAD ((AUDIO_SOURCE_SET_QUEUE_SIZE / 2) - 1)

// [b31:b8] : Check code, [b7:b0] : Index
#define SOURCE_CLIENT_CONTROL_HANDLE_PATTERN 0xC3A76A00UL
#define VERIFY_CALLING_CONTEXT_IS_NORMAL() assert(calling_context_ == CALLING_CONTEXT_NORMAL)

#ifdef USE_OSLMP_DEBUG_FEATURES
#define MIXER_TRACE_ACTIVE()    ATRACE_COUNTER("AudioMixerState", 1)
#define MIXER_TRACE_INACTIVE()    ATRACE_COUNTER("AudioMixerState", 0)
#else
#define MIXER_TRACE_ACTIVE()
#define MIXER_TRACE_INACTIVE()
#endif

namespace oslmp {
namespace impl {

struct MixerSourceClient;

enum item_owner_t { OWNER_REQUEST_THREAD, OWNER_MIXER_THREAD, };

enum calling_context_t {
    CALLING_CONTEXT_NORMAL,
    CALLING_CONTEXT_FROM_ON_MIXING_STARTED_EVENT,
    CALLING_CONTEXT_FROM_ON_MIXING_STOPPED_EVENT,
};

enum mixer_thread_control_flags {
    CONTROL_FLAG_REQUEST_STOP = (1 << 0),
    CONTROL_FLAG_REQUEST_SUSPENDED_STATE = (1 << 1),
    CONTROL_FLAG_STATUS_INITIALIZED = (1 << 16),
    CONTROL_FLAG_STATUS_RUNNING = (1 << 17),
    CONTROL_FLAG_STATUS_SUSPENDED = (1 << 18),
};

enum source_slot_state_t {
    SOURCE_SLOT_STATE_NOP,
    SOURCE_SLOT_STATE_UNUSED,
    SOURCE_SLOT_STATE_READY,
    SOURCE_SLOT_STATE_STARTED_BY_EXPLICIT_OPERATION,
    SOURCE_SLOT_STATE_STARTED_NO_LOOP_TRIGGERD,
    SOURCE_SLOT_STATE_STARTED_LOOP_TRIGGERED,
    SOURCE_SLOT_STATE_STOPPED_BY_EXPLICIT_OPERATION,
    SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_NO_TRIGGERED,
    SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_NO_LOOPING_SOURCE,
    SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_LOOPING_SOURCE,
    SOURCE_SLOT_STATE_STOPPED_BY_FADED_OUT,
};

typedef std::atomic<uint32_t> control_flags_t;

struct AudioSourceSlot {
    // handle
    AudioMixer::source_client_handle_t client_handle;

    // handle index
    uint32_t handle_index;

    // client
    MixerSourceClient *source_client;

    // operation
    AudioMixer::operation_t operation;

    // source
    AudioSourceDataPipe *source_pipe;
    uint32_t source_no;

    // mixing
    AudioMixer::mixing_mode_t mix_mode;
    float mix_phase;
    bool mix_phase_override;

    // trigger (for no looping)
    AudioMixer::trigger_mode_t trigger_no_loop_mode;
    AudioMixer::source_client_handle_t trigger_no_loop_target;
    uint32_t trigger_no_loop_source_no;

    // trigger (for looping)
    AudioMixer::trigger_mode_t trigger_loop_mode;
    AudioMixer::source_client_handle_t trigger_loop_target;
    uint32_t trigger_loop_source_no;

    // stop
    uint32_t stop_cond;

    enum source_slot_state_t state;

    AudioSourceSlot()
        : client_handle(), handle_index(0), source_client(nullptr), operation(AudioMixer::OPERATION_NONE),
          source_pipe(nullptr), source_no(0), mix_mode(AudioMixer::MIX_MODE_MUTE), mix_phase(0.0f),
          mix_phase_override(false), trigger_no_loop_mode(AudioMixer::TRIGGER_NONE), trigger_no_loop_target(),
          trigger_no_loop_source_no(0), trigger_loop_mode(AudioMixer::TRIGGER_NONE), trigger_loop_target(),
          trigger_loop_source_no(0), stop_cond(AudioMixer::STOP_NONE), state(SOURCE_SLOT_STATE_UNUSED)
    {
    }

    AudioSourceSlot &clear() noexcept
    {
        client_handle = AudioMixer::source_client_handle_t();
        handle_index = 0;
        source_client = nullptr;
        operation = AudioMixer::OPERATION_NONE;
        source_pipe = nullptr;
        source_no = 0;
        mix_mode = AudioMixer::MIX_MODE_MUTE;
        mix_phase = 0.0f;
        mix_phase_override = false;
        trigger_no_loop_mode = AudioMixer::TRIGGER_NONE;
        trigger_no_loop_target = AudioMixer::source_client_handle_t();
        trigger_no_loop_source_no = 0;
        trigger_loop_mode = AudioMixer::TRIGGER_NONE;
        trigger_loop_target = AudioMixer::source_client_handle_t();
        trigger_loop_source_no = 0;
        stop_cond = AudioMixer::STOP_NONE;
        state = SOURCE_SLOT_STATE_UNUSED;

        return (*this);
    }
};

struct AudioSinkSlot {
    AudioSinkDataPipe *pipe;

    AudioSinkSlot() : pipe(nullptr) {}
};

struct AudioSourceSet {
    const item_owner_t owner;
    uint32_t updated_bitmap;
    AudioSourceSlot slots[AudioMixer::NUM_MAX_SOURCE_PIPES];

    AudioSourceSet() : owner(OWNER_REQUEST_THREAD), updated_bitmap(0) {}

    AudioSourceSet &init_for_request_thread() noexcept
    {
        const_cast<item_owner_t &>(owner) = OWNER_REQUEST_THREAD;
        return clear();
    }

    AudioSourceSet &init_for_mixer_thread() noexcept
    {
        const_cast<item_owner_t &>(owner) = OWNER_MIXER_THREAD;
        return clear();
    }

    AudioSourceSet &clear() noexcept
    {
        for (auto &slot : slots) {
            slot.clear();
        }
        updated_bitmap = 0;

        return (*this);
    }

    AudioSourceSet &copy(const AudioSourceSet &src) noexcept
    {
        // NOTE: keeo the 'owner' field
        for (int i = 0; i < AudioMixer::NUM_MAX_SOURCE_PIPES; ++i) {
            slots[i] = src.slots[i];
        }
        updated_bitmap = src.updated_bitmap;

        return (*this);
    }

    int find(AudioSourceDataPipe *pipe) const noexcept
    {
        for (int i = 0; i < AudioMixer::NUM_MAX_SOURCE_PIPES; ++i) {
            if (slots[i].source_pipe == pipe) {
                return i;
            }
        }
        return -1;
    }

    int find_free() const noexcept
    {
        for (int i = 0; i < AudioMixer::NUM_MAX_SOURCE_PIPES; ++i) {
            if (!(slots[i].source_pipe)) {
                return i;
            }
        }
        return -1;
    }

    void mark_updated(int index) noexcept { updated_bitmap |= (1U << static_cast<uint32_t>(index)); }

    void unmark_updated(int index) noexcept { updated_bitmap &= ~(1U << static_cast<uint32_t>(index)); }
};

struct MixerSourceClient {
    AudioMixer::source_client_handle_t handle;
    AudioMixer::SourceClientEventHandler *event_handler;
    StereoVolumeDataPipe pipe;

    // for request thread
    float requested_volume_left;
    float requested_volume_right;
    bool modified;

    // for mixer thread
    float actual_volume_left;
    float actual_volume_right;

    MixerSourceClient()
        : handle(), event_handler(nullptr), pipe(), requested_volume_left(1.0f), requested_volume_right(1.0f),
          modified(false), actual_volume_left(1.0f), actual_volume_right(1.0f)
    {
    }

    int initialize() noexcept
    {
        clearFields();
        return pipe.initialize();
    }

    int reset() noexcept
    {
        clearFields();
        return pipe.reset();
    }

    void forceSynch() noexcept
    {
        modified = false;
        actual_volume_left = requested_volume_left;
        actual_volume_right = requested_volume_right;
        pipe.reset();
    }

    bool requestThreadApply() noexcept
    {
        StereoVolumeDataPipe::write_block_t wb;

        if (!pipe.lockWrite(wb)) {
            return false;
        }

        wb.volume_left = requested_volume_left;
        wb.volume_right = requested_volume_right;

        pipe.unlockWrite(wb);

        modified = false;

        return true;
    }

    bool mixerThreadUpdate() noexcept
    {
        StereoVolumeDataPipe::read_block_t rb;

        if (!pipe.lockRead(rb)) {
            return false;
        }

        actual_volume_left = rb.volume_left;
        actual_volume_right = rb.volume_right;

        pipe.unlockRead(rb);

        return true;
    }

private:
    void clearFields() noexcept
    {
        handle = AudioMixer::source_client_handle_t();
        event_handler = nullptr;
        requested_volume_left = 1.0f;
        requested_volume_right = 1.0f;
        modified = false;
        actual_volume_left = 1.0f;
        actual_volume_right = 1.0f;
    }
};

enum {
    FLAG_NONE = 0,
    FLAG_DETACH_REQUESTED = 1,
    FLAG_STARTED = 2,
    FLAG_END_OF_DATA_DETECTED = 4,
    FLAG_END_OF_DATA_WITH_LOOPING_ENABLED = 8,
    FLAG_TRIGGER_FIRED = 16,
};

union u32_float_t {
    uint32_t u32;
    float f;
};

struct MixerThreadContext {
    OpenSLMediaPlayerInternalContext *oslmp_context;

    // source
    AudioSourceSet currnt_src_set;
    uint32_t flags[AudioMixer::NUM_MAX_SOURCE_PIPES];
    MixingUnit::Context mixer_unit_context[AudioMixer::NUM_MAX_SOURCE_PIPES];

    // sink
    AudioSinkSlot sink_slot;

    uint32_t attached_bitmap;
    uint32_t started_bitmap;
    uint32_t detach_requested_bitmap;
    uint32_t looping_bitmap;

    uint32_t max_process_block_at_once;

    MixedOutputAudioEffect *mixout_effects[AudioMixer::NUM_MAX_MIXOOUT_EFFECTS];
    uint32_t num_mixout_effects;

    float global_premix_level;

#ifdef USE_OSLMP_DEBUG_FEATURES
    std::unique_ptr<NonBlockingTraceLoggerClient> nb_logger;
#endif

    MixerThreadContext()
        : oslmp_context(nullptr), attached_bitmap(0U), started_bitmap(0U), detach_requested_bitmap(0U),
          looping_bitmap(0U), max_process_block_at_once(0U), num_mixout_effects(0U), global_premix_level(0.0f)
    {
        currnt_src_set.init_for_mixer_thread();

        for (auto &f : flags) {
            f = FLAG_NONE;
        }
        for (auto &e : mixout_effects) {
            e = nullptr;
        }
    }
};

typedef lockfree::lockfree_circulation_buffer<AudioSourceSet *, AUDIO_SOURCE_SET_QUEUE_SIZE> audio_src_set_queue_t;

class AudioMixer::Impl {
public:
    Impl();
    ~Impl();

    int initialize(const initialize_args_t &args) noexcept;

    int start(bool suspended) noexcept;
    int stop() noexcept;
    int suspend() noexcept;
    int resume() noexcept;

    int attachOrUpdateSourcePipe(const AudioMixer::attach_update_source_args_t &args,
                                 AudioMixer::DeferredApplication *da) noexcept;
    int detachSourcePipe(AudioSourceDataPipe *pipe, AudioMixer::DeferredApplication *da) noexcept;

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

#ifdef LOG_TAG
    void dump_request_thread_source_source_slot_usage() noexcept;
    void dump_mixer_thread_source_source_slot_usage(const MixerThreadContext &c) noexcept;
#endif

private:
    void release() noexcept;

    bool requestThreadApplyAudioSourceSetItems() noexcept;
    bool requestThreadReceiveOneAudioSourceSetItem() noexcept;
    int requestThreadRecycleAllAudioSourceSetItems() noexcept;
    void requestThreadApplyAllVolumes() noexcept;
    void requestThreadRaiseOnMixingStarted(const AudioSourceSlot &slot, source_slot_state_t next_state) noexcept;
    void requestThreadRaiseOnMixingStopped(const AudioSourceSlot &slot, source_slot_state_t next_state) noexcept;
    void requestThreadHandleOnMixingStateChanged(const AudioSourceSlot &slot, source_slot_state_t cur_state,
                                                 source_slot_state_t next_state) noexcept;

    static void *mixerThreadEntry(void *args) noexcept;
    void mixerThreadProcess(void) noexcept;

    bool mixerThreadUpdateAudioSourceSets(MixerThreadContext &c, int max_iterate) noexcept;
    bool mixerThreadUpdateAudioSourceSetOne(MixerThreadContext &c) noexcept;
    bool mixerThreadNotifyCurrentAudioSourceSets(MixerThreadContext &c) noexcept;
    void mixerThreadInitializeSource(MixerThreadContext &c, int index, const AudioSourceSlot &new_slot) noexcept;
    bool mixerThreadCheckStartConditions(MixerThreadContext &c) noexcept;
    bool mixerThreadCheckStopConditions(MixerThreadContext &c) noexcept;
    void mixerThreadCleanUpCurrentSourceSet(MixerThreadContext &c) noexcept;
    bool mixerThreadHandleAudioDataBlocks(MixerThreadContext &c) noexcept;
    void mixerThreadHandleNonAudioDataBlocks(MixerThreadContext &c) noexcept;
    void mixerThreadUpdateGlobalPreMixLevel(MixerThreadContext &c) noexcept;
    bool mixerThreadUpdateMixVolumes(MixerThreadContext &c) noexcept;
    void mixerThreadUpdateLoopingBitmap(MixerThreadContext &c) noexcept;
    void mixerThreadPollMixOutEffects(MixerThreadContext &c) noexcept;

    bool isInitialized() const noexcept;
    bool isStarted() const noexcept;
    bool isStopped() const noexcept;

    static int isFadeIn(MixingUnit::mode_t mode) noexcept;
    static int isFadeOut(MixingUnit::mode_t mode) noexcept;
    static int getMixingDirection(MixingUnit::mode_t mode) noexcept;
    static float calcMixingPhase(MixingUnit::mode_t cur_mode, MixingUnit::mode_t next_mode, float cur_phase) noexcept;

    static void onSinkPullListenerCallback(void *args);

private:
    OpenSLMediaPlayerInternalContext *context_;

    state_t state_;

    pthread_t thread_;

    // for request thread
    AudioDataPipeManager *pipe_manager_;
    AudioSourceSet requested_source_set_;
    AudioSourceSet active_source_set_;
    AudioSinkSlot sink_slot_;

    // for communication with mixer thread
    AudioSourceSet audio_src_set_request_thread_item_pool_[NUM_AUDIO_SOURCE_SET_QUEUE_ITEMS_PER_THREAD];
    AudioSourceSet audio_src_set_mixer_thread_item_pool_[NUM_AUDIO_SOURCE_SET_QUEUE_ITEMS_PER_THREAD];

    audio_src_set_queue_t audio_src_set_request_thread_free_queue_;
    audio_src_set_queue_t audio_src_set_mixer_thread_free_queue_;
    audio_src_set_queue_t audio_src_set_request_to_mixer_thread_queue_;
    audio_src_set_queue_t audio_src_set_mixer_to_request_thread_queue_;

    MixerSourceClient source_clients_[NUM_MAX_SOURCE_CLIENTS];
    uint32_t source_client_counter_;
    std::atomic<uint32_t> looping_bitmap_;

    control_flags_t mixer_thread_control_flags_;
    utils::pt_mutex mutex_mixer_thread_;
    utils::pt_condition_variable cond_mixer_thread_;

    MixedOutputAudioEffect *mixout_effects_[NUM_MAX_MIXOOUT_EFFECTS];
    uint32_t num_mixout_effects_;

    // for mixer thread
    MixingUnit mixing_unit_;

    // audio data capturing
    AudioCaptureDataPipe *capture_pipe_;
    std::atomic_bool captuing_enabled_;

    std::atomic<uint32_t> u32_global_premix_level_;

    uint32_t sleep_duration_ns_;
    uint32_t max_process_block_at_once_;

    calling_context_t calling_context_;
};

//
// Utilities
//

static inline void clear(MixingUnit::Context &c) noexcept
{
    c.mode = MixingUnit::MODE_MUTE;
    c.phase = 0.0f;
}

static inline uint32_t getSourceClientControlHandleIndex(const AudioMixer::source_client_handle_t &handle) noexcept
{
    return static_cast<uint32_t>(handle.f1_ & 0xFF);
}

static inline bool isValidSourceClientControlHandle(const AudioMixer::source_client_handle_t &handle) noexcept
{
    const uint64_t check_code = handle.f1_ & 0xFFFFFF00UL;
    //  const uint32_t counter = handle.f2_;
    const uint32_t index = static_cast<uint32_t>(handle.f1_ & 0xFF);

    return (check_code == SOURCE_CLIENT_CONTROL_HANDLE_PATTERN) && (index < AudioMixer::NUM_MAX_SOURCE_CLIENTS);
}

static inline uint32_t update_bitmap(uint32_t value, uint32_t mask, bool cond) noexcept
{
    return (cond) ? (value | mask) : (value & ~mask);
}

static inline bool should_suspend(uint32_t flags) noexcept
{
    if (flags & CONTROL_FLAG_REQUEST_STOP)
        return false;
    if (!(flags & CONTROL_FLAG_REQUEST_SUSPENDED_STATE))
        return false;
    return true;
}

static inline bool is_started_state(source_slot_state_t state) noexcept
{
    switch (state) {
    case SOURCE_SLOT_STATE_STARTED_BY_EXPLICIT_OPERATION:
    case SOURCE_SLOT_STATE_STARTED_NO_LOOP_TRIGGERD:
    case SOURCE_SLOT_STATE_STARTED_LOOP_TRIGGERED:
        return true;
    default:
        return false;
    }
}

static inline bool can_transit_to_started_by_explicit_operation(source_slot_state_t cur_state) noexcept
{
    switch (cur_state) {
    case SOURCE_SLOT_STATE_READY:
    case SOURCE_SLOT_STATE_STOPPED_BY_EXPLICIT_OPERATION:
    case SOURCE_SLOT_STATE_STOPPED_BY_FADED_OUT:
        return true;
    case SOURCE_SLOT_STATE_NOP:
    case SOURCE_SLOT_STATE_UNUSED:
    case SOURCE_SLOT_STATE_STARTED_BY_EXPLICIT_OPERATION:
    case SOURCE_SLOT_STATE_STARTED_NO_LOOP_TRIGGERD:
    case SOURCE_SLOT_STATE_STARTED_LOOP_TRIGGERED:
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_NO_TRIGGERED:
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_NO_LOOPING_SOURCE:
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_LOOPING_SOURCE:
    default:
        return false;
    }
}

static inline bool can_transit_to_stopped_by_explicit_operation(source_slot_state_t cur_state) noexcept
{
    switch (cur_state) {
    case SOURCE_SLOT_STATE_STARTED_BY_EXPLICIT_OPERATION:
    case SOURCE_SLOT_STATE_STARTED_NO_LOOP_TRIGGERD:
    case SOURCE_SLOT_STATE_STARTED_LOOP_TRIGGERED:
        return true;
    case SOURCE_SLOT_STATE_NOP:
    case SOURCE_SLOT_STATE_UNUSED:
    case SOURCE_SLOT_STATE_READY:
    case SOURCE_SLOT_STATE_STOPPED_BY_EXPLICIT_OPERATION:
    case SOURCE_SLOT_STATE_STOPPED_BY_FADED_OUT:
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_NO_TRIGGERED:
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_NO_LOOPING_SOURCE:
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_LOOPING_SOURCE:
    default:
        return false;
    }
}

static inline audio_src_set_queue_t::data_type obtain_and_set_nullptr(audio_src_set_queue_t &queue,
                                                                      audio_src_set_queue_t::index_t index) noexcept
{
    audio_src_set_queue_t::data_type item = queue.at(index);
    queue.at(index) = nullptr;
    return item;
}

static inline float u32_to_float(const uint32_t &x) noexcept
{
    u32_float_t t;
    t.u32 = x;
    return t.f;
}

static inline uint32_t float_to_u32(const float &x) noexcept
{
    u32_float_t t;
    t.f = x;
    return t.u32;
}

//
// AudioMixer::Transaction
//

AudioMixer::DeferredApplication::DeferredApplication(AudioMixer *mixer) : mixer_(mixer) {}

AudioMixer::DeferredApplication::~DeferredApplication()
{
    if (mixer_) {
        mixer_->apply();
        mixer_ = nullptr;
    }
}

//
// AudioMixer
//
AudioMixer::AudioMixer() : impl_(new (std::nothrow) Impl()) {}

AudioMixer::~AudioMixer() {}

int AudioMixer::initialize(const AudioMixer::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioMixer::start(bool suspended) noexcept
{
    int result;

    if (!impl_)
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->start(suspended);

    LOGD("start(suspended = %d) - result = %d", suspended, result);

    return result;
}

int AudioMixer::stop() noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->stop();

    LOGD("stop() - result = %d", result);

    return result;
}

int AudioMixer::suspend() noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->suspend();

    LOGD("suspend() - result = %d", result);

    return result;
}

int AudioMixer::resume() noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->resume();

    LOGD("resume() - result = %d", result);

    return result;
}

int AudioMixer::attachOrUpdateSourcePipe(const AudioMixer::attach_update_source_args_t &args,
                                         AudioMixer::DeferredApplication *da) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->attachOrUpdateSourcePipe(args, da);

    LOGD("attachOrUpdateSourcePipe(\n"
         "{handle = %p, operation = %d, source_pipe = %p, source_no = %d, mix_mode = %d, mix_phase = %f, "
         "mix_phase_override = %d,\n"
         " trigger_no_loop = (%d, %p, %d), trigger_loop = (%d, %p, %d), stop_cond = %x}, da = %d) - result = %d",
         reinterpret_cast<void *>(args.handle.f1_), args.operation, args.source_pipe, args.source_no, args.mix_mode,
         args.mix_phase, args.mix_phase_override, args.trigger_no_loop_mode,
         reinterpret_cast<void *>(args.trigger_no_loop_target.f1_), args.trigger_no_loop_source_no,
         args.trigger_loop_mode, reinterpret_cast<void *>(args.trigger_loop_target.f1_), args.trigger_loop_source_no,
         args.stop_cond, (da ? 1 : 0), result);

    return result;
}

int AudioMixer::detachSourcePipe(AudioSourceDataPipe *pipe, AudioMixer::DeferredApplication *da) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->detachSourcePipe(pipe, da);

    LOGD("detachSourcePipe(pipe = %p, da = %d) - result = %d", pipe, (da ? 1 : 0), result);

    return result;
}

int AudioMixer::apply() noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->apply();

    LOGD("apply() - result = %d", result);

    return result;
}

bool AudioMixer::isPollingRequired() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isPollingRequired();
}

int AudioMixer::poll() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->poll();
}

AudioMixer::state_t AudioMixer::getState() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return MIXER_STATE_NOT_INITIALIZED;
    return impl_->getState();
}

bool AudioMixer::canSuspend() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->canSuspend();
}

AudioCaptureDataPipe *AudioMixer::getCapturePipe() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return nullptr;
    return impl_->getCapturePipe();
}

int AudioMixer::registerSourceClient(AudioMixer::register_source_client_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->registerSourceClient(args);
}

int AudioMixer::unregisterSourceClient(const AudioMixer::source_client_handle_t &control_handle) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->unregisterSourceClient(control_handle);
}

int AudioMixer::setVolume(const AudioMixer::source_client_handle_t &control_handle, float leftVolume,
                          float rightVolume) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setVolume(control_handle, leftVolume, rightVolume);
}

int AudioMixer::setLooping(const AudioMixer::source_client_handle_t &control_handle, bool looping) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setLooping(control_handle, looping);
}

int AudioMixer::setAudioCaptureEnabled(bool enabled) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAudioCaptureEnabled(enabled);
}

int AudioMixer::setGlobalPreMixVolumeLevel(float level) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setGlobalPreMixVolumeLevel(level);
}

int AudioMixer::getSinkPullListenerCallback(void (**ppfunc)(void *), void **pargs) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getSinkPullListenerCallback(ppfunc, pargs);
}


//
// AudioMixer::Impl
//
AudioMixer::Impl::Impl()
    : context_(nullptr), state_(MIXER_STATE_NOT_INITIALIZED), thread_(0), pipe_manager_(nullptr),
      requested_source_set_(), sink_slot_(), source_client_counter_(0U), looping_bitmap_(0U),
      mixer_thread_control_flags_(0U), num_mixout_effects_(0), mixing_unit_(), capture_pipe_(nullptr),
      captuing_enabled_(false), u32_global_premix_level_(0U), sleep_duration_ns_(0U), max_process_block_at_once_(0U),
      calling_context_(CALLING_CONTEXT_NORMAL)
{
}

AudioMixer::Impl::~Impl() { release(); }

int AudioMixer::Impl::initialize(const AudioMixer::initialize_args_t &args) noexcept
{
    // check arguments
    if (!args.context)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.pipe_manager)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.sink_pipe)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    // check states
    if (state_ != MIXER_STATE_NOT_INITIALIZED)
        return OSLMP_RESULT_ILLEGAL_STATE;

    // initialize free/request/recycle queues
    audio_src_set_request_thread_free_queue_.clear();
    audio_src_set_mixer_thread_free_queue_.clear();
    audio_src_set_request_to_mixer_thread_queue_.clear();
    audio_src_set_mixer_to_request_thread_queue_.clear();

    const uint32_t block_size = args.pipe_manager->getBlockSizeInFrames();

    {
        audio_src_set_queue_t &queue = audio_src_set_request_thread_free_queue_;

        for (auto &item : audio_src_set_request_thread_item_pool_) {
            audio_src_set_queue_t::index_t write_index = audio_src_set_queue_t::INVALID_INDEX;
            if (queue.lock_write(write_index)) {
                queue.at(write_index) = &(item.init_for_request_thread());
                queue.unlock_write(write_index);
            } else {
                return OSLMP_RESULT_INTERNAL_ERROR;
            }
        }
    }

    {
        audio_src_set_queue_t &queue = audio_src_set_mixer_thread_free_queue_;

        for (auto &item : audio_src_set_mixer_thread_item_pool_) {
            audio_src_set_queue_t::index_t write_index = audio_src_set_queue_t::INVALID_INDEX;
            if (queue.lock_write(write_index)) {
                queue.at(write_index) = &(item.init_for_mixer_thread());
                queue.unlock_write(write_index);
            } else {
                return OSLMP_RESULT_INTERNAL_ERROR;
            }
        }
    }

    // initialize source clients
    {
        for (auto &client : source_clients_) {
            int result = client.initialize();
            if (result != OSLMP_RESULT_SUCCESS) {
                return result;
            }
        }
    }

    // initialize mixing unit
    {
        MixingUnit::initialize_args_t init_args;

        init_args.block_size_in_frames = block_size;
        init_args.sampling_rate = args.sampling_rate;
        init_args.short_fade_duration_ms = args.short_fade_duration_ms;
        init_args.long_fade_duration_ms = args.long_fade_duration_ms;

        if (!mixing_unit_.initialize(init_args)) {
            return OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
        }
    }

    // set sink pipe user
    {
        int result = args.pipe_manager->setSinkPipeInPortUser(args.sink_pipe, this, true);
        if (result != OSLMP_RESULT_SUCCESS) {
            return result;
        }
    }

    // set capture pipe user
    {
        int result = args.pipe_manager->setCapturePipeInPortUser(args.capture_pipe, this, true);
        if (result != OSLMP_RESULT_SUCCESS) {
            (void)args.pipe_manager->setSinkPipeInPortUser(args.sink_pipe, this, false);
            return result;
        }
    }

    uint32_t sleep_duration_ns;
    uint32_t max_process_block_at_once;

    {
        const uint32_t sink_player_num_blocks = args.num_sink_player_blocks;
        const uint32_t sink_pipe_num_blocks = args.sink_pipe->getNumberOfBufferItems();
        const uint32_t sink_buffer_size_us = static_cast<uint32_t>((block_size * sink_player_num_blocks * 1000000ull) / (args.sampling_rate / 1000));

        sleep_duration_ns = static_cast<uint32_t>((sink_buffer_size_us * 1000) * 1.5f);
        max_process_block_at_once = (std::min)((sink_player_num_blocks * 2), sink_pipe_num_blocks);
    }

    // update fields
    context_ = args.context;
    pipe_manager_ = args.pipe_manager;
    sink_slot_.pipe = args.sink_pipe;
    capture_pipe_ = args.capture_pipe;
    thread_ = 0;
    u32_global_premix_level_ = float_to_u32(1.0f);
    mixer_thread_control_flags_ = 0;
    looping_bitmap_ = 0;
    sleep_duration_ns_ = sleep_duration_ns;
    max_process_block_at_once_ = max_process_block_at_once;

    num_mixout_effects_ = 0;
    for (int i = 0; i < NUM_MAX_MIXOOUT_EFFECTS; ++i) {
        if (args.mixout_effects[i]) {
            mixout_effects_[num_mixout_effects_] = args.mixout_effects[i];
            num_mixout_effects_ += 1;
        }
    }

    state_ = MIXER_STATE_STOPPED;

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::start(bool suspended) noexcept
{
    control_flags_t &ref_ctrl_flg = mixer_thread_control_flags_;

    // check states
    if (!isInitialized())
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (isStarted())
        return OSLMP_RESULT_SUCCESS;

    if (!isStopped())
        return OSLMP_RESULT_ILLEGAL_STATE;

    // prepare
    ref_ctrl_flg.store((suspended) ? CONTROL_FLAG_REQUEST_SUSPENDED_STATE : 0, std::memory_order_release);

    for (auto &client : source_clients_) {
        client.forceSynch();
    }

    // start thread
    int s = ::pthread_create(&thread_, nullptr, mixerThreadEntry, this);
    if (s != 0) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    // wait for initialized
    {
        utils::pt_unique_lock lock(mutex_mixer_thread_);

        while (!(ref_ctrl_flg.load(std::memory_order_acquire) & CONTROL_FLAG_STATUS_INITIALIZED)) {
            cond_mixer_thread_.wait(lock);
        }
    }

    // set current state
    state_ = (suspended) ? MIXER_STATE_SUSPENDED : MIXER_STATE_STARTED;

    // update fields
    active_source_set_.copy(requested_source_set_);

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::stop() noexcept
{
    void *thread_retval = nullptr;

    if (!isInitialized())
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (state_ == MIXER_STATE_STOPPED)
        return OSLMP_RESULT_SUCCESS;

    if (!isStarted())
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (thread_) {
        // set CONTROL_FLAG_REQUEST_STOP flag
        {
            utils::pt_unique_lock lock(mutex_mixer_thread_);
            control_flags_t &ref_ctrl_flg = mixer_thread_control_flags_;
            ref_ctrl_flg.fetch_or(CONTROL_FLAG_REQUEST_STOP, std::memory_order_release);
            cond_mixer_thread_.notify_one();
        }

        // join
        const int join_result = ::pthread_join(thread_, &thread_retval);
        assert(join_result == 0);
        thread_ = 0;
    }

    state_ = MIXER_STATE_STOPPED;

    // recycle all source set items
    requestThreadRecycleAllAudioSourceSetItems();

    // update fields
    active_source_set_.copy(requested_source_set_);

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::suspend() noexcept
{
    if (CXXPH_UNLIKELY(state_ == MIXER_STATE_SUSPENDED))
        return OSLMP_RESULT_SUCCESS;

    if (CXXPH_UNLIKELY(state_ != MIXER_STATE_STARTED))
        return OSLMP_RESULT_ILLEGAL_STATE;

    control_flags_t &ref_ctrl_flg = mixer_thread_control_flags_;

    {
        utils::pt_unique_lock lock(mutex_mixer_thread_);
        ref_ctrl_flg.fetch_or(CONTROL_FLAG_REQUEST_SUSPENDED_STATE, std::memory_order_release);
        cond_mixer_thread_.notify_one();

        state_ = MIXER_STATE_SUSPENDED;
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::resume() noexcept
{
    if (CXXPH_UNLIKELY(state_ == MIXER_STATE_STARTED))
        return OSLMP_RESULT_SUCCESS;

    if (CXXPH_UNLIKELY(state_ != MIXER_STATE_SUSPENDED))
        return OSLMP_RESULT_ILLEGAL_STATE;

    control_flags_t &ref_ctrl_flg = mixer_thread_control_flags_;

    {
        utils::pt_unique_lock lock(mutex_mixer_thread_);
        ref_ctrl_flg.fetch_and(~CONTROL_FLAG_REQUEST_SUSPENDED_STATE, std::memory_order_release);
        cond_mixer_thread_.notify_one();

        state_ = MIXER_STATE_STARTED;
    }

    return OSLMP_RESULT_SUCCESS;
}

void AudioMixer::Impl::release() noexcept
{
    stop();

    if (pipe_manager_ && sink_slot_.pipe) {
        (void)pipe_manager_->setSinkPipeInPortUser(sink_slot_.pipe, this, false);
        sink_slot_.pipe = nullptr;
    }

    if (pipe_manager_ && capture_pipe_) {
        (void)pipe_manager_->setCapturePipeInPortUser(capture_pipe_, this, false);
        capture_pipe_ = nullptr;
    }
}

int AudioMixer::Impl::attachOrUpdateSourcePipe(const AudioMixer::attach_update_source_args_t &args,
                                               AudioMixer::DeferredApplication *da) noexcept
{

    // check parameters
    if (CXXPH_UNLIKELY(!isValidSourceClientControlHandle(args.handle))) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (CXXPH_UNLIKELY(args.operation == OPERATION_DETACH)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (CXXPH_UNLIKELY(!args.source_pipe)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (CXXPH_UNLIKELY(args.stop_cond & ~(STOP_AFTER_FADE_OUT | STOP_ON_PLAYBACK_END))) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (CXXPH_UNLIKELY(!isValidSourceClientControlHandle(args.handle))) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    // check current state
    if (CXXPH_UNLIKELY(!isInitialized())) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    // find free slot
    int index;
    bool attach;

    {
        const int matched_index = requested_source_set_.find(args.source_pipe);
        const int free_index = requested_source_set_.find_free();

        if (matched_index >= 0) {
            index = matched_index;
            attach = false;
        } else if (free_index >= 0) {
            index = free_index;
            attach = true;
        } else {
            return OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
        }
    }

    int result;
    AudioSourceSlot &slot = requested_source_set_.slots[index];

    if (attach) {
        result = pipe_manager_->setSourcePipeOutPortUser(args.source_pipe, this, true);
    } else {
        if (slot.operation == OPERATION_DETACH) {
            // already detaching requested
            result = OSLMP_RESULT_ILLEGAL_STATE;
        } else {
            result = OSLMP_RESULT_SUCCESS;
        }
    }

    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    //  update slot info

    slot.client_handle = args.handle;
    slot.handle_index = getSourceClientControlHandleIndex(args.handle);
    slot.source_client = &source_clients_[slot.handle_index];
    slot.operation = args.operation;
    slot.mix_mode = args.mix_mode;
    slot.mix_phase = args.mix_phase;
    slot.mix_phase_override = args.mix_phase_override;
    slot.source_pipe = args.source_pipe;
    slot.source_no = args.source_no;
    slot.trigger_no_loop_mode = args.trigger_no_loop_mode;
    slot.trigger_no_loop_target = args.trigger_no_loop_target;
    slot.trigger_no_loop_source_no = args.trigger_no_loop_source_no;
    slot.trigger_loop_mode = args.trigger_loop_mode;
    slot.trigger_loop_target = args.trigger_loop_target;
    slot.trigger_loop_source_no = args.trigger_loop_source_no;
    slot.stop_cond = args.stop_cond;
    slot.state = SOURCE_SLOT_STATE_NOP;

    requested_source_set_.mark_updated(index);

    if (attach) {
        LOGD("[ReqThread (%d)] ATTACH REQ: slot[%d] pipe = %p", state_, index,
             requested_source_set_.slots[index].source_pipe);
#ifdef LOG_TAG
        dump_request_thread_source_source_slot_usage();
#endif
    }

    if (isStarted() && !da) {
        requestThreadApplyAudioSourceSetItems();
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::detachSourcePipe(AudioSourceDataPipe *pipe, AudioMixer::DeferredApplication *da) noexcept
{
    // check parameters
    if (CXXPH_UNLIKELY(!pipe))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    // check state
    if (CXXPH_UNLIKELY(!isInitialized()))
        return OSLMP_RESULT_ILLEGAL_STATE;

    const int matched_index = requested_source_set_.find(pipe);

    if (CXXPH_UNLIKELY(matched_index < 0)) {
        // not attached
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    AudioSourceSlot &slot = requested_source_set_.slots[matched_index];

    if (slot.operation == OPERATION_DETACH)
        return OSLMP_RESULT_SUCCESS;

    slot.operation = OPERATION_DETACH;

    requested_source_set_.mark_updated(matched_index);

    LOGD("[ReqThread (%d)] DETACH REQ: slot[%d] pipe = %p", state_, matched_index,
         requested_source_set_.slots[matched_index].source_pipe);
#ifdef LOG_TAG
    dump_request_thread_source_source_slot_usage();
#endif

    if (isStarted() && !da) {
        requestThreadApplyAudioSourceSetItems();
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::apply() noexcept
{
    if (CXXPH_UNLIKELY(!isInitialized()))
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (CXXPH_LIKELY(isStarted())) {
        requestThreadApplyAudioSourceSetItems();
    }

    return OSLMP_RESULT_SUCCESS;
}

bool AudioMixer::Impl::isPollingRequired() const noexcept
{
    if (!canSuspend()) {
        return true;
    }

    for (int i = 0; i < num_mixout_effects_; ++i) {
        if ((mixout_effects_[i])->isPollingRequired()) {
            return true;
        }
    }

    return false;
}

int AudioMixer::Impl::poll() noexcept
{
    if (CXXPH_UNLIKELY(!isInitialized()))
        return OSLMP_RESULT_ILLEGAL_STATE;

    if (CXXPH_LIKELY(isStarted())) {
        requestThreadRecycleAllAudioSourceSetItems();

        requestThreadApplyAudioSourceSetItems();

        requestThreadApplyAllVolumes();
    }

    for (int i = 0; i < num_mixout_effects_; ++i) {
        MixedOutputAudioEffect &effect = *(mixout_effects_[i]);
        if (effect.isPollingRequired()) {
            effect.poll();
        }
    }

    if (state_ == MIXER_STATE_SUSPENDED) {
        // temporary wakeup mixer thread to call MixedOutputAudioEffect::pollFromMixerThread()
        utils::pt_unique_lock lock(mutex_mixer_thread_);
        cond_mixer_thread_.notify_one();
    }

    return OSLMP_RESULT_SUCCESS;
}

AudioMixer::state_t AudioMixer::Impl::getState() const noexcept { return state_; }

bool AudioMixer::Impl::canSuspend() const noexcept
{
    uint32_t bitmap_act_started = 0;
    const bool is_req_to_mix_request_no_pending =
        (audio_src_set_request_thread_free_queue_.size() == NUM_AUDIO_SOURCE_SET_QUEUE_ITEMS_PER_THREAD);
    const bool is_mix_to_req_request_no_pending =
        (audio_src_set_mixer_thread_free_queue_.size() == NUM_AUDIO_SOURCE_SET_QUEUE_ITEMS_PER_THREAD);

    for (int i = 0; i < AudioMixer::NUM_MAX_SOURCE_PIPES; ++i) {
        const uint32_t index_mask = (1U << i);
        const AudioSourceSlot &act_slot = active_source_set_.slots[i];
        const AudioSourceSlot &req_slot = requested_source_set_.slots[i];

        if (is_started_state(act_slot.state)) {
            bitmap_act_started |= index_mask;
        }
    }

    return (bitmap_act_started == 0) && is_req_to_mix_request_no_pending && is_mix_to_req_request_no_pending;
}

AudioCaptureDataPipe *AudioMixer::Impl::getCapturePipe() const noexcept { return capture_pipe_; }

int AudioMixer::Impl::registerSourceClient(AudioMixer::register_source_client_args_t &args) noexcept
{
    // clear results
    args.control_handle.f1_ = 0;
    args.control_handle.f2_ = 0;
    args.control_handle.f3_ = 0;

    // check parameters
    if (CXXPH_UNLIKELY(!args.client)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
    if (CXXPH_UNLIKELY(!args.event_handler)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    // check current state
    if (state_ == MIXER_STATE_NOT_INITIALIZED) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    int matched_index = -1;
    int free_index = -1;

    for (int i = 0; i < NUM_MAX_SOURCE_CLIENTS; ++i) {
        auto &client = source_clients_[i];
        if (client.handle.f3_ == reinterpret_cast<uintptr_t>(args.client)) {
            matched_index = i;
            break;
        } else if (!(client.handle.f3_) && (free_index < 0)) {
            free_index = i;
        }
    }

    if (CXXPH_UNLIKELY(matched_index >= 0)) {
        // already registered
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    if (CXXPH_UNLIKELY(free_index < 0)) {
        // no free controller available
        return OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
    }

    MixerSourceClient &controller = source_clients_[free_index];

    int result = controller.reset();

    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS)) {
        return result;
    }

    uint32_t counter = source_client_counter_;
    source_client_counter_ += 1;

    args.control_handle.f1_ = static_cast<uint32_t>(SOURCE_CLIENT_CONTROL_HANDLE_PATTERN | free_index);
    args.control_handle.f2_ = counter;
    args.control_handle.f3_ = reinterpret_cast<uintptr_t>(args.client);

    controller.handle = args.control_handle;
    controller.event_handler = args.event_handler;

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::unregisterSourceClient(const AudioMixer::source_client_handle_t &control_handle) noexcept
{
    // check parameters
    if (CXXPH_UNLIKELY(!isValidSourceClientControlHandle(control_handle))) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (CXXPH_UNLIKELY(state_ == MIXER_STATE_NOT_INITIALIZED)) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    const uint32_t index = getSourceClientControlHandleIndex(control_handle);
    if (source_clients_[index].handle != control_handle) {
        // not registered
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    (void)source_clients_[index].reset();

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::setVolume(const AudioMixer::source_client_handle_t &control_handle, float leftVolume,
                                float rightVolume) noexcept
{

    // check parameters
    const uint32_t index = getSourceClientControlHandleIndex(control_handle);

    if (CXXPH_UNLIKELY(index >= NUM_MAX_SOURCE_CLIENTS)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (CXXPH_UNLIKELY(state_ == MIXER_STATE_NOT_INITIALIZED)) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    MixerSourceClient &client = source_clients_[index];

    if (CXXPH_UNLIKELY(client.handle != control_handle)) {
        // client is not registered
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    // clipping
    leftVolume = (std::min)((std::max)(leftVolume, 0.0f), 1.0f);
    rightVolume = (std::min)((std::max)(rightVolume, 0.0f), 1.0f);

    if (client.requested_volume_left == leftVolume && client.requested_volume_right == rightVolume) {
        return OSLMP_RESULT_SUCCESS;
    }

    // update
    client.requested_volume_left = leftVolume;
    client.requested_volume_right = rightVolume;
    client.modified = true;

    if (CXXPH_LIKELY(isStarted())) {
        client.requestThreadApply();
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::setLooping(const source_client_handle_t &control_handle, bool looping) noexcept
{
    if (CXXPH_UNLIKELY(!isValidSourceClientControlHandle(control_handle))) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    const uint32_t index = getSourceClientControlHandleIndex(control_handle);
    const uint32_t mask = (1U << index);

    if (looping) {
        looping_bitmap_ |= mask;
    } else {
        looping_bitmap_ &= ~mask;
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::setAudioCaptureEnabled(bool enabled) noexcept
{
    captuing_enabled_ = enabled;
    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::setGlobalPreMixVolumeLevel(float level) noexcept
{
    u32_global_premix_level_ = float_to_u32(level);
    return OSLMP_RESULT_SUCCESS;
}

int AudioMixer::Impl::getSinkPullListenerCallback(void (**ppfunc)(void *), void **pargs) noexcept
{
    *ppfunc = onSinkPullListenerCallback;
    *pargs = this;

    return OSLMP_RESULT_SUCCESS;
}

bool AudioMixer::Impl::requestThreadApplyAudioSourceSetItems() noexcept
{
    if (requested_source_set_.updated_bitmap == 0)
        return false;

    bool applied = false;

    AudioSourceSet *item = nullptr;

    // obtain free item
    {
        audio_src_set_queue_t &queue = audio_src_set_request_thread_free_queue_;
        audio_src_set_queue_t::index_t index = audio_src_set_queue_t::INVALID_INDEX;

        if (queue.lock_read(index)) {
            item = obtain_and_set_nullptr(queue, index);
            queue.unlock_read(index);
        }
    }

    // push requested_source_set_ into the queue
    if (item) {
        audio_src_set_queue_t &queue = audio_src_set_request_to_mixer_thread_queue_;
        audio_src_set_queue_t::index_t index = audio_src_set_queue_t::INVALID_INDEX;

        if (queue.lock_write(index)) {
            queue.at(index) = &(item->copy(requested_source_set_));
            queue.unlock_write(index);

            // clear updated flags
            requested_source_set_.updated_bitmap = 0;

            applied = true;
        } else {
            LOGE("applyAudioSourceSetItems() - audio_src_set_request_to_mixer_thread_queue_.lock() failed");
        }
    }

    // wake up mixer thread
    if (applied) {
        utils::pt_unique_lock lock(mutex_mixer_thread_);
        cond_mixer_thread_.notify_one();
    }

#ifdef LOG_TAG
    if (applied) {
        LOGD("requestThreadApplyAudioSourceSetItems() - result = %d", applied);
    }
#endif

    return applied;
}

bool AudioMixer::Impl::requestThreadReceiveOneAudioSourceSetItem() noexcept
{
    AudioSourceSet *item = nullptr;

    VERIFY_CALLING_CONTEXT_IS_NORMAL();

    // dequeue from recycle queue
    {
        audio_src_set_queue_t &queue = audio_src_set_mixer_to_request_thread_queue_;
        audio_src_set_queue_t::index_t index = audio_src_set_queue_t::INVALID_INDEX;

        if (queue.lock_read(index)) {
            item = obtain_and_set_nullptr(queue, index);
            queue.unlock_read(index);
        }
    }

    if (!item)
        return false;

    // update
    utils::bitmap_looper looper(item->updated_bitmap);
    while (looper.loop()) {
        const int index = looper.index();

        const AudioSourceSlot &notify_slot = item->slots[index];
        AudioSourceSlot &active_slot = active_source_set_.slots[index];
        AudioSourceSlot &requested_slot = requested_source_set_.slots[index];

        if (!active_slot.source_pipe && !notify_slot.source_pipe) {
            // free slot
        } else if (!active_slot.source_pipe && notify_slot.source_pipe) {
            // attach

            // copy
            active_slot = notify_slot;

            requestThreadHandleOnMixingStateChanged(active_slot, SOURCE_SLOT_STATE_READY, active_slot.state);

            LOGD("[ReqThread (%d)] ATTACHED: slot[%d] pipe = %p", state_, index, active_slot.source_pipe);
        } else if (active_slot.source_pipe && (active_slot.source_pipe == notify_slot.source_pipe)) {
            if (notify_slot.operation == OPERATION_DETACH) {
                // detached

                // release used pipe
                int result = pipe_manager_->setSourcePipeOutPortUser(active_slot.source_pipe, this, false);
                if (result != OSLMP_RESULT_SUCCESS) {
                    LOGE("setSourcePipeOutPortUser(pipe = %p, user = %p, false) returns error = %d",
                         active_slot.source_pipe, this, result);
                }

                requestThreadHandleOnMixingStateChanged(active_slot, active_slot.state, notify_slot.state);

                // clear
                active_slot.clear();
                requested_slot.clear();
                requested_source_set_.unmark_updated(index);

                LOGD("[ReqThread (%d)] DETACHED: slot[%d] pipe = %p", state_, index, notify_slot.source_pipe);
            } else {
                // updated

                if (active_slot.state != notify_slot.state) {
                    requestThreadHandleOnMixingStateChanged(active_slot, active_slot.state, notify_slot.state);
                }

                // copy
                active_slot = notify_slot;
            }
        } else {
            LOGE("recycleOneAudioSourceSetItem() Unexpected condition: "
                 "slot[%d] active_slot.source_pipe = %p, notify_slot.source_pipe = %p",
                 index, active_slot.source_pipe, notify_slot.source_pipe);
        }
    }

    // recycle
    if (item->owner == OWNER_REQUEST_THREAD) {
        audio_src_set_queue_t &recycle_queue = audio_src_set_request_thread_free_queue_;
        audio_src_set_queue_t::index_t index = audio_src_set_queue_t::INVALID_INDEX;

        if (recycle_queue.lock_write(index)) {
            recycle_queue.at(index) = &(item->clear());
            recycle_queue.unlock_write(index);
        } else {
            LOGE("requestThreadReceiveOneAudioSourceSetItem() audio_src_set_request_thread_free_queue_.lock_write() "
                 "failed");
        }
    } else { // else if(item->owner == OWNER_MIXER_THREAD)
        // recycle
        audio_src_set_queue_t &recycle_queue = audio_src_set_request_to_mixer_thread_queue_;
        audio_src_set_queue_t::index_t index = audio_src_set_queue_t::INVALID_INDEX;

        if (recycle_queue.lock_write(index)) {
            recycle_queue.at(index) = &(item->clear());
            recycle_queue.unlock_write(index);
        } else {
            LOGE("requestThreadReceiveOneAudioSourceSetItem() "
                 "audio_src_set_request_to_mixer_thread_queue_.lock_write() failed");
        }
    }

#ifdef LOG_TAG
    dump_request_thread_source_source_slot_usage();
#endif

    return (item != nullptr);
}

int AudioMixer::Impl::requestThreadRecycleAllAudioSourceSetItems() noexcept
{
    int count = 0;

    // skip if called from callback
    if (calling_context_ == CALLING_CONTEXT_NORMAL) {
        while (requestThreadReceiveOneAudioSourceSetItem()) {
            count += 1;
        }
    }

    return count;
}

void AudioMixer::Impl::requestThreadApplyAllVolumes() noexcept
{
    for (auto &client : source_clients_) {
        if (client.modified) {
            client.requestThreadApply();
        }
    }
}

void AudioMixer::Impl::requestThreadRaiseOnMixingStarted(const AudioSourceSlot &slot,
                                                         source_slot_state_t next_state) noexcept
{
    VERIFY_CALLING_CONTEXT_IS_NORMAL();

    SourceClientEventHandler *event_handler = slot.source_client->event_handler;

    if (!event_handler)
        return;

    mixing_start_cause_t cause;

    switch (next_state) {
    case SOURCE_SLOT_STATE_STARTED_BY_EXPLICIT_OPERATION:
        cause = MIXING_START_CAUSE_EXPLICIT_OPERATION;
        break;
    case SOURCE_SLOT_STATE_STARTED_NO_LOOP_TRIGGERD:
        cause = MIXING_START_CAUSE_NO_LOOP_TRIGGERED;
        break;
    case SOURCE_SLOT_STATE_STARTED_LOOP_TRIGGERED:
        cause = MIXING_START_CAUSE_LOOP_TRIGGERED;
        break;
    default:
        assert(false);
        cause = MIXING_START_CAUSE_EXPLICIT_OPERATION;
        break;
    }

    calling_context_ = CALLING_CONTEXT_FROM_ON_MIXING_STARTED_EVENT;
    event_handler->onMixingStarted(slot.source_pipe, cause);
    calling_context_ = CALLING_CONTEXT_NORMAL;
}

void AudioMixer::Impl::requestThreadRaiseOnMixingStopped(const AudioSourceSlot &slot,
                                                         source_slot_state_t next_state) noexcept
{
    VERIFY_CALLING_CONTEXT_IS_NORMAL();

    SourceClientEventHandler *event_handler = slot.source_client->event_handler;

    if (!event_handler)
        return;

    mixing_stop_cause_t cause;

    switch (next_state) {
    case SOURCE_SLOT_STATE_UNUSED:
        cause = MIXING_STOP_CAUSE_DETACHED;
        break;
    case SOURCE_SLOT_STATE_STOPPED_BY_EXPLICIT_OPERATION:
        cause = MIXING_STOP_CAUSE_EXPLICIT_OPERATION;
        break;
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_NO_TRIGGERED:
        cause = MIXING_STOP_CAUSE_END_OF_DATA_NO_TRIGGERED;
        break;
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_NO_LOOPING_SOURCE:
        cause = MIXING_STOP_CAUSE_END_OF_DATA_TRIGGERED_NO_LOOPING_SOURCE;
        break;
    case SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_LOOPING_SOURCE:
        cause = MIXING_STOP_CAUSE_END_OF_DATA_TRIGGERED_LOOPING_SOURCE;
        break;
    case SOURCE_SLOT_STATE_STOPPED_BY_FADED_OUT:
        cause = MIXING_STOP_CAUSE_FADED_OUT;
        break;
    default:
        assert(false);
        cause = MIXING_STOP_CAUSE_EXPLICIT_OPERATION;
        break;
    }

    calling_context_ = CALLING_CONTEXT_FROM_ON_MIXING_STOPPED_EVENT;
    event_handler->onMixingStopped(slot.source_pipe, cause);
    calling_context_ = CALLING_CONTEXT_NORMAL;
}

void AudioMixer::Impl::requestThreadHandleOnMixingStateChanged(const AudioSourceSlot &slot,
                                                               source_slot_state_t cur_state,
                                                               source_slot_state_t next_state) noexcept
{
    const bool cur_is_started = is_started_state(cur_state);
    const bool next_is_started = is_started_state(next_state);

    if (!cur_is_started && next_is_started) {
        // started
        requestThreadRaiseOnMixingStarted(slot, next_state);
    } else if (cur_is_started && !next_is_started) {
        // stopped
        requestThreadRaiseOnMixingStopped(slot, next_state);
    }
}

void *AudioMixer::Impl::mixerThreadEntry(void *args) noexcept
{
    Impl *thiz = static_cast<Impl *>(args);

    // set thread priority
    AndroidHelper::setThreadPriority(thiz->context_->getJavaVM(), 0, ANDROID_THREAD_PRIORITY_AUDIO + ANDROID_THREAD_PRIORITY_LESS_FAVORABLE * 6);

    // set thread name
    AndroidHelper::setCurrentThreadName("OSLMPAudioMix");

    thiz->mixerThreadProcess();

    return nullptr;
}

void AudioMixer::Impl::mixerThreadProcess() noexcept
{
    control_flags_t &ref_ctrl_flg = mixer_thread_control_flags_;

    MixerThreadContext c;

    {
        utils::pt_unique_lock lock(mutex_mixer_thread_);

        // initialize
        for (int i = 0; i < NUM_MAX_SOURCE_PIPES; ++i) {
            const AudioSourceSlot &req_slot = requested_source_set_.slots[i];
            if (req_slot.source_pipe) {
                mixerThreadInitializeSource(c, i, req_slot);
            }
        }
        c.oslmp_context = context_;
        c.sink_slot = sink_slot_;
        c.max_process_block_at_once = max_process_block_at_once_;

        c.num_mixout_effects = num_mixout_effects_;
        for (int i = 0; i < c.num_mixout_effects; ++i) {
            c.mixout_effects[i] = mixout_effects_[i];
        }
        c.global_premix_level = u32_to_float(u32_global_premix_level_.load(std::memory_order_acquire));

#if USE_OSLMP_DEBUG_FEATURES
        c.nb_logger.reset(c.oslmp_context->getNonBlockingTraceLogger().create_new_client());
#endif

        // call MixedOutputAudioEffect::onAttachedToMixerThread()
        std::atomic_thread_fence(std::memory_order_seq_cst);
        for (int i = 0; i < c.num_mixout_effects; ++i) {
            (c.mixout_effects[i])->onAttachedToMixerThread();
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);

        // set CONTROL_FLAG_STATUS_RUNNING and CONTROL_FLAG_STATUS_INITIALIZED flags
        ref_ctrl_flg.fetch_or(CONTROL_FLAG_STATUS_INITIALIZED | CONTROL_FLAG_STATUS_RUNNING, std::memory_order_release);

        // notify initialized
        cond_mixer_thread_.notify_one();
    }

    REF_NB_LOGGER_CLIENT(c.nb_logger);

    // process loop
    int processed_count = 0;

    // timespec ts_prev;
    // utils::timespec_utils::get_current_time(ts_prev);
    MIXER_TRACE_ACTIVE();
    while (true) {
        uint32_t ctrl_flg = ref_ctrl_flg.load(std::memory_order_acquire);

        // handle suspend request
        if (should_suspend(ctrl_flg)) {
            utils::pt_unique_lock lock(mutex_mixer_thread_, true);

            lock.try_lock();

            if (lock.owns_lock()) {
                NB_LOGD("mixerThreadProcess() enter suspended state");

                // set CONTROL_FLAG_STATUS_SUSPENDED flag
                ctrl_flg = ref_ctrl_flg.fetch_or(CONTROL_FLAG_STATUS_SUSPENDED, std::memory_order_release);

                while (should_suspend(ctrl_flg)) {
                    // Process non-audio blocks
                    mixerThreadHandleNonAudioDataBlocks(c);

                    // Call effect polling method
                    mixerThreadPollMixOutEffects(c);

                    // wait
                    cond_mixer_thread_.wait(lock);
                    ctrl_flg = ref_ctrl_flg.load(std::memory_order_relaxed);
                }

                // clear CONTROL_FLAG_STATUS_SUSPENDED flag
                ref_ctrl_flg.fetch_and(~CONTROL_FLAG_STATUS_SUSPENDED, std::memory_order_release);

                NB_LOGD("mixerThreadProcess() leave suspended state");
            }
        }

        // handle stop request
        if (ctrl_flg & CONTROL_FLAG_REQUEST_STOP) {
            break;
        }

        // Update looping
        mixerThreadUpdateLoopingBitmap(c);

        // Call effect polling method
        mixerThreadPollMixOutEffects(c);

        // Update audio source set
        mixerThreadUpdateAudioSourceSets(c, AUDIO_SOURCE_SET_QUEUE_SIZE);

        // Process non-audio blocks
        mixerThreadHandleNonAudioDataBlocks(c);
        mixerThreadCheckStartConditions(c);
        mixerThreadCheckStopConditions(c);

        // Update global pre.mixing level
        mixerThreadUpdateGlobalPreMixLevel(c);

        // Update volume
        mixerThreadUpdateMixVolumes(c);

        // Process audio blocks
        bool audioBloockProcessed = mixerThreadHandleAudioDataBlocks(c);
        if (audioBloockProcessed) {
            // Process non-audio blocks
            mixerThreadHandleNonAudioDataBlocks(c);
            mixerThreadCheckStartConditions(c);
            mixerThreadCheckStopConditions(c);
        }

        mixerThreadNotifyCurrentAudioSourceSets(c);

        if (audioBloockProcessed) {
            processed_count += 1;
        }

        if (!audioBloockProcessed || processed_count >= c.max_process_block_at_once) {
            processed_count = 0;

            // determine sleep duration
            uint32_t sleep_ns = sleep_duration_ns_;

            // timespec ts_now, ts_next;
            // utils::timespec_utils::get_current_time(ts_now);
            // ts_next = utils::timespec_utils::add_ns(ts_prev, sleep_duration_ns_);

            // if (utils::timespec_utils::compare_greater_than_or_equals(ts_next, ts_now)) {
            //     const int64_t diff_ns = utils::timespec_utils::sub_ret_ns(ts_next, ts_now);
            //     sleep_ns = (std::min)(static_cast<uint32_t>(diff_ns), sleep_duration_ns_);
            //     NB_LOGV("mixerThreadProcess() - (ts_next >= ts_now)");
            // } else {
            //     // may be audio glitch has been generated
            //     NB_LOGI("mixerThreadProcess() - (ts_next < ts_now)");
            //     sleep_ns = (sleep_duration_ns_ / 2);
            //     ts_next = utils::timespec_utils::add_ns(ts_now, sleep_ns);
            // }

            // ts_prev = ts_next;

            // timespec ts_sleep(utils::timespec_utils::ZERO());
            // ts_sleep = utils::timespec_utils::add_ns(ts_sleep, sleep_ns);
            // ::clock_nanosleep(CLOCK_MONOTONIC, 0, &ts_sleep, nullptr);
            {
                utils::pt_unique_lock lock(mutex_mixer_thread_, true);

                lock.try_lock();
                
                if (lock.owns_lock()) {
                    MIXER_TRACE_INACTIVE();
                    cond_mixer_thread_.wait_relative_ns(lock, sleep_ns);
                    MIXER_TRACE_ACTIVE();
                }
            }
        }
    }

    // call MixedOutputAudioEffect::onDetachedFromMixerThread()
    std::atomic_thread_fence(std::memory_order_seq_cst);
    for (int i = 0; i < c.num_mixout_effects; ++i) {
        (c.mixout_effects[i])->onDetachedFromMixerThread();
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);

    // clean up
    while (mixerThreadUpdateAudioSourceSetOne(c))
        ;
    while (mixerThreadUpdateMixVolumes(c))
        ;

    // clear CONTROL_FLAG_STATUS_RUNNING
    ref_ctrl_flg.fetch_and(~CONTROL_FLAG_STATUS_RUNNING, std::memory_order_release);
}

bool AudioMixer::Impl::mixerThreadHandleAudioDataBlocks(MixerThreadContext &c) noexcept
{
    REF_NB_LOGGER_CLIENT(c.nb_logger);
    const uint32_t FILTER_AUDIO_DATA = (1U << AudioSourceDataPipe::TAG_AUDIO_DATA);

    AudioSinkDataPipe::write_block_t dest_block;

    if (CXXPH_LIKELY(!(c.sink_slot.pipe->lockWrite(dest_block)))) {
        NB_LOGV("mixerThreadHandleAudioDataBlocks() - lockWrite() returns false");
        return false;
    }

    const bool capture_enabled = (capture_pipe_ && captuing_enabled_);
    AudioCaptureDataPipe::write_block_t capture_block;
    AudioCaptureDataPipe::data_type *capture_buff = nullptr;

    if (capture_enabled) {
        if (CXXPH_LIKELY(capture_pipe_->lockWrite(capture_block, 0))) {
            assert(dest_block.num_channels == capture_block.num_channels);
            assert(dest_block.num_frames == capture_block.num_frames);

            capture_buff = capture_block.dest;
        } else {
            LOGI("mixerThreadHandleAudioDataBlocks() - Failed to obtain capture buffer");
        }
    }

    // mixing
    bool source_data_available = false;

    if (CXXPH_LIKELY(mixing_unit_.begin(dest_block.dest, dest_block.sample_format, capture_buff, dest_block.num_frames,
                                        c.mixout_effects, c.num_mixout_effects))) {

        utils::bitmap_looper looper(c.started_bitmap);
        while (CXXPH_LIKELY(looper.loop())) {
            const int index = looper.index();
            AudioSourceSlot &src_slot = c.currnt_src_set.slots[index];

            AudioSourceDataPipe::consume_block_t src_block;
            if (src_slot.source_pipe->lockConsume(src_block, 0, FILTER_AUDIO_DATA)) {
                mixing_unit_.mix(&(c.mixer_unit_context[index]), src_block.src, src_block.num_frames);
                src_slot.source_pipe->unlockConsume(src_block);
                source_data_available = true;
            } else {
                c.mixer_unit_context[index].phase = 1.0f;
            }
        }

        mixing_unit_.end();
    }

    if (capture_buff) {
        utils::timespec_utils::get_current_time(capture_block.timestamp);
        capture_pipe_->unlockWrite(capture_block);
    }

#if USE_OSLMP_DEBUG_FEATURES
    const uint16_t dest_locked_index = dest_block.dbg_lock_index;
#endif

    c.sink_slot.pipe->unlockWrite(dest_block);

    if (CXXPH_LIKELY(source_data_available)) {
        NB_LOGV("mixerThreadHandleAudioDataBlocks() - OK (index = %d)", dest_locked_index);
    } else {
        NB_LOGI("mixerThreadHandleAudioDataBlocks() - Source data is not present (index = %d)", dest_locked_index);
    }

    return true;
}

void AudioMixer::Impl::mixerThreadHandleNonAudioDataBlocks(MixerThreadContext &c) noexcept
{
    const uint32_t FILTER_NON_AUDIO_DATA = ~(1U << AudioSourceDataPipe::TAG_AUDIO_DATA);
    utils::bitmap_looper looper(c.started_bitmap);

    while (looper.loop()) {
        const int index = looper.index();
        AudioSourceSlot &src_slot = c.currnt_src_set.slots[index];

        if (!src_slot.source_pipe)
            continue;

        AudioSourceDataPipe::consume_block_t block;
        while (CXXPH_UNLIKELY(src_slot.source_pipe->lockConsume(block, 0, FILTER_NON_AUDIO_DATA))) {

            switch (block.tag) {
            case AudioSourceDataPipe::TAG_EVENT_END_OF_DATA:
                c.flags[index] |= FLAG_END_OF_DATA_DETECTED;
                if (c.looping_bitmap & (1U << src_slot.handle_index)) {
                    c.flags[index] |= FLAG_END_OF_DATA_WITH_LOOPING_ENABLED;

                    // override tag
                    block.tag = AudioSourceDataPipe::TAG_EVENT_END_OF_DATA_WITH_LOOP_POINT;
                }
                break;
            default:
                LOGE("Unexpected TAG block found (tag = %d)", block.tag);
                break;
            }

            src_slot.source_pipe->unlockConsume(block);
        }
    }
}

bool AudioMixer::Impl::mixerThreadUpdateAudioSourceSets(MixerThreadContext &c, int max_iterate) noexcept
{
    int updated_count = 0;
    for (int i = 0; i < max_iterate; ++i) {
        if (CXXPH_UNLIKELY(mixerThreadUpdateAudioSourceSetOne(c))) {
            updated_count += 1;
        } else {
            break;
        }
    }

    return (updated_count > 0);
}

void AudioMixer::Impl::mixerThreadInitializeSource(MixerThreadContext &c, int index,
                                                   const AudioSourceSlot &new_slot) noexcept
{
    AudioSourceSlot &cur_slot = c.currnt_src_set.slots[index];
    MixingUnit::Context &mix_context = c.mixer_unit_context[index];
    uint32_t &flags = c.flags[index];
    const bool update =
        (cur_slot.client_handle == new_slot.client_handle) && (cur_slot.source_pipe == new_slot.source_pipe);

    // set flags
    flags = (update) ? flags : FLAG_NONE;
    switch (new_slot.source_pipe->consumerGetLastBlockTag()) {
    case AudioSourceDataPipe::TAG_EVENT_END_OF_DATA:
        flags |= FLAG_END_OF_DATA_DETECTED;
        break;
    case AudioSourceDataPipe::TAG_EVENT_END_OF_DATA_WITH_LOOP_POINT:
        flags |= FLAG_END_OF_DATA_DETECTED | FLAG_END_OF_DATA_WITH_LOOPING_ENABLED;
        break;
    default:
        break;
    }

    switch (new_slot.operation) {
    case OPERATION_NONE:
        break;
    case OPERATION_START:
        if (!(flags & FLAG_DETACH_REQUESTED)) {
            flags |= FLAG_STARTED;
        }
        break;
    case OPERATION_STOP:
        if (!(flags & FLAG_DETACH_REQUESTED)) {
            flags &= ~FLAG_STARTED;
        }
        break;
    case OPERATION_DETACH:
        flags &= ~FLAG_STARTED;
        flags |= FLAG_DETACH_REQUESTED;
        break;
    }

    // set mixing unit context
    const MixingUnit::mode_t prev_mix_mode = mix_context.mode;
    mix_context.mode = static_cast<MixingUnit::mode_t>(new_slot.mix_mode);
    if (new_slot.mix_phase_override) {
        mix_context.phase = new_slot.mix_phase;
    } else {
        if (cur_slot.source_pipe == new_slot.source_pipe) {
            mix_context.phase = calcMixingPhase(prev_mix_mode, mix_context.mode, mix_context.phase);
        } else {
            mix_context.phase = 0.0f;
        }
    }

    // volume
    mix_context.volume[0] = (new_slot.source_client)->actual_volume_left * c.global_premix_level;
    mix_context.volume[1] = (new_slot.source_client)->actual_volume_right * c.global_premix_level;

    // determine next state
    source_slot_state_t next_state;

    switch (new_slot.operation) {
    case OPERATION_NONE:
        if (update) {
            next_state = cur_slot.state;
        } else {
            next_state = SOURCE_SLOT_STATE_READY;
        }
        break;
    case OPERATION_START:
        if (update) {
            if (can_transit_to_started_by_explicit_operation(cur_slot.state)) {
                next_state = SOURCE_SLOT_STATE_STARTED_BY_EXPLICIT_OPERATION;
            } else {
                next_state = cur_slot.state;
            }
        } else {
            next_state = SOURCE_SLOT_STATE_STARTED_BY_EXPLICIT_OPERATION;
        }
        break;
    case OPERATION_STOP:
        if (update) {
            if (can_transit_to_stopped_by_explicit_operation(cur_slot.state)) {
                next_state = SOURCE_SLOT_STATE_STOPPED_BY_EXPLICIT_OPERATION;
            } else {
                next_state = cur_slot.state;
            }
        } else {
            next_state = SOURCE_SLOT_STATE_STOPPED_BY_EXPLICIT_OPERATION;
        }
        break;
    case OPERATION_DETACH:
        next_state = SOURCE_SLOT_STATE_UNUSED;
        break;
    }

    // update current slot
    cur_slot = new_slot;
    cur_slot.state = next_state;

    // update fields
    const uint32_t bitmap_mask = (1U << index);
    c.attached_bitmap = update_bitmap(c.attached_bitmap, bitmap_mask, true);
    c.started_bitmap = update_bitmap(c.started_bitmap, bitmap_mask, (flags & FLAG_STARTED));
    c.detach_requested_bitmap = update_bitmap(c.detach_requested_bitmap, bitmap_mask, (flags & FLAG_DETACH_REQUESTED));
}

bool AudioMixer::Impl::mixerThreadUpdateAudioSourceSetOne(MixerThreadContext &c) noexcept
{
    audio_src_set_queue_t &request_queue = audio_src_set_request_to_mixer_thread_queue_;

    AudioSourceSet *item = nullptr;

    // retrieve from the request_queue
    {
        audio_src_set_queue_t::index_t index = audio_src_set_queue_t::INVALID_INDEX;

        if (CXXPH_UNLIKELY(request_queue.lock_read(index))) {
            item = obtain_and_set_nullptr(request_queue, index);
            (void)request_queue.unlock_read(index);
        }
    }

    if (CXXPH_LIKELY(!item)) {
        return false;
    }

    bool applied = false;

    if (item->owner == OWNER_REQUEST_THREAD) {
        // update
        utils::bitmap_looper looper(item->updated_bitmap);

        while (looper.loop()) {
            const int index = looper.index();
            mixerThreadInitializeSource(c, index, item->slots[index]);
        }

        c.currnt_src_set.updated_bitmap |= item->updated_bitmap;

        // recycle
        {
            audio_src_set_queue_t &recycle_queue = audio_src_set_mixer_to_request_thread_queue_;
            audio_src_set_queue_t::index_t index = audio_src_set_queue_t::INVALID_INDEX;

            if (CXXPH_LIKELY(recycle_queue.lock_write(index))) {
                item->copy(c.currnt_src_set);
                recycle_queue.at(index) = item;
                (void)recycle_queue.unlock_write(index);
                applied = true;
            } else {
                LOGE("AudioMixer::Impl::updateAudioSourceSet() "
                     "audio_src_set_mixer_to_request_thread_queue_.lock_write() failed");
            }
        }
    } else { // else if (item->owner == OWNER_MIXER_THREAD)
        // recycle
        audio_src_set_queue_t &recycle_queue = audio_src_set_mixer_thread_free_queue_;
        audio_src_set_queue_t::index_t index = audio_src_set_queue_t::INVALID_INDEX;

        if (CXXPH_LIKELY(recycle_queue.lock_write(index))) {
            recycle_queue.at(index) = &(item->clear());
            (void)recycle_queue.unlock_write(index);
        } else {
            LOGE("AudioMixer::Impl::updateAudioSourceSet() audio_src_set_mixer_thread_free_queue_.lock_write() failed");
        }
    }

    mixerThreadCheckStartConditions(c);

    if (applied) {
        c.currnt_src_set.updated_bitmap = 0;
        mixerThreadCleanUpCurrentSourceSet(c);
    }

#ifdef LOG_TAG
    dump_mixer_thread_source_source_slot_usage(c);
#endif

    return true;
}

bool AudioMixer::Impl::mixerThreadNotifyCurrentAudioSourceSets(MixerThreadContext &c) noexcept
{
    audio_src_set_queue_t &free_queue = audio_src_set_mixer_thread_free_queue_;
    audio_src_set_queue_t &notify_queue = audio_src_set_mixer_to_request_thread_queue_;

    audio_src_set_queue_t::index_t free_index = audio_src_set_queue_t::INVALID_INDEX;
    audio_src_set_queue_t::index_t notify_index = audio_src_set_queue_t::INVALID_INDEX;

    if (!(c.currnt_src_set.updated_bitmap)) {
        // no need to notify
        return false;
    }

    // lock free queue
    if (free_queue.lock_read(free_index)) {
        // NOTE: don't unlock the free_queue here
    } else {
        return false;
    }

    // lock destination queue
    if (notify_queue.lock_write(notify_index)) {
        // obtain free item
        AudioSourceSet *free_item = obtain_and_set_nullptr(free_queue, free_index);

        // unlock the free_aueue
        free_queue.unlock_read(free_index, true);

        // copy
        free_item->copy(c.currnt_src_set);

        // push
        notify_queue.at(notify_index) = free_item;
        notify_queue.unlock_write(notify_index);

        c.currnt_src_set.updated_bitmap = 0;

#ifdef LOG_TAG
        dump_mixer_thread_source_source_slot_usage(c);
#endif

        return true;
    } else {
        // unlock the free_queue without commit
        free_queue.unlock_read(free_index, false);

        return false;
    }
}

void AudioMixer::Impl::mixerThreadCleanUpCurrentSourceSet(MixerThreadContext &c) noexcept
{
    utils::bitmap_looper looper(c.detach_requested_bitmap);

    while (looper.loop()) {
        const int index = looper.index();
        uint32_t mask = (1U << index);

        c.currnt_src_set.slots[index].clear();
        c.flags[index] = 0;
        clear(c.mixer_unit_context[index]);

        c.attached_bitmap = update_bitmap(c.attached_bitmap, mask, false);
        c.detach_requested_bitmap = update_bitmap(c.detach_requested_bitmap, mask, false);
        c.started_bitmap = update_bitmap(c.started_bitmap, mask, false);
    }
}

bool AudioMixer::Impl::mixerThreadCheckStartConditions(MixerThreadContext &c) noexcept
{
    uint32_t started_bitmap = 0;

    utils::bitmap_looper outer_looper((c.attached_bitmap) & (~c.started_bitmap));
    while (outer_looper.loop()) {
        const int target_index = outer_looper.index();
        AudioSourceSlot &target_slot = c.currnt_src_set.slots[target_index];
        uint32_t &target_flags = c.flags[target_index];

        int triggered_index = -1;

        utils::bitmap_looper inner_looper(c.attached_bitmap & ~(1U << target_index));
        while (inner_looper.loop() && (triggered_index < 0)) {
            const int trigger_index = inner_looper.index();
            const AudioSourceSlot &trigger_slot = c.currnt_src_set.slots[trigger_index];
            const uint32_t trigger_flags = c.flags[trigger_index];

            if (target_flags & FLAG_END_OF_DATA_DETECTED)
                continue;

            if (trigger_flags & FLAG_TRIGGER_FIRED)
                continue;

            if (trigger_flags & FLAG_END_OF_DATA_DETECTED) {
                if (trigger_flags & FLAG_END_OF_DATA_WITH_LOOPING_ENABLED) {
                    if ((trigger_slot.trigger_loop_mode == TRIGGER_ON_END_OF_DATA) &&
                        (trigger_slot.trigger_loop_target == target_slot.client_handle) &&
                        (trigger_slot.trigger_loop_source_no == target_slot.source_no)) {
                        triggered_index = trigger_index;
                        target_slot.state = SOURCE_SLOT_STATE_STARTED_LOOP_TRIGGERED;
                    }
                } else {
                    if ((trigger_slot.trigger_no_loop_mode == TRIGGER_ON_END_OF_DATA) &&
                        (trigger_slot.trigger_no_loop_target == target_slot.client_handle) &&
                        (trigger_slot.trigger_no_loop_source_no == target_slot.source_no)) {
                        triggered_index = trigger_index;
                        target_slot.state = SOURCE_SLOT_STATE_STARTED_NO_LOOP_TRIGGERD;
                    }
                }
            }
        }

        if (triggered_index >= 0) {
            LOGD("TRIGGER_AUDIO_DATA_END fired");

            target_flags |= FLAG_STARTED;
            c.flags[triggered_index] |= FLAG_TRIGGER_FIRED;

            // inherit mixer state (copy mixer unit context)
            c.mixer_unit_context[target_index] = c.mixer_unit_context[triggered_index];

            started_bitmap |= (1U << target_index);
        }
    }

    c.started_bitmap |= started_bitmap;
    c.currnt_src_set.updated_bitmap |= started_bitmap;

#ifdef LOG_TAG
    if (started_bitmap != 0) {
        dump_mixer_thread_source_source_slot_usage(c);
    }
#endif

    return (started_bitmap != 0);
}

bool AudioMixer::Impl::mixerThreadCheckStopConditions(MixerThreadContext &c) noexcept
{
    uint32_t stopped_bitmap = 0;

    utils::bitmap_looper looper(c.started_bitmap);

    while (looper.loop()) {
        const int index = looper.index();
        AudioSourceSlot &slot = c.currnt_src_set.slots[index];
        MixingUnit::Context &mix_context = c.mixer_unit_context[index];
        uint32_t &flags = c.flags[index];

        source_slot_state_t stop_state = SOURCE_SLOT_STATE_NOP;

        if (slot.stop_cond & STOP_AFTER_FADE_OUT) {
            if (isFadeOut(mix_context.mode) && mix_context.phase >= 1.0f) {
                stop_state = SOURCE_SLOT_STATE_STOPPED_BY_FADED_OUT;
            }
        }

        if (slot.stop_cond & STOP_ON_PLAYBACK_END) {
            if (flags & FLAG_END_OF_DATA_DETECTED) {
                if (flags & FLAG_TRIGGER_FIRED) {
                    if (flags & FLAG_END_OF_DATA_WITH_LOOPING_ENABLED) {
                        stop_state = SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_LOOPING_SOURCE;
                    } else {
                        stop_state = SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_TRIGGERED_NO_LOOPING_SOURCE;
                    }
                } else {
                    stop_state = SOURCE_SLOT_STATE_STOPPED_BY_DATA_END_NO_TRIGGERED;
                }
            }
        }

        if (stop_state != SOURCE_SLOT_STATE_NOP) {
            flags &= ~FLAG_STARTED;
            slot.state = stop_state;

            stopped_bitmap |= (1U << index);
        }
    }

    c.started_bitmap &= ~stopped_bitmap;
    c.currnt_src_set.updated_bitmap |= stopped_bitmap;

#ifdef LOG_TAG
    if (stopped_bitmap != 0) {
        dump_mixer_thread_source_source_slot_usage(c);
    }
#endif

    return (stopped_bitmap != 0);
}

void AudioMixer::Impl::mixerThreadUpdateGlobalPreMixLevel(MixerThreadContext &c) noexcept
{
    c.global_premix_level = u32_to_float(u32_global_premix_level_.load(std::memory_order_acquire));
}

bool AudioMixer::Impl::mixerThreadUpdateMixVolumes(MixerThreadContext &c) noexcept
{
    bool updated = false;

    for (auto &client : source_clients_) {
        updated |= client.mixerThreadUpdate();
    }

    utils::bitmap_looper looper(c.attached_bitmap);
    while (looper.loop()) {
        const int index = looper.index();
        AudioSourceSlot &slot = c.currnt_src_set.slots[index];

        MixerSourceClient &client = (*(slot.source_client));
        MixingUnit::Context &mix_context = c.mixer_unit_context[index];

        mix_context.volume[0] = client.actual_volume_left * c.global_premix_level;
        mix_context.volume[1] = client.actual_volume_right * c.global_premix_level;
    }

    return updated;
}

void AudioMixer::Impl::mixerThreadUpdateLoopingBitmap(MixerThreadContext &c) noexcept
{
    c.looping_bitmap = looping_bitmap_;
}

void AudioMixer::Impl::mixerThreadPollMixOutEffects(MixerThreadContext &c) noexcept
{
    for (int i = 0; i < c.num_mixout_effects; ++i) {
        (c.mixout_effects[i])->pollFromMixerThread();
    }
}

int AudioMixer::Impl::isFadeIn(MixingUnit::mode_t mode) noexcept
{
    switch (mode) {
    case MixingUnit::MODE_SHORT_FADE_IN:
    case MixingUnit::MODE_LONG_FADE_IN:
        return true;
    default:
        return false;
    }
}

int AudioMixer::Impl::isFadeOut(MixingUnit::mode_t mode) noexcept
{
    switch (mode) {
    case MixingUnit::MODE_SHORT_FADE_OUT:
    case MixingUnit::MODE_LONG_FADE_OUT:
        return true;
    default:
        return false;
    }
}

int AudioMixer::Impl::getMixingDirection(MixingUnit::mode_t mode) noexcept
{
    // NOTE:
    // -1 : decreasing (fade out)
    //  0 : neutral
    // +1 : increasing (fade in)
    switch (mode) {
    case MixingUnit::MODE_MUTE:
        return 0;
    case MixingUnit::MODE_ADD:
        return 0;
    case MixingUnit::MODE_SHORT_FADE_IN:
    case MixingUnit::MODE_LONG_FADE_IN:
        return +1;
    case MixingUnit::MODE_SHORT_FADE_OUT:
    case MixingUnit::MODE_LONG_FADE_OUT:
        return -1;
    default:
        return 0;
    }
}

float AudioMixer::Impl::calcMixingPhase(MixingUnit::mode_t cur_mode, MixingUnit::mode_t next_mode,
                                        float cur_phase) noexcept
{
    if (cur_mode == next_mode) {
        return cur_phase;
    } else {
        const int cur_dir = getMixingDirection(cur_mode);
        const int next_dir = getMixingDirection(next_mode);

        switch (cur_dir * next_dir) {
        case +1:
            return cur_phase;
        case -1:
            return (1.0f - cur_phase);
        default:
            return 0.0f;
        }
    }
}

bool AudioMixer::Impl::isInitialized() const noexcept
{
    switch (state_) {
    case MIXER_STATE_NOT_INITIALIZED:
        return false;
    case MIXER_STATE_STARTED:
    case MIXER_STATE_STOPPED:
    case MIXER_STATE_SUSPENDED:
        return true;
    default:
        LOGE("Unknown mixer state = %d", state_);
        return false;
    }
}

bool AudioMixer::Impl::isStarted() const noexcept
{
    return (state_ == MIXER_STATE_STARTED) || (state_ == MIXER_STATE_SUSPENDED);
}

bool AudioMixer::Impl::isStopped() const noexcept { return (state_ == MIXER_STATE_STOPPED); }

void AudioMixer::Impl::onSinkPullListenerCallback(void *args) {
    AudioMixer::Impl *thiz = static_cast<AudioMixer::Impl*>(args);

    thiz->cond_mixer_thread_.notify_one();
}


#ifdef LOG_TAG
void AudioMixer::Impl::dump_request_thread_source_source_slot_usage() noexcept
{
    std::string str;

    for (int i = 0; i < NUM_MAX_SOURCE_PIPES; ++i) {
        const auto &req_slot = requested_source_set_.slots[i];
        const auto &act_slot = active_source_set_.slots[i];

        str += "[";

        if (req_slot.source_pipe) {
            if (req_slot.operation == OPERATION_DETACH) {
                str += "D";
            } else {
                str += "*";
            }
        } else {
            str += "-";
        }

        if (act_slot.source_pipe) {
            str += static_cast<char>('0' + act_slot.state);
        } else {
            str += "-";
        }

        if (requested_source_set_.updated_bitmap & (1 << i)) {
            str += "!";
        } else {
            str += "-";
        }

        if (req_slot.source_pipe != act_slot.source_pipe) {
            str += "X";
        } else {
            str += "-";
        }

        str += "] ";
    }

    LOGD("[ReqThread (%d)] Source slots usage: %s", state_, str.c_str());
}

void AudioMixer::Impl::dump_mixer_thread_source_source_slot_usage(const MixerThreadContext &c) noexcept
{
    std::string str;

    for (int i = 0; i < NUM_MAX_SOURCE_PIPES; ++i) {
        const auto &slot = c.currnt_src_set.slots[i];

        str += "[";

        if (slot.source_pipe) {
            if (slot.operation == OPERATION_DETACH) {
                str += "D";
            } else {
                str += "*";
            }
        } else {
            str += "-";
        }

        if (slot.source_pipe) {
            str += static_cast<char>('0' + slot.state);
        } else {
            str += "-";
        }

        if (c.currnt_src_set.updated_bitmap & (1 << i)) {
            str += "!";
        } else {
            str += "-";
        }

        str += "] ";
    }

    LOGD("[MixThread] Source slots usage: %s", str.c_str());
}
#endif

} // namespace impl
} // namespace oslmp
