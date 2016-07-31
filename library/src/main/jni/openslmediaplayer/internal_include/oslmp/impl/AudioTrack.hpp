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

#ifndef AUDIOTRACK_HPP_
#define AUDIOTRACK_HPP_

#include <jni.h>
#include <cxxporthelper/cstdint>

namespace oslmp {
namespace impl {

class AudioTrack {
public:
    static const int32_t SUCCESS = 0;
    static const int32_t ERROR = -1;
    static const int32_t ERROR_BAD_VALUE = -2;
    static const int32_t ERROR_INVALID_OPERATION = -3;

    static const int32_t PLAYSTATE_STOPPED = 1;
    static const int32_t PLAYSTATE_PAUSED = 2;
    static const int32_t PLAYSTATE_PLAYING = 3;

    static const int32_t STATE_UNINITIALIZED = 0;
    static const int32_t STATE_INITIALIZED = 1;
    static const int32_t STATE_NO_STATIC_DATA = 2;

    enum track_mode_t {
        MODE_STATIC = 0,
        MODE_STREAM  = 1,
    };

    enum write_mode_t {
        WRITE_BLOCKING = 0, // since API level 21
        WRITE_NON_BLOCKING = 1, // since API level 21
    };

public:
    AudioTrack();
    ~AudioTrack();

    bool create(JNIEnv *env, int32_t stream_type, int32_t sample_rate, int32_t num_channels, int32_t format, int32_t buffer_size_in_frames, track_mode_t mode, int32_t session_id) noexcept;
    void release(JNIEnv *env) noexcept;
    int32_t play(JNIEnv *env) noexcept;
    int32_t pause(JNIEnv *env) noexcept;
    int32_t stop(JNIEnv *env) noexcept;
    int32_t flush(JNIEnv *env) noexcept;
    int32_t write(JNIEnv *env, jshortArray data, size_t offset, size_t size) noexcept;
    int32_t write(JNIEnv *env, jfloatArray data, size_t offset, size_t size, write_mode_t mode) noexcept;
    int32_t write(JNIEnv *env, jobject data, size_t size_in_bytes, write_mode_t mode) noexcept;
    int32_t getState(JNIEnv *env) noexcept;
    int32_t getPlayState(JNIEnv *env) noexcept;
    int32_t getAudioSessionId(JNIEnv *env) noexcept;
    int32_t setAuxEffectSendLevel(JNIEnv *env, float level) noexcept;
    int32_t attachAuxEffect(JNIEnv *env, int effect_id) noexcept;
    int32_t getAudioFormat() const noexcept;
    int32_t getBufferSizeInFrames() const noexcept;
    int32_t getBufferSizeInBytes() const noexcept;
    int32_t getChannelCount() const noexcept;

    bool supportsByteBufferMethods() const noexcept;

private:
    jclass cls_;
    jobject obj_;
    jmethodID m_play_;
    jmethodID m_pause_;
    jmethodID m_stop_;
    jmethodID m_flush_;
    jmethodID m_release_;
    jmethodID m_get_state_;
    jmethodID m_get_play_state_;
    jmethodID m_get_audio_session_;
    jmethodID m_set_aux_effect_send_level_;
    jmethodID m_attach_aux_effect_;
    jmethodID m_write_sa_;
    jmethodID m_write_fa_;
    jmethodID m_write_bb_;
    int32_t audio_format_;
    int32_t channel_count_;
    int32_t buffer_size_in_frames_;
    int32_t buffer_size_in_bytes_;
};

}
}

#endif // AUDIOTRACK_HPP_
