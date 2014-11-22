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

#ifndef HQEQUALIZERBANDCALCULATOR_HPP_
#define HQEQUALIZERBANDCALCULATOR_HPP_

#include <cxxporthelper/cmath>

#include <cxxdasp/filter/digital_filter.hpp>
#include <cxxdasp/filter/biquad/biquad_filter_coeffs.hpp>

class HQEqualizerBandCalculator {
public:
    HQEqualizerBandCalculator();
    ~HQEqualizerBandCalculator();

    void init_peaking(double fs, double f0, double bandwidth, double init_gain, double max_gain_step) noexcept;
    void init_lowshelf(double fs, double f0, double slope, double init_gain, double max_gain_step) noexcept;
    void init_highshelf(double fs, double f0, double slope, double init_gain, double max_gain_step) noexcept;

    bool update() noexcept;
    bool is_stabled() const noexcept;
    void set_gain(double gain) noexcept;
    const cxxdasp::filter::biquad_filter_coeffs &get_coeffs() const noexcept;
    bool can_bypass() const noexcept;

private:
    void init_common(cxxdasp::filter::filter_type_t filter_type, double fs, double f0, double q, double init_gain,
                     double max_gain_step) noexcept;
    void calculate_coeffs() noexcept;

    static double calc_w0(double fs, double f0) { return ((2.0 * M_PI) * f0 / fs); }

    static double calc_A(double db_gain) { return pow(10.0, (db_gain * (1.0 / 40.0))); }

    static double calc_alpha_q(double sin_w0, double q) { return (sin_w0 / (2.0 * q)); }

private:
    cxxdasp::filter::filter_type_t filter_type_;
    cxxdasp::filter::biquad_filter_coeffs coeffs_;
    double bandwidth_slope_;
    double sin_w0_;
    double cos_w0_;
    double alpha_;
    double current_gain_;  // [dB]
    double target_gain_;   // [dB]
    double max_gain_step_; // [dB]
    bool is_dirty_;
};

HQEqualizerBandCalculator::HQEqualizerBandCalculator()
    : filter_type_(cxxdasp::filter::types::Unknown), coeffs_(), bandwidth_slope_(0), sin_w0_(0), cos_w0_(0), alpha_(0),
      current_gain_(0), target_gain_(0), max_gain_step_(0), is_dirty_(false)
{
}
HQEqualizerBandCalculator::~HQEqualizerBandCalculator() {}

inline void HQEqualizerBandCalculator::init_peaking(double fs, double f0, double bandwidth, double init_gain,
                                                    double max_gain_step) noexcept
{

    const double q = cxxdasp::filter::filter_params_t::calc_q_from_band_width(fs, f0, bandwidth);

    bandwidth_slope_ = bandwidth;
    init_common(cxxdasp::filter::types::Peak, fs, f0, q, init_gain, max_gain_step);
}

inline void HQEqualizerBandCalculator::init_lowshelf(double fs, double f0, double slope, double init_gain,
                                                     double max_gain_step) noexcept
{

    const double q = cxxdasp::filter::filter_params_t::calc_q_from_slope(init_gain, slope);

    bandwidth_slope_ = slope;
    init_common(cxxdasp::filter::types::LowShelf, fs, f0, q, init_gain, max_gain_step);
}

inline void HQEqualizerBandCalculator::init_highshelf(double fs, double f0, double slope, double init_gain,
                                                      double max_gain_step) noexcept
{

    const double q = cxxdasp::filter::filter_params_t::calc_q_from_slope(init_gain, slope);

    bandwidth_slope_ = slope;
    init_common(cxxdasp::filter::types::HighShelf, fs, f0, q, init_gain, max_gain_step);
}

inline bool HQEqualizerBandCalculator::update() noexcept
{
    if (!is_dirty_) {
        return false;
    }

    if (std::abs(current_gain_ - target_gain_) < max_gain_step_) {
        current_gain_ = target_gain_;
        is_dirty_ = false;
    } else if (current_gain_ > target_gain_) {
        current_gain_ -= max_gain_step_;
    } else /* if (current_gain_ < target_gain_) */ {
        current_gain_ += max_gain_step_;
    }

    switch (filter_type_) {
    case cxxdasp::filter::types::LowShelf:
    case cxxdasp::filter::types::HighShelf: {
        const double q = cxxdasp::filter::filter_params_t::calc_q_from_slope(current_gain_, bandwidth_slope_);
        alpha_ = calc_alpha_q(sin_w0_, q);
    } break;
    default:
        break;
    }

    calculate_coeffs();

    return is_dirty_;
}

