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

// #define LOG_TAG "OpenSLMediaPlayerImpl"

#include "oslmp/impl/OpenSLMediaPlayerImpl.hpp"

#include <cassert>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerInternalDefs.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/OpenSLMediaPlayerThreadMessage.hpp"
#include "oslmp/utils/timespec_utils.hpp"

//
// constants
//

#define INTERNAL_MESSAGE_CATEGORY 0

//
// macros
//
#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define UNUSED_ARG(arg) (void)(arg)

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define STATE_MASK(state) (1 << (state))
#define SMASK(state_name) STATE_MASK(STATE_##state_name)

#define STATE_MASK_ANY                                                                                                 \
    (SMASK(IDLE) | SMASK(INITIALIZED) | SMASK(PREPARING_SYNC) | SMASK(PREPARING_ASYNC) | SMASK(PREPARED) |             \
     SMASK(STARTED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED) | SMASK(STOPPED) | SMASK(ERROR))

#define STATE_MASK_PREPARING (SMASK(PREPARING_SYNC) | SMASK(PREPARING_ASYNC))

#define STATE_MASK_PREPARED (SMASK(PREPARED) | SMASK(STARTED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED))

#define CHECK_EXT_MODULE_NO(no) (((no) >= 1) && ((no) <= NUM_EXTENSIONS))

