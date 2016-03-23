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

#ifdef USE_OSLMP_DEBUG_FEATURES
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ATRACE_MESSAGE_LEN 256
#endif

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
typedef local_ref_deleter<std::remove_pointer<jclass>::type> jclass_local_ref_deleter_t;
typedef std::unique_ptr<std::remove_pointer<jclass>::type, jclass_local_ref_deleter_t> jclass_unique_ptr_t;

#ifdef USE_OSLMP_DEBUG_FEATURES
int AndroidHelper::atrace_marker_fd_ = -1;
#endif

void AndroidHelper::init() noexcept
{
#ifdef USE_OSLMP_DEBUG_FEATURES
    atrace_marker_fd_ = ::open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
#endif
}


bool AndroidHelper::setThreadPriority(JNIEnv *env, pid_t tid, int prio) noexcept
{
    jclass_local_ref_deleter_t del_jclass(env);

    if (env->ExceptionCheck()) {
        return false;
    }

    jclass_unique_ptr_t cls_android_os_process(env->FindClass("android/os/Process"), del_jclass);
    jmethodID mid = nullptr;

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

    return setThreadPriority(env.get(), tid, prio);
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

#ifdef USE_OSLMP_DEBUG_FEATURES
void AndroidHelper::traceBeginSection(const char *name) noexcept
{
    if (atrace_marker_fd_ == -1) {
        return;
    }

    char buf[ATRACE_MESSAGE_LEN];
    int len = ::snprintf(buf, ATRACE_MESSAGE_LEN, "B|%d|%s", ::getpid(), name);
    ::write(atrace_marker_fd_, buf, len);
}

void AndroidHelper::traceEndSection() noexcept
{
    if (atrace_marker_fd_ == -1) {
        return;
    }

    char c = 'E';
    ::write(atrace_marker_fd_, &c, 1);
}

void AndroidHelper::traceCounter(const char *name, int32_t value) noexcept
{
    if (atrace_marker_fd_ == -1) {
        return;
    }

    char buf[ATRACE_MESSAGE_LEN];
    int len = ::snprintf(buf, ATRACE_MESSAGE_LEN, "C|%d|%s|%i", ::getpid(), name, value);
    ::write(atrace_marker_fd_, buf, len);
}
#endif


} // namespace impl
} // namespace oslmp
