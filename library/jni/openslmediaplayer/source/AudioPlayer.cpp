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

// #define LOG_TAG "AudioPlayer"

#include "oslmp/impl/AudioPlayer.hpp"

#include <cassert>

#include <cxxporthelper/memory>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/impl/AudioSource.hpp"
#include "oslmp/impl/AudioDataPipeManager.hpp"
#include "oslmp/impl/AudioSystem.hpp"
#include "oslmp/impl/AudioMixer.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/OpenSLMediaPlayerMetadata.hpp"
#include "oslmp/utils/timespec_utils.hpp"

#define MIN_SEEK_REQUEST_PERIOD_MSEC 100

// #define OUTPUT_DETAIL_DEBUG_LOGS

#ifdef OUTPUT_DETAIL_DEBUG_LOGS
#define DEBUG_CURRENT_AUDIO_SOURCE_STATE(msg) dumpCurrentAudioSourceState(msg)
#else
#define DEBUG_CURRENT_AUDIO_SOURCE_STATE(msg)
#endif

#define MIXING_STOP_CAUSE_INVALID   ((AudioMixer::mixing_stop_cause_t) (-1))

namespace oslmp {
namespace impl {

enum mix_source_no_t {
    MIX_SOURCE_NO_DEFAULT = 0,
    MIX_SOURCE_NO_ACTIVE = 1,
    MIX_SOURCE_NO_NEXT = 2,
    MIX_SOURCE_NO_OLD = 3,
};

class OpenSLMediaPlayerInternalContext;

enum audio_source_create_reason_t {
    AUDIO_SOURCE_CREATE_REASON_NONE,
    AUDIO_SOURCE_CREATE_REASON_PREPARING,
    AUDIO_SOURCE_CREATE_REASON_SEEKING,
    AUDIO_SOURCE_CREATE_REASON_REWINDING,
};

struct poll_results_info_t {
    bool prepare_completed;
    int prepare_result;

    bool seek_completed;
    int seek_result;

    bool playback_completed;
    bool playback_looped;
    bool next_player_started;

    bool buffering_status_updated;
    int32_t bufferred_percentage;

    poll_results_info_t()
        : prepare_completed(false), prepare_result(0), seek_completed(false), seek_result(0), playback_completed(false),
          playback_looped(false), next_player_started(false), buffering_status_updated(false), bufferred_percentage(0)
    {
    }
};

class AudioPlayer::Impl : public AudioMixer::SourceClientEventHandler {
public:
    Impl(AudioPlayer *holder);
    virtual ~Impl();

    int initialize(const initialize_args_t &args) noexcept;

    int setDataSourcePath(const char *path) noexcept;
    int setDataSourceUri(const char *uri) noexcept;
    int setDataSourceFd(int fd) noexcept;
    int setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept;

    int preparePatialStart() noexcept;
    bool isPreparing() const noexcept;

    int start() noexcept;
    int startAsNextPlayer() noexcept;
    int pause() noexcept;
    int stop() noexcept;
    int reset() noexcept;
    int setVolume(float left_volume, float right_volume) noexcept;
    int getAudioSessionId(int32_t *audio_session_id) noexcept;
    int getDuration(int32_t *duration) noexcept;
    int getCurrentPosition(int32_t *position) noexcept;
    int seekTo(int32_t msec) noexcept;
    int setLooping(bool looping) noexcept;
    int isLooping(bool *looping) noexcept;
    int setAudioStreamType(int stream_type) noexcept;
    int getAudioStreamType(int *stream_type) const noexcept;
    int setNextMediaPlayer(AudioPlayer *next) noexcept;
    int attachAuxEffect(int effect_id) noexcept;
    int setAuxEffectSendLevel(float level) noexcept;
    int setFadeInOutEnabled(bool enabled) noexcept;

    bool isPollingRequired() const noexcept;
    void poll() noexcept;

