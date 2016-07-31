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

#ifndef OPENSLMEDIAPLAYERCONTEXTJNIBINDER_HPP_
#define OPENSLMEDIAPLAYERCONTEXTJNIBINDER_HPP_

#include <jni.h>
#include <oslmp/OpenSLMediaPlayerContext.hpp>

namespace oslmp {
namespace jni {

class OpenSLMediaPlayerContextJNIBinder : public oslmp::OpenSLMediaPlayerContext::InternalThreadEventListener {
public:
    OpenSLMediaPlayerContextJNIBinder(JNIEnv *env);
    virtual ~OpenSLMediaPlayerContextJNIBinder();

    virtual void onEnterInternalThread(OpenSLMediaPlayerContext *context) noexcept override;
    virtual void onLeaveInternalThread(OpenSLMediaPlayerContext *context) noexcept override;

    // This function returns JNIEnv for internal thread
    JNIEnv *getJNIEnv() const noexcept { return env_; }

private:
    JavaVM *jvm_;
    JNIEnv *env_;
    bool jvm_attached_;
};

} // namespace jni
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERCONTEXTJNIBINDER_HPP_
