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

// #define LOG_TAG "AudioSystem"
// #define NB_LOG_TAG "NB AudioSystem"

#include "oslmp/impl/AudioSystem.hpp"

#include <vector>
#include <algorithm>
#include <cstring>

#include <cxxporthelper/cmath>
#include <cxxporthelper/memory>

#include <cxxdasp/cxxdasp.hpp>

#include <SLESCXX/OpenSLES_CXX.hpp>

#include <oslmp/OpenSLMediaPlayer.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/AudioPlayer.hpp"
#include "oslmp/impl/AudioSink.hpp"
#include "oslmp/impl/AudioMixer.hpp"
#include "oslmp/impl/AudioDataPipeManager.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"
#include "oslmp/impl/PreAmp.hpp"
#include "oslmp/impl/HQEqualizer.hpp"
#include "oslmp/utils/timespec_utils.hpp"

#define TRANSLATE_RESULT(result) InternalUtils::sTranslateOpenSLErrorCode(result)

#define MATCH_PLAYER(player, id)                                                                                       \
    [player, id](const audio_player_info_t &c) { return (c.id == (id)) && (c.instance == (player)); }

// #define DEBUG_PROFILE_POLLING_METHOD

#ifdef DEBUG_PROFILE_POLLING_METHOD
#define DEBUG_PROFILE_POLLING_PROCESS(process) process
#else
#define DEBUG_PROFILE_POLLING_PROCESS(process)
#endif
#define DEBUG_PROFILE_POLLING_GET_TIMESTAMP(ts)                                                                        \
    DEBUG_PROFILE_POLLING_PROCESS(utils::timespec_utils::get_current_time(ts))

namespace oslmp {
namespace impl {

using namespace ::opensles;

typedef OpenSLMediaPlayerInternalUtils InternalUtils;

struct audio_player_info_t {
    uint32_t id;
    AudioPlayer *instance;
    bool is_active;

    audio_player_info_t() : id(0), instance(nullptr), is_active(false) {}

    audio_player_info_t(uint32_t id, AudioPlayer *instance) : id(id), instance(instance), is_active(false) {}
};

struct AuxEffectStatus {
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

class AudioSystem::Impl {
public:
    Impl();
    ~Impl();

    int initialize(const initialize_args_t &args) noexcept;

    SLresult getInterfaceFromEngine(opensles::CSLInterface *itf) noexcept;
    SLresult getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept;
    SLresult getInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept;

    AudioDataPipeManager *getPipeManager() const noexcept;
    AudioMixer *getMixer() const noexcept;

    int determineNextPollingTime() const noexcept;
    int poll() noexcept;

    int registerAudioPlayer(AudioPlayer *player, uint32_t &id) noexcept;
    int unregisterAudioPlayer(AudioPlayer *player, uint32_t id) noexcept;
    bool checkAudioPlayerIsAlive(AudioPlayer *player, uint32_t id) const noexcept;
    int notifyAudioPlayerState(AudioPlayer *player, uint32_t id, bool is_active) noexcept;

    void setAudioCaptureEventListener(AudioCaptureEventListener *listener) noexcept;
    void setAudioCaptureEnabled(bool enabled) noexcept;

    int selectActiveAuxEffect(int aux_effect_id) noexcept;
    int setAuxEffectSendLevel(float level) noexcept;
    int setAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept;

    int setAudioStreamType(int streamtype) noexcept;
    int getAudioStreamType(int *streamtype) const noexcept;

    int getSystemOutputSamplingRate(uint32_t *sampling_rate) const noexcept;
    int getOutputLatencyInFrames(uint32_t *latency) const noexcept;

    int getParamResamplerQualityLevel(uint32_t *quality_level) const noexcept;

    int getPreAmp(PreAmp **p_preamp) const noexcept;
    int getHQEqualizer(HQEqualizer **p_hq_equalizer) const noexcept;

private:
    int initSubmodules(const AudioSystem::initialize_args_t &args, uint32_t output_frame_size,
                       std::unique_ptr<AudioSink> &sink, std::unique_ptr<AudioDataPipeManager> &pipe_mgr,
                       std::unique_ptr<AudioMixer> &mixer, MixedOutputAudioEffect *mixout_effects[]) const noexcept;

    int initEngineOutputMix(uint32_t opts, CSLObjectItf &engineObj, CSLObjectItf &outputMixObj) const noexcept;

    int initMixOutAudioEffects(const AudioSystem::initialize_args_t &args, uint32_t opts, uint32_t output_frame_size,
                               uint32_t sampling_rate, std::unique_ptr<HQEqualizer> &hq_equalizer) const noexcept;

    int initPreAmp(uint32_t opts, std::unique_ptr<PreAmp> &preamp, const std::unique_ptr<AudioMixer> &mixer) const
        noexcept;

    int applyActiveAuxEffectSettings() noexcept;

    void pollControlSinkMute() noexcept;
    void pollControlMixerAndSinkSuspendResume() noexcept;
    void pollObtainCapturedAudioData() noexcept;

    static SLresult applyAuxEffectSettings(CSLEffectSendItf &effect_send, const void *aux_effect, SLboolean enabled,
                                           SLmillibel sendLevel) noexcept;

    static bool check_is_low_latency(const initialize_args_t &args) noexcept;
    static uint32_t determine_output_frame_size(const initialize_args_t &args, bool is_low_latency) noexcept;
    static uint32_t calc_android_NormalMixer_FrameCount(uint32_t frame_count, uint32_t sample_rate_hz) noexcept;

private:
    OpenSLMediaPlayerInternalContext *context_;