    // implementations of AudioMixer::SourceClientEventHandler
    virtual void onMixingStarted(AudioSourceDataPipe *pipe, AudioMixer::mixing_start_cause_t cause) noexcept override;
    virtual void onMixingStopped(AudioSourceDataPipe *pipe, AudioMixer::mixing_stop_cause_t cause) noexcept override;

private:
    int startReadySourceInternal(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;

    int release() noexcept;

    int pollPreparingSourceOnce(poll_results_info_t &results, bool &need_retry) noexcept;
    void pollPreparingSource(poll_results_info_t &results) noexcept;
    void pollCheckBufferingUpdate(poll_results_info_t &results) noexcept;
    int handlePreparationCompleteForPrepareing(int result) noexcept;
    int handlePreparationCompleteForSeeking(int result) noexcept;
    int handlePreparationCompleteForRewinding(int result) noexcept;
    void pollHandleStartPending() noexcept;
    void pollHandlePendingSeekRequest() noexcept;
    void pollHandlePlaybackCompletion(poll_results_info_t &results) noexcept;
    bool checkConditionForRewindedSourceCreation() const noexcept;
    int createAndStartPreparingAudioSource(std::unique_ptr<AudioSource> &dest_source, int32_t seek_position) noexcept;
    int createAndStartPreparingNextAudioSource(audio_source_create_reason_t reason, int32_t seek_position) noexcept;

    int refreshCurrentSourceToMixer(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;

    void detachFromMixer(const std::unique_ptr<AudioSource> &source,
                         AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void releaseAudioSource(std::unique_ptr<AudioSource> &source,
                            AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void releasePreparingAudioSource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void releaseReadyAudioSource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void releaseActiveAudioSource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void releaseNextAudioSource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void releaseOldAudioSource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;

    void movePreparingSourceToReadySource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void movePreparingSourceToNextSource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void moveNextSourceToReadySource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void moveActiveSourceToReadySource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void moveReadySourceToActiveSource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void moveActiveSourceToOldSource(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;
    void releaseAllAudioSources(AudioMixer::DeferredApplication *mixer_da = nullptr) noexcept;

    Impl *getNextPlayerImpl() const noexcept;
    std::unique_ptr<AudioSource> &getCurrentSource() noexcept;
    const std::unique_ptr<AudioSource> &getCurrentSource() const noexcept;
    AudioMixer *getAudioMixer() const noexcept;

    void setHandle(AudioMixer::attach_update_source_args_t &args) const noexcept;
    void setTriggerConditions(AudioMixer::attach_update_source_args_t &args) const noexcept;
    void clearTriggerConditions(AudioMixer::attach_update_source_args_t &args) const noexcept;
    void setStopConditions(AudioMixer::attach_update_source_args_t &args) const noexcept;
    void setMixModeFadeIn(AudioMixer::attach_update_source_args_t &args) const noexcept;
    void setMixModeFadeOut(AudioMixer::attach_update_source_args_t &args) const noexcept;
    void makeActiveFadeInParams(AudioMixer::attach_update_source_args_t &args,
                                std::unique_ptr<AudioSource> &source) const noexcept;
    void makeActiveFadeOutParams(AudioMixer::attach_update_source_args_t &args,
                                 std::unique_ptr<AudioSource> &source) const noexcept;
    void makeNextFadeInParams(AudioMixer::attach_update_source_args_t &args, std::unique_ptr<AudioSource> &source) const
        noexcept;
    void makeOldFadeOutParams(AudioMixer::attach_update_source_args_t &args, std::unique_ptr<AudioSource> &source) const
        noexcept;

    void setStartedStatus(bool started, bool clear_pending) noexcept;
    bool isSeeking() const noexcept;

    void raiseOnPlayerStartedAsNextPlayerEvent() noexcept;
    void raiseOnPlaybackCompletionEvent(AudioPlayer::PlaybackCompletionType completion_type) noexcept;
    void raiseOnDecoderBufferingUpdate(int32_t percent) noexcept;
    void raiseOnSeekCompleted(int seek_result) noexcept;
    void raiseOnPrepareCompleted(int prepare_result) noexcept;

#ifdef OUTPUT_DETAIL_DEBUG_LOGS
    void dumpCurrentAudioSourceState(const char *msg) const noexcept;
#endif

private:
    AudioPlayer *holder_;
    uint32_t player_instance_id_;
    std::unique_ptr<AudioSource> preparing_source_;
    std::unique_ptr<AudioSource> next_source_;
    std::unique_ptr<AudioSource> ready_source_;
    std::unique_ptr<AudioSource> active_source_;
    std::unique_ptr<AudioSource> old_source_;
    OpenSLMediaPlayerInternalContext *context_;
    AudioPlayer::EventHandler *event_handler_;

    OpenSLMediaPlayerMetadata metadata_;
    AudioSource::data_source_info_t data_source_;

    AudioMixer::source_client_handle_t mixer_control_handle_;

    bool looping_;
    bool fade_in_out_enabled_;

    bool prepared_;
    bool started_;
    bool start_pending_;
    bool playback_completed_;
    int32_t last_stopped_position_;
    audio_source_create_reason_t preparing_source_create_reason_;

    AudioPlayer *next_player_;
    uint32_t next_player_instance_id_;

    int32_t last_buffering_update_notified_position_;

    timespec ts_last_seek_request_;
    bool seek_pending_;
    int32_t pending_seek_position_;

    AudioMixer::mixing_stop_cause_t current_source_stop_cause_;
};

class TimeoutChecker {
public:
    TimeoutChecker(int deadline_us) : deadline_us_(deadline_us)
    {
        utils::timespec_utils::get_current_time(start_time_);
    }

    const bool check() const noexcept
    {
        timespec now;
        utils::timespec_utils::get_current_time(now);
        return (utils::timespec_utils::sub_ret_us(now, start_time_) < deadline_us_);
    }

private:
    const int deadline_us_;
    timespec start_time_;
};

//
// Utility functions
//

static inline void clear(AudioSource::data_source_info_t &info) noexcept
{
    info.type = AudioSource::DATA_SOURCE_NONE;
    info.path_uri.clear();
    info.fd = 0;
    info.offset = 0;
    info.length = 0;
}

static inline bool safeIsPreparing(const std::unique_ptr<AudioSource> &source) noexcept
{
    return (source && source->isPreparing());
}

static inline bool safeIsPrepared(const std::unique_ptr<AudioSource> &source) noexcept
{
    return (source && source->isPrepared());
}

static inline AudioSource::playback_completion_type_t
safeGetPlaybackCompletionType(const std::unique_ptr<AudioSource> &source) noexcept
{
    if (source) {
        return source->getPlaybackCompletionType();
    } else {
        return AudioSource::PLAYBACK_NOT_COMPLETED;
    }
}

static inline AudioSourceDataPipe *safeGetPipe(const std::unique_ptr<AudioSource> &source) noexcept
{
    return (source) ? (source->getPipe()) : nullptr;
}

static inline bool safeIsPlaybackCompleted(const std::unique_ptr<AudioSource> &source) noexcept
{
    auto t = safeGetPlaybackCompletionType(source);
    return (t == AudioSource::PLAYBACK_COMPLETED || t == AudioSource::PLAYBACK_COMPLETED_WITH_LOOP_POINT);
}

static inline void safeStopDecoder(const std::unique_ptr<AudioSource> &source) noexcept
{
    if (source) {
        source->stopDecoder();
    }
}

static inline void checkedMoveAudioSource(std::unique_ptr<AudioSource> &dest,
                                          std::unique_ptr<AudioSource> &src) noexcept
{
    assert(!dest); // destination pointer must be released
    dest = std::move(src);
}

//
// AudioPlayer
//
AudioPlayer::AudioPlayer() : impl_(new (std::nothrow) Impl(this)) {}

AudioPlayer::~AudioPlayer() {}

int AudioPlayer::initialize(const AudioPlayer::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioPlayer::setDataSourcePath(const char *path) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setDataSourcePath(path);
}

int AudioPlayer::setDataSourceUri(const char *uri) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setDataSourceUri(uri);
}

int AudioPlayer::setDataSourceFd(int fd) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setDataSourceFd(fd);
}

int AudioPlayer::setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setDataSourceFd(fd, offset, length);
}

int AudioPlayer::preparePatialStart() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->preparePatialStart();
}

bool AudioPlayer::isPreparing() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isPreparing();
}

int AudioPlayer::start() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->start();
}

int AudioPlayer::pause() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->pause();
}

int AudioPlayer::stop() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->stop();
}

int AudioPlayer::reset() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->reset();
}

int AudioPlayer::setVolume(float left_volume, float right_volume) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setVolume(left_volume, right_volume);
}

int AudioPlayer::getAudioSessionId(int32_t *audio_session_id) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getAudioSessionId(audio_session_id);
}

int AudioPlayer::getDuration(int32_t *duration) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getDuration(duration);
}

int AudioPlayer::getCurrentPosition(int32_t *position) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getCurrentPosition(position);
}

int AudioPlayer::seekTo(int32_t msec) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->seekTo(msec);
}

int AudioPlayer::setLooping(bool looping) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setLooping(looping);
}

int AudioPlayer::isLooping(bool *looping) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->isLooping(looping);
}

int AudioPlayer::setAudioStreamType(int stream_type) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAudioStreamType(stream_type);
}

int AudioPlayer::getAudioStreamType(int *stream_type) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getAudioStreamType(stream_type);
}

int AudioPlayer::setNextMediaPlayer(AudioPlayer *next) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setNextMediaPlayer(next);
}

int AudioPlayer::attachAuxEffect(int effect_id) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->attachAuxEffect(effect_id);
}

int AudioPlayer::setAuxEffectSendLevel(float level) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAuxEffectSendLevel(level);
}

int AudioPlayer::setFadeInOutEnabled(bool enabled) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setFadeInOutEnabled(enabled);
}

bool AudioPlayer::isPollingRequired() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isPollingRequired();
}

void AudioPlayer::poll() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return;
    impl_->poll();
}

//
// AudioPlayer::Impl
//
AudioPlayer::Impl::Impl(AudioPlayer *holder)
    : holder_(holder), player_instance_id_(0), active_source_(), next_source_(), context_(nullptr),
      event_handler_(nullptr), data_source_(), mixer_control_handle_(), looping_(false), fade_in_out_enabled_(false),
      prepared_(false), started_(false), start_pending_(false), playback_completed_(false), last_stopped_position_(0),
      preparing_source_create_reason_(AUDIO_SOURCE_CREATE_REASON_NONE), next_player_(nullptr),
      next_player_instance_id_(0), last_buffering_update_notified_position_(0),
      ts_last_seek_request_(utils::timespec_utils::ZERO()), seek_pending_(false), pending_seek_position_(false),
      current_source_stop_cause_(MIXING_STOP_CAUSE_INVALID)
{
}

