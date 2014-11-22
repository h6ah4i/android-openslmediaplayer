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

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import junit.framework.TestSuite;
import android.content.Context;
import android.media.AudioManager;
import android.media.audiofx.Visualizer;
import android.util.Log;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.audiofx.IVisualizer;
import com.h6ah4i.android.media.standard.StandardMediaPlayer;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.test.base.TestVisualizerWrapper;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.EmptyOnDataCaptureListenerObj;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.media.test.utils.SeekCompleteListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class VisualizerTestCase extends BasicMediaPlayerTestCaseBase {
    private static final int GET_AUDIO_CAPTURE_DATA_DELAY = 200;
    private static final int GET_MEASUREMENTS_DELAY = 100;
    private static final int DEFAULT_CAPTURE_SIZE = 1024;

    // NOTE: StandardVisualizer and OpenSLVisualizer specific value.
    private static final int MIN_CAPTURE_RATE = 100; // milli herts

    private static final class TestParams extends BasicTestParams {
        private final PlayerState mPlayerState;

        public TestParams(Class<? extends IMediaPlayerFactory> factoryClass,
                PlayerState playerState) {
            super(factoryClass);
            mPlayerState = playerState;
        }

        public PlayerState getPlayerState() {
            return mPlayerState;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mPlayerState;
        }
    }

    @Override
    protected void setDataSourceForCommonTests(IBasicMediaPlayer player,
            Object args) throws IOException {
        // // 44 kHz
        // player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_MP3));
        // 48 kHz
        player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_48K_MP3));
    }

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        TestSuite suite = new TestSuite();

        // parameterized tests
        ParameterizedTestSuiteBuilder.Filter filter = ParameterizedTestSuiteBuilder
                .matches("testSetDataCaptureListenerWaveFormOnly",
                        "testSetDataCaptureListenerFftOnly",
                        "testSetDataCaptureListenerBothWaveFormFft",
                        "testSetDataCaptureListenerWithValidRateParameters",
                        "testSetDataCaptureListenerWithInvalidRateParameters",
                        "checkSetDataCaptureListenerWhenEnabled",
                        "testSetDataCaptureListenerMeasureWaveFormCaptureRate",
                        "testSetDataCaptureListenerMeasureFftCaptureRate",
                        "dummy");

        List<TestParams> params = new ArrayList<TestParams>();

        List<PlayerState> playerStates = new ArrayList<PlayerState>();
        playerStates.addAll(Arrays.asList(PlayerState.values()));
        playerStates.remove(PlayerState.End);

        for (PlayerState playerState : playerStates) {
            params.add(new TestParams(factoryClazz, playerState));
        }

        suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                VisualizerTestCase.class, params, filter, false));

        // not parameterized tests
        suite.addTest(makeSingleBasicTest(
                VisualizerTestCase.class, "testPlayerStateTransition", factoryClazz));

        return suite;
    }

    public VisualizerTestCase(ParameterizedTestArgs args) {
        super(args);
    }

    //
    // Exposed test cases
    //
    public void testDefaultCaptureSize() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkDefaultCaptureSize(player);
            }
        });
    }

    public void testSetAndGetEnabled() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetAndGetEnabled(player);
            }
        });
    }

    public void testSetCaptureSizeWithVaildParameters() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetCaptureSizeWithVaildParameters(player);
            }
        });
    }

    public void testSetCaptureSizeWithInvaildParameters() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetCaptureSizeWithInvaildParameters(player);
            }
        });
    }

    public void testSetCaptureSizeWhenEnabledWithValidParameters()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetCaptureSizeWhenEnabledWithValidParameters(player);
            }
        });
    }

    public void testSetCaptureSizeWhenEnabledWithInvalidParameters()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetCaptureSizeWhenEnabledWithInvalidParameters(player);
            }
        });
    }

    public void testGetCaptureRangeCompat() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetCaptureSizeRangeCompat(player);
            }
        });
    }

    public void testGetMaxCaptureRateCompat() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetMaxCaptureRateCompat(player);
            }
        });
    }

    public void testDefaultScalingModeCompat() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkDefaultScalingModeCompat(player);
            }
        });
    }

    public void testSetScalingModeCompatWithValidParams() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetScalingModeCompatWithValidParams(player);
            }
        });
    }

    public void testSetScalingModeCompatWithInvalidParams() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetScalingModeCompatWithInvalidParams(player);
            }
        });
    }

    public void testSetScalingModeCompatWhenEnabledWithValidParams()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetScalingModeCompatWhenEnabledWithValidParams(player);
            }
        });
    }

    public void testSetScalingModeCompaWhenEnabledtWithInvalidParams()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetScalingModeCompatWhenEnabledWithInvalidParams(player);
            }
        });
    }

    public void testDefaultMeasurementModeCompat() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkDefaultMeasurementModeCompat(player);
            }
        });
    }

    public void testSetMeasurementModeCompatWithValidParams() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetMeasurementModeCompatWithValidParams(player);
            }
        });
    }

    public void testSetMeasurementModeCompatWithInvalidParams()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetMeasurementModeCompatWithInvalidParams(player);
            }
        });
    }

    public void testSetMeasurementModeCompatWhenEnabledWithValidParams()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetMeasurementModeCompatWhenEnabledWithValidParams(player);
            }
        });
    }

    public void testSetMeasurementModeCompaWhenEnabledtWithInvalidParams()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetMeasurementModeCompatWhenEnabledWithInvalidParams(player);
            }
        });
    }

    public void testGetWaveForm() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetWaveForm(player);
            }
        });
    }

    public void testGetWaveFormWhenDisabled() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetWaveFormWhenDisabled(player);
            }
        });
    }

    public void testGetWaveFormWithInvalidParameters() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetWaveFormWithInvalidParameters(player);
            }
        });
    }

    public void testGetWaveFormWhenDisabledWithInvalidParameters()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetWaveFormWhenDisabledWithInvalidParameters(player);
            }
        });
    }

    public void testGetFft() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetFft(player);
            }
        });
    }

    public void testGetFftWhenDisabled() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetFftWhenDisabled(player);
            }
        });
    }

    public void testGetFftWithInvalidParameters() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetFftWithInvalidParameters(player);
            }
        });
    }

    public void testGetFftWhenDisabledWithInvalidParameters() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetFftWhenDisabledWithInvalidParameters(player);
            }
        });
    }

    public void testGetSamplingRate() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetSamplingRate(player);
            }
        });
    }

    public void testGetSamplingRateWhenEnabled() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetSamplingRateWhenEnabled(player);
            }
        });
    }

    public void testSetDataCaptureListenerWaveFormOnly() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerWaveFormOnly(player);
            }
        });
    }

    public void testSetDataCaptureListenerFftOnly() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerFftOnly(player);
            }
        });
    }

    public void testSetDataCaptureListenerBothWaveFormFft() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerBothWaveFormFft(player);
            }
        });
    }

    public void testSetDataCaptureListenerWithValidRateParameters()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerWithValidRateParameters(player);
            }
        });
    }

    public void testSetDataCaptureListenerWithInvalidRateParameters()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerWithInvalidRateParameters(player);
            }
        });
    }

    public void testSetDataCaptureListenerWhenEnabled() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerWhenEnabled(player);
            }
        });
    }

    public void testSetDataCaptureListenerMeasureWaveFormCaptureRate()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerMeasureWaveFormCaptureRate(player);
            }
        });
    }

    public void testSetDataCaptureListenerMeasureFftCaptureRate()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerMeasureFftCaptureRate(player);
            }
        });
    }

    public void testSetDataCaptureListenerBoth() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetDataCaptureListenerFftOnly(player);
            }
        });
    }

    public void testCaptureSizeWorksAsExpected() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkCaptureSizeWorksAsExpected(player);
            }
        });
    }

    public void testGetMeasurementPeakRmsCompatWithNull() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetMeasurementPeakRmsCompatWithNull(player);
            }
        });
    }

    public void testGetMeasurementPeakRmsCompatWhenEnabledWithNull()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetMeasurementPeakRmsCompatWhenEnabledWithNull(player);
            }
        });
    }

    public void testGetMeasurementPeakRmsCompatWhenMODE_NONE() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetMeasurementPeakRmsCompatWhenMODE_NONE(player);
            }
        });
    }

    public void testGetMeasurementPeakRmsCompatWorksAsExpected()
            throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetMeasurementPeakRmsCompatWorksAsExpected(player);
            }
        });
    }

    public void testAfterRelease() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkAfterRelease(player);
            }
        });
    }

    public void testMultiInstanceBehavior() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkMultiInstanceBehavior(player);
            }
        });
    }

    public void testPlayerReleasedBeforeEffect() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkPlayerReleasedBeforeEffect(player);
            }
        });
    }

    public void testAfterControlLost() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkAfterControlLost(player);
            }
        });
    }

    private void checkDefaultCaptureSize(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(DEFAULT_CAPTURE_SIZE, visualizer.getCaptureSize());
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetAndGetEnabled(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertFalse(visualizer.getEnabled());

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(false));
            assertFalse(visualizer.getEnabled());
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetCaptureSizeWithVaildParameters(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] range = Visualizer.getCaptureSizeRange();

            assertFalse(visualizer.getEnabled());

            for (int value = range[0] * 2; value <= range[1]; value *= 2) {
                checkSetCaptureSizeSUCCESS(visualizer, value);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetCaptureSizeWithInvaildParameters(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] range = Visualizer.getCaptureSizeRange();

            assertFalse(visualizer.getEnabled());

            // minimum / 2
            checkSetCaptureSizeERROR_BAD_VALUE(visualizer, range[0] / 2);

            // maximum * 2
            checkSetCaptureSizeERROR_BAD_VALUE(visualizer, range[1] * 2);

            // minimum + 1 (not power of two)
            assertFalse(isPowOfTwo(range[0] + 1));
            checkSetCaptureSizeERROR_BAD_VALUE(visualizer, range[0] + 1);

            // maximum - 1 (not power of two)
            assertFalse(isPowOfTwo(range[1] - 1));
            checkSetCaptureSizeERROR_BAD_VALUE(visualizer, range[1] - 1);

            // medium (not power of two)
            int mid = (range[0] * 2) - 1;
            assertRange((range[0] + 1), (range[1] - 1), mid);
            assertFalse(isPowOfTwo(mid));
            checkSetCaptureSizeERROR_BAD_VALUE(visualizer, mid);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetCaptureSizeWhenEnabledWithInvalidParameters(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] range = Visualizer.getCaptureSizeRange();

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            // minimum / 2
            checkSetCaptureSizeIllegalStateException(visualizer, range[0] / 2);

            // maximum * 2
            checkSetCaptureSizeIllegalStateException(visualizer, range[1] * 2);

            // minimum + 1 (not power of two)
            assertFalse(isPowOfTwo(range[0] + 1));
            checkSetCaptureSizeIllegalStateException(visualizer, range[0] + 1);

            // maximum - 1 (not power of two)
            assertFalse(isPowOfTwo(range[1] - 1));
            checkSetCaptureSizeIllegalStateException(visualizer, range[1] - 1);

            // medium (not power of two)
            int mid = (range[0] * 2) - 1;
            assertRange((range[0] + 1), (range[1] - 1), mid);
            assertFalse(isPowOfTwo(mid));
            checkSetCaptureSizeIllegalStateException(visualizer, mid);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetCaptureSizeWhenEnabledWithValidParameters(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] range = Visualizer.getCaptureSizeRange();

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            for (int value = range[0] * 2; value <= range[1]; value *= 2) {
                checkSetCaptureSizeIllegalStateException(visualizer, value);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetCaptureSizeRangeCompat(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] range = visualizer.getCaptureSizeRangeCompat();

            assertEquals(128, range[0]);
            assertEquals(1024, range[1]);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetMaxCaptureRateCompat(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(20000, visualizer.getMaxCaptureRateCompat());
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkDefaultScalingModeCompat(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(IVisualizer.SCALING_MODE_NORMALIZED,
                    visualizer.getScalingModeCompat());
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetScalingModeCompatWithValidParams(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertFalse(visualizer.getEnabled());

            checkSetScalingModeCompatSUCCESS(visualizer,
                    IVisualizer.SCALING_MODE_NORMALIZED);

            checkSetScalingModeCompatSUCCESS(visualizer,
                    IVisualizer.SCALING_MODE_AS_PLAYED);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetScalingModeCompatWithInvalidParams(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertFalse(visualizer.getEnabled());

            checkSetScalingModeCompatERROR_BAD_VALUE(visualizer, -1);

            checkSetScalingModeCompatERROR_BAD_VALUE(visualizer, 2);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetScalingModeCompatWhenEnabledWithValidParams(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            checkSetScalingModeCompatSUCCESS(visualizer,
                    IVisualizer.SCALING_MODE_NORMALIZED);

            checkSetScalingModeCompatSUCCESS(visualizer,
                    IVisualizer.SCALING_MODE_AS_PLAYED);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetScalingModeCompatWhenEnabledWithInvalidParams(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            checkSetScalingModeCompatERROR_BAD_VALUE(visualizer, -1);

            checkSetScalingModeCompatERROR_BAD_VALUE(visualizer, 2);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkDefaultMeasurementModeCompat(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(IVisualizer.MEASUREMENT_MODE_NONE,
                    visualizer.getMeasurementModeCompat());
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetMeasurementModeCompatWithValidParams(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertFalse(visualizer.getEnabled());

            checkSetMeasurementModeCompatSUCCESS(visualizer,
                    IVisualizer.MEASUREMENT_MODE_NONE);

            checkSetMeasurementModeCompatSUCCESS(visualizer,
                    IVisualizer.MEASUREMENT_MODE_PEAK_RMS);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetMeasurementModeCompatWithInvalidParams(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertFalse(visualizer.getEnabled());

            checkSetMeasurementModeCompatERROR_BAD_VALUE(visualizer, -1);

            checkSetMeasurementModeCompatERROR_BAD_VALUE(visualizer, 2);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetMeasurementModeCompatWhenEnabledWithValidParams(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            checkSetMeasurementModeCompatSUCCESS(visualizer,
                    IVisualizer.MEASUREMENT_MODE_NONE);

            checkSetMeasurementModeCompatSUCCESS(visualizer,
                    IVisualizer.MEASUREMENT_MODE_PEAK_RMS);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetMeasurementModeCompatWhenEnabledWithInvalidParams(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            checkSetMeasurementModeCompatERROR_BAD_VALUE(visualizer, -1);

            checkSetMeasurementModeCompatERROR_BAD_VALUE(visualizer, 2);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetWaveForm(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            // modify volume settings
            // (StandardVisualizer is affected by system volume settings)
            fixMusicStreamVolume();

            {
                visualizer.setEnabled(true);
                assertTrue(visualizer.getEnabled());

                Thread.sleep(GET_AUDIO_CAPTURE_DATA_DELAY);

                byte[] waveform = new byte[visualizer.getCaptureSize()];
                int result = visualizer.getWaveForm(waveform);
                assertEquals(IVisualizer.SUCCESS, result);

                if (player.isPlaying()) {
                    checkArrayNotFilledWith((byte) 128, waveform);
                } else {
                    checkArrayFilledWith((byte) 128, waveform);
                }
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetWaveFormWhenDisabled(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertFalse(visualizer.getEnabled());

            byte[] waveform = new byte[visualizer.getCaptureSize()];
            try {
                int result = visualizer.getWaveForm(waveform);
                fail("result = " + result);
            } catch (IllegalStateException e) {
                // expected
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetWaveFormWithInvalidParameters(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] sizeRange = visualizer.getCaptureSizeRangeCompat();
            int validSize = visualizer.getCaptureSize();
            int[] sizeList = new int[] {
                    -1, // null
                    sizeRange[0] / 2, sizeRange[1] * 2, sizeRange[0] - 1,
                    sizeRange[1] + 1,
                    (validSize == sizeRange[0]) ? sizeRange[1] : sizeRange[0],
            };

            visualizer.setEnabled(true);
            assertTrue(visualizer.getEnabled());

            for (int size : sizeList) {
                byte[] waveform = (size < 0) ? null : new byte[size];
                int result = visualizer.getWaveForm(waveform);
                assertEquals(IVisualizer.ERROR_BAD_VALUE, result);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetWaveFormWhenDisabledWithInvalidParameters(
            IBasicMediaPlayer player) throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] sizeRange = visualizer.getCaptureSizeRangeCompat();
            int validSize = visualizer.getCaptureSize();
            int[] sizeList = new int[] {
                    -1, // null
                    sizeRange[0] / 2, sizeRange[1] * 2, sizeRange[0] - 1,
                    sizeRange[1] + 1,
                    (validSize == sizeRange[0]) ? sizeRange[1] : sizeRange[0],
            };

            assertFalse(visualizer.getEnabled());

            for (int size : sizeList) {
                byte[] waveform = (size < 0) ? null : new byte[size];
                int result = visualizer.getWaveForm(waveform);
                assertEquals(IVisualizer.ERROR_BAD_VALUE, result);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetFft(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            // modify volume settings
            // (StandardVisualizer is affected by system volume settings)
            fixMusicStreamVolume();

            visualizer.setEnabled(true);
            assertTrue(visualizer.getEnabled());

            Thread.sleep(GET_AUDIO_CAPTURE_DATA_DELAY);

            byte[] fft = new byte[visualizer.getCaptureSize()];
            int result = visualizer.getFft(fft);
            assertEquals(IVisualizer.SUCCESS, result);

            if (player.isPlaying()) {
                checkArrayNotFilledWith((byte) 0, fft);
            } else {
                assertEquals((byte) 0, fft[0]);
                checkArrayFilledWith((byte) 0, fft, 1, fft.length - 1);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetFftWhenDisabled(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertFalse(visualizer.getEnabled());

            byte[] fft = new byte[visualizer.getCaptureSize()];
            try {
                int result = visualizer.getFft(fft);
                fail("result = " + result);
            } catch (IllegalStateException e) {
                // expected
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetFftWithInvalidParameters(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] sizeRange = visualizer.getCaptureSizeRangeCompat();
            int validSize = visualizer.getCaptureSize();
            int[] sizeList = new int[] {
                    -1, // null
                    sizeRange[0] / 2, sizeRange[1] * 2, sizeRange[0] - 1,
                    sizeRange[1] + 1,
                    (validSize == sizeRange[0]) ? sizeRange[1] : sizeRange[0],
            };

            visualizer.setEnabled(true);
            assertTrue(visualizer.getEnabled());

            for (int size : sizeList) {
                byte[] fft = (size < 0) ? null : new byte[size];
                int result = visualizer.getFft(fft);
                assertEquals(IVisualizer.ERROR_BAD_VALUE, result);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetFftWhenDisabledWithInvalidParameters(
            IBasicMediaPlayer player) throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] sizeRange = visualizer.getCaptureSizeRangeCompat();
            int validSize = visualizer.getCaptureSize();
            int[] sizeList = new int[] {
                    -1, // null
                    sizeRange[0] / 2, sizeRange[1] * 2, sizeRange[0] - 1,
                    sizeRange[1] + 1,
                    (validSize == sizeRange[0]) ? sizeRange[1] : sizeRange[0],
            };

            assertFalse(visualizer.getEnabled());

            for (int size : sizeList) {
                byte[] fft = (size < 0) ? null : new byte[size];
                int result = visualizer.getFft(fft);
                assertEquals(IVisualizer.ERROR_BAD_VALUE, result);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetSamplingRate(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertFalse(visualizer.getEnabled());
            int samplingRate = visualizer.getSamplingRate();
            assertEquals(getExpectedSamplingRate(getContext()), samplingRate);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetSamplingRateWhenEnabled(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());
            int samplingRate = visualizer.getSamplingRate();
            assertEquals(getExpectedSamplingRate(getContext()), samplingRate);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerWaveFormOnly(
            IBasicMediaPlayer player) throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            int rate = visualizer.getMaxCaptureRateCompat();
            int expectedDataLen = visualizer.getCaptureSize();
            int expectedSamplingRate = getExpectedSamplingRate(getContext());

            Thread.sleep(GET_AUDIO_CAPTURE_DATA_DELAY);

            CapturedDataSet captured = getCaptureDatas(visualizer, rate, true,
                    false, DEFAULT_EVENT_WAIT_DURATION);

            assertNotNull(captured.waveform);
            assertEquals(expectedDataLen, captured.waveform.length);
            assertEquals(expectedSamplingRate, captured.waveformSamplingRate);

            assertNull(captured.fft);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerFftOnly(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            int rate = visualizer.getMaxCaptureRateCompat();
            int expectedDataLen = visualizer.getCaptureSize();
            int expectedSamplingRate = getExpectedSamplingRate(getContext());

            Thread.sleep(GET_AUDIO_CAPTURE_DATA_DELAY);

            CapturedDataSet captured = getCaptureDatas(visualizer, rate, false,
                    true, DEFAULT_EVENT_WAIT_DURATION);

            assertNull(captured.waveform);

            assertNotNull(captured.fft);
            assertEquals(expectedDataLen, captured.fft.length);
            assertEquals(expectedSamplingRate, captured.fftSamplingRate);

        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerBothWaveFormFft(
            IBasicMediaPlayer player) throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            int rate = visualizer.getMaxCaptureRateCompat();
            int expectedDataLen = visualizer.getCaptureSize();
            int expectedSamplingRate = getExpectedSamplingRate(getContext());

            Thread.sleep(GET_AUDIO_CAPTURE_DATA_DELAY);

            CapturedDataSet captured = getCaptureDatas(visualizer, rate, true,
                    true, DEFAULT_EVENT_WAIT_DURATION);

            assertNotNull(captured.waveform);
            assertEquals(expectedDataLen, captured.waveform.length);
            assertEquals(expectedSamplingRate, captured.waveformSamplingRate);

            assertNotNull(captured.fft);
            assertEquals(expectedDataLen, captured.fft.length);
            assertEquals(expectedSamplingRate, captured.fftSamplingRate);

        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerWithValidRateParameters(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            final int minRate = MIN_CAPTURE_RATE;
            final int maxRate = visualizer.getMaxCaptureRateCompat();
            IVisualizer.OnDataCaptureListener listener = new EmptyOnDataCaptureListenerObj();

            assertEquals(IVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(listener, minRate, true,
                            true));

            assertEquals(IVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(listener,
                            (minRate + maxRate) / 2, true, true));

            assertEquals(IVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(listener, minRate, true,
                            true));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerWithInvalidRateParameters(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            final int minRate = MIN_CAPTURE_RATE;
            final int maxRate = visualizer.getMaxCaptureRateCompat();
            IVisualizer.OnDataCaptureListener listener = new EmptyOnDataCaptureListenerObj();

            assertEquals(IVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener, maxRate + 1,
                            true, true));

            assertEquals(IVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener, minRate - 1,
                            true, true));

            assertEquals(IVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener,
                            Integer.MIN_VALUE, true, true));

            assertEquals(IVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener,
                            Integer.MAX_VALUE, true, true));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerWhenEnabled(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            final int maxRate = visualizer.getMaxCaptureRateCompat();
            IVisualizer.OnDataCaptureListener listener = new EmptyOnDataCaptureListenerObj();

            // with valid params
            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer.setDataCaptureListener(listener, maxRate, true,
                            true));

            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer.setDataCaptureListener(null, maxRate, false,
                            false));

            // with invalid params
            assertEquals(IVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener, maxRate + 1,
                            true, true));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerMeasureWaveFormCaptureRate(
            IBasicMediaPlayer player) throws InterruptedException,
            TimeoutException {
        IVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            DataCaptureRateMeasure measure = new DataCaptureRateMeasure();
            int maxRate = visualizer.getMaxCaptureRateCompat();

            {
                // @ max. rate
                checkWaveFormCaptureRate(visualizer, measure, maxRate);

                // @ 10 Hz
                checkWaveFormCaptureRate(visualizer, measure, 10000);

                // @ 1 Hz
                checkWaveFormCaptureRate(visualizer, measure, 1000);

                // @ 0.5 Hz
                checkWaveFormCaptureRate(visualizer, measure, 500);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerMeasureFftCaptureRate(
            IBasicMediaPlayer player) throws InterruptedException,
            TimeoutException {
        IVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            DataCaptureRateMeasure measure = new DataCaptureRateMeasure();
            int maxRate = visualizer.getMaxCaptureRateCompat();

            {
                // @ max. rate
                checkFftCaptureRate(visualizer, measure, maxRate);

                // @ 10 Hz
                checkFftCaptureRate(visualizer, measure, 10000);

                // @ 1 Hz
                checkFftCaptureRate(visualizer, measure, 1000);

                // @ 0.5 Hz
                checkFftCaptureRate(visualizer, measure, 500);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkCaptureSizeWorksAsExpected(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            int range[] = visualizer.getCaptureSizeRangeCompat();
            int rate = visualizer.getMaxCaptureRateCompat();

            for (int size = range[0]; size <= range[1]; size *= 2) {
                visualizer.setCaptureSize(size);

                CapturedDataSet captured = getCaptureDatas(visualizer, rate,
                        true, true, DEFAULT_EVENT_WAIT_DURATION);

                assertEquals(size, captured.waveform.length);
                assertEquals(size, captured.fft.length);
            }

        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetMeasurementPeakRmsCompatWithNull(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertFalse(visualizer.getEnabled());

            IVisualizer.MeasurementPeakRms measurement = null;

            assertEquals(IVisualizer.ERROR_BAD_VALUE,
                    visualizer.getMeasurementPeakRmsCompat(measurement));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetMeasurementPeakRmsCompatWhenEnabledWithNull(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            IVisualizer.MeasurementPeakRms measurement = null;

            assertEquals(IVisualizer.ERROR_BAD_VALUE,
                    visualizer.getMeasurementPeakRmsCompat(measurement));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetMeasurementPeakRmsCompatWhenMODE_NONE(
            IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertEquals(
                    IVisualizer.SUCCESS,
                    visualizer
                            .setMeasurementModeCompat(IVisualizer.MEASUREMENT_MODE_NONE));

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            IVisualizer.MeasurementPeakRms measurement = new IVisualizer.MeasurementPeakRms();

            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer.getMeasurementPeakRmsCompat(measurement));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetMeasurementPeakRmsCompatWorksAsExpected(
            IBasicMediaPlayer player) throws InterruptedException {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            fixMusicStreamVolume();

            assertEquals(
                    IVisualizer.SUCCESS,
                    visualizer
                            .setMeasurementModeCompat(IVisualizer.MEASUREMENT_MODE_PEAK_RMS));

            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            IVisualizer.MeasurementPeakRms measurement = new IVisualizer.MeasurementPeakRms();

            Thread.sleep(GET_MEASUREMENTS_DELAY);

            assertEquals(IVisualizer.SUCCESS,
                    visualizer.getMeasurementPeakRmsCompat(measurement));

            // -9600: -96 dB
            if (player.isPlaying()) {
                assertNotEquals(-9600, measurement.mPeak);
                assertNotEquals(-9600, measurement.mRms);
            } else {
                assertEquals(-9600, measurement.mPeak);
                assertEquals(-9600, measurement.mRms);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkAfterRelease(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int origSize = visualizer.getCaptureSize();

            // release
            visualizer.release();

            try {
                visualizer.getEnabled();
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.setEnabled(true);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.getSamplingRate();
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                byte[] fft = new byte[origSize];
                visualizer.getFft(fft);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                byte[] waveform = new byte[origSize];
                visualizer.getWaveForm(waveform);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.getCaptureSize();
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.setCaptureSize(origSize);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.setDataCaptureListener(null, 0, false, false);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.getCaptureSizeRangeCompat();
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.getMaxCaptureRateCompat();
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.getScalingModeCompat();
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer
                        .setScalingModeCompat(IVisualizer.SCALING_MODE_NORMALIZED);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.getMeasurementModeCompat();
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer
                        .setMeasurementModeCompat(IVisualizer.MEASUREMENT_MODE_NONE);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                IVisualizer.MeasurementPeakRms measurement = new IVisualizer.MeasurementPeakRms();
                visualizer.getMeasurementPeakRmsCompat(measurement);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            visualizer.release();
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkMultiInstanceBehavior(IBasicMediaPlayer player)
            throws InterruptedException {
        IVisualizer visualizer1 = null, visualizer2 = null;
        try {
            final CapturedDataSet captuerd1 = new CapturedDataSet();
            final CapturedDataSet captuerd2 = new CapturedDataSet();
            final CountDownLatch latch1 = new CountDownLatch(2);
            final CountDownLatch latch2 = new CountDownLatch(2);

            IVisualizer.OnDataCaptureListener listener1 = new IVisualizer.OnDataCaptureListener() {
                @Override
                public void onWaveFormDataCapture(IVisualizer visualizer,
                        byte[] waveform, int samplingRate) {
                    if (captuerd1.waveform == null) {
                        captuerd1.waveform = waveform.clone();
                        latch1.countDown();
                    }
                }

                @Override
                public void onFftDataCapture(IVisualizer visualizer,
                        byte[] fft, int samplingRate) {
                    if (captuerd1.fft == null) {
                        captuerd1.fft = fft.clone();
                        latch1.countDown();
                    }
                }
            };

            IVisualizer.OnDataCaptureListener listener2 = new IVisualizer.OnDataCaptureListener() {
                @Override
                public void onWaveFormDataCapture(IVisualizer visualizer,
                        byte[] waveform, int samplingRate) {
                    if (captuerd2.waveform == null) {
                        captuerd2.waveform = waveform.clone();
                        latch2.countDown();
                    }
                }

                @Override
                public void onFftDataCapture(IVisualizer visualizer,
                        byte[] fft, int samplingRate) {
                    if (captuerd2.fft == null) {
                        captuerd2.fft = fft.clone();
                        latch2.countDown();
                    }
                }
            };

            // create visualizer 1
            visualizer1 = createVisualizer(player);

            assertEquals(
                    IVisualizer.SUCCESS,
                    visualizer1.setDataCaptureListener(listener1,
                            visualizer1.getMaxCaptureRateCompat(), true, true));

            // create visualizer 2
            visualizer2 = createVisualizer(player);

            assertEquals(
                    IVisualizer.SUCCESS,
                    visualizer2.setDataCaptureListener(listener2,
                            visualizer2.getMaxCaptureRateCompat(), true, true));

            int[] sizeRange = visualizer2.getCaptureSizeRangeCompat();
            int size2 = sizeRange[0] * 2;

            assertNotEquals(DEFAULT_CAPTURE_SIZE, size2);

            assertFalse(visualizer1.getEnabled());
            assertFalse(visualizer2.getEnabled());

            // tweak capture size (vis1: min., vis2: max.)
            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setCaptureSize(size2));
            assertEquals(IVisualizer.SUCCESS, visualizer2.setCaptureSize(size2));

            // XXX StandardVisualizer fails on this assertion,
            // visualizer1.getCaptureSize() returns DEFAULT_CAPTURE_SIZE
            assertEquals(size2, visualizer1.getCaptureSize());
            assertEquals(size2, visualizer2.getCaptureSize());

            // start capturing

            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setEnabled(true));
            assertEquals(IVisualizer.SUCCESS, visualizer2.setEnabled(true));

            assertTrue(latch2.await(SHORT_EVENT_WAIT_DURATION,
                    TimeUnit.MILLISECONDS));

            assertFalse(latch1.await(SHORT_EVENT_WAIT_DURATION,
                    TimeUnit.MILLISECONDS));

            // check captured results
            assertEquals(2, latch1.getCount());
            assertNull(captuerd1.waveform);
            assertNull(captuerd1.fft);

            assertEquals(0, latch2.getCount());
            assertEquals(size2, captuerd2.waveform.length);
            assertEquals(size2, captuerd2.fft.length);

            // release visualizer 2
            visualizer2.release();
            visualizer2 = null;

            // check visualizer 1 now gains control

            assertTrue(visualizer1.getEnabled());

            assertTrue(latch1.await(SHORT_EVENT_WAIT_DURATION,
                    TimeUnit.MILLISECONDS));

            assertEquals(0, latch1.getCount());
            assertEquals(size2, captuerd1.waveform.length);
            assertEquals(size2, captuerd1.fft.length);

            // release visualizer 1
            visualizer1.release();
            visualizer1 = null;
        } finally {
            releaseQuietly(visualizer1);
            visualizer1 = null;
            releaseQuietly(visualizer2);
            visualizer2 = null;
        }
    }

    private void checkPlayerReleasedBeforeEffect(IBasicMediaPlayer player) {
        IVisualizer visualizer = null;
        try {
            visualizer = createVisualizer(player);

            final int origSize = visualizer.getCaptureSize();

            // release player
            player.release();
            player = null;

            // post check

            // enabled = false
            visualizer.getEnabled();

            visualizer.setCaptureSize(origSize);
            visualizer.setDataCaptureListener(null, 0, false, false);

            // enabled = true
            visualizer.setEnabled(true);
            visualizer.getSamplingRate();
            visualizer.getCaptureSize();
            visualizer.getCaptureSizeRangeCompat();
            visualizer.getMaxCaptureRateCompat();
            visualizer.getScalingModeCompat();
            visualizer
                    .setScalingModeCompat(IVisualizer.SCALING_MODE_NORMALIZED);
            visualizer.getMeasurementModeCompat();
            visualizer
                    .setMeasurementModeCompat(IVisualizer.MEASUREMENT_MODE_NONE);
            try {
                byte[] fft = new byte[origSize];
                visualizer.getFft(fft);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }
            try {
                byte[] waveform = new byte[origSize];
                visualizer.getWaveForm(waveform);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }
            try {
                IVisualizer.MeasurementPeakRms measurement = new IVisualizer.MeasurementPeakRms();
                visualizer.getMeasurementPeakRmsCompat(measurement);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            visualizer.release();
        } finally {
            releaseQuietly(visualizer);
            visualizer = null;
        }
    }

    private void checkAfterControlLost(IBasicMediaPlayer player) {
        final boolean isStandardVisualizer = unwrap(player) instanceof StandardMediaPlayer;
        IVisualizer visualizer1 = null, visualizer2 = null;

        // When disabled

        try {
            visualizer1 = createVisualizer(player);

            final int origSize = visualizer1.getCaptureSize();

            // create instance 2
            visualizer2 = createVisualizer(player);

            // state changing methods should return ERROR_INVALID_OPERATION

            // XXX StandardVisualizer returns SUCCESS when enabled == false
            if (isStandardVisualizer) {
                Log.w("checkAfterControlLost", "ASSERTION SKIPPED for "
                        + player.getClass().getSimpleName());
            } else {
                assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                        visualizer1.setEnabled(false));
            }

            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setEnabled(true));

            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setCaptureSize(origSize));

            // XXX StandardVisualizer returns SUCCESS
            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setDataCaptureListener(null, 0, false, false));
        } finally {
            releaseQuietly(visualizer1);
            visualizer1 = null;
            releaseQuietly(visualizer2);
            visualizer2 = null;
        }

        // When enabled
        try {
            visualizer1 = createVisualizer(player);

            visualizer1.setEnabled(true);

            final int origSize = visualizer1.getCaptureSize();
            final int origSamplingRate = visualizer1.getSamplingRate();
            final int[] origRange = visualizer1.getCaptureSizeRangeCompat();
            final int origMaxCapRate = visualizer1.getMaxCaptureRateCompat();
            final int origScalingMode = visualizer1.getScalingModeCompat();
            final int origMeasurementMode = visualizer1
                    .getMeasurementModeCompat();
            final byte[] buffer = new byte[origSize];

            // create instance 2
            visualizer2 = createVisualizer(player);

            // no state changing methods should not raise any errors
            assertEquals(true, visualizer1.getEnabled());
            assertEquals(origSamplingRate, visualizer1.getSamplingRate());
            assertEquals(origRange[0],
                    visualizer1.getCaptureSizeRangeCompat()[0]);
            assertEquals(origRange[1],
                    visualizer1.getCaptureSizeRangeCompat()[1]);
            assertEquals(origMaxCapRate, visualizer1.getMaxCaptureRateCompat());
            assertEquals(origScalingMode, visualizer1.getScalingModeCompat());
            assertEquals(origMeasurementMode,
                    visualizer1.getMeasurementModeCompat());
            assertEquals(origSize, visualizer1.getCaptureSize());

            // XXX StandardVisualizer returns SUCCESS when enabled == true
            if (isStandardVisualizer) {
                Log.w("checkAfterControlLost", "ASSERTION SKIPPED for "
                        + player.getClass().getSimpleName());
            } else {
                assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                        visualizer1.setEnabled(true));
            }

            // state changing methods should return ERROR_INVALID_OPERATION
            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setEnabled(false));

            assertEquals(
                    IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1
                            .setScalingModeCompat(IVisualizer.SCALING_MODE_NORMALIZED));

            assertEquals(
                    IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1
                            .setMeasurementModeCompat(IVisualizer.MEASUREMENT_MODE_NONE));

            {
                IVisualizer.MeasurementPeakRms measurement = new IVisualizer.MeasurementPeakRms();
                assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                        visualizer1.getMeasurementPeakRmsCompat(measurement));
            }

            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.getFft(buffer));

            assertEquals(IVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.getWaveForm(buffer));
        } finally {
            releaseQuietly(visualizer1);
            visualizer1 = null;
            releaseQuietly(visualizer2);
            visualizer2 = null;
        }
    }

    private static final class VisualizerStatus {

        public boolean enabled;
        public int captureSize;
        public int scalingMode;
        public int measurementMode;

        public VisualizerStatus(IVisualizer visualizer) {
            enabled = visualizer.getEnabled();
            captureSize = visualizer.getCaptureSize();
            scalingMode = visualizer.getScalingModeCompat();
            measurementMode = visualizer.getMeasurementModeCompat();
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj)
                return true;
            if (obj == null)
                return false;
            if (getClass() != obj.getClass())
                return false;

            VisualizerStatus other = (VisualizerStatus) obj;
            if (captureSize != other.captureSize)
                return false;
            if (enabled != other.enabled)
                return false;
            if (measurementMode != other.measurementMode)
                return false;
            if (scalingMode != other.scalingMode)
                return false;
            return true;
        }

        @Override
        public String toString() {
            return "VisualizerStatus [enabled=" + enabled + ", captureSize="
                    + captureSize + ", scalingMode=" + scalingMode
                    + ", measurementMode=" + measurementMode + "]";
        }
    }

    private static void assertVisualizerState(VisualizerStatus expected,
            IVisualizer visualizer) {
        VisualizerStatus actual = new VisualizerStatus(visualizer);
        assertEquals(expected, actual);
    }

    public void testPlayerStateTransition() throws Exception {
        IBasicMediaPlayer player = null;
        IVisualizer visualizer = null;

        try {
            // check effect settings are preserved along player state transition
            Object waitObj = new Object();
            CompletionListenerObject comp = new CompletionListenerObject(waitObj);
            SeekCompleteListenerObject seekComp = new SeekCompleteListenerObject(waitObj);

            player = createWrappedPlayerInstance();
            visualizer = createVisualizer(unwrap(player));

            player.setOnCompletionListener(comp);
            player.setOnSeekCompleteListener(seekComp);

            // configure
            assertEquals(
                    IVisualizer.SUCCESS,
                    visualizer
                            .setMeasurementModeCompat(IVisualizer.MEASUREMENT_MODE_PEAK_RMS));
            assertEquals(
                    IVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(
                            new IVisualizer.OnDataCaptureListener() {
                                @Override
                                public void onWaveFormDataCapture(
                                        IVisualizer visualizer,
                                        byte[] waveform, int samplingRate) {
                                }

                                @Override
                                public void onFftDataCapture(
                                        IVisualizer visualizer, byte[] fft,
                                        int samplingRate) {
                                }
                            }, visualizer.getMaxCaptureRateCompat(), true, true));
            assertEquals(IVisualizer.SUCCESS, visualizer.setEnabled(true));

            VisualizerStatus expectedStatus = new VisualizerStatus(visualizer);

            // player: idle

            // player: initialized
            setDataSourceForCommonTests(player, null);
            assertVisualizerState(expectedStatus, visualizer);

            // player: prepared
            player.prepare();

            assertTrue(visualizer.getEnabled());
            assertVisualizerState(expectedStatus, visualizer);

            // player: started
            player.start();

            assertTrue(visualizer.getEnabled());
            assertVisualizerState(expectedStatus, visualizer);

            // player: paused
            player.pause();

            assertTrue(visualizer.getEnabled());
            assertVisualizerState(expectedStatus, visualizer);

            // player: playback completed
            player.seekTo(player.getDuration());
            if (!seekComp.await(DEFAULT_EVENT_WAIT_DURATION)) {
                fail();
            }
            player.start();
            // XXX This assertion fails on Android 5.0 with StandardMediaPlayer
            if (!comp.await(SHORT_EVENT_WAIT_DURATION)) {
                fail();
            }

            assertTrue(visualizer.getEnabled());
            assertVisualizerState(expectedStatus, visualizer);

            // player: stop
            player.stop();

            assertTrue(visualizer.getEnabled());
            assertVisualizerState(expectedStatus, visualizer);

            // player: idle
            player.reset();

            assertTrue(visualizer.getEnabled());
            assertVisualizerState(expectedStatus, visualizer);

            // player: end
            player.release();
            player = null;

            assertTrue(visualizer.getEnabled());
            assertVisualizerState(expectedStatus, visualizer);
        } finally {
            releaseQuietly(player);
            releaseQuietly(visualizer);
        }
    }

    //
    // Utilities
    //

    private IVisualizer createVisualizer(IBasicMediaPlayer player) {
        return TestVisualizerWrapper.create(this, getFactory(), player);
    }

    private static boolean isPowOfTwo(int x) {
        return (x != 0) && ((x & (x - 1)) == 0);
    }

    private static void checkSetCaptureSizeSUCCESS(IVisualizer visualizer,
            int size) {
        assertTrue(isPowOfTwo(size));
        assertEquals(IVisualizer.SUCCESS, visualizer.setCaptureSize(size));
        assertEquals(size, visualizer.getCaptureSize());
    }

    private static void checkSetCaptureSizeERROR_BAD_VALUE(
            IVisualizer visualizer, int size) {
        int origSize = visualizer.getCaptureSize();
        assertEquals(IVisualizer.ERROR_BAD_VALUE,
                visualizer.setCaptureSize(size));
        assertEquals(origSize, visualizer.getCaptureSize());
    }

    private static void checkSetCaptureSizeIllegalStateException(
            IVisualizer visualizer, int size) {
        int origSize = visualizer.getCaptureSize();

        try {
            int result = visualizer.setCaptureSize(size);
            fail("result = " + result);
        } catch (IllegalStateException e) {
            // expected
        }

        assertEquals(origSize, visualizer.getCaptureSize());
    }

    private static void checkSetScalingModeCompatSUCCESS(
            IVisualizer visualizer, int mode) {
        assertEquals(IVisualizer.SUCCESS, visualizer.setScalingModeCompat(mode));
        assertEquals(mode, visualizer.getScalingModeCompat());
    }

    private static void checkSetScalingModeCompatERROR_BAD_VALUE(
            IVisualizer visualizer, int mode) {
        int origMode = visualizer.getScalingModeCompat();
        assertEquals(IVisualizer.ERROR_BAD_VALUE,
                visualizer.setScalingModeCompat(mode));
        assertEquals(origMode, visualizer.getScalingModeCompat());
    }

    private static void checkSetMeasurementModeCompatSUCCESS(
            IVisualizer visualizer, int mode) {
        assertEquals(IVisualizer.SUCCESS,
                visualizer.setMeasurementModeCompat(mode));
        assertEquals(mode, visualizer.getMeasurementModeCompat());
    }

    private static void checkSetMeasurementModeCompatERROR_BAD_VALUE(
            IVisualizer visualizer, int mode) {
        int origMode = visualizer.getMeasurementModeCompat();
        assertEquals(IVisualizer.ERROR_BAD_VALUE,
                visualizer.setMeasurementModeCompat(mode));
        assertEquals(origMode, visualizer.getMeasurementModeCompat());
    }

    private static interface BasicMediaPlayerTestRunnable {
        public void run(IBasicMediaPlayer player, Object args) throws Throwable;
    }

    private void checkWithNoPlayerErrors(TestParams params,
            BasicMediaPlayerTestRunnable checkProcess) throws Throwable {
        IBasicMediaPlayer player = null;

        try {
            player = createWrappedPlayerInstance();
            transitState(params.getPlayerState(), player, null);

            Object sharedSyncObj = new Object();
            ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj,
                    false);

            // set callbacks
            player.setOnErrorListener(err);

            // check
            checkProcess.run(unwrap(player), null);

            // expects no errors
            assertFalse(err.occurred());
        } finally {
            releaseQuietly(player);
        }
    }

    private static final class CapturedDataSet {
        public byte[] waveform;
        public int waveformSamplingRate = -1;
        public byte[] fft;
        public int fftSamplingRate = -1;
    }

    private static CapturedDataSet getCaptureDatas(IVisualizer visualizer,
            int rate, boolean waveform, boolean fft, int timeoutMillis)
            throws InterruptedException {
        int numCounts = 0;
        final CapturedDataSet captured = new CapturedDataSet();

        if (fft) {
            numCounts++;
        }
        if (waveform) {
            numCounts++;
        }

        if (numCounts == 0)
            return captured;

        final CountDownLatch latch = new CountDownLatch(numCounts);

        visualizer.setEnabled(false);
        visualizer.setDataCaptureListener(
                new IVisualizer.OnDataCaptureListener() {
                    @Override
                    public void onWaveFormDataCapture(IVisualizer visualizer,
                            byte[] waveform, int samplingRate) {
                        if (captured.waveform == null) {
                            captured.waveform = waveform.clone();
                            captured.waveformSamplingRate = samplingRate;
                            latch.countDown();
                        }
                    }

                    @Override
                    public void onFftDataCapture(IVisualizer visualizer,
                            byte[] fft, int samplingRate) {
                        if (captured.fft == null) {
                            captured.fft = fft.clone();
                            captured.fftSamplingRate = samplingRate;
                            latch.countDown();
                        }
                    }
                }, rate, waveform, fft);

        visualizer.setEnabled(true);
        latch.await(timeoutMillis, TimeUnit.MILLISECONDS);
        visualizer.setEnabled(false);

        return captured;
    }

    private static void checkArrayFilledWith(byte excepted, byte[] array) {
        checkArrayFilledWith(excepted, array, 0, array.length);
    }

    private static void checkArrayFilledWith(byte excepted, byte[] array,
            int offset, int count) {
        int sameCnt = 0;
        for (int i = 0; i < count; i++) {
            if (array[offset + i] == excepted) {
                sameCnt++;
            }
        }
        assertEquals(count, sameCnt);
    }

    private static void checkArrayNotFilledWith(byte excepted, byte[] array) {
        checkArrayNotFilledWith(excepted, array, 0, array.length);
    }

    private static void checkArrayNotFilledWith(byte excepted, byte[] array,
            int offset, int count) {
        int sameCnt = 0;
        for (int i = 0; i < count; i++) {
            if (array[offset + i] == excepted) {
                sameCnt++;
            }
        }
        assertNotEquals(count, sameCnt);
    }

    private static final class DataCaptureRateMeasure implements
            IVisualizer.OnDataCaptureListener {

        public DataCaptureRateMeasure() {
        }

        @Override
        public void onWaveFormDataCapture(IVisualizer visualizer,
                byte[] waveform, int samplingRate) {
            if (!mWaveform.sample()) {
                mLatch.countDown();
                return;
            }
        }

        @Override
        public void onFftDataCapture(IVisualizer visualizer, byte[] fft,
                int samplingRate) {
            if (!mFft.sample()) {
                mLatch.countDown();
                return;
            }
        }

        public void measure(IVisualizer visualizer, int timeoutMs)
                throws InterruptedException, TimeoutException {
            mMeasureStartTime = System.nanoTime();

            mWaveform.reset();
            mFft.reset();

            mLatch = new CountDownLatch(1);

            if (visualizer.setEnabled(true) != IVisualizer.SUCCESS)
                throw new IllegalStateException(
                        "setEnabled(true) returns an error");

            if (!mLatch.await(timeoutMs, TimeUnit.MILLISECONDS))
                throw new TimeoutException();

            if (visualizer.setEnabled(false) != IVisualizer.SUCCESS)
                throw new IllegalStateException(
                        "setEnabled(false) returns an error");

            if (mWaveform.analyze())
                Log.d("DataCaptureRateMeasure-WaveForm",
                        mWaveform.summaryText());

            if (mFft.analyze())
                Log.d("DataCaptureRateMeasure-FFT", mFft.summaryText());
        }

        private static long diffTime(long t1, long t2) {
            if (t2 >= t1)
                return (t2 - t1);
            else
                return (Long.MAX_VALUE - t1) + t2 + 1;
        }

        public int getWaveFormCaptureRate() {
            return mWaveform.rate;
        }

        public int getFftCaptureRate() {
            return mFft.rate;
        }

        private static final int NUM_MEASURE_POINTS = 10;

        private static final class MeasuredData {
            int rate; // [milli hertz]
            int avgInterval; // [micro sec.]
            int minInterval; // [micro sec.]
            int maxInterval; // [micro sec.]
            int intervalDeviation; // [micro sec.]

            long[] capturedTimestamp = new long[NUM_MEASURE_POINTS];
            int capturedCount;

            public void reset() {
                rate = 0;
                avgInterval = 0;
                minInterval = 0;
                maxInterval = 0;
                intervalDeviation = 0;
                capturedCount = 0;
                Arrays.fill(capturedTimestamp, 0);
            }

            public String summaryText() {
                return "rate = "
                        + rate
                        + ", interval = ("
                        + minInterval
                        + ", "
                        + avgInterval
                        + ", "
                        + maxInterval
                        + "), "
                        + "intervalDeviation = "
                        + intervalDeviation
                        + " ("
                        + String.format("%.2f",
                                (100. * intervalDeviation / avgInterval))
                        + " %)";
            }

            public boolean sample() {
                if (capturedCount == NUM_MEASURE_POINTS)
                    return false;

                capturedTimestamp[capturedCount] = System.nanoTime();
                capturedCount++;
                return true;
            }

            public boolean analyze() {
                if (capturedCount < 3)
                    return false;

                long[] diff = new long[capturedCount - 1];
                long totalDiff = 0;
                long minIntervalNs = Long.MAX_VALUE;
                long maxIntervalNs = Long.MIN_VALUE;
                for (int i = 0; i < (capturedCount - 1); i++) {
                    diff[i] = diffTime(capturedTimestamp[i],
                            capturedTimestamp[i + 1]);

                    totalDiff += diff[i];

                    minIntervalNs = Math.min(minIntervalNs, diff[i]);
                    maxIntervalNs = Math.max(maxIntervalNs, diff[i]);
                }

                double averageIntervalNs = totalDiff / diff.length;

                if (averageIntervalNs == 0.0)
                    return false;

                double unbiasedVariance = 0;
                for (int i = 0; i < diff.length; i++) {
                    double d = (averageIntervalNs - diff[i]) / 1000.;
                    unbiasedVariance += d * d;
                }
                unbiasedVariance /= (diff.length - 1);

                double freqHz = 1000000000. / averageIntervalNs;

                rate = (int) (freqHz * 1000);
                avgInterval = (int) (averageIntervalNs / 1000);
                minInterval = (int) (minIntervalNs / 1000);
                maxInterval = (int) (maxIntervalNs / 1000);
                intervalDeviation = (int) (Math.sqrt(unbiasedVariance));

                return true;
            }
        }

        @SuppressWarnings("unused")
        private long mMeasureStartTime;
        MeasuredData mWaveform = new MeasuredData();
        MeasuredData mFft = new MeasuredData();
        private CountDownLatch mLatch;
    }

    private static void checkWaveFormCaptureRate(IVisualizer visualizer,
            DataCaptureRateMeasure measure, int rate)
            throws InterruptedException, TimeoutException {
        {
            int timeout = (1000000 / rate)
                    * DataCaptureRateMeasure.NUM_MEASURE_POINTS * 2;

            assertEquals(IVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(measure, rate, true,
                            false));

            measure.measure(visualizer, timeout);

            int measuredRate = measure.getWaveFormCaptureRate();
            int tolerance = calcRateTolerance(rate);

            double error = (1.0 * (measuredRate - rate) / rate);

            Log.d("checkWaveFormCaptureRate",
                    "expected: " + rate + ", measured: " + measuredRate
                            + "  (error: "
                            + String.format("%.2f", (error * 100)) + " %)");

            // XXX This assertion may be fail when using StandardMediaPlayer
            assertRange(rate - tolerance, rate + tolerance, measuredRate);
        }
    }

    private static void checkFftCaptureRate(IVisualizer visualizer,
            DataCaptureRateMeasure measure, int rate)
            throws InterruptedException, TimeoutException {
        {
            int timeout = (1000000 / rate)
                    * DataCaptureRateMeasure.NUM_MEASURE_POINTS * 2;

            assertEquals(IVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(measure, rate, false,
                            true));

            measure.measure(visualizer, timeout);

            int measuredRate = measure.getFftCaptureRate();
            int tolerance = calcRateTolerance(rate);

            double error = (1.0 * (measuredRate - rate) / rate);

            Log.d("checkFftCaptureRate", "expected: " + rate + ", measured: "
                    + measuredRate + "  (error: " + (error * 100) + " %)");

            // XXX This assertion may be fail when using StandardMediaPlayer
            assertRange(rate - tolerance, rate + tolerance, measuredRate);
        }
    }

    private static int calcRateTolerance(int rate) {
        // 5 % (min: 0.2 Hz, max:2.0 Hz)
        return Math.min(Math.max((int) (rate * 0.05), 200), 2000);
    }

    private void fixMusicStreamVolume() {
        // Change STREAM_MUSIC volume if muted
        AudioManager am = (AudioManager) getContext().getSystemService(
                Context.AUDIO_SERVICE);
        if (am.getStreamVolume(AudioManager.STREAM_MUSIC) == 0) {
            am.setStreamVolume(AudioManager.STREAM_MUSIC, 1, 0);
        }
    }

    private static int getExpectedSamplingRate(Context context) {
        int outputSampleRate = 44100000;
        try {
            AudioManager am = (AudioManager) context
                    .getSystemService(Context.AUDIO_SERVICE);
            Method m = AudioManager.class.getMethod("getProperty",
                    String.class);
            String strOutputSampleRate = (String) (m.invoke(am,
                    "android.media.property.OUTPUT_SAMPLE_RATE"));
            outputSampleRate = Integer.parseInt(strOutputSampleRate) * 1000;
        } catch (NoSuchMethodException e) {
        } catch (IllegalAccessException e) {
        } catch (IllegalArgumentException e) {
        } catch (InvocationTargetException e) {
        }
        return outputSampleRate;
    }
}
