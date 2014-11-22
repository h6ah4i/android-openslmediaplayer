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

// #define LOG_TAG "HQEqualizer"
#include "oslmp/impl/HQEqualizer.hpp"

#include <cassert>
#include <algorithm>

#include <lockfree/lockfree_circulation_buffer.hpp>

#include <cxxdasp/datatype/audio_frame.hpp>
#include <cxxdasp/filter/biquad/biquad_filter.hpp>
#include <cxxdasp/filter/biquad/biquad_filter_core_operators.hpp>
#include <cxxdasp/filter/cascaded_biquad/cascaded_biquad_filter.hpp>
#include <cxxdasp/filter/cascaded_biquad/cascaded_biquad_filter_core_operators.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/OpenSLMediaPlayerResultCodes.hpp"
#include "oslmp/impl/HQEqualizerBandCalculator.hpp"
#include "oslmp/utils/bitmap_looper.hpp"

#define REQUEST_QUEUE_SIZE 3

#define BAND_NO_MASK(band_no) (static_cast<uint32_t>(1U << (band_no)))
#define ALL_BANDS_NO_MASK (static_cast<uint32_t>((1U << HQEqualizer::NUM_BANDS) - 1U))
#define LOWEST_BAND_NO_MASK BAND_NO_MASK(0)
#define HIGHEST_BAND_NO_MASK BAND_NO_MASK(HQEqualizer::NUM_BANDS - 1)
#define MID_BANDS_NO_MASK (ALL_BANDS_NO_MASK & ~(LOWEST_BAND_NO_MASK | HIGHEST_BAND_NO_MASK))

#define BAND_LEVEL_UPDATED_BITS ALL_BANDS_NO_MASK
#define ENABLED_UPDATED_BIT (1U << 31U)

#define MAX_BAND_LEVEL_MILLIBEL (static_cast<int16_t>(15 * 100))  // 15 dB
#define MIN_BAND_LEVEL_MILLIBEL (static_cast<int16_t>(-15 * 100)) // -15 dB

#define MAX_GAIN_STEP (0.125)         // [dB]
#define BLOCK_SIZE_WHILE_UPDATING 256 // [samples]

// for PeakingFilterEqualizerProcessor
#define PFEQ_NEIGHBOR_BAND_CORRELATION_COEFF (-0.15)
#define PFEQ_PEAKING_FILTER_BAND_WIDTH (1.0)

// for FlatGainResponseEqualizerProcessor
#define FGREQ_LOWEST_BAND_SINGLE_FILTER_INDEX (0)
#define FGREQ_HIGHEST_BAND_SINGLE_FILTER_INDEX (1)
#define FGREQ_MID_BAND_CASCADED_FILTER_INDEX(band_no) ((band_no) - 1)

#define FGREQ_LOWEST_BAND_CALC_INDEX (0)
#define FGREQ_HIGHEST_BAND_CALC_INDEX (1 + 2 * (HQEqualizer::NUM_BANDS - 2))
#define FGREQ_MID_BAND_CALC_INDEX_1(band_no) (1 + 2 * ((band_no) - 1))
#define FGREQ_MID_BAND_CALC_INDEX_2(band_no) (1 + 2 * ((band_no) - 1) + 1)

namespace oslmp {
namespace impl {

struct HQEqualizerRequest {
    bool enabled;
    int16_t band_level[HQEqualizer::NUM_BANDS];
    uint32_t updated_bitmap;

    void reset()
    {
        enabled = false;
        for (auto &l : band_level) {
            l = 0;
        }
        updated_bitmap = 0;
    }
};

struct HQEqualizerBandInfo {
    uint32_t min_freq;
    uint32_t center_freq;
    uint32_t max_freq;
};

typedef lockfree::lockfree_circulation_buffer<HQEqualizerRequest *, (REQUEST_QUEUE_SIZE + 1)> hqeq_request_queue_t;

typedef cxxdasp::datatype::f32_stereo_frame_t f32_stereo_frame_t;
#if ((CXXPH_TARGET_ARCH == CXXPH_ARCH_ARM) || (CXXPH_TARGET_ARCH == CXXPH_ARCH_ARM64)) &&                              \
    CXXPH_COMPILER_SUPPORTS_ARM_NEON
// use NEON optimized implementation
typedef cxxdasp::filter::f32_stereo_neon_biquad_transposed_direct_form_2_core_operator
f32_stereo_single_biquad_core_operator_t;
typedef cxxdasp::filter::f32_stereo_neon_cascaded_2_biquad_transposed_direct_form_2_core_operator
f32_stereo_cascaded_2_biquad_core_operator_t;
#elif(CXXPH_TARGET_ARCH == CXXPH_ARCH_I386) || (CXXPH_TARGET_ARCH == CXXPH_ARCH_X86_64)
// use SSE optimized implementation (not available for now...)
typedef cxxdasp::filter::general_biquad_transposed_direct_form_2_core_operator<f32_stereo_frame_t>
f32_stereo_single_biquad_core_operator_t;
typedef cxxdasp::filter::general_cascaded_biquad_core_operator<f32_stereo_single_biquad_core_operator_t, 2>
f32_stereo_cascaded_2_biquad_core_operator_t;
#else
// use general implementation
typedef cxxdasp::filter::general_biquad_transposed_direct_form_2_core_operator<f32_stereo_frame_t>
f32_stereo_single_biquad_core_operator_t;
typedef cxxdasp::filter::general_cascaded_biquad_core_operator<f32_stereo_single_biquad_core_operator_t, 2>
f32_stereo_cascaded_2_biquad_core_operator_t;
#endif
typedef cxxdasp::filter::biquad_filter<f32_stereo_frame_t, f32_stereo_single_biquad_core_operator_t>
single_band_filter_t;
typedef cxxdasp::filter::cascaded_biquad_filter<f32_stereo_frame_t, f32_stereo_cascaded_2_biquad_core_operator_t>
cascaded_band_filter_t;

class BaseEqualizerProcessor {
public:
    enum { NUM_BANDS = HQEqualizer::NUM_BANDS, };

