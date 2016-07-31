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
#include <oslmp/OpenSLMediaPlayerPreAmp.hpp>

extern "C" bool OpenSLMediaPlayerContext_GetInstanceFromJniHandle(jlong handle,
                                                                  android::sp<oslmp::OpenSLMediaPlayerContext> &dest);

class PreaMpJniContextHolder {
public:
    android::sp<oslmp::OpenSLMediaPlayerPreAmp> preamp;

public:
    PreaMpJniContextHolder() : preamp() {}

    ~PreaMpJniContextHolder() { preamp.clear(); }

    static jlong toJniHandle(PreaMpJniContextHolder *holder) noexcept
    {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(holder));
    }

    static PreaMpJniContextHolder *fromJniHandle(jlong handle) noexcept
    {
        return reinterpret_cast<PreaMpJniContextHolder *>(handle);
    }
};
typedef PreaMpJniContextHolder Holder;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLPreAmp_createNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                 jlong context_handle) noexcept
{

    try
    {
        std::unique_ptr<Holder> holder(new Holder());
        android::sp<oslmp::OpenSLMediaPlayerContext> context;

        if (!OpenSLMediaPlayerContext_GetInstanceFromJniHandle(context_handle, context))
            return 0;

        holder->preamp = new oslmp::OpenSLMediaPlayerPreAmp(context);

        // check the instance is alive
        bool hasControl;
        if (holder->preamp->hasControl(&hasControl) != OSLMP_RESULT_SUCCESS)
            return 0;

        return Holder::toJniHandle(holder.release());
    }
    catch (const std::bad_alloc & /*e*/) {}
    return 0;
}

JNIEXPORT void JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLPreAmp_deleteNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                 jlong handle) noexcept
{
    if (handle) {
        Holder *holder = Holder::fromJniHandle(handle);
        delete holder;
    }
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLPreAmp_setEnabledImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                               jboolean enabled) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->preamp->setEnabled((enabled == JNI_TRUE));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLPreAmp_getEnabledImplNative(JNIEnv *env, jclass clazz, jlong handle,
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

    int result = holder->preamp->getEnabled(&value);

    enabled_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLPreAmp_getIdImplNative(JNIEnv *env, jclass clazz, jlong handle,
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

    return holder->preamp->getId(&(id_[0]));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLPreAmp_hasControlImplNative(JNIEnv *env, jclass clazz, jlong handle,
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

    int result = holder->preamp->hasControl(&value);

    hasControl_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLPreAmp_getLevelImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                             jfloatArray level) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jfloat_array level_(env, level);

    if (!level_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    float value = 0;

    int result = holder->preamp->getLevel(&value);

    level_[0] = static_cast<jfloat>(value);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLPreAmp_setLevelImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                             jfloat level) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->preamp->setLevel(level);
}

#ifdef __cplusplus
}
#endif
