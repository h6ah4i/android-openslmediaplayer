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

// #define LOG_TAG "OpenSLMediaPlayerContext"

#include "oslmp/OpenSLMediaPlayerContext.hpp"

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContextImpl.hpp"

//
// macros
//
namespace oslmp {

using namespace ::opensles;
using namespace ::oslmp::impl;

class OpenSLMediaPlayerContext::Impl {
public:
    Impl();
    ~Impl();

    android::sp<OpenSLMediaPlayerInternalContextImpl> internal_;
};

//
// OpenSLMediaPlayerContext
//
OpenSLMediaPlayerContext::OpenSLMediaPlayerContext(OpenSLMediaPlayerContext::Impl *impl) : impl_(impl) {}

OpenSLMediaPlayerContext::~OpenSLMediaPlayerContext()
{
    delete impl_;
    impl_ = nullptr;
}

OpenSLMediaPlayerContext::InternalThreadEventListener *OpenSLMediaPlayerContext::getInternalThreadEventListener() const
    noexcept
{
    return getInternal().getInternalThreadEventListener();
}

OpenSLMediaPlayerInternalContext &OpenSLMediaPlayerContext::getInternal() const noexcept
{
    return (*(impl_->internal_));
}

android::sp<OpenSLMediaPlayerContext>
OpenSLMediaPlayerContext::create(JNIEnv *env, const OpenSLMediaPlayerContext::create_args_t &args) noexcept
{
    typedef android::sp<OpenSLMediaPlayerContext> sp_context_t;

    if (!env) {
        LOGD("OpenSLMediaPlayerContext::create(); requires valid JNIEnv object");
        return sp_context_t();
    }

    typedef OpenSLMediaPlayerContext context_t;
    try
    {
        Impl *impl = new (std::nothrow) Impl();
        sp_context_t instance(new (std::nothrow) context_t(impl));

        SLresult result;
        if (impl->internal_.get()) {
            result = impl->internal_->initialize(env, instance.get(), args);
        } else {
            result = OSLMP_RESULT_INTERNAL_ERROR;
        }

        if (result != SL_RESULT_SUCCESS) {
            LOGD("OpenSLMediaPlayerContext::create() failed; result = %d", result);
            return sp_context_t();
        }

        return instance;
    }
    catch (const std::bad_alloc & /*e*/) { LOGD("OpenSLMediaPlayerContext::create(); memory allocation failed"); }

    return sp_context_t();
}

//
// OpenSLMediaPlayerInternalContext::Impl
//
OpenSLMediaPlayerContext::Impl::Impl() : internal_(new (std::nothrow) OpenSLMediaPlayerInternalContextImpl()) {}

OpenSLMediaPlayerContext::Impl::~Impl() { internal_.clear(); }

} // namespace oslmp
