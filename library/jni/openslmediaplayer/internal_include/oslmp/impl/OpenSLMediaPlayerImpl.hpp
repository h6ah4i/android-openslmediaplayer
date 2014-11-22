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

#ifndef OPENSLMEDIAPLAYERIMPL_HPP_
#define OPENSLMEDIAPLAYERIMPL_HPP_

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

#include <utils/RefBase.h>

#include <oslmp/OpenSLMediaPlayer.hpp>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/AudioPlayer.hpp"
#include "oslmp/utils/pthread_utils.hpp"
#include "oslmp/utils/optional.hpp"

//
// forward declarations
//
namespace oslmp {
namespace impl {
struct OpenSLMediaPlayerThreadMessage;
} // namespace impl
} // namespace oslmp

namespace oslmp {

class OpenSLMediaPlayer::Impl : public virtual android::RefBase,
                                public impl::AudioPlayer::EventHandler,
                                public impl::OpenSLMediaPlayerInternalMessageHandler {
public:
    typedef impl::OpenSLMediaPlayerThreadMessage Message;

    Impl(const android::sp<OpenSLMediaPlayerContext> &context, OpenSLMediaPlayer *holder);
    ~Impl();

    int initialize(const OpenSLMediaPlayer::initialize_args_t &args) noexcept;
    int release() noexcept;
    int setDataSourcePath(const char *path) noexcept;
    int setDataSourceUri(const char *uri) noexcept;
    int setDataSourceFd(int fd) noexcept;
    int setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept;
    int prepare() noexcept;
    int prepareAsync() noexcept;
    int start() noexcept;
    int stop() noexcept;
    int pause() noexcept;
    int reset() noexcept;
    int setVolume(float leftVolume, float rightVolume) noexcept;
    int getDuration(int32_t *duration) noexcept;
    int getCurrentPosition(int32_t *position) noexcept;
    int seekTo(int32_t msec) noexcept;
    int setLooping(bool looping) noexcept;
    int isLooping(bool *looping) noexcept;
    int isPlaying(bool *playing) noexcept;
    int setAudioStreamType(int streamtype) noexcept;
    int attachAuxEffect(int effectId) noexcept;
    int setAuxEffectSendLevel(float level) noexcept;

    int setNextMediaPlayer(const android::sp<OpenSLMediaPlayer> *next) noexcept;

    int setOnCompletionListener(OpenSLMediaPlayer::OnCompletionListener *listener) noexcept;
    int setOnPreparedListener(OpenSLMediaPlayer::OnPreparedListener *listener) noexcept;
    int setOnSeekCompleteListener(OpenSLMediaPlayer::OnSeekCompleteListener *listener) noexcept;
    int setOnBufferingUpdateListener(OpenSLMediaPlayer::OnBufferingUpdateListener *listener) noexcept;
    int setOnInfoListener(OpenSLMediaPlayer::OnInfoListener *listener) noexcept;
    int setOnErrorListener(OpenSLMediaPlayer::OnErrorListener *listener) noexcept;

    int setInternalThreadEventListener(OpenSLMediaPlayer::InternalThreadEventListener *listener) noexcept;

    android::sp<OpenSLMediaPlayerContext> getPlayerContext() noexcept;

    // implementations of AudioPlayer::EventHandler
    virtual void onDecoderBufferingUpdate(int32_t percent) noexcept override;
    virtual void onPlaybackCompletion() noexcept override;
    virtual void onPrepareCompleted(int prepare_result) noexcept override;
    virtual void onSeekCompleted(int seek_result) noexcept override;
    virtual void onPlayerStartedAsNextPlayer() noexcept override;

    // implementations of OpenSLMediaPlayerInternalMessageHandler
    virtual int onRegisteredAsMessageHandler() noexcept override;
    virtual void onUnregisteredAsMessageHandler() noexcept override;
    virtual void onHandleMessage(const void *msg) noexcept override;

private:
    static bool checkStateTransition(impl::PlayerState current, impl::PlayerState next) noexcept;
    static bool checkStateMask(impl::PlayerState state, int mask) noexcept;
    static const char *getStateName(impl::PlayerState state) noexcept;
    static const char *getCorrespondOperationName(int msg) noexcept;
    bool verifyNextState(impl::PlayerState requested) const noexcept;
    bool checkCurrentState(int mask) const noexcept;
    bool setState(impl::PlayerState state, int opt_arg = 0) noexcept;
    bool checkResultAndSetError(int result) noexcept;
    bool checkResultAndSetState(int result, impl::PlayerState state, bool transit_error) noexcept;
    bool raiseError(int32_t what, int32_t extra) noexcept;

    bool post(Message *msg) noexcept;
    int postAndWaitResult(Message *msg) noexcept;
    int waitResult(Message *msg) noexcept;
    void notifyResult(const Message *msg, int result) noexcept;

    OpenSLMediaPlayer *getHolder() const noexcept;

    int handleInternalExtAttachOrInstall(const Message *msg) noexcept;
    int handleInternalExtDetachOrUninstall(const Message *msg) noexcept;

    int handleInternalAuxEffectSettingsChanged(const Message *msg) noexcept;

    void handleOnPrepareFinished(int result, bool async) noexcept;
    int processOnCompletion() noexcept;
    int translateToErrorWhat(int result) const noexcept;

private:
    OpenSLMediaPlayer *holder_;

    std::unique_ptr<impl::AudioPlayer> player_;

    impl::PlayerState state_;
    impl::PlayerState prev_error_state_;

    utils::optional<int> prepared_result_;

    utils::pt_condition_variable cond_wait_processed_;
    mutable utils::pt_mutex mutex_wait_processed_;

    android::wp<OpenSLMediaPlayer::OnCompletionListener> on_completion_listener_;
    android::wp<OpenSLMediaPlayer::OnPreparedListener> on_prepared_listener_;
    android::wp<OpenSLMediaPlayer::OnSeekCompleteListener> on_seek_complete_listener_;
    android::wp<OpenSLMediaPlayer::OnBufferingUpdateListener> on_buffering_update_listener_;
    android::wp<OpenSLMediaPlayer::OnInfoListener> on_info_listener_;
    android::wp<OpenSLMediaPlayer::OnErrorListener> on_error_listener_;
    android::wp<OpenSLMediaPlayer::InternalThreadEventListener> internal_thread_event_listener_;

    android::sp<OpenSLMediaPlayerContext> context_;
    impl::OpenSLMediaPlayerInternalMessageHandlerToken msg_handler_token_;

    // temporary variable for initialization
    OpenSLMediaPlayer::initialize_args_t tmp_init_args_;
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYERTHRIMPLP_
