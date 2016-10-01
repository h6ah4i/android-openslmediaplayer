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

// #define LOG_TAG "AudioDataAdapter"

#include "oslmp/impl/AudioDataAdapter.hpp"

#include <new>
#include <algorithm>

#include <cxxporthelper/compiler.hpp>
#include <cxxporthelper/memory>
#include <cxxporthelper/aligned_memory.hpp>

#include <cxxdasp/fft/fft.hpp>
#include <cxxdasp/resampler/smart/smart_resampler.hpp>
#include <cxxdasp/resampler/smart/smart_resampler_params_factory.hpp>
#include <cxxdasp/converter/sample_format_converter.hpp>
#include <cxxdasp/converter/sample_format_converter_core_operators.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/utils/timespec_utils.hpp"

namespace oslmp {
namespace impl {

class AudioDataAdapter::Impl {
public:
    Impl();
    ~Impl();

    bool init(const AudioDataAdapter::initialize_args_t &args) noexcept;

    bool is_output_data_ready() const noexcept;

    bool put_input_data(const int16_t *data, uint32_t num_channels, uint32_t num_frames) noexcept;
    bool get_output_data(float *data, uint32_t num_channels, uint32_t num_frames) noexcept;

    bool flush() noexcept;

private:
    typedef cxxdasp::datatype::audio_frame<int16_t, 1> s16_mono_frame_t;
    typedef cxxdasp::datatype::audio_frame<int16_t, 2> s16_stereo_frame_t;
    typedef cxxdasp::datatype::f32_stereo_frame_t f32_stereo_frame_t;

#if ((CXXPH_TARGET_ARCH == CXXPH_ARCH_ARM) || (CXXPH_TARGET_ARCH == CXXPH_ARCH_ARM64)) &&                              \
    CXXPH_COMPILER_SUPPORTS_ARM_NEON
    // use NEON optimized implementation
    typedef cxxdasp::resampler::f32_stereo_neon_halfband_x2_resampler_core_operator f32_stereo_halfband_core_operator_t;
    typedef cxxdasp::resampler::f32_stereo_neon_polyphase_core_operator polyphase_core_operator_t;
    typedef cxxdasp::converter::s16_mono_to_f32_stereo_neon_fast_sample_format_converter_core_operator
    s16_mono_to_f32_stereo_sample_format_converter_core_operator_t;
    typedef cxxdasp::converter::s16_to_f32_stereo_neon_fast_sample_format_converter_core_operator
    s16_to_f32_stereo_sample_format_converter_core_operator_t;
#elif(CXXPH_TARGET_ARCH == CXXPH_ARCH_I386) || (CXXPH_TARGET_ARCH == CXXPH_ARCH_X86_64)
    // use SSE optimized implementation
    typedef cxxdasp::resampler::f32_stereo_sse_halfband_x2_resampler_core_operator f32_stereo_halfband_core_operator_t;
    typedef cxxdasp::resampler::f32_stereo_sse_polyphase_core_operator polyphase_core_operator_t;
    typedef cxxdasp::converter::s16_mono_to_f32_stereo_sse_sample_format_converter_core_operator
    s16_mono_to_f32_stereo_sample_format_converter_core_operator_t;
    typedef cxxdasp::converter::s16_to_f32_stereo_sse_sample_format_converter_core_operator
    s16_to_f32_stereo_sample_format_converter_core_operator_t;
#else
    // use general implementation
    typedef cxxdasp::resampler::general_polyphase_core_operator<float, float, float, 2> polyphase_core_operator_t;
    typedef cxxdasp::resampler::f32_stereo_basic_halfband_x2_resampler_core_operator
    f32_stereo_halfband_core_operator_t;
    typedef cxxdasp::converter::general_sample_format_converter_core_operator<s16_mono_frame_t, f32_stereo_frame_t>
    s16_mono_to_f32_stereo_sample_format_converter_core_operator_t;
    typedef cxxdasp::converter::general_sample_format_converter_core_operator<s16_stereo_frame_t, f32_stereo_frame_t>
    s16_to_f32_stereo_sample_format_converter_core_operator_t;
#endif

#if CXXDASP_USE_FFT_BACKEND_NE10
    typedef cxxdasp::fft::backend::f::ne10 fft_backend_t;
#elif CXXDASP_USE_FFT_BACKEND_PFFFT
    typedef cxxdasp::fft::backend::f::pffft fft_backend_t;
#else
#error No FFT backend available
#endif

    void fill_output_buffer() noexcept;

    typedef cxxdasp::resampler::smart_resampler<f32_stereo_frame_t, f32_stereo_frame_t,
                                                f32_stereo_halfband_core_operator_t, fft_backend_t,
                                                polyphase_core_operator_t> f32_stereo_smart_resampler;

