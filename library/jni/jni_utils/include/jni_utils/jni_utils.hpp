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

#ifndef JNI_UTILS_HPP_
#define JNI_UTILS_HPP_

#include <cxxporthelper/cstddef>
#include <cxxporthelper/type_traits>

#include <jni.h>

#ifdef __cplusplus

namespace jref_type {

typedef enum {
    no_reference,
    local_reference,
    local_reference_explicit_delete,
    new_local_reference,
    global_reference,
    weak_global_reference,
} jref_type_t;

} // namespace jref_type

//
// jlocal_ref_wrapper
//
template <typename T>
class jlocal_ref_wrapper {
public:
    typedef jref_type::jref_type_t ref_type_t;

    jlocal_ref_wrapper()
        : env_(0), ref_(0), ref_type_(jref_type::no_reference) {
    }

    virtual ~jlocal_ref_wrapper() {
        release();
    }

    jlocal_ref_wrapper &assign(JNIEnv *env, T const &ref, ref_type_t ref_type) noexcept {
        release();

        if (ref && env) {
            env_ = env;
            ref_type_ = ref_type;

            switch (ref_type) {
            case jref_type::no_reference:
                break;
            case jref_type::local_reference:
            case jref_type::local_reference_explicit_delete:
                ref_ = ref;
                break;
            case jref_type::new_local_reference:
                ref_ = reinterpret_cast<T>(env->NewLocalRef(ref));
                break;
            default:
                break;
            }
        }

        return *this;
    }

    void release() noexcept {
        if (ref_) {
            switch (ref_type_) {
            case jref_type::no_reference:
            case jref_type::local_reference:
                break;
            case jref_type::local_reference_explicit_delete:
            case jref_type::new_local_reference:
                env_->DeleteLocalRef(ref_);
                break;
            default:
                break;
            }
        }
        clear();
    }

    jlocal_ref_wrapper &move(jlocal_ref_wrapper &&from) noexcept {
        release();

        this->env_ = from.env_;
        this->ref_ = from.ref_;
        this->ref_type_ = from.ref_type_;

        from.clear();

        return *this;
    }

    T detach() noexcept {
        T ref = ref_;
        clear();
        return ref;
    }

    ref_type_t ref_type() const noexcept {
        return ref_type_;
    }

    T& operator ()() noexcept {
        return ref_;
    }

    const T& operator ()() const noexcept {
        return ref_;
    }

    operator bool() const noexcept {
        return (!!ref_);
    }

private:
    // inhibit copy operations
    jlocal_ref_wrapper(const jlocal_ref_wrapper &) = delete;
    jlocal_ref_wrapper& operator = (const jlocal_ref_wrapper &) = delete;

    void clear() noexcept {
        ref_ = 0;
        env_ = 0;
        ref_type_ = jref_type::no_reference;
    }

protected:
    JNIEnv *env() const noexcept {
        return env_;
    }

private:
    JNIEnv *env_;
    T ref_;
    ref_type_t ref_type_;
};

//
// jglobal_ref_wrapper
//
template <typename T>
class jglobal_ref_wrapper {
public:
    typedef jref_type::jref_type_t ref_type_t;

    jglobal_ref_wrapper()
        : ref_(0), ref_type_(jref_type::no_reference) {
    }

    virtual ~jglobal_ref_wrapper() {
    }

    jglobal_ref_wrapper &assign(JNIEnv *env, const T &ref, ref_type_t ref_type) {
        release(env);

        if (ref && env) {
            ref_type_ = ref_type;

            switch (ref_type) {
            case jref_type::no_reference:
                break;
            case jref_type::global_reference:
                ref_ = reinterpret_cast<T>(env->NewGlobalRef(ref));
                break;
            default:
                break;
            }
        }

        return *this;
    }

    void release(JNIEnv *env) noexcept {
        if (ref_ && env) {
            switch (ref_type_) {
            case jref_type::no_reference:
                break;
            case jref_type::global_reference:
                env->DeleteGlobalRef(ref_);
                break;
            default:
                break;
            }
        }
        clear();
    }

    jglobal_ref_wrapper &move(JNIEnv *env, jglobal_ref_wrapper &&from) noexcept {
        release(env);

        this->ref_ = from.ref_;
        this->ref_type_ = from.ref_type_;

        from.clear();

        return *this;
    }

    ref_type_t ref_type() const noexcept {
        return ref_type_;
    }

    T& operator ()() noexcept {
        return ref_;
    }

    const T& operator ()() const noexcept {
        return ref_;
    }

    explicit operator bool() const noexcept {
        return ref_;
    }

private:
    // inhibit copy operations
    jglobal_ref_wrapper(const jglobal_ref_wrapper &) = delete;
    jglobal_ref_wrapper& operator = (const jglobal_ref_wrapper &) = delete;

