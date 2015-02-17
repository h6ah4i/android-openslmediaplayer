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

#ifndef AUDIO_PLAYER_HPP_
#define AUDIO_PLAYER_HPP_

#include <cxxporthelper/memory>
#include <SLES/OpenSLES.h>

//
// forward declarations
//
namespace oslmp {
namespace impl {
class OpenSLMediaPlayerInternalContext;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

class AudioPlayer {
public:
    enum PlaybackCompletionType {
        kCompletionNormal,
        kCompletionStartNextPlayer,
        kPlaybackLooped,
    };

    class EventHandler {
    public:
        virtual ~EventHandler() {}
        virtual void onDecoderBufferingUpdate(int32_t percent) noexcept = 0;
        virtual void onPlaybackCompletion(PlaybackCompletionType completion_type) noexcept = 0;
        virtual void onPrepareCompleted(int prepare_result) noexcept = 0;
        virtual void onSeekCompleted(int seek_result) noexcept = 0;
        virtual void onPlayerStartedAsNextPlayer() noexcept = 0;
    };

    struct initialize_args_t {
        EventHandler *event_handler;
        OpenSLMediaPlayerInternalContext *context;

        initialize_args_t() : event_handler(nullptr), context(nullptr) {}
    };

    AudioPlayer();
    ~AudioPlayer();

    int initialize(const initialize_args_t &args) noexcept;

    int setDataSourcePath(const char *path) noexcept;
    int setDataSourceUri(const char *uri) noexcept;
    int setDataSourceFd(int fd) noexcept;
    int setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept;

    int preparePatialStart() noexcept;
    int preparePatialPoll(bool *completed, bool *needRetry) noexcept;
    void preparePartialCleanup() noexcept;
    bool isPreparing() const noexcept;

    int start() noexcept;
    int pause() noexcept;
    int stop() noexcept;
    int reset() noexcept;
    int setVolume(float leftVolume, float rightVolume) noexcept;
    int getDuration(int32_t *duration) noexcept;
    int getCurrentPosition(int32_t *position) noexcept;
    int seekTo(int32_t msec) noexcept;
    int setLooping(bool looping) noexcept;
    int isLooping(bool *looping) noexcept;
    int setAudioStreamType(int streamtype) noexcept;
    int getAudioStreamType(int *streamtype) const noexcept;
    int setNextMediaPlayer(AudioPlayer *next) noexcept;
    int attachAuxEffect(int effectId) noexcept;
    int setAuxEffectSendLevel(float level) noexcept;
    int setFadeInOutEnabled(bool enabled) noexcept;

    bool isPollingRequired() const noexcept;
    void poll() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIO_PLAYER_HPP_
