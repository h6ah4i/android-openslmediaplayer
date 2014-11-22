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

#ifndef OPENSLMEDIAPLAYERJNIBINDER_HPP_
#define OPENSLMEDIAPLAYERJNIBINDER_HPP_

#include <jni.h>
#include <jni_utils/jni_utils.hpp>
#include <utils/RefBase.h>
#include <oslmp/OpenSLMediaPlayer.hpp>

namespace oslmp {
namespace jni {

class OpenSLMediaPlayerJNIBinder : public virtual android::RefBase,
                                   public oslmp::OpenSLMediaPlayer::InternalThreadEventListener,
                                   public oslmp::OpenSLMediaPlayer::OnCompletionListener,
                                   public oslmp::OpenSLMediaPlayer::OnPreparedListener,
                                   public oslmp::OpenSLMediaPlayer::OnSeekCompleteListener,
                                   public oslmp::OpenSLMediaPlayer::OnBufferingUpdateListener,
                                   public oslmp::OpenSLMediaPlayer::OnInfoListener,
                                   public oslmp::OpenSLMediaPlayer::OnErrorListener {
public:
    OpenSLMediaPlayerJNIBinder(JNIEnv *env, jclass clazz, jobject weak_thiz);
    virtual ~OpenSLMediaPlayerJNIBinder();

    void bind(const android::sp<OpenSLMediaPlayerContext> &context,
              const android::sp<OpenSLMediaPlayer> &player) noexcept;
    void unbind() noexcept;

    // implementations of InternalThreadEventListener
    virtual void onEnterInternalThread(OpenSLMediaPlayer *mp) noexcept override;
    virtual void onLeaveInternalThread(OpenSLMediaPlayer *mp) noexcept override;

    // implementations of OnCompletionListener
    virtual void onCompletion(OpenSLMediaPlayer *mp) noexcept override;

    // implementations of OnPreparedListener
    virtual void onPrepared(OpenSLMediaPlayer *mp) noexcept override;

    // implementations of OnSeekCompleteListener
    virtual void onSeekComplete(OpenSLMediaPlayer *mp) noexcept override;

    // implementations of OnBufferingUpdateListener
    virtual void onBufferingUpdate(OpenSLMediaPlayer *mp, int32_t percent) noexcept override;

    // implementations of OnInfoListener
    virtual bool onInfo(OpenSLMediaPlayer *mp, int32_t what, int32_t extra) noexcept override;

    // implementations of OnErrorListener
    virtual bool onError(OpenSLMediaPlayer *mp, int32_t what, int32_t extra) noexcept override;

private:
    void postMessageToJava(int32_t what, int32_t arg1, int32_t arg2, jobject obj) noexcept;

    // inhibit copy operations
    OpenSLMediaPlayerJNIBinder(const OpenSLMediaPlayerJNIBinder &) = delete;
    OpenSLMediaPlayerJNIBinder &operator=(const OpenSLMediaPlayerJNIBinder &) = delete;

private:
    JavaVM *jvm_;
    JNIEnv *env_;
    jglobal_ref_wrapper<jclass> jplayer_class_;
    jglobal_ref_wrapper<jobject> jplayer_weak_thiz_;
    jmethodID methodIdPostEventFromNative_;
    android::wp<OpenSLMediaPlayerContext> context_;
    android::wp<OpenSLMediaPlayer> player_;
};

} // namespace jni
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERJNIBINDER_HPP_
