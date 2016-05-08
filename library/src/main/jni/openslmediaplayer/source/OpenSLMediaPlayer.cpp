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

#include "oslmp/OpenSLMediaPlayer.hpp"

#include <new>

#include "oslmp/impl/OpenSLMediaPlayerImpl.hpp"

namespace oslmp {

//
// OpenSLMediaPlayer
//

OpenSLMediaPlayer::OpenSLMediaPlayer(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(context, this))
{
}

OpenSLMediaPlayer::~OpenSLMediaPlayer() { impl_.clear(); }

int OpenSLMediaPlayer::initialize(const initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->initialize(args);
}

int OpenSLMediaPlayer::release() noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->release();
}

int OpenSLMediaPlayer::setDataSourcePath(const char *path) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setDataSourcePath(path);
}

int OpenSLMediaPlayer::setDataSourceUri(const char *uri) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setDataSourceUri(uri);
}

int OpenSLMediaPlayer::setDataSourceFd(int fd) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setDataSourceFd(fd);
}

int OpenSLMediaPlayer::setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setDataSourceFd(fd, offset, length);
}

int OpenSLMediaPlayer::prepare() noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->prepare();
}

int OpenSLMediaPlayer::prepareAsync() noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->prepareAsync();
}

int OpenSLMediaPlayer::start() noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->start();
}

int OpenSLMediaPlayer::stop() noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->stop();
}

int OpenSLMediaPlayer::pause() noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->pause();
}

int OpenSLMediaPlayer::reset() noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->reset();
}

int OpenSLMediaPlayer::setVolume(float leftVolume, float rightVolume) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setVolume(leftVolume, rightVolume);
}

int OpenSLMediaPlayer::getAudioSessionId(int32_t *audioSessionId) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->getAudioSessionId(audioSessionId);
}

int OpenSLMediaPlayer::getDuration(int32_t *duration) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->getDuration(duration);
}

int OpenSLMediaPlayer::getCurrentPosition(int32_t *position) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->getCurrentPosition(position);
}

int OpenSLMediaPlayer::seekTo(int32_t msec) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->seekTo(msec);
}

int OpenSLMediaPlayer::setLooping(bool looping) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setLooping(looping);
}

int OpenSLMediaPlayer::isLooping(bool *looping) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->isLooping(looping);
}

int OpenSLMediaPlayer::isPlaying(bool *playing) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->isPlaying(playing);
}

int OpenSLMediaPlayer::attachAuxEffect(int effectId) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->attachAuxEffect(effectId);
}

int OpenSLMediaPlayer::setAuxEffectSendLevel(float level) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setAuxEffectSendLevel(level);
}

int OpenSLMediaPlayer::setAudioStreamType(int streamtype) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setAudioStreamType(streamtype);
}

int OpenSLMediaPlayer::setNextMediaPlayer(const android::sp<OpenSLMediaPlayer> *next) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setNextMediaPlayer(next);
}

int OpenSLMediaPlayer::setOnCompletionListener(OpenSLMediaPlayer::OnCompletionListener *listener) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setOnCompletionListener(listener);
}

int OpenSLMediaPlayer::setOnPreparedListener(OpenSLMediaPlayer::OnPreparedListener *listener) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setOnPreparedListener(listener);
}

int OpenSLMediaPlayer::setOnSeekCompleteListener(OpenSLMediaPlayer::OnSeekCompleteListener *listener) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setOnSeekCompleteListener(listener);
}

int OpenSLMediaPlayer::setOnBufferingUpdateListener(OpenSLMediaPlayer::OnBufferingUpdateListener *listener) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setOnBufferingUpdateListener(listener);
}

int OpenSLMediaPlayer::setOnInfoListener(OnInfoListener *listener) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setOnInfoListener(listener);
}

int OpenSLMediaPlayer::setOnErrorListener(OnErrorListener *listener) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setOnErrorListener(listener);
}

int OpenSLMediaPlayer::setInternalThreadEventListener(OpenSLMediaPlayer::InternalThreadEventListener *listener) noexcept
{
    if (CXXPH_UNLIKELY(!impl_.get()))
        return OSLMP_RESULT_ERROR;
    return impl_->setInternalThreadEventListener(listener);
}

} // namespace oslmp
