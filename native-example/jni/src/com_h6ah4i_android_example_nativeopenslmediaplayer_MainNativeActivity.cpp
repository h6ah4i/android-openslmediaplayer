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
#include <string>

#include "app_engine.hpp"

extern "C" {

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_example_nativeopenslmediaplayer_MainNativeActivity_nativeSetDataSourceAndPrepare(
    JNIEnv *env, jclass clazz, jlong handle, jstring uri) noexcept
{
    std::string uri_str;

    {
        const char *uri_utf;
        jboolean uri_is_copy = JNI_FALSE;
        uri_utf = env->GetStringUTFChars(uri, &uri_is_copy);
        uri_str = uri_utf;
        env->ReleaseStringUTFChars(uri, uri_utf);
    }

    AppCommandPipe *cmd_pipe = AppEngine::sGetAppCommandPipeFromHandle(handle);

    return cmd_pipe->setDataSourceAndPrepare(uri_str.data());
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_example_nativeopenslmediaplayer_MainNativeActivity_nativePlay(JNIEnv *env, jclass clazz,
                                                                                      jlong handle) noexcept
{
    AppCommandPipe *cmd_pipe = AppEngine::sGetAppCommandPipeFromHandle(handle);

    return cmd_pipe->play();
}

JNIEXPORT jint JNICALL
Java_com_h6ah4i_android_example_nativeopenslmediaplayer_MainNativeActivity_nativePause(JNIEnv *env, jclass clazz,
                                                                                       jlong handle) noexcept
{
    AppCommandPipe *cmd_pipe = AppEngine::sGetAppCommandPipeFromHandle(handle);

    return cmd_pipe->pause();
}
}