namespace oslmp {

using namespace ::oslmp::impl;

typedef OpenSLMediaPlayerInternalUtils InternalUtils;
typedef OpenSLMediaPlayerInternalContext InternalContext;

//
// message codes
//
enum {
    MSG_NOP,
    MSG_SET_DATA_SOURCE_PATH,
    MSG_SET_DATA_SOURCE_URI,
    MSG_SET_DATA_SOURCE_FD,
    MSG_SET_DATA_SOURCE_FD_OFFSET_LENGTH,
    MSG_PREPARE,
    MSG_PREPARE_ASYNC,
    MSG_START,
    MSG_STOP,
    MSG_PAUSE,
    MSG_RESET,
    MSG_SET_VOLUME,
    MSG_GET_DURATION,
    MSG_GET_CURRENT_POSITION,
    MSG_SEEK_TO,
    MSG_SET_LOOPING,
    MSG_GET_IS_LOOPING,
    MSG_GET_IS_PLAYING,
    MSG_SET_AUDIO_STREAM_TYPE,
    MSG_SET_NEXT_MEDIA_PLAYER,
    MSG_ATTACH_AUX_EFFECT,
    MSG_SET_AUX_EFFECT_SEND_LEVEL,
};

//
// message structures
//
struct msg_blob_set_data_source_path {
    const char *path;
};

struct msg_blob_set_data_source_uri {
    const char *uri;
};

struct msg_blob_set_data_source_fd {
    int fd;
    int64_t offset;
    int64_t length;
};

struct msg_blob_set_volume {
    float leftVolume;
    float rightVolume;
};

struct msg_blob_get_duration {
    int32_t *duration;
};

struct msg_blob_get_current_position {
    int32_t *position;
};

struct msg_blob_seek_to {
    int32_t msec;
};

struct msg_blob_set_looping {
    bool looping;
};

struct msg_blob_get_is_looping {
    bool *looping;
};

struct msg_blob_get_is_playing {
    bool *playing;
};

struct msg_blob_set_next_media_player {
    void *next_player; // NOTE: OpenSLMediaPlayer::Impl cannot be accessed here
};

struct msg_blob_attach_aux_effect {
    int effectId;
};

struct msg_blob_set_aux_effect_send_level {
    float level;
};

struct msg_blob_set_audio_stream_type {
    int streamtype;
};

//
// OpenSLMediaPlayer::Impl::MessageEventHandlerAdapter
//

OpenSLMediaPlayer::Impl::Impl(const android::sp<OpenSLMediaPlayerContext> &context, OpenSLMediaPlayer *holder)
    : holder_(holder), player_(), cond_wait_processed_(), mutex_wait_processed_(), state_(STATE_CREATED),
      prev_error_state_(STATE_CREATED), context_(context), msg_handler_token_(0), tmp_init_args_()
{
}

OpenSLMediaPlayer::Impl::~Impl() { release(); }

bool OpenSLMediaPlayer::Impl::checkStateTransition(PlayerState current, PlayerState next) noexcept
{
    // State diagram:
    // http://developer.android.com/reference/android/media/MediaPlayer.html#StateDiagram

    int next_mask = SMASK(IDLE) | SMASK(ERROR) | SMASK(END);

    switch (current) {
    case STATE_CREATED:
        next_mask |= 0;
        break;
    case STATE_IDLE:
        next_mask |= SMASK(INITIALIZED);
        break;
    case STATE_INITIALIZED:
        next_mask |= STATE_MASK_PREPARING | SMASK(PREPARED);
        break;
    case STATE_PREPARING_SYNC:
        next_mask |= SMASK(PREPARED);
        break;
    case STATE_PREPARING_ASYNC:
        next_mask |= SMASK(PREPARED);
        break;
    case STATE_PREPARED:
        next_mask |= SMASK(PREPARED) | SMASK(STARTED) | SMASK(STOPPED);
        break;
    case STATE_STARTED:
        next_mask |= SMASK(STARTED) | SMASK(PAUSED) | SMASK(STOPPED) | SMASK(PLAYBACK_COMPLETED);
        break;
    case STATE_PAUSED:
        next_mask |= SMASK(PAUSED) | SMASK(STARTED) | SMASK(STOPPED);
#if 1
        next_mask |= SMASK(PLAYBACK_COMPLETED);
#endif
        break;
    case STATE_PLAYBACK_COMPLETED:
        next_mask |= SMASK(PLAYBACK_COMPLETED) | SMASK(STARTED) | SMASK(STOPPED);
        break;
    case STATE_STOPPED:
        next_mask |= SMASK(STOPPED) | STATE_MASK_PREPARING | SMASK(PREPARED);
        break;
    case STATE_END:
        next_mask = SMASK(END);
        break;
    case STATE_ERROR:
        next_mask |= 0;
        break;
    default:
        next_mask = 0;
        break;
    }

    return checkStateMask(next, next_mask);
}

bool OpenSLMediaPlayer::Impl::checkStateMask(PlayerState state, int mask) noexcept
{
    return ((STATE_MASK(state) & mask) != 0);
}

const char *OpenSLMediaPlayer::Impl::getStateName(PlayerState state) noexcept
{
    return InternalUtils::sGetPlayerStateName(state);
}

const char *OpenSLMediaPlayer::Impl::getCorrespondOperationName(int msg) noexcept
{
    switch (msg) {
    case MSG_NOP:
        return "nop";
    case MSG_SET_DATA_SOURCE_PATH:
        return "setDataSource_path";
    case MSG_SET_DATA_SOURCE_URI:
        return "setDataSource_uri";
    case MSG_SET_DATA_SOURCE_FD:
        return "setDataSource_fd";
    case MSG_SET_DATA_SOURCE_FD_OFFSET_LENGTH:
        return "setDataSource_fd_offset_length";
    case MSG_PREPARE:
        return "prepare";
    case MSG_PREPARE_ASYNC:
        return "prepareAsync";
    case MSG_START:
        return "start";
    case MSG_STOP:
        return "stop";
    case MSG_PAUSE:
        return "pause";
    case MSG_RESET:
        return "reset";
    case MSG_SET_VOLUME:
        return "setVolume";
    case MSG_GET_DURATION:
        return "getDuration";
    case MSG_GET_CURRENT_POSITION:
        return "getCurrentPosition";
    case MSG_SEEK_TO:
        return "seekTo";
    case MSG_SET_LOOPING:
        return "setLooping";
    case MSG_GET_IS_LOOPING:
        return "getIsLooping";
    case MSG_GET_IS_PLAYING:
        return "getIsPlaying";
    case MSG_SET_AUDIO_STREAM_TYPE:
        return "setAudioStreamType";
    case MSG_SET_NEXT_MEDIA_PLAYER:
        return "setNextMediaPlayer";
    case MSG_ATTACH_AUX_EFFECT:
        return "attachAuxEffect";
    case MSG_SET_AUX_EFFECT_SEND_LEVEL:
        return "setAuxEffectSendLevel";
    default:
        return "unknown";
    }
}

bool OpenSLMediaPlayer::Impl::verifyNextState(PlayerState requested) const noexcept
{
    return checkStateTransition(state_, requested);
}

bool OpenSLMediaPlayer::Impl::checkCurrentState(int mask) const noexcept { return checkStateMask(state_, mask); }

bool OpenSLMediaPlayer::Impl::setState(PlayerState state, int opt_arg) noexcept
{
    LOGD("%p setState(); %s -> %s", this, getStateName(state_), getStateName(state));

    if (verifyNextState(state)) {
        const PlayerState prev_state = state_;

        // update current state
        state_ = state;

        // update prev_error_state_
        if (prev_state != STATE_ERROR && state == STATE_ERROR) {
            prev_error_state_ = prev_state;
        }

        return true;
    } else {
        LOCAL_ASSERT(false);
        return false;
    }
}

bool OpenSLMediaPlayer::Impl::checkResultAndSetError(int result) noexcept
{
    switch (result) {
    case OSLMP_RESULT_SUCCESS:
        return true;
    case OSLMP_RESULT_ILLEGAL_ARGUMENT:
        return false;
    default:
        setState(STATE_ERROR, result);
        return false;
    }
}

bool OpenSLMediaPlayer::Impl::checkResultAndSetState(int result, PlayerState state, bool transit_error) noexcept
{
    if (result == OSLMP_RESULT_SUCCESS) {
        return setState(state);
    } else {
        if (transit_error) {
            return checkResultAndSetError(result);
        } else {
            return false;
        }
    }
}

bool OpenSLMediaPlayer::Impl::raiseError(int32_t what, int32_t extra) noexcept
{
    bool handled = false;
    android::sp<OnErrorListener> on_error_listener(on_error_listener_.promote());
    android::sp<OnCompletionListener> on_completion_listener(on_completion_listener_.promote());

    LOGE("Error(%d, %d)", what, extra);

    if (on_error_listener.get()) {
        handled = on_error_listener->onError(getHolder(), what, extra);
    }

    if (!handled) {
        if (on_completion_listener.get()) {
            on_completion_listener->onCompletion(getHolder());
        }
    }

    return handled;
}

int OpenSLMediaPlayer::Impl::initialize(const OpenSLMediaPlayer::initialize_args_t &args) noexcept
{
    InternalContext &c = InternalContext::sGetInternal(*context_);

    // copy arguments to field
    tmp_init_args_ = args;

    // register message handler
    if (!c.registerMessageHandler(this, &msg_handler_token_)) {
        setState(STATE_END);
        return OSLMP_RESULT_ERROR;
    }

    setState(STATE_IDLE);

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayer::Impl::release() noexcept
{
    // unregister message handler
    android::sp<OpenSLMediaPlayerContext> context(context_);

    if (context.get()) {
        InternalContext &c = InternalContext::sGetInternal(*context);
        c.unregisterMessageHandler(this, msg_handler_token_);
        msg_handler_token_ = 0;
    }

    player_.reset();

    on_completion_listener_ = nullptr;
    on_prepared_listener_ = nullptr;
    on_seek_complete_listener_ = nullptr;
    on_buffering_update_listener_ = nullptr;
    on_info_listener_ = nullptr;
    on_error_listener_ = nullptr;
    internal_thread_event_listener_ = nullptr;

    setState(STATE_END);

    context_.clear();

    holder_ = nullptr;

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayer::Impl::setDataSourcePath(const char *path) noexcept
{
    typedef msg_blob_set_data_source_path blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_DATA_SOURCE_PATH);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.path = path;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::setDataSourceUri(const char *uri) noexcept
{
    typedef msg_blob_set_data_source_uri blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_DATA_SOURCE_URI);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.uri = uri;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::setDataSourceFd(int fd) noexcept
{
    typedef msg_blob_set_data_source_fd blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_DATA_SOURCE_FD);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.fd = fd;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept
{
    typedef msg_blob_set_data_source_fd blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_DATA_SOURCE_FD_OFFSET_LENGTH);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.fd = fd;
        blob.offset = offset;
        blob.length = length;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::prepare() noexcept
{
    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_PREPARE);
    int result = postAndWaitResult(&msg);

    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    // wait for PREPARED state
    {
        utils::pt_unique_lock lock(mutex_wait_processed_);

        while (!prepared_result_.available()) {
            cond_wait_processed_.wait(lock);
        }

        result = prepared_result_.get();
        prepared_result_.clear();
    }

    return result;
}

int OpenSLMediaPlayer::Impl::prepareAsync() noexcept
{
    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_PREPARE_ASYNC);
    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::start() noexcept
{
    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_START);
    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::stop() noexcept
{
    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_STOP);
    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::pause() noexcept
{
    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_PAUSE);
    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::reset() noexcept
{
    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_RESET);
    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::setVolume(float leftVolume, float rightVolume) noexcept
{
    typedef msg_blob_set_volume blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_VOLUME);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.leftVolume = leftVolume;
        blob.rightVolume = rightVolume;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::getDuration(int32_t *duration) noexcept
{
    typedef msg_blob_get_duration blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_GET_DURATION);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.duration = duration;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::getCurrentPosition(int32_t *position) noexcept
{
    typedef msg_blob_get_current_position blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_GET_CURRENT_POSITION);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.position = position;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::seekTo(int32_t msec) noexcept
{
    typedef msg_blob_seek_to blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SEEK_TO);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.msec = msec;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::setLooping(bool looping) noexcept
{
    typedef msg_blob_set_looping blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_LOOPING);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.looping = looping;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::isLooping(bool *looping) noexcept
{
    typedef msg_blob_get_is_looping blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_GET_IS_LOOPING);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.looping = looping;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::isPlaying(bool *playing) noexcept
{
    typedef msg_blob_get_is_playing blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_GET_IS_PLAYING);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.playing = playing;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::setAudioStreamType(int streamtype) noexcept
{
    typedef msg_blob_set_audio_stream_type blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_AUDIO_STREAM_TYPE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.streamtype = streamtype;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::attachAuxEffect(int effectId) noexcept
{
    typedef msg_blob_attach_aux_effect blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_ATTACH_AUX_EFFECT);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.effectId = effectId;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::setAuxEffectSendLevel(float level) noexcept
{
    typedef msg_blob_set_aux_effect_send_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_AUX_EFFECT_SEND_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.level = level;
    }

    return postAndWaitResult(&msg);
}

// implementations of AudioPlayer::EventHandler
void OpenSLMediaPlayer::Impl::onDecoderBufferingUpdate(int32_t percent) noexcept
{
    const int state_mask = SMASK(STARTED);

    if (!checkCurrentState(state_mask))
        return;

    // raise onBufferingUpdate() event
    android::sp<OnBufferingUpdateListener> listener(on_buffering_update_listener_.promote());

    if (listener.get()) {
        listener->onBufferingUpdate(getHolder(), percent);
    }
}

void OpenSLMediaPlayer::Impl::onPlaybackCompletion(impl::AudioPlayer::PlaybackCompletionType completion_type) noexcept
{
    LOGD("onPlaybackCompletion(completion_type = %d)", completion_type);

    const int state_mask = SMASK(STARTED) | SMASK(PAUSED);

    if (!checkCurrentState(state_mask))
        return;

    processOnCompletion(completion_type);
}

void OpenSLMediaPlayer::Impl::onPrepareCompleted(int prepare_result) noexcept
{
    if (checkCurrentState(STATE_MASK_PREPARING)) {
        const bool async = (state_ == STATE_PREPARING_ASYNC);
        if (prepare_result == OSLMP_RESULT_SUCCESS) {
            setState(STATE_PREPARED);
        } else {
            setState(STATE_ERROR);
            raiseError(ERROR_WHAT_INVALID_OPERATION, 0);
        }

        handleOnPrepareFinished(prepare_result, async);
    } else {
        LOGE("\"prepare_completed\" flag is set when state %s", getStateName(state_));
        setState(STATE_ERROR);
        raiseError(ERROR_WHAT_UNKNOWN_ERROR, 0);
    }
}

void OpenSLMediaPlayer::Impl::onSeekCompleted(int seek_result) noexcept
{
    if (seek_result == OSLMP_RESULT_SUCCESS) {
        android::sp<OnSeekCompleteListener> listener(on_seek_complete_listener_.promote());

        if (listener.get()) {
            listener->onSeekComplete(getHolder());
        }
    } else {
        setState(STATE_ERROR);
        raiseError(ERROR_WHAT_INVALID_OPERATION, 0);
    }
}

void OpenSLMediaPlayer::Impl::onPlayerStartedAsNextPlayer() noexcept
{
    const int state_mask = SMASK(PREPARED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED);

    if (!checkCurrentState(state_mask))
        return;

    setState(STATE_STARTED);
}

int OpenSLMediaPlayer::Impl::setNextMediaPlayer(const android::sp<OpenSLMediaPlayer> *next) noexcept
{
    typedef msg_blob_set_next_media_player blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(INTERNAL_MESSAGE_CATEGORY, MSG_SET_NEXT_MEDIA_PLAYER);

    {
        blob_t &blob = GET_MSG_BLOB(msg);

        if (!next || !((*next).get()) || !((*next)->impl_.get())) {
            blob.next_player = nullptr;
        } else {
            if ((*next)->impl_ == this)
                return OSLMP_RESULT_ILLEGAL_ARGUMENT;
            blob.next_player = (*next)->impl_.get();
        }
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayer::Impl::setOnCompletionListener(OpenSLMediaPlayer::OnCompletionListener *listener) noexcept
{
    on_completion_listener_ = listener;

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayer::Impl::setOnPreparedListener(OpenSLMediaPlayer::OnPreparedListener *listener) noexcept
{
    on_prepared_listener_ = listener;

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayer::Impl::setOnSeekCompleteListener(OpenSLMediaPlayer::OnSeekCompleteListener *listener) noexcept
{
    on_seek_complete_listener_ = listener;

    return OSLMP_RESULT_SUCCESS;
}

int
OpenSLMediaPlayer::Impl::setOnBufferingUpdateListener(OpenSLMediaPlayer::OnBufferingUpdateListener *listener) noexcept
{
    on_buffering_update_listener_ = listener;

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayer::Impl::setOnInfoListener(OpenSLMediaPlayer::OnInfoListener *listener) noexcept
{
    on_info_listener_ = listener;

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayer::Impl::setOnErrorListener(OpenSLMediaPlayer::OnErrorListener *listener) noexcept
{
    on_error_listener_ = listener;

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayer::Impl::setInternalThreadEventListener(
    OpenSLMediaPlayer::InternalThreadEventListener *listener) noexcept
{
    // NOTE:
    // This method can only be called before
    // calling the initialize() method.
    if (state_ != STATE_CREATED)
        return OSLMP_RESULT_ILLEGAL_STATE;

    internal_thread_event_listener_ = listener;

    return OSLMP_RESULT_SUCCESS;
}

android::sp<OpenSLMediaPlayerContext> OpenSLMediaPlayer::Impl::getPlayerContext() noexcept { return context_; }

void OpenSLMediaPlayer::Impl::handleOnPrepareFinished(int result, bool async) noexcept
{
    if (!async) {
        // prepare()
        utils::pt_unique_lock lock(mutex_wait_processed_);

        if (result == OSLMP_RESULT_SUCCESS) {
            prepared_result_.set(OSLMP_RESULT_SUCCESS);
        } else {
            prepared_result_.set(OSLMP_RESULT_INTERNAL_ERROR);
        }

        cond_wait_processed_.notify_all();
    }

    // prepare() / prepareAsync()
    if (result == OSLMP_RESULT_SUCCESS) {
        // raise prepared event
        android::sp<OnPreparedListener> listener(on_prepared_listener_.promote());

        if (listener.get()) {
            listener->onPrepared(getHolder());
        }
    }
}

//
// implementations of OpenSLMediaPlayerInternalMessageHandler
//
int OpenSLMediaPlayer::Impl::onRegisteredAsMessageHandler() noexcept
{
    const OpenSLMediaPlayer::initialize_args_t init_args = tmp_init_args_;

    // raise onEnterInternalThread() event
    {
        android::sp<InternalThreadEventListener> listener(internal_thread_event_listener_.promote());

        if (listener.get()) {
            listener->onEnterInternalThread(getHolder());
        }
    }

    // initialize
    AudioPlayer::initialize_args_t player_args;

    player_args.context = &InternalContext::sGetInternal(*context_);
    player_args.event_handler = this;

    std::unique_ptr<AudioPlayer> player(new (std::nothrow) AudioPlayer());

    int result;

    if (!player) {
        return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
    }

    result = player->initialize(player_args);
    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    result = player->setFadeInOutEnabled(init_args.use_fade);
    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    // update fields
    player_ = std::move(player);

    return OSLMP_RESULT_SUCCESS;
}

void OpenSLMediaPlayer::Impl::onUnregisteredAsMessageHandler() noexcept
{
    player_.reset();

    // raise onLeaveInternalThread() event
    {
        android::sp<InternalThreadEventListener> listener(internal_thread_event_listener_.promote());

        if (listener.get()) {
            listener->onLeaveInternalThread(getHolder());
        }
    }
}

void OpenSLMediaPlayer::Impl::onHandleMessage(const void *msg_) noexcept
{
    const Message *msg = reinterpret_cast<const Message *>(msg_);

    int result = OSLMP_RESULT_ERROR;
    const PlayerState prev_state = state_;
    const bool is_error = (prev_state == STATE_ERROR);
    bool raise_error = false;

    assert(msg->category == INTERNAL_MESSAGE_CATEGORY);

    // respond to message (pre. process)
    switch (msg->what) {
    case MSG_NOP: {
        LOCAL_ASSERT(false);
    } break;
    case MSG_SET_DATA_SOURCE_PATH: {
        typedef msg_blob_set_data_source_path blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE);
        const PlayerState next_state = STATE_INITIALIZED;

        if (checkCurrentState(state_mask)) {
            result = player_->setDataSourcePath(blob.path);
        } else {
            result = (is_error) ? OSLMP_RESULT_IN_ERROR_STATE : OSLMP_RESULT_ILLEGAL_STATE;
        }

        checkResultAndSetState(result, next_state, false);
    } break;
    case MSG_SET_DATA_SOURCE_URI: {
        typedef msg_blob_set_data_source_uri blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE);
        const PlayerState next_state = STATE_INITIALIZED;

        if (checkCurrentState(state_mask)) {
            result = player_->setDataSourceUri(blob.uri);
        } else {
            result = (is_error) ? OSLMP_RESULT_IN_ERROR_STATE : OSLMP_RESULT_ILLEGAL_STATE;
        }

        checkResultAndSetState(result, next_state, false);
    } break;
    case MSG_SET_DATA_SOURCE_FD: {
        typedef msg_blob_set_data_source_fd blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE);
        const PlayerState next_state = STATE_INITIALIZED;

        if (checkCurrentState(state_mask)) {
            result = player_->setDataSourceFd(blob.fd);
        } else {
            result = (is_error) ? OSLMP_RESULT_IN_ERROR_STATE : OSLMP_RESULT_ILLEGAL_STATE;
        }

        checkResultAndSetState(result, next_state, false);
    } break;
    case MSG_SET_DATA_SOURCE_FD_OFFSET_LENGTH: {
        typedef msg_blob_set_data_source_fd blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE);
        const PlayerState next_state = STATE_INITIALIZED;

        if (checkCurrentState(state_mask)) {
            result = player_->setDataSourceFd(blob.fd, blob.offset, blob.length);
        } else {
            result = (is_error) ? OSLMP_RESULT_IN_ERROR_STATE : OSLMP_RESULT_ILLEGAL_STATE;
        }

        checkResultAndSetState(result, next_state, false);
    } break;
    case MSG_PREPARE: {
        const int state_mask = SMASK(INITIALIZED) | SMASK(STOPPED);
        const PlayerState next_state = STATE_PREPARING_SYNC;

        if (checkCurrentState(state_mask)) {
            prepared_result_.clear();
            result = player_->preparePatialStart();
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }

        checkResultAndSetState(result, next_state, false);
    } break;
    case MSG_PREPARE_ASYNC: {
        const int state_mask = SMASK(INITIALIZED) | SMASK(STOPPED);
        const PlayerState next_state = STATE_PREPARING_ASYNC;

        if (checkCurrentState(state_mask)) {
            result = player_->preparePatialStart();
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }

        checkResultAndSetState(result, next_state, false);
    } break;
    case MSG_START: {
        const int state_mask = SMASK(PREPARED) | SMASK(STARTED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED);
        const PlayerState next_state = STATE_STARTED;

        if (checkCurrentState(state_mask)) {
            result = player_->start();
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
            raise_error = true;
        }

        checkResultAndSetState(result, next_state, true);
    } break;
    case MSG_STOP: {
        const int state_mask =
            SMASK(PREPARED) | SMASK(STARTED) | SMASK(PAUSED) | SMASK(STOPPED) | SMASK(PLAYBACK_COMPLETED);
        const PlayerState next_state = STATE_STOPPED;

        if (checkCurrentState(state_mask)) {
            result = player_->stop();
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
            raise_error = checkStateMask(prev_error_state_, STATE_MASK_PREPARED);
        }

        checkResultAndSetState(result, next_state, true);
    } break;
    case MSG_PAUSE: {
        const int state_mask = SMASK(STARTED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED);
        const PlayerState next_state = (state_ == STATE_PLAYBACK_COMPLETED) ? STATE_PLAYBACK_COMPLETED : STATE_PAUSED;

        if (checkCurrentState(state_mask)) {
            result = player_->pause();
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
            raise_error = checkStateMask(prev_error_state_, STATE_MASK_PREPARED);
        }

        checkResultAndSetState(result, next_state, true);
    } break;
    case MSG_RESET: {
        const int state_mask = SMASK(IDLE) | SMASK(INITIALIZED) | STATE_MASK_PREPARING | SMASK(PREPARED) |
                               SMASK(STARTED) | SMASK(PAUSED) | SMASK(STOPPED) | SMASK(PLAYBACK_COMPLETED) |
                               SMASK(ERROR);
        const PlayerState next_state = STATE_IDLE;

        if (checkCurrentState(state_mask)) {
            result = player_->reset();
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }

        checkResultAndSetState(result, next_state, true);
    } break;
    case MSG_SET_VOLUME: {
        typedef msg_blob_set_volume blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE) | SMASK(INITIALIZED) | STATE_MASK_PREPARING | SMASK(PREPARED) |
                               SMASK(STARTED) | SMASK(PAUSED) | SMASK(STOPPED) | SMASK(PLAYBACK_COMPLETED);

        if (checkCurrentState(state_mask)) {
            result = player_->setVolume(blob.leftVolume, blob.rightVolume);
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }
    } break;
    case MSG_GET_DURATION: {
        typedef msg_blob_get_duration blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask =
            SMASK(PREPARED) | SMASK(STARTED) | SMASK(PAUSED) | SMASK(STOPPED) | SMASK(PLAYBACK_COMPLETED) | SMASK(ERROR);

        if (checkCurrentState(state_mask)) {
            result = player_->getDuration(blob.duration);
        } else {
            if (blob.duration) {
                (*blob.duration) = 0;
            }
            result = OSLMP_RESULT_ILLEGAL_STATE;
            raise_error = checkStateMask(prev_error_state_, STATE_MASK_PREPARED);
        }

        checkResultAndSetError(result);
    } break;
    case MSG_GET_CURRENT_POSITION: {
        typedef msg_blob_get_current_position blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE) | SMASK(INITIALIZED) | SMASK(PREPARED) | SMASK(STARTED) | SMASK(PAUSED) |
                               SMASK(STOPPED) | SMASK(PLAYBACK_COMPLETED);

        if (checkCurrentState(state_mask)) {
            result = player_->getCurrentPosition(blob.position);
        } else {
            if (blob.position) {
                (*blob.position) = 0;
            }
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }

        checkResultAndSetError(result);
    } break;
    case MSG_SEEK_TO: {
        typedef msg_blob_seek_to blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(PREPARED) | SMASK(STARTED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED);

        if (checkCurrentState(state_mask)) {
            result = player_->seekTo(blob.msec);
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
            raise_error = checkStateMask(prev_error_state_, STATE_MASK_PREPARED);
        }

        checkResultAndSetError(result);
    } break;
    case MSG_SET_LOOPING: {
        typedef msg_blob_set_looping blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE) | SMASK(INITIALIZED) | STATE_MASK_PREPARING | SMASK(PREPARED) |
                               SMASK(STARTED) | SMASK(PAUSED) | SMASK(STOPPED) | SMASK(PLAYBACK_COMPLETED);

        if (checkCurrentState(state_mask)) {
            result = player_->setLooping(blob.looping);
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }

        // NOTE: don't raise error callback when preparing state
        if (!checkCurrentState(STATE_MASK_PREPARING)) {
            checkResultAndSetError(result);
        }
    } break;
    case MSG_GET_IS_LOOPING: {
        typedef msg_blob_get_is_looping blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = STATE_MASK_ANY;

        if (checkCurrentState(state_mask)) {
            result = player_->isLooping(blob.looping);
        } else {
            if (blob.looping) {
                (*blob.looping) = false;
            }
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }
    } break;
    case MSG_GET_IS_PLAYING: {
        typedef msg_blob_get_is_playing blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE) | SMASK(INITIALIZED) | STATE_MASK_PREPARING | SMASK(PREPARED) |
                               SMASK(STARTED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED) | SMASK(STOPPED);

        if (checkCurrentState(state_mask)) {
            if (blob.playing) {
                (*blob.playing) = checkCurrentState(SMASK(STARTED));
                result = OSLMP_RESULT_SUCCESS;
            } else {
                result = OSLMP_RESULT_ILLEGAL_ARGUMENT;
            }
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
            if (blob.playing) {
                (*blob.playing) = false;
            }
        }

        checkResultAndSetError(result);
    } break;
    case MSG_SET_AUDIO_STREAM_TYPE: {
        typedef msg_blob_set_audio_stream_type blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask = SMASK(IDLE) | SMASK(INITIALIZED) | SMASK(STOPPED) /* |
 SMASK(PREPARED) | SMASK(STARTED) |
 SMASK(PAUSED) |
 SMASK(PLAYBACK_COMPLETED)*/;

        if (checkCurrentState(state_mask)) {
            result = player_->setAudioStreamType(blob.streamtype);
        } else {
            int current_streamtype = OSLMP_STREAM_MUSIC;

            player_->getAudioStreamType(&current_streamtype);

            result = OSLMP_RESULT_ILLEGAL_STATE;
            raise_error = (!is_error) && (blob.streamtype != current_streamtype);
        }
    } break;
    case MSG_SET_NEXT_MEDIA_PLAYER: {
        typedef msg_blob_set_next_media_player blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask =
            SMASK(INITIALIZED) | SMASK(PREPARING_SYNC) |
            SMASK(PREPARING_ASYNC) | SMASK(PREPARED) | SMASK(STARTED) |
            SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED) | SMASK(STOPPED);
        const int next_state_mask =
            SMASK(PREPARED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED);
        Impl *next_player = static_cast<Impl *>(blob.next_player);

        if (checkCurrentState(state_mask)) {
            if (!next_player) {
                result = player_->setNextMediaPlayer(nullptr);
            } else if (next_player->checkCurrentState(next_state_mask)) {
                result = player_->setNextMediaPlayer(next_player->player_.get());
            } else {
                result = OSLMP_RESULT_ILLEGAL_STATE;
            }
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
        }
    } break;
    case MSG_ATTACH_AUX_EFFECT: {
        typedef msg_blob_attach_aux_effect blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask1 = SMASK(INITIALIZED) | STATE_MASK_PREPARING | SMASK(PREPARED);
        const int state_mask2 = SMASK(STARTED) | SMASK(PAUSED) | SMASK(STOPPED) | SMASK(PLAYBACK_COMPLETED);

        if (checkCurrentState(state_mask1 | state_mask2)) {
            result = player_->attachAuxEffect(blob.effectId);
            raise_error = (result != OSLMP_RESULT_SUCCESS) && checkCurrentState(state_mask2);
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
            if (is_error) {
                raise_error = false;
            } else {
                raise_error = true;
            }
        }
    } break;
    case MSG_SET_AUX_EFFECT_SEND_LEVEL: {
        typedef msg_blob_set_aux_effect_send_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const int state_mask1 =
            SMASK(IDLE) | SMASK(INITIALIZED) | STATE_MASK_PREPARING | SMASK(PREPARED) | SMASK(ERROR);
        const int state_mask2 = SMASK(STARTED) | SMASK(PAUSED) | SMASK(PLAYBACK_COMPLETED) | SMASK(STOPPED);

        if (checkCurrentState(state_mask1 | state_mask2)) {
            result = player_->setAuxEffectSendLevel(blob.level);
            raise_error = (result != OSLMP_RESULT_SUCCESS) && checkCurrentState(state_mask2);
        } else {
            result = OSLMP_RESULT_ILLEGAL_STATE;
            raise_error = true;
        }
    } break;
    default:
        LOCAL_ASSERT(false);
        break;
    }

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, result);
    }

    // post process
    const PlayerState cur_state = state_;
    const bool state_changed_to_error = (cur_state == STATE_ERROR) && (prev_state != STATE_ERROR);
    const bool is_invalid_state_error = (result == OSLMP_RESULT_ILLEGAL_STATE) && (cur_state == STATE_ERROR);

    // error log
    if (is_invalid_state_error) {
        switch (msg->what) {
        case MSG_GET_DURATION:
            LOGE("Attempt to call getDuration without a valid mediaplayer");
            break;
        case MSG_GET_CURRENT_POSITION:
            // suppress error message
            break;
        default:
            LOGE("%s called in state %s", getCorrespondOperationName(msg->what), getStateName(prev_state));
            break;
        }
    }

    raise_error |= state_changed_to_error;

    // raise error (+ completion) callback
    if (raise_error) {
        raiseError(translateToErrorWhat(result), 0);
    }
}

