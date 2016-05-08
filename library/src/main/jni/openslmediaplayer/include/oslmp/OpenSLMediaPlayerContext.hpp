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

#ifndef OPENSLMEDIAPLAYERCONTEXT_HPP_
#define OPENSLMEDIAPLAYERCONTEXT_HPP_

#include <jni.h> // for JNIEnv

#include <oslmp/OpenSLMediaPlayerAPICommon.hpp>

// constants
#define OSLMP_CONTEXT_OPTION_USE_BASSBOOST (1 << 0)   // NOTE: low-latency playback is disabled if specified this option
#define OSLMP_CONTEXT_OPTION_USE_VIRTUALIZER (1 << 1) // NOTE: low-latency playback is disabled if specified this option
#define OSLMP_CONTEXT_OPTION_USE_EQUALIZER (1 << 2)   // NOTE: low-latency playback is disabled if specified this option
#define OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB                                                                   \
    (1 << 8) // NOTE: low-latency playback is disabled if specified this option
#define OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB                                                                         \
    (1 << 9) // NOTE: low-latency playback is disabled if specified this option
#define OSLMP_CONTEXT_OPTION_USE_VISUALIZER (1 << 16)
#define OSLMP_CONTEXT_OPTION_USE_HQ_EQUALIZER (1 << 17)
#define OSLMP_CONTEXT_OPTION_USE_PREAMP (1 << 18)
#define OSLMP_CONTEXT_OPTION_USE_HQ_VISUALIZER (1 << 19)

// resampler quality specifier
#define OSLMP_CONTEXT_RESAMPLER_QUALITY_LOW 0
#define OSLMP_CONTEXT_RESAMPLER_QUALITY_MIDDLE 1
#define OSLMP_CONTEXT_RESAMPLER_QUALITY_HIGH 2

// HQEqualizer implementation type specifier
#define OSLMP_CONTEXT_HQ_EAUALIZER_IMPL_BASIC_PEAKING_FILTER 0
#define OSLMP_CONTEXT_HQ_EAUALIZER_IMPL_FLAT_GAIN_RESPOINSE 1

// Sink backend implementation type specifier
#define OSLMP_CONTEXT_SINK_BACKEND_TYPE_OPENSL      0
#define OSLMP_CONTEXT_SINK_BACKEND_TYPE_AUDIO_TRACK 1

//
// forward declarations
//
namespace oslmp {
namespace impl {
class OpenSLMediaPlayerInternalContext;
} // namespace impl
} // namespace oslmp

namespace oslmp {

class OpenSLMediaPlayerContext : public virtual android::RefBase {
    friend class impl::OpenSLMediaPlayerInternalContext;

public:
    class InternalThreadEventListener;

    struct create_args_t {
        uint32_t system_out_sampling_rate;     // [millihertz]
        uint32_t system_out_frames_per_buffer; // [frames]
        bool system_supports_low_latency;
        bool system_supports_floating_point;
        uint32_t options;
        int stream_type;
        uint32_t short_fade_duration; // Fade duration used to avoid audio artifacts (unit: [ms])
        uint32_t long_fade_duration;  // Fade duration used when calling start(), pause() and seek() (unit: [ms])
        uint32_t resampler_quality;
        uint32_t hq_equalizer_impl_type;
        uint32_t sink_backend_type;
        bool use_low_latency_if_available;
        bool use_floating_point_if_available;
        InternalThreadEventListener *listener;

        create_args_t() OSLMP_API_ABI : system_out_sampling_rate(44100000),
                                        system_out_frames_per_buffer(512),
                                        system_supports_low_latency(false),
                                        system_supports_floating_point(false),
                                        options(0),
                                        stream_type(3), // 3 = STREAM_MUSIC
                                        short_fade_duration(25),
                                        long_fade_duration(1500),
                                        listener(nullptr),
                                        resampler_quality(OSLMP_CONTEXT_RESAMPLER_QUALITY_MIDDLE),
                                        hq_equalizer_impl_type(OSLMP_CONTEXT_HQ_EAUALIZER_IMPL_BASIC_PEAKING_FILTER),
                                        sink_backend_type(OSLMP_CONTEXT_SINK_BACKEND_TYPE_OPENSL),
                                        use_low_latency_if_available(false),
                                        use_floating_point_if_available(false)
        {
        }
    };

public:
    virtual ~OpenSLMediaPlayerContext() OSLMP_API_ABI;

    static android::sp<OpenSLMediaPlayerContext> create(JNIEnv *env, const create_args_t &args) noexcept OSLMP_API_ABI;

    InternalThreadEventListener *getInternalThreadEventListener() const noexcept OSLMP_API_ABI;

    int32_t getAudioSessionId() const noexcept OSLMP_API_ABI;

private:
    class Impl;
    OpenSLMediaPlayerContext(Impl *impl);

    impl::OpenSLMediaPlayerInternalContext &getInternal() const noexcept OSLMP_API_ABI;

private:
    Impl *impl_; // NOTE: do not use unique_ptr to avoid cxxporthelper dependencies
};

class OpenSLMediaPlayerContext::InternalThreadEventListener : public virtual android::RefBase {
public:
    virtual ~InternalThreadEventListener() {}
    virtual void onEnterInternalThread(OpenSLMediaPlayerContext *context) noexcept OSLMP_API_ABI = 0;
    virtual void onLeaveInternalThread(OpenSLMediaPlayerContext *context) noexcept OSLMP_API_ABI = 0;
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYERCONTEXT_HPP_
