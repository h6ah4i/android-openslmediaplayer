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

// #define LOG_TAG "AudioMixer"

#include "oslmp/impl/MixingUnit.hpp"

#include <limits>

#include <cxxporthelper/memory>
#include <cxxporthelper/cmath>
#include <cxxporthelper/cstdint>
#include <cxxporthelper/compiler.hpp>
#include <cxxporthelper/aligned_memory.hpp>

#include <cxxdasp/utils/utils.hpp>
#include <cxxdasp/utils/fast_sincos_generator.hpp>
#include <cxxdasp/converter/sample_format_converter.hpp>
#include <cxxdasp/converter/sample_format_converter_core_operators.hpp>
#include <cxxdasp/mixer/mixer.hpp>
#include <cxxdasp/mixer/mixer_core_operators.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/MixedOutputAudioEffect.hpp"

namespace oslmp {
namespace impl {

class FadeTable {
public:
    FadeTable() : table_(), size_() {}
    ~FadeTable() {}

    bool initialize(uint32_t size) noexcept;

    const float *table() const noexcept { return table_.get(); }

    uint32_t size() const noexcept { return size_; }

    explicit operator bool() const noexcept { return static_cast<bool>(table_); }

    const float &operator[](int i) const { return table_[i]; }

private:
    static void makeFadeTable(float *table, uint32_t n) noexcept;

private:
    std::unique_ptr<float[]> table_;
    uint32_t size_;
};

class MixingUnit::Impl {
public:
    Impl();
    ~Impl();

    bool initialize(const initialize_args_t &args) noexcept;

    uint32_t numChannels() const noexcept;
    uint32_t blockSizeInFrames() const noexcept;

    bool begin(void *CXXPH_RESTRICT dest, sample_format_type dest_sample_format,
               capture_data_type *CXXPH_RESTRICT capture, uint32_t size_in_frames,
               MixedOutputAudioEffect *mixout_effects[], uint32_t num_mixout_effects) noexcept;
    bool end() noexcept;

    bool mix(Context *context, const in_data_type *src, uint32_t size_in_frames) noexcept;

private:
    typedef cxxdasp::datatype::s16_stereo_frame_t s16_stereo_frame_t;
    typedef cxxdasp::datatype::f32_stereo_frame_t f32_stereo_frame_t;

#if ((CXXPH_TARGET_ARCH == CXXPH_ARCH_ARM) || (CXXPH_TARGET_ARCH == CXXPH_ARCH_ARM64)) &&                              \
    CXXPH_COMPILER_SUPPORTS_ARM_NEON
    // use NEON optimized implementation
    typedef cxxdasp::converter::f32_to_s16_stereo_neon_fast_sample_format_converter_core_operator
    f32_to_s16_stereo_sample_format_converter_core_operator_t;
    typedef cxxdasp::mixer::f32_stereo_neon_mixer_core_operator f32_stereo_mixer_core_operator_t;
#elif(CXXPH_TARGET_ARCH == CXXPH_ARCH_I386) || (CXXPH_TARGET_ARCH == CXXPH_ARCH_X86_64)
    // use SSE optimized implementation
    typedef cxxdasp::converter::f32_to_s16_stereo_sse_sample_format_converter_core_operator
    f32_to_s16_stereo_sample_format_converter_core_operator_t;
    typedef cxxdasp::mixer::f32_stereo_sse_mixer_core_operator f32_stereo_mixer_core_operator_t;
#else
    // use general implementation
    typedef cxxdasp::converter::general_sample_format_converter_core_operator<f32_stereo_frame_t, s16_stereo_frame_t>
    f32_to_s16_stereo_sample_format_converter_core_operator_t;
    typedef cxxdasp::mixer::general_mixer_core_operator<f32_stereo_frame_t, float> f32_stereo_mixer_core_operator_t;
#endif

    bool prepared() const noexcept;

    bool mixMute(Context *context, f32_stereo_frame_t *CXXPH_RESTRICT mix_buff,
                 const f32_stereo_frame_t *CXXPH_RESTRICT src, uint32_t size_in_frames, bool first_mix) const noexcept;
    bool mixAdd(Context *context, f32_stereo_frame_t *CXXPH_RESTRICT mix_buff,
                const f32_stereo_frame_t *CXXPH_RESTRICT src, uint32_t size_in_frames, bool first_mix) const noexcept;
    bool mixFadeIn(Context *context, const FadeTable &fade_table, f32_stereo_frame_t *CXXPH_RESTRICT mix_buff,
                   const f32_stereo_frame_t *CXXPH_RESTRICT src, uint32_t size_in_frames, bool first_mix) const
        noexcept;
    bool mixFadeOut(Context *context, const FadeTable &fade_table, f32_stereo_frame_t *CXXPH_RESTRICT mix_buff,
                    const f32_stereo_frame_t *CXXPH_RESTRICT src, uint32_t size_in_frames, bool first_mix) const
        noexcept;