// ---

bool OpenSLMediaPlayer::Impl::post(OpenSLMediaPlayer::Impl::Message *msg) noexcept
{
    InternalContext &c = InternalContext::sGetInternal(*context_);
    return c.postMessage(this, msg_handler_token_, msg, sizeof(Message));
}

int OpenSLMediaPlayer::Impl::postAndWaitResult(OpenSLMediaPlayer::Impl::Message *msg) noexcept
{
    if (state_ == STATE_CREATED || state_ == STATE_END)
        return OSLMP_RESULT_ILLEGAL_STATE;

    volatile int result = OSLMP_RESULT_ERROR;
    volatile bool processed = false;

    msg->result = &result;
    msg->processed = &processed;

    InternalContext &c = InternalContext::sGetInternal(*context_);

    if (!c.postMessage(this, msg_handler_token_, msg, sizeof(Message)))
        return OSLMP_RESULT_ERROR;

    return waitResult(msg);
}

int OpenSLMediaPlayer::Impl::waitResult(OpenSLMediaPlayer::Impl::Message *msg) noexcept
{

    LOCAL_ASSERT(msg);
    LOCAL_ASSERT(msg->what != MSG_NOP);

    utils::pt_unique_lock lock(mutex_wait_processed_);

    while (!(*(msg->processed))) {
        cond_wait_processed_.wait(lock);
    }

    return *(msg->result);
}

