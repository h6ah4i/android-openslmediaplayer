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

// #define LOG_TAG "OpenSLMediaPlayerContextJNIBinder"

#include "OpenSLMediaPlayerContextJNIBinder.hpp"

#include <loghelper/loghelper.h>

namespace oslmp {
namespace jni {

OpenSLMediaPlayerContextJNIBinder::OpenSLMediaPlayerContextJNIBinder(JNIEnv *env)
    : jvm_(nullptr), env_(nullptr), jvm_attached_(false)
{
    (void)env->GetJavaVM(&jvm_);
}

OpenSLMediaPlayerContextJNIBinder::~OpenSLMediaPlayerContextJNIBinder() { jvm_ = nullptr; }

void OpenSLMediaPlayerContextJNIBinder::onEnterInternalThread(oslmp::OpenSLMediaPlayerContext *context) noexcept
{
    LOGD("onEnterInternalThread()");

    JavaVM *vm = jvm_;
    JNIEnv *env = nullptr;

    if (jvm_ && (vm->AttachCurrentThread(&env, nullptr) == JNI_OK)) {
        jvm_attached_ = true;
        env_ = env;
    } else {
        LOGE("onEnterInternalThread(); failed to attach to VM");
    }
}

void OpenSLMediaPlayerContextJNIBinder::onLeaveInternalThread(oslmp::OpenSLMediaPlayerContext *context) noexcept
{
    LOGD("onLeaveInternalThread()");

    JavaVM *vm = jvm_;

    if (jvm_attached_) {
        (void)vm->DetachCurrentThread();
        jvm_attached_ = false;
    }
    env_ = nullptr;
}

} // namespace jni
} // namespace oslmp
