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
#include <oslmp/OpenSLMediaPlayerHQVisualizer.hpp>

#include "OpenSLMediaPlayerHQVisualizerJNIBinder.hpp"

extern "C" bool OpenSLMediaPlayerContext_GetInstanceFromJniHandle(jlong handle,
                                                                  android::sp<oslmp::OpenSLMediaPlayerContext> &dest);

class HQVisualizerJniContextHolder {
public:
    android::sp<oslmp::OpenSLMediaPlayerHQVisualizer> visualizer;
    android::sp<oslmp::jni::OpenSLMediaPlayerHQVisualizerJNIBinder> binder;

public:
    HQVisualizerJniContextHolder() : visualizer(), binder() {}

    ~HQVisualizerJniContextHolder()
    {
        if (binder.get()) {
            binder->unbind(visualizer);
        }
        binder.clear();
        visualizer.clear();
    }

    static jlong toJniHandle(HQVisualizerJniContextHolder *holder) noexcept
    {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(holder));
    }

    static HQVisualizerJniContextHolder *fromJniHandle(jlong handle) noexcept
    {
        return reinterpret_cast<HQVisualizerJniContextHolder *>(handle);
    }
};
typedef HQVisualizerJniContextHolder Holder;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_createNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                       jlong context_handle,
                                                                                       jobject weak_thiz) noexcept
{
    try
    {
        std::unique_ptr<Holder> holder(new Holder());
        android::sp<oslmp::OpenSLMediaPlayerContext> context;

        if (!OpenSLMediaPlayerContext_GetInstanceFromJniHandle(context_handle, context))
            return 0;

        holder->visualizer = new oslmp::OpenSLMediaPlayerHQVisualizer(context);
        holder->binder = new oslmp::jni::OpenSLMediaPlayerHQVisualizerJNIBinder(env, clazz, weak_thiz);

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
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_deleteNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                       jlong handle) noexcept
{
    if (handle) {
        Holder *holder = Holder::fromJniHandle(handle);
        delete holder;
    }
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_setEnabledImplNative(JNIEnv *env, jclass clazz,
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
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_getEnabledImplNative(JNIEnv *env, jclass clazz,
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
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_getNumChannelsImplNative(JNIEnv *env, jclass clazz,
                                                                                         jlong handle,
                                                                                         jintArray numChannels) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array numChannels_(env, numChannels);

    if (!numChannels_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    uint32_t value = 0;

    int result = holder->visualizer->getNumChannels(&value);

    numChannels_[0] = static_cast<jint>(value);

    return result;
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_getSamplingRateImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jintArray samplingRate) noexcept
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
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_getCaptureSizeImplNative(JNIEnv *env, jclass clazz,
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
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_setCaptureSizeImplNative(JNIEnv *env, jclass clazz,
                                                                                         jlong handle,
                                                                                         jint size) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->visualizer->setCaptureSize(static_cast<uint32_t>(size));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_getCaptureSizeRangeImplNative(JNIEnv *env, jclass clazz,
                                                                                              jintArray range) noexcept
{
    jint_array range_(env, range);

    if (!range_) {
        return OSLMP_RESULT_ERROR;
    }

    size_t values[2] = { 0, 0 };

    int result = oslmp::OpenSLMediaPlayerHQVisualizer::sGetCaptureSizeRange(&values[0]);

    range_[0] = static_cast<jint>(values[0]);
    range_[1] = static_cast<jint>(values[1]);

    return result;
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_setDataCaptureListenerImplNative(
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
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_getMaxCaptureRateImplNative(JNIEnv *env, jclass clazz,
                                                                                            jintArray rate) noexcept
{
    jint_array rate_(env, rate);

    if (!rate_) {
        return OSLMP_RESULT_ERROR;
    }

    uint32_t value = 0;

    int result = oslmp::OpenSLMediaPlayerHQVisualizer::sGetMaxCaptureRate(&value);

    rate_[0] = value;

    return result;
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_getWindowFunctionImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jintArray windowType) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array windowType_(env, windowType);

    if (!windowType_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    uint32_t value = 0;

    int result = holder->visualizer->getWindowFunction(&value);

    windowType_[0] = static_cast<jint>(value);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQVisualizer_setWindowFunctionImplNative(JNIEnv *env, jclass clazz,
                                                                                            jlong handle,
                                                                                            jint windowType) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->visualizer->setWindowFunction(static_cast<uint32_t>(windowType));
}

#ifdef __cplusplus
}
#endif
