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

#ifndef AUDIOSYSTEM_HPP_
#define AUDIOSYSTEM_HPP_

#include <time.h>

#include <cxxporthelper/memory>
#include <cxxporthelper/cstdint>

#include <SLES/OpenSLES.h>

//
// forward declarations
//
namespace opensles {
class CSLInterface;
} // namespace opensles

namespace oslmp {
namespace impl {
class OpenSLMediaPlayerInternalContext;
class AudioDataPipeManager;
class AudioMixer;
class AudioSource;
class AudioPlayer;
class PreAmp;
class HQEqualizer;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

class AudioSystem {
public:
    struct initialize_args_t {
        OpenSLMediaPlayerInternalContext *context;
        uint32_t system_out_sampling_rate;     // [millihertz]
        uint32_t system_out_frames_per_buffer; // [frames]
        bool system_supports_low_latency;
        bool system_supports_floating_point;
        int stream_type;
        uint32_t short_fade_duration_ms;
        uint32_t long_fade_duration_ms;
        uint32_t resampler_quality;
        uint32_t hq_equalizer_impl_type;

        initialize_args_t()
            : context(nullptr), system_out_sampling_rate(0), system_out_frames_per_buffer(0),
              system_supports_low_latency(false), system_supports_floating_point(false), stream_type(0),
              short_fade_duration_ms(0), long_fade_duration_ms(0), resampler_quality(0), hq_equalizer_impl_type(0)
        {
        }
    };

    class AudioCaptureEventListener {
    public:
        virtual ~AudioCaptureEventListener() {}

        virtual void onCaptureAudioData(const float *data, uint32_t num_channels, size_t num_frames,
                                        uint32_t sample_rate_millihertz, const timespec *timestamp) noexcept = 0;
    };

    AudioSystem();
    ~AudioSystem();

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
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIOSYSTEM_HPP_
