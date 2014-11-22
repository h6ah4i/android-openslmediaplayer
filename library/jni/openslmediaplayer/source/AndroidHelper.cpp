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

#include "oslmp/impl/AndroidHelper.hpp"
// #define LOG_TAG "AndroidHelper"

#include <string>
#include <cerrno>

#include <cxxporthelper/memory>

#include <sys/prctl.h>

#include <loghelper/loghelper.h>

namespace oslmp {
namespace impl {

struct jni_env_deleter {
    JavaVM *vm_;

    jni_env_deleter(JavaVM *vm) : vm_(vm) {}

    void operator()(JNIEnv *ptr) const
    {
        if (vm_) {
            vm_->DetachCurrentThread();
        }
    }
};

template <typename T>
struct local_ref_deleter {
    JNIEnv *env_;

    local_ref_deleter(JNIEnv *env) : env_(env) {}

    void operator()(T *ptr) const
    {
        if (env_ && ptr) {
            env_->DeleteLocalRef(ptr);
        }
    }
};

typedef std::unique_ptr<JNIEnv, jni_env_deleter> jni_env_unique_ptr_t;
typedef local_ref_deleter<std::remove_pointer<jclass>::type> jclass_deleter_t;
typedef std::unique_ptr<std::remove_pointer<jclass>::type, jclass_deleter_t> jclass_unique_ptr_t;

bool AndroidHelper::setThreadPriority(JavaVM *vm, pid_t tid, int prio) noexcept
{
    if (!vm) {
        return false;
    }

    JNIEnv *raw_env = nullptr;
    vm->AttachCurrentThread(&raw_env, nullptr);

    jni_env_deleter del_jni_env(vm);
    jni_env_unique_ptr_t env(raw_env, del_jni_env);

    if (!env) {
        return false;
    }

    jclass_deleter_t del_jclass(env.get());

    if (env->ExceptionCheck()) {
        return false;
    }

    jclass_unique_ptr_t cls_android_os_process(nullptr, del_jclass);
    jmethodID mid = nullptr;

    cls_android_os_process.reset(env->FindClass("android/os/Process"));

    if (!cls_android_os_process) {
        return false;
    }

    mid = env->GetStaticMethodID(cls_android_os_process.get(), "setThreadPriority", "(II)V");
    env->CallStaticVoidMethod(cls_android_os_process.get(), mid, static_cast<jint>(tid), static_cast<jint>(prio));

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }

    return true;
}

bool AndroidHelper::setCurrentThreadName(const char *name) noexcept
{
    if (!name) {
        return false;
    }

    std::string sname(name);

    if (sname.length() > 15) {
        return false;
    }

    const int result = ::prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(sname.c_str()), 0, 0, 0);

    if (result != 0) {
        LOGE("setCurrentThreadName()  ::prctl() returns %d (errno = %d)", result, errno);
        return false;
    }

    return true;
}

} // namespace impl
} // namespace oslmp