    BaseEqualizerProcessor(const HQEqualizerBandInfo *band_info, uint32_t sampling_rate,
                           hqeq_request_queue_t &request_queue, hqeq_request_queue_t &used_queue,
                           HQEqualizerRequest &request);
    virtual ~BaseEqualizerProcessor() {}

    void onAttachedToMixerThread() noexcept;
    void onDetachedFromMixerThread() noexcept;
    int pollFromMixerThread() noexcept;
    int process(f32_stereo_frame_t *data, uint32_t num_frames) noexcept;

protected:
    virtual void onSetup() noexcept {}
    virtual uint32_t onProcessBands(uint32_t bands_mask, f32_stereo_frame_t *data, uint32_t num_frames) noexcept
    {
        return 0;
    }
    virtual uint32_t onUpdateBandsLevel(uint32_t bands_mask) noexcept { return 0; }

    const HQEqualizerBandInfo &getBandInfo(int band_no) const noexcept { return band_info_[band_no]; }

    uint32_t getSamplingRate() const noexcept { return sampling_rate_; }

    double getCurrentBandLevel(int band_no) const noexcept { return cur_band_gain_db_[band_no]; }

    uint32_t getPendingSamplesCount(int band_no) const noexcept { return num_pending_samples_[band_no]; }

    void setPendingSamplesCount(int band_no, uint32_t value) noexcept { num_pending_samples_[band_no] = value; }

private:
    HQEqualizerBandInfo const *const band_info_;
    const uint32_t sampling_rate_;
    hqeq_request_queue_t &request_queue_;
    hqeq_request_queue_t &used_queue_;
    HQEqualizerRequest current_;
    double req_band_gain_db_[NUM_BANDS];
    double cur_band_gain_db_[NUM_BANDS];
    uint32_t num_pending_samples_[NUM_BANDS];
    uint32_t bitmap_updating_bands_;

    // NOTE: these fields are only accessible in onAttachedToMixerThread()/onDetachedFromMixerThread()
    HQEqualizerRequest &request_;
};

// Basic graphic equalizer using peaking filter
//
// "[Normal quality] Frequency response (Peak only)"
// https://docs.google.com/spreadsheets/d/1hj2aoW83rGraANzHxKaCpECFQ0WawVbap4tgxZ9FSmo/edit#gid=596598004
class PeakingFilterEqualizerProcessor : public BaseEqualizerProcessor {
public:
    PeakingFilterEqualizerProcessor(const HQEqualizerBandInfo *band_info, uint32_t sampling_rate,
                                    hqeq_request_queue_t &request_queue, hqeq_request_queue_t &used_queue,
                                    HQEqualizerRequest &request);
    virtual ~PeakingFilterEqualizerProcessor();

protected:
    virtual void onSetup() noexcept override;
    virtual uint32_t onProcessBands(uint32_t bands_mask, f32_stereo_frame_t *data,
                                    uint32_t num_frames) noexcept override;
    virtual uint32_t onUpdateBandsLevel(uint32_t bands_mask) noexcept override;

private:
    bool processBandSet(int band_set_no, f32_stereo_frame_t *data, uint32_t num_frames) noexcept;

    double getCorrectedBandLevel(int band_no) const noexcept;

private:
    static_assert((NUM_BANDS % 2 == 0), "NUM_BANDS have to be multiples of two");

    cascaded_band_filter_t cascaded_band_filters_[NUM_BANDS / 2];
    HQEqualizerBandCalculator band_calcs_[NUM_BANDS];
};

// Flat gain response, but requires about x2 processor resources
//
// "[High quality] Frequency response (Shelf)"
// https://docs.google.com/spreadsheets/d/1hj2aoW83rGraANzHxKaCpECFQ0WawVbap4tgxZ9FSmo/edit#gid=596598004
class FlatGainResponseEqualizerProcessor : public BaseEqualizerProcessor {
public:
    FlatGainResponseEqualizerProcessor(const HQEqualizerBandInfo *band_info, uint32_t sampling_rate,
                                       hqeq_request_queue_t &request_queue, hqeq_request_queue_t &used_queue,
                                       HQEqualizerRequest &request);
    virtual ~FlatGainResponseEqualizerProcessor();

protected:
    virtual void onSetup() noexcept override;
    virtual uint32_t onProcessBands(uint32_t bands_mask, f32_stereo_frame_t *data,
                                    uint32_t num_frames) noexcept override;
    virtual uint32_t onUpdateBandsLevel(uint32_t bands_mask) noexcept override;

private:
    bool processEdgeBand(int band_no, f32_stereo_frame_t *data, uint32_t num_frames) noexcept;
    bool processMidBand(int band_no, f32_stereo_frame_t *data, uint32_t num_frames) noexcept;

private:
    single_band_filter_t single_band_filters_[2]; // 0: lowest band, 1: highest band
    cascaded_band_filter_t cascaded_band_filters_[NUM_BANDS - 2];
    HQEqualizerBandCalculator band_calcs_[2 + 2 * (NUM_BANDS - 2)];
    uint32_t num_pending_samples_[NUM_BANDS];
};

class HQEqualizer::Impl {
public:
    Impl();
    ~Impl();

