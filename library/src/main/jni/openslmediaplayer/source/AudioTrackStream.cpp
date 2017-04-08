//
//    Copyright (C) 2016 Haruki Hasegawa
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

// #define LOG_TAG "AudioTrackStream"

#include "oslmp/impl/AudioTrackStream.hpp"

#include <new>
#include <cxxporthelper/compiler.hpp>
#include <jni_utils/jni_utils.hpp>
#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/impl/AudioTrack.hpp"
#include "oslmp/impl/AudioFormat.hpp"
#include "oslmp/impl/AndroidHelper.hpp"
#include "oslmp/utils/timespec_utils.hpp"

// #define DEBUG_FLOAT_TONE_GEN
#ifdef DEBUG_FLOAT_TONE_GEN
#define TONE_FREQ_MULTIPLE  64
#include <cmath>
#endif

#ifdef USE_OSLMP_DEBUG_FEATURES
#define STREAM_COUNTER_INIT()   int32_t callback_counter_ = 0;
#define STREAM_COUNTER_LOG()    do { callback_counter_ = ((callback_counter_ + 1) & 0xf); ATRACE_COUNTER("AudioTrackStreamCallbackCount", callback_counter_); } while(0)
#else
#define STREAM_COUNTER_INIT()
#define STREAM_COUNTER_LOG()
#endif

namespace oslmp {
namespace impl {

class LocalByteBuffer {
public:
    LocalByteBuffer(JNIEnv *env, void *buffer, size_t size)
        : env_(env), buffer_(buffer), size_(size), bb_(0), m_rewind_(0)
    {
        bb_ = env_->NewDirectByteBuffer(buffer, static_cast<jlong>(size));

        {
            jclass cls = env_->FindClass("java/nio/Buffer");
            m_rewind_ = env_->GetMethodID(cls, "rewind", "()Ljava/nio/Buffer;");
            env_->DeleteLocalRef(cls);
        }
    }

    ~LocalByteBuffer() {
        if (env_ && bb_) {
            env_->DeleteLocalRef(bb_);
        }
    }

    bool is_valid() const noexcept {
        return (bb_ != 0);
    }

    size_t size() const noexcept {
        return size_;
    }

    void rewind() noexcept {
        jobject bb2 = env_->CallObjectMethod(bb_, m_rewind_);
        env_->DeleteLocalRef(bb2);
    }

    jobject get() const noexcept {
        return bb_;
    }

private:
    JNIEnv *env_;
    void *buffer_;
    size_t size_;
    jobject bb_;
    jmethodID m_rewind_;
};

// WORKAROUND: AudioTrack.write() sometimes returns smaller value than specified
// https://github.com/h6ah4i/android-openslmediaplayer/issues/26
class AudioTrackWrapper {
public:
    const int MAX_RETRIES = 10;
    const int RETRY_DELAY_MS = 10;

    AudioTrackWrapper(AudioTrack &track)
            : track_(track),
              ts_sleep_(utils::timespec_utils::add_ms(utils::timespec_utils::ZERO(), RETRY_DELAY_MS)) {
    }

    int32_t getChannelCount() const noexcept {
        return track_.getChannelCount();
    }

    int32_t play(JNIEnv *env) noexcept {
        return track_.play(env);
    }

    int32_t write(JNIEnv *env, jshortArray data, size_t offset, size_t size) noexcept {
        int32_t written = 0;
        int retry = 0;

        while (true) {
            const int32_t ret = track_.write(env, data, (offset + written), (size - written));

            if (ret < 0) {
                return ret;
            } else {
                written += ret;
            }

            if (!((written < size) && (retry < MAX_RETRIES))) {
                break;
            }

            retry += 1;
            LOGD("AudioTrackWrapper - retry[%d] / size: %d / written: %d", retry, size, written);
            retry_delay();
        }

        return written;
    }

    int32_t write(JNIEnv *env, jfloatArray data, size_t offset, size_t size, AudioTrack::write_mode_t mode) noexcept {
        int32_t written = 0;
        int retry = 0;

        while (true) {
            const int32_t ret = track_.write(env, data, (offset + written), (size - written), mode);

            if (ret < 0) {
                return ret;
            } else {
                written += ret;
            }

            if (!((written < size) && (retry < MAX_RETRIES) && (mode == AudioTrack::WRITE_BLOCKING))) {
                break;
            }

            retry += 1;
            LOGD("AudioTrackWrapper - retry[%d] / size: %d / written: %d", retry, size, written);
            retry_delay();
        }

        return written;
    }

