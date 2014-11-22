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

#include "oslmp/impl/EqualizerBandInfoCorrector.hpp"

#include <algorithm>

#include <cxxporthelper/cmath>

// constatns
#define FLAG_RANGE_INVERSION (1 << 0)
#define FLAG_RANGE_GAP (1 << 1)
#define FLAG_RANGE_OVERWRAP (1 << 2)

#define FLAG_CENTER_WRONG_ORDER (1 << 8)
#define FLAG_CENTER_OUT_OF_RANGE (1 << 9)

#define NYQUIST_FREQ 24000000

namespace oslmp {
namespace impl {

// internal functions
static bool makeCenterFreq(int numBands, const int (*inBandFreqRange)[2], int *outCenterFreq) noexcept;

static bool correctRangeGapAndOverwrap(int numBands, const int *inCenterFreq, const int (*inBandFreqRange)[2],
                                       int (*outBandFreqRange)[2]) noexcept;

static void copyCenterFreqArray(int numBands, const int *inCenterFreq, int *outCenterFreq) noexcept;

static void copyBandFreqRangeArray(int numBands, const int (*inBandFreqRange)[2], int (*outBandFreqRange)[2]) noexcept;

static bool correctRangeMax(int numBands, const int *inCenterFreq, const int (*inBandFreqRange)[2], int *outCenterFreq,
                            int (*outBandFreqRange)[2], const int *analyzeResult) noexcept;

static int roundFrequency(double frequency, int significantFigures) noexcept;

static int calcSumResult(int numBands, const int *result) noexcept;

static void analyzeBandFreqRange(int numBands, const int (*range)[2], int *result) noexcept;

static void analyzeCenterFreq(int numBands, const int (*range)[2], const int *center, int *result) noexcept;

bool EqualizerBandInfoCorrector::correct(int numBands, const int *inCenterFreq, const int (*inBandFreqRange)[2],
                                         int *outCenterFreq, int (*outBandFreqRange)[2]) noexcept
{

    int analyzeResult[numBands];

    ::memset(analyzeResult, 0, sizeof(analyzeResult));

    analyzeBandFreqRange(numBands, inBandFreqRange, analyzeResult);
    analyzeCenterFreq(numBands, inBandFreqRange, inCenterFreq, analyzeResult);

    int sumResult = calcSumResult(numBands, analyzeResult);
    bool processed = false;

    if (sumResult == 0) {
        // valid, nothing to do
        copyCenterFreqArray(numBands, inCenterFreq, outCenterFreq);
        copyBandFreqRangeArray(numBands, inBandFreqRange, outBandFreqRange);
        processed = true;
    } else if (sumResult == (FLAG_RANGE_INVERSION | FLAG_CENTER_OUT_OF_RANGE)) {
        // Nexus series and AOSP based implementation
        if (correctRangeMax(numBands, inCenterFreq, inBandFreqRange, outCenterFreq, outBandFreqRange, analyzeResult)) {
            processed = true;
        }
    } else if ((sumResult & FLAG_RANGE_INVERSION) == 0) {
        bool outCenterFreqAvail = false;

        if ((sumResult & (FLAG_CENTER_OUT_OF_RANGE | FLAG_CENTER_WRONG_ORDER)) != 0) {
            if (makeCenterFreq(numBands, inBandFreqRange, outCenterFreq)) {
                outCenterFreqAvail = true;
            }
        } else {
            copyCenterFreqArray(numBands, inCenterFreq, outCenterFreq);
            outCenterFreqAvail = true;
        }

        if (outCenterFreqAvail) {
            if (correctRangeGapAndOverwrap(numBands, outCenterFreq, inBandFreqRange, outBandFreqRange)) {
                processed = true;
            }
        }
    }

    if (!processed)
        return false;

    // verify
    ::memset(analyzeResult, 0, sizeof(analyzeResult));
    analyzeBandFreqRange(numBands, outBandFreqRange, analyzeResult);
    analyzeCenterFreq(numBands, outBandFreqRange, outCenterFreq, analyzeResult);

    if (calcSumResult(numBands, analyzeResult) != 0)
        return false;

    return true;
}

static bool makeCenterFreq(int numBands, const int (*inBandFreqRange)[2], int *outCenterFreq) noexcept
{
    for (int band = 0; band < numBands; band++) {
        const int rangeMin = inBandFreqRange[band][0];
        const int rangeMax = inBandFreqRange[band][1];

        if (rangeMin != 0 && rangeMin < rangeMax) {
            const double ratioMinMax = (double)rangeMax / rangeMin;
            const double ratioMinCenter = std::sqrt(ratioMinMax);

            outCenterFreq[band] = roundFrequency(rangeMin * ratioMinCenter, 2);

            // check order inversion
            if (band > 0) {
                if (outCenterFreq[band - 1] >= outCenterFreq[band]) {
                    return false;
                }
            }
        } else {
            return false;
        }
    }

    return true;
}

static bool correctRangeGapAndOverwrap(int numBands, const int *inCenterFreq, const int (*inBandFreqRange)[2],
                                       int (*outBandFreqRange)[2]) noexcept
{
    for (int band = 0; band < numBands; band++) {
        const int rangeMin = inBandFreqRange[band][0];
        const int rangeMax = inBandFreqRange[band][1];
        const int center = inCenterFreq[band];

        if (band < (numBands - 1)) {
            const int correctedRangeMax = inBandFreqRange[band + 1][0] - 1;

            if (rangeMin < center && center < correctedRangeMax) {
                outBandFreqRange[band][0] = rangeMin;
                outBandFreqRange[band][1] = correctedRangeMax;
            } else {
                return false;
            }
        } else {
            if (rangeMin < center && center < rangeMax) {
                outBandFreqRange[band][0] = rangeMin;
                outBandFreqRange[band][1] = rangeMax;
            } else {
                return false;
            }
        }
    }

    return true;
}

static void copyCenterFreqArray(int numBands, const int *inCenterFreq, int *outCenterFreq) noexcept
{
    for (int band = 0; band < numBands; band++) {
        outCenterFreq[band] = inCenterFreq[band];
    }
}

static void copyBandFreqRangeArray(int numBands, const int (*inBandFreqRange)[2], int (*outBandFreqRange)[2]) noexcept
{
    for (int band = 0; band < numBands; band++) {
        outBandFreqRange[band][0] = inBandFreqRange[band][0];
        outBandFreqRange[band][1] = inBandFreqRange[band][1];
    }
}

static bool correctRangeMax(int numBands, const int *inCenterFreq, const int (*inBandFreqRange)[2], int *outCenterFreq,
                            int (*outBandFreqRange)[2], const int *analyzeResult) noexcept
{
    for (int band = 0; band < numBands; band++) {
        if (analyzeResult[band] == 0) {
            // valid
            outCenterFreq[band] = inCenterFreq[band];
            outBandFreqRange[band][0] = inBandFreqRange[band][0];
            outBandFreqRange[band][1] = inBandFreqRange[band][1];
        } else if (analyzeResult[band] == (FLAG_RANGE_INVERSION | FLAG_CENTER_OUT_OF_RANGE)) {
            if ((inCenterFreq[band] > inBandFreqRange[band][0]) && !(inCenterFreq[band] < inBandFreqRange[band][1]) &&
                (inBandFreqRange[band][0] > 0)) {
                // range max is incorrect
                double ratio = (double)inCenterFreq[band] / inBandFreqRange[band][0];
                int correctedRangeMax = roundFrequency(ratio * inCenterFreq[band], 2);

                outCenterFreq[band] = inCenterFreq[band];
                outBandFreqRange[band][0] = inBandFreqRange[band][0];
                outBandFreqRange[band][1] = correctedRangeMax;
            } else {
                // unexpected
                return false;
            }
        } else {
            // unexpected
            return false;
        }
    }

    return true;
}

static int roundFrequency(double frequency, int significantFigures) noexcept
{
    const int exp10 = static_cast<int>(std::log10(frequency));
    const double normalizeCoeff = std::pow(10.0, significantFigures - exp10 - 1);

    const double normalized = frequency * normalizeCoeff;
    const int roundNormalized = static_cast<int>(normalized + 0.5);

    return (std::min)(NYQUIST_FREQ, static_cast<int>(roundNormalized / normalizeCoeff + 0.5));
}

static int calcSumResult(int numBands, const int *result) noexcept
{
    int flag = 0;

    for (int i = 0; i < numBands; ++i) {
        flag |= result[i];
    }

    return flag;
}

static void analyzeBandFreqRange(int numBands, const int (*range)[2], int *result) noexcept
{
    for (int band = 0; band < numBands; band++) {
        int flag = result[band];

        if (range[band][1] < range[band][0]) {
            flag |= FLAG_RANGE_INVERSION;
        }

        if (band > 0) {
            if ((range[band - 1][1] + 1) < range[band][0]) {
                flag |= FLAG_RANGE_GAP;
            } else if ((range[band - 1][1] + 1) > range[band][0]) {
                flag |= FLAG_RANGE_OVERWRAP;
            }
        }

        result[band] = flag;
    }
}

static void analyzeCenterFreq(int numBands, const int (*range)[2], const int *center, int *result) noexcept
{
    for (int band = 0; band < numBands; band++) {
        int flag = result[band];

        if (!((range[band][0] <= center[band]) && (center[band] <= range[band][1]))) {
            flag |= FLAG_CENTER_OUT_OF_RANGE;
        }

        if (band > 0) {
            if (center[band - 1] > center[band]) {
                flag |= FLAG_CENTER_WRONG_ORDER;
            }
        }

        result[band] = flag;
    }
}

} // namespace impl
} // namespace oslmp
