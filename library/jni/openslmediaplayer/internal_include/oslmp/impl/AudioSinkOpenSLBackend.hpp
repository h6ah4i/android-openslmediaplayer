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

#include <SLESCXX/OpenSLES_CXX.hpp>

#include "oslmp/impl/AudioSink.hpp"
#include "oslmp/impl/AudioPipeBufferQueueBinder.hpp"

namespace oslmp {
namespace impl {

//
// AudioSinkOpenSLBackend
//
class AudioSinkOpenSLBackend : public AudioSinkBackend {
public:
    AudioSinkOpenSLBackend();
    virtual ~AudioSinkOpenSLBackend() override;

    virtual int onInitialize(const AudioSink::initialize_args_t &args, void *pipe_user) noexcept override;
    virtual int onStart() noexcept override;
    virtual int onPause() noexcept override;
    virtual int onResume() noexcept override;
    virtual int onStop() noexcept override;
    virtual opensles::CSLObjectItf *onGetPlayerObj() const noexcept override;
    virtual uint32_t onGetLatencyInFrames() const noexcept override;
    virtual int32_t onGetAudioSessionId() const noexcept override;

private:
    void releaseOpenSLResources() noexcept;

    static void playbackBufferQueueCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) noexcept;

private:
    AudioPipeBufferQueueBinder queue_binder_;
    opensles::CSLObjectItf obj_player_;

    opensles::CSLPlayItf player_;
    opensles::CSLVolumeItf volume_;
    opensles::CSLAndroidSimpleBufferQueueItf buffer_queue_;

    int block_size_in_frames_;
    uint32_t num_player_blocks_;
    uint32_t num_pipe_blocks_;
};

} // namespace impl
} // namespace oslmp