    int32_t write(JNIEnv *env, jobject data, size_t size_in_bytes, AudioTrack::write_mode_t mode) noexcept {
        int32_t written = 0;
        int retry = 0;

        while (true) {
            const int32_t ret = track_.write(env, data, (size_in_bytes - written), mode);

            if (ret < 0) {
                return ret;
            } else {
                written += ret;
            }

            if (!((written < size_in_bytes) && (retry < MAX_RETRIES) && (mode == AudioTrack::WRITE_BLOCKING))) {
                break;
            }

            retry += 1;
            LOGD("AudioTrackWrapper - retry[%d] / size: %d / written: %d", retry, size_in_bytes, written);
            retry_delay();
        }

        return written;
    }

private:
    void retry_delay() {
        ::clock_nanosleep(CLOCK_MONOTONIC, 0, &ts_sleep_, nullptr);
    }

private:
    AudioTrack &track_;
    const timespec ts_sleep_;
};

static int translate_audio_track_result(int result) {
    switch (result) {
    case AudioTrack::SUCCESS:
        return OSLMP_RESULT_SUCCESS;
    case AudioTrack::ERROR:
        return OSLMP_RESULT_ERROR;
    case AudioTrack::ERROR_BAD_VALUE:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    case AudioTrack::ERROR_INVALID_OPERATION:
        return OSLMP_RESULT_ILLEGAL_STATE;
    default:    
        return OSLMP_RESULT_INTERNAL_ERROR;
    }
}

AudioTrackStream::AudioTrackStream()
    : vm_(nullptr), track_(nullptr), pt_handle_(0), 
    buffer_size_in_frames_(0), buffer_block_count_(0),
    callback_func_(nullptr), callback_args_(nullptr), stop_request_(false)
{
}

AudioTrackStream::~AudioTrackStream()
{
    if (pt_handle_) {
        stop_request_ = true;
        (void) ::pthread_join(pt_handle_, nullptr);
    }

    JNIEnv *env = getJNIEnv();
    if (env && track_) {
        track_->release(env);
        track_.reset();
    }

    vm_ = nullptr;
}

int AudioTrackStream::init(
    JNIEnv *env, int stream_type, sample_format_type format, 
    uint32_t sample_rate_in_hz, uint32_t num_channels, 
    uint32_t buffer_size_in_frames, uint32_t buffer_block_count) noexcept
{
    JavaVM *vm = nullptr;
    int32_t encoding;

    switch (format) {
    case kAudioSampleFormatType_S16:
        encoding = AudioFormat::ENCODING_PCM_16BIT;
        break;
    case kAudioSampleFormatType_F32:
        encoding = AudioFormat::ENCODING_PCM_FLOAT;
        break;
    default:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (num_channels != 2) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (env->GetJavaVM(&vm) != JNI_OK) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    std::unique_ptr<AudioTrack> track(new(std::nothrow) AudioTrack());

    if (!(track && track->create(
        env, stream_type, sample_rate_in_hz, num_channels, encoding, 
        (buffer_size_in_frames * buffer_block_count), AudioTrack::MODE_STREAM, 0))) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    vm_ = vm;
    track_ = std::move(track);
    buffer_size_in_frames_ = buffer_size_in_frames;
    buffer_block_count_ = buffer_block_count;

    return OSLMP_RESULT_SUCCESS;
}

int AudioTrackStream::start(render_callback_func_t callback, void *args) noexcept
{
    LOGD("AudioTrackStream::start");

    if (!callback) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }
    if (!track_) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }
    if (callback_func_) {
        LOGE("AudioTrackStream::start  --- stream is already active");
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    pthread_t pt_handle;
    int pt_create_result;

    callback_func_ = callback;
    callback_args_ = args;
    pt_create_result = ::pthread_create(&pt_handle, nullptr, &AudioTrackStream::sinkWriterThreadEntryFunc, this);

    if (pt_create_result != 0) {
        callback_func_ = nullptr;
        callback_args_ = nullptr;
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    pt_handle_ = pt_handle;

    return OSLMP_RESULT_SUCCESS;
}

int AudioTrackStream::stop() noexcept
{
    LOGD("AudioTrackStream::stop");
    if (!pt_handle_) {
        return OSLMP_RESULT_SUCCESS;
    }

    stop_request_ = true;
    ::pthread_join(pt_handle_, nullptr);
    pt_handle_ = 0;
    callback_func_ = nullptr;
    callback_args_ = nullptr;
    stop_request_ = false;

    return OSLMP_RESULT_SUCCESS;
}

int32_t AudioTrackStream::getAudioSessionId() noexcept
{
    JNIEnv *env = getJNIEnv();

    if (!(track_ && env)) {
        return 0;
    }

    return track_->getAudioSessionId(env);
}

int32_t AudioTrackStream::setAuxEffectSendLevel(float level) noexcept {
    JNIEnv *env = getJNIEnv();

    if (!(track_ && env)) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    const int result = track_->setAuxEffectSendLevel(env, level);

    return translate_audio_track_result(result);
}

int32_t AudioTrackStream::attachAuxEffect(int effect_id) noexcept {
    JNIEnv *env = getJNIEnv();

    if (!(track_ && env)) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    const int result = track_->attachAuxEffect(env, effect_id);

    return translate_audio_track_result(result);
}

JNIEnv *AudioTrackStream::getJNIEnv() noexcept {
    JNIEnv *env = nullptr;

    if (vm_ && vm_->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK) {
        return env;
    } else {
        return nullptr;
    }
}

void* AudioTrackStream::sinkWriterThreadEntryFunc(void *args) noexcept
{
    LOGD("AudioTrackStream::sinkWriterThreadEntryFunc");

    AudioTrackStream *thiz = static_cast<AudioTrackStream*>(args);
    JNIEnv *env = nullptr;

    if (thiz->vm_->AttachCurrentThread(&env, nullptr) == JNI_OK) {
        // set thread priority
        // AndroidHelper::setThreadPriority(env, 0, ANDROID_THREAD_PRIORITY_AUDIO + ANDROID_THREAD_PRIORITY_LESS_FAVORABLE);
        AndroidHelper::setThreadPriority(env, 0, ANDROID_THREAD_PRIORITY_MORE_FAVORABLE * 2);

        // set thread name
        AndroidHelper::setCurrentThreadName("AudioTrackStrm");

        thiz->sinkWriterThreadProcess(env);

        thiz->vm_->DetachCurrentThread();
        env = nullptr;
    }

    LOGD("AudioTrackStream::sinkWriterThreadEntryFunc - Exit");

    return nullptr;
}

void AudioTrackStream::sinkWriterThreadProcess(JNIEnv *env) noexcept
{
    LOGD("AudioTrackStream::sinkWriterThreadProcess");

    ATRACE_BEGIN_SECTION("AudioTrackStream");

    const int32_t format = track_->getAudioFormat();

    int32_t play_result = AudioTrack::ERROR;
    const bool supports_bb = track_->supportsByteBufferMethods();

    switch (format) {
        case AudioFormat::ENCODING_PCM_16BIT:
            if (supports_bb) {
                play_result = sinkWriterThreadLoopS16ByteBuffer(env);
            } else {
                play_result = sinkWriterThreadLoopS16(env);
            }
            break;
        case AudioFormat::ENCODING_PCM_FLOAT:
            if (supports_bb) {
                play_result = sinkWriterThreadLoopFloatByteBuffer(env);
            } else {
                play_result = sinkWriterThreadLoopFloat(env);
            }
            break;
        default:
            break;
    }

    if (play_result == AudioTrack::SUCCESS) {
        track_->pause(env);
        track_->flush(env);
    }

    ATRACE_END_SECTION();
}

int32_t AudioTrackStream::sinkWriterThreadLoopS16(JNIEnv *env) noexcept
{
    LOGD("AudioTrackStream::sinkWriterThreadLoopS16");

    int32_t play_result = AudioTrack::ERROR;
    AudioTrackWrapper trackWrapper(*track_);
    const sample_format_type format = kAudioSampleFormatType_S16;
    const int32_t num_channels = trackWrapper.getChannelCount();
    const int32_t buffer_size_in_frames = buffer_size_in_frames_;
    const int32_t num_data_write = num_channels * buffer_size_in_frames;

    jshortArray buffer = env->NewShortArray(num_channels * buffer_size_in_frames);

    if (!buffer) {
        return play_result;
    }

    jlocal_ref_wrapper<jshortArray> buffer_ref;
    buffer_ref.assign(env, buffer, jref_type::local_reference);

    const int32_t write_result = trackWrapper.write(env, buffer, 0, num_data_write);
    if (write_result != num_data_write) {
        LOGW("AudioTrack::write() returns unexpected result  (actual: %d, expected: %d)", write_result, num_data_write);
        return play_result;
    }

    play_result = trackWrapper.play(env);
    if (play_result != AudioTrack::SUCCESS) {
        return play_result;
    }

    STREAM_COUNTER_INIT();

    while (CXXPH_UNLIKELY(!stop_request_)) {
        STREAM_COUNTER_LOG();
        {
            jshort_array buffer_array(env, buffer);
            (*callback_func_)(buffer_array.data(), format, num_channels, buffer_size_in_frames, callback_args_);
        }

        const int32_t write_result = trackWrapper.write(env, buffer, 0, num_data_write);

        if (write_result != num_data_write) {
            LOGW("AudioTrack::write() returns unexpected result  (actual: %d, expected: %d)", write_result, num_data_write);
            break;
        }
    }

    return play_result;
}

int32_t AudioTrackStream::sinkWriterThreadLoopFloat(JNIEnv *env) noexcept
{
    LOGD("AudioTrackStream::sinkWriterThreadLoopFloat");

    int32_t play_result = AudioTrack::ERROR;
    AudioTrackWrapper trackWrapper(*track_);
    const sample_format_type format = kAudioSampleFormatType_F32;
    const int32_t num_channels = trackWrapper.getChannelCount();
    const int32_t buffer_size_in_frames = buffer_size_in_frames_;
    const int32_t num_data_write = num_channels * buffer_size_in_frames;

    jfloatArray buffer = env->NewFloatArray(num_channels * buffer_size_in_frames);

    if (!buffer) {
        return play_result;
    }

    jlocal_ref_wrapper<jfloatArray> buffer_ref;
    buffer_ref.assign(env, buffer, jref_type::local_reference);

#ifdef DEBUG_FLOAT_TONE_GEN
    {
        jfloat_array buffer_array(env, buffer);

        double delta = 2 * M_PI * TONE_FREQ_MULTIPLE / buffer_size_in_frames;

        float *p = buffer_array.data();
        for (int i = 0; i < buffer_size_in_frames_; ++i) {
            p[2 * i + 0] = p[2 * i + 1] = static_cast<float>(std::sin(delta * i));
        }
    }
#endif

    const int32_t write_result = trackWrapper.write(env, buffer, 0, num_data_write, AudioTrack::WRITE_BLOCKING);
    if (write_result != num_data_write) {
        LOGW("AudioTrack::write() returns unexpected result  (actual: %d, expected: %d)", write_result, num_data_write);
        return play_result;
    }

    play_result = trackWrapper.play(env);
    if (play_result != AudioTrack::SUCCESS) {
        return play_result;
    }

    STREAM_COUNTER_INIT();

    while (CXXPH_UNLIKELY(!stop_request_)) {
        STREAM_COUNTER_LOG();

#ifndef DEBUG_FLOAT_TONE_GEN
        {
            jfloat_array buffer_array(env, buffer);
            (void) (*callback_func_)(buffer_array.data(), format, num_channels, buffer_size_in_frames, callback_args_);
        }
#endif

        const int32_t write_result = trackWrapper.write(env, buffer, 0, num_data_write, AudioTrack::WRITE_BLOCKING);

        if (write_result != num_data_write) {
            LOGW("AudioTrack::write() returns unexpected result  (actual: %d, expected: %d)", write_result, num_data_write);
            break;
        }
    }

    return play_result;
}

int32_t AudioTrackStream::sinkWriterThreadLoopS16ByteBuffer(JNIEnv *env) noexcept
{
    LOGD("AudioTrackStream::sinkWriterThreadLoopS16ByteBuffer");

    int32_t play_result = AudioTrack::ERROR;
    AudioTrackWrapper trackWrapper(*track_);
    const sample_format_type format = kAudioSampleFormatType_S16;
    const int32_t num_channels = trackWrapper.getChannelCount();
    const int32_t buffer_size_in_frames = buffer_size_in_frames_;
    const int32_t num_data_write = num_channels * buffer_size_in_frames;
    const size_t num_data_write_in_bytes = num_data_write * getBytesPerSample(format);
    std::unique_ptr<int16_t[]> buffer(new int16_t[num_data_write]());

    LocalByteBuffer bb(env, buffer.get(), num_data_write_in_bytes);

    if (!bb.is_valid()) {
        return play_result;
    }

    const int32_t write_result = trackWrapper.write(env, bb.get(), bb.size(), AudioTrack::WRITE_BLOCKING);

    if (write_result != bb.size()) {
        LOGW("AudioTrack::write() returns unexpected result  (actual: %d, expected: %d)", write_result, bb.size());
        return play_result;
    }

    bb.rewind();

    play_result = trackWrapper.play(env);
    if (play_result != AudioTrack::SUCCESS) {
        return play_result;
    }

    STREAM_COUNTER_INIT();

    while (CXXPH_UNLIKELY(!stop_request_)) {
        STREAM_COUNTER_LOG();
        (*callback_func_)(buffer.get(), format, num_channels, buffer_size_in_frames, callback_args_);
        const int32_t write_result = trackWrapper.write(env, bb.get(), bb.size(), AudioTrack::WRITE_BLOCKING);

        if (write_result != bb.size()) {
            LOGW("AudioTrack::write() returns unexpected result  (actual: %d, expected: %d)", write_result, bb.size());
            break;
        }

        bb.rewind();
    }

    return play_result;
}

int32_t AudioTrackStream::sinkWriterThreadLoopFloatByteBuffer(JNIEnv *env) noexcept
{
    LOGD("AudioTrackStream::sinkWriterThreadLoopFloatByteBuffer");

    int32_t play_result = AudioTrack::ERROR;
    AudioTrackWrapper trackWrapper(*track_);
    const sample_format_type format = kAudioSampleFormatType_F32;
    const int32_t num_channels = trackWrapper.getChannelCount();
    const int32_t buffer_size_in_frames = buffer_size_in_frames_;
    const int32_t num_data_write = num_channels * buffer_size_in_frames;
    const size_t num_data_write_in_bytes = num_data_write * getBytesPerSample(format);
    std::unique_ptr<float[]> buffer(new float[num_data_write]());

    LocalByteBuffer bb(env, buffer.get(), num_data_write_in_bytes);

    if (!bb.is_valid()) {
        return play_result;
    }

#ifdef DEBUG_FLOAT_TONE_GEN
    {
        double delta = 2 * M_PI * TONE_FREQ_MULTIPLE / buffer_size_in_frames;

        for (int i = 0; i < buffer_size_in_frames_; ++i) {
            buffer[2 * i + 0] = buffer[2 * i + 1] = static_cast<float>(std::sin(delta * i));
        }
    }
#endif

    const int32_t write_result = trackWrapper.write(env, bb.get(), bb.size(), AudioTrack::WRITE_BLOCKING);

    if (write_result != bb.size()) {
        LOGW("AudioTrack::write() returns unexpected result  (actual: %d, expected: %d)", write_result, bb.size());
        return play_result;
    }

    bb.rewind();

    play_result = trackWrapper.play(env);
    if (play_result != AudioTrack::SUCCESS) {
        return play_result;
    }

    STREAM_COUNTER_INIT();

    while (CXXPH_UNLIKELY(!stop_request_)) {
        STREAM_COUNTER_LOG();
#ifndef DEBUG_FLOAT_TONE_GEN
        (*callback_func_)(buffer.get(), format, num_channels, buffer_size_in_frames, callback_args_);
#endif

        const int32_t write_result = trackWrapper.write(env, bb.get(), bb.size(), AudioTrack::WRITE_BLOCKING);

        if (write_result != bb.size()) {
            LOGW("AudioTrack::write() returns unexpected result  (actual: %d, expected: %d)", write_result, bb.size());
            break;
        }

        bb.rewind();
    }

    return play_result;
}

}
}
