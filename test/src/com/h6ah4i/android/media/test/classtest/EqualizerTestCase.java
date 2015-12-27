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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.audiofx.IAudioEffect;
import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.audiofx.IEqualizer.Settings;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.media.test.utils.SeekCompleteListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class EqualizerTestCase
        extends BasicMediaPlayerTestCaseBase {

    private static final class TestParams extends BasicTestParams {
        private final PlayerState mPlayerState;
        private final boolean mEnabled;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                PlayerState playerState,
                boolean enabled) {
            super(factoryClass);
            mPlayerState = playerState;
            mEnabled = enabled;
        }

        public PlayerState getPlayerState() {
            return mPlayerState;
        }

        public boolean getEqualizerEnabled() {
            return mEnabled;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mPlayerState + ", " + mEnabled;
        }
    }

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        TestSuite suite = new TestSuite();

        String[] testsWithoutPreconditionEqualizerEnabled = new String[] {
                "testDefaultParameters",
                "testSetAndGetEnabled",
                "testAfterRelease",
                "testPlayerReleasedBeforeEffect",
                "testHasControl",
                "testMultiInstanceBehavior",
                "testGetPresetName",
        };
        String[] testsJustUseBasicTestParams = new String[] {
                "testPlayerStateTransition",
        };

        // use TestParam.getEualizerEnabled()
        {
            List<String> excludes = new ArrayList<String>();
            excludes.addAll(Arrays.asList(testsWithoutPreconditionEqualizerEnabled));
            excludes.addAll(Arrays.asList(testsJustUseBasicTestParams));
            ParameterizedTestSuiteBuilder.Filter filter =
                    ParameterizedTestSuiteBuilder.notMatches(excludes);

            List<TestParams> params = new ArrayList<TestParams>();

            List<PlayerState> playerStates = new ArrayList<PlayerState>();
            playerStates.addAll(Arrays.asList(PlayerState.values()));
            playerStates.remove(PlayerState.End);

            for (PlayerState playerState : playerStates) {
                params.add(new TestParams(factoryClazz, playerState, false));
                params.add(new TestParams(factoryClazz, playerState, true));
            }

            suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                    EqualizerTestCase.class, params, filter, false));
        }

        // don't use TestParam.getEualizerEnabled()
        {
            ParameterizedTestSuiteBuilder.Filter filter =
                    ParameterizedTestSuiteBuilder.matches(
                            testsWithoutPreconditionEqualizerEnabled);
            List<TestParams> params = new ArrayList<TestParams>();

            List<PlayerState> playerStates = new ArrayList<PlayerState>();
            playerStates.addAll(Arrays.asList(PlayerState.values()));
            playerStates.remove(PlayerState.End);

            for (PlayerState playerState : playerStates) {
                params.add(new TestParams(factoryClazz, playerState, false));
            }

            suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                    EqualizerTestCase.class, params, filter, false));
        }

        // not parameterized tests
        for (String testName : testsJustUseBasicTestParams) {
            suite.addTest(makeSingleBasicTest(
                    EqualizerTestCase.class, testName, factoryClazz));
        }

        return suite;
    }

    public EqualizerTestCase(ParameterizedTestArgs args) {
        super(args);
    }

    //
    // Exposed test cases
    //
    public void testDefaultParameters() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkDefaultParameters(player);
                    }
                });
    }

    public void testSetAndGetEnabled() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkSetAndGetEnabled(player, params);
                    }
                });
    }

    public void testBandLevelParamWithValidRange() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkBandLevelParamWithValidRange(player, params);
                    }
                });
    }

    public void testBandLevelParamWithInvalidBandLevelRange() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkBandLevelParamWithInvalidBandLevelRange(player, params);
                    }
                });
    }

    public void testBandLevelParamWithInvalidBandRange() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkBandLevelParamWithInvalidBandRange(player, params);
                    }
                });
    }

    public void testPropertiesCompatWithValidCurPresetParams() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesCompatWithValidCurPresetParams(player, params);
                    }
                });
    }

    public void testPropertiesCompatWithUndefinedCurPresetParams() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesCompatWithUndefinedCurPresetParams(player, params);
                    }
                });
    }

    public void testPropertiesCompatWithInvalidBandLevels() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesCompatWithInvalidBandLevels(player, params);
                    }
                });
    }

    public void testPropertiesCompatWithInvalidBandLevelsLength() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesCompatWithInvalidBandLevelsLength(player, params);
                    }
                });
    }

    public void testPropertiesCompatWithNullBandLevels() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesCompatWithNullBandLevels(player, params);
                    }
                });
    }

    public void testPropertiesCompatWithInvalidNumBands() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesCompatWithInvalidNumBands(player, params);
                    }
                });
    }

    public void testPropertiesCompatWithInvalidCurPreset() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesCompatWithInvalidCurPreset(player, params);
                    }
                });
    }

    public void testPropertiesCompatWithNullSettings() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesCompatWithNullSettings(player);
                    }
                });
    }

    public void testCurPresetResetBySetBandLevel() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkCurPresetResetBySetBandLevel(player, params);
                    }
                });
    }

    public void testAfterRelease() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkAfterRelease(player, params);
                    }
                });
    }

    public void testHasControl() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkHasControl(player, params);
                    }
                });
    }

    public void testAfterControlLost() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkAfterControlLost(player);
                    }
                });
    }

    public void testMultiInstanceBehavior() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkMultiInstanceBehavior(player);
                    }
                });
    }

    public void testPlayerReleasedBeforeEffect() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPlayerReleasedBeforeEffect(player, params);
                    }
                });
    }

    public void testGetPresetName() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkGetPresetName(player, params);
                    }
                });
    }

    public void testGetBand() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkGetBand(player, params);
                    }
                });
    }

    public void testGetBandFreqRange() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkGetBandFreqRange(player, params);
                    }
                });
    }

    public void testGetCenterFreq() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkGetCenterFreq(player, params);
                    }
                });
    }

    public void testGetBandLevelRange() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkGetBandLevelRange(player, params);
                    }
                });
    }

    public void testPlayerStateTransition() throws Exception {
        IBasicMediaPlayer player = null;
        IEqualizer effect = null;

        try {
            // check effect settings are preserved along player state transition
            Object waitObj = new Object();
            CompletionListenerObject comp = new CompletionListenerObject(waitObj);
            SeekCompleteListenerObject seekComp = new SeekCompleteListenerObject(waitObj);

            player = createWrappedPlayerInstance();
            effect = createEqualizer(player);

            player.setOnCompletionListener(comp);
            player.setOnSeekCompleteListener(seekComp);

            // configure
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(true));
            effect.usePreset((short) (effect.getNumberOfPresets() - 1));

            final IEqualizer.Settings expectedSettings = effect.getProperties();

            // player: idle

            // player: initialized
            setDataSourceForCommonTests(player, null);

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getProperties());

            // player: prepared
            player.prepare();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getProperties());

            // player: started
            player.start();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getProperties());

            // player: paused
            player.pause();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getProperties());

            // player: playback completed
            player.seekTo(player.getDuration());
            if (!seekComp.await(DEFAULT_EVENT_WAIT_DURATION)) {
                fail();
            }
            player.start();
            if (!comp.await(SHORT_EVENT_WAIT_DURATION)) {
                // XXX This assertion fails on Android 5.0 with
                // StandardMediaPlayer
                fail();
            }

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getProperties());

            // player: stop
            player.stop();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getProperties());

            // player: idle
            player.reset();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getProperties());

            // player: end
            player.release();
            player = null;

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getProperties());
        } finally {
            releaseQuietly(player);
            releaseQuietly(effect);
        }
    }

    private IEqualizer createEqualizer(IBasicMediaPlayer player) {
        return getFactory().createEqualizer(unwrap(player));
    }

    //
    // Implementations
    //
    private void checkSetAndGetEnabled(IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            assertEquals(false, effect.getEnabled());

            effect.setEnabled(false);
            assertEquals(false, effect.getEnabled());

            effect.setEnabled(true);
            assertEquals(true, effect.getEnabled());
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkDefaultParameters(IBasicMediaPlayer player) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            // check
            checkIsDefaultState(effect);

            // modify parameters
            effect.setEnabled(true);
            effect.usePreset((short) (effect.getNumberOfPresets() - 1));

            // release
            effect.release();
            effect = null;

            // re-confirm with new instance
            effect = createEqualizer(player);

            checkIsDefaultState(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkBandLevelParamWithValidRange(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckVaildBandLevel(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkBandLevelParamWithInvalidBandLevelRange(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckBandLevelWithInvalidBandLevel(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkBandLevelParamWithInvalidBandRange(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckBandLevelWithInvalidBand(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithValidCurPresetParams(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckValidCurPresetProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithUndefinedCurPresetParams(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckUndefinedCurPresetProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithInvalidBandLevels(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckInvalidBandLevelsProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithInvalidBandLevelsLength(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckInvalidBandLevelsLengthProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithNullBandLevels(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckNullBandLevelsProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithInvalidNumBands(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckInvalidNumBandsProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithInvalidCurPreset(
            IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());
            setAndCheckInvalidCurPresetProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithNullSettings(IBasicMediaPlayer player) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);
            try {
                effect.setProperties(null);
                fail();
            } catch (IllegalArgumentException e) {
                // expected
            }

        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkCurPresetResetBySetBandLevel(IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;

        try {
            effect = createEqualizer(player);

            effect.setEnabled(params.getEqualizerEnabled());

            IEqualizer.Settings settings = new Settings();
            settings.numBands = effect.getNumberOfBands();
            settings.curPreset = 1;
            settings.bandLevels = new short[settings.numBands];

            effect.setProperties(settings);

            assertEquals((short) 1, effect.getCurrentPreset());

            effect.setBandLevel((short) 0, (short) 123);
            assertEquals(IEqualizer.PRESET_UNDEFINED, effect.getCurrentPreset());
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkAfterRelease(IBasicMediaPlayer player, TestParams params) {
        try {
            createReleasedEqualizer(player).getId();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getEnabled();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedEqualizer(player).hasControl();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedEqualizer(player).setEnabled(true);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedEqualizer(player).setEnabled(false);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getBand(0);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getBandFreqRange((short) 0);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getBandLevelRange();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getNumberOfBands();
            fail();
        } catch (Exception e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getNumberOfPresets();
            fail();
        } catch (Exception e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getCenterFreq((short) 0);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getCurrentPreset();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).usePreset((short) 0);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getBandLevel((short) 0);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).setBandLevel((short) 0, (short) 0);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEqualizer(player).getProperties();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            IEqualizer equalizer = createEqualizer(player);
            IEqualizer.Settings settings = createSettings(equalizer, (short) 0, (short) 0);

            equalizer.release();

            createReleasedEqualizer(player).setProperties(settings);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
    }

    private void checkHasControl(IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect1 = null, effect2 = null, effect3 = null;

        try {
            // create instance 1
            // NOTE: [1]: has control, [2] not created, [3] not created
            effect1 = createEqualizer(player);

            assertTrue(effect1.hasControl());

            // create instance 2
            // NOTE: [1]: lost control, [2] has control, [3] not created
            effect2 = createEqualizer(player);

            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            assertEquals(
                    IAudioEffect.ERROR_INVALID_OPERATION,
                    effect1.setEnabled(false));

            assertEquals(
                    IAudioEffect.ERROR_INVALID_OPERATION,
                    effect1.setEnabled(true));

            assertEquals(
                    IAudioEffect.SUCCESS,
                    effect2.setEnabled(true));

            assertEquals(
                    IAudioEffect.SUCCESS,
                    effect2.setEnabled(false));

            // create instance 3
            // NOTE: [1]: lost control, [2] lost control, [3] has control
            effect3 = createEqualizer(player);

            assertFalse(effect1.hasControl());
            assertFalse(effect2.hasControl());
            assertTrue(effect3.hasControl());

            assertEquals(
                    IAudioEffect.ERROR_INVALID_OPERATION,
                    effect1.setEnabled(true));

            assertEquals(
                    IAudioEffect.ERROR_INVALID_OPERATION,
                    effect2.setEnabled(true));

            assertEquals(
                    IAudioEffect.SUCCESS,
                    effect3.setEnabled(true));

            assertEquals(
                    IAudioEffect.SUCCESS,
                    effect3.setEnabled(false));

            // release the instance 3
            // NOTE: [1]: lost control, [2] has control, [3] released
            effect3.release();
            effect3 = null;

            assertFalse(effect1.hasControl());
            // XXX This assertion may be fail when using StandardMediaPlayer
            assertTrue(effect2.hasControl());

            assertEquals(
                    IAudioEffect.ERROR_INVALID_OPERATION,
                    effect1.setEnabled(true));

            assertEquals(
                    IAudioEffect.SUCCESS,
                    effect2.setEnabled(true));

            assertEquals(
                    IAudioEffect.SUCCESS,
                    effect2.setEnabled(false));

            // release the instance 2
            // NOTE: [1]: has control, [2] released, [3] released
            effect2.release();
            effect2 = null;

            // XXX This assertion may be fail when using StandardMediaPlayer
            assertTrue(effect1.hasControl());

            assertEquals(
                    IAudioEffect.SUCCESS,
                    effect1.setEnabled(true));

            assertEquals(
                    IAudioEffect.SUCCESS,
                    effect1.setEnabled(false));
        } finally {
            releaseQuietly(effect1);
            effect1 = null;

            releaseQuietly(effect2);
            effect2 = null;

            releaseQuietly(effect3);
            effect3 = null;
        }
    }

    private void checkAfterControlLost(IBasicMediaPlayer player) {
        IEqualizer effect1 = null, effect2 = null;

        try {
            effect1 = createEqualizer(player);
            effect2 = createEqualizer(player);

            final boolean initialEnabledState = effect2.getEnabled();
            final IEqualizer.Settings initialSettings = effect2.getProperties();

            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            // no state changing methods should not raise any errors
            assertEquals(effect2.getEnabled(), effect1.getEnabled());
            assertEquals(effect2.getId(), effect1.getId());
            assertEquals(effect2.getCurrentPreset(), effect1.getCurrentPreset());
            assertEquals(effect2.getCenterFreq((short) 0), effect1.getCenterFreq((short) 0));
            assertEquals(effect2.getBand(1000), effect1.getBand(1000));
            assertEquals(effect2.getBandLevel((short) 0), effect1.getBandLevel((short) 0));
            assertEquals(effect2.getNumberOfBands(), effect1.getNumberOfBands());
            assertEquals(effect2.getNumberOfPresets(), effect1.getNumberOfPresets());
            assertEquals(effect2.getPresetName((short) 0), effect1.getPresetName((short) 0));
            assertEquals(effect2.getBandFreqRange((short) 0)[0],
                    effect1.getBandFreqRange((short) 0)[0]);
            assertEquals(effect2.getBandLevelRange()[0], effect1.getBandLevelRange()[0]);
            assertEquals(effect2.getProperties(), effect1.getProperties());

            // setEnabled() should return IAudioEffect.ERROR_INVALID_OPERATION
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(false));
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(true));

            // state changing methods should raise UnsupportedOperationException
            try {
                effect1.setBandLevel((short) 0, (short) 100);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            try {
                effect1.usePreset((short) 0);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            try {
                IEqualizer.Settings settings = new Settings();

                settings.curPreset = 0;
                settings.numBands = initialSettings.numBands;
                settings.bandLevels = new short[settings.numBands];

                effect1.setProperties(settings);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }

            // confirm object state
            assertEquals(initialEnabledState, effect1.getEnabled());
            assertEquals(initialSettings, effect1.getProperties());
        } finally {
            releaseQuietly(effect1);
            effect1 = null;

            releaseQuietly(effect2);
            effect2 = null;
        }
    }

    private void checkMultiInstanceBehavior(IBasicMediaPlayer player) {
        IEqualizer effect1 = null, effect2 = null;

        try {
            effect1 = createEqualizer(player);
            effect2 = createEqualizer(player);

            // check pre. conditions
            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            assertFalse(effect1.getEnabled());
            assertFalse(effect2.getEnabled());

            assertEquals((short) 0, effect1.getCurrentPreset());
            assertEquals((short) 0, effect2.getCurrentPreset());

            effect2.usePreset((short) 0);

            IEqualizer.Settings expectedSettings = effect2.getProperties();

            // check effect 1 lost controls
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(false));
            try {
                effect1.setBandLevel((short) 0, (short) 123);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertNotEquals(expectedSettings.bandLevels[0], (short) 123);
            assertEquals(expectedSettings.bandLevels[0], effect1.getBandLevel((short) 0));

            try {
                IEqualizer.Settings settings = new Settings();

                settings.curPreset = 1;
                settings.numBands = expectedSettings.numBands;
                settings.bandLevels = new short[settings.numBands];

                effect1.setProperties(settings);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertEquals(expectedSettings, effect1.getProperties());

            try {
                effect1.usePreset((short) 2);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertEquals(expectedSettings, effect1.getProperties());

            // change states
            assertEquals(IAudioEffect.SUCCESS, effect2.setEnabled(true));

            final short expectedPreset = (short) (effect2.getNumberOfPresets() - 1);
            effect2.usePreset(expectedPreset);

            // check post conditions
            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            // XXX This assertion may be fail when using StandardMediaPlayer
            assertTrue(effect1.getEnabled());
            assertTrue(effect2.getEnabled());

            // XXX Galaxy S4, HTC phones and CyanogenMod fails on this assertion
            assertEquals(expectedPreset, effect1.getCurrentPreset());
            assertEquals(expectedPreset, effect2.getCurrentPreset());

            // release effect 2
            effect2.release();
            effect2 = null;

            // check effect 1 gains control
            // XXX This assertion may be fail when using StandardMediaPlayer
            assertTrue(effect1.hasControl());
            assertEquals(IAudioEffect.SUCCESS, effect1.setEnabled(false));
        } finally {
            releaseQuietly(effect1);
            releaseQuietly(effect2);
        }
    }

    private void checkPlayerReleasedBeforeEffect(IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;
        try {
            effect = createEqualizer(player);

            // pre. check
            assertTrue(effect.hasControl());
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(true));

            // release player
            player.release();
            player = null;

            // post check
            assertTrue(effect.hasControl());
            assertEquals(true, effect.getEnabled());
            // XXX Galaxy S2 returns -7 when its in Idle, Preparing,
            // Prepared, ErrorBeforePrepared or ErrorAfterPrepared state.
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(false));

            // release effect
            effect.release();
            effect = null;
        } finally {
            releaseQuietly(effect);
            effect = null;
        }
    }

    private void checkGetPresetName(IBasicMediaPlayer player, TestParams params) {
        final String[] presetNamesNormal = {
                "Normal",
                "Classical",
                "Dance",
                "Flat",
                "Folk",
                "Heavy Metal",
                "Hip Hop",
                "Jazz",
                "Pop",
                "Rock"
        };
        final String[] presetNamesCyanogenMod = {
                "Acoustic",
                "Bass Booster",
                "Bass Reducer",
                "Classical",
                "Deep",
                "Flat",
                "R&B",
                "Rock",
                "Small Speakers",
                "Treble Booster",
                "Treble Reducer",
                "Vocal Booster",
        };
        // XXX HTC Evo 3D

        IEqualizer effect = null;
        try {
            effect = createEqualizer(player);
            final String[] presetNames = (effect.getNumberOfBands() == 5)
                    ? presetNamesNormal
                    : presetNamesCyanogenMod;

            assertEquals(presetNames.length, effect.getNumberOfPresets());

            for (short i = 0; i < presetNames.length; i++) {
                assertEquals(presetNames[i], effect.getPresetName(i));
            }
            assertEquals("", effect.getPresetName(IEqualizer.PRESET_UNDEFINED));
            assertEquals("", effect.getPresetName((short) (presetNames.length + 1)));
        } finally {
            releaseQuietly(effect);
            effect = null;
        }
    }

    private void checkGetBand(IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;
        try {
            effect = createEqualizer(player);
            short numBands = effect.getNumberOfBands();

            for (short band = 0; band < numBands; band++) {
                int[] range = effect.getBandFreqRange(band);

                // range[0]
                assertEquals(band, effect.getBand(range[0]));

                // range[1]
                assertEquals(band, effect.getBand(range[1]));

                // range[0] - 1
                if (band == 0) {
                    assertEquals((short) -1, effect.getBand(range[0] - 1));
                } else {
                    assertEquals((short) (band - 1), effect.getBand(range[0] - 1));
                }

                // range[1] + 1
                if (band == (numBands - 1)) {
                    assertEquals((short) -1, effect.getBand(range[1] + 1));
                } else {
                    assertEquals((short) (band + 1), effect.getBand(range[1] + 1));
                }
            }

        } finally {
            releaseQuietly(effect);
            effect = null;
        }
    }

    private void checkGetBandFreqRange(IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;
        try {
            effect = createEqualizer(player);

            short numBands = effect.getNumberOfBands();
            for (short band = 0; band < numBands; band++) {
                int[] range = effect.getBandFreqRange(band);

                // min. and max. relationship
                assertTrue(range[1] > range[0]);

                // no gap & no over wrap
                if (band > 0) {
                    int[] prevRange = effect.getBandFreqRange((short) (band - 1));
                    assertEquals((prevRange[1] + 1), range[0]);
                }
            }
        } finally {
            releaseQuietly(effect);
            effect = null;
        }
    }

    private void checkGetCenterFreq(IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;
        try {
            effect = createEqualizer(player);

            short numBands = effect.getNumberOfBands();
            for (short band = 0; band < numBands; band++) {
                int[] range = effect.getBandFreqRange(band);
                int center = effect.getCenterFreq(band);

                assertTrue(
                        "range = " + Arrays.toString(range) + ", center = " + center,
                        (range[0] < center && center < range[1]));
            }
        } finally {
            releaseQuietly(effect);
            effect = null;
        }
    }

    private void checkGetBandLevelRange(IBasicMediaPlayer player, TestParams params) {
        IEqualizer effect = null;
        try {
            effect = createEqualizer(player);

            short[] range = effect.getBandLevelRange();

            assertTrue(range[0] < 0);
            assertTrue(range[1] > 0);
        } finally {
            releaseQuietly(effect);
            effect = null;
        }
    }

    private static final class ShortValues {
        final short setvalue;
        final short expected;

        public ShortValues(short value1, short value2) {
            this.setvalue = value1;
            this.expected = value2;
        }
    }

    private void setAndCheckVaildBandLevel(IEqualizer equalizer) {
        short range[] = equalizer.getBandLevelRange();
        ShortValues[] TEST_VALUES = new ShortValues[] {
                new ShortValues(range[0], range[0]),
                new ShortValues((short) ((range[0] + range[1]) / 2),
                        (short) ((range[0] + range[1]) / 2)),
                new ShortValues(range[1], range[1]),
        };
        short numBands = equalizer.getNumberOfBands();

        for (ShortValues v : TEST_VALUES) {
            for (short band = 0; band < numBands; band++) {
                IEqualizer.Settings prevSettings = equalizer.getProperties();

                equalizer.setBandLevel(band, v.setvalue);
                assertBandLevelWithOthersArePreserved(v.expected, equalizer, band, prevSettings);
            }
        }
    }

    private void setAndCheckBandLevelWithInvalidBandLevel(IEqualizer equalizer) {
        short range[] = equalizer.getBandLevelRange();
        ShortValues[] TEST_VALUES = new ShortValues[] {
                new ShortValues((short) (range[0] - 1), range[0]),
                new ShortValues((short) (range[1] + 1), range[1]),
        };
        short numBands = equalizer.getNumberOfBands();

        for (ShortValues v : TEST_VALUES) {
            for (short band = 0; band < numBands; band++) {
                IEqualizer.Settings prevSettings = equalizer.getProperties();

                try {
                    equalizer.setBandLevel(band, v.setvalue);
                    fail();
                } catch (IllegalArgumentException e) {
                    // expected
                }

                assertEquals(prevSettings, equalizer.getProperties());
            }
        }
    }

    private void setAndCheckBandLevelWithInvalidBand(IEqualizer equalizer) {
        short numBands = equalizer.getNumberOfBands();
        short[] TEST_VALUES = {
                (short) -1, numBands
        };

        for (short band : TEST_VALUES) {
            IEqualizer.Settings prevSettings = equalizer.getProperties();

            try {
                equalizer.setBandLevel(band, (short) 123);
                fail();
            } catch (IllegalArgumentException e) {
                // expected
            }

            try {
                equalizer.getBandLevel(band);
                fail();
            } catch (IllegalArgumentException e) {
                // expected
            }

            assertEquals(prevSettings, equalizer.getProperties());
        }
    }

    private void setAndCheckValidCurPresetProperties(IEqualizer equalizer) {
        short numBands = equalizer.getNumberOfBands();
        short numPresets = equalizer.getNumberOfPresets();
        short range[] = equalizer.getBandLevelRange();

        ShortValues[] TEST_VALUES = new ShortValues[] {
                new ShortValues(range[0], range[0]),
                new ShortValues(
                        (short) ((range[0] + range[1]) / 2),
                        (short) ((range[0] + range[1]) / 2)),
                new ShortValues(range[1], range[1]),
        };

        for (ShortValues v : TEST_VALUES) {
            for (short preset = 0; preset < numPresets; preset++) {
                for (short band = 0; band < numBands; band++) {
                    equalizer.usePreset(preset);
                    IEqualizer.Settings expectedSettings = equalizer.getProperties();
                    String expectedName = equalizer.getPresetName(preset);
                    assertEquals(preset, expectedSettings.curPreset);

                    equalizer.usePreset((short) 0);

                    IEqualizer.Settings settings = new Settings();

                    settings.curPreset = preset;
                    settings.numBands = numBands;
                    settings.bandLevels = new short[settings.numBands];

                    settings.bandLevels[band] = v.expected;

                    equalizer.setProperties(settings);

                    // NOTE
                    // bandLevels values are not used
                    assertEquals(expectedSettings, equalizer.getProperties());
                    assertEquals(expectedName,
                            equalizer.getPresetName(equalizer.getCurrentPreset()));
                }
            }
        }
    }

    private void setAndCheckUndefinedCurPresetProperties(IEqualizer equalizer) {

        equalizer.usePreset((short) 0);

        String expectedName = "";
        IEqualizer.Settings settings = new Settings();

        settings.curPreset = IEqualizer.PRESET_UNDEFINED;
        settings.numBands = equalizer.getNumberOfBands();
        settings.bandLevels = new short[settings.numBands];

        settings.bandLevels[0] = (short) 100;
        settings.bandLevels[1] = (short) -200;
        settings.bandLevels[2] = (short) 300;
        settings.bandLevels[3] = (short) -400;
        settings.bandLevels[4] = (short) 500;

        equalizer.setProperties(settings);

        assertEquals(settings, equalizer.getProperties());
        assertEquals(expectedName, equalizer.getPresetName(equalizer.getCurrentPreset()));
    }

    private void setAndCheckInvalidBandLevelsProperties(IEqualizer equalizer) {
        short numBands = equalizer.getNumberOfBands();
        short numPresets = equalizer.getNumberOfPresets();
        short range[] = equalizer.getBandLevelRange();

        ShortValues[] TEST_VALUES = new ShortValues[] {
                new ShortValues((short) (range[0] - 1), range[0]),
                new ShortValues((short) (range[1] + 1), range[1]),
        };

        for (ShortValues v : TEST_VALUES) {
            for (short preset = 0; preset < numPresets; preset++) {
                for (short band = 0; band < numBands; band++) {
                    equalizer.usePreset(preset);
                    IEqualizer.Settings expectedSettings = equalizer.getProperties();
                    String expectedName = equalizer.getPresetName(preset);
                    assertEquals(preset, expectedSettings.curPreset);

                    equalizer.usePreset((short) 0);

                    IEqualizer.Settings settings = new Settings();

                    settings.curPreset = preset;
                    settings.numBands = numBands;
                    settings.bandLevels = new short[settings.numBands];

                    settings.bandLevels[band] = v.expected;

                    equalizer.setProperties(settings);

                    // NOTE
                    // bandLevels values are not used
                    assertEquals(expectedSettings, equalizer.getProperties());
                    assertEquals(expectedName,
                            equalizer.getPresetName(equalizer.getCurrentPreset()));
                }
            }
        }
    }

    private void setAndCheckNullBandLevelsProperties(IEqualizer equalizer) {
        short numBands = equalizer.getNumberOfBands();
        IEqualizer.Settings settings = new Settings();

        settings.curPreset = 0;
        settings.numBands = numBands;
        settings.bandLevels = null;

        try {
            equalizer.setProperties(settings);
            fail();
        } catch (IllegalArgumentException e) {
            // excepted
        }
    }

    private void setAndCheckInvalidNumBandsProperties(IEqualizer equalizer) {
        short numBands = equalizer.getNumberOfBands();
        short[] TEST_VALUES = new short[] {
                0, (short) (numBands - 1), (short) (numBands + 1)
        };

        for (short n : TEST_VALUES) {
            IEqualizer.Settings settings = new Settings();

            settings.curPreset = 0;
            settings.numBands = n; // invalid
            settings.bandLevels = new short[numBands];

            try {
                equalizer.setProperties(settings);
                fail();
            } catch (IllegalArgumentException e) {
                // excepted
            }
        }
    }

    private void setAndCheckInvalidCurPresetProperties(IEqualizer equalizer) {
        short numBands = equalizer.getNumberOfBands();
        short numPresets = equalizer.getNumberOfPresets();
        short[] TEST_VALUES = new short[] {
                (short) -2, (short) (numPresets + 1)
        };

        for (short preset : TEST_VALUES) {
            equalizer.usePreset((short) 0);

            IEqualizer.Settings settings = new Settings();

            settings.curPreset = preset; // invalid
            settings.numBands = numBands;
            settings.bandLevels = new short[numBands];

            try {
                equalizer.setProperties(settings);
                fail("actual = " + equalizer.getProperties());
            } catch (IllegalArgumentException e) {
                // excepted
            }
        }
    }

    private void setAndCheckInvalidBandLevelsLengthProperties(IEqualizer equalizer) {
        short numBands = equalizer.getNumberOfBands();
        short[] TEST_VALUES = new short[] {
                0, (short) (numBands - 1), (short) (numBands + 1)
        };

        // numBands is invalid
        for (short n : TEST_VALUES) {
            IEqualizer.Settings settings = new Settings();

            settings.curPreset = 0;
            settings.numBands = numBands;
            settings.bandLevels = new short[n]; // invalid

            try {
                equalizer.setProperties(settings);
                fail();
            } catch (IllegalArgumentException e) {
                // excepted
            }
        }

        // bandLevels.length is invalid
        for (short n : TEST_VALUES) {
            IEqualizer.Settings settings = new Settings();

            settings.curPreset = 0;
            settings.numBands = numBands;
            settings.bandLevels = new short[n];

            try {
                equalizer.setProperties(settings);
                fail();
            } catch (IllegalArgumentException e) {
                // excepted
            }
        }
    }

    //
    // Utilities
    //

    private static void assertEquals(IEqualizer.Settings expected, IEqualizer.Settings actual) {
        assertEquals(expected.toString(), actual.toString());
    }

    static void assertBandLevelEquals(short expected, IEqualizer equalizer, short band) {
        assertEquals(expected, equalizer.getBandLevel(band));
        assertEquals(expected, equalizer.getProperties().bandLevels[band]);
    }

    static void assertBandLevelAllPreserved(
            short expected, IEqualizer equalizer, IEqualizer.Settings prevSettings) {
        short numBands = equalizer.getNumberOfBands();

        for (short i = 0; i < numBands; i++) {
            assertBandLevelEquals(prevSettings.bandLevels[i], equalizer, i);
        }
    }

    static void assertBandLevelWithOthersArePreserved(
            short expected, IEqualizer equalizer, short band, IEqualizer.Settings prevSettings) {
        short numBands = equalizer.getNumberOfBands();

        for (short i = 0; i < numBands; i++) {
            if (i == band)
                assertBandLevelEquals(expected, equalizer, i);
            else
                assertBandLevelEquals(prevSettings.bandLevels[i], equalizer, i);
        }
    }

    static void assertBandLevelWithOthersAreZero(
            short expected, IEqualizer equalizer, short band) {
        short numBands = equalizer.getNumberOfBands();

        for (short i = 0; i < numBands; i++) {
            if (i == band)
                assertBandLevelEquals(expected, equalizer, i);
            else
                assertBandLevelEquals((short) 0, equalizer, i);
        }
    }

    static IEqualizer.Settings createSettings(IEqualizer equalizer, short band, short bandLevel) {
        IEqualizer.Settings settings = new Settings();

        settings.numBands = equalizer.getNumberOfBands();
        settings.bandLevels = new short[settings.numBands];
        settings.curPreset = 0;

        settings.bandLevels[band] = bandLevel;

        return settings;
    }

    private IEqualizer createReleasedEqualizer(IBasicMediaPlayer player) {
        IEqualizer equalizer = createEqualizer(player);
        equalizer.release();
        return equalizer;
    }

    private static interface BasicMediaPlayerTestRunnable {
        public void run(IBasicMediaPlayer player, Object args) throws Throwable;
    }

    private void checkWithNoPlayerErrors(
            TestParams params, BasicMediaPlayerTestRunnable checkProcess) throws Throwable {
        IBasicMediaPlayer player = null;

        try {
            player = createWrappedPlayerInstance();
            transitState(params.getPlayerState(), player, null);

            Object sharedSyncObj = new Object();
            ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);

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

    private void checkIsDefaultState(IEqualizer effect) {
        short numBands = effect.getNumberOfBands();

        assertEquals(false, effect.getEnabled());
        assertEquals(0, effect.getCurrentPreset());

        if (numBands == 5) {
            // "Default"
            assertEquals((short) 300, effect.getBandLevel((short) 0));
            assertEquals((short) 0, effect.getBandLevel((short) 1));
            assertEquals((short) 0, effect.getBandLevel((short) 2));
            assertEquals((short) 0, effect.getBandLevel((short) 3));
            assertEquals((short) 300, effect.getBandLevel((short) 4));
        } else if (numBands == 6) {
            // CyanogenMod "Acoustic"
            assertEquals((short) 450, effect.getBandLevel((short) 0));
            assertEquals((short) 450, effect.getBandLevel((short) 1));
            assertEquals((short) 350, effect.getBandLevel((short) 2));
            assertEquals((short) 175, effect.getBandLevel((short) 3));
            assertEquals((short) 350, effect.getBandLevel((short) 4));
            assertEquals((short) 250, effect.getBandLevel((short) 5));
        } else {
            fail();
        }
    }
}
