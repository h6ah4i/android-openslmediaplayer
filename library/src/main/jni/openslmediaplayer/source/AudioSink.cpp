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

// #define LOG_TAG "AudioSink"
// #define NB_LOG_TAG "NB AudioSink"

#include "oslmp/impl/AudioSink.hpp"

#include <cxxporthelper/memory>
#include <cxxporthelper/utility>

#include <loghelper/loghelper.h>

#include <jni_utils/jni_utils.hpp>

#include <oslmp/OpenSLMediaPlayerContext.hpp>
#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/AudioDataPipeManager.hpp"
#include "oslmp/impl/AudioSinkDataPipeReadBlockQueue.hpp"
#include "oslmp/impl/AudioSinkOpenSLBackend.hpp"
#include "oslmp/impl/AudioSinkAudioTrackBackend.hpp"
#include "oslmp/impl/AudioTrackStream.hpp"
#include "oslmp/utils/timespec_utils.hpp"

namespace oslmp {
namespace impl {

typedef OpenSLMediaPlayerInternalUtils InternalUtils;

class AudioSinkBackend;

class AudioSink::Impl {
public:
    Impl(AudioSink *holder);
    ~Impl();

    int initialize(const initialize_args_t &args) noexcept;
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
    SLresult getInterfaceFromSinkPlayer(opensles::CSLInterface *itf, bool instantiate) noexcept;
    SLresult releaseInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept;


    int setNotifyPullCallback(void (*pfunc)(void *), void *args) noexcept;

private:
    void updateState(state_t state);

private:
    AudioSink *holder_;
    state_t state_;

    OpenSLMediaPlayerInternalContext *context_;
    AudioDataPipeManager *pipe_mgr_;
    AudioSinkDataPipe *pipe_;

    std::unique_ptr<AudioSinkBackend> backend_;
};

//
// AudioSink
//
AudioSink::AudioSink() : impl_(new (std::nothrow) Impl(this)) {}

AudioSink::~AudioSink() {}

int AudioSink::initialize(const AudioSink::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioSink::start() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->start();
}

int AudioSink::pause() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->pause();
}

int AudioSink::resume() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->resume();
}

int AudioSink::stop() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->stop();
}

uint32_t AudioSink::getLatencyInFrames() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0UL;
    return impl_->getLatencyInFrames();
}

AudioSink::state_t AudioSink::getState() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SINK_STATE_NOT_INITIALIZED;
    return impl_->getState();
}

AudioSinkDataPipe *AudioSink::getPipe() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return nullptr;
    return impl_->getPipe();
}

int32_t AudioSink::getAudioSessionId() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0;
    return impl_->getAudioSessionId();
}

int AudioSink::selectActiveAuxEffect(int aux_effect_id) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->selectActiveAuxEffect(aux_effect_id);
}

int AudioSink::setAuxEffectSendLevel(float level) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAuxEffectSendLevel(level);
}

int AudioSink::setAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAuxEffectEnabled(aux_effect_id, enabled);
}

SLresult AudioSink::getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SL_RESULT_RESOURCE_ERROR;
    return impl_->getInterfaceFromOutputMixer(itf);
}

SLresult AudioSink::getInterfaceFromSinkPlayer(opensles::CSLInterface *itf, bool instantiate) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SL_RESULT_RESOURCE_ERROR;
    return impl_->getInterfaceFromSinkPlayer(itf, instantiate);
}

SLresult AudioSink::releaseInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SL_RESULT_RESOURCE_ERROR;
    return impl_->releaseInterfaceFromSinkPlayer(itf);
}

int AudioSink::setNotifyPullCallback(void (*pfunc)(void *), void *args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SL_RESULT_RESOURCE_ERROR;
    return impl_->setNotifyPullCallback(pfunc, args);
}

//
// AudioSink::Impl
//
AudioSink::Impl::Impl(AudioSink *holder)
    : holder_(holder), state_(SINK_STATE_NOT_INITIALIZED), context_(nullptr), pipe_mgr_(nullptr), pipe_(nullptr), backend_()
{
}

AudioSink::Impl::~Impl()
{
    stop();
    backend_.reset();

    if (pipe_mgr_ && pipe_) {
        (void)pipe_mgr_->setSinkPipeOutPortUser(pipe_, holder_, false);
    }
    pipe_ = nullptr;
    pipe_mgr_ = nullptr;
    context_ = nullptr;
    holder_ = nullptr;
}

