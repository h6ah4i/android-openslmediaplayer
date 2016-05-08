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

// #define LOG_TAG "OpenSLMediaPlayerJNIBinder"

#include "OpenSLMediaPlayerJNIBinder.hpp"

#include <jni_utils/jni_utils.hpp>
#include <loghelper/loghelper.h>

#include "OpenSLMediaPlayerContextJNIBinder.hpp"

#define JNI_NULL nullptr

#define MESSAGE_NOP 0
#define MESSAGE_ON_COMPLETION 1
#define MESSAGE_ON_PREPARED 2
#define MESSAGE_ON_SEEK_COMPLETE 3
#define MESSAGE_ON_BUFFERING_UPDATE 4
#define MESSAGE_ON_INFO 5
#define MESSAGE_ON_ERROR 6

namespace oslmp {
namespace jni {

OpenSLMediaPlayerJNIBinder::OpenSLMediaPlayerJNIBinder(JNIEnv *env, jclass clazz, jobject weak_thiz)
    : jvm_(nullptr), env_(nullptr), methodIdPostEventFromNative_(0), context_(), player_()
{
    (void)env->GetJavaVM(&jvm_);

    methodIdPostEventFromNative_ =
        env->GetStaticMethodID(clazz, "postEventFromNative", "(Ljava/lang/Object;IIILjava/lang/Object;)V");

    jplayer_class_.assign(env, clazz, jref_type::global_reference);
    jplayer_weak_thiz_.assign(env, weak_thiz, jref_type::global_reference);
}

OpenSLMediaPlayerJNIBinder::~OpenSLMediaPlayerJNIBinder()
{
    unbind();

    JNIEnv *env = nullptr;

    if (jvm_) {
        (void)jvm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    }

    if (env) {
        jplayer_class_.release(env);
        jplayer_weak_thiz_.release(env);
    }

    jvm_ = nullptr;
}

// NOTE:
// This method must be called before the player.initialize() method call
void OpenSLMediaPlayerJNIBinder::bind(const android::sp<oslmp::OpenSLMediaPlayerContext> &context,
                                      const android::sp<OpenSLMediaPlayer> &player) noexcept
{

    context_ = context;
    player_ = player;

    player->setOnCompletionListener(this);
    player->setOnPreparedListener(this);
    player->setOnSeekCompleteListener(this);
    player->setOnBufferingUpdateListener(this);
    player->setOnInfoListener(this);
    player->setOnErrorListener(this);
    player->setInternalThreadEventListener(this);
}

void OpenSLMediaPlayerJNIBinder::unbind() noexcept
{
    android::sp<OpenSLMediaPlayer> player(player_.promote());

    if (!player.get())
        return;

    player->setOnCompletionListener(nullptr);
    player->setOnPreparedListener(nullptr);
    player->setOnSeekCompleteListener(nullptr);
    player->setOnBufferingUpdateListener(nullptr);
    player->setOnInfoListener(nullptr);
    player->setOnErrorListener(nullptr);
    player->setInternalThreadEventListener(nullptr);

    player_.clear();
    context_.clear();
}

void OpenSLMediaPlayerJNIBinder::onEnterInternalThread(OpenSLMediaPlayer *mp) noexcept
{
    LOGD("onEnterInternalThread()");

    android::sp<oslmp::OpenSLMediaPlayerContext> context(context_.promote());

    if (context.get()) {
        oslmp::OpenSLMediaPlayerContext::InternalThreadEventListener *listener;
        OpenSLMediaPlayerContextJNIBinder *binder;

        listener = context->getInternalThreadEventListener();
        binder = dynamic_cast<OpenSLMediaPlayerContextJNIBinder *>(listener);

        env_ = binder->getJNIEnv();
    }
}

void OpenSLMediaPlayerJNIBinder::onLeaveInternalThread(OpenSLMediaPlayer *mp) noexcept
{
    LOGE("onLeaveInternalThread()");

    env_ = nullptr;
}

void OpenSLMediaPlayerJNIBinder::onCompletion(OpenSLMediaPlayer *mp) noexcept
{
    LOGD("OpenSLMediaPlayerJNIBinder::onCompletion");
    postMessageToJava(MESSAGE_ON_COMPLETION, 0, 0, JNI_NULL);
}

void OpenSLMediaPlayerJNIBinder::onPrepared(OpenSLMediaPlayer *mp) noexcept
{
    postMessageToJava(MESSAGE_ON_PREPARED, 0, 0, JNI_NULL);
}

void OpenSLMediaPlayerJNIBinder::onSeekComplete(OpenSLMediaPlayer *mp) noexcept
{
    postMessageToJava(MESSAGE_ON_SEEK_COMPLETE, 0, 0, JNI_NULL);
}

void OpenSLMediaPlayerJNIBinder::onBufferingUpdate(OpenSLMediaPlayer *mp, int32_t percent) noexcept
{
    postMessageToJava(MESSAGE_ON_BUFFERING_UPDATE, percent, 0, JNI_NULL);
}

bool OpenSLMediaPlayerJNIBinder::onInfo(OpenSLMediaPlayer *mp, int32_t what, int32_t extra) noexcept
{
    postMessageToJava(MESSAGE_ON_INFO, what, extra, JNI_NULL);

    return true;
}

bool OpenSLMediaPlayerJNIBinder::onError(OpenSLMediaPlayer *mp, int32_t what, int32_t extra) noexcept
{
    LOGD("OpenSLMediaPlayerJNIBinder::onError");

    postMessageToJava(MESSAGE_ON_ERROR, what, extra, JNI_NULL);

    return true;
}

void OpenSLMediaPlayerJNIBinder::postMessageToJava(int32_t what, int32_t arg1, int32_t arg2, jobject obj) noexcept
{
    LOGD("postMessageToJava(what = %d, arg1 = %d, arg2 = %d, obj = %p)", what, arg1, arg2, obj);

    if (env_ && jplayer_class_() && jplayer_weak_thiz_() && methodIdPostEventFromNative_) {
        // void postEventFromNative(
        //     Object ref, int what, int arg1, int arg2, Object obj);
        env_->CallStaticVoidMethod(jplayer_class_(), methodIdPostEventFromNative_, jplayer_weak_thiz_(), what, arg1,
                                   arg2, obj);
    } else {
        if (!env_) {
            LOGE("env_");
        }
        if (!jplayer_weak_thiz_()) {
            LOGE("jplayer_weak_thiz_");
        }
        if (!methodIdPostEventFromNative_) {
            LOGE("methodIdPostEventFromNative_");
        }
    }
}

} // namespace jni
} // namespace oslmp
