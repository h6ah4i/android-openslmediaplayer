/*
 *    Copyright (C) 2014 Haruki Hasegawa
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */


package com.h6ah4i.android.media.utils;

import java.util.Arrays;

public class EqualizerBandInfoCorrector {
    private static final int FLAG_RANGE_INVERSION = (1 << 0);
    private static final int FLAG_RANGE_GAP = (1 << 1);
    private static final int FLAG_RANGE_OVERWRAP = (1 << 2);

    private static final int FLAG_CENTER_WRONG_ORDER = (1 << 8);
    private static final int FLAG_CENTER_OUT_OF_RANGE = (1 << 9);

    private static final int NYQUIST_FREQ = 24000000;

    private EqualizerBandInfoCorrector() {
    }

    public static boolean correct(
            int numBands,
            int[] inCenterFreq, int[][] inBandFreqRange,
            int[] outCenterFreq, int[][] outBandFreqRange
            ) {
        int[] analyzeResult = new int[numBands];

        analyzeBandFreqRange(numBands, inBandFreqRange, analyzeResult);
        analyzeCenterFreq(numBands, inBandFreqRange, inCenterFreq, analyzeResult);

        int sumResult = calcSumResult(numBands, analyzeResult);
        boolean processed = false;

        if (sumResult == 0) {
            // valid, nothing to do
            copyCenterFreqArray(numBands, inCenterFreq, outCenterFreq);
            copyBandFreqRangeArray(numBands, inBandFreqRange, outBandFreqRange);
            processed = true;
        } else if (sumResult == (FLAG_RANGE_INVERSION | FLAG_CENTER_OUT_OF_RANGE)) {
            // Nexus series and AOSP based implementation
            if (correctRangeMax(
                    numBands,
                    inCenterFreq, inBandFreqRange,
                    outCenterFreq, outBandFreqRange,
                    analyzeResult)) {
                processed = true;
            }
        } else if ((sumResult & FLAG_RANGE_INVERSION) == 0) {
            boolean outCenterFreqAvail = false;

            if ((sumResult & (FLAG_CENTER_OUT_OF_RANGE | FLAG_CENTER_WRONG_ORDER)) != 0) {
                if (makeCenterFreq(numBands, inBandFreqRange, outCenterFreq)) {
                    outCenterFreqAvail = true;
                }
            } else {
                copyCenterFreqArray(numBands, inCenterFreq, outCenterFreq);
                outCenterFreqAvail = true;
            }

            if (outCenterFreqAvail) {
                if (correctRangeGapAndOverwrap(numBands, outCenterFreq, inBandFreqRange,
                        outBandFreqRange)) {
                    processed = true;
                }
            }
        }

        if (!processed)
            return false;

        // verify
        Arrays.fill(analyzeResult, 0);
        analyzeBandFreqRange(numBands, outBandFreqRange, analyzeResult);
        analyzeCenterFreq(numBands, outBandFreqRange, outCenterFreq, analyzeResult);

        if (calcSumResult(numBands, analyzeResult) != 0)
            return false;

        return true;
    }

    private static boolean makeCenterFreq(
            int numBands, int[][] inBandFreqRange, int[] outCenterFreq) {
        for (int band = 0; band < numBands; band++) {
            final int rangeMin = inBandFreqRange[band][0];
            final int rangeMax = inBandFreqRange[band][1];

            if (rangeMin != 0 && rangeMin < rangeMax) {
                final double ratioMinMax = (double) rangeMax / rangeMin;
                final double ratioMinCenter = Math.sqrt(ratioMinMax);

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

    private static boolean correctRangeGapAndOverwrap(
            int numBands, int[] inCenterFreq, int[][] inBandFreqRange, int[][] outBandFreqRange) {
        for (int band = 0; band < numBands; band++) {
            final int rangeMin = inBandFreqRange[band][0];
            final int rangeMax = inBandFreqRange[band][1];
            final int center = inCenterFreq[band];

            if (band < (numBands - 1)) {
                final int correctedRangeMax = inBandFreqRange[band + 1][0] - 1;

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

    private static void copyCenterFreqArray(
            int numBands, int[] inCenterFreq, int[] outCenterFreq) {
        for (int band = 0; band < numBands; band++) {
            outCenterFreq[band] = inCenterFreq[band];
        }
    }

    private static void copyBandFreqRangeArray(
            int numBands, int[][] inBandFreqRange, int[][] outBandFreqRange) {
        for (int band = 0; band < numBands; band++) {
            outBandFreqRange[band][0] = inBandFreqRange[band][0];
            outBandFreqRange[band][1] = inBandFreqRange[band][1];
        }
    }

    private static boolean correctRangeMax(
            int numBands,
            int[] inCenterFreq, int[][] inBandFreqRange,
            int[] outCenterFreq, int[][] outBandFreqRange,
            int[] analyzeResult) {
        for (int band = 0; band < numBands; band++) {
            if (analyzeResult[band] == 0) {
                // valid
                outCenterFreq[band] = inCenterFreq[band];
                outBandFreqRange[band][0] = inBandFreqRange[band][0];
                outBandFreqRange[band][1] = inBandFreqRange[band][1];
            } else if (analyzeResult[band] == (FLAG_RANGE_INVERSION | FLAG_CENTER_OUT_OF_RANGE)) {
                if ((inCenterFreq[band] > inBandFreqRange[band][0]) &&
                        !(inCenterFreq[band] < inBandFreqRange[band][1]) &&
                        (inBandFreqRange[band][0] > 0)) {
                    // range max is incorrect
                    double ratio = (double) inCenterFreq[band] / inBandFreqRange[band][0];
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

    private static int roundFrequency(double frequency, int significantFigures) {
        final int exp10 = (int) Math.log10(frequency);
        final double normalizeCoeff = Math.pow(10, significantFigures - exp10 - 1);

        final double normalized = frequency * normalizeCoeff;
        final int roundNormalized = (int) Math.round(normalized);

        return Math.min(
                NYQUIST_FREQ,
                (int) (roundNormalized / normalizeCoeff + 0.5));
    }

    private static int calcSumResult(int numBands, int[] result) {
        int flag = 0;

        for (int i = 0; i < numBands; i++) {
            flag |= result[i];
        }

        return flag;
    }

    private static void analyzeBandFreqRange(int numBands, int[][] range, int[] result) {
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

    private static void analyzeCenterFreq(int numBands, int[][] range, int[] center, int[] result) {
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
}