    bool initialize(const initialize_args_t &args) noexcept;

    int setEnabled(bool enabled) noexcept;
    int getEnabled(bool *enabled) const noexcept;
    int getBand(uint32_t frequency, uint16_t *band) const noexcept;
    int setBandLevel(uint16_t band, int16_t level) noexcept;
    int getBandLevel(uint16_t band, int16_t *level) const noexcept;
    int getBandFreqRange(uint16_t band, uint32_t *min_freq, uint32_t *max_freq) const noexcept;
    int getBandLevelRange(int16_t *min_level, int16_t *max_level) const noexcept;
    int getCenterFreq(uint16_t band, uint32_t *center_freq) const noexcept;
    int getNumberOfBands(uint16_t *num_bands) const noexcept;
    int setAllBandLevel(const int16_t *level, uint16_t num_bands) noexcept;
    int getAllBandLevel(int16_t *level, uint16_t num_bands) const noexcept;

    bool isPollingRequired() const noexcept;
    int poll() noexcept;

    void onAttachedToMixerThread() noexcept;
    void onDetachedFromMixerThread() noexcept;
    int pollFromMixerThread() noexcept;
    int process(float *data, uint32_t num_channels, uint32_t num_frames) noexcept;

private:
    void recycleRequestItems() noexcept;
    void apply() noexcept;

    initialize_args_t init_args_;

    hqeq_request_queue_t free_queue_;
    hqeq_request_queue_t request_queue_;
    hqeq_request_queue_t used_queue_;

    HQEqualizerRequest request_item_pool_[REQUEST_QUEUE_SIZE];

    HQEqualizerRequest request_;

    // these fields are modified from mixer thread context
    std::unique_ptr<BaseEqualizerProcessor> processor_;

    // constant fields
    static const HQEqualizerBandInfo band_info_[NUM_BANDS];
};

// Band parameters calculation sheet:
// https://docs.google.com/spreadsheets/d/1hj2aoW83rGraANzHxKaCpECFQ0WawVbap4tgxZ9FSmo/edit?usp=sharing
#define BAND_SLOPE (2)
#define HALF_BAND_WIDTH (1.4142135623730951)     // = sqrt(2)
#define HALF_BAND_WIDTH_INV (0.7071067811865475) // = 1 / sqrt(2)

const HQEqualizerBandInfo HQEqualizer::Impl::band_info_[HQEqualizer::NUM_BANDS] = { { 22097, 31250, 44193 },
                                                                                    { 44194, 62500, 88387 },
                                                                                    { 88388, 125000, 176775 },
                                                                                    { 176776, 250000, 353552 },
                                                                                    { 353553, 500000, 707105 },
                                                                                    { 707106, 1000000, 1414212 },
                                                                                    { 1414213, 2000000, 2828426 },
                                                                                    { 2828427, 4000000, 5656853 },
                                                                                    { 5656854, 8000000, 11313707 },
                                                                                    { 11313708, 16000000, 22627415 }, };

//
// Utilities
//
static bool move_and_update_queue_item(hqeq_request_queue_t &src_queue, hqeq_request_queue_t &dest_queue,
                                       const HQEqualizerRequest *set_value, HQEqualizerRequest *get_value)
{
    hqeq_request_queue_t::index_t src_lock_index = hqeq_request_queue_t::INVALID_INDEX;
    hqeq_request_queue_t::index_t req_lock_index = hqeq_request_queue_t::INVALID_INDEX;

    if (!src_queue.lock_read(src_lock_index)) {
        return false;
    }

    if (dest_queue.lock_write(req_lock_index)) {
        HQEqualizerRequest *item = src_queue.at(src_lock_index);

        assert(item);

        if (get_value) {
            (*get_value) = (*item);
        }

        if (set_value) {
            (*item) = (*set_value);
        }

        src_queue.at(src_lock_index) = nullptr;
        dest_queue.at(req_lock_index) = item;

        dest_queue.unlock_write(req_lock_index);
    } else {
        assert(false);
    }

    src_queue.unlock_read(src_lock_index);
    return true;
}

static inline double millibel_to_decibel(int16_t mb) noexcept { return 0.01 * mb; }

static inline bool is_valid_band_no(uint16_t band) noexcept { return (band < HQEqualizer::NUM_BANDS); }

//
// HQEqualizer
//
HQEqualizer::HQEqualizer() : MixedOutputAudioEffect(), impl_(new (std::nothrow) Impl()) {}

HQEqualizer::~HQEqualizer() {}

bool HQEqualizer::initialize(const initialize_args_t &args) noexcept
{
    if (!impl_)
        return false;
    return impl_->initialize(args);
}

int HQEqualizer::setEnabled(bool enabled) noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setEnabled(enabled);
}

int HQEqualizer::getEnabled(bool *enabled) const noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getEnabled(enabled);
}

int HQEqualizer::getBand(uint32_t frequency, uint16_t *band) const noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getBand(frequency, band);
}