AudioPlayer::Impl::~Impl() { release(); }

int AudioPlayer::Impl::initialize(const AudioPlayer::initialize_args_t &args) noexcept
{
    if (!args.context)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.event_handler)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    AudioSystem *audio_system = args.context->getAudioSystem();
    AudioMixer *mixer = (audio_system) ? audio_system->getMixer() : nullptr;

    if (!(audio_system && mixer))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    int result;
    uint32_t player_instance_id = 0;

    // register as audio player client
    result = audio_system->registerAudioPlayer(holder_, player_instance_id);
    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    // register as volume controller client
    AudioMixer::register_source_client_args_t reg_src_args;
    reg_src_args.client = this;
    reg_src_args.event_handler = this;

    result = mixer->registerSourceClient(reg_src_args);
    if (result != OSLMP_RESULT_SUCCESS) {
        (void)audio_system->unregisterAudioPlayer(holder_, player_instance_id);
        return result;
    }

    // update fields
    context_ = args.context;
    player_instance_id_ = player_instance_id;
    event_handler_ = args.event_handler;
    mixer_control_handle_ = reg_src_args.control_handle;
    preparing_source_create_reason_ = AUDIO_SOURCE_CREATE_REASON_NONE;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::release() noexcept
{
    reset();

    AudioSystem *audio_system = (context_) ? context_->getAudioSystem() : nullptr;
    AudioMixer *mixer = (audio_system) ? audio_system->getMixer() : nullptr;

    // unregister volume controller client
    if (mixer) {
        (void)mixer->unregisterSourceClient(mixer_control_handle_);
    }

    // unregister audio player client
    if (audio_system && holder_) {
        (void)audio_system->unregisterAudioPlayer(holder_, player_instance_id_);
    }

    holder_ = nullptr;
    player_instance_id_ = 0;
    context_ = nullptr;
    event_handler_ = nullptr;
    mixer_control_handle_ = AudioMixer::source_client_handle_t();

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::setDataSourcePath(const char *path) noexcept
{
    data_source_.type = AudioSource::DATA_SOURCE_PATH;
    data_source_.path_uri = path;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::setDataSourceUri(const char *uri) noexcept
{
    data_source_.type = AudioSource::DATA_SOURCE_URI;
    data_source_.path_uri = uri;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::setDataSourceFd(int fd) noexcept
{
    data_source_.type = AudioSource::DATA_SOURCE_FD;
    data_source_.fd = fd;
    data_source_.offset = -1;
    data_source_.length = -1;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept
{
    data_source_.type = AudioSource::DATA_SOURCE_FD;
    data_source_.fd = fd;
    data_source_.offset = offset;
    data_source_.length = length;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::createAndStartPreparingAudioSource(std::unique_ptr<AudioSource> &dest_source,
                                                          int32_t seek_position) noexcept
{

    // create new source
    std::unique_ptr<AudioSource> new_source(new (std::nothrow) AudioSource());
    if (!new_source) {
        return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
    }

    // initialize audio source
    int result;
    AudioDataPipeManager *pipe_manager = context_->getAudioSystem()->getPipeManager();
    AudioSourceDataPipe *source_pipe = nullptr;

    result = pipe_manager->obtainSourcePipe(&source_pipe);
    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    {
        AudioSource::initialize_args_t init_args;
        uint32_t sampling_rate = 0;

        context_->getAudioSystem()->getSystemOutputSamplingRate(&sampling_rate);

        init_args.context = context_;
        init_args.pipe_manager = pipe_manager;
        init_args.pipe = source_pipe;
        init_args.sampling_rate = sampling_rate;

        result = new_source->initialize(init_args);
    }

    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    // stop other decoders
    // NOTE: This is REQUIRED because we can't share a single file descriptor between
    // two or more decoders. Otherwise app will be crashed.
    safeStopDecoder(active_source_);
    safeStopDecoder(ready_source_);
    safeStopDecoder(next_source_);
    safeStopDecoder(old_source_);
    safeStopDecoder(preparing_source_);

    // start preparing
    {
        AudioSource::prepare_args_t prepare_args;

        prepare_args.data_source = &data_source_;
        prepare_args.initial_seek_position_msec = seek_position;

        result = new_source->startPreparing(prepare_args);
    }

    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    // update fields
    checkedMoveAudioSource(dest_source, new_source);

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::createAndStartPreparingNextAudioSource(audio_source_create_reason_t reason,
                                                              int32_t seek_position) noexcept
{
    const int result = createAndStartPreparingAudioSource(preparing_source_, seek_position);

    if (result == OSLMP_RESULT_SUCCESS) {
        preparing_source_create_reason_ = reason;
    } else {
        preparing_source_create_reason_ = AUDIO_SOURCE_CREATE_REASON_NONE;
    }

    return result;
}

int AudioPlayer::Impl::refreshCurrentSourceToMixer(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    std::unique_ptr<AudioSource> &current_source = getCurrentSource();

    if (current_source) {
        AudioMixer *mixer = getAudioMixer();
        AudioMixer::attach_update_source_args_t args;
        makeActiveFadeInParams(args, current_source);

        return mixer->attachOrUpdateSourcePipe(args, mixer_da);
    } else {
        return OSLMP_RESULT_SUCCESS;
    }
}

int AudioPlayer::Impl::preparePatialStart() noexcept
{
    stop();

    return createAndStartPreparingNextAudioSource(AUDIO_SOURCE_CREATE_REASON_PREPARING, last_stopped_position_);
}

int AudioPlayer::Impl::pollPreparingSourceOnce(poll_results_info_t &results, bool &need_retry) noexcept
{
    if (!preparing_source_)
        return OSLMP_RESULT_ILLEGAL_STATE;

    AudioSource::prepare_poll_args_t source_poll_args;
    int result;

    result = preparing_source_->pollPreparing(source_poll_args);
    need_retry = source_poll_args.need_retry;

    if (source_poll_args.completed) {
        const audio_source_create_reason_t reason = preparing_source_create_reason_;

        switch (reason) {
        case AUDIO_SOURCE_CREATE_REASON_NONE:
            LOGE("%d preparePatialPollNextSource() - reason = AUDIO_SOURCE_CREATE_REASON_NONE", player_instance_id_);
            break;

        case AUDIO_SOURCE_CREATE_REASON_PREPARING:
            result = handlePreparationCompleteForPrepareing(result);
            results.prepare_completed = true;
            results.prepare_result = result;
            break;

        case AUDIO_SOURCE_CREATE_REASON_SEEKING:
            result = handlePreparationCompleteForSeeking(result);
            results.seek_completed = true;
            results.seek_result = result;
            break;

        case AUDIO_SOURCE_CREATE_REASON_REWINDING:
            result = handlePreparationCompleteForRewinding(result);
            break;

        default:
            LOGE("%d preparePatialPollNextSource() - Unknown reason (%d)", player_instance_id_, reason);
            break;
        }
    }

    return result;
}

void AudioPlayer::Impl::pollPreparingSource(poll_results_info_t &results) noexcept
{
    const int MAX_POLL_AT_ONCE = 10;
    TimeoutChecker timeout(1000);

    for (int i = 0; i < MAX_POLL_AT_ONCE; ++i) {
        bool need_retry = false;
        const int result = pollPreparingSourceOnce(results, need_retry);

        if ((!need_retry) || (result != OSLMP_RESULT_SUCCESS) || results.seek_completed || results.prepare_completed) {
            break;
        }

        if (!timeout.check()) {
            break;
        }
    }
}

void AudioPlayer::Impl::pollCheckBufferingUpdate(poll_results_info_t &results) noexcept
{
    if (!metadata_.duration.available())
        return;

    const int32_t duration = metadata_.duration.get();

    if (duration <= 0)
        return;

    int32_t &prev_buffered_pos = last_buffering_update_notified_position_;

    if (active_source_ && active_source_->isStarted() && active_source_->isNetworkSource()) {

        int32_t cur_buffered_pos = 0;

        if (active_source_->getBufferedPosition(&cur_buffered_pos) == OSLMP_RESULT_SUCCESS) {
            const int32_t prev_percentage = static_cast<int32_t>((prev_buffered_pos * 100LL) / duration);
            const int32_t cur_percentage = static_cast<int32_t>((cur_buffered_pos * 100LL) / duration);

            if (prev_percentage != cur_percentage) {
                results.buffering_status_updated = true;
                results.bufferred_percentage = cur_percentage;

                prev_buffered_pos = cur_buffered_pos;
            }
        }
    } else {
        prev_buffered_pos = 0;
    }
}

int AudioPlayer::Impl::handlePreparationCompleteForPrepareing(int result) noexcept
{

    // obatain metadata
    if (result == OSLMP_RESULT_SUCCESS) {
        result = preparing_source_->getMetaData(&metadata_);

        if (result != OSLMP_RESULT_SUCCESS) {
            if (!metadata_.isValid()) {
                result = OSLMP_RESULT_ILLEGAL_STATE;
            }
        }
    }

    {
        AudioMixer *mixer = getAudioMixer();
        AudioMixer::DeferredApplication mixer_da(mixer);

        if (result == OSLMP_RESULT_SUCCESS) {
            AudioMixer::attach_update_source_args_t args;
            makeActiveFadeInParams(args, preparing_source_);
            result = mixer->attachOrUpdateSourcePipe(args, &mixer_da);
        }

        // update fields
        if (result == OSLMP_RESULT_SUCCESS) {
            movePreparingSourceToReadySource(&mixer_da);
            prepared_ = true;
        } else {
            releaseAllAudioSources(&mixer_da);
        }
    }

    return result;
}

int AudioPlayer::Impl::handlePreparationCompleteForSeeking(int result) noexcept
{
    AudioMixer *mixer = getAudioMixer();

    if (result == OSLMP_RESULT_SUCCESS) {
        if (started_) {
            AudioMixer::DeferredApplication mixer_da(mixer);

            // fade out
            {
                AudioMixer::attach_update_source_args_t active_args;
                makeOldFadeOutParams(active_args, active_source_);
                result = mixer->attachOrUpdateSourcePipe(active_args, &mixer_da);
            }

            // fade in
            if (result == OSLMP_RESULT_SUCCESS) {
                AudioMixer::attach_update_source_args_t next_args;
                makeActiveFadeInParams(next_args, preparing_source_);
                next_args.operation = AudioMixer::OPERATION_START;
                result = mixer->attachOrUpdateSourcePipe(next_args, &mixer_da);
            }

            // update fields
            if (result == OSLMP_RESULT_SUCCESS) {
                moveActiveSourceToOldSource(&mixer_da);
                movePreparingSourceToReadySource(&mixer_da);
                moveReadySourceToActiveSource(&mixer_da);
            } else {
                releaseAllAudioSources(&mixer_da);
            }
        } else {
            AudioMixer::DeferredApplication mixer_da(mixer);

            {
                AudioMixer::attach_update_source_args_t next_args;
                makeActiveFadeInParams(next_args, preparing_source_);

                int32_t position;
                preparing_source_->getCurrentPosition(&position);

                if (position == 0) {
                    next_args.mix_phase = 1.0f;
                    next_args.mix_phase_override = true;
                }

                result = mixer->attachOrUpdateSourcePipe(next_args, &mixer_da);
            }

            if (result == OSLMP_RESULT_SUCCESS) {
                moveActiveSourceToOldSource(&mixer_da);
                movePreparingSourceToReadySource(&mixer_da);
            } else {
                releasePreparingAudioSource(&mixer_da);
            }
        }
    } else {
        releasePreparingAudioSource();
    }

    return result;
}

int AudioPlayer::Impl::handlePreparationCompleteForRewinding(int result) noexcept
{
    AudioMixer *mixer = getAudioMixer();
    AudioMixer::DeferredApplication mixer_da(mixer);
    const bool set_as_active = (!ready_source_ && !active_source_);

    if (result == OSLMP_RESULT_SUCCESS) {
        AudioMixer::attach_update_source_args_t args;
        if (set_as_active) {
            makeActiveFadeInParams(args, preparing_source_);
        } else {
            makeNextFadeInParams(args, preparing_source_);
        }
        result = mixer->attachOrUpdateSourcePipe(args, &mixer_da);
    }

    // update fields
    if (result == OSLMP_RESULT_SUCCESS) {
        if (set_as_active) {
            movePreparingSourceToReadySource(&mixer_da);

            if (started_) {
                startReadySourceInternal(&mixer_da);
            }
        } else {
            movePreparingSourceToNextSource(&mixer_da);
        }
    } else {
        releaseAllAudioSources(&mixer_da);
    }

    return result;
}

void AudioPlayer::Impl::pollHandleStartPending() noexcept
{
    int state = 0; // 0: not started, 1: started, 2: start next player
    int result;
    Impl *next_player_impl = getNextPlayerImpl();

    if (!looping_ && next_player_impl) {
        result = next_player_impl->startAsNextPlayer();

        if (result == OSLMP_RESULT_SUCCESS) {
            next_player_impl->raiseOnPlayerStartedAsNextPlayerEvent();
            state = 2;
        }
    }

    if (!state && !ready_source_ && next_source_) {
        moveNextSourceToReadySource();
    }

    if (!state && ready_source_) {
        result = startReadySourceInternal();

        if (result == OSLMP_RESULT_SUCCESS) {
            state = 1;
        }
    }

    if (state) {
        if (state == 1) {
            setStartedStatus(true, true);
        } else {
            start_pending_ = false;
        }
    }
}

void AudioPlayer::Impl::pollHandlePendingSeekRequest() noexcept
{
    timespec now;

    utils::timespec_utils::get_current_time(now);

    if (utils::timespec_utils::sub_ret_ms(now, ts_last_seek_request_) < MIN_SEEK_REQUEST_PERIOD_MSEC) {
        return;
    }

    int result;
    const int32_t msec = pending_seek_position_;

    seek_pending_ = false;
    pending_seek_position_ = 0;

    // release preparing audio source
    releasePreparingAudioSource();

    result = createAndStartPreparingNextAudioSource(AUDIO_SOURCE_CREATE_REASON_SEEKING, msec);

    if (result == OSLMP_RESULT_SUCCESS) {
        ts_last_seek_request_ = now;
    }
}

void AudioPlayer::Impl::pollHandlePlaybackCompletion(poll_results_info_t &results) noexcept
{
    std::unique_ptr<AudioSource> &current_source = getCurrentSource();
    const AudioMixer::mixing_stop_cause_t stop_cause = current_source_stop_cause_;

    if (!safeIsPlaybackCompleted(current_source))
        return;

    if (!(stop_cause == AudioMixer::MIXING_STOP_CAUSE_END_OF_DATA_NO_TRIGGERED ||
        stop_cause == AudioMixer::MIXING_STOP_CAUSE_END_OF_DATA_TRIGGERED_NO_LOOPING_SOURCE ||
        stop_cause == AudioMixer::MIXING_STOP_CAUSE_END_OF_DATA_TRIGGERED_LOOPING_SOURCE)) {
        return;
    }

    const AudioSource::playback_completion_type_t comp_type = safeGetPlaybackCompletionType(current_source);
    const bool looped = (stop_cause == AudioMixer::MIXING_STOP_CAUSE_END_OF_DATA_TRIGGERED_LOOPING_SOURCE);
    const bool next_player_started = (stop_cause == AudioMixer::MIXING_STOP_CAUSE_END_OF_DATA_TRIGGERED_NO_LOOPING_SOURCE);

    if (looped || (comp_type == AudioSource::PLAYBACK_COMPLETED_WITH_LOOP_POINT)) {
        if (!(looped && (comp_type == AudioSource::PLAYBACK_COMPLETED_WITH_LOOP_POINT))) {
            LOGE("pollHandlePlaybackCompletion() - Unexpected condition: stop_cause = %d, comp_type = %d", stop_cause, comp_type);
        }
    }

    int32_t completed_position = 0;
    current_source->getCurrentPosition(&completed_position);

    {
        AudioMixer *mixer = getAudioMixer();
        AudioMixer::DeferredApplication mixer_da(mixer);

        if (started_) {
            releaseOldAudioSource(&mixer_da);
            releaseActiveAudioSource(&mixer_da);

            if (next_source_) {
                moveNextSourceToReadySource(&mixer_da);
            }

            if (!looped) {
                playback_completed_ = true;
                last_stopped_position_ = completed_position;
                setStartedStatus(false, false);
            } else {
                moveReadySourceToActiveSource(&mixer_da);
            }
        } else {
            releaseReadyAudioSource(&mixer_da);
            if (next_source_) {
                moveNextSourceToReadySource(&mixer_da);
            }

            playback_completed_ = true;
            last_stopped_position_ = completed_position;
        }

        refreshCurrentSourceToMixer(&mixer_da);
    }

    if (!next_source_ && !preparing_source_ && !active_source_ && !ready_source_) {
        createAndStartPreparingNextAudioSource(AUDIO_SOURCE_CREATE_REASON_REWINDING, 0);
    }

    results.playback_completed = (playback_completed_ && !looped && !next_player_started);
    results.playback_looped = looped;
    results.next_player_started = next_player_started;
}

bool AudioPlayer::Impl::isPreparing() const noexcept
{
    if (!prepared_) {
        return static_cast<bool>(preparing_source_);
    } else {
        return false;
    }
}

int AudioPlayer::Impl::start() noexcept
{
    if (started_ || start_pending_) {
        return OSLMP_RESULT_SUCCESS;
    }

    if (!prepared_) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    int result;

    if (!ready_source_) {
        start_pending_ = true;
        playback_completed_ = false;

        if (!next_source_ && !preparing_source_) {
            createAndStartPreparingNextAudioSource(AUDIO_SOURCE_CREATE_REASON_REWINDING, 0);
        }

        result = OSLMP_RESULT_SUCCESS;
    } else {
        result = startReadySourceInternal();
    }

    return result;
}

int AudioPlayer::Impl::startAsNextPlayer() noexcept
{
    if (started_ || start_pending_) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    if (!prepared_) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    if (!ready_source_) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    int result;

    if (safeIsPlaybackCompleted(ready_source_)) {
        if (next_source_) {
            moveNextSourceToReadySource();
            result = startReadySourceInternal();
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }
    } else {
        result = startReadySourceInternal();
    }

    return result;
}

int AudioPlayer::Impl::startReadySourceInternal(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    if (!safeIsPrepared(ready_source_)) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    int32_t cur_position = 0;
    int result = ready_source_->getCurrentPosition(&cur_position);

    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    AudioMixer *mixer = getAudioMixer();
    AudioMixer::attach_update_source_args_t args;

    makeActiveFadeInParams(args, ready_source_);
    args.operation = AudioMixer::OPERATION_START;
    if (cur_position == 0) {
        args.mix_phase = 1.0f;
        args.mix_phase_override = true;
    }

    result = mixer->attachOrUpdateSourcePipe(args, mixer_da);

    if (result == OSLMP_RESULT_SUCCESS) {
        moveReadySourceToActiveSource(mixer_da);
        playback_completed_ = false;
        setStartedStatus(true, true);
    }

    return result;
}

int AudioPlayer::Impl::pause() noexcept
{
    if (!(started_ || start_pending_)) {
        return OSLMP_RESULT_SUCCESS;
    }

    int result;

    if (started_) {
        if (active_source_) {
            // fade out
            AudioMixer *mixer = getAudioMixer();
            AudioMixer::DeferredApplication mixer_da(mixer);

            AudioMixer::attach_update_source_args_t args;
            makeActiveFadeOutParams(args, active_source_);
            result = mixer->attachOrUpdateSourcePipe(args, &mixer_da);

            if (result == OSLMP_RESULT_SUCCESS) {
                moveActiveSourceToReadySource(&mixer_da);
                setStartedStatus(false, true);
            }
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }
    } else {
        start_pending_ = false;
        result = OSLMP_RESULT_SUCCESS;
    }

    return result;
}

int AudioPlayer::Impl::stop() noexcept
{
    // save playback position
    if (active_source_) {
        active_source_->getCurrentPosition(&last_stopped_position_);
    } else if (ready_source_) {
        ready_source_->getCurrentPosition(&last_stopped_position_);
    }

    // release audio sources
    releaseAllAudioSources();

    // clear fields
    prepared_ = false;
    playback_completed_ = false;
    seek_pending_ = false;
    pending_seek_position_ = 0;
    utils::timespec_utils::set_zero(ts_last_seek_request_);

    setStartedStatus(false, true);

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::reset() noexcept
{
    stop();
    setLooping(false);

    clear(data_source_);
    metadata_.clear();
    prepared_ = false;
    last_stopped_position_ = 0;
    current_source_stop_cause_ = MIXING_STOP_CAUSE_INVALID;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::setVolume(float left_volume, float right_volume) noexcept
{
    AudioSystem *audio_system = (context_) ? context_->getAudioSystem() : nullptr;
    AudioMixer *mixer = (audio_system) ? audio_system->getMixer() : nullptr;

    if (!mixer) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    return mixer->setVolume(mixer_control_handle_, left_volume, right_volume);
}

int AudioPlayer::Impl::getAudioSessionId(int32_t *audio_session_id) noexcept
{
    if (!audio_session_id)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    AudioSystem *audio_system = (context_) ? context_->getAudioSystem() : nullptr;
    int result;

    if (audio_system) {
        result = audio_system->getAudioSessionId(audio_session_id);
    } else {
        result = OSLMP_RESULT_ILLEGAL_STATE;
        (*audio_session_id) = 0;
    }

    return result;
}

int AudioPlayer::Impl::getDuration(int32_t *duration) noexcept
{
    if (!duration)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    int result;

    if (metadata_.duration.available()) {
        result = OSLMP_RESULT_SUCCESS;
        (*duration) = metadata_.duration.get();
    } else {
        result = OSLMP_RESULT_ILLEGAL_STATE;
        (*duration) = 0;
    }

    return result;
}

int AudioPlayer::Impl::getCurrentPosition(int32_t *position) noexcept
{
    int result;

    if (!position)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*position) = 0;

    if (playback_completed_) {
        // playback completed
        *position = last_stopped_position_;
        result = OSLMP_RESULT_SUCCESS;
    } else if (isSeeking()) {
        // seeking
        *position = preparing_source_->getInitialSeekPosition();
        result = OSLMP_RESULT_SUCCESS;
    } else if (seek_pending_) {
        // seek pending
        *position = pending_seek_position_;
        result = OSLMP_RESULT_SUCCESS;
    } else if (active_source_) {
        // playing
        result = active_source_->getCurrentPosition(position);
    } else if (ready_source_) {
        // paused
        result = ready_source_->getCurrentPosition(position);
    } else {
        *position = 0;
        result = OSLMP_RESULT_SUCCESS;
    }

    return result;
}

int AudioPlayer::Impl::seekTo(int32_t msec) noexcept
{
    if (!prepared_) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    // check whether the specified seek position is the same as
    // current seeking position.
    int32_t current_seeking_position = -1;
    if (isSeeking()) {
        current_seeking_position = preparing_source_->getInitialSeekPosition();
    } else if (seek_pending_) {
        current_seeking_position = pending_seek_position_;
    }

    if (current_seeking_position == msec) {
        return OSLMP_RESULT_SUCCESS;
    }

    // check seek request time
    timespec now;
    bool defer_request = false;

    utils::timespec_utils::get_current_time(now);

    if (!utils::timespec_utils::is_zero(ts_last_seek_request_)) {
        if (utils::timespec_utils::sub_ret_ms(now, ts_last_seek_request_) < MIN_SEEK_REQUEST_PERIOD_MSEC) {
            defer_request = true;
        }
    }

    // update fields
    playback_completed_ = false;

    int result;

    if (!defer_request) {
        seek_pending_ = false;
        pending_seek_position_ = 0;

        // release preparing audio source
        releasePreparingAudioSource();

        result = createAndStartPreparingNextAudioSource(AUDIO_SOURCE_CREATE_REASON_SEEKING, msec);

        if (result == OSLMP_RESULT_SUCCESS) {
            ts_last_seek_request_ = now;
        }
    } else {
        // fade out
        if (!seek_pending_ && active_source_) {
            AudioMixer *mixer = getAudioMixer();
            AudioMixer::DeferredApplication mixer_da(mixer);

            AudioMixer::attach_update_source_args_t args;
            makeActiveFadeOutParams(args, active_source_);
            (void)mixer->attachOrUpdateSourcePipe(args, &mixer_da);
        }

        // release preparing audio source
        releasePreparingAudioSource();

        seek_pending_ = true;
        pending_seek_position_ = msec;
        result = OSLMP_RESULT_SUCCESS;
    }

    return result;
}

int AudioPlayer::Impl::setLooping(bool looping) noexcept
{
    if (looping_ == looping)
        return OSLMP_RESULT_SUCCESS;

    looping_ = looping;

    AudioMixer *mixer = getAudioMixer();

    if (mixer) {
        mixer->setLooping(mixer_control_handle_, looping);
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::isLooping(bool *looping) noexcept
{
    if (!looping)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*looping) = looping_;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::setAudioStreamType(int stream_type) noexcept
{
    AudioSystem *audio_system = context_->getAudioSystem();
    audio_system->setAudioStreamType(stream_type);
    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::getAudioStreamType(int *stream_type) const noexcept
{
    AudioSystem *audio_system = context_->getAudioSystem();
    return audio_system->getAudioStreamType(stream_type);
}

int AudioPlayer::Impl::setNextMediaPlayer(AudioPlayer *next) noexcept
{
    if (next_player_ == next) {
        return OSLMP_RESULT_SUCCESS;
    }

    // update fields
    next_player_ = next;
    next_player_instance_id_ = (next) ? next->impl_->player_instance_id_ : 0;

    refreshCurrentSourceToMixer();

    return OSLMP_RESULT_SUCCESS;
}

int AudioPlayer::Impl::attachAuxEffect(int effect_id) noexcept
{
    return context_->getAudioSystem()->selectActiveAuxEffect(effect_id);
}

int AudioPlayer::Impl::setAuxEffectSendLevel(float level) noexcept
{
    return context_->getAudioSystem()->setAuxEffectSendLevel(level);
}

int AudioPlayer::Impl::setFadeInOutEnabled(bool enabled) noexcept
{
    fade_in_out_enabled_ = enabled;
    return OSLMP_RESULT_SUCCESS;
}

bool AudioPlayer::Impl::isPollingRequired() const noexcept
{
    if (started_)
        return true;
    if (start_pending_)
        return true;
    if (safeIsPreparing(preparing_source_))
        return true;
    if (seek_pending_)
        return true;

    return false;
}

void AudioPlayer::Impl::poll() noexcept
{
    assert(isPollingRequired());

    poll_results_info_t results;

    // handle pending seek request
    if (seek_pending_) {
        pollHandlePendingSeekRequest();
    }

    // next source creation
    if (checkConditionForRewindedSourceCreation()) {
        createAndStartPreparingNextAudioSource(AUDIO_SOURCE_CREATE_REASON_REWINDING, 0);
    }

    // prepare source
    if (safeIsPreparing(preparing_source_)) {
        pollPreparingSource(results);
    }

    // check buffering update
    pollCheckBufferingUpdate(results);

    // handle pending start
    if (start_pending_) {
        pollHandleStartPending();
    }

    // check playback completion
    pollHandlePlaybackCompletion(results);

    // raise events
    if (results.buffering_status_updated) {
        raiseOnDecoderBufferingUpdate(results.bufferred_percentage);
    }
    if (results.prepare_completed) {
        raiseOnPrepareCompleted(results.prepare_result);
    }
    if (results.seek_completed) {
        raiseOnSeekCompleted(results.seek_result);
    }
    if (results.playback_completed) {
        raiseOnPlaybackCompletionEvent(AudioPlayer::kCompletionNormal);
    }
    if (results.next_player_started) {
        raiseOnPlaybackCompletionEvent(AudioPlayer::kCompletionStartNextPlayer);
    }
    if (results.playback_looped) {
        raiseOnPlaybackCompletionEvent(AudioPlayer::kPlaybackLooped);
    }
}

bool AudioPlayer::Impl::checkConditionForRewindedSourceCreation() const noexcept
{
    const std::unique_ptr<AudioSource> &current_source = getCurrentSource();

    if (prepared_ && current_source && !preparing_source_ && !next_source_) {
        return current_source->isDecoderReachedToEndOfData();
    }

    return false;
}

void AudioPlayer::Impl::onMixingStarted(AudioSourceDataPipe *pipe, AudioMixer::mixing_start_cause_t cause) noexcept
{
    LOGD("%d onMixingStarted(pipe = %p, cause = %d)", player_instance_id_, pipe, cause);

    if (safeGetPipe(active_source_) == pipe) {
        LOGD("onMixingStarted() - pipe == ACTIVE");
        active_source_->start();
    } else if (safeGetPipe(ready_source_) == pipe) {
        LOGD("onMixingStarted() - pipe == READY");
        if (!started_ && cause == AudioMixer::MIXING_START_CAUSE_NO_LOOP_TRIGGERED) {
            ready_source_->start();

            moveReadySourceToActiveSource();

            playback_completed_ = false;
            setStartedStatus(true, true);

            raiseOnPlayerStartedAsNextPlayerEvent();
        } else {
            LOGW("%d onMixingStarted(pipe = %p (ready), started = %d, cause = %d)", player_instance_id_, pipe, started_,
                 cause);
        }
    } else if (safeGetPipe(next_source_) == pipe) {
        LOGD("onMixingStarted() - pipe == NEXT");
        if (cause == AudioMixer::MIXING_START_CAUSE_LOOP_TRIGGERED) {
            next_source_->start();
        } else {
            LOGW("%d onMixingStarted(pipe = %p (next), cause = %d)", player_instance_id_, pipe, cause);
        }
    } else if (safeGetPipe(old_source_) == pipe) {
        LOGD("onMixingStarted() - pipe == OLD");
        old_source_->start();
    } else {
        LOGD("onMixingStarted() - pipe == ???");
    }
}

void AudioPlayer::Impl::onMixingStopped(AudioSourceDataPipe *pipe, AudioMixer::mixing_stop_cause_t cause) noexcept
{
    LOGD("%d onMixingStopped(pipe = %p, cause = %d)", player_instance_id_, pipe, cause);

    if (safeGetPipe(getCurrentSource()) == pipe) {
        current_source_stop_cause_ = cause;
    }

    if (safeGetPipe(active_source_) == pipe) {
        active_source_->pause();
    } else if (safeGetPipe(ready_source_) == pipe) {
        ready_source_->pause();
        // NOTE:
        // still keep the pipe attached to the mixer in order
        // to wait for start request as a next player
    } else if (safeGetPipe(next_source_) == pipe) {
        next_source_->pause();
    } else if (safeGetPipe(old_source_) == pipe) {
        releaseOldAudioSource();
    }
}

void AudioPlayer::Impl::detachFromMixer(const std::unique_ptr<AudioSource> &source,
                                        AudioMixer::DeferredApplication *mixer_da) noexcept
{
    AudioMixer::DeferredApplication local_mixer_da((mixer_da) ? nullptr : getAudioMixer());
    AudioMixer::DeferredApplication *da = (mixer_da) ? mixer_da : &local_mixer_da;

    AudioSourceDataPipe *pipe = source->getPipe();
    AudioMixer *mixer = getAudioMixer();

    assert(mixer);
    assert(da);
    assert(da->isValid());

    if (mixer && pipe) {
        mixer->detachSourcePipe(pipe, da);
    }
}

void AudioPlayer::Impl::releaseAudioSource(std::unique_ptr<AudioSource> &source,
                                           AudioMixer::DeferredApplication *mixer_da) noexcept
{
    if (!source)
        return;

    detachFromMixer(source, mixer_da);

    source.reset();
}

void AudioPlayer::Impl::releasePreparingAudioSource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseAudioSource(preparing_source_, mixer_da);
    preparing_source_create_reason_ = AUDIO_SOURCE_CREATE_REASON_NONE;
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("releasePreparingAudioSource");
}

void AudioPlayer::Impl::releaseActiveAudioSource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseAudioSource(active_source_, mixer_da);
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("releaseActiveAudioSource");
}

void AudioPlayer::Impl::releaseReadyAudioSource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseAudioSource(ready_source_, mixer_da);
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("releaseReadyAudioSource");
}

void AudioPlayer::Impl::releaseNextAudioSource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseAudioSource(next_source_, mixer_da);
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("releaseNextAudioSource");
}

void AudioPlayer::Impl::releaseOldAudioSource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseAudioSource(old_source_, mixer_da);
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("releaseOldAudioSource");
}

void AudioPlayer::Impl::movePreparingSourceToReadySource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseReadyAudioSource(mixer_da);
    checkedMoveAudioSource(ready_source_, preparing_source_);
    preparing_source_create_reason_ = AUDIO_SOURCE_CREATE_REASON_NONE;
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("movePreparingSourceToReadySource");
}

void AudioPlayer::Impl::movePreparingSourceToNextSource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseNextAudioSource(mixer_da);
    checkedMoveAudioSource(next_source_, preparing_source_);
    preparing_source_create_reason_ = AUDIO_SOURCE_CREATE_REASON_NONE;
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("movePreparingSourceToNextSource");
}

void AudioPlayer::Impl::moveNextSourceToReadySource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseReadyAudioSource(mixer_da);
    checkedMoveAudioSource(ready_source_, next_source_);
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("moveNextSourceToReadySource");
}

void AudioPlayer::Impl::moveActiveSourceToReadySource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    checkedMoveAudioSource(ready_source_, active_source_);
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("moveActiveSourceToReadySource");
}

void AudioPlayer::Impl::moveReadySourceToActiveSource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    checkedMoveAudioSource(active_source_, ready_source_);
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("moveReadySourceToActiveSource");
}

