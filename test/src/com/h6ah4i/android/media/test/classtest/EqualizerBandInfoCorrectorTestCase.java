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


package com.h6ah4i.android.media.test.classtest;

import android.media.MediaPlayer;
import android.media.audiofx.Equalizer;
import android.test.AndroidTestCase;

import com.h6ah4i.android.media.utils.EqualizerBandInfoCorrector;

public class EqualizerBandInfoCorrectorTestCase extends AndroidTestCase {

    // Nexus series and AOSP based ROM
    public void testNexusAndAOSP() {
        // @formatter:off
        final int numBands = 5;
        final int[] inputCenterFreq = new int[] {
                60000, 230000, 910000, 3600000, 14000000
        };
        final int[][] inputBandFreqRange = new int[][] {
                { 30000, 120000 }, 
                { 120001, 460000 }, 
                { 460001, 1800000 }, 
                { 1800001, 7000000 }, 
                { 7000001, 1 }      // !!! max value is wrong !!!
        };
        // @formatter:on

        final int[] outputCenterFreq = new int[numBands];
        final int[][] outputBandFreqRange = new int[numBands][2];

        final boolean result = EqualizerBandInfoCorrector.correct(
                numBands,
                inputCenterFreq, inputBandFreqRange,
                outputCenterFreq, outputBandFreqRange);

        assertTrue(result);
        verify(numBands, outputCenterFreq, outputBandFreqRange);
    }

    // HTC Evo 3D (ISW12HT)
    public void testHTCEvo3D() {
        // @formatter:off
        final int numBands = 5;
        final int[] inputCenterFreq = new int[] {
                100000, 300000, 1000000, 3000000, 10000000
        };
        final int[][] inputBandFreqRange = new int[][] {
                {0, 199999},
                {200000, 649999}, 
                {650000, 1999999}, 
                {2000000, 6499999},
                {6500000, 19999999}
        };
        // @formatter:on

        final int[] outputCenterFreq = new int[numBands];
        final int[][] outputBandFreqRange = new int[numBands][2];

        final boolean result = EqualizerBandInfoCorrector.correct(
                numBands,
                inputCenterFreq, inputBandFreqRange,
                outputCenterFreq, outputBandFreqRange);

        assertTrue(result);
        verify(numBands, outputCenterFreq, outputBandFreqRange);
    }

    // Samsung Galaxy S4 (SC-04E)
    public void testSamsungGalaxyS4() {
        // @formatter:off
        final int numBands = 5;
        final int[] inputCenterFreq = new int[] {
                60000, 230000, 910000, 3600000, 14000000    /* !!! completely wrong !!! */

        };
        final int[][] inputBandFreqRange = new int[][] {    /* !!! has gap and over wrap !!! */
                {75000, 225000},
                {200000, 600000}, 
                {550000, 1650000}, 
                {1750000, 5250000}, 
                {4000000, 6000000}
        };
        // @formatter:on

        final int[] outputCenterFreq = new int[numBands];
        final int[][] outputBandFreqRange = new int[numBands][2];

        final boolean result = EqualizerBandInfoCorrector.correct(
                numBands,
                inputCenterFreq, inputBandFreqRange,
                outputCenterFreq, outputBandFreqRange);

        assertTrue(result);
        verify(numBands, outputCenterFreq, outputBandFreqRange);
    }

    // CyanogenMod 11 custom ROM
    public void testCyanogenMod11() {
        // @formatter:off
        final int numBands = 6;     /* !!! not 5 bands !!! */
        final int[] inputCenterFreq = new int[] {
                15625, 62500, 250000, 1000000, 4000000, 16000000

        };
        final int[][] inputBandFreqRange = new int[][] {    /* !!! has gap and over wrap !!! */
                {7812, 23437}, 
                {31250, 93750}, 
                {125000, 375000}, 
                {500000, 1500000}, 
                {2000000, 6000000}, 
                {8000000, 24000000}
        };
        // @formatter:on

        final int[] outputCenterFreq = new int[numBands];
        final int[][] outputBandFreqRange = new int[numBands][2];

        final boolean result = EqualizerBandInfoCorrector.correct(
                numBands,
                inputCenterFreq, inputBandFreqRange,
                outputCenterFreq, outputBandFreqRange);

        assertTrue(result);
        verify(numBands, outputCenterFreq, outputBandFreqRange);
    }

    public void testActualDevice() {
        MediaPlayer player = null;
        Equalizer equalizer = null;

        try {
            player = new MediaPlayer();
            equalizer = new Equalizer(0, player.getAudioSessionId());

            final int numBands = equalizer.getNumberOfBands();
            final int[] inputCenterFreq = new int[numBands];
            final int[][] inputBandFreqRange = new int[numBands][2];

            final int[] outputCenterFreq = new int[numBands];
            final int[][] outputBandFreqRange = new int[numBands][2];

            // fill inputCenterFreq and inputBandFreqRange
            for (short band = 0; band < numBands; band++) {
                int center = equalizer.getCenterFreq(band);
                int[] range = equalizer.getBandFreqRange(band);

                inputCenterFreq[band] = center;
                inputBandFreqRange[band][0] = range[0];
                inputBandFreqRange[band][1] = range[1];
            }

            final boolean result = EqualizerBandInfoCorrector.correct(
                    numBands,
                    inputCenterFreq, inputBandFreqRange,
                    outputCenterFreq, outputBandFreqRange);

            assertTrue(result);
            verify(numBands, outputCenterFreq, outputBandFreqRange);
        } finally {
            if (equalizer != null) {
                equalizer.release();
            }
            if (player != null) {
                player.release();
            }
        }
    }

    //
    // Utilities
    //
    private static final int NYQUIST_FREQ = 24000000;

    private static void verifyCenterFrequencyArray(
            int numBands, int[] centerFreq) {

        for (int band = 0; band < numBands; band++) {
            assertTrue(centerFreq[band] > 0 && centerFreq[band] < NYQUIST_FREQ);

            if (band > 0) {
                assertTrue(centerFreq[band] > centerFreq[band - 1]);
            }
        }
    }

    private static void verifyBandFrequencyRangeArray(
            int numBands, int[][] bandFreqRange) {

        for (int band = 0; band < numBands; band++) {
            final int rangeMin = bandFreqRange[band][0];
            final int rangeMax = bandFreqRange[band][1];

            assertTrue(rangeMin >= 0 && rangeMin <= NYQUIST_FREQ);
            assertTrue(rangeMax >= 0 && rangeMax <= NYQUIST_FREQ);

            assertTrue(rangeMax > rangeMin);

            if (band > 0) {
                final int prevRangeMax = bandFreqRange[band - 1][1];
                assertEquals(rangeMin, (prevRangeMax + 1));
            }
        }
    }

    private static void verifyCenterFreqAndRangeRelationship(
            int numBands, int[] centerFreq, int[][] bandFreqRange) {

        for (int band = 0; band < numBands; band++) {
            final int center = centerFreq[band];
            final int rangeMin = bandFreqRange[band][0];
            final int rangeMax = bandFreqRange[band][1];

            assertTrue(rangeMin < center && center < rangeMax);
        }
    }

    private static void verify(int numBands, int[] centerFreq, int[][] bandFreqRange) {
        verifyCenterFrequencyArray(numBands, centerFreq);
        verifyBandFrequencyRangeArray(numBands, bandFreqRange);
        verifyCenterFreqAndRangeRelationship(numBands, centerFreq, bandFreqRange);
    }
}
