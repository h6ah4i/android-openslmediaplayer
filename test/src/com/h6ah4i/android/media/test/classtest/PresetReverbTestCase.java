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
import java.util.List;

import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.audiofx.IAudioEffect;
import com.h6ah4i.android.media.audiofx.IPresetReverb;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class PresetReverbTestCase
        extends BasicMediaPlayerTestCaseBase {

    private static final class TestParams extends BasicTestParams {
        private final boolean mEnabled;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                boolean enabled) {
            super(factoryClass);
            mEnabled = enabled;
        }

        public boolean getPreconditionEnabled() {
            return mEnabled;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mEnabled;
        }
    }

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        TestSuite suite = new TestSuite();

        String[] testsWithoutPreconditionEnabled = new String[] {
                "testSetAndGetEnabled",
                "testHasControl",
                "testMultiInstanceBehavior",
                "testAfterReleased",
        };

        // use TestParam.getPreconditionEnabled()
        {
            ParameterizedTestSuiteBuilder.Filter filter =
                    ParameterizedTestSuiteBuilder.notMatches(
                            testsWithoutPreconditionEnabled);
            List<TestParams> params = new ArrayList<TestParams>();

            params.add(new TestParams(factoryClazz, false));
            params.add(new TestParams(factoryClazz, true));

            suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                    PresetReverbTestCase.class, params, filter, false));
        }

        // don't use TestParam.getPreconditionEnabled()
        {
            ParameterizedTestSuiteBuilder.Filter filter =
                    ParameterizedTestSuiteBuilder.matches(
                            testsWithoutPreconditionEnabled);
            List<TestParams> params = new ArrayList<TestParams>();

            params.add(new TestParams(factoryClazz, false));

            suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                    PresetReverbTestCase.class, params, filter, false));
        }

        return suite;
    }

    public PresetReverbTestCase(ParameterizedTestArgs args) {
        super(args);
    }

    private static final short[] PRESETS = new short[] {
            IPresetReverb.PRESET_NONE,
            IPresetReverb.PRESET_SMALLROOM,
            IPresetReverb.PRESET_MEDIUMROOM,
            IPresetReverb.PRESET_LARGEROOM,
            IPresetReverb.PRESET_MEDIUMHALL,
            IPresetReverb.PRESET_LARGEHALL,
            IPresetReverb.PRESET_PLATE,
    };

    //
    // Exposed test cases
    //
    public void testSetAndGetEnabled() {
        IPresetReverb reverb = null;

        try {
            reverb = getFactory().createPresetReverb();

            assertFalse(reverb.getEnabled());

            assertEquals(IAudioEffect.SUCCESS, reverb.setEnabled(true));
            assertTrue(reverb.getEnabled());

            assertEquals(IAudioEffect.SUCCESS, reverb.setEnabled(false));
            assertFalse(reverb.getEnabled());
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testDefaultPreset() {
        TestParams params = (TestParams) getTestParams();
        IPresetReverb reverb = null;

        try {
            reverb = getFactory().createPresetReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            assertEquals(IPresetReverb.PRESET_NONE, reverb.getPreset());
            assertEquals(IPresetReverb.PRESET_NONE, reverb.getProperties().preset);
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetPreset() {
        TestParams params = (TestParams) getTestParams();
        IPresetReverb reverb = null;

        try {
            reverb = getFactory().createPresetReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short preset : PRESETS) {
                reverb.setPreset(preset);
                assertEquals(preset, reverb.getPreset());
                assertEquals(preset, reverb.getProperties().preset);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidPreset() {
        TestParams params = (TestParams) getTestParams();
        short[] INVALID_PRESETS = new short[] {
                (short) -1,
                (short) (IPresetReverb.PRESET_PLATE + 1),
        };
        IPresetReverb reverb = null;

        try {
            reverb = getFactory().createPresetReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short preset : INVALID_PRESETS) {
                try {
                    reverb.setPreset(preset);
                    fail();
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetPropertiesCompat() {
        TestParams params = (TestParams) getTestParams();
        IPresetReverb reverb = null;

        try {
            reverb = getFactory().createPresetReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short preset : PRESETS) {
                IPresetReverb.Settings settings = new IPresetReverb.Settings();

                settings.preset = preset;

                reverb.setProperties(settings);

                assertEquals(preset, reverb.getPreset());
                assertEquals(preset, reverb.getProperties().preset);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testGetId() {
        TestParams params = (TestParams) getTestParams();
        IPresetReverb reverb = null;

        try {
            reverb = getFactory().createPresetReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            assertNotEquals(0, reverb.getId());
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testHasControl() {
        IPresetReverb reverb1 = null, reverb2 = null, reverb3 = null;

        try {
            // create instance 1
            // NOTE: [1]: has control, [2] not created, [3] not created
            reverb1 = getFactory().createPresetReverb();

            assertTrue(reverb1.hasControl());

            // create instance 2
            // NOTE: [1]: lost control, [2] has control, [3] not created
            reverb2 = getFactory().createPresetReverb();

            assertFalse(reverb1.hasControl());
            assertTrue(reverb2.hasControl());

            // create instance 3
            // NOTE: [1]: lost control, [2] lost control, [3] not created
            reverb3 = getFactory().createPresetReverb();

            assertFalse(reverb1.hasControl());
            assertFalse(reverb2.hasControl());
            assertTrue(reverb3.hasControl());

            // release instance 3
            // NOTE: [1]: lost control, [2] has control, [3] released
            reverb3.release();
            reverb3 = null;

            assertFalse(reverb1.hasControl());
            assertTrue(reverb2.hasControl());

            // release instance 2
            // NOTE: [1]: lost control, [2] released, [3] released
            reverb2.release();
            reverb2 = null;

            assertTrue(reverb1.hasControl());
        } finally {
            releaseQuietly(reverb1);
            releaseQuietly(reverb2);
            releaseQuietly(reverb3);
        }
    }

    public void testMultiInstanceBehavior() {
        IPresetReverb reverb1 = null, reverb2 = null;

        try {
            reverb1 = getFactory().createPresetReverb();
            reverb2 = getFactory().createPresetReverb();

            // check pre. conditions
            assertFalse(reverb1.hasControl());
            assertTrue(reverb2.hasControl());

            assertFalse(reverb1.getEnabled());
            assertFalse(reverb2.getEnabled());

            assertEquals(IPresetReverb.PRESET_NONE, reverb1.getPreset());
            assertEquals(IPresetReverb.PRESET_NONE, reverb2.getPreset());

            // change states
            assertEquals(IAudioEffect.SUCCESS, reverb2.setEnabled(true));
            reverb2.setPreset(IPresetReverb.PRESET_SMALLROOM);

            // check post conditions
            assertFalse(reverb1.hasControl());
            assertTrue(reverb2.hasControl());

            assertTrue(reverb1.getEnabled());
            assertTrue(reverb2.getEnabled());

            assertEquals(IPresetReverb.PRESET_SMALLROOM, reverb1.getPreset());
            assertEquals(IPresetReverb.PRESET_SMALLROOM, reverb2.getPreset());

            // release effect 2
            reverb2.release();
            reverb2 = null;

            // check effect 1 gains control
            assertTrue(reverb1.hasControl());
            assertEquals(IAudioEffect.SUCCESS, reverb1.setEnabled(false));

        } finally {
            releaseQuietly(reverb1);
            releaseQuietly(reverb2);
        }
    }

    public void testAfterReleased() {
        try {
            createReleasedPresetReverb().hasControl();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedPresetReverb().getEnabled();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedPresetReverb().setEnabled(false);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedPresetReverb().getId();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedPresetReverb().getPreset();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedPresetReverb().setPreset(IPresetReverb.PRESET_NONE);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedPresetReverb().getProperties();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            IPresetReverb.Settings settings = new IPresetReverb.Settings();
            settings.preset = IPresetReverb.PRESET_NONE;

            createReleasedPresetReverb().setProperties(settings);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
    }

    public void testReleaseAfterAttachedPlayerReleased() throws Exception {
        TestParams params = (TestParams) getTestParams();
        IBasicMediaPlayer player = null;
        IPresetReverb reverb = null;

        try {
            player = createWrappedPlayerInstance();
            reverb = getFactory().createPresetReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            transitStateToPrepared(player, null);

            // attach the effect
            int effectId = reverb.getId();
            assertNotEquals(0, effectId);
            player.attachAuxEffect(effectId);

            // release player
            player.release();

            // release reverb
            reverb.release();
        } finally {
            releaseQuietly(player);
            releaseQuietly(reverb);
        }
    }

    public void testReleaseBeforeAttachedPlayerReleased() throws Exception {
        TestParams params = (TestParams) getTestParams();
        IBasicMediaPlayer player = null;
        IPresetReverb reverb = null;

        try {
            player = createWrappedPlayerInstance();
            reverb = getFactory().createPresetReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            transitStateToPrepared(player, null);

            // attach the effect
            int effectId = reverb.getId();
            assertNotEquals(0, effectId);
            player.attachAuxEffect(effectId);

            // release reverb
            reverb.release();

            // release player
            player.release();
        } finally {
            releaseQuietly(player);
            releaseQuietly(reverb);
        }
    }

    public void testReleaseAfterFactoryReleased() throws Exception {
        IPresetReverb reverb = null;

        try {
            reverb = getFactory().createPresetReverb();

            IPresetReverb.Settings origSettings = reverb.getProperties();
            getFactory().release();

            // NOTE: The reverb object is still usable

            assertTrue(reverb.hasControl());
            assertFalse(reverb.getEnabled());
            assertNotEquals(0, reverb.getId());
            reverb.getPreset();
            reverb.setPreset(IPresetReverb.PRESET_NONE);
            reverb.getProperties();
            reverb.setProperties(origSettings);

            reverb.release();
            reverb = null;
        } finally {
            releaseQuietly(reverb);
        }
    }

    //
    // Utilities
    //

    static void assertEquals(IPresetReverb.Settings expected, IPresetReverb.Settings actual) {
        assertEquals(expected.toString(), actual.toString());
    }

    IPresetReverb createReleasedPresetReverb() {
        IPresetReverb reverb = getFactory().createPresetReverb();
        reverb.release();
        return reverb;
    }
}
