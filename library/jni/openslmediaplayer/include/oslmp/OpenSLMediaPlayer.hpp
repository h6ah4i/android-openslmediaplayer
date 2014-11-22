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

#ifndef OPENSLMEDIAPLAYER_HPP_
#define OPENSLMEDIAPLAYER_HPP_

#include <SLES/OpenSLES.h>

#include <oslmp/OpenSLMediaPlayerAPICommon.hpp>
#include <oslmp/OpenSLMediaPlayerContext.hpp>

#define OSLMP_AUX_EFFECT_NULL 0
#define OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB 1
#define OSLMP_AUX_EFFECT_PRESET_REVERB 2

#define OSLMP_MEDIA_ERROR_UNKNOWN 1
#define OSLMP_MEDIA_ERROR_SERVER_DIED 100
#define OSLMP_MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK 200
#define OSLMP_MEDIA_ERROR_TIMED_OUT -110
#define OSLMP_MEDIA_ERROR_IO -1004
#define OSLMP_MEDIA_ERROR_MALFORMED -1007
#define OSLMP_MEDIA_ERROR_UNSUPPORTED -1010

#define OSLMP_MEDIA_INFO_UNKNOWN 1
#define OSLMP_MEDIA_INFO_VIDEO_RENDERING_START 3
#define OSLMP_MEDIA_INFO_VIDEO_TRACK_LAGGING 700
#define OSLMP_MEDIA_INFO_BUFFERING_START 701
#define OSLMP_MEDIA_INFO_BUFFERING_END 702
#define OSLMP_MEDIA_INFO_BAD_INTERLEAVING 800
#define OSLMP_MEDIA_INFO_NOT_SEEKABLE 801
#define OSLMP_MEDIA_INFO_METADATA_UPDATE 802
#define OSLMP_MEDIA_INFO_UNSUPPORTED_SUBTITLE 901
#define OSLMP_MEDIA_INFO_SUBTITLE_TIMED_OUT 902

#define OSLMP_STREAM_VOICE 0x00000000
#define OSLMP_STREAM_SYSTEM 0x00000001
#define OSLMP_STREAM_RING 0x00000002
#define OSLMP_STREAM_MUSIC 0x00000003
#define OSLMP_STREAM_ALARM 0x00000004
#define OSLMP_STREAM_NOTIFICATION 0x00000005

namespace oslmp {

class OpenSLMediaPlayer : public virtual android::RefBase {
public:
    class OnCompletionListener;
    class OnPreparedListener;
    class OnSeekCompleteListener;
    class OnBufferingUpdateListener;
    class OnInfoListener;
    class OnErrorListener;
    class InternalThreadEventListener;

    struct initialize_args_t {
        bool use_fade;

        initialize_args_t() OSLMP_API_ABI : use_fade(true) {}
    };

public:
    OpenSLMediaPlayer(const android::sp<OpenSLMediaPlayerContext> &context) OSLMP_API_ABI;
    virtual ~OpenSLMediaPlayer() OSLMP_API_ABI;

    int initialize(const initialize_args_t &args) noexcept OSLMP_API_ABI;
    int release() noexcept OSLMP_API_ABI;
    int setDataSourcePath(const char *path) noexcept OSLMP_API_ABI;
    int setDataSourceUri(const char *uri) noexcept OSLMP_API_ABI;
    int setDataSourceFd(int fd) noexcept OSLMP_API_ABI;
    int setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept OSLMP_API_ABI;
    int prepare() noexcept OSLMP_API_ABI;
    int prepareAsync() noexcept OSLMP_API_ABI;
    int start() noexcept OSLMP_API_ABI;
    int stop() noexcept OSLMP_API_ABI;
    int pause() noexcept OSLMP_API_ABI;
    int reset() noexcept OSLMP_API_ABI;
    int setVolume(float leftVolume, float rightVolume) noexcept OSLMP_API_ABI;
    int getDuration(int32_t *duration) noexcept OSLMP_API_ABI;
    int getCurrentPosition(int32_t *position) noexcept OSLMP_API_ABI;
    int seekTo(int32_t msec) noexcept OSLMP_API_ABI;
    int setLooping(bool looping) noexcept OSLMP_API_ABI;
    int isLooping(bool *looping) noexcept OSLMP_API_ABI;
    int isPlaying(bool *playing) noexcept OSLMP_API_ABI;

