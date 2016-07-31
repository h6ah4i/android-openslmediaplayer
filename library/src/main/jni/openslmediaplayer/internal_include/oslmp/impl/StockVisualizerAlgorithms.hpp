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

#ifndef STOCKVISUALIZERALGORITHMS_HPP_
#define STOCKVISUALIZERALGORITHMS_HPP_

#include <cxxporthelper/cstdint>
#include <cxxporthelper/compiler.hpp>

namespace oslmp {
namespace impl {

class StockVisualizerAlgorithms {
private:
    StockVisualizerAlgorithms();

public:
    static void doFft(int8_t *CXXPH_RESTRICT fft, const uint8_t *CXXPH_RESTRICT waveform, uint32_t capture_size);

    static void measurePeakRmsSquared(const int16_t *CXXPH_RESTRICT waveform, uint32_t frameCount,
                                      uint32_t channelCount, uint16_t *CXXPH_RESTRICT outPeakU16,
                                      float *CXXPH_RESTRICT outRmsSquared);

    static void convertWaveformS16StereoToU8Mono(uint8_t *CXXPH_RESTRICT out_u8, const int16_t *CXXPH_RESTRICT in_s16,
                                                 uint32_t frameCount, int32_t mode);

    static int32_t linearToMillibel(float linear);
};

} // namespace impl
} // namespace oslmp

#endif // STOCKVISUALIZERALGORITHMS_HPP_
