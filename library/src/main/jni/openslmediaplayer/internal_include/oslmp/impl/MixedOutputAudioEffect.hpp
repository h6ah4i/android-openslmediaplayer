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

#ifndef MIXEDOUTPUTAUDIOEFFECT_HPP_
#define MIXEDOUTPUTAUDIOEFFECT_HPP_

namespace oslmp {
namespace impl {

class MixedOutputAudioEffect {
protected:
    MixedOutputAudioEffect() {}

public:
    virtual ~MixedOutputAudioEffect() {}

    // these methods are called from message handler thread
    virtual bool isPollingRequired() const noexcept = 0;
    virtual int poll() noexcept = 0;

    // these methods are called from mixer thread context
    virtual void onAttachedToMixerThread() noexcept = 0;
    virtual void onDetachedFromMixerThread() noexcept = 0;

    virtual int pollFromMixerThread() noexcept = 0;
    virtual int process(float *data, uint32_t num_channels, uint32_t num_frames) noexcept = 0;
};

} // namespace impl
} // namespace oslmp

#endif // MIXEDOUTPUTAUDIOEFFECT_HPP_