    int attachAuxEffect(int effectId) noexcept OSLMP_API_ABI;
    int setAuxEffectSendLevel(float level) noexcept OSLMP_API_ABI;

    int setAudioStreamType(int streamtype) noexcept OSLMP_API_ABI;

    int setNextMediaPlayer(const android::sp<OpenSLMediaPlayer> *next) noexcept OSLMP_API_ABI;

    int setOnCompletionListener(OnCompletionListener *listener) noexcept OSLMP_API_ABI;
    int setOnPreparedListener(OnPreparedListener *listener) noexcept OSLMP_API_ABI;
    int setOnSeekCompleteListener(OnSeekCompleteListener *listener) noexcept OSLMP_API_ABI;
    int setOnBufferingUpdateListener(OnBufferingUpdateListener *listener) noexcept OSLMP_API_ABI;
    int setOnInfoListener(OnInfoListener *listener) noexcept OSLMP_API_ABI;
    int setOnErrorListener(OnErrorListener *listener) noexcept OSLMP_API_ABI;

    int setInternalThreadEventListener(InternalThreadEventListener *listener) noexcept OSLMP_API_ABI;

private:
    // inhibit copy operations
    OpenSLMediaPlayer(const OpenSLMediaPlayer &) = delete;
    OpenSLMediaPlayer &operator=(const OpenSLMediaPlayer &) = delete;

private:
    class Impl;
    android::sp<Impl> impl_;
};

class OpenSLMediaPlayer::OnCompletionListener : public virtual android::RefBase {
public:
    virtual ~OnCompletionListener() OSLMP_API_ABI {}
    virtual void onCompletion(OpenSLMediaPlayer *mp) noexcept OSLMP_API_ABI = 0;
};

class OpenSLMediaPlayer::OnPreparedListener : public virtual android::RefBase {
public:
    virtual ~OnPreparedListener() OSLMP_API_ABI {}
    virtual void onPrepared(OpenSLMediaPlayer *mp) noexcept OSLMP_API_ABI = 0;
};

class OpenSLMediaPlayer::OnSeekCompleteListener : public virtual android::RefBase {
public:
    virtual ~OnSeekCompleteListener() OSLMP_API_ABI {}
    virtual void onSeekComplete(OpenSLMediaPlayer *mp) noexcept OSLMP_API_ABI = 0;
};

class OpenSLMediaPlayer::OnBufferingUpdateListener : public virtual android::RefBase {
public:
    virtual ~OnBufferingUpdateListener() OSLMP_API_ABI {}
    virtual void onBufferingUpdate(OpenSLMediaPlayer *mp, int32_t percent) noexcept OSLMP_API_ABI = 0;
};

class OpenSLMediaPlayer::OnInfoListener : public virtual android::RefBase {
public:
    virtual ~OnInfoListener() OSLMP_API_ABI {}
    virtual bool onInfo(OpenSLMediaPlayer *mp, int32_t what, int32_t extra) noexcept OSLMP_API_ABI = 0;
};

class OpenSLMediaPlayer::OnErrorListener : public virtual android::RefBase {
public:
    virtual ~OnErrorListener() OSLMP_API_ABI {}
    virtual bool onError(OpenSLMediaPlayer *mp, int32_t what, int32_t extra) noexcept OSLMP_API_ABI = 0;
};

class OpenSLMediaPlayer::InternalThreadEventListener : public virtual android::RefBase {
public:
    virtual ~InternalThreadEventListener() OSLMP_API_ABI {}
    virtual void onEnterInternalThread(OpenSLMediaPlayer *mp) noexcept OSLMP_API_ABI = 0;
    virtual void onLeaveInternalThread(OpenSLMediaPlayer *mp) noexcept OSLMP_API_ABI = 0;
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYER_HPP_