    typedef cxxdasp::converter::sample_format_converter<s16_mono_frame_t, f32_stereo_frame_t,
                                                        s16_mono_to_f32_stereo_sample_format_converter_core_operator_t>
    s16_mono_to_f32_stereo_sample_format_converter;

    typedef cxxdasp::converter::sample_format_converter<s16_stereo_frame_t, f32_stereo_frame_t,
                                                        s16_to_f32_stereo_sample_format_converter_core_operator_t>
    s16_to_f32_stereo_sample_format_converter;

    AudioDataAdapter::initialize_args_t init_args_;
    size_t pooled_input_data_count_;
    size_t pooled_output_data_count_;

    bool flushed_;
    bool resampler_flushed_;

    std::unique_ptr<f32_stereo_smart_resampler> resampler_;

    s16_mono_to_f32_stereo_sample_format_converter mono2stereo_converter_;
    s16_to_f32_stereo_sample_format_converter stereo_converter_;

    cxxporthelper::aligned_memory<f32_stereo_frame_t> f32_stereo_input_buffer_;
    cxxporthelper::aligned_memory<f32_stereo_frame_t> f32_stereo_output_buffer_;
};

//
// AudioDataAdapter
//

AudioDataAdapter::AudioDataAdapter() : impl_(new (std::nothrow) Impl()) {}

AudioDataAdapter::~AudioDataAdapter() {}

bool AudioDataAdapter::init(const AudioDataAdapter::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->init(args);
}

bool AudioDataAdapter::is_output_data_ready() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->is_output_data_ready();
}

bool AudioDataAdapter::put_input_data(const int16_t *data, uint32_t num_channels, uint32_t num_frames) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->put_input_data(data, num_channels, num_frames);
}

bool AudioDataAdapter::get_output_data(float *data, uint32_t num_channels, uint32_t num_frames) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->get_output_data(data, num_channels, num_frames);
}

bool AudioDataAdapter::flush() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->flush();
}

//
// AudioDataAdapter::Impl
//
AudioDataAdapter::Impl::Impl()
    : pooled_input_data_count_(0), pooled_output_data_count_(0), flushed_(false), resampler_flushed_(false),
      mono2stereo_converter_(), stereo_converter_()
{
}

AudioDataAdapter::Impl::~Impl() {}

bool AudioDataAdapter::Impl::init(const AudioDataAdapter::initialize_args_t &args) noexcept
{
    if (!(args.in_num_channels == 1 || args.in_num_channels == 2)) {
        return false;
    }

    if (!(args.in_sampling_rate == 8000000 ||
          args.in_sampling_rate == 11025000 || args.in_sampling_rate == 12000000 ||
          args.in_sampling_rate == 16000000 ||
          args.in_sampling_rate == 22050000 || args.in_sampling_rate == 24000000 ||
          args.in_sampling_rate == 32000000 ||
          args.in_sampling_rate == 44100000 || args.in_sampling_rate == 48000000 ||
          args.in_sampling_rate == 88200000 || args.in_sampling_rate == 96000000 ||
          args.in_sampling_rate == 176400000 || args.in_sampling_rate == 192000000)) {
        return false;
    }

    if (args.in_block_size == 0) {
        return false;
    }

    if (!(args.out_num_channels == 1 || args.out_num_channels == 2)) {
        return false;
    }

    if (!(args.out_sampling_rate == 44100000 || args.out_sampling_rate == 48000000)) {
        return false;
    }

    if (args.out_block_size == 0) {
        return false;
    }

    cxxdasp::resampler::smart_resampler_params_factory::quality_spec_t smart_resampler_qspec;
    switch (args.resampler_quality_spec) {
    case RESAMPLER_QUALITY_LOW:
        smart_resampler_qspec =
            cxxdasp::resampler::smart_resampler_params_factory::LowQuality; // S/N = 70 dB, heavy aliasing
        break;
    case RESAMPLER_QUALITY_MIDDLE:
        smart_resampler_qspec =
            cxxdasp::resampler::smart_resampler_params_factory::MidQuality; // slight aliasing, 16-bit quality
        break;
    case RESAMPLER_QUALITY_HIGH:
        smart_resampler_qspec =
            cxxdasp::resampler::smart_resampler_params_factory::HighQuality; // no aliasing, 24-bit quality
        break;
    default:
        return false;
    }

    LOGD("Resampler quality: %d", smart_resampler_qspec);

    cxxdasp::resampler::smart_resampler_params_factory pf((args.in_sampling_rate / 1000),
                                                          (args.out_sampling_rate / 1000), smart_resampler_qspec);

    if (!pf) {
        return false;
    }

    std::unique_ptr<f32_stereo_smart_resampler> resampler(new (std::nothrow) f32_stereo_smart_resampler(pf.params()));

    if (!resampler) {
        return false;
    }

    cxxporthelper::aligned_memory<f32_stereo_frame_t> f32_stereo_input_buffer(args.in_block_size);
    cxxporthelper::aligned_memory<f32_stereo_frame_t> f32_stereo_output_buffer(args.out_block_size);

    if (!(f32_stereo_input_buffer && f32_stereo_output_buffer)) {
        return false;
    }

    // update fields
    init_args_ = args;
    flushed_ = false;
    resampler_flushed_ = false;
    resampler_ = std::move(resampler);
    pooled_input_data_count_ = 0;
    pooled_output_data_count_ = 0;
    f32_stereo_input_buffer_ = std::move(f32_stereo_input_buffer);
    f32_stereo_output_buffer_ = std::move(f32_stereo_output_buffer);

    return true;
}

