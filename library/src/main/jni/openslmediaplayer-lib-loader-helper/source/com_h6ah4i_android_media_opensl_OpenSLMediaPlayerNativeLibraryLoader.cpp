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
#include <cpu-features.h>


#ifdef __cplusplus
extern "C" {
#endif


JNIEXPORT jboolean JNICALL
Java_com_h6ah4i_android_media_opensl_OpenSLMediaPlayerNativeLibraryLoader_checkIsNeonDisabledLibRequired(
    JNIEnv *env, jclass clazz) noexcept
{
    const AndroidCpuFamily cpu_family = android_getCpuFamily();
    const uint64_t cpu_features = android_getCpuFeatures();

    const bool is_arm_v7a =
            (cpu_family == ANDROID_CPU_FAMILY_ARM) &&
            ((cpu_features & ANDROID_CPU_ARM_FEATURE_ARMv7) != 0);

    if (!is_arm_v7a) {
        return false;
    }

    const bool support_neon = ((cpu_features & ANDROID_CPU_ARM_FEATURE_NEON) != 0);

    if (support_neon) {
        return false;
    }

    return true;
}

#ifdef __cplusplus
}
#endif
