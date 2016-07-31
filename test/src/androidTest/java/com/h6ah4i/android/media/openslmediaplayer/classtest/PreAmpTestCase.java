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


package com.h6ah4i.android.media.openslmediaplayer.classtest;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.audiofx.IAudioEffect;
import com.h6ah4i.android.media.audiofx.IPreAmp;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.utils.CompletionListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.utils.ErrorListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.utils.SeekCompleteListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestSuiteBuilder;

import junit.framework.TestSuite;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class PreAmpTestCase
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

        public boolean getPreAmpEnabled() {
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

        String[] testsWithoutPreconditionPreAmpEnabled = new String[] {
                "testDefaultParameters",
                "testSetAndGetEnabled",
                "testAfterRelease",
                "testPlayerReleasedBeforeEffect",
                "testHasControl",
                "testMultiInstanceBehavior",
        };
        String[] testsJustUseBasicTestParams = new String[] {
                "testPlayerStateTransition",
        };

        // use TestParam.getEualizerEnabled()
        {
            List<String> excludes = new ArrayList<String>();
            excludes.addAll(Arrays.asList(testsWithoutPreconditionPreAmpEnabled));
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
                    PreAmpTestCase.class, params, filter, false));
        }

        // don't use TestParam.getEualizerEnabled()
        {
            ParameterizedTestSuiteBuilder.Filter filter =
                    ParameterizedTestSuiteBuilder.matches(
                            testsWithoutPreconditionPreAmpEnabled);
            List<TestParams> params = new ArrayList<TestParams>();

            List<PlayerState> playerStates = new ArrayList<PlayerState>();
            playerStates.addAll(Arrays.asList(PlayerState.values()));
            playerStates.remove(PlayerState.End);

            for (PlayerState playerState : playerStates) {
                params.add(new TestParams(factoryClazz, playerState, false));
            }

            suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                    PreAmpTestCase.class, params, filter, false));
        }

        // not parameterized tests
        for (String testName : testsJustUseBasicTestParams) {
            suite.addTest(makeSingleBasicTest(
                    PreAmpTestCase.class, testName, factoryClazz));
        }

        return suite;
    }

    public PreAmpTestCase(ParameterizedTestArgs args) {
        super(args);
    }

    private IPreAmp createPreAmp() {
        return getFactory().createPreAmp();
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

    public void testLevelParamWithValidRange() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkLevelParamWithValidRange(player, params);
                    }
                });
    }

    public void testLevelParamWithInvalidLevelRange() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkLevelParamWithInvalidLevelRange(player, params);
                    }
                });
    }

    public void testPropertiesWithValidLevels() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesWithValidLevels(player, params);
                    }
                });
    }

    public void testPropertiesWithInvalidLevels() throws Throwable {
        final TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPropertiesWithInvalidLevels(player, params);
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

    public void testPlayerStateTransition() throws Exception {
        IBasicMediaPlayer player = null;
        IPreAmp effect = null;

        try {
            // check effect settings are preserved along player state transition
            Object waitObj = new Object();
            CompletionListenerObject comp = new CompletionListenerObject(waitObj);
            SeekCompleteListenerObject seekComp = new SeekCompleteListenerObject(waitObj);

            player = createWrappedPlayerInstance();
            effect = createPreAmp();

            player.setOnCompletionListener(comp);
            player.setOnSeekCompleteListener(seekComp);

            // configure
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(true));
            effect.setLevel(0.5f);

            final IPreAmp.Settings expectedSettings = effect.getProperties();

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

    //
    // Implementations
    //
    private void checkSetAndGetEnabled(IBasicMediaPlayer player, TestParams params) {
        IPreAmp effect = null;

        try {
            effect = createPreAmp();

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
        IPreAmp effect = null;

        try {
            effect = createPreAmp();

            // check
            checkIsDefaultState(effect);

            // modify parameters
            effect.setEnabled(true);
            effect.setLevel(0.5f);

            // release
            effect.release();
            effect = null;

            // re-confirm with new instance
            effect = createPreAmp();

            checkIsDefaultState(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkLevelParamWithValidRange(
            IBasicMediaPlayer player, TestParams params) {
        IPreAmp effect = null;

        try {
            effect = createPreAmp();

            effect.setEnabled(params.getPreAmpEnabled());
            setAndCheckVaildLevel(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkLevelParamWithInvalidLevelRange(
            IBasicMediaPlayer player, TestParams params) {
        IPreAmp effect = null;

        try {
            effect = createPreAmp();

            effect.setEnabled(params.getPreAmpEnabled());
            setAndCheckLevelWithInvalidLevel(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesWithValidLevels(
            IBasicMediaPlayer player, TestParams params) {
        IPreAmp effect = null;

        try {
            effect = createPreAmp();

            effect.setEnabled(params.getPreAmpEnabled());
            setAndCheckValidLevelsProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesWithInvalidLevels(
            IBasicMediaPlayer player, TestParams params) {
        IPreAmp effect = null;

        try {
            effect = createPreAmp();

            effect.setEnabled(params.getPreAmpEnabled());
            setAndCheckInvalidLevelsProperties(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithNullSettings(IBasicMediaPlayer player) {
        IPreAmp effect = null;

        try {
            effect = createPreAmp();
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

    private void checkAfterRelease(IBasicMediaPlayer player, TestParams params) {
        try {
            createReleasedPreAmp(player).getId();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedPreAmp(player).getEnabled();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedPreAmp(player).hasControl();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedPreAmp(player).setEnabled(true);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedPreAmp(player).setEnabled(false);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedPreAmp(player).getLevel();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedPreAmp(player).getProperties();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            IPreAmp preamp = createPreAmp();
            IPreAmp.Settings settings = new IPreAmp.Settings();

            preamp.release();

            createReleasedPreAmp(player).setProperties(settings);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
    }

    private void checkHasControl(IBasicMediaPlayer player, TestParams params) {
        IPreAmp effect1 = null, effect2 = null, effect3 = null;

        try {
            // create instance 1
            // NOTE: [1]: has control, [2] not created, [3] not created
            effect1 = createPreAmp();

            assertTrue(effect1.hasControl());

            // create instance 2
            // NOTE: [1]: lost control, [2] has control, [3] not created
            effect2 = createPreAmp();

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
            effect3 = createPreAmp();

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
        IPreAmp effect1 = null, effect2 = null;

        try {
            effect1 = createPreAmp();
            effect2 = createPreAmp();

            final boolean initialEnabledState = effect2.getEnabled();
            final IPreAmp.Settings initialSettings = effect2.getProperties();

            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            // no state changing methods should not raise any errors
            assertEquals(effect2.getEnabled(), effect1.getEnabled());
            assertEquals(effect2.getId(), effect1.getId());
            assertEquals(effect2.getLevel(), effect1.getLevel());
            assertEquals(effect2.getProperties(), effect1.getProperties());

            // setEnabled() should return IAudioEffect.ERROR_INVALID_OPERATION
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(false));
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(true));

            // state changing methods should raise UnsupportedOperationException
            try {
                effect1.setLevel(0.5f);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            try {
                IPreAmp.Settings settings = new IPreAmp.Settings();

                settings.level = 0.5f;

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
        IPreAmp effect1 = null, effect2 = null;

        try {
            effect1 = createPreAmp();
            effect2 = createPreAmp();

            // check pre. conditions
            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            assertFalse(effect1.getEnabled());
            assertFalse(effect2.getEnabled());

            assertEquals(1.0f, effect1.getLevel());
            assertEquals(1.0f, effect2.getLevel());

            effect2.setLevel(0.25f);

            IPreAmp.Settings expectedSettings = effect2.getProperties();

            // check effect 1 lost controls
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(false));
            try {
                effect1.setLevel(0.5f);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertNotEquals(expectedSettings.level, 0.5f);
            assertEquals(expectedSettings.level, effect1.getLevel());

            try {
                IPreAmp.Settings settings = new IPreAmp.Settings();

                settings.level = 0.5f;

                effect1.setProperties(settings);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertEquals(expectedSettings, effect1.getProperties());

            // change states
            assertEquals(IAudioEffect.SUCCESS, effect2.setEnabled(true));

            // check post conditions
            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            assertTrue(effect1.getEnabled());
            assertTrue(effect2.getEnabled());

            assertEquals(expectedSettings, effect1.getProperties());
            assertEquals(expectedSettings, effect2.getProperties());

            // release effect 2
            effect2.release();
            effect2 = null;

            // check effect 1 gains control
            assertTrue(effect1.hasControl());
            assertEquals(IAudioEffect.SUCCESS, effect1.setEnabled(false));
        } finally {
            releaseQuietly(effect1);
            releaseQuietly(effect2);
        }
    }

    private void checkPlayerReleasedBeforeEffect(IBasicMediaPlayer player, TestParams params) {
        IPreAmp effect = null;
        try {
            effect = createPreAmp();

            // pre. check
            assertTrue(effect.hasControl());
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(true));

            // release player
            player.release();
            player = null;

            // post check
            assertTrue(effect.hasControl());
            assertEquals(true, effect.getEnabled());
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

    private void setAndCheckVaildLevel(IPreAmp preamp) {
        final float[] TEST_VALUES = {
                -0.0f, +0.0f, 0.5f, 1.5f, 2.0f
        };
        for (float v : TEST_VALUES) {
            preamp.setLevel(v);
            assertEquals(v, preamp.getLevel());
        }
    }

    private void setAndCheckLevelWithInvalidLevel(IPreAmp preamp) {
        final float[] TEST_VALUES = {
                -0.1f, 2.1f,
                Float.MAX_VALUE,
                Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY,
                Float.NaN
        };

        for (float v : TEST_VALUES) {
            final float prevLevel = preamp.getLevel();
            try {
                preamp.setLevel(v);
                fail();
            } catch (IllegalArgumentException e) {
                // expected
            }
            assertEquals(prevLevel, preamp.getLevel());
        }
    }

    private void setAndCheckValidLevelsProperties(IPreAmp preamp) {
        final float[] TEST_VALUES = {
                -0.0f, +0.0f, 0.5f, 1.5f, 2.0f
        };

        for (float v : TEST_VALUES) {
            final IPreAmp.Settings settings = preamp.getProperties();

            settings.level = v;
            preamp.setProperties(settings);

            assertEquals(settings, preamp.getProperties());
        }
    }

    private void setAndCheckInvalidLevelsProperties(IPreAmp preamp) {
        final float[] TEST_VALUES = {
                -0.1f, 2.1f,
                Float.MAX_VALUE,
                Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY,
                Float.NaN
        };

        for (float v : TEST_VALUES) {
            final IPreAmp.Settings prevSettings = preamp.getProperties();
            final IPreAmp.Settings settings = prevSettings.clone();

            try {
                settings.level = v;
                preamp.setProperties(settings);
                fail();
            } catch (IllegalArgumentException e) {
                // expected
            }

            assertEquals(prevSettings, preamp.getProperties());
        }
    }

    //
    // Utilities
    //

    private static void assertEquals(IPreAmp.Settings expected, IPreAmp.Settings actual) {
        assertEquals(expected.toString(), actual.toString());
    }

    private IPreAmp createReleasedPreAmp(IBasicMediaPlayer player) {
        IPreAmp preamp = createPreAmp();
        preamp.release();
        return preamp;
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

    private void checkIsDefaultState(IPreAmp effect) {
        assertEquals(false, effect.getEnabled());
        assertEquals(1.0f, effect.getLevel());
    }
}
