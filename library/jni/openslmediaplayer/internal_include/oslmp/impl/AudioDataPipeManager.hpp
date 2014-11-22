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

#ifndef AUDIODATAPIPEMANAGER_HPP_
#define AUDIODATAPIPEMANAGER_HPP_

#include <cxxporthelper/memory>

#include "oslmp/impl/AudioDataTypes.hpp"
#include "oslmp/impl/AudioSourceDataPipe.hpp"
#include "oslmp/impl/AudioSinkDataPipe.hpp"
#include "oslmp/impl/AudioCaptureDataPipe.hpp"

namespace oslmp {
namespace impl {

class AudioDataPipeManager {
public:
    struct initialize_args_t {
        sample_format_type sink_format_type;
        uint32_t source_num_items;
        uint32_t sink_num_items;
        uint32_t capture_num_items;
        uint32_t block_size;

        initialize_args_t()
            : sink_format_type(kAudioSampleFormatType_Unknown), source_num_items(0), sink_num_items(0),
              capture_num_items(0), block_size(0)
        {
        }
    };

    struct status_t {
        int source_num_in_port_users;
        int source_num_out_port_users;

        int sink_num_in_port_users;
        int sink_num_out_port_users;

        int capture_num_in_port_users;
        int capture_num_out_port_users;

        status_t()
            : source_num_in_port_users(0), source_num_out_port_users(0), sink_num_in_port_users(0),
              sink_num_out_port_users(0), capture_num_in_port_users(0), capture_num_out_port_users(0)
        {
        }
    };

    class SourcePipeEventListener {
    public:
        virtual ~SourcePipeEventListener() {}
        virtual void onRecycleItem(AudioSourceDataPipe *pipe,
                                   const AudioSourceDataPipe::recycle_block_t *block) noexcept = 0;
    };

    AudioDataPipeManager();
    ~AudioDataPipeManager();

    int initialize(const initialize_args_t &arg) noexcept;

    int getBlockSizeInFrames() const noexcept;

    int obtainSourcePipe(AudioSourceDataPipe **pipe) noexcept;
    int setSourcePipeInPortUser(AudioSourceDataPipe *pipe, void *user, SourcePipeEventListener *listener,
                                bool set_or_clear) noexcept;
    int setSourcePipeOutPortUser(AudioSourceDataPipe *pipe, void *user, bool set_or_clear) noexcept;

    int obtainSinkPipe(AudioSinkDataPipe **pipe) noexcept;
    int setSinkPipeInPortUser(AudioSinkDataPipe *pipe, void *user, bool set_or_clear) noexcept;
    int setSinkPipeOutPortUser(AudioSinkDataPipe *pipe, void *user, bool set_or_clear) noexcept;

    int obtainCapturePipe(AudioCaptureDataPipe **pipe) noexcept;
    int setCapturePipeInPortUser(AudioCaptureDataPipe *pipe, void *user, bool set_or_clear) noexcept;
    int setCapturePipeOutPortUser(AudioCaptureDataPipe *pipe, void *user, bool set_or_clear) noexcept;

    int getStatus(status_t &status) const noexcept;

    bool isPollingRequired() const noexcept;
    void poll() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // AUDIODATAPIPEMANAGER_HPP_