int AudioSink::Impl::initialize(const AudioSink::initialize_args_t &args) noexcept
{
    // check arguments
    switch (args.sampling_rate) {
    case 44100000:
    case 48000000:
        break;
    default:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    switch (args.sample_format) {
    case kAudioSampleFormatType_S16:
    case kAudioSampleFormatType_F32:
        break;
    default:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (!args.context)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.pipe_manager)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.pipe)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (state_ != SINK_STATE_NOT_INITIALIZED)
        return OSLMP_RESULT_ILLEGAL_STATE;

    std::unique_ptr<AudioSinkBackend> backend;

    switch (args.backend) {
    case BACKEND_OPENSL:
        LOGD("Back-end type: OpenSL");
        backend.reset(new(std::nothrow) AudioSinkOpenSLBackend());
        break;
    case BACKEND_AUDIO_TRACK:
        LOGD("Back-end type: AudioTrack");
        backend.reset(new(std::nothrow) AudioSinkAudioTrackBackend());
        break;
    }

    if (!backend) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    int result = backend->onInitialize(args, holder_);

    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    backend_ = std::move(backend);
    context_ = args.context;
    pipe_mgr_ = args.pipe_manager;
    pipe_ = args.pipe;

    updateState(SINK_STATE_STOPPED);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSink::Impl::start() noexcept
{
    if (CXXPH_UNLIKELY(state_ == SINK_STATE_STARTED))
        return OSLMP_RESULT_SUCCESS;

    if (CXXPH_UNLIKELY(state_ != SINK_STATE_STOPPED))
        return OSLMP_RESULT_ILLEGAL_STATE;

    const int result = backend_->onStart();

    if (result == OSLMP_RESULT_SUCCESS) {
        updateState(SINK_STATE_STARTED);
    }

    return result;
}

int AudioSink::Impl::pause() noexcept
{
    if (CXXPH_UNLIKELY(state_ == SINK_STATE_PAUSED))
        return OSLMP_RESULT_SUCCESS;

    if (CXXPH_UNLIKELY(state_ != SINK_STATE_STARTED))
        return OSLMP_RESULT_ILLEGAL_STATE;

    const int result = backend_->onPause();

    if (result == OSLMP_RESULT_SUCCESS) {
        updateState(SINK_STATE_PAUSED);
    }

    return result;
}

int AudioSink::Impl::resume() noexcept
{
    if (CXXPH_UNLIKELY(state_ == SINK_STATE_STARTED))
        return OSLMP_RESULT_SUCCESS;

    const int result = backend_->onResume();

    if (result == OSLMP_RESULT_SUCCESS) {
        updateState(SINK_STATE_STARTED);
    }

    return result;
}

int AudioSink::Impl::stop() noexcept
{
    if (CXXPH_UNLIKELY(state_ == SINK_STATE_STOPPED))
        return OSLMP_RESULT_SUCCESS;

    if (CXXPH_UNLIKELY(!(state_ == SINK_STATE_STARTED || state_ == SINK_STATE_PAUSED)))
        return OSLMP_RESULT_ILLEGAL_STATE;

    const int result = backend_->onStop();

    if (result == OSLMP_RESULT_SUCCESS) {
        updateState(SINK_STATE_STOPPED);
    }

    return result;
}

uint32_t AudioSink::Impl::getLatencyInFrames() const noexcept
{
    return backend_->onGetLatencyInFrames();
}

AudioSink::state_t AudioSink::Impl::getState() const noexcept { return state_; }

AudioSinkDataPipe *AudioSink::Impl::getPipe() const noexcept { return pipe_; }

int32_t AudioSink::Impl::getAudioSessionId() const noexcept
{
    return backend_->onGetAudioSessionId();
}

int AudioSink::Impl::selectActiveAuxEffect(int aux_effect_id) noexcept
{
    return backend_->onSelectActiveAuxEffect(aux_effect_id);
}

int AudioSink::Impl::setAuxEffectSendLevel(float level) noexcept
{
    return backend_->onSetAuxEffectSendLevel(level);
}

int AudioSink::Impl::setAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept
{
    return backend_->onSetAuxEffectEnabled(aux_effect_id, enabled);
}

void AudioSink::Impl::updateState(state_t state) {
    if (state == state_) {
        return;
    }
    state_ = state;

#ifdef LOG_TAG
    switch (state_) {
    case SINK_STATE_NOT_INITIALIZED:
        LOGD("updateState(state = NOT_INITIALIZED)");
        break;
    case SINK_STATE_STOPPED:
        LOGD("updateState(state = STOPPED)");
        break;
    case SINK_STATE_STARTED:
        LOGD("updateState(state = STARTED)");
        break;
    case SINK_STATE_PAUSED:
        LOGD("updateState(state = PAUSED)");
        break;
    }
#endif
}

SLresult AudioSink::Impl::getInterfaceFromSinkPlayer(opensles::CSLInterface *itf, bool instantiate) noexcept
{
    return backend_->onGetInterfaceFromSinkPlayer(itf, instantiate);
}

SLresult AudioSink::Impl::getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept
{
    return backend_->onGetInterfaceFromOutputMixer(itf);
}

SLresult AudioSink::Impl::releaseInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept
{
    return backend_->onReleaseInterfaceFromOutputMixer(itf);
}

int AudioSink::Impl::setNotifyPullCallback(void (*pfunc)(void *), void *args) noexcept
{
    return backend_->onSetNotifyPullCallback(pfunc, args);
}



} // namespace impl
} // namespace oslmp