int HQEqualizer::setBandLevel(uint16_t band, int16_t level) noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setBandLevel(band, level);
}

int HQEqualizer::getBandLevel(uint16_t band, int16_t *level) const noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getBandLevel(band, level);
}

int HQEqualizer::getBandFreqRange(uint16_t band, uint32_t *min_freq, uint32_t *max_freq) const noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getBandFreqRange(band, min_freq, max_freq);
}

int HQEqualizer::getBandLevelRange(int16_t *min_level, int16_t *max_level) const noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getBandLevelRange(min_level, max_level);
}

int HQEqualizer::getCenterFreq(uint16_t band, uint32_t *center_freq) const noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getCenterFreq(band, center_freq);
}

int HQEqualizer::getNumberOfBands(uint16_t *num_bands) const noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getNumberOfBands(num_bands);
}

int HQEqualizer::setAllBandLevel(const int16_t *level, uint16_t num_bands) noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setAllBandLevel(level, num_bands);
}

int HQEqualizer::getAllBandLevel(int16_t *level, uint16_t num_bands) const noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getAllBandLevel(level, num_bands);
}

bool HQEqualizer::isPollingRequired() const noexcept
{
    if (!impl_)
        return false;
    return impl_->isPollingRequired();
}

int HQEqualizer::poll() noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->poll();
}

void HQEqualizer::onAttachedToMixerThread() noexcept
{
    if (!impl_)
        return;
    impl_->onAttachedToMixerThread();
}

void HQEqualizer::onDetachedFromMixerThread() noexcept
{
    if (!impl_)
        return;
    ;
    impl_->onDetachedFromMixerThread();
}

int HQEqualizer::pollFromMixerThread() noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->pollFromMixerThread();
}

int HQEqualizer::process(float *data, uint32_t num_channels, uint32_t num_frames) noexcept
{
    if (!impl_)
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->process(data, num_channels, num_frames);
}

//
// HQEqualizer::Impl
//

HQEqualizer::Impl::Impl() : init_args_() {}

HQEqualizer::Impl::~Impl() {}

bool HQEqualizer::Impl::initialize(const initialize_args_t &args) noexcept
{
    // initialize request queues
    free_queue_.clear();
    used_queue_.clear();
    request_queue_.clear();

    for (int i = 0; i < REQUEST_QUEUE_SIZE; ++i) {
        hqeq_request_queue_t::index_t lock_index = hqeq_request_queue_t::INVALID_INDEX;

        if (free_queue_.lock_write(lock_index)) {
            free_queue_.at(lock_index) = &(request_item_pool_[i]);
            free_queue_.unlock_write(lock_index);
        } else {
            assert(false);
            return false;
        }
    }

    std::unique_ptr<BaseEqualizerProcessor> processor;

    switch (args.impl_type) {
    case kImplBasicPeakingFilter:
        LOGD("Impl.: kImplBasicPeakingFilter");
        processor.reset(new (std::nothrow) PeakingFilterEqualizerProcessor(band_info_, args.sampling_rate,
                                                                           request_queue_, used_queue_, request_));
        break;
    case kImplFlatGain:
        LOGD("Impl.: kImplFlatGain");
        processor.reset(new (std::nothrow) FlatGainResponseEqualizerProcessor(band_info_, args.sampling_rate,
                                                                              request_queue_, used_queue_, request_));
        break;
    default:
        break;
    }

    if (!processor) {
        return false;
    }

    // update fields
    request_.reset();
    init_args_ = args;
    processor_ = std::move(processor);

    return true;
}

