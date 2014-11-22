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

#include "oslmp/impl/StockVisualizerAlgorithms.hpp"

#include <cassert>
#include <cxxporthelper/cmath>

#include <audio_utils/fixedfft.h>

#include <oslmp/OpenSLMediaPlayerVisualizer.hpp>

#define VISUALIZER_SCALING_MODE_NORMALIZED OpenSLMediaPlayerVisualizer::SCALING_MODE_NORMALIZED
#define VISUALIZER_SCALING_MODE_AS_PLAYED OpenSLMediaPlayerVisualizer::SCALING_MODE_AS_PLAYED

namespace oslmp {
namespace impl {

/*
 * doFft()
 *
 * Original:
 *
 *  status_t Visualizer::doFft(uint8_t *fft, uint8_t *waveform)
 *
 *  from android/platform/system/av/media/libmedia/Visualizer.cpp
 */
/*
**
** Copyright 2010, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
void StockVisualizerAlgorithms::doFft(int8_t *CXXPH_RESTRICT fft, const uint8_t *CXXPH_RESTRICT waveform,
                                      uint32_t capture_size)
{
    int32_t workspace[capture_size >> 1];
    int32_t nonzero = 0;

    for (uint32_t i = 0; i < capture_size; i += 2) {
        workspace[i >> 1] = ((waveform[i] ^ 0x80) << 24) | ((waveform[i + 1] ^ 0x80) << 8);
        nonzero |= workspace[i >> 1];
    }

    if (nonzero) {
        fixed_fft_real(capture_size >> 1, workspace);
    }

    for (uint32_t i = 0; i < capture_size; i += 2) {
        short tmp = workspace[i >> 1] >> 21;
        while (tmp > 127 || tmp < -128)
            tmp >>= 1;
        fft[i] = tmp;
        tmp = workspace[i >> 1];
        tmp >>= 5;
        while (tmp > 127 || tmp < -128)
            tmp >>= 1;
        fft[i + 1] = tmp;
    }
}
/* doFft() */

/*
 * measurePeakRmsSquared()
 *
 * Original:
 *
 *  int Visualizer_process(
 *       effect_handle_t self,audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
 *
 *  from android/platform/system/av/media/libeffects/visualizer/EffectVisualizer.cpp
 */
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
void StockVisualizerAlgorithms::measurePeakRmsSquared(const int16_t *CXXPH_RESTRICT waveform, uint32_t frameCount,
                                                      uint32_t channelCount, uint16_t *CXXPH_RESTRICT outPeakU16,
                                                      float *CXXPH_RESTRICT outRmsSquared)
{

    // find the peak and RMS squared for the new buffer
    uint32_t inIdx;
    int16_t maxSample = 0;
    float rmsSqAcc = 0;
    for (inIdx = 0; inIdx < frameCount * channelCount; inIdx++) {
        if (waveform[inIdx] > maxSample) {
            maxSample = waveform[inIdx];
        } else if (-waveform[inIdx] > maxSample) {
            maxSample = -waveform[inIdx];
        }
        rmsSqAcc += (waveform[inIdx] * waveform[inIdx]);
    }

    // store the measurement
    (*outPeakU16) = (uint16_t)maxSample;
    (*outRmsSquared) = rmsSqAcc / (frameCount * channelCount);
}
/* measurePeakRmsSquared() */

/*
 * convertWaveformS16StereoToU8Mono()
 *
 * Original:
 *
 *  int Visualizer_process(
 *       effect_handle_t self,audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
 *
 *  from android/platform/system/av/media/libeffects/visualizer/EffectVisualizer.cpp
 */
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
void StockVisualizerAlgorithms::convertWaveformS16StereoToU8Mono(uint8_t *CXXPH_RESTRICT out_u8,
                                                                 const int16_t *CXXPH_RESTRICT in_s16,
                                                                 uint32_t frameCount, int32_t mode)
{

    // all code below assumes stereo 16 bit PCM output and input
    int32_t shift;

    if (mode == VISUALIZER_SCALING_MODE_NORMALIZED) {
        // derive capture scaling factor from peak value in current buffer
        // this gives more interesting captures for display.
        shift = 32;
        int len = frameCount * 2;
        for (int i = 0; i < len; i++) {
            int32_t smp = in_s16[i];
            if (smp < 0)
                smp = -smp - 1; // take care to keep the max negative in range
            int32_t clz = __builtin_clz(smp);
            if (shift > clz)
                shift = clz;
        }
        // A maximum amplitude signal will have 17 leading zeros, which we want to
        // translate to a shift of 8 (for converting 16 bit to 8 bit)
        shift = 25 - shift;
        // Never scale by less than 8 to avoid returning unaltered PCM signal.
        if (shift < 3) {
            shift = 3;
        }
        // add one to combine the division by 2 needed after summing left and right channels below
        shift++;
    } else {
        assert(mode == VISUALIZER_SCALING_MODE_AS_PLAYED);
        shift = 9;
    }

    for (uint32_t idx = 0; idx < frameCount; ++idx) {
        int32_t smp = in_s16[2 * idx] + in_s16[2 * idx + 1];
        smp = smp >> shift;
        out_u8[idx] = ((uint8_t)smp) ^ 0x80;
    }
}
/* convertWaveformS16StereoToU8Mono() */

/*
 * linearToMillibel()
 *
 * Original:
 *
 * int Visualizer_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
 *         void *pCmdData, uint32_t *replySize, void *pReplyData)
 *
 *  from android/platform/system/av/media/libeffects/visualizer/EffectVisualizer.cpp
 */
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
int32_t StockVisualizerAlgorithms::linearToMillibel(float linear)
{
    if (linear < 0.000016f) {
        return -9600; // -96.0 dB
    } else {
        return (int32_t)(2000 * log10f(linear / 32767.0f));
    }
}
/* linearToMillibel() */

} // namespace impl
} // namespace oslmp
