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
#include <oslmp/OpenSLMediaPlayerHQEqualizer.hpp>

extern "C" bool OpenSLMediaPlayerContext_GetInstanceFromJniHandle(jlong handle,
                                                                  android::sp<oslmp::OpenSLMediaPlayerContext> &dest);

class HQEqualizerJniContextHolder {
public:
    android::sp<oslmp::OpenSLMediaPlayerHQEqualizer> equalizer;

public:
    HQEqualizerJniContextHolder() : equalizer() {}

    ~HQEqualizerJniContextHolder() { equalizer.clear(); }

    static jlong toJniHandle(HQEqualizerJniContextHolder *holder) noexcept
    {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(holder));
    }

    static HQEqualizerJniContextHolder *fromJniHandle(jlong handle) noexcept
    {
        return reinterpret_cast<HQEqualizerJniContextHolder *>(handle);
    }
};
typedef HQEqualizerJniContextHolder Holder;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_createNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                      jlong context_handle) noexcept
{

    try
    {
        std::unique_ptr<Holder> holder(new Holder());
        android::sp<oslmp::OpenSLMediaPlayerContext> context;

        if (!OpenSLMediaPlayerContext_GetInstanceFromJniHandle(context_handle, context))
            return 0;

        holder->equalizer = new oslmp::OpenSLMediaPlayerHQEqualizer(context);

        // check the instance is alive
        bool hasControl;
        if (holder->equalizer->hasControl(&hasControl) != OSLMP_RESULT_SUCCESS)
            return 0;

        return Holder::toJniHandle(holder.release());
    }
    catch (const std::bad_alloc & /*e*/) {}
    return 0;
}

JNIEXPORT void JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_deleteNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                      jlong handle) noexcept
{
    if (handle) {
        Holder *holder = Holder::fromJniHandle(handle);
        delete holder;
    }
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_setEnabledImplNative(JNIEnv *env, jclass clazz,
                                                                                    jlong handle,
                                                                                    jboolean enabled) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->setEnabled((enabled == JNI_TRUE));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getEnabledImplNative(JNIEnv *env, jclass clazz,
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

    int result = holder->equalizer->getEnabled(&value);

    enabled_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getIdImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                               jintArray id) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array id_(env, id);

    if (!id_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getId(&(id_[0]));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_hasControlImplNative(JNIEnv *env, jclass clazz,
                                                                                    jlong handle,
                                                                                    jbooleanArray hasControl) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jboolean_array hasControl_(env, hasControl);

    if (!hasControl_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    bool value = false;

    int result = holder->equalizer->hasControl(&value);

    hasControl_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getBandImplNative(JNIEnv *env, jclass clazz,
                                                                                 jlong handle, jint frequency,
                                                                                 jshortArray band) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array band_(env, band);

    if (!band_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getBand(static_cast<uint32_t>(frequency), reinterpret_cast<uint16_t *>(band_.data()));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getBandFreqRangeImplNative(JNIEnv *env, jclass clazz,
                                                                                          jlong handle, jshort band,
                                                                                          jintArray range) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array range_(env, range);

    if (!range_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getBandFreqRange(static_cast<uint16_t>(band),
                                               reinterpret_cast<uint32_t *>(range_.data()));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getBandLevelImplNative(JNIEnv *env, jclass clazz,
                                                                                      jlong handle, jshort band,
                                                                                      jshortArray level) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array level_(env, level);

    if (!level_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getBandLevel(static_cast<uint16_t>(band), static_cast<int16_t *>(level_.data()));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getBandLevelRangeImplNative(JNIEnv *env, jclass clazz,
                                                                                           jlong handle,
                                                                                           jshortArray range) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array range_(env, range);

    if (!range_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getBandLevelRange(static_cast<int16_t *>(range_.data()));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getCenterFreqImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle, jshort band,
                                                                                       jintArray freq) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array freq_(env, freq);

    if (!freq_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getCenterFreq(static_cast<uint16_t>(band), reinterpret_cast<uint32_t *>(freq_.data()));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getCurrentPresetImplNative(JNIEnv *env, jclass clazz,
                                                                                          jlong handle,
                                                                                          jshortArray preset) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array preset_(env, preset);

    if (!preset_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getCurrentPreset(reinterpret_cast<uint16_t *>(preset_.data()));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getNumberOfBandsImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray num_bands) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array num_bands_(env, num_bands);

    if (!num_bands_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getNumberOfBands(reinterpret_cast<uint16_t *>(num_bands_.data()));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getNumberOfPresetsImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray num_presets) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array num_presets_(env, num_presets);

    if (!num_presets_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->getNumberOfPresets(reinterpret_cast<uint16_t *>(num_presets_.data()));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getPresetNameImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle, jshort preset,
                                                                                       jbyteArray name) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jbyte_array name_(env, name);

    if (!name_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    const char *c_name = nullptr;

    const int result = holder->equalizer->getPresetName(static_cast<uint16_t>(preset), &c_name);

    (void)::memset(name_.data(), 0, name_.length());

    if (result == OSLMP_RESULT_SUCCESS) {
        ::strncpy(reinterpret_cast<char *>(name_.data()), c_name, name_.length());
    }

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_getPropertiesImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle,
                                                                                       jintArray settings) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array settings_(env, settings);

    if (!settings_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    oslmp::OpenSLMediaPlayerHQEqualizer::Settings tmp;

    const int result = holder->equalizer->getProperties(&tmp);

    if (result == OSLMP_RESULT_SUCCESS) {
        // check the array length
        if (settings_.length() < (tmp.numBands + 2)) {
            return OSLMP_RESULT_ILLEGAL_ARGUMENT;
        }

        settings_[0] = tmp.curPreset & 0xffff;
        settings_[1] = tmp.numBands & 0xffff;

        for (int i = 0; i < tmp.numBands; ++i) {
            settings_[i + 2] = tmp.bandLevels[i] & 0xffff;
        }
    }

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_setBandLevelImplNative(JNIEnv *env, jclass clazz,
                                                                                      jlong handle, jshort band,
                                                                                      jshort level) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->setBandLevel(static_cast<uint16_t>(band), static_cast<int16_t>(level));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_usePresetImplNative(JNIEnv *env, jclass clazz,
                                                                                   jlong handle, jshort preset) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->equalizer->usePreset(static_cast<uint16_t>(preset));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLHQEqualizer_setPropertiesImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle,
                                                                                       jintArray settings) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    const_jint_array settings_(env, settings);

    if (!settings_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    oslmp::OpenSLMediaPlayerHQEqualizer::Settings tmp;

    tmp.curPreset = static_cast<uint16_t>(settings_[0] & 0xffff);
    tmp.numBands = static_cast<uint16_t>(settings_[1] & 0xffff);

    if (tmp.numBands > (sizeof(tmp.bandLevels) / sizeof(tmp.bandLevels[0]))) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    for (int i = 0; i < tmp.numBands; ++i) {
        tmp.bandLevels[i] = static_cast<int16_t>(settings_[i + 2]);
    }

    return holder->equalizer->setProperties(&tmp);
}

#ifdef __cplusplus
}
#endif
