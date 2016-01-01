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
    : AudioSinkBackend(), block_size_in_frames_(0), num_player_blocks_(0), num_pipe_blocks_(0) 
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

    stream_ = std::move(stream);

    // SLint32 sl_stream_type;
    // SLresult slResult;
    // int result;

    // if (!InternalUtils::sTranslateToOpenSLStreamType(args.stream_type, &sl_stream_type))
    //     return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    // const int block_size_in_frames = args.pipe_manager->getBlockSizeInFrames();
    // const uint32_t num_pipe_blocks = args.pipe->getNumberOfBufferItems();
    // const uint32_t num_player_blocks = args.num_player_blocks;

    // if (!(num_pipe_blocks > (num_player_blocks + 1))) { // +1: silent block
    //     return OSLMP_RESULT_INTERNAL_ERROR;
    // }

    // // create SLPlayer
    // CSLEngineItf engine;
    // CSLObjectItf obj_outmix(false); // No auto-destroy

    // slResult = args.context->getInterfaceFromEngine(&engine);
    // if (!IS_SL_RESULT_SUCCESS(slResult))
    //     return TRANSLATE_RESULT(slResult);

    // slResult = args.context->getInterfaceFromOutputMixer(&obj_outmix);
    // if (!IS_SL_RESULT_SUCCESS(slResult))
    //     return TRANSLATE_RESULT(slResult);

    // // configure audio source
    // SLDataLocator_AndroidSimpleBufferQueue src_loc_bufq = {};
    // SLDataFormat_PCM src_format_pcm = {};
    // SLAndroidDataFormat_PCM_EX src_format_pcm_ex = {};
    // SLDataSource audio_src = {};

    // src_loc_bufq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    // src_loc_bufq.numBuffers = num_player_blocks;

    // if (args.sample_format == kAudioSampleFormatType_F32) {
    //     src_format_pcm_ex.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
    //     src_format_pcm_ex.numChannels = AudioSink::NUM_CHANNELS;
    //     src_format_pcm_ex.sampleRate = args.sampling_rate;
    //     src_format_pcm_ex.bitsPerSample = 32;
    //     src_format_pcm_ex.containerSize = 32;
    //     src_format_pcm_ex.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    //     src_format_pcm_ex.endianness = SL_BYTEORDER_LITTLEENDIAN;
    //     src_format_pcm_ex.representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;

    //     audio_src.pFormat = &src_format_pcm_ex;
    // } else {
    //     src_format_pcm.formatType = SL_DATAFORMAT_PCM;
    //     src_format_pcm.numChannels = AudioSink::NUM_CHANNELS;
    //     src_format_pcm.samplesPerSec = args.sampling_rate;
    //     src_format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    //     src_format_pcm.containerSize = 16;
    //     src_format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    //     src_format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

    //     audio_src.pFormat = &src_format_pcm;
    // }

    // audio_src.pLocator = &src_loc_bufq;

    // // configure audio sink
    // SLDataLocator_OutputMix sink_outmix;
    // SLDataSink audio_sink = {};

    // sink_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    // sink_outmix.outputMix = obj_outmix.self();

    // audio_sink.pLocator = &sink_outmix;
    // audio_sink.pFormat = nullptr;

    // CSLObjectItf obj_player;
    // CSLAndroidConfigurationItf config;
    // CSLAndroidSimpleBufferQueueItf buffer_queue;
    // CSLPlayItf player;
    // CSLVolumeItf volume;

    // // create audio player
    // {
    //     SLInterfaceID ids[7];
    //     SLboolean req[7];
    //     SLuint32 cnt = 0;

    //     ids[cnt] = config.getIID();
    //     req[cnt] = SL_BOOLEAN_TRUE;
    //     ++cnt;

    //     ids[cnt] = buffer_queue.getIID();
    //     req[cnt] = SL_BOOLEAN_TRUE;
    //     ++cnt;

    //     ids[cnt] = volume.getIID();
    //     req[cnt] = SL_BOOLEAN_FALSE;
    //     ++cnt;

    //     // effect send
    //     if (args.opts & (OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB | OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB)) {
    //         ids[cnt] = CSLEffectSendItf::sGetIID();
    //         req[cnt] = SL_BOOLEAN_FALSE;
    //         ++cnt;
    //     }

    //     // bass boost
    //     if (args.opts & OSLMP_CONTEXT_OPTION_USE_BASSBOOST) {
    //         ids[cnt] = CSLBassBoostItf::sGetIID();
    //         req[cnt] = SL_BOOLEAN_FALSE;
    //         ++cnt;
    //     }

    //     // virtualizer
    //     if (args.opts & OSLMP_CONTEXT_OPTION_USE_VIRTUALIZER) {
    //         ids[cnt] = CSLVirtualizerItf::sGetIID();
    //         req[cnt] = SL_BOOLEAN_FALSE;
    //         ++cnt;
    //     }

    //     // equalizer
    //     if (args.opts & OSLMP_CONTEXT_OPTION_USE_EQUALIZER) {
    //         ids[cnt] = CSLEqualizerItf::sGetIID();
    //         req[cnt] = SL_BOOLEAN_FALSE;
    //         ++cnt;
    //     }

    //     slResult = engine.CreateAudioPlayer(&obj_player, &audio_src, &audio_sink, cnt, ids, req);

    //     if (!IS_SL_RESULT_SUCCESS(slResult))
    //         return TRANSLATE_RESULT(slResult);
    // }

    // // get configuration interface
    // slResult = obj_player.GetInterface(&config);
    // if (!IS_SL_RESULT_SUCCESS(slResult))
    //     return TRANSLATE_RESULT(slResult);

    // // configure the player
    // slResult = config.SetConfiguration(SL_ANDROID_KEY_STREAM_TYPE, &sl_stream_type, sizeof(SLint32));
    // if (!IS_SL_RESULT_SUCCESS(slResult))
    //     return TRANSLATE_RESULT(slResult);

    // // realize the player
    // slResult = obj_player.Realize(SL_BOOLEAN_FALSE);
    // if (!IS_SL_RESULT_SUCCESS(slResult))
    //     return TRANSLATE_RESULT(slResult);

    // // get the play interface
    // slResult = obj_player.GetInterface(&player);
    // if (!IS_SL_RESULT_SUCCESS(slResult))
    //     return TRANSLATE_RESULT(slResult);

    // // get buffer queue interface
    // slResult = obj_player.GetInterface(&buffer_queue);
    // if (!IS_SL_RESULT_SUCCESS(slResult))
    //     return TRANSLATE_RESULT(slResult);

    // // get the volume interface
    // slResult = obj_player.GetInterface(&volume);
    // if (!IS_SL_RESULT_SUCCESS(slResult))
    //     return TRANSLATE_RESULT(slResult);

    // // set pipe user
    // result = args.pipe_manager->setSinkPipeOutPortUser(args.pipe, pipe_user, true);
    // if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS))
    //     return result;

    // // bind buffer
    // result = queue_binder_.initialize(args.context, args.pipe, &buffer_queue, num_player_blocks);
    // if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS)) {
    //     args.pipe_manager->setSinkPipeOutPortUser(args.pipe, pipe_user, false);
    //     return result;
    // }

    // // update fields
    // player_ = std::move(player);
    // buffer_queue_ = std::move(buffer_queue);
    // volume_ = std::move(volume);

    // obj_player_ = std::move(obj_player);

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

opensles::CSLObjectItf *AudioSinkAudioTrackBackend::onGetPlayerObj() const noexcept
{
    return nullptr;
}

uint32_t AudioSinkAudioTrackBackend::onGetLatencyInFrames() const noexcept
{
    return static_cast<uint32_t>(block_size_in_frames_ * (num_pipe_blocks_ - 1));
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
        size_in_frames = buffer_size_in_frames;
        (void) ::memset(buffer, 0, (bytes_per_sample * size_in_frames * num_channels));
    }

    return size_in_frames;
}

} // namespace impl
} // namespace oslmp