    static f32_stereo_frame_t getStereoVolume(const Context *context) noexcept
    {
        f32_stereo_frame_t vol;
        vol.c(0) = context->volume[0];
        vol.c(1) = context->volume[1];
        return vol;
    }

    static bool isMuted(const Context *context) noexcept
    {
        return (context->volume[0] == 0.0f) && (context->volume[1] == 0.0f);
    }

private:
    initialize_args_t init_args_;
    cxxporthelper::aligned_memory<f32_stereo_frame_t> internal_mix_buff_;

    FadeTable short_fade_table_;
    FadeTable long_fade_table_;

    sample_format_type dest_sample_format_;
    void *dest_buff_;
    f32_stereo_frame_t *capture_buff_;
    f32_stereo_frame_t *mix_buff_;
    MixedOutputAudioEffect **mixout_effects_;
    uint32_t num_mixout_effects_;
    int processed_count_;

    cxxdasp::converter::sample_format_converter<f32_stereo_frame_t, s16_stereo_frame_t,
                                                f32_to_s16_stereo_sample_format_converter_core_operator_t>
    f32_to_s16_stereo_converter_;
    cxxdasp::mixer::mixer<f32_stereo_frame_t, float, f32_stereo_mixer_core_operator_t> f32_stereo_mixer_;
};

static inline void setPhase(MixingUnit::Context *context, float phase)
{
    context->phase = (std::min)((std::max)(phase, 0.0f), 1.0f);
}

//
// MixingUnit
//

MixingUnit::MixingUnit() : impl_(new (std::nothrow) Impl()) {}

MixingUnit::~MixingUnit() {}

bool MixingUnit::initialize(const initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0;
    return impl_->initialize(args);
}

uint32_t MixingUnit::blockSizeInFrames() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0;
    return impl_->blockSizeInFrames();
}

bool MixingUnit::begin(void *CXXPH_RESTRICT dest, sample_format_type dest_sample_format,
                       capture_data_type *CXXPH_RESTRICT capture, uint32_t size_in_frames,
                       MixedOutputAudioEffect *mixout_effects[], uint32_t num_mixout_effects) noexcept
{

    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->begin(dest, dest_sample_format, capture, size_in_frames, mixout_effects, num_mixout_effects);
}

bool MixingUnit::end() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->end();
}

bool MixingUnit::mix(MixingUnit::Context *context, const in_data_type *src, uint32_t size_in_frames) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->mix(context, src, size_in_frames);
}

//
// MixingUnit::Impl
//

MixingUnit::Impl::Impl()
    : init_args_(), internal_mix_buff_(), short_fade_table_(), long_fade_table_(),
      dest_sample_format_(kAudioSampleFormatType_Unknown), dest_buff_(nullptr), capture_buff_(nullptr),
      mix_buff_(nullptr), mixout_effects_(nullptr), num_mixout_effects_(0), processed_count_(0),
      f32_to_s16_stereo_converter_(), f32_stereo_mixer_()
{
}

MixingUnit::Impl::~Impl() {}

bool MixingUnit::Impl::initialize(const MixingUnit::initialize_args_t &args) noexcept
{
    if (args.sampling_rate == 0) {
        return false;
    }

    if (args.block_size_in_frames == 0) {
        return false;
    }

    cxxporthelper::aligned_memory<f32_stereo_frame_t> mix_buff(args.block_size_in_frames);

    if (!(mix_buff)) {
        return false;
    }

    const uint32_t sampling_rate_in_hz = (args.sampling_rate / 1000);
    const uint32_t short_fade_duration_in_frames = (sampling_rate_in_hz * args.short_fade_duration_ms) / 1000;
    const uint32_t long_fade_duration_in_frames = (sampling_rate_in_hz * args.long_fade_duration_ms) / 1000;

    if (!short_fade_table_.initialize(short_fade_duration_in_frames)) {
        return false;
    }

    if (!long_fade_table_.initialize(long_fade_duration_in_frames)) {
        return false;
    }

    internal_mix_buff_ = std::move(mix_buff);
    init_args_ = args;

    return true;
}

uint32_t MixingUnit::Impl::numChannels() const noexcept { return 2; }

uint32_t MixingUnit::Impl::blockSizeInFrames() const noexcept { return init_args_.block_size_in_frames; }