    void clear() noexcept {
        ref_ = 0;
        ref_type_ = jref_type::no_reference;
    }

private:
    T ref_;
    ref_type_t ref_type_;
};

//
// jweak_ref_wrapper
//
template <typename T>
class jweak_ref_wrapper {
public:
    typedef jref_type::jref_type_t ref_type_t;

    jweak_ref_wrapper()
        : ref_(0), ref_type_(jref_type::no_reference) {
    }

    virtual ~jweak_ref_wrapper() {
    }

    jweak_ref_wrapper &assign(JNIEnv *env, const T &ref, ref_type_t ref_type) {
        release(env);

        if (ref && env) {
            ref_type_ = ref_type;

            switch (ref_type) {
            case jref_type::no_reference:
                break;
            case jref_type::weak_global_reference:
                ref_ = reinterpret_cast<T>(env->NewWeakGlobalRef(ref));
                break;
            default:
                break;
            }
        }

        return *this;
    }

    void release(JNIEnv *env) noexcept {
        if (ref_ && env) {
            switch (ref_type_) {
            case jref_type::no_reference:
                break;
            case jref_type::weak_global_reference:
                env->DeleteWeakGlobalRef(ref_);
                break;
            default:
                break;
            }
        }
        clear();
    }

    bool lock(JNIEnv *env, jlocal_ref_wrapper<T> &local_ref) noexcept {
        local_ref.assign(env, ref_, jlocal_ref_wrapper<T>::new_local_reference);
        return local_ref();
    }

    bool lock(JNIEnv *env, jglobal_ref_wrapper<T> &local_ref) noexcept {
        local_ref.assign(env, ref_, jglobal_ref_wrapper<T>::global_ref);
        return local_ref();
    }

    jweak_ref_wrapper &move(JNIEnv *env, jweak_ref_wrapper &&from) noexcept {
        release(env);

        this->ref_ = from.ref_;
        this->ref_type_ = from.ref_type_;

        from.clear();

        return *this;
    }

    ref_type_t ref_type() const noexcept {
        return ref_type_;
    }

    T& operator ()() noexcept {
        return ref_;
    }

    const T& operator ()() const noexcept {
        return ref_;
    }

private:
    // inhibit copy operations
    jweak_ref_wrapper(const jweak_ref_wrapper &) = delete;
    jweak_ref_wrapper& operator = (const jweak_ref_wrapper &) = delete;

    void clear() noexcept {
        ref_ = 0;
        ref_type_ = jref_type::no_reference;
    }

private:
    T ref_;
    ref_type_t ref_type_;
};

//
// jclass_local_ref_wrapper
//
class jclass_local_ref_wrapper : public jlocal_ref_wrapper<jclass>
{
public:
    jclass_local_ref_wrapper &find_class(JNIEnv *env, const char *class_name, ref_type_t ref_type) noexcept {
        jclass local_ref = 0;

        if (env) {
            local_ref = env->FindClass(class_name);
        }

        jlocal_ref_wrapper<jclass>::assign(env, local_ref, ref_type);

        return *this;
    }

    jclass_local_ref_wrapper &get_class(JNIEnv *env, jobject obj, ref_type_t ref_type) noexcept {
        jclass local_ref = 0;

        if (env) {
            local_ref = env->GetObjectClass(obj);
        }

        jlocal_ref_wrapper<jclass>::assign(env, local_ref, ref_type);

        return *this;
    }
};

//
// jarray_accessor
//
template<typename T, typename TArray>
class jarray_accessor {
private:
    jarray_accessor() = delete;  // inhibit instantiation
};

#define DECL_SPECIALIZED_JARRAY_ACCESSOR(Type_, jtype_)                                     \
    template<typename T>                                                                    \
    class jarray_accessor<T, jtype_ ## Array> {                                             \
    public:                                                                                 \
        static T* get_carray(                                                               \
                JNIEnv *env, jtype_ ## Array array,                                         \
                jboolean *isCopy) noexcept{                                                 \
            return env->Get ## Type_ ## ArrayElements(array, isCopy);                       \
        }                                                                                   \
                                                                                            \
        static void release_carray(                                                         \
                JNIEnv *env, jtype_ ## Array array,                                         \
                const T *carray, jint mode) noexcept {                                      \
            env->Release ## Type_ ## ArrayElements(                                         \
                array, const_cast<typename std::remove_const<T>::type *>(carray), mode);    \
        }                                                                                   \
    };

