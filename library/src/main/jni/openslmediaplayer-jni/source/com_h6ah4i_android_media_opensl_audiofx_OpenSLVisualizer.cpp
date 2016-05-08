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

#include <oslmp/OpenSLMediaPlayer.hpp>
#include <oslmp/OpenSLMediaPlayerVisualizer.hpp>

#include "OpenSLMediaPlayerVisualizerJNIBinder.hpp"

extern "C" bool OpenSLMediaPlayerContext_GetInstanceFromJniHandle(jlong handle,
                                                                  android::sp<oslmp::OpenSLMediaPlayerContext> &dest);

class VisualizerJniContextHolder {
public:
    android::sp<oslmp::OpenSLMediaPlayerVisualizer> visualizer;
    android::sp<oslmp::jni::OpenSLMediaPlayerVisualizerJNIBinder> binder;

public:
    VisualizerJniContextHolder() : visualizer(), binder() {}

    ~VisualizerJniContextHolder()
    {
        if (binder.get()) {
            binder->unbind(visualizer);
        }
        binder.clear();
        visualizer.clear();
    }

    static jlong toJniHandle(VisualizerJniContextHolder *holder) noexcept
    {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(holder));
    }

    static VisualizerJniContextHolder *fromJniHandle(jlong handle) noexcept
    {
        return reinterpret_cast<VisualizerJniContextHolder *>(handle);
    }
};
typedef VisualizerJniContextHolder Holder;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_createNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                     jlong context_handle,
                                                                                     jobject weak_thiz) noexcept
{
    try
    {
        std::unique_ptr<Holder> holder(new Holder());
        android::sp<oslmp::OpenSLMediaPlayerContext> context;

        if (!OpenSLMediaPlayerContext_GetInstanceFromJniHandle(context_handle, context))
            return 0;

        holder->visualizer = new oslmp::OpenSLMediaPlayerVisualizer(context);
        holder->binder = new oslmp::jni::OpenSLMediaPlayerVisualizerJNIBinder(env, clazz, weak_thiz);

        // check the instance is alive
        bool isEnabled;
        if (holder->visualizer->getEnabled(&isEnabled) != OSLMP_RESULT_SUCCESS)
            return 0;

        return Holder::toJniHandle(holder.release());
    }
    catch (const std::bad_alloc & /*e*/) {}
    return 0;
}

JNIEXPORT void JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_deleteNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                     jlong handle) noexcept
{
    if (handle) {
        Holder *holder = Holder::fromJniHandle(handle);
        delete holder;
    }
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_setEnabledImplNative(JNIEnv *env, jclass clazz,
                                                                                   jlong handle,
                                                                                   jboolean enabled) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->visualizer->setEnabled((enabled == JNI_TRUE));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getEnabledImplNative(JNIEnv *env, jclass clazz,
                                                                                   jlong handle,
                                                                                   jbooleanArray enabled) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jboolean_array enabled_(env, enabled);

    if (!enabled_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    bool value = false;

    int result = holder->visualizer->getEnabled(&value);

    enabled_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getSamplingRateImplNative(JNIEnv *env, jclass clazz,
                                                                                        jlong handle,
                                                                                        jintArray samplingRate) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array samplingRate_(env, samplingRate);

    if (!samplingRate_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    uint32_t value = 0;

    int result = holder->visualizer->getSamplingRate(&value);

    samplingRate_[0] = static_cast<jint>(value);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getCaptureSizeImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle,
                                                                                       jintArray size) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array size_(env, size);

    if (!size_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    size_t value = 0;

    int result = holder->visualizer->getCaptureSize(&value);

    size_[0] = static_cast<jint>(value);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_setCaptureSizeImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle, jint size) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->visualizer->setCaptureSize(static_cast<uint32_t>(size));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getCaptureSizeRangeImplNative(JNIEnv *env, jclass clazz,
                                                                                            jintArray range) noexcept
{
    jint_array range_(env, range);

    if (!range_) {
        return OSLMP_RESULT_ERROR;
    }

    size_t values[2] = { 0, 0 };

    int result = oslmp::OpenSLMediaPlayerVisualizer::sGetCaptureSizeRange(&values[0]);

    range_[0] = static_cast<jint>(values[0]);
    range_[1] = static_cast<jint>(values[1]);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getFftImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                               jbyteArray fft) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    if (!fft) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    jbyte_critical_array fft_(env, fft);

    if (!fft_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    int result = holder->visualizer->getFft(reinterpret_cast<int8_t *>(fft_.data()), fft_.length());

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getWaveformImplNative(JNIEnv *env, jclass clazz,
                                                                                    jlong handle,
                                                                                    jbyteArray waveform) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    if (!waveform) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    jbyte_critical_array waveform_(env, waveform);

    if (!waveform_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    int result = holder->visualizer->getWaveForm(reinterpret_cast<uint8_t *>(waveform_.data()), waveform_.length());

    return result;
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_setDataCaptureListenerImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jint rate, jboolean waveform, jboolean fft) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    int result;

    if (rate != 0) {
        result = holder->binder->bind(holder->visualizer, rate, (waveform == JNI_TRUE), (fft == JNI_TRUE));
    } else {
        result = holder->binder->unbind(holder->visualizer);
    }

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getMaxCaptureRateImplNative(JNIEnv *env, jclass clazz,
                                                                                          jintArray rate) noexcept
{
    jint_array rate_(env, rate);

    if (!rate_) {
        return OSLMP_RESULT_ERROR;
    }

    uint32_t value = 0;

    int result = oslmp::OpenSLMediaPlayerVisualizer::sGetMaxCaptureRate(&value);
    rate_[0] = value;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getScalingModeImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle,
                                                                                       jintArray mode) noexcept
{
    jint_array mode_(env, mode);

    if (!mode_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    int32_t value = 0;

    int result = holder->visualizer->getScalingMode(&value);

    mode_[0] = value;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_setScalingModeImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle, jint mode) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->visualizer->setScalingMode(mode);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getMeasurementModeImplNative(JNIEnv *env, jclass clazz,
                                                                                           jlong handle,
                                                                                           jintArray mode) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array mode_(env, mode);

    if (!mode_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    int32_t value = 0;

    int result = holder->visualizer->getMeasurementMode(&value);

    mode_[0] = value;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_setMeasurementModeImplNative(JNIEnv *env, jclass clazz,
                                                                                           jlong handle,
                                                                                           jint mode) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->visualizer->setMeasurementMode(mode);
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLVisualizer_getMeasurementPeakRmsImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jintArray measurement) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array measurement_(env, measurement);

    if (!measurement_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    oslmp::OpenSLMediaPlayerVisualizer::MeasurementPeakRms values;

    int result = holder->visualizer->getMeasurementPeakRms(&values);

    measurement_[0] = values.mPeak;
    measurement_[1] = values.mRms;

    return result;
}

#ifdef __cplusplus
}
#endif