bool MixingUnit::Impl::begin(void *CXXPH_RESTRICT dest, sample_format_type dest_sample_format,
                             capture_data_type *CXXPH_RESTRICT capture, uint32_t size_in_frames,
                             MixedOutputAudioEffect *mixout_effects[], uint32_t num_mixout_effects) noexcept
{

    // check parameters
    if (CXXPH_UNLIKELY(!(dest && size_in_frames == blockSizeInFrames()))) {
        return false;
    }

    if (CXXPH_UNLIKELY(
            !(dest_sample_format == kAudioSampleFormatType_S16 || dest_sample_format == kAudioSampleFormatType_F32))) {
        return false;
    }

    if (!CXXPH_UNLIKELY(cxxdasp::utils::is_aligned(dest, 4))) {
        return false;
    }

    if (capture) {
        if (!CXXPH_UNLIKELY(cxxdasp::utils::is_aligned(capture, 8))) {
            return false;
        }
    }

    if (CXXPH_UNLIKELY(!(internal_mix_buff_))) {
        return false;
    }

    dest_sample_format_ = dest_sample_format;
    dest_buff_ = dest;
    capture_buff_ = reinterpret_cast<f32_stereo_frame_t *>(capture);

    if (dest_sample_format == kAudioSampleFormatType_F32) {
        mix_buff_ = static_cast<f32_stereo_frame_t *>(dest);
    } else {
        mix_buff_ = (capture_buff_) ? capture_buff_ : &internal_mix_buff_[0];
    }

    mixout_effects_ = mixout_effects;
    num_mixout_effects_ = num_mixout_effects;

    processed_count_ = -1;

    return true;
}

bool MixingUnit::Impl::end() noexcept
{
    if (CXXPH_UNLIKELY(!prepared())) {
        return false;
    }

    // clear mixing buffer if no audio block have been processed
    if (!(processed_count_ > 0)) {
        ::memset(mix_buff_, 0, sizeof(f32_stereo_frame_t) * blockSizeInFrames());
    }

    // apply filters
    if (num_mixout_effects_ > 0 && mixout_effects_) {
        for (int i = 0; i < num_mixout_effects_; ++i) {
            (mixout_effects_[i])->process(reinterpret_cast<float *>(&mix_buff_[0]), numChannels(), blockSizeInFrames());
        }
    }

    // copy to destination buffer
    if (dest_buff_ != mix_buff_) {
        if (dest_sample_format_ == kAudioSampleFormatType_S16) {
            f32_to_s16_stereo_converter_.perform(&mix_buff_[0], static_cast<s16_stereo_frame_t *>(dest_buff_),
                                                 blockSizeInFrames());
        } else {
            assert(false);
        }
    }

    // copy to capture buffer
    if (capture_buff_ && (capture_buff_ != mix_buff_)) {
        ::memcpy(capture_buff_, mix_buff_, sizeof(f32_stereo_frame_t) * blockSizeInFrames());
    }

    // clear fields
    dest_sample_format_ = kAudioSampleFormatType_Unknown;
    dest_buff_ = nullptr;
    capture_buff_ = nullptr;
    mix_buff_ = nullptr;
    mixout_effects_ = nullptr;
    num_mixout_effects_ = 0;
    processed_count_ = 0;

    return true;
}

bool MixingUnit::Impl::prepared() const noexcept { return (dest_buff_ && mix_buff_); }

bool MixingUnit::Impl::mix(MixingUnit::Context *context, const in_data_type *src, uint32_t size_in_frames) noexcept
{

    // check parameters
    if (CXXPH_UNLIKELY(!(context && src && size_in_frames == blockSizeInFrames()))) {
        return false;
    }

    if (CXXPH_UNLIKELY(!(context->phase >= 0.0f && context->phase <= 1.0f))) {
        return false;
    }

    if (CXXPH_UNLIKELY((reinterpret_cast<uintptr_t>(src) % (4 * 2)) != 0)) {
        return false;
    }

    if (CXXPH_UNLIKELY(!prepared())) {
        return false;
    }

    f32_stereo_frame_t *mix_buff = &mix_buff_[0];
    const f32_stereo_frame_t *f32_stereo_src = reinterpret_cast<const f32_stereo_frame_t *>(src);

    bool first_mix = false;
    if (processed_count_ < 0) {
        processed_count_ = 0;
        first_mix = true;
    }

    bool processed = false;

    mode_t mix_mode = context->mode;
    if (isMuted(context)) {
        mix_mode = MODE_MUTE;
    }

    switch (mix_mode) {
    case MODE_MUTE:
        processed = mixMute(context, mix_buff, f32_stereo_src, size_in_frames, first_mix);
        break;
    case MODE_ADD:
        processed = mixAdd(context, mix_buff, f32_stereo_src, size_in_frames, first_mix);
        break;
    case MODE_SHORT_FADE_IN:
        processed = mixFadeIn(context, short_fade_table_, mix_buff, f32_stereo_src, size_in_frames, first_mix);
        break;
    case MODE_SHORT_FADE_OUT:
        processed = mixFadeOut(context, short_fade_table_, mix_buff, f32_stereo_src, size_in_frames, first_mix);
        break;
    case MODE_LONG_FADE_IN:
        processed = mixFadeIn(context, long_fade_table_, mix_buff, f32_stereo_src, size_in_frames, first_mix);
        break;
    case MODE_LONG_FADE_OUT:
        processed = mixFadeOut(context, long_fade_table_, mix_buff, f32_stereo_src, size_in_frames, first_mix);
        break;
    default:
        processed = false;
        break;
    }

    if (processed) {
        processed_count_++;
    }

    return true;
}