inline bool HQEqualizerBandCalculator::is_stabled() const noexcept { return !(is_dirty_); }

inline void HQEqualizerBandCalculator::set_gain(double gain) noexcept
{
    target_gain_ = gain;
    is_dirty_ = true;
}

inline const cxxdasp::filter::biquad_filter_coeffs &HQEqualizerBandCalculator::get_coeffs() const noexcept
{
    return coeffs_;
}

inline bool HQEqualizerBandCalculator::can_bypass() const noexcept
{
    return is_stabled() && (current_gain_ == 0.0) && (target_gain_ == 0.0);
}

void HQEqualizerBandCalculator::init_common(cxxdasp::filter::filter_type_t filter_type, double fs, double f0, double q,
                                            double init_gain, double max_gain_step) noexcept
{

    const double w0 = calc_w0(fs, f0);

    filter_type_ = filter_type;
    sin_w0_ = sin(w0);
    cos_w0_ = cos(w0);
    alpha_ = calc_alpha_q(sin_w0_, q);
    current_gain_ = init_gain;
    target_gain_ = init_gain;
    max_gain_step_ = max_gain_step;
    is_dirty_ = false;

    calculate_coeffs();
}

inline void HQEqualizerBandCalculator::calculate_coeffs() noexcept
{
    const double A = calc_A(current_gain_);
    const double cos_w0 = cos_w0_;
    const double sin_w0 = sin_w0_;
    const double alpha = alpha_;

    double b0, b1, b2, a0, a1, a2;

    switch (filter_type_) {
    case cxxdasp::filter::types::Peak: {
        const double alpha_mul_A = alpha * A;
        const double alpha_div_A = alpha / A;

        b0 = 1 + alpha_mul_A;
        b1 = -2 * cos_w0;
        b2 = 1 - alpha_mul_A;
        a0 = 1 + alpha_div_A;
        a1 = -2 * cos_w0;
        a2 = 1 - alpha_div_A;
    } break;

    case cxxdasp::filter::types::LowShelf: {
        const double sqrt_A = sqrt(A);
        const double d_sqrt_A_alpha = 2 * sqrt_A * alpha;
        const double b0_b2_common = (A + 1) - (A - 1) * cos_w0;
        const double a0_a2_common = (A + 1) + (A - 1) * cos_w0;
        const double b1_a1_common = (A + 1) * cos_w0;

        b0 = A * (b0_b2_common + d_sqrt_A_alpha);
        b1 = 2 * A * ((A - 1) - b1_a1_common);
        b2 = A * (b0_b2_common - d_sqrt_A_alpha);
        a0 = a0_a2_common + d_sqrt_A_alpha;
        a1 = -2 * ((A - 1) + b1_a1_common);
        a2 = a0_a2_common - d_sqrt_A_alpha;
    } break;

    case cxxdasp::filter::types::HighShelf: {
        const double sqrt_A = sqrt(A);
        const double d_sqrt_A_alpha = 2 * sqrt_A * alpha;
        const double b0_b2_common = (A + 1) + (A - 1) * cos_w0;
        const double a0_a2_common = (A + 1) - (A - 1) * cos_w0;
        const double b1_a1_common = (A + 1) * cos_w0;

        b0 = A * (b0_b2_common + d_sqrt_A_alpha);
        b1 = -2 * A * ((A - 1) + b1_a1_common);
        b2 = A * (b0_b2_common - d_sqrt_A_alpha);
        a0 = a0_a2_common + d_sqrt_A_alpha;
        a1 = 2 * ((A - 1) - b1_a1_common);
        a2 = a0_a2_common - d_sqrt_A_alpha;
    } break;

    default:
        // bypass
        b0 = 1;
        b1 = 0;
        b2 = 0;
        a0 = 1;
        a1 = 0;
        a2 = 0;
        break;
    }

    const double ia0 = 1.0 / a0;

    coeffs_.b0 = (b0 * ia0);
    coeffs_.b1 = (b1 * ia0);
    coeffs_.b2 = (b2 * ia0);
    coeffs_.a1 = (a1 * ia0);
    coeffs_.a2 = (a2 * ia0);
}

#endif // HQEQUALIZERBANDCALCULATOR_HPP_
