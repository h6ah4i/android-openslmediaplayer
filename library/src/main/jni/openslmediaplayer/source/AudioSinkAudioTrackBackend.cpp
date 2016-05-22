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

// #define LOG_TAG "ASAudioTrackBackend"
// #define NB_LOG_TAG "NB ASAudioTrackBackend"

#include "oslmp/impl/AudioSinkAudioTrackBackend.hpp"

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/AudioDataPipeManager.hpp"
#include "oslmp/impl/AudioTrackStream.hpp"


namespace oslmp {
namespace impl {


//
// AudioSinkAudioTrackBackend
//
AudioSinkAudioTrackBackend::AudioSinkAudioTrackBackend()
    : AudioSinkBackend(), block_size_in_frames_(0), num_player_blocks_(0), num_pipe_blocks_(0), audio_session_id_(0),
      notify_pull_callback_pfunc_(nullptr), notify_pull_callback_args_(nullptr)
{
}

AudioSinkAudioTrackBackend::~AudioSinkAudioTrackBackend() {
}

int AudioSinkAudioTrackBackend::onInitialize(const AudioSink::initialize_args_t &args, void *pipe_user) noexcept
{
    std::unique_ptr<AudioTrackStream> stream(new AudioTrackStream());

    const int block_size_in_frames = args.pipe_manager->getBlockSizeInFrames();
    const uint32_t num_pipe_blocks = args.pipe->getNumberOfBufferItems();
    const uint32_t num_player_blocks = args.num_player_blocks;

    JavaVM *vm = args.context->getJavaVM();
    JNIEnv *env = nullptr;
    int result;

    (void) vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

    if (!env) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    result = stream->init(
        env, args.stream_type, args.sample_format, 
        (args.sampling_rate / 1000), AudioSink::NUM_CHANNELS, block_size_in_frames, num_player_blocks);

    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    const int32_t audio_session_id = stream->getAudioSessionId();

    if (audio_session_id == 0) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    stream_ = std::move(stream);

    audio_session_id_ = audio_session_id;
    block_size_in_frames_ = block_size_in_frames;
    num_player_blocks_ = num_player_blocks;
    num_pipe_blocks_ = num_pipe_blocks;
    pipe_ = args.pipe;

#if USE_OSLMP_DEBUG_FEATURES
    nb_logger_.reset(args.context->getNonBlockingTraceLogger().create_new_client());
#endif

    return OSLMP_RESULT_SUCCESS;
}

int AudioSinkAudioTrackBackend::onStart() noexcept
{
    return stream_->start(audioTrackStreamCallback, this);
}

int AudioSinkAudioTrackBackend::onPause() noexcept
{
    return stream_->stop();
}

int AudioSinkAudioTrackBackend::onResume() noexcept
{
    return stream_->start(audioTrackStreamCallback, this);
}

int AudioSinkAudioTrackBackend::onStop() noexcept
{
    return stream_->stop();
}

uint32_t AudioSinkAudioTrackBackend::onGetLatencyInFrames() const noexcept
{
    return static_cast<uint32_t>(block_size_in_frames_ * (num_pipe_blocks_ - 1));
}

int32_t AudioSinkAudioTrackBackend::onGetAudioSessionId() const noexcept
{
    return audio_session_id_;
}

int AudioSinkAudioTrackBackend::onSelectActiveAuxEffect(int aux_effect_id) noexcept
{
    return stream_->attachAuxEffect(aux_effect_id);
}

int AudioSinkAudioTrackBackend::onSetAuxEffectSendLevel(float level) noexcept
{
    return stream_->setAuxEffectSendLevel(level);
}

int AudioSinkAudioTrackBackend::onSetAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept
{
    return OSLMP_RESULT_SUCCESS;
}

SLresult AudioSinkAudioTrackBackend::onGetInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept
{
    return SL_RESULT_FEATURE_UNSUPPORTED;
}

SLresult AudioSinkAudioTrackBackend::onGetInterfaceFromSinkPlayer(opensles::CSLInterface *itf, bool instantiate) noexcept
{
    return SL_RESULT_FEATURE_UNSUPPORTED;
}

SLresult AudioSinkAudioTrackBackend::onReleaseInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept
{
    return SL_RESULT_FEATURE_UNSUPPORTED;
}

int AudioSinkAudioTrackBackend::onSetNotifyPullCallback(void (*pfunc)(void *), void *args) noexcept
{
    notify_pull_callback_pfunc_ = pfunc;
    notify_pull_callback_args_ = args;

    return OSLMP_RESULT_SUCCESS;
}

int32_t AudioSinkAudioTrackBackend::audioTrackStreamCallback(
    void *buffer, sample_format_type format, uint32_t num_channels, uint32_t buffer_size_in_frames, void *args) noexcept
{

    AudioSinkAudioTrackBackend *thiz = static_cast<AudioSinkAudioTrackBackend *>(args);

    REF_NB_LOGGER_CLIENT(thiz->nb_logger_);

    const size_t bytes_per_sample = getBytesPerSample(format);

    AudioSinkDataPipe *pipe = thiz->pipe_;

    NB_LOGV("audioTrackStreamCallback");

    AudioSinkDataPipe::read_block_t rb;
    int32_t size_in_frames;

    if (CXXPH_LIKELY(pipe->lockRead(rb, 0))) {
        size_in_frames = (std::min)(rb.num_frames, buffer_size_in_frames);
        (void) ::memcpy(buffer, rb.src, (bytes_per_sample * size_in_frames * num_channels));
        pipe->unlockRead(rb);
    } else {
        size_in_frames = 0;
    }

    if (thiz->notify_pull_callback_pfunc_) {
        (*(thiz->notify_pull_callback_pfunc_))(thiz->notify_pull_callback_args_);
    }

    return size_in_frames;
}

} // namespace impl
} // namespace oslmp