    initialize_args_t init_args_;

    bool is_low_latency_mode_;
    uint32_t output_frame_size_;

    std::unique_ptr<AudioSink> sink_;
    std::unique_ptr<AudioMixer> mixer_;
    std::unique_ptr<AudioDataPipeManager> pipe_mgr_;

    AudioCaptureDataPipe *capture_pipe_;
    AudioCaptureEventListener *audio_capture_event_listener_;

    timespec ts_mixer_enter_can_suspend_;
    timespec ts_prev_polling_;

    CSLObjectItf objEngine_;
    CSLObjectItf objOutputMixer_;

    std::vector<audio_player_info_t> audio_players_info_;
    uint32_t audio_player_player_id_counter_;

    AuxEffectStatus aux_;

    std::unique_ptr<PreAmp> preamp_;
    std::unique_ptr<HQEqualizer> mixout_effect_hq_equalizer_;

    bool audio_player_instance_updated_;
};

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

static inline uint32_t round_up_16(uint32_t x) { return (x + (16U - 1U)) & ~(16U - 1U); }

static inline uint32_t round_down_16(uint32_t x) { return (x) & ~(16U - 1U); }

//
// AudioSystem
//

AudioSystem::AudioSystem() : impl_(new (std::nothrow) Impl()) {}

AudioSystem::~AudioSystem() {}

int AudioSystem::initialize(const initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

SLresult AudioSystem::getInterfaceFromEngine(opensles::CSLInterface *itf) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SL_RESULT_INTERNAL_ERROR;
    return impl_->getInterfaceFromEngine(itf);
}

SLresult AudioSystem::getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SL_RESULT_INTERNAL_ERROR;
    return impl_->getInterfaceFromOutputMixer(itf);
}

SLresult AudioSystem::getInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SL_RESULT_INTERNAL_ERROR;
    return impl_->getInterfaceFromSinkPlayer(itf);
}

AudioDataPipeManager *AudioSystem::getPipeManager() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return nullptr;
    return impl_->getPipeManager();
}

AudioMixer *AudioSystem::getMixer() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return nullptr;
    return impl_->getMixer();
}

int AudioSystem::determineNextPollingTime() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return -1; // infinity
    return impl_->determineNextPollingTime();
}

int AudioSystem::poll() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->poll();
}

int AudioSystem::registerAudioPlayer(AudioPlayer *player, uint32_t &id) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->registerAudioPlayer(player, id);
}

int AudioSystem::unregisterAudioPlayer(AudioPlayer *player, uint32_t id) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->unregisterAudioPlayer(player, id);
}

bool AudioSystem::checkAudioPlayerIsAlive(AudioPlayer *player, uint32_t id) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->checkAudioPlayerIsAlive(player, id);
}

int AudioSystem::notifyAudioPlayerState(AudioPlayer *player, uint32_t id, bool is_active) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->notifyAudioPlayerState(player, id, is_active);
}

void AudioSystem::setAudioCaptureEventListener(AudioCaptureEventListener *listener) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return;
    impl_->setAudioCaptureEventListener(listener);
}

void AudioSystem::setAudioCaptureEnabled(bool enabled) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return;
    impl_->setAudioCaptureEnabled(enabled);
}

int AudioSystem::selectActiveAuxEffect(int aux_effect_id) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->selectActiveAuxEffect(aux_effect_id);
}

int AudioSystem::setAuxEffectSendLevel(float level) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAuxEffectSendLevel(level);
}

int AudioSystem::setAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAuxEffectEnabled(aux_effect_id, enabled);
}

int AudioSystem::setAudioStreamType(int streamtype) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAudioStreamType(streamtype);
}

int AudioSystem::getAudioStreamType(int *streamtype) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getAudioStreamType(streamtype);
}

int AudioSystem::getSystemOutputSamplingRate(uint32_t *sampling_rate) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getSystemOutputSamplingRate(sampling_rate);
}

int AudioSystem::getOutputLatencyInFrames(uint32_t *latency) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getOutputLatencyInFrames(latency);
}

int AudioSystem::getParamResamplerQualityLevel(uint32_t *quality_level) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getParamResamplerQualityLevel(quality_level);
}

int AudioSystem::getPreAmp(PreAmp **p_preamp) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getPreAmp(p_preamp);
}

int AudioSystem::getHQEqualizer(HQEqualizer **p_hq_equalizer) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getHQEqualizer(p_hq_equalizer);
}

//
// AudioSystem::Impl
//

AudioSystem::Impl::Impl()
    : context_(nullptr), init_args_(), is_low_latency_mode_(false), output_frame_size_(0), sink_(), mixer_(),
      pipe_mgr_(), capture_pipe_(nullptr), audio_capture_event_listener_(nullptr), objEngine_(), objOutputMixer_(),
      audio_players_info_(), audio_player_player_id_counter_(0), aux_(),
      ts_mixer_enter_can_suspend_(utils::timespec_utils::ZERO()), ts_prev_polling_(utils::timespec_utils::ZERO()),
      preamp_(), mixout_effect_hq_equalizer_(), audio_player_instance_updated_(false)
{
    cxxdasp::cxxdasp_init();
}

AudioSystem::Impl::~Impl()
{
    if (pipe_mgr_ && capture_pipe_) {
        pipe_mgr_->setCapturePipeOutPortUser(capture_pipe_, this, false);
    }

    mixer_.reset();
    sink_.reset();

    objOutputMixer_.Destroy();
    objEngine_.Destroy();

    pipe_mgr_.reset();

    capture_pipe_ = nullptr;
    audio_capture_event_listener_ = nullptr;

    context_ = nullptr;
}