void AudioPlayer::Impl::moveActiveSourceToOldSource(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    releaseOldAudioSource(mixer_da);
    checkedMoveAudioSource(old_source_, active_source_);
    DEBUG_CURRENT_AUDIO_SOURCE_STATE("moveActiveSourceToOldSource");
}

void AudioPlayer::Impl::releaseAllAudioSources(AudioMixer::DeferredApplication *mixer_da) noexcept
{
    AudioMixer::DeferredApplication local_mixer_da((mixer_da) ? nullptr : getAudioMixer());
    AudioMixer::DeferredApplication *da = (mixer_da) ? mixer_da : &local_mixer_da;

    DEBUG_CURRENT_AUDIO_SOURCE_STATE("PRE releaseAllAudioSources");

    releasePreparingAudioSource(da);
    releaseOldAudioSource(da);
    releaseNextAudioSource(da);
    releaseActiveAudioSource(da);
    releaseReadyAudioSource(da);
}

AudioPlayer::Impl *AudioPlayer::Impl::getNextPlayerImpl() const noexcept
{
    AudioSystem *as = context_->getAudioSystem();

    if (next_player_ && as && as->checkAudioPlayerIsAlive(next_player_, next_player_instance_id_)) {
        return next_player_->impl_.get();
    } else {
        return nullptr;
    }
}

