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

#include <jni.h>
#include <jni_utils/jni_utils.hpp>

#include <cxxporthelper/cstdint>
#include <cxxporthelper/memory>

#include <oslmp/OpenSLMediaPlayerContext.hpp>

#include "OpenSLMediaPlayerContextJNIBinder.hpp"

#define CHECK_ARG(cond)                                                                                                \
    if (!(cond)) {                                                                                                     \
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;                                                                          \
    }

class OpenSLMediaPlayerContextJniContextHolder {
public:
    android::sp<oslmp::OpenSLMediaPlayerContext> context;

public:
    OpenSLMediaPlayerContextJniContextHolder() : context() {}

    ~OpenSLMediaPlayerContextJniContextHolder() { context.clear(); }

    static jlong toJniHandle(OpenSLMediaPlayerContextJniContextHolder *holder) noexcept
    {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(holder));
    }

    static OpenSLMediaPlayerContextJniContextHolder *fromJniHandle(jlong handle) noexcept
    {
        return reinterpret_cast<OpenSLMediaPlayerContextJniContextHolder *>(handle);
    }
};
typedef OpenSLMediaPlayerContextJniContextHolder Holder;

#ifdef __cplusplus
extern "C" {
#endif

// utility methods
bool OpenSLMediaPlayerContext_GetInstanceFromJniHandle(jlong handle,
                                                       android::sp<oslmp::OpenSLMediaPlayerContext> &dest) noexcept
{
    Holder *holder = Holder::fromJniHandle(handle);

    dest.clear();

    if (!holder) {
        return false;
    }

    dest = holder->context;

    return dest.get();
}

// ---

JNIEXPORT jlong JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayerContext_createNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                     jintArray params) noexcept
{

    const_jint_array params_(env, params);

    if (!params) {
        return 0;
    }

    try
    {
        std::unique_ptr<Holder> holder(new Holder());

        oslmp::OpenSLMediaPlayerContext::create_args_t create_args;

        create_args.listener = new oslmp::jni::OpenSLMediaPlayerContextJNIBinder(env);
        create_args.system_out_sampling_rate = params_[0];
        create_args.system_out_frames_per_buffer = params_[1];
        create_args.system_supports_low_latency = (params_[2]) ? true : false;
        create_args.system_supports_floating_point = (params_[3]) ? true : false;
        create_args.options = params_[4];
        create_args.stream_type = params_[5];
        create_args.short_fade_duration = params_[6];
        create_args.long_fade_duration = params_[7];
        create_args.resampler_quality = params_[8];
        create_args.hq_equalizer_impl_type = params_[9];

        holder->context = oslmp::OpenSLMediaPlayerContext::create(env, create_args);

        if (!(holder->context.get()))
            return 0;

        return Holder::toJniHandle(holder.release());
    }
    catch (const std::bad_alloc & /*e*/) {}

    return 0;
}

JNIEXPORT void JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayerContext_deleteNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                     jlong handle) noexcept
{
    if (handle) {
        Holder *holder = Holder::fromJniHandle(handle);
        delete holder;
    }
}

#ifdef __cplusplus
}
#endif