bool MixingUnit::Impl::mixMute(MixingUnit::Context *context, f32_stereo_frame_t *CXXPH_RESTRICT mix_buff,
                               const f32_stereo_frame_t *CXXPH_RESTRICT src, uint32_t size_in_frames,
                               bool first_mix) const noexcept
{

    // nothing to do

    return false;
}

bool MixingUnit::Impl::mixAdd(MixingUnit::Context *context, f32_stereo_frame_t *CXXPH_RESTRICT mix_buff,
                              const f32_stereo_frame_t *CXXPH_RESTRICT src, uint32_t size_in_frames,
                              bool first_mix) const noexcept
{

    f32_stereo_mixer_.mul_scale(&mix_buff[0], &src[0], getStereoVolume(context), size_in_frames, !first_mix);

    return true;
}

bool MixingUnit::Impl::mixFadeIn(MixingUnit::Context *context, const FadeTable &fade_table,
                                 f32_stereo_frame_t *CXXPH_RESTRICT mix_buff,
                                 const f32_stereo_frame_t *CXXPH_RESTRICT src, uint32_t size_in_frames,
                                 bool first_mix) const noexcept
{

    const f32_stereo_frame_t volume = getStereoVolume(context);
    const uint32_t fade_table_size = fade_table.size();
    const uint32_t iphase = static_cast<uint32_t>(context->phase * fade_table_size);
    const uint32_t fade_remains = (std::min)((fade_table_size - iphase), size_in_frames);

    if (fade_remains > 0) {
        const float *CXXPH_RESTRICT table = &(fade_table[iphase]);

        f32_stereo_mixer_.mul_forward_table_and_scale(&mix_buff[0], &src[0], &table[0], volume, fade_remains,
                                                      !first_mix);
    }

    if (fade_remains < size_in_frames) {
        f32_stereo_mixer_.mul_scale(&mix_buff[fade_remains], &src[fade_remains], volume,
                                    (size_in_frames - fade_remains), !first_mix);
    }

    if (fade_table_size > 0) {
        setPhase(context, context->phase + (static_cast<float>(fade_remains) / fade_table_size));
    }

    return true;
}

bool MixingUnit::Impl::mixFadeOut(MixingUnit::Context *context, const FadeTable &fade_table,
                                  f32_stereo_frame_t *CXXPH_RESTRICT mix_buff,
                                  const f32_stereo_frame_t *CXXPH_RESTRICT src, uint32_t size_in_frames,
                                  bool first_mix) const noexcept
{

    const f32_stereo_frame_t volume = getStereoVolume(context);
    const uint32_t fade_table_size = fade_table.size();
    const uint32_t iphase = static_cast<uint32_t>(context->phase * fade_table_size);
    const uint32_t fade_remains = (std::min)((fade_table_size - iphase), size_in_frames);

    if (fade_remains > 0) {
        const float *CXXPH_RESTRICT table = &(fade_table[fade_table_size - iphase - 1]);

        f32_stereo_mixer_.mul_backward_table_and_scale(&mix_buff[0], &src[0], &table[0], volume, fade_remains,
                                                       !first_mix);
    }

    if (fade_remains < size_in_frames) {
        if (first_mix) {
            ::memset(&mix_buff[fade_remains], 0, sizeof(f32_stereo_frame_t) * (size_in_frames - fade_remains));
        }
    }

    if (fade_table_size > 0) {
        setPhase(context, context->phase + (static_cast<float>(fade_remains) / fade_table_size));
    }

    return (fade_remains > 0);
}

//
// FadeTable
//
bool FadeTable::initialize(uint32_t size) noexcept
{
    std::unique_ptr<float[]> table;

    if (size > 0) {
        table.reset(new (std::nothrow) float[size]);

        if (!table) {
            return false;
        }

        makeFadeTable(&table[0], size);
    }

    table_ = std::move(table);
    size_ = size;

    return true;
}

void FadeTable::makeFadeTable(float *table, uint32_t n) noexcept
{
    const double phase_step = (n > 1) ? (M_PI / (n - 1)) : 0.0;
    cxxdasp::utils::fast_sincos_generator<float> gen(0.0, phase_step);

    for (size_t i = 0; i < n; ++i) {
        table[i] = 0.5f - 0.5f * gen.c();
        gen.update();
    }
}

} // namespace impl
} // namespace oslmp
