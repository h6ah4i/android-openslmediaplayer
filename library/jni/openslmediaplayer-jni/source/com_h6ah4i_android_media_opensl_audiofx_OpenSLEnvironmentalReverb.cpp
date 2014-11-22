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
#include <oslmp/OpenSLMediaPlayerEnvironmentalReverb.hpp>

extern "C" bool OpenSLMediaPlayerContext_GetInstanceFromJniHandle(jlong handle,
                                                                  android::sp<oslmp::OpenSLMediaPlayerContext> &dest);

class EnvironmentalReverbJniContextHolder {
public:
    android::sp<oslmp::OpenSLMediaPlayerEnvironmentalReverb> envreverb;

public:
    EnvironmentalReverbJniContextHolder() : envreverb() {}

    ~EnvironmentalReverbJniContextHolder() { envreverb.clear(); }

    static jlong toJniHandle(EnvironmentalReverbJniContextHolder *holder) noexcept
    {
        return static_cast<jlong>(reinterpret_cast<uintptr_t>(holder));
    }

    static EnvironmentalReverbJniContextHolder *fromJniHandle(jlong handle) noexcept
    {
        return reinterpret_cast<EnvironmentalReverbJniContextHolder *>(handle);
    }
};
typedef EnvironmentalReverbJniContextHolder Holder;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_createNativeImplHandle(
    JNIEnv *env, jclass clazz, jlong context_handle) noexcept
{

    try
    {
        std::unique_ptr<Holder> holder(new Holder());
        android::sp<oslmp::OpenSLMediaPlayerContext> context;

        if (!OpenSLMediaPlayerContext_GetInstanceFromJniHandle(context_handle, context))
            return 0;

        holder->envreverb = new oslmp::OpenSLMediaPlayerEnvironmentalReverb(context);

        // check the instance is alive
        bool hasControl;
        if (holder->envreverb->hasControl(&hasControl) != OSLMP_RESULT_SUCCESS)
            return 0;

        return Holder::toJniHandle(holder.release());
    }
    catch (const std::bad_alloc & /*e*/) {}
    return 0;
}