int AudioSystem::Impl::initialize(const initialize_args_t &args) noexcept
{
    std::unique_ptr<AudioDataPipeManager> pipe_mgr;
    std::unique_ptr<AudioSink> sink;
    std::unique_ptr<AudioMixer> mixer;
    CSLObjectItf engineObj;
    CSLObjectItf outputMixObj;
    std::unique_ptr<PreAmp> preamp;
    std::unique_ptr<HQEqualizer> mixout_effect_hq_equalizer;
    MixedOutputAudioEffect *mixout_effects[AudioMixer::NUM_MAX_MIXOOUT_EFFECTS] = { nullptr };
    int result;

    const bool is_low_latency_mode = check_is_low_latency(args);
    const uint32_t output_frame_size = determine_output_frame_size(args, is_low_latency_mode);
    const uint32_t context_opts = args.context->getContextOptions();

// check parameter
#if !CXXDASP_ENABLE_POLYPHASE_RESAMPLER_FACTORY_LOW_QUALITY
    if (!args.use_high_quality_resampler) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
#endif
#if !CXXDASP_ENABLE_POLYPHASE_RESAMPLER_FACTORY_HIGH_QUALITY
    if (args.use_high_quality_resampler) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
#endif

    // initialize OpenSL Engine & OutputMixer
    result = initEngineOutputMix(context_opts, engineObj, outputMixObj);

    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    // initialize mixed output effects
    result = initMixOutAudioEffects(args, context_opts, output_frame_size, args.system_out_sampling_rate,
                                    mixout_effect_hq_equalizer);

    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    objEngine_ = std::move(engineObj);
    objOutputMixer_ = std::move(outputMixObj);
    context_ = args.context;

    // initialize sub modules
    mixout_effects[0] = mixout_effect_hq_equalizer.get();
    result = initSubmodules(args, output_frame_size, sink, pipe_mgr, mixer, mixout_effects);

    if (result != OSLMP_RESULT_SUCCESS) {
        objEngine_.Destroy();
        objOutputMixer_.Destroy();
        context_ = nullptr;
        return result;
    }

    // set up capture pipe
    AudioCaptureDataPipe *capture_pipe = mixer->getCapturePipe();

    result = pipe_mgr->setCapturePipeOutPortUser(capture_pipe, this, true);
    if (result != OSLMP_RESULT_SUCCESS) {
        objEngine_.Destroy();
        objOutputMixer_.Destroy();
        context_ = nullptr;
        return result;
    }

    // initialize pre.amp module
    // (ignore the result because preamp module is optional)
    (void)initPreAmp(context_opts, preamp, mixer);

    // update fields
    init_args_ = args;

    is_low_latency_mode_ = is_low_latency_mode;
    output_frame_size_ = output_frame_size;

    sink_ = std::move(sink);
    mixer_ = std::move(mixer);
    pipe_mgr_ = std::move(pipe_mgr);
    capture_pipe_ = capture_pipe;
    preamp_ = std::move(preamp);
    mixout_effect_hq_equalizer_ = std::move(mixout_effect_hq_equalizer);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::initSubmodules(const AudioSystem::initialize_args_t &args, uint32_t output_frame_size,
                                      std::unique_ptr<AudioSink> &sink, std::unique_ptr<AudioDataPipeManager> &pipe_mgr,
                                      std::unique_ptr<AudioMixer> &mixer,
                                      MixedOutputAudioEffect *mixout_effects[]) const noexcept
{

    const uint32_t kSourcePipeMinDurationInMsec = 2000;  // 2 sec.
    const uint32_t kSourcePipeRoomDurationInMsec = 1000; // 1 sec.
    const uint32_t kSourcePipeMaxNumBlocks = static_cast<uint32_t>(AudioSourceDataPipe::MAX_BUFFER_ITEM_COUNT);

    const uint32_t kAudioMixerSinkPooledNumBlocks = 4;
    const uint32_t kSinkPlayerNumBlocks = 4;
    const uint32_t kSinkPipeNumBlocks =
        kSinkPlayerNumBlocks + kAudioMixerSinkPooledNumBlocks + 1; // +1: silent buffer internally used in AudioSink

    const uint32_t kCapturePipeDurationInMsec = 200;
    const uint32_t kCapturePipeMinNumBlocks = 8;
    const uint32_t kCapturePipeMaxNumBlocks = static_cast<uint32_t>(AudioCaptureDataPipe::MAX_BUFFER_ITEM_COUNT);

    // instantiate modules
    sink.reset(new (std::nothrow) AudioSink());
    pipe_mgr.reset(new (std::nothrow) AudioDataPipeManager());
    mixer.reset(new (std::nothrow) AudioMixer());

    if (!(sink && pipe_mgr && mixer)) {
        return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
    }

    const uint32_t sampling_rate_hz = (args.system_out_sampling_rate / 1000);
    const uint32_t source_pipe_duration_ms =
        (std::max)(kSourcePipeMinDurationInMsec, (args.long_fade_duration_ms + kSourcePipeRoomDurationInMsec));

    int result;
    AudioSinkDataPipe *sink_pipe = nullptr;
    AudioCaptureDataPipe *capture_pipe = nullptr;
    AudioDataPipeManager::initialize_args_t pipe_mgr_args;
    sample_format_type sink_sample_format =
        (args.system_supports_floating_point) ? kAudioSampleFormatType_F32 : kAudioSampleFormatType_S16;

    pipe_mgr_args.sink_format_type = sink_sample_format;
    pipe_mgr_args.source_num_items =
        (std::min)(kSourcePipeMaxNumBlocks, ((source_pipe_duration_ms * sampling_rate_hz / 1000) / output_frame_size));
    pipe_mgr_args.sink_num_items = kSinkPipeNumBlocks;
    pipe_mgr_args.capture_num_items =
        std::max(kCapturePipeMinNumBlocks,
                 (std::min)(kCapturePipeMaxNumBlocks,
                            ((kCapturePipeDurationInMsec * sampling_rate_hz / 1000) / output_frame_size)));
    pipe_mgr_args.block_size = output_frame_size;

    // initialize pipe manager
    result = pipe_mgr->initialize(pipe_mgr_args);
    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    result = pipe_mgr->obtainSinkPipe(&sink_pipe);
    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    result = pipe_mgr->obtainCapturePipe(&capture_pipe);
    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    // initialize sink
    {
        AudioSink::initialize_args_t init_args;

        init_args.context = args.context;
        init_args.opts = args.context->getContextOptions();
        init_args.sample_format = sink_sample_format;
        init_args.sampling_rate = args.system_out_sampling_rate;
        init_args.stream_type = args.stream_type;
        init_args.pipe_manager = pipe_mgr.get();
        init_args.pipe = sink_pipe;
        init_args.num_player_blocks = kSinkPlayerNumBlocks;

        result = sink->initialize(init_args);

        if (result != OSLMP_RESULT_SUCCESS)
            return result;
    }

    // initialize mixer
    {
        AudioMixer::initialize_args_t init_args;

        init_args.context = args.context;
        init_args.pipe_manager = pipe_mgr.get();
        init_args.sink_pipe = sink_pipe;
        init_args.capture_pipe = capture_pipe;
        init_args.sampling_rate = args.system_out_sampling_rate;
        init_args.short_fade_duration_ms = args.short_fade_duration_ms;
        init_args.long_fade_duration_ms = args.long_fade_duration_ms;
        init_args.num_sink_player_blocks = kSinkPlayerNumBlocks;

        for (int i = 0; i < AudioMixer::NUM_MAX_MIXOOUT_EFFECTS; ++i) {
            init_args.mixout_effects[i] = mixout_effects[i];
        }

        result = mixer->initialize(init_args);

        if (result != OSLMP_RESULT_SUCCESS)
            return result;
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::initEngineOutputMix(uint32_t opts, CSLObjectItf &engineObj, CSLObjectItf &outputMixObj) const
    noexcept
{

    CSLEngineItf engine;
    SLresult result;

    // create engine
    {
        const SLEngineOption engineOptions[] = { { static_cast<SLuint32>(SL_ENGINEOPTION_THREADSAFE),
                                                   static_cast<SLuint32>(SL_BOOLEAN_TRUE) } };

        SLInterfaceID ids[1];
        SLboolean req[1];

        ids[0] = CSLEngineCapabilitiesItf::sGetIID();
        req[0] = SL_BOOLEAN_FALSE;

        result = ::slCreateEngine(&(engineObj.self()), 1, engineOptions, 1, ids, req);

        if (result != SL_RESULT_SUCCESS)
            return TRANSLATE_RESULT(result);
    }

    // realize engine
    result = engineObj.Realize(SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS)
        return TRANSLATE_RESULT(result);

    // obtain engine interface
    result = engineObj.GetInterface(&engine);
    if (result != SL_RESULT_SUCCESS)
        return TRANSLATE_RESULT(result);

    // create output mix, with reverb objects specified as a non-required interface
    {
        SLInterfaceID ids[2];
        SLboolean req[2];
        SLuint32 cnt = 0;

        // environmental reverb
        if (opts & OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB) {
            ids[cnt] = CSLEnvironmentalReverbItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        // preset reverb
        if (opts & OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB) {
            ids[cnt] = CSLPresetReverbItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        result = engine.CreateOutputMix(&outputMixObj, cnt, ids, req);
    }
    if (result != SL_RESULT_SUCCESS)
        return TRANSLATE_RESULT(result);

    // realize the output mix
    result = outputMixObj.Realize(SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS)
        return TRANSLATE_RESULT(result);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::initMixOutAudioEffects(const AudioSystem::initialize_args_t &args, uint32_t opts,
                                              uint32_t output_frame_size, uint32_t sampling_rate,
                                              std::unique_ptr<HQEqualizer> &hq_equalizer) const noexcept
{

    if (opts & OSLMP_CONTEXT_OPTION_USE_HQ_EQUALIZER) {
        hq_equalizer.reset(new (std::nothrow) HQEqualizer());

        if (!hq_equalizer) {
            return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
        }

        HQEqualizer::initialize_args_t init_args;

        init_args.num_channels = 2;
        init_args.sampling_rate = sampling_rate;
        init_args.block_size_in_frames = output_frame_size;
        init_args.impl_type = static_cast<HQEqualizer::impl_type_specifiler>(args.hq_equalizer_impl_type);

        if (!hq_equalizer->initialize(init_args)) {
            return OSLMP_RESULT_INTERNAL_ERROR;
        }
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::initPreAmp(uint32_t opts, std::unique_ptr<PreAmp> &preamp,
                                  const std::unique_ptr<AudioMixer> &mixer) const noexcept
{

    if (opts & OSLMP_CONTEXT_OPTION_USE_PREAMP) {
        preamp.reset(new (std::nothrow) PreAmp());

        if (!preamp) {
            return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
        }

        PreAmp::initialize_args_t init_args;

        init_args.mixer = mixer.get();

        if (!preamp->initialize(init_args)) {
            return OSLMP_RESULT_INTERNAL_ERROR;
        }
    }

    return OSLMP_RESULT_SUCCESS;
}

SLresult AudioSystem::Impl::getInterfaceFromEngine(opensles::CSLInterface *itf) noexcept
{
    if (!itf)
        return SL_RESULT_PARAMETER_INVALID;

    if (itf->getIID() == CSLObjectItf::sGetIID()) {
        // if the interface is SLObjectItf, auto-destroying is not permitted
        if (dynamic_cast<const CSLObjectItf *>(itf)->autoDestroy()) {
            LOGE("getInterfaceFromEngine() - CSLObjectItf::autoDestroy() is not permitted");
            return SL_RESULT_PARAMETER_INVALID;
        }
    }

    return this->objEngine_.GetInterface(itf);
}

SLresult AudioSystem::Impl::getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept
{
    if (!itf)
        return SL_RESULT_PARAMETER_INVALID;

    const SLInterfaceID iid = itf->getIID();

    if (iid == CSLObjectItf::sGetIID()) {
        // if the interface is SLObjectItf, auto-destroying is not permitted
        if (dynamic_cast<const CSLObjectItf *>(itf)->autoDestroy()) {
            LOGE("getInterfaceFromOutputMixer() - CSLObjectItf::autoDestroy() is not permitted");
            return SL_RESULT_PARAMETER_INVALID;
        }
    }

    // masking
    const uint32_t opts = context_->getContextOptions();
    uint32_t opts_mask = 0;

    if (iid == CSLEnvironmentalReverbItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB;
    } else if (iid == CSLPresetReverbItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB;
    }

    if (opts_mask) {
        if (!(opts & opts_mask)) {
            return SL_RESULT_FEATURE_UNSUPPORTED;
        }
    }

    return this->objOutputMixer_.GetInterface(itf);
}

SLresult AudioSystem::Impl::getInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept
{
    if (!itf)
        return SL_RESULT_PARAMETER_INVALID;

    const SLInterfaceID iid = itf->getIID();

    if (iid == CSLObjectItf::sGetIID()) {
        // if the interface is SLObjectItf, auto-destroying is not permitted
        if (dynamic_cast<const CSLObjectItf *>(itf)->autoDestroy()) {
            LOGE("getInterfaceFromSinkPlayer() - CSLObjectItf::autoDestroy() is not permitted");
            return SL_RESULT_PARAMETER_INVALID;
        }
    }

    // masking
    const uint32_t opts = context_->getContextOptions();
    uint32_t opts_mask = 0;

    if (iid == CSLBassBoostItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_BASSBOOST;
    } else if (iid == CSLVirtualizerItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_VIRTUALIZER;
    } else if (iid == CSLEqualizerItf::sGetIID()) {
        opts_mask = OSLMP_CONTEXT_OPTION_USE_EQUALIZER;
    }

    if (opts_mask) {
        if (!(opts & opts_mask)) {
            return SL_RESULT_FEATURE_UNSUPPORTED;
        }
    }

    CSLObjectItf *objSinkPlayer = (this->sink_) ? (this->sink_->getPlayerObj()) : nullptr;

    if (!objSinkPlayer) {
        return SL_RESULT_RESOURCE_ERROR;
    }

    return objSinkPlayer->GetInterface(itf);
}

AudioDataPipeManager *AudioSystem::Impl::getPipeManager() const noexcept { return pipe_mgr_.get(); }

AudioMixer *AudioSystem::Impl::getMixer() const noexcept { return mixer_.get(); }

int AudioSystem::Impl::determineNextPollingTime() const noexcept
{
    int kDefaultPollingPeriodMs = 30;

    bool required = false;
    bool immediately = false;

    if (audio_player_instance_updated_) {
        required |= true;
        immediately |= true;
    }

    // check polling is required
    if (!required && mixer_) {
        required |= (mixer_->isPollingRequired());
        required |= (mixer_->getState() == AudioMixer::MIXER_STATE_STARTED);
    }

    if (!required) {
        for (const auto &player : audio_players_info_) {
            if (player.instance->isPollingRequired()) {
                required = true;
                break;
            }
        }
    }

    if (!required && capture_pipe_) {
        required = capture_pipe_->isCapturedDataAvailable();
    }

#if 0 // skip this (pipe manager always returns "true")
    if (!required) {
        required = (pipe_mgr_ && pipe_mgr_->isPollingRequired());
    }
#endif

    // determine next polling time
    int wait_time_ms;

    if (required && immediately) {
        return 0; // 0: immediately
    } else if (required) {
        timespec now;
        utils::timespec_utils::get_current_time(now);

        const int64_t diff_ms = utils::timespec_utils::sub_ret_ms(now, ts_prev_polling_);

        if (utils::timespec_utils::is_zero(ts_prev_polling_)) {
            wait_time_ms = 0; // 0: immediately
        } else if (diff_ms >= 0 && diff_ms <= kDefaultPollingPeriodMs) {
            wait_time_ms = kDefaultPollingPeriodMs - static_cast<int>(diff_ms);
        } else {
            wait_time_ms = 0; // 0: immediately
        }
    } else {
        LOGV("Enter idle state");
        wait_time_ms = -1; // -1: infinity
    }

    return wait_time_ms;
}

int AudioSystem::Impl::poll() noexcept
{
    DEBUG_PROFILE_POLLING_PROCESS(timespec ts_profile[7]);

    DEBUG_PROFILE_POLLING_GET_TIMESTAMP(ts_profile[0]);

    // save current time
    utils::timespec_utils::get_current_time(ts_prev_polling_);

    // === polling ===
    pollControlSinkMute();

    DEBUG_PROFILE_POLLING_GET_TIMESTAMP(ts_profile[1]);

    pollControlMixerAndSinkSuspendResume();

    DEBUG_PROFILE_POLLING_GET_TIMESTAMP(ts_profile[2]);

    for (auto &player : audio_players_info_) {
        if (player.instance->isPollingRequired()) {
            player.instance->poll();
        }
    }

    DEBUG_PROFILE_POLLING_GET_TIMESTAMP(ts_profile[3]);

    if (mixer_ && mixer_->isPollingRequired()) {
        mixer_->poll();
    }

    DEBUG_PROFILE_POLLING_GET_TIMESTAMP(ts_profile[4]);

    if (pipe_mgr_ && pipe_mgr_->isPollingRequired()) {
        pipe_mgr_->poll();
    }

    DEBUG_PROFILE_POLLING_GET_TIMESTAMP(ts_profile[5]);

    if (capture_pipe_ && mixer_ && (AudioMixer::MIXER_STATE_STARTED == mixer_->getState())) {
        pollObtainCapturedAudioData();
    }
    DEBUG_PROFILE_POLLING_GET_TIMESTAMP(ts_profile[6]);

// ===============

#ifdef DEBUG_PROFILE_POLLING_METHOD
    LOGV("PROF: {t1: %d, t2: %d, t3: %d, t4: %d, t5: %d, t6: %d}",
         static_cast<int>(utils::timespec_utils::sub_ret_us(ts_profile[1], ts_profile[0])),
         static_cast<int>(utils::timespec_utils::sub_ret_us(ts_profile[2], ts_profile[1])),
         static_cast<int>(utils::timespec_utils::sub_ret_us(ts_profile[3], ts_profile[2])),
         static_cast<int>(utils::timespec_utils::sub_ret_us(ts_profile[4], ts_profile[3])),
         static_cast<int>(utils::timespec_utils::sub_ret_us(ts_profile[5], ts_profile[4])),
         static_cast<int>(utils::timespec_utils::sub_ret_us(ts_profile[6], ts_profile[5])));
#endif

    return OSLMP_RESULT_SUCCESS;
}

void AudioSystem::Impl::pollControlSinkMute() noexcept
{
    audio_player_instance_updated_ = false;

// skip this method to wait until sink buffer is flushed
#if 0
    CSLVolumeItf volume;
    getInterfaceFromSinkPlayer(&volume);

    SLboolean cur_mute_state = SL_BOOLEAN_FALSE;
    volume.GetMute(&cur_mute_state);

    if (!cur_mute_state) {
        if (audio_players_info_.empty()) {
            volume.SetMute(SL_BOOLEAN_TRUE);

            // NOTE: This delay is required to avoid SLEqualizer's bug.
            // (Unexpected audible issue: press "Create" button during playing when equalizer is used)
            // However I decided not to sleep here, because 100 ms is too long!
            // I recommend reuse OpenSLMediaPlayerEqualizer instance instead of re-creating each time.
#if 0
            timespec ts_sleep({0, 100000000});   // 100 ms
            ::clock_nanosleep(CLOCK_MONOTONIC, 0, &ts_sleep, nullptr);
#endif
        }
    } else {
        bool found_active_player = false;

        for (const auto &p : audio_players_info_) {
            if (p.is_active) {
                found_active_player = true;
                break;
            }
        }

        if (found_active_player) {
            volume.SetMute(SL_BOOLEAN_FALSE);
        }
    }
#endif
}

void AudioSystem::Impl::pollControlMixerAndSinkSuspendResume() noexcept
{
    const int kMixerSuspendDelayMs = 250; // to wait all pooled data flushed

    if (!mixer_) {
        return;
    }

    const AudioMixer::state_t mixer_state = mixer_->getState();
    const bool mixer_can_suspend = mixer_->canSuspend();

    timespec now;
    utils::timespec_utils::get_current_time(now);

    if (mixer_can_suspend) {
        if (utils::timespec_utils::is_zero(ts_mixer_enter_can_suspend_)) {
            // save current time
            ts_mixer_enter_can_suspend_ = now;
        }
    } else {
        utils::timespec_utils::set_zero(ts_mixer_enter_can_suspend_);
    }

    switch (mixer_state) {
    case AudioMixer::MIXER_STATE_STARTED:
        if (mixer_can_suspend) {
            // enter suspended state after elapsed kMixerSuspendDelayMs delay
            if (!utils::timespec_utils::is_zero(ts_mixer_enter_can_suspend_) &&
                (utils::timespec_utils::sub_ret_ms(now, ts_mixer_enter_can_suspend_) >= kMixerSuspendDelayMs)) {

                // OnBeforeAudioSinkStateChanged(false)
                context_->raiseOnBeforeAudioSinkStateChanged(false);

                LOGD("Suspend mixer");

                (void)mixer_->suspend();

                LOGD("Stop sink");
                (void)sink_->stop();
            }
        }
        break;
    case AudioMixer::MIXER_STATE_SUSPENDED:
        if (!mixer_can_suspend) {
            int result;

            // OnBeforeAudioSinkStateChanged(true)
            context_->raiseOnBeforeAudioSinkStateChanged(true);

            LOGD("Resume mixer");
            result = mixer_->resume();

            if (result == OSLMP_RESULT_SUCCESS) {
                LOGD("Start sink");
                result = sink_->start();
            }

            if (result != OSLMP_RESULT_SUCCESS) {
                (void)mixer_->stop();
            }
        }
        break;
    default:
        break;
    }
}

void AudioSystem::Impl::pollObtainCapturedAudioData() noexcept
{
    const int kNumMaxProcessPerPoll = 5;

    for (int i = 0; i < kNumMaxProcessPerPoll; ++i) {
        AudioCaptureDataPipe::read_block_t rb;

        if (!capture_pipe_->lockRead(rb, 0)) {
            return;
        }

        // raise event
        if (audio_capture_event_listener_) {
            audio_capture_event_listener_->onCaptureAudioData(rb.src, rb.num_channels, rb.num_frames,
                                                              init_args_.system_out_sampling_rate, &(rb.timestamp));
        }

        capture_pipe_->unlockRead(rb);
    }
}

int AudioSystem::Impl::registerAudioPlayer(AudioPlayer *player, uint32_t &id) noexcept
{
    id = 0;

    if (!player)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    auto &players = audio_players_info_;

    if (std::find_if(players.begin(), players.end(), MATCH_PLAYER(player, id)) != players.end()) {
        // already registered
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    // generate new id
    uint32_t new_id = audio_player_player_id_counter_ + 1;
    if (new_id == 0) {
        new_id += 1;
    }

    try { players.push_back(audio_player_info_t(new_id, player)); }
    catch (const std::bad_alloc &) { return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED; }

    // update field
    audio_player_player_id_counter_ = new_id;
    audio_player_instance_updated_ = true;

    // set result
    id = new_id;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::unregisterAudioPlayer(AudioPlayer *player, uint32_t id) noexcept
{
    if (!player)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    auto &players = audio_players_info_;

    // remove player
    auto find_result = std::remove_if(players.begin(), players.end(), MATCH_PLAYER(player, id));

    if (find_result == players.end()) {
        // not registered
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    players.erase(find_result, players.end());

    // update fields
    audio_player_instance_updated_ = true;

    return OSLMP_RESULT_SUCCESS;
}

bool AudioSystem::Impl::checkAudioPlayerIsAlive(AudioPlayer *player, uint32_t id) const noexcept
{
    const auto &players = audio_players_info_;
    return (std::find_if(players.begin(), players.end(), MATCH_PLAYER(player, id)) != players.end());
}

int AudioSystem::Impl::notifyAudioPlayerState(AudioPlayer *player, uint32_t id, bool is_active) noexcept
{
    auto &players = audio_players_info_;

    auto matched = std::find_if(players.begin(), players.end(), MATCH_PLAYER(player, id));

    if (matched == players.end()) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    if (matched->is_active != is_active) {
        matched->is_active = is_active;
        audio_player_instance_updated_ = true;
    }

    return OSLMP_RESULT_SUCCESS;
}

void AudioSystem::Impl::setAudioCaptureEventListener(AudioCaptureEventListener *listener) noexcept
{
    audio_capture_event_listener_ = listener;
}

void AudioSystem::Impl::setAudioCaptureEnabled(bool enabled) noexcept { mixer_->setAudioCaptureEnabled(enabled); }

int AudioSystem::Impl::selectActiveAuxEffect(int aux_effect_id) noexcept
{
    bool updated = false;

    switch (aux_effect_id) {
    case OSLMP_AUX_EFFECT_NULL:
    case OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB:
    case OSLMP_AUX_EFFECT_PRESET_REVERB:
        if (aux_.active_effect_id != aux_effect_id) {
            aux_.active_effect_id = aux_effect_id;
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

int AudioSystem::Impl::setAuxEffectSendLevel(float level) noexcept
{
    if (std::isnan(level) || (level < 0.0f) || (level > 1.0f)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (aux_.send_level == level) {
        return OSLMP_RESULT_SUCCESS;
    }

    aux_.send_level = level;
    aux_.send_level_millibel = linearToMillibel(aux_.send_level);

    return applyActiveAuxEffectSettings();
}

int AudioSystem::Impl::setAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept
{
    bool updated = false;

    switch (aux_effect_id) {
    case OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB:
        if (aux_.environmental_reverb_enabled != enabled) {
            aux_.environmental_reverb_enabled = enabled;
            updated = true;
        }
        break;
    case OSLMP_AUX_EFFECT_PRESET_REVERB:
        if (aux_.preset_reverb_enabled != enabled) {
            aux_.preset_reverb_enabled = enabled;
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

int AudioSystem::Impl::setAudioStreamType(int streamtype) noexcept
{
    // NOTE:
    // stream type cannot be changed after initialized
    if (streamtype == init_args_.stream_type) {
        return OSLMP_RESULT_SUCCESS;
    } else {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
}

int AudioSystem::Impl::getAudioStreamType(int *streamtype) const noexcept
{
    if (!streamtype)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*streamtype) = init_args_.stream_type;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::getSystemOutputSamplingRate(uint32_t *sampling_rate) const noexcept
{
    if (!sampling_rate)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*sampling_rate) = init_args_.system_out_sampling_rate;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::getOutputLatencyInFrames(uint32_t *latency) const noexcept
{
    if (!latency)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*latency) = 0;

    if (!sink_) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    (*latency) = sink_->getLatencyInFrames();

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::getParamResamplerQualityLevel(uint32_t *quality_level) const noexcept
{
    if (!quality_level)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*quality_level) = init_args_.resampler_quality;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::getPreAmp(PreAmp **p_preamp) const noexcept
{
    if (!p_preamp)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!preamp_)
        return OSLMP_RESULT_ILLEGAL_STATE;

    (*p_preamp) = preamp_.get();

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::getHQEqualizer(HQEqualizer **p_hq_equalizer) const noexcept
{
    if (!p_hq_equalizer)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!mixout_effect_hq_equalizer_)
        return OSLMP_RESULT_ILLEGAL_STATE;

    (*p_hq_equalizer) = mixout_effect_hq_equalizer_.get();

    return OSLMP_RESULT_SUCCESS;
}

int AudioSystem::Impl::applyActiveAuxEffectSettings() noexcept
{
    CSLEffectSendItf effect_send;
    CSLEnvironmentalReverbItf env_reverb;
    CSLPresetReverbItf preset_reverb;

    (void)getInterfaceFromSinkPlayer(&effect_send);
    (void)getInterfaceFromOutputMixer(&env_reverb);
    (void)getInterfaceFromOutputMixer(&preset_reverb);

    SLresult slResult = SL_RESULT_UNKNOWN_ERROR;

    switch (aux_.active_effect_id) {
    case OSLMP_AUX_EFFECT_NULL:
        (void)applyAuxEffectSettings(effect_send, env_reverb.self(), SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        (void)applyAuxEffectSettings(effect_send, preset_reverb.self(), SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        slResult = SL_RESULT_SUCCESS;
        break;
    case OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB:
        (void)applyAuxEffectSettings(effect_send, preset_reverb.self(), SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        slResult =
            applyAuxEffectSettings(effect_send, env_reverb.self(), CSLboolean::toSL(aux_.environmental_reverb_enabled),
                                   aux_.send_level_millibel);
        break;
    case OSLMP_AUX_EFFECT_PRESET_REVERB:
        (void)applyAuxEffectSettings(effect_send, env_reverb.self(), SL_BOOLEAN_FALSE, SL_MILLIBEL_MIN);
        slResult = applyAuxEffectSettings(effect_send, preset_reverb.self(),
                                          CSLboolean::toSL(aux_.preset_reverb_enabled), aux_.send_level_millibel);
        break;
    default:
        slResult = SL_RESULT_INTERNAL_ERROR;
        break;
    }

    return InternalUtils::sTranslateOpenSLErrorCode(slResult);
}

SLresult AudioSystem::Impl::applyAuxEffectSettings(CSLEffectSendItf &effect_send, const void *aux_effect,
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

bool AudioSystem::Impl::check_is_low_latency(const initialize_args_t &args) noexcept
{
    // NOTE: Normal mixer (not FastMixer) is used if these flags are enabled
    const uint32_t normal_mixer_used_condition_mask =
        OSLMP_CONTEXT_OPTION_USE_BASSBOOST | OSLMP_CONTEXT_OPTION_USE_VIRTUALIZER | OSLMP_CONTEXT_OPTION_USE_EQUALIZER |
        OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB | OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB;

    if (args.system_supports_low_latency) {
        const uint32_t options = args.context->getContextOptions();
        return ((options & normal_mixer_used_condition_mask) == 0);
    } else {
        return false;
    }
}

uint32_t AudioSystem::Impl::determine_output_frame_size(const initialize_args_t &args, bool is_low_latency) noexcept
{
    // if (is_low_latency) {
    //     return args.system_out_frames_per_buffer;
    // } else {
        return calc_android_NormalMixer_FrameCount(args.system_out_frames_per_buffer,
                                                   args.system_out_sampling_rate / 1000);
    // }
}

// This function refers to Android framework's implementation
// (av/services/audiofliner/Threads.cpp)
uint32_t AudioSystem::Impl::calc_android_NormalMixer_FrameCount(uint32_t frame_count, uint32_t sample_rate_hz) noexcept
{
    static const uint32_t kMinNormalMixBufferSizeMs = 20;
    static const uint32_t kMaxNormalMixBufferSizeMs = 24;

    uint32_t minNormalFrameCount = (kMinNormalMixBufferSizeMs * sample_rate_hz) / 1000;
    uint32_t maxNormalFrameCount = (kMaxNormalMixBufferSizeMs * sample_rate_hz) / 1000;

    minNormalFrameCount = round_up_16(minNormalFrameCount);
    maxNormalFrameCount = round_down_16(maxNormalFrameCount);
    maxNormalFrameCount = (std::max)(maxNormalFrameCount, minNormalFrameCount);

    double multiplier = static_cast<double>(minNormalFrameCount) / frame_count;
    if (multiplier <= 1.0) {
        multiplier = 1.0;
    } else if (multiplier <= 2.0) {
        if (2 * frame_count <= maxNormalFrameCount) {
            multiplier = 2.0;
        } else {
            multiplier = static_cast<double>(maxNormalFrameCount) / frame_count;
        }
    } else {
        uint32_t truncMult = static_cast<uint32_t>(multiplier);
        if ((truncMult & 1)) {
            if ((truncMult + 1) * frame_count <= maxNormalFrameCount) {
                ++truncMult;
            }
        }
        multiplier = static_cast<double>(truncMult);
    }

    const uint32_t normalFrameCount = round_up_16(multiplier * frame_count);

    return normalFrameCount;
}

} // namespace impl
} // namespace oslmp