std::unique_ptr<AudioSource> &AudioPlayer::Impl::getCurrentSource() noexcept
{
    return (active_source_) ? active_source_ : ready_source_;
}

const std::unique_ptr<AudioSource> &AudioPlayer::Impl::getCurrentSource() const noexcept
{
    return (active_source_) ? active_source_ : ready_source_;
}

AudioMixer *AudioPlayer::Impl::getAudioMixer() const noexcept
{
    AudioSystem *as = (context_) ? context_->getAudioSystem() : nullptr;
    AudioMixer *mixer = (as) ? as->getMixer() : nullptr;
    return mixer;
}

void AudioPlayer::Impl::setHandle(AudioMixer::attach_update_source_args_t &args) const noexcept
{
    args.handle = mixer_control_handle_;
}

void AudioPlayer::Impl::setTriggerConditions(AudioMixer::attach_update_source_args_t &args) const noexcept
{
    args.trigger_loop_mode = AudioMixer::TRIGGER_ON_END_OF_DATA;
    args.trigger_loop_target = mixer_control_handle_;
    args.trigger_loop_source_no = MIX_SOURCE_NO_NEXT;
    Impl *player_impl = getNextPlayerImpl();
    if (player_impl) {
        args.trigger_no_loop_mode = AudioMixer::TRIGGER_ON_END_OF_DATA;
        args.trigger_no_loop_target = player_impl->mixer_control_handle_;
        args.trigger_no_loop_source_no = MIX_SOURCE_NO_ACTIVE;
    } else {
        args.trigger_no_loop_mode = AudioMixer::TRIGGER_NONE;
        args.trigger_no_loop_target = AudioMixer::source_client_handle_t();
        args.trigger_no_loop_source_no = MIX_SOURCE_NO_DEFAULT;
    }
}