DECL_SPECIALIZED_JARRAY_ACCESSOR(Boolean, jboolean) // jarray_accessor<T, jbooleanArray>
DECL_SPECIALIZED_JARRAY_ACCESSOR(Byte, jbyte)       // jarray_accessor<T, jbyteArray>
DECL_SPECIALIZED_JARRAY_ACCESSOR(Char, jchar)       // jarray_accessor<T, jcharArray>
DECL_SPECIALIZED_JARRAY_ACCESSOR(Short, jshort)     // jarray_accessor<T, jshortArray>
DECL_SPECIALIZED_JARRAY_ACCESSOR(Int, jint)         // jarray_accessor<T, jintArray>
DECL_SPECIALIZED_JARRAY_ACCESSOR(Long, jlong)       // jarray_accessor<T, jlongArray>
DECL_SPECIALIZED_JARRAY_ACCESSOR(Float, jfloat)     // jarray_accessor<T, jfloatArray>
DECL_SPECIALIZED_JARRAY_ACCESSOR(Double, jdouble)   // jarray_accessor<T, jdoubleArray>

//
// jcritical_array_accessor
//
template<typename T, typename TArray>
class jcritical_array_accessor {
private:
    jcritical_array_accessor() = delete;  // inhibit instantiation

public:
    static T* get_carray(
        JNIEnv *env, TArray array, jboolean *isCopy) noexcept {
        return reinterpret_cast<T *>(
            env->GetPrimitiveArrayCritical(array, isCopy));
    }

    static void release_carray(
        JNIEnv *env, TArray array, T *carray, jint mode) noexcept {
        env->ReleasePrimitiveArrayCritical(
            array,
            const_cast<void *>(reinterpret_cast<const void *>(carray)),
            mode);
    }
};

//
// jarray_wrapper
//
template<class TArrayAccessor, typename T, typename TArray, int ReleaseMode>
class jarray_wrapper {
    typedef jarray_wrapper<TArrayAccessor, T, TArray, ReleaseMode> self_t;

public:
    jarray_wrapper(JNIEnv *env, TArray array)
        :
        env_(env), array_(array),
        release_mode_(ReleaseMode),
        ptr_(0), length_(0), is_copy_(JNI_FALSE)
    {
        if (!(env_ && array_)) {
            return;
        }

        if (env_->ExceptionCheck()) {
            return;
        }

        ptr_ = TArrayAccessor::get_carray(env_, array_, &is_copy_);

        if (env_->ExceptionCheck()) {
            ptr_ = 0;
            return;
        }

        length_ = static_cast<size_t>(
                      env_->GetArrayLength(array));
    }

    ~jarray_wrapper() {
        if (env_ && ptr_ && array_) {
            TArrayAccessor::release_carray(env_, array_, ptr_, release_mode_);
        }

        array_ = 0;
        env_ = 0;
        ptr_ = 0;
    }

    T* data() const noexcept {
        return ptr_;
    }

    size_t length() const noexcept {
        return length_;
    }

    operator bool() const noexcept {
        return (ptr_ != 0);
    }

    T& operator [](size_t index) noexcept {
        return ptr_[index];
    }

    const T& operator [](size_t index) const noexcept {
        return ptr_[index];
    }

    const bool is_copy() const noexcept {
        return (is_copy_ == JNI_TRUE);
    }

    const jint release_mode() const noexcept {
        return release_mode_;
    }

private:
    // inhibit copy operations
    jarray_wrapper(const self_t &) = delete;
    self_t& operator = (const self_t &) = delete;

private:
    JNIEnv *env_;
    TArray array_;
    jint release_mode_;
    T *ptr_;
    size_t length_;
    jboolean is_copy_;
};

// helper macros
#define TYPEDEF_JARRAY_WRAPPER(jtype_)                          \
    typedef jarray_wrapper<                                     \
            jarray_accessor<jtype_, jtype_ ## Array>,           \
            jtype_, jtype_ ## Array, 0>                         \
            jtype_ ## _array;

#define TYPEDEF_READONLY_JARRAY_WRAPPER(jtype_)                 \
    typedef jarray_wrapper<                                     \
            jarray_accessor<const jtype_, jtype_ ## Array>,     \
            const jtype_, jtype_ ## Array, JNI_ABORT>           \
            const_ ## jtype_ ## _array;

#define TYPEDEF_CRITICAL_JARRAY_WRAPPER(jtype_)                 \
    typedef jarray_wrapper<                                     \
            jarray_accessor<jtype_, jtype_ ## Array>,           \
            jtype_, jtype_ ## Array, 0>                         \
            jtype_ ## _critical_array;

#define TYPEDEF_CRITICAL_READONLY_JARRAY_WRAPPER(jtype_)        \
    typedef jarray_wrapper<                                     \
            jarray_accessor<const jtype_, jtype_ ## Array>,     \
            const jtype_, jtype_ ## Array, JNI_ABORT>           \
            const_ ## jtype_ ## _critical_array;