void OpenSLMediaPlayer::Impl::notifyResult(const OpenSLMediaPlayer::Impl::Message *msg, int result) noexcept
{
    utils::pt_unique_lock lock(mutex_wait_processed_);

    (*(msg->result)) = result;
    (*(msg->processed)) = true;

    cond_wait_processed_.notify_all();
}

OpenSLMediaPlayer *OpenSLMediaPlayer::Impl::getHolder() const noexcept { return holder_; }

int OpenSLMediaPlayer::Impl::processOnCompletion(impl::AudioPlayer::PlaybackCompletionType completion_type) noexcept
{
#if 0
    LOCAL_ASSERT(state_ == STATE_STARTED);
#else
    LOCAL_ASSERT(state_ == STATE_STARTED || state_ == STATE_PAUSED);
#endif

    int result = OSLMP_RESULT_SUCCESS;

    switch (completion_type) {
        case AudioPlayer::kCompletionNormal:
        case AudioPlayer::kCompletionStartNextPlayer: {
            // update player state
            setState(STATE_PLAYBACK_COMPLETED);

            android::sp<OnCompletionListener> listener(on_completion_listener_.promote());
            if (listener.get()) {
                listener->onCompletion(getHolder());
            }
        }    break;
        case AudioPlayer::kPlaybackLooped: {
            android::sp<OnSeekCompleteListener> listener(on_seek_complete_listener_.promote());
            if (listener.get()) {
                // NOTE:
                // Emulate AwesomePlayer behavior.
                listener->onSeekComplete(getHolder());
            }
        }   break;
        default:
            LOGE("Unexpected completion type: %d", completion_type);
            break;
    }

    return result;
}

int OpenSLMediaPlayer::Impl::translateToErrorWhat(int result) const noexcept
{
    int what;

    switch (result) {
    case OSLMP_RESULT_SUCCESS:
        what = ERROR_WHAT_NO_ERROR;
        break;
    case OSLMP_RESULT_ILLEGAL_STATE:
        what = ERROR_WHAT_INVALID_OPERATION;
        break;
    case OSLMP_RESULT_ILLEGAL_ARGUMENT:
        what = ERROR_WHAT_BAD_VALUE;
        break;
    case OSLMP_RESULT_MEMORY_ALLOCATION_FAILED:
    case OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED:
        what = ERROR_WHAT_NO_MEMORY;
        break;
    case OSLMP_RESULT_TIMED_OUT:
        what = ERROR_WHAT_TIMED_OUT;
        break;
    default:
        what = ERROR_WHAT_UNKNOWN_ERROR;
        break;
    }

    return what;
}

} // namespace oslmp
