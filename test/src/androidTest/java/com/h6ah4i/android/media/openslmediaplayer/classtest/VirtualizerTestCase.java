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
import com.h6ah4i.android.media.audiofx.IVirtualizer;
import com.h6ah4i.android.media.audiofx.IVirtualizer.Settings;
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

public class VirtualizerTestCase
        extends BasicMediaPlayerTestCaseBase {

    private static final short DEFAULT_STRENGTH = 750;

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
                VirtualizerTestCase.class, params, filter, false));

        // not parameterized tests
        suite.addTest(makeSingleBasicTest(
                VirtualizerTestCase.class, "testPlayerStateTransition", factoryClazz));

        return suite;
    }

    public VirtualizerTestCase(ParameterizedTestArgs args) {
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
        IVirtualizer effect = null;

        try {
            // check effect settings are preserved along player state transition
            Object waitObj = new Object();
            CompletionListenerObject comp = new CompletionListenerObject(waitObj);
            SeekCompleteListenerObject seekComp = new SeekCompleteListenerObject(waitObj);

            player = createWrappedPlayerInstance();
            effect = getFactory().createVirtualizer(unwrap(player));

            player.setOnCompletionListener(comp);
            player.setOnSeekCompleteListener(seekComp);

            // configure
            assertEquals(IAudioEffect.SUCCESS, effect.setEnabled(true));
            effect.setStrength((short) 123);

            final IVirtualizer.Settings expectedSettings = effect.getProperties();

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

    private void checkDefaultParameters(IBasicMediaPlayer player) {
        IVirtualizer effect = null;

        try {
            effect = getFactory().createVirtualizer(player);

            // check
            checkIsDefaultState(effect);

            // modify parameters
            effect.setEnabled(true);
            effect.setStrength((short) 123);

            // release
            effect.release();
            effect = null;

            // re-confirm with new instance
            effect = getFactory().createVirtualizer(player);

            checkIsDefaultState(effect);
        } finally {
            releaseQuietly(effect);
        }
    }

    private void checkSetAndGetEnabled(IBasicMediaPlayer player) {
        IVirtualizer effect = null;

        try {
            effect = getFactory().createVirtualizer(player);

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
        IVirtualizer effect = null;

        try {
            effect = getFactory().createVirtualizer(player);

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
        IVirtualizer effect = null;

        try {
            effect = getFactory().createVirtualizer(player);
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

    private void checkStrengthParamWithInvalidRange(IBasicMediaPlayer player) {
        IVirtualizer effect = null;

        try {
            effect = getFactory().createVirtualizer(player);

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
            createReleasedVirtualizer(player).getId();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedVirtualizer(player).getEnabled();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedVirtualizer(player).hasControl();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedVirtualizer(player).setEnabled(true);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedVirtualizer(player).setEnabled(false);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedVirtualizer(player).getStrengthSupported();
            // this method should not raise any exceptions
        } catch (IllegalStateException e) {
            fail();
        }

        try {
            createReleasedVirtualizer(player).getRoundedStrength();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
        try {
            createReleasedVirtualizer(player).setStrength((short) 0);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedVirtualizer(player).getProperties();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedVirtualizer(player).setProperties(createSettings((short) 0));
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
    }

    private void checkHasControl(IBasicMediaPlayer player) {
        IVirtualizer effect1 = null, effect2 = null, effect3 = null;

        try {
            // create instance 1
            // NOTE: [1]: has control, [2] not created, [3] not created
            effect1 = getFactory().createVirtualizer(player);

            assertTrue(effect1.hasControl());

            // create instance 2
            // NOTE: [1]: lost control, [2] has control, [3] not created
            effect2 = getFactory().createVirtualizer(player);

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
            effect3 = getFactory().createVirtualizer(player);

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
        IVirtualizer effect1 = null, effect2 = null;

        try {
            effect1 = getFactory().createVirtualizer(player);
            effect2 = getFactory().createVirtualizer(player);

            final boolean initialEnabledState = effect2.getEnabled();
            final IVirtualizer.Settings initialSettings = effect2.getProperties();

            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            // no state changing methods should not raise any errors
            assertEquals(effect2.getEnabled(), effect1.getEnabled());
            assertEquals(effect2.getId(), effect1.getId());
            assertEquals(effect2.getStrengthSupported(), effect1.getStrengthSupported());
            assertEquals(effect2.getRoundedStrength(), effect1.getRoundedStrength());
            assertEquals(effect2.getProperties(), effect1.getProperties());

            // setEnabled() should return IAudioEffect.ERROR_INVALID_OPERATION
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(false));
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(true));

            // state changing methods should raise UnsupportedOperationException
            try {
                effect1.setStrength(DEFAULT_STRENGTH);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            try {
                effect1.setProperties(createSettings((short) 100));
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
        IVirtualizer effect1 = null, effect2 = null;

        try {
            effect1 = getFactory().createVirtualizer(player);
            effect2 = getFactory().createVirtualizer(player);

            // check pre. conditions
            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            assertFalse(effect1.getEnabled());
            assertFalse(effect2.getEnabled());

            assertEquals(DEFAULT_STRENGTH, effect1.getRoundedStrength());
            assertEquals(DEFAULT_STRENGTH, effect2.getRoundedStrength());

            // check effect 1 lost controls
            assertEquals(IAudioEffect.ERROR_INVALID_OPERATION, effect1.setEnabled(false));
            try {
                effect1.setStrength((short) 123);
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertEquals(DEFAULT_STRENGTH, effect1.getRoundedStrength());

            try {
                effect1.setProperties(createSettings((short) 123));
                fail();
            } catch (UnsupportedOperationException e) {
                // expected
            }
            assertEquals(DEFAULT_STRENGTH, effect1.getRoundedStrength());

            // change states
            assertEquals(IAudioEffect.SUCCESS, effect2.setEnabled(true));
            effect2.setStrength((short) 123);

            // check post conditions
            assertFalse(effect1.hasControl());
            assertTrue(effect2.hasControl());

            assertTrue(effect1.getEnabled());
            assertTrue(effect2.getEnabled());

            assertEquals((short) 123, effect1.getRoundedStrength());
            assertEquals((short) 123, effect2.getRoundedStrength());

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
        IVirtualizer effect = null;
        try {
            effect = getFactory().createVirtualizer(player);

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

    private void setAndCheckVaildStrength(IVirtualizer virtualizer) {
        // by setStrength()
        virtualizer.setStrength((short) 0);
        assertStrengthEquals((short) 0, virtualizer);

        virtualizer.setStrength((short) 500);
        assertStrengthEquals((short) 500, virtualizer);

        virtualizer.setStrength((short) 1000);
        assertStrengthEquals((short) 1000, virtualizer);

        // by setProperties()
        virtualizer.setProperties(createSettings((short) 0));
        assertStrengthEquals((short) 0, virtualizer);

        virtualizer.setProperties(createSettings((short) 500));
        assertStrengthEquals((short) 500, virtualizer);

        virtualizer.setProperties(createSettings((short) 1000));
        assertStrengthEquals((short) 1000, virtualizer);
    }

    private void setAndCheckInvalidStrength(IVirtualizer virtualizer) {
        short expected = 123;

        // set pre. condition
        virtualizer.setStrength(expected);
        assertStrengthEquals(expected, virtualizer);

        try {
            virtualizer.setStrength((short) -1);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        assertStrengthEquals(expected, virtualizer);

        try {
            virtualizer.setStrength((short) 1001);
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        assertStrengthEquals(expected, virtualizer);

        // by setProperties()
        try {
            virtualizer.setProperties(createSettings((short) -1));
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        assertStrengthEquals(expected, virtualizer);

        try {
            virtualizer.setProperties(createSettings((short) 1001));
            fail();
        } catch (IllegalArgumentException e) {
            // expected
        }
        assertStrengthEquals(expected, virtualizer);
    }

    //
    // Utilities
    //

    static void assertStrengthEquals(short expected, IVirtualizer virtualizer) {
        assertEquals(expected, virtualizer.getRoundedStrength());
        assertEquals(expected, virtualizer.getProperties().strength);
    }

    static IVirtualizer.Settings createSettings(short strength) {
        IVirtualizer.Settings settings = new Settings();
        settings.strength = strength;
        return settings;
    }

    private IVirtualizer createReleasedVirtualizer(IBasicMediaPlayer player) {
        IVirtualizer virtualizer = getFactory().createVirtualizer(player);
        virtualizer.release();
        return virtualizer;
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

    private void checkIsDefaultState(IVirtualizer effect) {
        assertEquals(false, effect.getEnabled());
        // XXX Default Strength
        // XXX HTC Evo 3D
        assertEquals(DEFAULT_STRENGTH, effect.getRoundedStrength());
    }

    private static void assertEquals(IVirtualizer.Settings expected, IVirtualizer.Settings actual) {
        assertEquals(expected.toString(), actual.toString());
    }

}
