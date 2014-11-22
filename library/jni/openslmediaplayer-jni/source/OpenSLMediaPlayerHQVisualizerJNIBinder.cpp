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

// #define LOG_TAG "OpenSLMediaPlayerVisualzierJNIBinder"

#include "OpenSLMediaPlayerHQVisualizerJNIBinder.hpp"

#include <jni_utils/jni_utils.hpp>
#include <loghelper/loghelper.h>

// constants

#define JNI_NULL nullptr

#define EVENT_TYPE_ON_WAVEFORM_DATA_CAPTURE 0
#define EVENT_TYPE_ON_FFT_DATA_CAPTURE 1

#define CAPTURE_BUFFER_TYPE_WAVEFORM                                                                                   \
    OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener::BUFFER_TYPE_WAVEFORM
#define CAPTURE_BUFFER_TYPE_FFT                                                                                        \
    OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener::BUFFER_TYPE_FFT

namespace oslmp {
namespace jni {

OpenSLMediaPlayerHQVisualizerJNIBinder::OpenSLMediaPlayerHQVisualizerJNIBinder(JNIEnv *env, jclass clazz,
                                                                               jobject weak_thiz)
    : jvm_(nullptr), env_(nullptr), jvm_attached_(false), jvisualizer_class_(), jvisualizer_weak_thiz_(),
      methodIdRaiseCaptureEventFromNative_(0), visualizer_(), waveform_buffer_index_(0), fft_buffer_index_(0)
{
    (void)env->GetJavaVM(&jvm_);

    methodIdRaiseCaptureEventFromNative_ =
        env->GetStaticMethodID(clazz, "raiseCaptureEventFromNative", "(Ljava/lang/Object;I[FII)V");

    jvisualizer_class_.assign(env, clazz, jref_type::global_reference);
    jvisualizer_weak_thiz_.assign(env, weak_thiz, jref_type::global_reference);
}

OpenSLMediaPlayerHQVisualizerJNIBinder::~OpenSLMediaPlayerHQVisualizerJNIBinder()
{
    JNIEnv *env = nullptr;

    if (jvm_) {
        (void)jvm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    }

    if (env) {
        jvisualizer_class_.release(env);
        jvisualizer_weak_thiz_.release(env);
    }

    jvm_ = nullptr;
}

int OpenSLMediaPlayerHQVisualizerJNIBinder::bind(const android::sp<OpenSLMediaPlayerHQVisualizer> &visualizer,
                                                 uint32_t rate, bool waveform, bool fft) noexcept
{

    if (!visualizer.get())
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    return visualizer->setInternalPeriodicCaptureThreadEventListener(this, rate, waveform, fft);
}

int
OpenSLMediaPlayerHQVisualizerJNIBinder::unbind(const android::sp<OpenSLMediaPlayerHQVisualizer> &visualizer) noexcept
{
    if (!visualizer.get())
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    return visualizer->setInternalPeriodicCaptureThreadEventListener(nullptr, 0, false, false);
}

void OpenSLMediaPlayerHQVisualizerJNIBinder::onEnterInternalPeriodicCaptureThread(
    OpenSLMediaPlayerHQVisualizer *visualizer) noexcept
{
    LOGD("onEnterInternalPeriodicCaptureThread()");
    JavaVM *vm = jvm_;
    JNIEnv *env = nullptr;

    if (vm && (vm->AttachCurrentThread(&env, nullptr) == JNI_OK)) {
        jvm_attached_ = true;
        env_ = env;
    } else {
        LOGE("onEnterInternalThread(); failed to attach to VM");
    }
}

void OpenSLMediaPlayerHQVisualizerJNIBinder::onLeaveInternalPeriodicCaptureThread(
    OpenSLMediaPlayerHQVisualizer *visualizer) noexcept
{
    LOGD("onLeaveInternalPeriodicCaptureThread()");

    JavaVM *vm = jvm_;

    if (jvm_attached_) {
        // release global references
        for (auto &t : jwaveform_data_) {
            t.release(env_);
        }
        for (auto &t : jfft_data_) {
            t.release(env_);
        }

        // detach
        (void)vm->DetachCurrentThread();
        jvm_attached_ = false;
    }
    env_ = nullptr;
}

void OpenSLMediaPlayerHQVisualizerJNIBinder::onAllocateCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer,
                                                                     int32_t type, int32_t size) noexcept
{
    if (!jvm_attached_)
        return;

    switch (type) {
    case CAPTURE_BUFFER_TYPE_WAVEFORM:
        waveform_buffer_index_ = 0;
        for (int i = 0; i < NUM_BUFFERS; ++i) {
            jwaveform_data_[i].assign(env_, env_->NewFloatArray(size), jref_type::global_reference);
        }
        break;
    case CAPTURE_BUFFER_TYPE_FFT:
        fft_buffer_index_ = 0;
        for (int i = 0; i < NUM_BUFFERS; ++i) {
            jfft_data_[i].assign(env_, env_->NewFloatArray(size), jref_type::global_reference);
        }
        break;
    }
}

void OpenSLMediaPlayerHQVisualizerJNIBinder::onDeAllocateCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer,
                                                                       int32_t type) noexcept
{
    if (!jvm_attached_)
        return;

    switch (type) {
    case CAPTURE_BUFFER_TYPE_WAVEFORM:
        for (auto &t : jwaveform_data_) {
            t.release(env_);
        }
        waveform_buffer_index_ = 0;
        break;
    case CAPTURE_BUFFER_TYPE_FFT:
        for (auto &t : jfft_data_) {
            t.release(env_);
        }
        fft_buffer_index_ = 0;
        break;
    }
}

