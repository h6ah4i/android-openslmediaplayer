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
import com.h6ah4i.android.media.audiofx.ILoudnessEnhancer;
import com.h6ah4i.android.media.audiofx.ILoudnessEnhancer.Settings;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.media.test.utils.SeekCompleteListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class LoudnessEnhancerTestCase
        extends BasicMediaPlayerTestCaseBase {

    private static final short DEFAULT_TARGET_GAIN_MB = 0;

    private static final class TestParams extends BasicTestParams {
        private final PlayerState mPlayerState;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
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

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        TestSuite suite = new TestSuite();

        // parameterized tests
        ParameterizedTestSuiteBuilder.Filter filter =
                ParameterizedTestSuiteBuilder.notMatches("testPlayerStateTransition");

        List<TestParams> params = new ArrayList<TestParams>();

        List<PlayerState> playerStates = new ArrayList<PlayerState>();
        playerStates.addAll(Arrays.asList(PlayerState.values()));
        playerStates.remove(PlayerState.End);

        for (PlayerState playerState : playerStates) {
            params.add(new TestParams(factoryClazz, playerState));
        }

        suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                LoudnessEnhancerTestCase.class, params, filter, false));

        // not parameterized tests
        suite.addTest(makeSingleBasicTest(
                LoudnessEnhancerTestCase.class, "testPlayerStateTransition", factoryClazz));

        return suite;
    }

    public LoudnessEnhancerTestCase(ParameterizedTestArgs args) {
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
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkSetAndGetEnabled(player);
                    }
                });
    }

    public void testStrengthParamWithValidRange() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkStrengthParamWithValidRange(player);
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

    public void testStrengthParamWithInvalidRange() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkStrengthParamWithInvalidRange(player);
                    }
                });
    }

    public void testAfterRelease() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkAfterRelease(player);
                    }
                });
    }

    public void testHasControl() throws Throwable {
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkHasControl(player);
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
        TestParams params = (TestParams) getTestParams();

        checkWithNoPlayerErrors(
                params,
                new BasicMediaPlayerTestRunnable() {
                    @Override
                    public void run(IBasicMediaPlayer player, Object args) throws Throwable {
                        checkPlayerReleasedBeforeEffect(player);
                    }
                });
    }

    public void testPlayerStateTransition() throws Exception {
        IBasicMediaPlayer player = null;
        ILoudnessEnhancer effect = null;

        try {
            // check effect settings are preserved along player state transition
            Object waitObj = new Object();
            CompletionListenerObject comp = new CompletionListenerObject(waitObj);
            SeekCompleteListenerObject seekComp = new SeekCompleteListenerObject(waitObj);

            player = createWrappedPlayerInstance();
            effect = getFactory().createLoudnessEnhancer(unwrap(player));

            player.setOnCompletionListener(comp);
            player.setOnSeekCompleteListener(seekComp);

            // configure
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(true));
            effect.setTargetGain((short) 123);

            final ILoudnessEnhancer.Settings expectedSettings = effect.getPropertiesCompat();

            // player: idle

            // player: initialized
            setDataSourceForCommonTests(player, null);

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getPropertiesCompat());

            // player: prepared
            player.prepare();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getPropertiesCompat());

            // player: started
            player.start();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getPropertiesCompat());

            // player: paused
            player.pause();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getPropertiesCompat());

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
            assertEquals(expectedSettings, effect.getPropertiesCompat());

            // player: stop
            player.stop();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getPropertiesCompat());

            // player: idle
            player.reset();

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getPropertiesCompat());

            // player: end
            player.release();
            player = null;

            assertTrue(effect.getEnabled());
            assertEquals(expectedSettings, effect.getPropertiesCompat());
        } finally {
            releaseQuietly(player);
            releaseQuietly(effect);
        }
    }

    private void checkDefaultParameters(IBasicMediaPlayer player) {
        ILoudnessEnhancer effect = null;

        try {
            effect = getFactory().createLoudnessEnhancer(player);

            // check
            checkIsDefaultState(effect);

            // modify parameters
            effect.setEnabled(true);
            effect.setTargetGain((short) 123);

            // release
            effect.release();
            effect = null;

            // re-confirm with new instance
            effect = getFactory().createLoudnessEnhancer(player);

            checkIsDefaultState(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkSetAndGetEnabled(IBasicMediaPlayer player) {
        ILoudnessEnhancer effect = null;

        try {
            effect = getFactory().createLoudnessEnhancer(player);

            assertEquals(false, effect.getEnabled());

            effect.setEnabled(false);
            assertEquals(false, effect.getEnabled());

            effect.setEnabled(true);
            assertEquals(true, effect.getEnabled());
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkStrengthParamWithValidRange(IBasicMediaPlayer player) {
        ILoudnessEnhancer effect = null;

        try {
            effect = getFactory().createLoudnessEnhancer(player);

            // when not enabled
            effect.setEnabled(false);
            setAndCheckVaildStrength(effect);

            // when enabled
            effect.setEnabled(true);
            setAndCheckVaildStrength(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkPropertiesCompatWithNullSettings(IBasicMediaPlayer player) {
        ILoudnessEnhancer effect = null;

        try {
            effect = getFactory().createLoudnessEnhancer(player);
            try {
                effect.setPropertiesCompat(null);
                fail();
            } catch (IllegalArgumentException e) {
                // expected
            }

        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkStrengthParamWithInvalidRange(IBasicMediaPlayer player) {
        ILoudnessEnhancer effect = null;

        try {
            effect = getFactory().createLoudnessEnhancer(player);

            // when not enabled
            effect.setEnabled(false);
            setAndCheckInvalidStrength(effect);

            // when enabled
            effect.setEnabled(true);
            setAndCheckInvalidStrength(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkAfterRelease(IBasicMediaPlayer player) {
        try {
            createReleasedLoudnessEnhancer(player).getId();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedLoudnessEnhancer(player).getEnabled();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedLoudnessEnhancer(player).hasControl();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedLoudnessEnhancer(player).setEnabled(true);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedLoudnessEnhancer(player).setEnabled(false);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedLoudnessEnhancer(player).getTargetGain();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedLoudnessEnhancer(player).setTargetGain((short) 0);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedLoudnessEnhancer(player).getPropertiesCompat();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedLoudnessEnhancer(player).setPropertiesCompat(createSettings((short) 0));
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
    }

    private void checkHasControl(IBasicMediaPlayer player) {
        ILoudnessEnhancer effect1 = null, effect2 = null, effect3 = null;

        try {
            // create instance 1
            // NOTE: [1]: has control, [2] not created, [3] not created
            effect1 = getFactory().createLoudnessEnhancer(player);

            assertTrue(effect1.hasControl());

            // create instance 2
            // NOTE: [1]: lost control, [2] has control, [3] not created
            effect2 = getFactory().createLoudnessEnhancer(player);

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
            effect3 = getFactory().createLoudnessEnhancer(player);

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

            // XXX This assertion fails when Prepared state...
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
        ILoudnessEnhancer effect1 = null, effect2 = null;

        try {
            effect1 = getFactory().createLoudnessEnhancer(player);
            effect2 = getFactory().createLoudnessEnhancer(player);

            final boolean initialEnabledState = effect2.getEnabled();
            final ILoudnessEnhancer.Settings initialSettings = effect2.getPropertiesCompat();

            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            // no state changing methods should not raise any errors
            assertEquals(effect2.getEnabled(), effect1.getEnabled());
            assertEquals(effect2.getId(), effect1.getId());
            assertEquals(effect2.getTargetGain(), effect1.getTargetGain());
            assertEquals(effect2.getPropertiesCompat(), effect1.getPropertiesCompat());

            // setEnabled() should return IAudioEffect.ERROR_INVALID_OPERATION
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(false));
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(true));

            // state changing methods should raise UnsupportedOperationException
            try {
                effect1.setTargetGain(DEFAULT_TARGET_GAIN_MB);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            try {
                effect1.setPropertiesCompat(createSettings((short) 100));
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }

            // confirm object state
            assertEquals(initialEnabledState, effect1.getEnabled());
            assertEquals(initialSettings, effect1.getPropertiesCompat());
        } finally {
            releaseQuietly(effect1);
            effect1 = null;

            releaseQuietly(effect2);
            effect2 = null;
        }
    }

    private void checkMultiInstanceBehavior(IBasicMediaPlayer player) {
        ILoudnessEnhancer effect1 = null, effect2 = null;

        try {
            effect1 = getFactory().createLoudnessEnhancer(player);
            effect2 = getFactory().createLoudnessEnhancer(player);

            // check pre. conditions
            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            assertFalse(effect1.getEnabled());
            assertFalse(effect2.getEnabled());

            assertEquals((float) DEFAULT_TARGET_GAIN_MB, effect1.getTargetGain());
            assertEquals((float) DEFAULT_TARGET_GAIN_MB, effect2.getTargetGain());

            // check effect 1 lost controls
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(false));
            try {
                effect1.setTargetGain((short) 123);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertEquals((float) DEFAULT_TARGET_GAIN_MB, effect1.getTargetGain());

            try {
                effect1.setPropertiesCompat(createSettings((short) 123));
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertEquals((float) DEFAULT_TARGET_GAIN_MB, effect1.getTargetGain());

            // change states
            assertEquals(IAudioEffect.SUCCESS, effect2.setEnabled(true));
            effect2.setTargetGain((short) 123);

            // check post conditions
            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            assertTrue(effect1.getEnabled());
            assertTrue(effect2.getEnabled());

            assertEquals((float) 123, effect1.getTargetGain());
            assertEquals((float) 123, effect2.getTargetGain());

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

    private void checkPlayerReleasedBeforeEffect(IBasicMediaPlayer player) {
        ILoudnessEnhancer effect = null;
        try {
            effect = getFactory().createLoudnessEnhancer(player);

            // pre. check
            assertTrue(effect.hasControl());
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(true));

            // release player
            player.release();
            player = null;

            // post check
            assertTrue(effect.hasControl());
            assertEquals(true, effect.getEnabled());
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(false));

            // release effect
            effect.release();
            effect = null;
        } finally {
            releaseQuietly(effect);
            effect = null;
        }
    }

    private void setAndCheckVaildStrength(ILoudnessEnhancer loudnessEnhancer) {
        // by setTargetGain()
        loudnessEnhancer.setTargetGain(-1000);
        assertTargetGainEquals(-1000, loudnessEnhancer);

        loudnessEnhancer.setTargetGain(0);
        assertTargetGainEquals(0, loudnessEnhancer);

        loudnessEnhancer.setTargetGain(1000);
        assertTargetGainEquals(1000, loudnessEnhancer);

        loudnessEnhancer.setTargetGain(2000);
        assertTargetGainEquals(2000, loudnessEnhancer);

        // by setPropertiesCompat()
        loudnessEnhancer.setPropertiesCompat(createSettings(-1000));
        assertTargetGainEquals(-1000, loudnessEnhancer);

        loudnessEnhancer.setPropertiesCompat(createSettings(0));
        assertTargetGainEquals(0, loudnessEnhancer);

        loudnessEnhancer.setPropertiesCompat(createSettings(1000));
        assertTargetGainEquals(1000, loudnessEnhancer);

        loudnessEnhancer.setPropertiesCompat(createSettings(2000));
        assertTargetGainEquals(2000, loudnessEnhancer);
    }

    private void setAndCheckInvalidStrength(ILoudnessEnhancer loudnessEnhancer) {
        int expected = 123;

        // set pre. condition
        loudnessEnhancer.setTargetGain(expected);
        assertTargetGainEquals(expected, loudnessEnhancer);

        try {
            loudnessEnhancer.setTargetGain(2001);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        assertTargetGainEquals(expected, loudnessEnhancer);

        // by setPropertiesCompat()
        try {
            loudnessEnhancer.setPropertiesCompat(createSettings(2001));
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        assertTargetGainEquals(expected, loudnessEnhancer);
    }

    //
    // Utilities
    //

    static void assertTargetGainEquals(int expected, ILoudnessEnhancer loudnessEnhancer) {
        assertEquals((float) expected, loudnessEnhancer.getTargetGain());
        assertEquals(expected, loudnessEnhancer.getPropertiesCompat().targetGainmB);
    }

    static ILoudnessEnhancer.Settings createSettings(int targetGainMb) {
        ILoudnessEnhancer.Settings settings = new Settings();
        settings.targetGainmB = targetGainMb;
        return settings;
    }

    private ILoudnessEnhancer createReleasedLoudnessEnhancer(IBasicMediaPlayer player) {
        ILoudnessEnhancer loudnessEnhancer = getFactory().createLoudnessEnhancer(player);
        loudnessEnhancer.release();
        return loudnessEnhancer;
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

    private void checkIsDefaultState(ILoudnessEnhancer effect) {
        assertEquals(false, effect.getEnabled());
        assertEquals((float) DEFAULT_TARGET_GAIN_MB, effect.getTargetGain());
    }

    private static void assertEquals(ILoudnessEnhancer.Settings expected,
            ILoudnessEnhancer.Settings actual) {
        assertEquals(expected.toString(), actual.toString());
    }

}