void AudioPlayer::Impl::clearTriggerConditions(AudioMixer::attach_update_source_args_t &args) const noexcept
{
    args.trigger_loop_mode = AudioMixer::TRIGGER_NONE;
    args.trigger_loop_target = AudioMixer::source_client_handle_t();
    args.trigger_loop_source_no = MIX_SOURCE_NO_ACTIVE;
    args.trigger_no_loop_mode = AudioMixer::TRIGGER_NONE;
    args.trigger_no_loop_target = AudioMixer::source_client_handle_t();
    args.trigger_no_loop_source_no = MIX_SOURCE_NO_DEFAULT;
}

void AudioPlayer::Impl::setStopConditions(AudioMixer::attach_update_source_args_t &args) const noexcept
{
    args.stop_cond = AudioMixer::STOP_ON_PLAYBACK_END | AudioMixer::STOP_AFTER_FADE_OUT;
}

void AudioPlayer::Impl::setMixModeFadeIn(AudioMixer::attach_update_source_args_t &args) const noexcept
{
    args.mix_mode = (fade_in_out_enabled_) ? AudioMixer::MIX_MODE_LONG_FADE_IN : AudioMixer::MIX_MODE_SHORT_FADE_IN;
}

void AudioPlayer::Impl::setMixModeFadeOut(AudioMixer::attach_update_source_args_t &args) const noexcept
{
    args.mix_mode = (fade_in_out_enabled_) ? AudioMixer::MIX_MODE_LONG_FADE_OUT : AudioMixer::MIX_MODE_SHORT_FADE_OUT;
}