// Non-critical, read/write access
TYPEDEF_JARRAY_WRAPPER(jboolean)                    // jboolean_array
TYPEDEF_JARRAY_WRAPPER(jbyte)                       // jbyte_array
TYPEDEF_JARRAY_WRAPPER(jchar)                       // jchar_array
TYPEDEF_JARRAY_WRAPPER(jshort)                      // jshort_array
TYPEDEF_JARRAY_WRAPPER(jint)                        // jint_array
TYPEDEF_JARRAY_WRAPPER(jlong)                       // jlong_array
TYPEDEF_JARRAY_WRAPPER(jfloat)                      // jfloat_array
TYPEDEF_JARRAY_WRAPPER(jdouble)                     // jdouble_array

// Non-critical, read-only access
TYPEDEF_READONLY_JARRAY_WRAPPER(jbyte)              // const_jbyte_array
TYPEDEF_READONLY_JARRAY_WRAPPER(jchar)              // const_jchar_array
TYPEDEF_READONLY_JARRAY_WRAPPER(jshort)             // const_jshort_array
TYPEDEF_READONLY_JARRAY_WRAPPER(jint)               // const_jint_array
TYPEDEF_READONLY_JARRAY_WRAPPER(jlong)              // const_jlong_array
TYPEDEF_READONLY_JARRAY_WRAPPER(jfloat)             // const_jfloat_array
TYPEDEF_READONLY_JARRAY_WRAPPER(jdouble)            // const_jdouble_array

// Critical, read/write access
TYPEDEF_CRITICAL_JARRAY_WRAPPER(jboolean)           // jboolean_critical_array
TYPEDEF_CRITICAL_JARRAY_WRAPPER(jbyte)              // jbyte_critical_array
TYPEDEF_CRITICAL_JARRAY_WRAPPER(jchar)              // jchar_critical_array
TYPEDEF_CRITICAL_JARRAY_WRAPPER(jshort)             // jshort_critical_array
TYPEDEF_CRITICAL_JARRAY_WRAPPER(jint)               // jint_critical_array
TYPEDEF_CRITICAL_JARRAY_WRAPPER(jlong)              // jlong_critical_array
TYPEDEF_CRITICAL_JARRAY_WRAPPER(jfloat)             // jfloat_critical_array
TYPEDEF_CRITICAL_JARRAY_WRAPPER(jdouble)            // jdouble_critical_array

// Critical, read-only access
TYPEDEF_CRITICAL_READONLY_JARRAY_WRAPPER(jbyte)     // const_jbyte_critical_array
TYPEDEF_CRITICAL_READONLY_JARRAY_WRAPPER(jchar)     // const_jchar_critical_array
TYPEDEF_CRITICAL_READONLY_JARRAY_WRAPPER(jshort)    // const_jshort_critical_array
TYPEDEF_CRITICAL_READONLY_JARRAY_WRAPPER(jint)      // const_jint_critical_array
TYPEDEF_CRITICAL_READONLY_JARRAY_WRAPPER(jlong)     // const_jlong_critical_array
TYPEDEF_CRITICAL_READONLY_JARRAY_WRAPPER(jfloat)    // const_jfloat_critical_array
TYPEDEF_CRITICAL_READONLY_JARRAY_WRAPPER(jdouble)   // const_jdouble_critical_array

//
// jstring_wrapper
//
class jstring_wrapper {
public:
    jstring_wrapper(JNIEnv *env, jstring string)
        :
        env_(env), string_(string),
        utf_(0), length_(0), is_copy_(JNI_FALSE)
    {
        if (env_->ExceptionCheck()) {
            return;
        }

        utf_ = env_->GetStringUTFChars(string, &is_copy_);

        if (env_->ExceptionCheck()) {
            utf_ = 0;
            return;
        }

        length_ = static_cast<size_t>(
                      env->GetStringUTFLength(string));
    }

    ~jstring_wrapper() {
        if (env_ && string_ && utf_) {
            env_->ReleaseStringUTFChars(string_, utf_);
        }

        string_ = 0;
        env_ = 0;
        utf_ = 0;
    }

    const char* data() const noexcept {
        return utf_;
    }

    size_t length() const noexcept {
        return length_;
    }

    bool is_copy() const noexcept {
        return (is_copy_ != JNI_FALSE) ? true : false;
    }

    const char& operator [](size_t index) const noexcept {
        return utf_[index];
    }

private:
    // inhibit copy operations
    jstring_wrapper(const jstring_wrapper &) = delete;
    jstring_wrapper& operator = (const jstring_wrapper &) = delete;

private:
    JNIEnv *env_;
    jstring string_;
    const char *utf_;
    size_t length_;
    jboolean is_copy_;
};

#endif

#endif // JNI_UTILS_HPP_
