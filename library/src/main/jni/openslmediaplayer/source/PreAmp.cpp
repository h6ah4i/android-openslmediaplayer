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

#include "oslmp/impl/PreAmp.hpp"

#include "oslmp/OpenSLMediaPlayerResultCodes.hpp"
#include "oslmp/impl/AudioMixer.hpp"

namespace oslmp {
namespace impl {

class PreAmp::Impl {
public:
    Impl();
    ~Impl();

    bool initialize(const initialize_args_t &args) noexcept;

    int setEnabled(bool enabled) noexcept;
    int getEnabled(bool *enabled) const noexcept;
    int setLevel(float level) noexcept;
    int getLevel(float *level) const noexcept;

private:
    void apply() noexcept;

private:
    AudioMixer *mixer_;
    bool enabled_;
    float level_;
};

PreAmp::PreAmp() : impl_(new (std::nothrow) Impl()) {}

PreAmp::~PreAmp() {}

bool PreAmp::initialize(const initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int PreAmp::setEnabled(bool enabled) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setEnabled(enabled);
}

int PreAmp::getEnabled(bool *enabled) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getEnabled(enabled);
}

int PreAmp::setLevel(float level) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->setLevel(level);
}

int PreAmp::getLevel(float *level) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getLevel(level);
}

PreAmp::Impl::Impl() : mixer_(nullptr), enabled_(false), level_(1.0f) {}

PreAmp::Impl::~Impl() {}

bool PreAmp::Impl::initialize(const initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!(args.mixer))) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    mixer_ = args.mixer;

    return OSLMP_RESULT_SUCCESS;
}

int PreAmp::Impl::setEnabled(bool enabled) noexcept
{
    enabled_ = enabled;

    apply();

    return OSLMP_RESULT_SUCCESS;
}

int PreAmp::Impl::getEnabled(bool *enabled) const noexcept
{
    if (CXXPH_UNLIKELY(!enabled)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    (*enabled) = enabled_;

    return OSLMP_RESULT_SUCCESS;
}

int PreAmp::Impl::setLevel(float level) noexcept
{
    level_ = level;

    apply();

    return OSLMP_RESULT_SUCCESS;
}

int PreAmp::Impl::getLevel(float *level) const noexcept
{
    if (CXXPH_UNLIKELY(!level)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    (*level) = level_;

    return OSLMP_RESULT_SUCCESS;
}

void PreAmp::Impl::apply() noexcept
{
    const float l = (enabled_) ? level_ : 1.0f;
    mixer_->setGlobalPreMixVolumeLevel(l);
}

} // namespace impl
} // namespace oslmp