void AudioPlayer::Impl::makeActiveFadeInParams(AudioMixer::attach_update_source_args_t &args,
                                               std::unique_ptr<AudioSource> &source) const noexcept
{
    setHandle(args);
    setTriggerConditions(args);
    setStopConditions(args);
    setMixModeFadeIn(args);

    args.operation = AudioMixer::OPERATION_NONE;
    args.source_pipe = source->getPipe();
    args.source_no = MIX_SOURCE_NO_ACTIVE;
}

void AudioPlayer::Impl::makeNextFadeInParams(AudioMixer::attach_update_source_args_t &args,
                                             std::unique_ptr<AudioSource> &source) const noexcept
{
    setHandle(args);
    setTriggerConditions(args);
    setStopConditions(args);
    setMixModeFadeIn(args);

    args.operation = AudioMixer::OPERATION_NONE;
    args.source_pipe = source->getPipe();
    args.source_no = MIX_SOURCE_NO_NEXT;
}

void AudioPlayer::Impl::makeActiveFadeOutParams(AudioMixer::attach_update_source_args_t &args,
                                                std::unique_ptr<AudioSource> &source) const noexcept
{
    setHandle(args);
    clearTriggerConditions(args);
    setStopConditions(args);
    setMixModeFadeOut(args);

    args.operation = AudioMixer::OPERATION_NONE;
    args.source_pipe = source->getPipe();
    args.source_no = MIX_SOURCE_NO_ACTIVE;
}

