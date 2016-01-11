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

// #define LOG_TAG "ASOpenSLBackend"
// #define NB_LOG_TAG "NB ASOpenSLBackend"

#include "oslmp/impl/AudioSinkOpenSLBackend.hpp"

#include <SLESCXX/OpenSLES_CXX.hpp>

#include <oslmp/OpenSLMediaPlayer.hpp>
#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLES_Android_Complements.h"
#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/AudioDataPipeManager.hpp"


#define TRANSLATE_RESULT(result) InternalUtils::sTranslateOpenSLErrorCode(result)

#define IS_SL_RESULT_SUCCESS(slresult) (CXXPH_LIKELY((slresult) == SL_RESULT_SUCCESS))


namespace oslmp {
namespace impl {

using namespace ::opensles;


struct AudioSinkOpenSLBackend::AuxEffectStatus {
    int active_effect_id;
    bool environmental_reverb_enabled;
    bool preset_reverb_enabled;
    float send_level;
    SLmillibel send_level_millibel;

    AuxEffectStatus()
        : active_effect_id(OSLMP_AUX_EFFECT_NULL), environmental_reverb_enabled(false), preset_reverb_enabled(false),
          send_level(0.0f), send_level_millibel(SL_MILLIBEL_MIN)
    {
    }
};

typedef OpenSLMediaPlayerInternalUtils InternalUtils;


//
// Utilities
//

static inline SLmillibel linearToMillibel(float linear) noexcept
{
    const float LIMIT_MIN_DB = 0.000251189f; // -72 dB
    const float clipped = (std::min)((std::max)(linear, 0.0f), 1.0f);

    if (clipped < LIMIT_MIN_DB) {
        return SL_MILLIBEL_MIN;
    } else {
        return static_cast<SLmillibel>((1000 * 2) * std::log10(clipped));
    }
}


//
// AudioSinkOpenSLBackend
//
AudioSinkOpenSLBackend::AudioSinkOpenSLBackend()
    : opts_(0), AudioSinkBackend(), block_size_in_frames_(0), num_player_blocks_(0), num_pipe_blocks_(0), aux_()
{
    aux_.reset(new(std::nothrow) AuxEffectStatus());
}

AudioSinkOpenSLBackend::~AudioSinkOpenSLBackend() {
    releaseOpenSLResources();
    queue_binder_.release();
}

int AudioSinkOpenSLBackend::onInitialize(const AudioSink::initialize_args_t &args, void *pipe_user) noexcept
{
    SLint32 sl_stream_type;
    SLresult slResult;
    int result;

    if (!InternalUtils::sTranslateToOpenSLStreamType(args.stream_type, &sl_stream_type))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    const int block_size_in_frames = args.pipe_manager->getBlockSizeInFrames();
    const uint32_t num_pipe_blocks = args.pipe->getNumberOfBufferItems();
    const uint32_t num_player_blocks = args.num_player_blocks;

    if (!(num_pipe_blocks > (num_player_blocks + 1))) { // +1: silent block
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    CSLEngineItf engine;
    CSLObjectItf obj_outmix;

    slResult = args.context->getInterfaceFromEngine(&engine);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // create output mix, with reverb objects specified as a non-required interface
    {
        SLInterfaceID ids[2];
        SLboolean req[2];
        SLuint32 cnt = 0;

        // environmental reverb
        if (args.opts & OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB) {
            ids[cnt] = CSLEnvironmentalReverbItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        // preset reverb
        if (args.opts & OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB) {
            ids[cnt] = CSLPresetReverbItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        result = engine.CreateOutputMix(&obj_outmix, cnt, ids, req);
    }

    if (result != SL_RESULT_SUCCESS)
        return TRANSLATE_RESULT(result);

    // realize the output mix
    result = obj_outmix.Realize(SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS)
        return TRANSLATE_RESULT(result);

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue src_loc_bufq = {};
    SLDataFormat_PCM src_format_pcm = {};
    SLAndroidDataFormat_PCM_EX src_format_pcm_ex = {};
    SLDataSource audio_src = {};

    src_loc_bufq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    src_loc_bufq.numBuffers = num_player_blocks;

    if (args.sample_format == kAudioSampleFormatType_F32) {
        src_format_pcm_ex.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
        src_format_pcm_ex.numChannels = AudioSink::NUM_CHANNELS;
        src_format_pcm_ex.sampleRate = args.sampling_rate;
        src_format_pcm_ex.bitsPerSample = 32;
        src_format_pcm_ex.containerSize = 32;
        src_format_pcm_ex.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        src_format_pcm_ex.endianness = SL_BYTEORDER_LITTLEENDIAN;
        src_format_pcm_ex.representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;

        audio_src.pFormat = &src_format_pcm_ex;
    } else {
        src_format_pcm.formatType = SL_DATAFORMAT_PCM;
        src_format_pcm.numChannels = AudioSink::NUM_CHANNELS;
        src_format_pcm.samplesPerSec = args.sampling_rate;
        src_format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
        src_format_pcm.containerSize = 16;
        src_format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        src_format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

        audio_src.pFormat = &src_format_pcm;
    }

    audio_src.pLocator = &src_loc_bufq;

    // configure audio sink
    SLDataLocator_OutputMix sink_outmix;
    SLDataSink audio_sink = {};

    sink_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    sink_outmix.outputMix = obj_outmix.self();

    audio_sink.pLocator = &sink_outmix;
    audio_sink.pFormat = nullptr;

    CSLObjectItf obj_player;
    CSLAndroidConfigurationItf config;
    CSLAndroidSimpleBufferQueueItf buffer_queue;
    CSLPlayItf player;
    CSLVolumeItf volume;

    // create audio player
    {
        SLInterfaceID ids[7];
        SLboolean req[7];
        SLuint32 cnt = 0;

        ids[cnt] = config.getIID();
        req[cnt] = SL_BOOLEAN_TRUE;
        ++cnt;

        ids[cnt] = buffer_queue.getIID();
        req[cnt] = SL_BOOLEAN_TRUE;
        ++cnt;

        ids[cnt] = volume.getIID();
        req[cnt] = SL_BOOLEAN_FALSE;
        ++cnt;

        // effect send
        if (args.opts & (OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB | OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB)) {
            ids[cnt] = CSLEffectSendItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        // bass boost
        if (args.opts & OSLMP_CONTEXT_OPTION_USE_BASSBOOST) {
            ids[cnt] = CSLBassBoostItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        // virtualizer
        if (args.opts & OSLMP_CONTEXT_OPTION_USE_VIRTUALIZER) {
            ids[cnt] = CSLVirtualizerItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        // equalizer
        if (args.opts & OSLMP_CONTEXT_OPTION_USE_EQUALIZER) {
            ids[cnt] = CSLEqualizerItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        slResult = engine.CreateAudioPlayer(&obj_player, &audio_src, &audio_sink, cnt, ids, req);

        if (!IS_SL_RESULT_SUCCESS(slResult))
            return TRANSLATE_RESULT(slResult);
    }

    // get configuration interface
    slResult = obj_player.GetInterface(&config);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // configure the player
    slResult = config.SetConfiguration(SL_ANDROID_KEY_STREAM_TYPE, &sl_stream_type, sizeof(SLint32));
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // realize the player
    slResult = obj_player.Realize(SL_BOOLEAN_FALSE);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the play interface
    slResult = obj_player.GetInterface(&player);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get buffer queue interface
    slResult = obj_player.GetInterface(&buffer_queue);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the volume interface
    slResult = obj_player.GetInterface(&volume);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // set pipe user
    result = args.pipe_manager->setSinkPipeOutPortUser(args.pipe, pipe_user, true);
    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS))
        return result;

    // bind buffer
    result = queue_binder_.initialize(args.context, args.pipe, &buffer_queue, num_player_blocks);
    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS)) {
        args.pipe_manager->setSinkPipeOutPortUser(args.pipe, pipe_user, false);
        return result;
    }

    // update fields
    player_ = std::move(player);
    buffer_queue_ = std::move(buffer_queue);
    volume_ = std::move(volume);

    obj_outmix_ = std::move(obj_outmix);
    obj_player_ = std::move(obj_player);

    block_size_in_frames_ = block_size_in_frames;
    num_player_blocks_ = num_player_blocks;
    num_pipe_blocks_ = num_pipe_blocks;
    opts_ = args.opts;
    pipe_ = args.pipe;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSinkOpenSLBackend::onStart() noexcept
{
    int result = queue_binder_.beforeStart(&buffer_queue_);
    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS))
        return result;

    SLresult slResult;

    slResult = buffer_queue_.RegisterCallback(playbackBufferQueueCallback, this);
    if (!IS_SL_RESULT_SUCCESS(slResult)) {
        return TRANSLATE_RESULT(slResult);
    }

    slResult = player_.SetPlayState(SL_PLAYSTATE_PLAYING);

    return TRANSLATE_RESULT(slResult);
}

int AudioSinkOpenSLBackend::onPause() noexcept
{
    SLresult slResult;

    slResult = player_.SetPlayState(SL_PLAYSTATE_STOPPED);
    (void)buffer_queue_.RegisterCallback(NULL, nullptr);

    queue_binder_.afterStop(&buffer_queue_);

    return TRANSLATE_RESULT(slResult);
}

int AudioSinkOpenSLBackend::onResume() noexcept
{
    SLresult slResult = player_.SetPlayState(SL_PLAYSTATE_PLAYING);

    return TRANSLATE_RESULT(slResult);
}

int AudioSinkOpenSLBackend::onStop() noexcept
{
    SLresult slResult;

    slResult = player_.SetPlayState(SL_PLAYSTATE_STOPPED);

    (void) queue_binder_.afterStop(&buffer_queue_);

    return OSLMP_RESULT_SUCCESS;
}

uint32_t AudioSinkOpenSLBackend::onGetLatencyInFrames() const noexcept
{
    return static_cast<uint32_t>(block_size_in_frames_ * (num_pipe_blocks_ - 1));
}

int32_t AudioSinkOpenSLBackend::onGetAudioSessionId() const noexcept
{
    // can not retrieve the session id because OpenSL ES does not expose a corresponding method 
    return 0;
}

int AudioSinkOpenSLBackend::onSelectActiveAuxEffect(int aux_effect_id) noexcept
{
    if (!aux_) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    bool updated = false;

    switch (aux_effect_id) {
    case OSLMP_AUX_EFFECT_NULL:
    case OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB:
    case OSLMP_AUX_EFFECT_PRESET_REVERB:
        if (aux_->active_effect_id != aux_effect_id) {
            aux_->active_effect_id = aux_effect_id;
            updated = true;
        }
        break;
    default:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (!updated) {
        return OSLMP_RESULT_SUCCESS;
    }

    return applyActiveAuxEffectSettings();
}

int AudioSinkOpenSLBackend::onSetAuxEffectSendLevel(float level) noexcept
{
    if (!aux_) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    if (aux_->send_level == level) {
        return OSLMP_RESULT_SUCCESS;
    }

    aux_->send_level = level;
    aux_->send_level_millibel = linearToMillibel(aux_->send_level);

    return applyActiveAuxEffectSettings();
}

int AudioSinkOpenSLBackend::onSetAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept
{
    if (!aux_) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    bool updated = false;

    switch (aux_effect_id) {
    case OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB:
        if (aux_->environmental_reverb_enabled != enabled) {
            aux_->environmental_reverb_enabled = enabled;
            updated = true;
        }
        break;
    case OSLMP_AUX_EFFECT_PRESET_REVERB:
        if (aux_->preset_reverb_enabled != enabled) {
            aux_->preset_reverb_enabled = enabled;
            updated = true;
        }
        break;
    case OSLMP_AUX_EFFECT_NULL:
    default:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (!updated) {
        return OSLMP_RESULT_SUCCESS;
    }

    return applyActiveAuxEffectSettings();
}

SLresult AudioSinkOpenSLBackend::onGetInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept
{
    const SLInterfaceID iid = itf->getIID();

    if (iid == CSLObjectItf::sGetIID()) {
        // if the interface is SLObjectItf, auto-destroying is not permitted
        if (dynamic_cast<const CSLObjectItf *>(itf)->autoDestroy()) {
            LOGE("getInterfaceFromOutputMixer() - CSLObjectItf::autoDestroy() is not permitted");
            return SL_RESULT_PARAMETER_INVALID;
        }
    }

    // masking
    uint32_t opts_mask = 0;

    if (iid == CSLEnvironmentalReverbItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB;
    } else if (iid == CSLPresetReverbItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB;
    }

    if (opts_mask) {
        if (!(opts_ & opts_mask)) {
            return SL_RESULT_FEATURE_UNSUPPORTED;
        }
    }

    if (!obj_outmix_) {
        return SL_RESULT_RESOURCE_ERROR;
    }

    return obj_outmix_.GetInterface(itf);
}

SLresult AudioSinkOpenSLBackend::onGetInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept
{
    const SLInterfaceID iid = itf->getIID();

    if (iid == CSLObjectItf::sGetIID()) {
        // if the interface is SLObjectItf, auto-destroying is not permitted
        if (dynamic_cast<const CSLObjectItf *>(itf)->autoDestroy()) {
            LOGE("getInterfaceFromSinkPlayer() - CSLObjectItf::autoDestroy() is not permitted");
            return SL_RESULT_PARAMETER_INVALID;
        }
    }

    // masking
    uint32_t opts_mask = 0;

    if (iid == CSLBassBoostItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_BASSBOOST;
    } else if (iid == CSLVirtualizerItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_VIRTUALIZER;
    } else if (iid == CSLEqualizerItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_EQUALIZER;
    }

    if (opts_mask) {
        if (!(opts_ & opts_mask)) {
            return SL_RESULT_FEATURE_UNSUPPORTED;
        }
    }

    if (!obj_player_) {
        return SL_RESULT_RESOURCE_ERROR;
    }

    return obj_player_.GetInterface(itf);
}

int AudioSinkOpenSLBackend::applyActiveAuxEffectSettings() noexcept
{
    CSLEffectSendItf effect_send;
    CSLEnvironmentalReverbItf env_reverb;
    CSLPresetReverbItf preset_reverb;

    (void)onGetInterfaceFromSinkPlayer(&effect_send);
    (void)onGetInterfaceFromOutputMixer(&env_reverb);
    (void)onGetInterfaceFromOutputMixer(&preset_reverb);

    SLresult slResult = SL_RESULT_UNKNOWN_ERROR;

    switch (aux_->active_effect_id) {
    case OSLMP_AUX_EFFECT_NULL:
        (void)applyAuxEffectSettings(effect_send, env_reverb.self(), SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        (void)applyAuxEffectSettings(effect_send, preset_reverb.self(), SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        slResult = SL_RESULT_SUCCESS;
        break;
    case OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB:
        (void)applyAuxEffectSettings(effect_send, preset_reverb.self(), SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        slResult =
            applyAuxEffectSettings(effect_send, env_reverb.self(), CSLboolean::toSL(aux_->environmental_reverb_enabled),
                                   aux_->send_level_millibel);
        break;
    case OSLMP_AUX_EFFECT_PRESET_REVERB:
        (void)applyAuxEffectSettings(effect_send, env_reverb.self(), SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        slResult = applyAuxEffectSettings(effect_send, preset_reverb.self(),
                                          CSLboolean::toSL(aux_->preset_reverb_enabled), aux_->send_level_millibel);
        break;
    default:
        slResult = SL_RESULT_INTERNAL_ERROR;
        break;
    }

    return InternalUtils::sTranslateOpenSLErrorCode(slResult);
}

SLresult AudioSinkOpenSLBackend::applyAuxEffectSettings(opensles::CSLEffectSendItf &effect_send, const void *aux_effect,
                                                   SLboolean enabled, SLmillibel sendLevel) noexcept
{
    if (!(effect_send && aux_effect))
        return SL_RESULT_FEATURE_UNSUPPORTED;

    SLboolean cur_is_enabled = SL_BOOLEAN_FALSE;
    SLresult result;

    result = effect_send.IsEnabled(aux_effect, &cur_is_enabled);

    if (result != SL_RESULT_SUCCESS) {
        return result;
    }

    if (enabled && (sendLevel > SL_MILLIBEL_MIN)) {
        // enabled
        if (cur_is_enabled) {
            result = effect_send.SetSendLevel(aux_effect, sendLevel);
        } else {
            result = effect_send.EnableEffectSend(aux_effect, SL_BOOLEAN_TRUE, sendLevel);
        }
    } else {
        // disabled
        if (cur_is_enabled) {
            result = effect_send.EnableEffectSend(aux_effect, SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        }
    }

    return result;
}

void AudioSinkOpenSLBackend::releaseOpenSLResources() noexcept
{
    volume_.unbind();
    buffer_queue_.unbind();
    player_.unbind();

    obj_player_.Destroy();
    obj_outmix_.Destroy();
}

void AudioSinkOpenSLBackend::playbackBufferQueueCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) noexcept
{
    AudioSinkOpenSLBackend *thiz = static_cast<AudioSinkOpenSLBackend *>(pContext);
    AudioSinkDataPipe *pipe = thiz->pipe_;

    CSLAndroidSimpleBufferQueueItf buffer_queue;
    buffer_queue.assign(caller);

    thiz->queue_binder_.handleCallback(pipe, &buffer_queue);
}

} // namespace impl
} // namespace oslmp
