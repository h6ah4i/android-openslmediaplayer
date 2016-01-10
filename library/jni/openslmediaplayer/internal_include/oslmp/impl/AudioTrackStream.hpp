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

#ifndef AUDIOTRACKSTREAM_HPP_
#define AUDIOTRACKSTREAM_HPP_

#include <jni.h>
#include <pthread.h>
#include <cxxporthelper/cstdint>
#include <cxxporthelper/atomic>
#include <cxxporthelper/memory>

#include "oslmp/impl/AudioDataTypes.hpp"

namespace oslmp {
namespace impl {

class AudioTrack;

class AudioTrackStream {
public:
    typedef int32_t (*render_callback_func_t)(void *buffer, sample_format_type format, uint32_t num_channels, uint32_t buffer_size_in_frames, void *args);

    AudioTrackStream();
    ~AudioTrackStream();

    int init(JNIEnv *env, int stream_type, sample_format_type format, uint32_t sample_rate_in_hz, uint32_t num_channels, uint32_t buffer_size_in_frames, uint32_t buffer_block_count) noexcept;

    int start(render_callback_func_t callback, void *args) noexcept;
    int stop() noexcept;

    int32_t getAudioSessionId() noexcept;

private:
    static void *sinkWriterThreadEntryFunc(void *args) noexcept;

    void sinkWriterThreadProcess(JNIEnv *env) noexcept;
    void sinkWriterThreadLoopS16(JNIEnv *env) noexcept;
    void sinkWriterThreadLoopFloat(JNIEnv *env) noexcept;
    void sinkWriterThreadLoopS16ByteBuffer(JNIEnv *env) noexcept;
    void sinkWriterThreadLoopFloatByteBuffer(JNIEnv *env) noexcept;

    JNIEnv *getJNIEnv() noexcept;

private:
    JavaVM *vm_;
    std::unique_ptr<AudioTrack> track_;
    pthread_t pt_handle_;

    uint32_t buffer_size_in_frames_;
    uint32_t buffer_block_count_;

    render_callback_func_t callback_func_;
    void *callback_args_;
    std::atomic_bool stop_request_;
};

}
}

#endif // AUDIOTRACKSTREAM_HPP_