void AudioPlayer::Impl::makeOldFadeOutParams(AudioMixer::attach_update_source_args_t &args,
                                             std::unique_ptr<AudioSource> &source) const noexcept
{
    setHandle(args);
    setTriggerConditions(args);
    setStopConditions(args);
    setMixModeFadeOut(args);

    args.operation = AudioMixer::OPERATION_NONE;
    args.source_pipe = source->getPipe();
    args.source_no = MIX_SOURCE_NO_OLD;
}

void AudioPlayer::Impl::setStartedStatus(bool started, bool clear_pending) noexcept
{
    const bool changed = started_ ^ started;

    started_ = started;

    if (clear_pending) {
        start_pending_ = false;
    }

    if (changed) {
        AudioSystem *audio_system = context_->getAudioSystem();
        if (audio_system) {
            audio_system->notifyAudioPlayerState(holder_, player_instance_id_, started);
        }
    }
}

bool AudioPlayer::Impl::isSeeking() const noexcept
{
    return (preparing_source_ && (preparing_source_create_reason_ == AUDIO_SOURCE_CREATE_REASON_SEEKING));
}

void AudioPlayer::Impl::raiseOnPlayerStartedAsNextPlayerEvent() noexcept
{
    if (event_handler_) {
        event_handler_->onPlayerStartedAsNextPlayer();
    }
}

void AudioPlayer::Impl::raiseOnPlaybackCompletionEvent(AudioPlayer::PlaybackCompletionType completion_type) noexcept
{
    if (event_handler_) {
        event_handler_->onPlaybackCompletion(completion_type);
    }
}

void AudioPlayer::Impl::raiseOnDecoderBufferingUpdate(int32_t percent) noexcept
{
    if (event_handler_) {
        event_handler_->onDecoderBufferingUpdate(percent);
    }
}

void AudioPlayer::Impl::raiseOnSeekCompleted(int seek_result) noexcept
{
    if (event_handler_) {
        event_handler_->onSeekCompleted(seek_result);
    }
}

void AudioPlayer::Impl::raiseOnPrepareCompleted(int prepare_result) noexcept
{
    if (event_handler_) {
        event_handler_->onPrepareCompleted(prepare_result);
    }
}

#ifdef OUTPUT_DETAIL_DEBUG_LOGS
void AudioPlayer::Impl::dumpCurrentAudioSourceState(const char *msg) const noexcept
{
    LOGD("msg: %s, active = %p (%p), ready = %p (%p), next = %p (%p), old = %p (%p), preparing_source_ = %p (%p)", msg,
         active_source_.get(), safeGetPipe(active_source_), ready_source_.get(), safeGetPipe(ready_source_),
         next_source_.get(), safeGetPipe(next_source_), old_source_.get(), safeGetPipe(old_source_),
         preparing_source_.get(), safeGetPipe(preparing_source_));
}
#endif

} // namespace impl
} // namespace oslmp