int HQEqualizer::Impl::setEnabled(bool enabled) noexcept
{
    if (enabled == request_.enabled) {
        return OSLMP_RESULT_SUCCESS;
    }

    request_.enabled = enabled;
    request_.updated_bitmap |= ENABLED_UPDATED_BIT;

    apply();

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::getEnabled(bool *enabled) const noexcept
{
    if (!enabled) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    (*enabled) = request_.enabled;

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::getBand(uint32_t frequency, uint16_t *band) const noexcept
{
    if (!band) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    uint16_t matched = UNDEFINED;

    for (int i = 0; i < NUM_BANDS; ++i) {
        if (frequency >= band_info_[i].min_freq && frequency <= band_info_[i].max_freq) {
            matched = static_cast<uint16_t>(i);
            break;
        }
    }

    (*band) = matched;

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::setBandLevel(uint16_t band, int16_t level) noexcept
{
    if (!is_valid_band_no(band)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    level = (std::max)(level, MIN_BAND_LEVEL_MILLIBEL);
    level = (std::min)(level, MAX_BAND_LEVEL_MILLIBEL);

    if (request_.band_level[band] == level) {
        return OSLMP_RESULT_SUCCESS;
    }

    request_.band_level[band] = level;
    request_.updated_bitmap |= BAND_NO_MASK(band);

    apply();

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::getBandLevel(uint16_t band, int16_t *level) const noexcept
{
    if (!is_valid_band_no(band)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
    if (!level) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    (*level) = request_.band_level[band];

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::getBandFreqRange(uint16_t band, uint32_t *min_freq, uint32_t *max_freq) const noexcept
{
    if (!is_valid_band_no(band)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (!min_freq) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (!max_freq) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    (*min_freq) = band_info_[band].min_freq;
    (*max_freq) = band_info_[band].max_freq;

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::getBandLevelRange(int16_t *min_level, int16_t *max_level) const noexcept
{
    if (!min_level) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (!max_level) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    (*min_level) = MIN_BAND_LEVEL_MILLIBEL;
    (*max_level) = MAX_BAND_LEVEL_MILLIBEL;

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::getCenterFreq(uint16_t band, uint32_t *center_freq) const noexcept
{
    if (!is_valid_band_no(band)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (!center_freq) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    (*center_freq) = band_info_[band].center_freq;

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::getNumberOfBands(uint16_t *num_bands) const noexcept
{
    if (!num_bands) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    (*num_bands) = NUM_BANDS;

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::setAllBandLevel(const int16_t *level, uint16_t num_bands) noexcept
{
    if (!level) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
    if (num_bands != NUM_BANDS) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    for (int i = 0; i < NUM_BANDS; ++i) {
        if (request_.band_level[i] == level[i]) {
            continue;
        }

        request_.band_level[i] = level[i];
        request_.updated_bitmap |= BAND_NO_MASK(i);
    }

    apply();

    return OSLMP_RESULT_SUCCESS;
}

int HQEqualizer::Impl::getAllBandLevel(int16_t *level, uint16_t num_bands) const noexcept
{
    if (!level) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
    if (num_bands != NUM_BANDS) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    for (int i = 0; i < NUM_BANDS; ++i) {
        level[i] = request_.band_level[i];
    }

    return OSLMP_RESULT_SUCCESS;
}

bool HQEqualizer::Impl::isPollingRequired() const noexcept
{
    return ((request_.updated_bitmap != 0) || !free_queue_.full());
}

int HQEqualizer::Impl::poll() noexcept
{
    recycleRequestItems();
    apply();

    return OSLMP_RESULT_SUCCESS;
}

void HQEqualizer::Impl::recycleRequestItems() noexcept
{
    while (move_and_update_queue_item(used_queue_, free_queue_, nullptr, nullptr))
        ;
}

void HQEqualizer::Impl::apply() noexcept
{
    // free queue -> request queue
    if (!(request_.updated_bitmap)) {
        return;
    }

    if (move_and_update_queue_item(free_queue_, request_queue_, &request_, nullptr)) {
        request_.updated_bitmap = 0;
    }
}

void HQEqualizer::Impl::onAttachedToMixerThread() noexcept
{
    // NOTE: It's safe to access class fields from this function
    // (message thread is paused and memory barrier is properly issued)
    processor_->onAttachedToMixerThread();
}

void HQEqualizer::Impl::onDetachedFromMixerThread() noexcept
{
    // NOTE: It's safe to access class fields from this function
    // (message thread is paused and memory barrier is properly issued)
    processor_->onDetachedFromMixerThread();
}

int HQEqualizer::Impl::pollFromMixerThread() noexcept { return processor_->pollFromMixerThread(); }

int HQEqualizer::Impl::process(float *data, uint32_t num_channels, uint32_t num_frames) noexcept
{
    if (!data) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
    if (num_channels != init_args_.num_channels) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
    if (num_frames != init_args_.block_size_in_frames) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    return processor_->process(reinterpret_cast<f32_stereo_frame_t *>(data), num_frames);
}

//
// BaseEqualizerProcessor
//
BaseEqualizerProcessor::BaseEqualizerProcessor(const HQEqualizerBandInfo *band_info, uint32_t sampling_rate,
                                               hqeq_request_queue_t &request_queue, hqeq_request_queue_t &used_queue,
                                               HQEqualizerRequest &request)
    : band_info_(band_info), sampling_rate_(sampling_rate), request_queue_(request_queue), used_queue_(used_queue),
      request_(request), bitmap_updating_bands_(0)
{
}

void BaseEqualizerProcessor::onAttachedToMixerThread() noexcept
{
    current_ = request_;
    request_.updated_bitmap = 0;

    bitmap_updating_bands_ = 0;

    for (auto &t : num_pending_samples_) {
        t = 0;
    }

    // calculate band gain
    for (int i = 0; i < NUM_BANDS; ++i) {
        const double db_level = millibel_to_decibel(current_.band_level[i]);
        req_band_gain_db_[i] = db_level;
        cur_band_gain_db_[i] = db_level;
    }

    onSetup();
}

void BaseEqualizerProcessor::onDetachedFromMixerThread() noexcept {}

int BaseEqualizerProcessor::pollFromMixerThread() noexcept
{
    uint32_t updated_mask = 0;

    while (move_and_update_queue_item(request_queue_, used_queue_, nullptr, &current_)) {
        updated_mask |= current_.updated_bitmap;
    }

    const uint32_t bitmap_updated_bands = (updated_mask & BAND_LEVEL_UPDATED_BITS);

    // calculate updated band gain
    {
        utils::bitmap_looper looper(bitmap_updated_bands);
        while (looper.loop()) {
            const int i = looper.index();
            req_band_gain_db_[i] = millibel_to_decibel(current_.band_level[i]);

            if (current_.enabled) {
                cur_band_gain_db_[i] = req_band_gain_db_[i];
            }
        }
    }

    if (updated_mask & ENABLED_UPDATED_BIT) {
        if (current_.enabled) {
            // disabled -> enabled
            for (int16_t i = 0; i < NUM_BANDS; ++i) {
                cur_band_gain_db_[i] = req_band_gain_db_[i];
            }
        } else {
            // enabled -> disabled
            for (int16_t i = 0; i < NUM_BANDS; ++i) {
                cur_band_gain_db_[i] = 0.0;
            }
        }

        bitmap_updating_bands_ |= onUpdateBandsLevel(ALL_BANDS_NO_MASK);
    } else {
        if (current_.enabled && (bitmap_updated_bands != 0)) {
            utils::bitmap_looper looper(bitmap_updated_bands);
            bitmap_updating_bands_ |= onUpdateBandsLevel(bitmap_updated_bands);
        }
    }

    return OSLMP_RESULT_SUCCESS;
}

int BaseEqualizerProcessor::process(f32_stereo_frame_t *data, uint32_t num_frames) noexcept
{
    const uint32_t active_bands_mask = (current_.enabled) ? ALL_BANDS_NO_MASK : bitmap_updating_bands_;
    bitmap_updating_bands_ = onProcessBands(active_bands_mask, data, num_frames);
    return OSLMP_RESULT_SUCCESS;
}

//
// PeakingFilterEqualizerProcessor
//
PeakingFilterEqualizerProcessor::PeakingFilterEqualizerProcessor(const HQEqualizerBandInfo *band_info,
                                                                 uint32_t sampling_rate,
                                                                 hqeq_request_queue_t &request_queue,
                                                                 hqeq_request_queue_t &used_queue,
                                                                 HQEqualizerRequest &request)
    : BaseEqualizerProcessor(band_info, sampling_rate, request_queue, used_queue, request)
{
}

PeakingFilterEqualizerProcessor::~PeakingFilterEqualizerProcessor() {}

double PeakingFilterEqualizerProcessor::getCorrectedBandLevel(int band_no) const noexcept
{
    const double lower_neighbor_db_level = (band_no == 0) ? 0.0 : getCurrentBandLevel(band_no - 1);
    const double db_level = getCurrentBandLevel(band_no);
    const double higher_neighbor_db_level = (band_no == (NUM_BANDS - 1)) ? 0.0 : getCurrentBandLevel(band_no + 1);

    return db_level + (PFEQ_NEIGHBOR_BAND_CORRELATION_COEFF * (lower_neighbor_db_level + higher_neighbor_db_level));
}

void PeakingFilterEqualizerProcessor::onSetup() noexcept
{
    const double kMillHertz_to_Hertz = 0.001;
    const double fs = getSamplingRate() * kMillHertz_to_Hertz;

    for (int i = 0; i < NUM_BANDS; ++i) {
        const int cascaded_filter_no = (i >> 1);
        const int sub_filter_no = (i & 1);

        cascaded_band_filter_t &filter = cascaded_band_filters_[cascaded_filter_no];
        HQEqualizerBandCalculator &calc = band_calcs_[i];

        const double db_level = getCorrectedBandLevel(i);

        calc.init_peaking(fs, (getBandInfo(i).center_freq * kMillHertz_to_Hertz), PFEQ_PEAKING_FILTER_BAND_WIDTH,
                          db_level, MAX_GAIN_STEP);

        filter.init_partial(sub_filter_no, &(calc.get_coeffs()));
    }
}

uint32_t PeakingFilterEqualizerProcessor::onProcessBands(uint32_t band_mask, f32_stereo_frame_t *data,
                                                         uint32_t num_frames) noexcept
{
    uint32_t updating_bands_mask = 0;

    const uint32_t mod_bands_mask = band_mask | (band_mask >> 1);
    for (int i = 0; i < (NUM_BANDS / 2); ++i) {
        if (!(mod_bands_mask & BAND_NO_MASK(2 * i))) {
            continue;
        }

        if (processBandSet(i, data, num_frames)) {
            updating_bands_mask |= (BAND_NO_MASK(2 * i) | BAND_NO_MASK(2 * i + 1));
        }
    }

    return updating_bands_mask;
}

uint32_t PeakingFilterEqualizerProcessor::onUpdateBandsLevel(uint32_t bands_mask) noexcept
{
    const uint32_t mod_bands_mask = (bands_mask | (bands_mask << 1) | (bands_mask >> 1)) & ALL_BANDS_NO_MASK;

    utils::bitmap_looper looper(mod_bands_mask);
    while (looper.loop()) {
        const int band_no = looper.index();
        band_calcs_[band_no].set_gain(getCorrectedBandLevel(band_no));
    }

    return mod_bands_mask;
}

bool PeakingFilterEqualizerProcessor::processBandSet(int band_set_no, f32_stereo_frame_t *data,
                                                     uint32_t num_frames) noexcept
{
    const int band_no_1 = 2 * band_set_no + 0;
    const int band_no_2 = 2 * band_set_no + 1;
    cascaded_band_filter_t &filter = cascaded_band_filters_[band_set_no];
    HQEqualizerBandCalculator &calc1 = band_calcs_[band_no_1];
    HQEqualizerBandCalculator &calc2 = band_calcs_[band_no_2];
    uint32_t num_pending_1 = getPendingSamplesCount(band_no_1);
    uint32_t num_pending_2 = getPendingSamplesCount(band_no_2);
    uint32_t remains = num_frames;

    // handle rest of updating block
    if (remains > 0 && ((num_pending_1 > 0) || (num_pending_2 > 0))) {
        const uint32_t n = (std::min)(remains, std::max(num_pending_1, num_pending_2));

        filter.perform(data, n);

        remains -= n;
        data += n;
        num_pending_1 = std::max(0U, num_pending_1 - n);
        num_pending_2 = std::max(0U, num_pending_2 - n);
    }

    // update filter parameter
    while (remains > 0) {
        bool updating = false;

        updating |= calc1.update();
        updating |= calc2.update();

        if (!updating) {
            break;
        }

        const uint32_t n = (std::min)(remains, static_cast<uint32_t>(BLOCK_SIZE_WHILE_UPDATING));

        filter.update_partial(0, &(calc1.get_coeffs()));
        filter.update_partial(1, &(calc2.get_coeffs()));
        filter.perform(data, n);

        remains -= n;
        data += n;
        num_pending_1 = (BLOCK_SIZE_WHILE_UPDATING - n);
        num_pending_2 = (BLOCK_SIZE_WHILE_UPDATING - n);
    }

    if (remains > 0 && !(calc1.can_bypass() && calc2.can_bypass())) {
        const uint32_t n = remains;
        filter.perform(data, n);
    }

    // update fields
    setPendingSamplesCount(band_no_1, num_pending_1);
    setPendingSamplesCount(band_no_2, num_pending_2);

    return !(calc1.is_stabled() && calc2.is_stabled());
}

//
// FlatGainResponseEqualizerProcessor
//
FlatGainResponseEqualizerProcessor::FlatGainResponseEqualizerProcessor(const HQEqualizerBandInfo *band_info,
                                                                       uint32_t sampling_rate,
                                                                       hqeq_request_queue_t &request_queue,
                                                                       hqeq_request_queue_t &used_queue,
                                                                       HQEqualizerRequest &request)
    : BaseEqualizerProcessor(band_info, sampling_rate, request_queue, used_queue, request)
{
}

FlatGainResponseEqualizerProcessor::~FlatGainResponseEqualizerProcessor() {}

void FlatGainResponseEqualizerProcessor::onSetup() noexcept
{
    const double kMillHertz_to_Hertz = 0.001;
    const double fs = getSamplingRate() * kMillHertz_to_Hertz;

    {
        single_band_filter_t &filter = single_band_filters_[FGREQ_LOWEST_BAND_SINGLE_FILTER_INDEX];
        HQEqualizerBandCalculator &calc = band_calcs_[FGREQ_LOWEST_BAND_CALC_INDEX];
        const double db_level = getCurrentBandLevel(0);

        calc.init_lowshelf(fs, (getBandInfo(0).center_freq * kMillHertz_to_Hertz * HALF_BAND_WIDTH), BAND_SLOPE,
                           db_level, MAX_GAIN_STEP);

        filter.init(calc.get_coeffs());
    }

    for (int i = 1; i < (NUM_BANDS - 1); ++i) {
        cascaded_band_filter_t &filter = cascaded_band_filters_[FGREQ_MID_BAND_CASCADED_FILTER_INDEX(i)];
        HQEqualizerBandCalculator &calc1 = band_calcs_[FGREQ_MID_BAND_CALC_INDEX_1(i)];
        HQEqualizerBandCalculator &calc2 = band_calcs_[FGREQ_MID_BAND_CALC_INDEX_2(i)];

        const double db_level = getCurrentBandLevel(i);

        calc1.init_lowshelf(fs, (getBandInfo(i).center_freq * kMillHertz_to_Hertz * HALF_BAND_WIDTH_INV), BAND_SLOPE,
                            (-db_level), MAX_GAIN_STEP);

        calc2.init_lowshelf(fs, (getBandInfo(i).center_freq * kMillHertz_to_Hertz * HALF_BAND_WIDTH), BAND_SLOPE,
                            db_level, MAX_GAIN_STEP);

        filter.init_partial(0, &(calc1.get_coeffs()));
        filter.init_partial(1, &(calc2.get_coeffs()));
    }

    {
        single_band_filter_t &filter = single_band_filters_[FGREQ_HIGHEST_BAND_SINGLE_FILTER_INDEX];
        HQEqualizerBandCalculator &calc = band_calcs_[FGREQ_HIGHEST_BAND_CALC_INDEX];

        const double db_level = getCurrentBandLevel(NUM_BANDS - 1);

        calc.init_highshelf(fs, (getBandInfo(NUM_BANDS - 1).center_freq * kMillHertz_to_Hertz * HALF_BAND_WIDTH_INV),
                            BAND_SLOPE, db_level, MAX_GAIN_STEP);

        filter.init(calc.get_coeffs());
    }
}

uint32_t FlatGainResponseEqualizerProcessor::onProcessBands(uint32_t band_mask, f32_stereo_frame_t *data,
                                                            uint32_t num_frames) noexcept
{
    uint32_t updating_bands_mask = 0;

    if (band_mask & LOWEST_BAND_NO_MASK) {
        if (processEdgeBand(0, data, num_frames)) {
            updating_bands_mask |= LOWEST_BAND_NO_MASK;
        }
    }
    if (band_mask & MID_BANDS_NO_MASK) {
        utils::bitmap_looper looper(band_mask & MID_BANDS_NO_MASK);

        while (looper.loop()) {
            const int band_no = looper.index();

            if (processMidBand(band_no, data, num_frames)) {
                updating_bands_mask |= BAND_NO_MASK(band_no);
            }
        }
    }
    if (band_mask & HIGHEST_BAND_NO_MASK) {
        if (processEdgeBand((NUM_BANDS - 1), data, num_frames)) {
            updating_bands_mask |= HIGHEST_BAND_NO_MASK;
        }
    }

    return updating_bands_mask;
}

uint32_t FlatGainResponseEqualizerProcessor::onUpdateBandsLevel(uint32_t bands_mask) noexcept
{
    if (bands_mask & LOWEST_BAND_NO_MASK) {
        const double db_level = getCurrentBandLevel(0);
        HQEqualizerBandCalculator &calc = band_calcs_[FGREQ_LOWEST_BAND_CALC_INDEX];
        calc.set_gain(db_level);
    }

    if (bands_mask & MID_BANDS_NO_MASK) {
        utils::bitmap_looper looper(bands_mask & MID_BANDS_NO_MASK);

        while (looper.loop()) {
            const int band_no = looper.index();
            const double db_level = getCurrentBandLevel(band_no);
            HQEqualizerBandCalculator &calc1 = band_calcs_[FGREQ_MID_BAND_CALC_INDEX_1(band_no)];
            HQEqualizerBandCalculator &calc2 = band_calcs_[FGREQ_MID_BAND_CALC_INDEX_2(band_no)];
            calc1.set_gain(-db_level);
            calc2.set_gain(db_level);
        }
    }

    if (bands_mask & HIGHEST_BAND_NO_MASK) {
        const double db_level = getCurrentBandLevel(NUM_BANDS - 1);
        HQEqualizerBandCalculator &calc = band_calcs_[FGREQ_HIGHEST_BAND_CALC_INDEX];
        calc.set_gain(db_level);
    }

    return bands_mask;
}

bool FlatGainResponseEqualizerProcessor::processEdgeBand(int band_no, f32_stereo_frame_t *data,
                                                         uint32_t num_frames) noexcept
{
    assert((band_no == 0) || (band_no == (NUM_BANDS - 1)));

    single_band_filter_t &filter = single_band_filters_[(band_no == 0) ? FGREQ_LOWEST_BAND_SINGLE_FILTER_INDEX
                                                                       : FGREQ_HIGHEST_BAND_SINGLE_FILTER_INDEX];
    HQEqualizerBandCalculator &calc =
        band_calcs_[(band_no == 0) ? FGREQ_LOWEST_BAND_CALC_INDEX : FGREQ_HIGHEST_BAND_CALC_INDEX];
    uint32_t num_pending = getPendingSamplesCount(band_no);
    uint32_t remains = num_frames;

    // handle rest of updating block
    if (remains > 0 && num_pending > 0) {
        const uint32_t n = (std::min)(remains, num_pending);

        filter.perform(data, n);

        remains -= n;
        data += n;
        num_pending -= n;
    }

    // update filter parameter
    while (remains > 0) {
        bool updating = false;

        updating |= calc.update();

        if (!updating) {
            break;
        }

        const uint32_t n = (std::min)(remains, static_cast<uint32_t>(BLOCK_SIZE_WHILE_UPDATING));

        filter.update(calc.get_coeffs());
        filter.perform(data, n);

        remains -= n;
        data += n;
        num_pending = (BLOCK_SIZE_WHILE_UPDATING - n);
    }

    if (remains > 0 && !(calc.can_bypass())) {
        const uint32_t n = remains;
        filter.perform(data, n);
    }

    // update fields
    setPendingSamplesCount(band_no, num_pending);

    return !(calc.is_stabled());
}

bool FlatGainResponseEqualizerProcessor::processMidBand(int band_no, f32_stereo_frame_t *data,
                                                        uint32_t num_frames) noexcept
{
    assert((band_no > 0) && (band_no < (NUM_BANDS - 1)));

    cascaded_band_filter_t &filter = cascaded_band_filters_[FGREQ_MID_BAND_CASCADED_FILTER_INDEX(band_no)];
    HQEqualizerBandCalculator &calc1 = band_calcs_[FGREQ_MID_BAND_CALC_INDEX_1(band_no)];
    HQEqualizerBandCalculator &calc2 = band_calcs_[FGREQ_MID_BAND_CALC_INDEX_2(band_no)];
    uint32_t num_pending = getPendingSamplesCount(band_no);
    uint32_t remains = num_frames;

    // handle rest of updating block
    if (remains > 0 && num_pending > 0) {
        const uint32_t n = (std::min)(remains, num_pending);

        filter.perform(data, n);

        remains -= n;
        data += n;
        num_pending -= n;
    }

    // update filter parameter
    while (remains > 0) {
        bool updating = false;

        updating |= calc1.update();
        updating |= calc2.update();

        if (!updating) {
            break;
        }

        const uint32_t n = (std::min)(remains, static_cast<uint32_t>(BLOCK_SIZE_WHILE_UPDATING));

        filter.update_partial(0, &(calc1.get_coeffs()));
        filter.update_partial(1, &(calc2.get_coeffs()));
        filter.perform(data, n);

        remains -= n;
        data += n;
        num_pending = (BLOCK_SIZE_WHILE_UPDATING - n);
    }

    if (remains > 0 && !(calc1.can_bypass() && calc2.can_bypass())) {
        const uint32_t n = remains;
        filter.perform(data, n);
    }

    // update fields
    setPendingSamplesCount(band_no, num_pending);

    return !(calc1.is_stabled() && calc2.is_stabled());
}

} // namespace impl
} // namespace oslmp