void *OpenSLMediaPlayerHQVisualizerJNIBinder::onLockCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer,
                                                                  int32_t type) noexcept
{
    if (!jvm_attached_)
        return nullptr;

    jboolean isCopy = JNI_FALSE;
    void *buffer = nullptr;

    switch (type) {
    case CAPTURE_BUFFER_TYPE_WAVEFORM:
        buffer = env_->GetPrimitiveArrayCritical(jwaveform_data_[waveform_buffer_index_](), &isCopy);
        break;
    case CAPTURE_BUFFER_TYPE_FFT:
        buffer = env_->GetPrimitiveArrayCritical(jfft_data_[fft_buffer_index_](), &isCopy);
        break;
    }

#ifndef _NDEBUG
    if (isCopy) {
        LOGD("onLockCaptureBuffer() - Buffer is not mapped directly (type = %d)", type);
    }
#endif

    return buffer;
}

void OpenSLMediaPlayerHQVisualizerJNIBinder::onUnlockCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer,
                                                                   int32_t type, void *buffer) noexcept
{
    if (!jvm_attached_)
        return;

    switch (type) {
    case CAPTURE_BUFFER_TYPE_WAVEFORM:
        env_->ReleasePrimitiveArrayCritical(jwaveform_data_[waveform_buffer_index_](), buffer, 0);
        break;
    case CAPTURE_BUFFER_TYPE_FFT:
        env_->ReleasePrimitiveArrayCritical(jfft_data_[fft_buffer_index_](), buffer, 0);
        break;
    }
}

void OpenSLMediaPlayerHQVisualizerJNIBinder::onWaveFormDataCapture(OpenSLMediaPlayerHQVisualizer *visualizer,
                                                                   const float *waveform, uint32_t numChannels,
                                                                   size_t sizeInFrames, uint32_t samplingRate) noexcept
{

    // NOTE:
    // The 'waveform' argument is nullptr, but the jwaveform_data_ field is
    // already filled with captured data

    if (!(jvm_attached_ && static_cast<bool>(jwaveform_data_)))
        return;

    if (!(jwaveform_data_[waveform_buffer_index_]))
        return;

    raiseCaptureEvent(EVENT_TYPE_ON_WAVEFORM_DATA_CAPTURE, jwaveform_data_[waveform_buffer_index_](), numChannels,
                      samplingRate);

    // rotate buffer index
    waveform_buffer_index_ = (waveform_buffer_index_ + 1) % NUM_BUFFERS;
}

void OpenSLMediaPlayerHQVisualizerJNIBinder::onFftDataCapture(OpenSLMediaPlayerHQVisualizer *visualizer,
                                                              const float *fft, uint32_t numChannels,
                                                              size_t sizeInFrames, uint32_t samplingRate) noexcept
{

    // NOTE:
    // The 'fft' argument is nullptr, but the jfft_data_ field is
    // already filled with captured data

    if (!(jvm_attached_ && static_cast<bool>(jfft_data_)))
        return;

    if (!(jfft_data_[fft_buffer_index_]))
        return;

    raiseCaptureEvent(EVENT_TYPE_ON_FFT_DATA_CAPTURE, jfft_data_[fft_buffer_index_](), numChannels, samplingRate);

    // rotate buffer index
    fft_buffer_index_ = (fft_buffer_index_ + 1) % NUM_BUFFERS;
}

void OpenSLMediaPlayerHQVisualizerJNIBinder::raiseCaptureEvent(jint type, jfloatArray data, jint numChannels,
                                                               jint samplingRate) noexcept
{

    if (env_ && jvm_attached_ && jvisualizer_class_() && jvisualizer_class_() && methodIdRaiseCaptureEventFromNative_) {
        // void eaiseCaptureEventFromNative(
        //     Object ref, int type, float[] data, int numChannels, int samplingRate);
        env_->CallStaticVoidMethod(jvisualizer_class_(), methodIdRaiseCaptureEventFromNative_, jvisualizer_weak_thiz_(),
                                   type, data, numChannels, samplingRate);
    } else {
        if (!env_) {
            LOGE("env_");
        }
        if (!jvm_attached_) {
            LOGE("jvm_attached_");
        }
        if (!jvisualizer_weak_thiz_()) {
            LOGE("jvisualizer_weak_thiz_");
        }
        if (!methodIdRaiseCaptureEventFromNative_) {
            LOGE("methodIdRaiseCaptureEventFromNative_");
        }
    }
}

} // namespace jni
} // namespace oslmp
