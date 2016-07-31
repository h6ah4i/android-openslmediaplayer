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

#include "OpenSLMediaPlayerJNIBinder.hpp"

extern "C" bool OpenSLMediaPlayerContext_GetInstanceFromJniHandle(jlong handle,
                                                                  android::sp<oslmp::OpenSLMediaPlayerContext> &dest);

class OpenSLMediaPlayerJniContextHolder {
public:
    android::sp<oslmp::OpenSLMediaPlayerContext> context;
    android::sp<oslmp::OpenSLMediaPlayer> mp;
    android::sp<oslmp::jni::OpenSLMediaPlayerJNIBinder> binder;

public:
    OpenSLMediaPlayerJniContextHolder() : context(), mp(), binder() {}

    ~OpenSLMediaPlayerJniContextHolder()
    {
        if (binder.get()) {
            binder->unbind();
        }
        mp.clear();
        binder.clear();
        context.clear();
    }

    static jlong toJniHandle(OpenSLMediaPlayerJniContextHolder *holder) noexcept
    {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(holder));
    }

    static OpenSLMediaPlayerJniContextHolder *fromJniHandle(jlong handle) noexcept
    {
        return reinterpret_cast<OpenSLMediaPlayerJniContextHolder *>(handle);
    }
};
typedef OpenSLMediaPlayerJniContextHolder Holder;

#ifdef __cplusplus
extern "C" {
#endif

// utility methods
bool OpenSLMediaPlayer_GetInstanceFromJniHandle(jlong handle, android::sp<oslmp::OpenSLMediaPlayer> &dest) noexcept
{
    Holder *holder = Holder::fromJniHandle(handle);

    dest.clear();

    if (!holder) {
        return false;
    }

    dest = holder->mp;

    return dest.get();
}

// ---

JNIEXPORT jlong JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_createNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                              jlong contextHandle, jobject weak_thiz,
                                                                              jintArray params) noexcept
{

    const_jint_array params_(env, params);

    if (!params) {
        return 0;
    }

    try
    {
        std::unique_ptr<Holder> holder(new Holder());

        OpenSLMediaPlayerContext_GetInstanceFromJniHandle(contextHandle, holder->context);

        if (!(holder->context.get()))
            return 0;

        oslmp::OpenSLMediaPlayer::initialize_args_t init_args;

        init_args.use_fade = (params_[0] != 0);

        holder->mp = new oslmp::OpenSLMediaPlayer(holder->context);
        holder->binder = new oslmp::jni::OpenSLMediaPlayerJNIBinder(env, clazz, weak_thiz);

        holder->binder->bind(holder->context, holder->mp);
        if (holder->mp->initialize(init_args) != OSLMP_RESULT_SUCCESS)
            return 0;

        return Holder::toJniHandle(holder.release());
    }
    catch (const std::bad_alloc & /*e*/) {}
    return 0;
}

JNIEXPORT void JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_deleteNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                              jlong handle) noexcept
{
    if (handle) {
        Holder *holder = Holder::fromJniHandle(handle);
        delete holder;
    }
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setDataSourcePathImplNative(JNIEnv *env, jclass clazz,
                                                                                   jlong handle, jstring path) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jstring_wrapper path_w(env, path);

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->setDataSourcePath(path_w.data());
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setDataSourceUriImplNative(JNIEnv *env, jclass clazz,
                                                                                  jlong handle, jstring uri) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jstring_wrapper uri_w(env, uri);

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->setDataSourceUri(uri_w.data());
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setDataSourceFdImplNative__JI(JNIEnv *env, jclass clazz,
                                                                                     jlong handle, jint fd) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->setDataSourceFd(fd);
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setDataSourceFdImplNative__JIJJ(
    JNIEnv *env, jclass clazz, jlong handle, jint fd, jlong offset, jlong length) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->setDataSourceFd(fd, offset, length);
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_prepareImplNative(JNIEnv *env,
                                                                                                jclass clazz,
                                                                                                jlong handle) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->prepare();
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_prepareAsyncImplNative(JNIEnv *env, jclass clazz,
                                                                              jlong handle) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->prepareAsync();
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_startImplNative(JNIEnv *env, jclass clazz, jlong handle) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->start();
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_stopImplNative(JNIEnv *env, jclass clazz, jlong handle) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->stop();
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_pauseImplNative(JNIEnv *env, jclass clazz, jlong handle) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->pause();
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_resetImplNative(JNIEnv *env, jclass clazz, jlong handle) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->reset();
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setVolumeImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                           jfloat leftVolume,
                                                                           jfloat rightVolume) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->setVolume(leftVolume, rightVolume);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_getAudioSessionIdImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                             jintArray audioSessionId) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array audioSessionId_(env, audioSessionId);

    if (!audioSessionId_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->getAudioSessionId(audioSessionId_.data());
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_getDurationImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                             jintArray duration) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array duration_(env, duration);

    if (!duration_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->getDuration(duration_.data());
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_getCurrentPositionImplNative(JNIEnv *env, jclass clazz,
                                                                                    jlong handle,
                                                                                    jintArray position) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array position_(env, position);

    if (!position_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->getCurrentPosition(position_.data());
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_seekToImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                        jint msec) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->seekTo(msec);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setLoopingImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                            jboolean looping) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->setLooping((looping == JNI_TRUE));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_isLoopingImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                           jbooleanArray looping) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jboolean_array looping_(env, looping);

    if (!looping_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    bool value = false;

    int result = holder->mp->isLooping(&value);

    looping_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_isPlayingImplNative(JNIEnv *env, jclass clazz, jlong handle,
                                                                           jbooleanArray playing) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jboolean_array playing_(env, playing);

    if (!playing_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    bool value = false;

    int result = holder->mp->isPlaying(&value);

    playing_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setAudioStreamTypeImplNative(JNIEnv *env, jclass clazz,
                                                                                    jlong handle,
                                                                                    jint streamtype) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    int result = holder->mp->setAudioStreamType(streamtype);

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setNextMediaPlayerImplNative(JNIEnv *env, jclass clazz,
                                                                                    jlong handle,
                                                                                    jlong nextHandle) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);
    Holder *nextHolder = Holder::fromJniHandle(nextHandle);

    android::sp<oslmp::OpenSLMediaPlayer> next;

    if (nextHolder) {
        next = nextHolder->mp;
    }

    return holder->mp->setNextMediaPlayer(&next);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_attachAuxEffectImplNative(JNIEnv *env, jclass clazz,
                                                                                 jlong handle, jint effectId) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->attachAuxEffect(effectId);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayer_setAuxEffectSendLevelImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle,
                                                                                       jfloat level) noexcept
{

    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->mp->setAuxEffectSendLevel(level);
}

#ifdef __cplusplus
}
#endif