JNIEXPORT void JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_deleteNativeImplHandle(JNIEnv *env, jclass clazz,
                                                                                              jlong handle) noexcept
{
    if (handle) {
        Holder *holder = Holder::fromJniHandle(handle);
        delete holder;
    }
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setEnabledImplNative(JNIEnv *env, jclass clazz,
                                                                                            jlong handle,
                                                                                            jboolean enabled) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setEnabled((enabled == JNI_TRUE));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getEnabledImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jbooleanArray enabled) noexcept
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

    int result = holder->envreverb->getEnabled(&value);

    enabled_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getIdImplNative(JNIEnv *env, jclass clazz,
                                                                                       jlong handle,
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

    return holder->envreverb->getId(&(id_[0]));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_hasControlImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jbooleanArray hasControl) noexcept
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

    int result = holder->envreverb->hasControl(&value);

    hasControl_[0] = (value) ? JNI_TRUE : JNI_FALSE;

    return result;
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getDecayHFRatioImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray decayHFRatio) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array decayHFRatio_(env, decayHFRatio);

    if (!decayHFRatio_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getDecayHFRatio(&(decayHFRatio_[0]));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getDecayTimeImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jintArray decayTime) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array decayTime_(env, decayTime);

    if (!decayTime_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getDecayTime(reinterpret_cast<uint32_t *>(&(decayTime_[0])));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getDensityImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray density) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array density_(env, density);

    if (!density_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getDensity(&(density_[0]));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getDiffusionImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray diffusion) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array diffusion_(env, diffusion);

    if (!diffusion_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getDiffusion(&(diffusion_[0]));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getPropertiesImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jintArray settings) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array settings_(env, settings);

    if (!settings_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    oslmp::OpenSLMediaPlayerEnvironmentalReverb::Settings tmp;

    const int result = holder->envreverb->getProperties(&tmp);

    settings_[0] = tmp.roomLevel & 0xffff;
    settings_[1] = tmp.roomHFLevel & 0xffff;
    settings_[2] = tmp.decayTime;
    settings_[3] = tmp.decayHFRatio & 0xffff;
    settings_[4] = tmp.reflectionsLevel & 0xffff;
    settings_[5] = tmp.reflectionsDelay;
    settings_[6] = tmp.reverbLevel & 0xffff;
    settings_[7] = tmp.reverbDelay;
    settings_[8] = tmp.diffusion & 0xffff;
    settings_[9] = tmp.density & 0xffff;

    return result;
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getReflectionsDelayImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jintArray reflectionsDelay) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array reflectionsDelay_(env, reflectionsDelay);

    if (!reflectionsDelay_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getReflectionsDelay(reinterpret_cast<uint32_t *>(&(reflectionsDelay_[0])));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getReflectionsLevelImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray reflectionsLevel) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array reflectionsLevel_(env, reflectionsLevel);

    if (!reflectionsLevel_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getReflectionsLevel(&(reflectionsLevel_[0]));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getReverbDelayImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jintArray reverbDelay) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jint_array reverbDelay_(env, reverbDelay);

    if (!reverbDelay_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getReverbDelay(reinterpret_cast<uint32_t *>(&(reverbDelay_[0])));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getReverbLevelImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray reverbLevel) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array reverbLevel_(env, reverbLevel);

    if (!reverbLevel_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getReverbLevel(&(reverbLevel_[0]));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getRoomHFLevelImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshortArray roomHF) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array roomHF_(env, roomHF);

    if (!roomHF_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getRoomHFLevel(&(roomHF_[0]));
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_getRoomLevelImplNative(JNIEnv *env, jclass clazz,
                                                                                              jlong handle,
                                                                                              jshortArray room) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    jshort_array room_(env, room);

    if (!room_) {
        return OSLMP_RESULT_ERROR;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->getRoomLevel(&(room_[0]));
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setDecayHFRatioImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshort decayHFRatio) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setDecayHFRatio(decayHFRatio);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setDecayTimeImplNative(JNIEnv *env, jclass clazz,
                                                                                              jlong handle,
                                                                                              jint decayTime) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setDecayTime(decayTime);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setDensityImplNative(JNIEnv *env, jclass clazz,
                                                                                            jlong handle,
                                                                                            jshort density) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setDensity(density);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setDiffusionImplNative(JNIEnv *env, jclass clazz,
                                                                                              jlong handle,
                                                                                              jshort diffusion) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setDiffusion(diffusion);
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setPropertiesImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jintArray settings) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    const_jint_array settings_(env, settings);

    if (!settings_) {
        return OSLMP_RESULT_ERROR;
    }

    oslmp::OpenSLMediaPlayerEnvironmentalReverb::Settings tmp;

    tmp.roomLevel = (int16_t)(settings_[0] & 0xffff);
    tmp.roomHFLevel = (int16_t)(settings_[1] & 0xffff);
    tmp.decayTime = (uint32_t)(settings_[2]);
    tmp.decayHFRatio = (int16_t)(settings_[3] & 0xffff);
    tmp.reflectionsLevel = (int16_t)(settings_[4] & 0xffff);
    tmp.reflectionsDelay = (uint32_t)(settings_[5]);
    tmp.reverbLevel = (int16_t)(settings_[6] & 0xffff);
    tmp.reverbDelay = (uint32_t)(settings_[7]);
    tmp.diffusion = (int16_t)(settings_[8] & 0xffff);
    tmp.density = (int16_t)(settings_[9] & 0xffff);

    return holder->envreverb->setProperties(&tmp);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setReflectionsDelayImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jint reflectionsDelay) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setReflectionsDelay(reflectionsDelay);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setReflectionsLevelImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshort reflectionsLevel) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setReflectionsLevel(reflectionsLevel);
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setReverbDelayImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jint reverbDelay) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setReverbDelay(reverbDelay);
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setReverbLevelImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshort reverbLevel) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setReverbLevel(reverbLevel);
}

JNIEXPORT jint JNICALL Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setRoomHFLevelImplNative(
    JNIEnv *env, jclass clazz, jlong handle, jshort roomHF) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setRoomHFLevel(roomHF);
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_media_opensl_audiofx_OpenSLEnvironmentalReverb_setRoomLevelImplNative(JNIEnv *env, jclass clazz,
                                                                                              jlong handle,
                                                                                              jshort room) noexcept
{
    if (!handle) {
        return OSLMP_RESULT_INVALID_HANDLE;
    }

    Holder *holder = Holder::fromJniHandle(handle);

    return holder->envreverb->setRoomLevel(room);
}

#ifdef __cplusplus
}
#endif