bool AudioDataAdapter::Impl::is_output_data_ready() const noexcept
{
    if (!resampler_flushed_) {
        return (pooled_output_data_count_ == init_args_.out_block_size);
    } else {
        return (pooled_output_data_count_ > 0);
    }
}

bool AudioDataAdapter::Impl::put_input_data(const int16_t *data, uint32_t num_channels, uint32_t num_frames) noexcept
{
    if (CXXPH_UNLIKELY(!data)) {
        return false;
    }

    if (CXXPH_UNLIKELY(!(num_channels == init_args_.in_num_channels && num_frames == init_args_.in_block_size))) {
        return false;
    }

    if (CXXPH_UNLIKELY(flushed_)) {
        return false;
    }

    if (CXXPH_UNLIKELY(pooled_input_data_count_ != 0)) {
        return false;
    }

    // S16-{Monaural|Stereo} -> F32-Stereo
    switch (init_args_.in_num_channels) {
    case 1:
        mono2stereo_converter_.perform(reinterpret_cast<const s16_mono_frame_t *>(data), &f32_stereo_input_buffer_[0],
                                       init_args_.in_block_size);
        break;
    case 2:
        stereo_converter_.perform(reinterpret_cast<const s16_stereo_frame_t *>(data), &f32_stereo_input_buffer_[0],
                                  init_args_.in_block_size);
        break;
    default:
        return false;
    }

    pooled_input_data_count_ = init_args_.in_block_size;

    fill_output_buffer();

    return true;
}

bool AudioDataAdapter::Impl::get_output_data(float *data, uint32_t num_channels, uint32_t num_frames) noexcept
{
    if (CXXPH_UNLIKELY(!data)) {
        return false;
    }

    if (CXXPH_UNLIKELY(!(num_channels == init_args_.out_num_channels && num_frames == init_args_.out_block_size))) {
        return false;
    }

    if (CXXPH_UNLIKELY(!is_output_data_ready())) {
        return false;
    }

    // copy resampled data
    {
        const size_t size_in_bytes = pooled_output_data_count_ * 2 * sizeof(float);
        ::memcpy(&data[0], &f32_stereo_output_buffer_[0], size_in_bytes);
    }

    // fill zero rest of the buffer
    if (CXXPH_UNLIKELY(pooled_output_data_count_ < init_args_.out_block_size)) {
        const size_t offset = pooled_output_data_count_ * 2;
        const size_t size_in_bytes = (init_args_.out_block_size - pooled_output_data_count_) * 2 * sizeof(float);
        ::memset(&data[offset], 0, size_in_bytes);
    }

    pooled_output_data_count_ = 0;

    fill_output_buffer();

    return true;
}

bool AudioDataAdapter::Impl::flush() noexcept
{
    if (CXXPH_UNLIKELY(flushed_)) {
        return true;
    }

    flushed_ = true;

    fill_output_buffer();

    return true;
}

void AudioDataAdapter::Impl::fill_output_buffer() noexcept
{

    while (true) {
        const int n_can_put = (std::min)(resampler_->num_can_put(), static_cast<int>(pooled_input_data_count_));

        if (CXXPH_LIKELY(n_can_put > 0)) {
            const size_t offset = init_args_.in_block_size - pooled_input_data_count_;
            resampler_->put_n(&f32_stereo_input_buffer_[offset], n_can_put);
            pooled_input_data_count_ -= n_can_put;
        }

        const int n_can_get = (std::min)(resampler_->num_can_get(),
                                         static_cast<int>(init_args_.out_block_size - pooled_output_data_count_));

        if (CXXPH_LIKELY(n_can_get > 0)) {
            const size_t offset = pooled_output_data_count_;
            resampler_->get_n(&f32_stereo_output_buffer_[offset], n_can_get);
            pooled_output_data_count_ += n_can_get;
        }

        if (CXXPH_UNLIKELY(n_can_put == 0 && n_can_get == 0)) {
            // flush resampler
            if (CXXPH_UNLIKELY(flushed_ && !resampler_flushed_)) {
                resampler_flushed_ = true;
                resampler_->flush();
                continue;
            }

            break;
        }
    }
}

} // namespace oslmp
} // namespace impl
