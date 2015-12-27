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
import com.h6ah4i.android.media.audiofx.IHQVisualizer;
import com.h6ah4i.android.media.standard.StandardMediaPlayer;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.test.base.TestHQVisualizerWrapper;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.EmptyOnDataCaptureListenerObj;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.media.test.utils.SeekCompleteListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class HQVisualizerTestCase extends BasicMediaPlayerTestCaseBase {
    private static final int GET_AUDIO_CAPTURE_DATA_DELAY = 200;
    private static final int DEFAULT_CAPTURE_SIZE = 1024;

    // NOTE: StandardVisualizer and OpenSLVisualizer specific value.
    private static final int MIN_CAPTURE_RATE = 100; // milli herts

    static final int[] WINDOW_TYPES = new int[] {
            IHQVisualizer.WINDOW_RECTANGULAR,
            IHQVisualizer.WINDOW_RECTANGULAR | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            IHQVisualizer.WINDOW_HANN,
            IHQVisualizer.WINDOW_HANN | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            IHQVisualizer.WINDOW_HAMMING,
            IHQVisualizer.WINDOW_HAMMING | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            IHQVisualizer.WINDOW_BLACKMAN,
            IHQVisualizer.WINDOW_BLACKMAN | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            IHQVisualizer.WINDOW_FLAT_TOP,
            IHQVisualizer.WINDOW_FLAT_TOP | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
    };

    static final int[] INVALID_WINDOW_TYPES = new int[] {
            (IHQVisualizer.WINDOW_FLAT_TOP + 1),
            (IHQVisualizer.WINDOW_FLAT_TOP + 1) | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            -1,
    };

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
                HQVisualizerTestCase.class, params, filter, false));

        // not parameterized tests
        suite.addTest(makeSingleBasicTest(
                HQVisualizerTestCase.class, "testPlayerStateTransition", factoryClazz));

        return suite;
    }

    public HQVisualizerTestCase(ParameterizedTestArgs args) {
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

    public void testSetCaptureSizeWithInvalidParameters() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetCaptureSizeWithInvalidParameters(player);
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

    public void testGetNumChannels() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetNumChannels(player);
            }
        });
    }

    public void testGetNumChannelsWhenEnabled() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetNumChannelsWhenEnabled(player);
            }
        });
    }

    public void testGetDefaultWindowFunction() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkGetDefaultWindowFunction(player);
            }
        });
    }

    public void testSetWindowFunction() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetWindowFunction(player);
            }
        });
    }

    public void testSetWindowFunctionWhenEnabled() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetWindowFunctionWhenEnabled(player);
            }
        });
    }

    public void testSetWindowFunctionWithInvalidParams() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetWindowFunctionWithInvalidParams(player);
            }
        });
    }

    public void testSetWindowFunctionWhenEnabledWithInvalidParams() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(params, new BasicMediaPlayerTestRunnable() {
            @Override
            public void run(IBasicMediaPlayer player, Object args)
                    throws Throwable {
                checkSetWindowFunctionWhenEnabledWithInvalidParams(player);
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
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(DEFAULT_CAPTURE_SIZE, visualizer.getCaptureSize());
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetAndGetEnabled(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertFalse(visualizer.getEnabled());

            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(false));
            assertFalse(visualizer.getEnabled());
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetCaptureSizeWithVaildParameters(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

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

    private void checkSetCaptureSizeWithInvalidParameters(
            IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

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
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] range = Visualizer.getCaptureSizeRange();

            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));
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
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] range = Visualizer.getCaptureSizeRange();

            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            for (int value = range[0] * 2; value <= range[1]; value *= 2) {
                checkSetCaptureSizeIllegalStateException(visualizer, value);
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetCaptureSizeRangeCompat(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            int[] range = visualizer.getCaptureSizeRange();

            assertEquals(128, range[0]);
            assertEquals(8192, range[1]);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetMaxCaptureRateCompat(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(60000, visualizer.getMaxCaptureRate());
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetSamplingRate(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

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
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());
            int samplingRate = visualizer.getSamplingRate();
            assertEquals(getExpectedSamplingRate(getContext()), samplingRate);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetNumChannels(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertFalse(visualizer.getEnabled());
            int numChannels = visualizer.getNumChannels();
            assertEquals(2, numChannels);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetNumChannelsWhenEnabled(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());
            int numChannels = visualizer.getNumChannels();
            assertEquals(2, numChannels);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkGetDefaultWindowFunction(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertFalse(visualizer.getEnabled());
            int windowType = visualizer.getWindowFunction();
            assertEquals(IHQVisualizer.WINDOW_RECTANGULAR, windowType);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetWindowFunction(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertFalse(visualizer.getEnabled());

            for (int wt : WINDOW_TYPES) {
                assertEquals(IHQVisualizer.SUCCESS, visualizer.setWindowFunction(wt));
                assertEquals(wt, visualizer.getWindowFunction());
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetWindowFunctionWhenEnabled(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            for (int wt : WINDOW_TYPES) {
                assertEquals(IHQVisualizer.SUCCESS, visualizer.setWindowFunction(wt));
                assertEquals(wt, visualizer.getWindowFunction());
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetWindowFunctionWithInvalidParams(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertFalse(visualizer.getEnabled());

            for (int wt : WINDOW_TYPES) {
                assertEquals(IHQVisualizer.ERROR_BAD_VALUE, visualizer.setWindowFunction(wt));
                assertEquals(IHQVisualizer.WINDOW_RECTANGULAR, visualizer.getWindowFunction());
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetWindowFunctionWhenEnabledWithInvalidParams(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);
            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            for (int wt : INVALID_WINDOW_TYPES) {
                assertEquals(IHQVisualizer.ERROR_BAD_VALUE, visualizer.setWindowFunction(wt));
                assertEquals(IHQVisualizer.WINDOW_RECTANGULAR, visualizer.getWindowFunction());
            }
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerWaveFormOnly(
            IBasicMediaPlayer player) throws InterruptedException {
        IHQVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            int rate = visualizer.getMaxCaptureRate();
            int expectedNumChannels = visualizer.getNumChannels();
            int expectedDataLen = visualizer.getCaptureSize() * expectedNumChannels;
            int expectedSamplingRate = getExpectedSamplingRate(getContext());

            Thread.sleep(GET_AUDIO_CAPTURE_DATA_DELAY);

            CapturedDataSet captured = getCaptureDatas(visualizer, rate, true,
                    false, DEFAULT_EVENT_WAIT_DURATION);

            assertNotNull(captured.waveform);
            assertEquals(expectedDataLen, captured.waveform.length);
            assertEquals(expectedNumChannels, captured.waveformNumChannels);
            assertEquals(expectedSamplingRate, captured.waveformSamplingRate);

            assertNull(captured.fft);
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerFftOnly(IBasicMediaPlayer player)
            throws InterruptedException {
        IHQVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            int rate = visualizer.getMaxCaptureRate();
            int expectedNumChannels = visualizer.getNumChannels();
            int expectedDataLen = visualizer.getCaptureSize() * expectedNumChannels;
            int expectedSamplingRate = getExpectedSamplingRate(getContext());

            Thread.sleep(GET_AUDIO_CAPTURE_DATA_DELAY);

            CapturedDataSet captured = getCaptureDatas(visualizer, rate, false,
                    true, DEFAULT_EVENT_WAIT_DURATION);

            assertNull(captured.waveform);

            assertNotNull(captured.fft);
            assertEquals(expectedDataLen, captured.fft.length);
            assertEquals(expectedNumChannels, captured.fftNumChannels);
            assertEquals(expectedSamplingRate, captured.fftSamplingRate);

        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerBothWaveFormFft(
            IBasicMediaPlayer player) throws InterruptedException {
        IHQVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            int rate = visualizer.getMaxCaptureRate();
            int expectedNumChannels = visualizer.getNumChannels();
            int expectedDataLen = visualizer.getCaptureSize() * expectedNumChannels;
            int expectedSamplingRate = getExpectedSamplingRate(getContext());

            Thread.sleep(GET_AUDIO_CAPTURE_DATA_DELAY);

            CapturedDataSet captured = getCaptureDatas(visualizer, rate, true,
                    true, DEFAULT_EVENT_WAIT_DURATION);

            assertNotNull(captured.waveform);
            assertEquals(expectedDataLen, captured.waveform.length);
            assertEquals(expectedNumChannels, captured.waveformNumChannels);
            assertEquals(expectedSamplingRate, captured.waveformSamplingRate);

            assertNotNull(captured.fft);
            assertEquals(expectedDataLen, captured.fft.length);
            assertEquals(expectedNumChannels, captured.fftNumChannels);
            assertEquals(expectedSamplingRate, captured.fftSamplingRate);

        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerWithValidRateParameters(
            IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            final int minRate = MIN_CAPTURE_RATE;
            final int maxRate = visualizer.getMaxCaptureRate();
            IHQVisualizer.OnDataCaptureListener listener = new EmptyOnDataCaptureListenerObj();

            assertEquals(IHQVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(listener, minRate, true,
                            true));

            assertEquals(IHQVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(listener,
                            (minRate + maxRate) / 2, true, true));

            assertEquals(IHQVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(listener, minRate, true,
                            true));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerWithInvalidRateParameters(
            IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            final int minRate = MIN_CAPTURE_RATE;
            final int maxRate = visualizer.getMaxCaptureRate();
            IHQVisualizer.OnDataCaptureListener listener = new EmptyOnDataCaptureListenerObj();

            assertEquals(IHQVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener, maxRate + 1,
                            true, true));

            assertEquals(IHQVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener, minRate - 1,
                            true, true));

            assertEquals(IHQVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener,
                            Integer.MIN_VALUE, true, true));

            assertEquals(IHQVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener,
                            Integer.MAX_VALUE, true, true));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerWhenEnabled(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

        try {
            visualizer = createVisualizer(player);

            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));
            assertTrue(visualizer.getEnabled());

            final int maxRate = visualizer.getMaxCaptureRate();
            IHQVisualizer.OnDataCaptureListener listener = new EmptyOnDataCaptureListenerObj();

            // with valid params
            assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                    visualizer.setDataCaptureListener(listener, maxRate, true,
                            true));

            assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                    visualizer.setDataCaptureListener(null, maxRate, false,
                            false));

            // with invalid params
            assertEquals(IHQVisualizer.ERROR_BAD_VALUE,
                    visualizer.setDataCaptureListener(listener, maxRate + 1,
                            true, true));
        } finally {
            releaseQuietly(visualizer);
        }
    }

    private void checkSetDataCaptureListenerMeasureWaveFormCaptureRate(
            IBasicMediaPlayer player) throws InterruptedException,
            TimeoutException {
        IHQVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            DataCaptureRateMeasure measure = new DataCaptureRateMeasure();
            int maxRate = visualizer.getMaxCaptureRate();

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
        IHQVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            DataCaptureRateMeasure measure = new DataCaptureRateMeasure();
            int maxRate = visualizer.getMaxCaptureRate();

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
        IHQVisualizer visualizer = null;

        try {
            // set looping (this test may take long times...)
            if (player.isPlaying()) {
                player.setLooping(true);
            }

            visualizer = createVisualizer(player);

            int range[] = visualizer.getCaptureSizeRange();
            int rate = visualizer.getMaxCaptureRate();

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

    private void checkAfterRelease(IBasicMediaPlayer player) {
        IHQVisualizer visualizer = null;

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
                visualizer.getCaptureSizeRange();
                fail();
            } catch (IllegalStateException e) {
                // expected
            }

            try {
                visualizer.getMaxCaptureRate();
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
        IHQVisualizer visualizer1 = null, visualizer2 = null;
        try {
            final CapturedDataSet captuerd1 = new CapturedDataSet();
            final CapturedDataSet captuerd2 = new CapturedDataSet();
            final CountDownLatch latch1 = new CountDownLatch(2);
            final CountDownLatch latch2 = new CountDownLatch(2);

            IHQVisualizer.OnDataCaptureListener listener1 = new IHQVisualizer.OnDataCaptureListener() {
                @Override
                public void onWaveFormDataCapture(IHQVisualizer visualizer,
                        float[] waveform, int numChannels, int samplingRate) {
                    if (captuerd1.waveform == null) {
                        captuerd1.waveform = waveform.clone();
                        latch1.countDown();
                    }
                }

                @Override
                public void onFftDataCapture(IHQVisualizer visualizer,
                        float[] fft, int numChannels, int samplingRate) {
                    if (captuerd1.fft == null) {
                        captuerd1.fft = fft.clone();
                        latch1.countDown();
                    }
                }
            };

            IHQVisualizer.OnDataCaptureListener listener2 = new IHQVisualizer.OnDataCaptureListener() {
                @Override
                public void onWaveFormDataCapture(IHQVisualizer visualizer,
                        float[] waveform, int numChannels, int samplingRate) {
                    if (captuerd2.waveform == null) {
                        captuerd2.waveform = waveform.clone();
                        latch2.countDown();
                    }
                }

                @Override
                public void onFftDataCapture(IHQVisualizer visualizer,
                        float[] fft, int numChannels, int samplingRate) {
                    if (captuerd2.fft == null) {
                        captuerd2.fft = fft.clone();
                        latch2.countDown();
                    }
                }
            };

            // create visualizer 1
            visualizer1 = createVisualizer(player);

            assertEquals(
                    IHQVisualizer.SUCCESS,
                    visualizer1.setDataCaptureListener(listener1,
                            visualizer1.getMaxCaptureRate(), true, true));

            // create visualizer 2
            visualizer2 = createVisualizer(player);

            assertEquals(
                    IHQVisualizer.SUCCESS,
                    visualizer2.setDataCaptureListener(listener2,
                            visualizer2.getMaxCaptureRate(), true, true));

            int[] sizeRange = visualizer2.getCaptureSizeRange();
            int size2 = sizeRange[0] * 2;

            assertNotEquals(DEFAULT_CAPTURE_SIZE, size2);

            assertFalse(visualizer1.getEnabled());
            assertFalse(visualizer2.getEnabled());

            // tweak capture size (vis1: min., vis2: max.)
            assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setCaptureSize(size2));
            assertEquals(IHQVisualizer.SUCCESS, visualizer2.setCaptureSize(size2));

            // XXX StandardVisualizer fails on this assertion,
            // visualizer1.getCaptureSize() returns DEFAULT_CAPTURE_SIZE
            assertEquals(size2, visualizer1.getCaptureSize());
            assertEquals(size2, visualizer2.getCaptureSize());

            // start capturing

            assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setEnabled(true));
            assertEquals(IHQVisualizer.SUCCESS, visualizer2.setEnabled(true));

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
        IHQVisualizer visualizer = null;
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
            visualizer.getCaptureSizeRange();
            visualizer.getMaxCaptureRate();

            visualizer.release();
        } finally {
            releaseQuietly(visualizer);
            visualizer = null;
        }
    }

    private void checkAfterControlLost(IBasicMediaPlayer player) {
        final boolean isStandardVisualizer = unwrap(player) instanceof StandardMediaPlayer;
        IHQVisualizer visualizer1 = null, visualizer2 = null;

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
                assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                        visualizer1.setEnabled(false));
            }

            assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setEnabled(true));

            assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setCaptureSize(origSize));

            // XXX StandardVisualizer returns SUCCESS
            assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
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
            final int[] origRange = visualizer1.getCaptureSizeRange();
            final int origMaxCapRate = visualizer1.getMaxCaptureRate();
            final int origWindowType = visualizer1.getWindowFunction();

            // create instance 2
            visualizer2 = createVisualizer(player);

            // no state changing methods should not raise any errors
            assertEquals(true, visualizer1.getEnabled());
            assertEquals(origSamplingRate, visualizer1.getSamplingRate());
            assertEquals(origRange[0],
                    visualizer1.getCaptureSizeRange()[0]);
            assertEquals(origRange[1],
                    visualizer1.getCaptureSizeRange()[1]);
            assertEquals(origMaxCapRate, visualizer1.getMaxCaptureRate());
            assertEquals(origSize, visualizer1.getCaptureSize());
            assertEquals(origWindowType, visualizer1.getWindowFunction());

            // XXX StandardVisualizer returns SUCCESS when enabled == true
            if (isStandardVisualizer) {
                Log.w("checkAfterControlLost", "ASSERTION SKIPPED for "
                        + player.getClass().getSimpleName());
            } else {
                assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                        visualizer1.setEnabled(true));
            }

            // state changing methods should return ERROR_INVALID_OPERATION
            assertEquals(IHQVisualizer.ERROR_INVALID_OPERATION,
                    visualizer1.setEnabled(false));

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
        public int windowType;

        public VisualizerStatus(IHQVisualizer visualizer) {
            enabled = visualizer.getEnabled();
            captureSize = visualizer.getCaptureSize();
            windowType = visualizer.getWindowFunction();
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
            if (windowType != other.windowType)
                return false;
            return true;
        }

        @Override
        public String toString() {
            return "VisualizerStatus [enabled=" + enabled + ", captureSize="
                    + captureSize + ", windowType=" + windowType + "]";
        }
    }

    private static void assertVisualizerState(VisualizerStatus expected,
            IHQVisualizer visualizer) {
        VisualizerStatus actual = new VisualizerStatus(visualizer);
        assertEquals(expected, actual);
    }

    public void testPlayerStateTransition() throws Exception {
        IBasicMediaPlayer player = null;
        IHQVisualizer visualizer = null;

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
                    IHQVisualizer.SUCCESS,
                    visualizer.setWindowFunction(IHQVisualizer.WINDOW_HAMMING));
            assertEquals(IHQVisualizer.SUCCESS, visualizer.setEnabled(true));

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

    private IHQVisualizer createVisualizer(IBasicMediaPlayer player) {
        return TestHQVisualizerWrapper.create(this, getFactory());
    }

    private static boolean isPowOfTwo(int x) {
        return (x != 0) && ((x & (x - 1)) == 0);
    }

    private static void checkSetCaptureSizeSUCCESS(IHQVisualizer visualizer,
            int size) {
        assertTrue(isPowOfTwo(size));
        assertEquals(IHQVisualizer.SUCCESS, visualizer.setCaptureSize(size));
        assertEquals(size, visualizer.getCaptureSize());
    }

    private static void checkSetCaptureSizeERROR_BAD_VALUE(
            IHQVisualizer visualizer, int size) {
        int origSize = visualizer.getCaptureSize();
        assertEquals(IHQVisualizer.ERROR_BAD_VALUE,
                visualizer.setCaptureSize(size));
        assertEquals(origSize, visualizer.getCaptureSize());
    }

    private static void checkSetCaptureSizeIllegalStateException(
            IHQVisualizer visualizer, int size) {
        int origSize = visualizer.getCaptureSize();

        try {
            int result = visualizer.setCaptureSize(size);
            fail("result = " + result);
        } catch (IllegalStateException e) {
            // expected
        }

        assertEquals(origSize, visualizer.getCaptureSize());
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
        public float[] waveform;
        public int waveformNumChannels = -1;
        public int waveformSamplingRate = -1;
        public float[] fft;
        public int fftNumChannels = -1;
        public int fftSamplingRate = -1;
    }

    private static CapturedDataSet getCaptureDatas(IHQVisualizer visualizer,
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
                new IHQVisualizer.OnDataCaptureListener() {
                    @Override
                    public void onWaveFormDataCapture(IHQVisualizer visualizer,
                            float[] waveform, int numChannels, int samplingRate) {
                        if (captured.waveform == null) {
                            captured.waveform = waveform.clone();
                            captured.waveformNumChannels = numChannels;
                            captured.waveformSamplingRate = samplingRate;
                            latch.countDown();
                        }
                    }

                    @Override
                    public void onFftDataCapture(IHQVisualizer visualizer,
                            float[] fft, int numChannels, int samplingRate) {
                        if (captured.fft == null) {
                            captured.fft = fft.clone();
                            captured.fftNumChannels = numChannels;
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

    private static final class DataCaptureRateMeasure implements
            IHQVisualizer.OnDataCaptureListener {

        public DataCaptureRateMeasure() {
        }

        @Override
        public void onWaveFormDataCapture(IHQVisualizer visualizer,
                float[] waveform, int numChannels, int samplingRate) {
            if (!mWaveform.sample()) {
                mLatch.countDown();
                return;
            }
        }

        @Override
        public void onFftDataCapture(IHQVisualizer visualizer, float[] fft,
                int numChannels, int samplingRate) {
            if (!mFft.sample()) {
                mLatch.countDown();
                return;
            }
        }

        public void measure(IHQVisualizer visualizer, int timeoutMs)
                throws InterruptedException, TimeoutException {
            mMeasureStartTime = System.nanoTime();

            mWaveform.reset();
            mFft.reset();

            mLatch = new CountDownLatch(1);

            if (visualizer.setEnabled(true) != IHQVisualizer.SUCCESS)
                throw new IllegalStateException(
                        "setEnabled(true) returns an error");

            if (!mLatch.await(timeoutMs, TimeUnit.MILLISECONDS))
                throw new TimeoutException();

            if (visualizer.setEnabled(false) != IHQVisualizer.SUCCESS)
                throw new IllegalStateException(
                        "setEnabled(false) returns an error");

            if (mWaveform.analyze())
                Log.d("CaptureRateMeasure - WF",
                        mWaveform.summaryText());

            if (mFft.analyze())
                Log.d("CaptureRateMeasure-FFT", mFft.summaryText());
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

    private static void checkWaveFormCaptureRate(IHQVisualizer visualizer,
            DataCaptureRateMeasure measure, int rate)
            throws InterruptedException, TimeoutException {
        {
            int timeout = (1000000 / rate)
                    * DataCaptureRateMeasure.NUM_MEASURE_POINTS * 2;

            assertEquals(IHQVisualizer.SUCCESS,
                    visualizer.setDataCaptureListener(measure, rate, true,
                            false));

            measure.measure(visualizer, timeout);

            int measuredRate = measure.getWaveFormCaptureRate();
            int tolerance = calcRateTolerance(rate);

            double error = (1.0 * (measuredRate - rate) / rate);

            Log.d("chkWaveFormCaptureRate",
                    "expected: " + rate + ", measured: " + measuredRate
                            + "  (error: "
                            + String.format("%.2f", (error * 100)) + " %)");

            // XXX This assertion may be fail when using StandardMediaPlayer
            assertRange(rate - tolerance, rate + tolerance, measuredRate);
        }
    }

    private static void checkFftCaptureRate(IHQVisualizer visualizer,
            DataCaptureRateMeasure measure, int rate)
            throws InterruptedException, TimeoutException {
        {
            int timeout = (1000000 / rate)
                    * DataCaptureRateMeasure.NUM_MEASURE_POINTS * 2;

            assertEquals(IHQVisualizer.SUCCESS,
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
