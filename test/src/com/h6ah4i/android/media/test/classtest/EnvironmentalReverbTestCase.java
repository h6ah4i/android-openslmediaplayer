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
import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.utils.EnvironmentalReverbPresets;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class EnvironmentalReverbTestCase
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
                    EnvironmentalReverbTestCase.class, params, filter, false));
        }

        // don't use TestParam.getPreconditionEnabled()
        {
            ParameterizedTestSuiteBuilder.Filter filter =
                    ParameterizedTestSuiteBuilder.matches(
                            testsWithoutPreconditionEnabled);
            List<TestParams> params = new ArrayList<TestParams>();

            params.add(new TestParams(factoryClazz, false));

            suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                    EnvironmentalReverbTestCase.class, params, filter, false));
        }

        return suite;
    }

    public EnvironmentalReverbTestCase(ParameterizedTestArgs args) {
        super(args);
    }

    //
    // Exposed test cases
    //
    public void testSetAndGetEnabled() {
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            assertFalse(reverb.getEnabled());

            assertEquals(IAudioEffect.SUCCESS, reverb.setEnabled(true));
            assertTrue(reverb.getEnabled());

            assertEquals(IAudioEffect.SUCCESS, reverb.setEnabled(false));
            assertFalse(reverb.getEnabled());
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetRoomLevel() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                ROOM_LEVEL_MIN, ROOM_LEVEL_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                reverb.setRoomLevel(value);
                assertEquals(value, reverb.getRoomLevel());
                assertEquals(value, reverb.getProperties().roomLevel);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidRoomLevel() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                (short) (ROOM_LEVEL_MIN - 1), (short) (ROOM_LEVEL_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                try {
                    reverb.setRoomLevel(value);
                    fail("expected = " + value + ", actual = " + reverb.getRoomLevel());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetRoomHFLevel() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                ROOM_HF_LEVEL_MIN, ROOM_HF_LEVEL_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                reverb.setRoomHFLevel(value);
                assertEquals(value, reverb.getRoomHFLevel());
                assertEquals(value, reverb.getProperties().roomHFLevel);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidRoomHFLevel() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                (short) (ROOM_HF_LEVEL_MIN - 1), (short) (ROOM_HF_LEVEL_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                try {
                    reverb.setRoomHFLevel(value);
                    fail("expected = " + value + ", actual = " + reverb.getRoomHFLevel());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetDecayTime() {
        TestParams params = (TestParams) getTestParams();
        int[] VALUES = new int[] {
                DECAY_TIME_MIN, DECAY_TIME_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (int value : VALUES) {
                reverb.setDecayTime(value);
                assertEquals(value, reverb.getDecayTime());
                assertEquals(value, reverb.getProperties().decayTime);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidDecayTime() {
        TestParams params = (TestParams) getTestParams();
        int[] VALUES = new int[] {
                (DECAY_TIME_MIN - 1), (DECAY_TIME_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (int value : VALUES) {
                try {
                    reverb.setDecayTime(value);
                    fail("expected = " + value + ", actual = " + reverb.getDecayTime());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetDecayHFRatio() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                DECAY_HF_RATIO_MIN, DECAY_HF_RATIO_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                reverb.setDecayHFRatio(value);
                assertEquals(value, reverb.getDecayHFRatio());
                assertEquals(value, reverb.getProperties().decayHFRatio);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidDecayHFRatio() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                (short) (DECAY_HF_RATIO_MIN - 1), (short) (DECAY_HF_RATIO_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                try {
                    reverb.setDecayHFRatio(value);
                    fail("expected = " + value + ", actual = " + reverb.getDecayHFRatio());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetReflectionsLevel() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                REFLECTIONS_LEVEL_MIN, REFLECTIONS_LEVEL_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                reverb.setReflectionsLevel(value);
                assertEquals(value, reverb.getReflectionsLevel());
                assertEquals(value, reverb.getProperties().reflectionsLevel);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidReflectionsLevel() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                (short) (REFLECTIONS_LEVEL_MIN - 1), (short) (REFLECTIONS_LEVEL_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                try {
                    reverb.setReflectionsLevel(value);
                    fail("expected = " + value + ", actual = " + reverb.getReflectionsLevel());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetReflectionsDelay() {
        TestParams params = (TestParams) getTestParams();
        int[] VALUES = new int[] {
                REFLECTIONS_DELAY_MIN, REFLECTIONS_DELAY_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (int value : VALUES) {
                reverb.setReflectionsDelay(value);
                assertEquals(value, reverb.getReflectionsDelay());
                assertEquals(value, reverb.getProperties().reflectionsDelay);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidReflectionsDelay() {
        TestParams params = (TestParams) getTestParams();
        int[] VALUES = new int[] {
                (REFLECTIONS_DELAY_MIN - 1), (REFLECTIONS_DELAY_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (int value : VALUES) {
                try {
                    reverb.setReflectionsDelay(value);
                    fail("expected = " + value + ", actual = " + reverb.getReflectionsDelay());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetReverbLevel() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                REVERB_LEVEL_MIN, REVERB_LEVEL_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                reverb.setReverbLevel(value);
                assertEquals(value, reverb.getReverbLevel());
                assertEquals(value, reverb.getProperties().reverbLevel);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidReverbLevel() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                (short) (REVERB_LEVEL_MIN - 1), (short) (REVERB_LEVEL_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                try {
                    reverb.setReverbLevel(value);
                    fail("expected = " + value + ", actual = " + reverb.getReverbLevel());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetReverbDelay() {
        TestParams params = (TestParams) getTestParams();
        int[] VALUES = new int[] {
                REVERB_DELAY_MIN, REVERB_DELAY_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (int value : VALUES) {
                reverb.setReverbDelay(value);
                assertEquals(value, reverb.getReverbDelay());
                assertEquals(value, reverb.getProperties().reverbDelay);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidReverbDelay() {
        TestParams params = (TestParams) getTestParams();
        int[] VALUES = new int[] {
                (REVERB_DELAY_MIN - 1), (REVERB_DELAY_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (int value : VALUES) {
                try {
                    reverb.setReverbDelay(value);
                    fail("expected = " + value + ", actual = " + reverb.getReverbDelay());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetDiffusion() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                DIFFUSION_MIN, DIFFUSION_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                reverb.setDiffusion(value);
                assertEquals(value, reverb.getDiffusion());
                // XXX HTC Evo 3D
                assertEquals(value, reverb.getProperties().diffusion);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidDiffusion() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                (short) (DIFFUSION_MIN - 1), (short) (DIFFUSION_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                try {
                    reverb.setDiffusion(value);
                    fail("expected = " + value + ", actual = " + reverb.getDiffusion());
                } catch (IllegalArgumentException e) {
                    // expected
                }
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetAndGetDensity() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                DENSITY_MIN, DENSITY_MAX
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                reverb.setDensity(value);
                assertEquals(value, reverb.getDensity());
                assertEquals(value, reverb.getProperties().density);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testSetInvalidDensity() {
        TestParams params = (TestParams) getTestParams();
        short[] VALUES = new short[] {
                (short) (DENSITY_MIN - 1), (short) (DENSITY_MAX + 1)
        };
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (short value : VALUES) {
                try {
                    reverb.setDensity(value);
                    fail("expected = " + value + ", actual = " + reverb.getDensity());
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
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            for (int i = 0; i < PRESETS.length; i++) {
                IEnvironmentalReverb.Settings expected = PRESETS[i];

                reverb.setProperties(expected);

                IEnvironmentalReverb.Settings actual = reverb.getProperties();

                assertEqualsExceptForNotImplementedParams(expected, actual);
            }
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testGetId() {
        TestParams params = (TestParams) getTestParams();
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            reverb.setEnabled(params.getPreconditionEnabled());

            assertNotEquals(0, reverb.getId());
        } finally {
            releaseQuietly(reverb);
        }
    }

    public void testHasControl() {
        IEnvironmentalReverb reverb1 = null, reverb2 = null, reverb3 = null;

        try {
            // create instance 1
            // NOTE: [1]: has control, [2] not created, [3] not created
            reverb1 = getFactory().createEnvironmentalReverb();

            assertTrue(reverb1.hasControl());

            // create instance 2
            // NOTE: [1]: lost control, [2] has control, [3] not created
            reverb2 = getFactory().createEnvironmentalReverb();

            assertFalse(reverb1.hasControl());
            assertTrue(reverb2.hasControl());

            // create instance 3
            // NOTE: [1]: lost control, [2] lost control, [3] not created
            reverb3 = getFactory().createEnvironmentalReverb();

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
        IEnvironmentalReverb reverb1 = null, reverb2 = null;

        try {
            reverb1 = getFactory().createEnvironmentalReverb();
            reverb2 = getFactory().createEnvironmentalReverb();

            // check pre. conditions
            assertFalse(reverb1.hasControl());
            assertTrue(reverb2.hasControl());

            assertFalse(reverb1.getEnabled());
            assertFalse(reverb2.getEnabled());

            assertEquals(reverb1.getProperties(), reverb2.getProperties());

            IEnvironmentalReverb.Settings expectedPreset = EnvironmentalReverbPresets.CAVE;

            // change states
            assertEquals(IAudioEffect.SUCCESS, reverb2.setEnabled(true));
            reverb2.setProperties(expectedPreset);

            // check post conditions
            assertFalse(reverb1.hasControl());
            assertTrue(reverb2.hasControl());

            assertTrue(reverb1.getEnabled());
            assertTrue(reverb2.getEnabled());

            assertEquals(expectedPreset.roomLevel, reverb1.getRoomLevel());
            assertEquals(expectedPreset.roomLevel, reverb2.getRoomLevel());

            assertEquals(expectedPreset.roomHFLevel, reverb1.getRoomHFLevel());
            assertEquals(expectedPreset.roomHFLevel, reverb2.getRoomHFLevel());

            assertEquals(expectedPreset.decayTime, reverb1.getDecayTime());
            assertEquals(expectedPreset.decayTime, reverb2.getDecayTime());

            assertEquals(expectedPreset.decayHFRatio, reverb1.getDecayHFRatio());
            assertEquals(expectedPreset.decayHFRatio, reverb2.getDecayHFRatio());

            // NOTE: This parameter is not implemented yet (Android 4.4)
            // assertEquals(expectedPreset.reflectionsLevel,
            // reverb1.getReflectionsLevel());
            // assertEquals(expectedPreset.reflectionsLevel,
            // reverb2.getReflectionsLevel());
            assertEquals(0, reverb1.getReflectionsLevel());
            assertEquals(0, reverb2.getReflectionsLevel());

            // NOTE: This parameter is not implemented yet (Android 4.4)
            // assertEquals(expectedPreset.reflectionsDelay,
            // reverb1.getReflectionsDelay());
            // assertEquals(expectedPreset.reflectionsDelay,
            // reverb2.getReflectionsDelay());
            assertEquals(0, reverb1.getReflectionsDelay());
            assertEquals(0, reverb2.getReflectionsDelay());

            assertEquals(expectedPreset.reverbLevel, reverb1.getReverbLevel());
            assertEquals(expectedPreset.reverbLevel, reverb2.getReverbLevel());

            // NOTE: This parameter is not implemented yet (Android 4.4)
            // assertEquals(expectedPreset.reverbDelay,
            // reverb1.getReverbDelay());
            // assertEquals(expectedPreset.reverbDelay,
            // reverb2.getReverbDelay());
            assertEquals(0, reverb1.getReverbDelay());
            assertEquals(0, reverb2.getReverbDelay());

            // XXX HTC Evo 3D
            assertEquals(expectedPreset.diffusion, reverb1.getDiffusion());
            assertEquals(expectedPreset.diffusion, reverb2.getDiffusion());

            assertEquals(expectedPreset.density, reverb1.getDensity());
            assertEquals(expectedPreset.density, reverb2.getDensity());

            assertEqualsExceptForNotImplementedParams(expectedPreset, reverb1.getProperties());
            assertEqualsExceptForNotImplementedParams(expectedPreset, reverb2.getProperties());

            // release effect 2
            reverb2.release();
            reverb2 = null;

            // check effect gains control
            // XXX This assertion may be fail when using StandardMediaPlayer
            assertTrue(reverb1.hasControl());
            assertEquals(IAudioEffect.SUCCESS, reverb1.setEnabled(false));

        } finally {
            releaseQuietly(reverb1);
            releaseQuietly(reverb2);
        }
    }

    public void testAfterReleased() {
        try {
            createReleasedEnvironmentalReverb().hasControl();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getEnabled();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setEnabled(false);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getId();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getRoomLevel();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setRoomLevel(ROOM_LEVEL_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getRoomHFLevel();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setRoomHFLevel(ROOM_HF_LEVEL_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getDecayTime();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setDecayTime(DECAY_TIME_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getDecayHFRatio();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setDecayHFRatio(DECAY_HF_RATIO_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getReflectionsLevel();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setReflectionsLevel(REFLECTIONS_LEVEL_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getReflectionsDelay();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setReflectionsDelay(REFLECTIONS_DELAY_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getReverbDelay();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setReverbDelay(REVERB_DELAY_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getDiffusion();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setDiffusion(DIFFUSION_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getDensity();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setDensity(DENSITY_MIN);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().getProperties();
            fail();
        } catch (IllegalStateException e) {
            // expected
        }

        try {
            createReleasedEnvironmentalReverb().setProperties(PRESETS[0]);
            fail();
        } catch (IllegalStateException e) {
            // expected
        }
    }

    public void testReleaseAfterAttachedPlayerReleased() throws Exception {
        IBasicMediaPlayer player = null;
        IEnvironmentalReverb reverb = null;

        try {
            player = createWrappedPlayerInstance();
            reverb = getFactory().createEnvironmentalReverb();

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
        IBasicMediaPlayer player = null;
        IEnvironmentalReverb reverb = null;

        try {
            player = createWrappedPlayerInstance();
            reverb = getFactory().createEnvironmentalReverb();

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
        IEnvironmentalReverb reverb = null;

        try {
            reverb = getFactory().createEnvironmentalReverb();

            IEnvironmentalReverb.Settings origSettings = reverb.getProperties();
            getFactory().release();

            // NOTE: The reverb object is still usable

            assertTrue(reverb.hasControl());
            assertFalse(reverb.getEnabled());
            assertNotEquals(0, reverb.getId());
            reverb.getRoomLevel();
            reverb.setRoomLevel(ROOM_LEVEL_MIN);
            reverb.getRoomHFLevel();
            reverb.setRoomHFLevel(ROOM_HF_LEVEL_MIN);
            reverb.getDecayHFRatio();
            reverb.setDecayHFRatio(DECAY_HF_RATIO_MIN);
            reverb.getDecayTime();
            reverb.setDecayTime(DECAY_TIME_MIN);
            reverb.getReflectionsLevel();
            reverb.setReflectionsLevel(REFLECTIONS_LEVEL_MIN);
            reverb.getReflectionsDelay();
            reverb.setReflectionsDelay(REFLECTIONS_DELAY_MIN);
            reverb.getDiffusion();
            reverb.setDiffusion(DIFFUSION_MIN);
            reverb.getDensity();
            reverb.setDensity(DENSITY_MIN);
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

    static void assertEquals(
            IEnvironmentalReverb.Settings expected,
            IEnvironmentalReverb.Settings actual) {
        assertEquals(expected.toString(), actual.toString());
    }

    static void assertEqualsExceptForNotImplementedParams(
            IEnvironmentalReverb.Settings expected,
            IEnvironmentalReverb.Settings actual) {
        assertEquals(expected.roomLevel, actual.roomLevel);
        assertEquals(expected.roomHFLevel, actual.roomHFLevel);
        assertEquals(expected.decayTime, actual.decayTime);
        assertEquals(expected.decayHFRatio, actual.decayHFRatio);
        assertEquals(0, actual.reflectionsLevel);
        assertEquals(0, actual.reflectionsDelay);
        // assertEquals(expected.reflectionsLevel, actual.reflectionsLevel);
        // assertEquals(expected.reflectionsDelay, actual.reflectionsDelay);
        assertEquals(expected.reverbLevel, actual.reverbLevel);
        // assertEquals(expected.reverbDelay, actual.reverbDelay);
        assertEquals(expected.diffusion, actual.diffusion);
        assertEquals(expected.density, actual.density);

    }

    IEnvironmentalReverb createReleasedEnvironmentalReverb() {
        IEnvironmentalReverb reverb = getFactory().createEnvironmentalReverb();
        reverb.release();
        return reverb;
    }

    static void verifySettings(IEnvironmentalReverb.Settings settings) {
        assertRange(settings.roomLevel, ROOM_LEVEL_MIN, ROOM_LEVEL_MAX);
        assertRange(settings.roomHFLevel, ROOM_HF_LEVEL_MIN, ROOM_HF_LEVEL_MAX);
        assertRange(settings.decayTime, DECAY_TIME_MIN, DECAY_TIME_MAX);
        assertRange(settings.decayHFRatio, DECAY_HF_RATIO_MIN, DECAY_HF_RATIO_MAX);
        assertRange(settings.reflectionsLevel, REFLECTIONS_LEVEL_MIN, REFLECTIONS_LEVEL_MAX);
        assertRange(settings.reverbDelay, REFLECTIONS_DELAY_MIN, REFLECTIONS_DELAY_MAX);
        assertRange(settings.reverbLevel, REVERB_LEVEL_MIN, REVERB_LEVEL_MAX);
        assertRange(settings.reflectionsDelay, REVERB_DELAY_MIN, REVERB_DELAY_MAX);
        assertRange(settings.diffusion, DIFFUSION_MIN, DIFFUSION_MAX);
        assertRange(settings.density, DENSITY_MIN, DENSITY_MAX);
    }

    private static final short ROOM_LEVEL_MIN = (short) -9000;
    private static final short ROOM_LEVEL_MAX = (short) 0;
    private static final short ROOM_HF_LEVEL_MIN = (short) -9000;
    private static final short ROOM_HF_LEVEL_MAX = (short) 0;
    private static final int DECAY_TIME_MIN = 100;
    /* Spec.: 20000, Actually(LVREV_MAX_T60): 7000 */
    private static final int DECAY_TIME_MAX = 7000;
    private static final short DECAY_HF_RATIO_MIN = (short) 100;
    private static final short DECAY_HF_RATIO_MAX = (short) 2000;
    /* Spec.: -9000, Actually: 0 (not implemented yet) */
    private static final short REFLECTIONS_LEVEL_MIN = (short) 0;
    /* Spec.: 1000, Actually: 0 (not implemented yet) */
    private static final short REFLECTIONS_LEVEL_MAX = (short) 0;
    /* Spec.: 0, Actually: 0 (not implemented yet) */
    private static final int REFLECTIONS_DELAY_MIN = 0;
    /* Spec.: 300, Actually: 0 (not implemented yet) */
    private static final int REFLECTIONS_DELAY_MAX = 0;
    private static final short REVERB_LEVEL_MIN = (short) -9000;
    private static final short REVERB_LEVEL_MAX = (short) 2000;
    /* Spec.: 0, Actually: 0 (not implemented yet) */
    private static final int REVERB_DELAY_MIN = 0;
    /* Spec.: 100, Actually: 0 (not implemented yet) */
    private static final int REVERB_DELAY_MAX = 0;
    private static final short DIFFUSION_MIN = (short) 0;
    private static final short DIFFUSION_MAX = (short) 1000;
    private static final short DENSITY_MIN = (short) 0;
    private static final short DENSITY_MAX = (short) 1000;

    private static final IEnvironmentalReverb.Settings[] PRESETS = new IEnvironmentalReverb.Settings[] {
            EnvironmentalReverbPresets.DEFAULT,
            EnvironmentalReverbPresets.GENERIC,
            EnvironmentalReverbPresets.PADDEDCELL,
            EnvironmentalReverbPresets.ROOM,
            EnvironmentalReverbPresets.BATHROOM,
            EnvironmentalReverbPresets.LIVINGROOM,
            EnvironmentalReverbPresets.STONEROOM,
            EnvironmentalReverbPresets.AUDITORIUM,
            EnvironmentalReverbPresets.CONCERTHALL,
            EnvironmentalReverbPresets.CAVE,
            EnvironmentalReverbPresets.ARENA,
            EnvironmentalReverbPresets.HANGAR,
            EnvironmentalReverbPresets.CARPETEDHALLWAY,
            EnvironmentalReverbPresets.HALLWAY,
            EnvironmentalReverbPresets.STONECORRIDOR,
            EnvironmentalReverbPresets.ALLEY,
            EnvironmentalReverbPresets.FOREST,
            EnvironmentalReverbPresets.CITY,
            EnvironmentalReverbPresets.MOUNTAINS,
            EnvironmentalReverbPresets.QUARRY,
            EnvironmentalReverbPresets.PLAIN,
            EnvironmentalReverbPresets.PARKINGLOT,
            EnvironmentalReverbPresets.SEWERPIPE,
            EnvironmentalReverbPresets.UNDERWATER,
            EnvironmentalReverbPresets.SMALLROOM,
            EnvironmentalReverbPresets.MEDIUMROOM,
            EnvironmentalReverbPresets.LARGEROOM,
            EnvironmentalReverbPresets.MEDIUMHALL,
            EnvironmentalReverbPresets.LARGEHALL,
            EnvironmentalReverbPresets.PLATE,
    };
}
