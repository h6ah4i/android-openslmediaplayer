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
#include <oslmp/OpenSLMediaPlayerBassBoost.hpp>

extern "C" bool OpenSLMediaPlayerContext_GetInstanceFromJniHandle(jlong handle,
                                                                  android::sp<oslmp::OpenSLMediaPlayerContext> &dest);

class BassBoostJniContextHolder {
public:
    android::sp<oslmp::OpenSLMediaPlayerBassBoost> bassboost;

public:
    BassBoostJniContextHolder() : bassboost() {}

    ~BassBoostJniContextHolder() { bassboost.clear(); }

    static jlong toJniHandle(BassBoostJniContextHolder *holder) noexcept
    {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(holder));
    }

    static BassBoostJniContextHolder *fromJniHandle(jlong handle) noexcept
    {
        return reinterpret_cast<BassBoostJniContextHolder *>(handle);
    }
};
typedef BassBoostJniContextHolder Holder;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_createNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                    jlong context_handle) noexcept
{

    try
    {
        std::unique_ptr<Holder> holder(new Holder());
        android::sp<oslmp::OpenSLMediaPlayerContext> context;

        if (!OpenSLMediaPlayerContext_GetInstanceFromJniHandle(context_handle, context))
            return 0;

        holder->bassboost = new oslmp::OpenSLMediaPlayerBassBoost(context);

        // check the instance is alive
        bool hasControl;
        if (holder->bassboost->hasControl(&hasControl) != OSLMP_RESULT_SUCCESS)
            return 0;

        return Holder::toJniHandle(holder.release());
    }
    catch (const std::bad_alloc & /*e*/) {}
    return 0;
}

JNIEXPORT void JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_deleteNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                    jlong handle) noexcept
{
    if (handle) {
        Holder *holder = Holder::fromJniHandle(handle);
        delete holder;
    }
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_setEnabledImplNative(JNIEnv *env, jclass clazz,
                                                                                  jlong handle,
                                                                                  jboolean enabled) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->bassboost->setEnabled((enabled == JNI_TRUE));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_getEnabledImplNative(JNIEnv *env, jclass clazz,
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

    int result = holder->bassboost->getEnabled(&value);

    enabled_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_getIdImplNative(JNIEnv *env, jclass clazz, jlong handle,
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

    return holder->bassboost->getId(&(id_[0]));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_hasControlImplNative(JNIEnv *env, jclass clazz,
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

    int result = holder->bassboost->hasControl(&value);

    hasControl_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_getStrengthSupportedImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jbooleanArray strengthSupported) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jboolean_array strengthSupported_(env, strengthSupported);

    if (!strengthSupported_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    bool value = false;

    int result = holder->bassboost->getStrengthSupported(&value);

    strengthSupported_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_getRoundedStrengthImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray roundedStrength) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array roundedStrength_(env, roundedStrength);

    if (!roundedStrength_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    int16_t value = 0;

    int result = holder->bassboost->getRoundedStrength(&value);

    roundedStrength_[0] = static_cast<jshort>(value);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_getPropertiesImplNative(JNIEnv *env, jclass clazz,
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

    oslmp::OpenSLMediaPlayerBassBoost::Settings tmp;

    const int result = holder->bassboost->getProperties(&tmp);

    settings_[0] = tmp.strength & 0xffff;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_setStrengthImplNative(JNIEnv *env, jclass clazz,
                                                                                   jlong handle,
                                                                                   jshort strength) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->bassboost->setStrength(strength);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLBassBoost_setPropertiesImplNative(JNIEnv *env, jclass clazz,
                                                                                     jlong handle,
                                                                                     jintArray settings) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    const_jint_array settings_(env, settings);

    if (!settings_) {
        return OSLMP_RESULT_ERROR;
    }

    oslmp::OpenSLMediaPlayerBassBoost::Settings tmp;

    tmp.strength = (int16_t)(settings_[0] & 0xffff);

    return holder->bassboost->setProperties(&tmp);
}

#ifdef __cplusplus
}
#endif
