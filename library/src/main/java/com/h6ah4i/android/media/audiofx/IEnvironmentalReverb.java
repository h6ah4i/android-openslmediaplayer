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

/// ===============================================================
// Most of declarations and Javadoc comments are copied from
// /frameworks/base/media/java/android/media/audiofx/EnvironmentalReverb.java
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/// ===============================================================

package com.h6ah4i.android.media.audiofx;

import java.util.StringTokenizer;

public interface IEnvironmentalReverb extends IAudioEffect {
    /**
     * The Settings class regroups all environmental reverb parameters. It is
     * used in conjunction with getProperties() and setProperties()
     * methods to backup and restore all parameters in a single call.
     */
    public static class Settings implements Cloneable {
        public short roomLevel;
        public short roomHFLevel;
        public int decayTime;
        public short decayHFRatio;
        public short reflectionsLevel;
        public int reflectionsDelay;
        public short reverbLevel;
        public int reverbDelay;
        public short diffusion;
        public short density;

        public Settings() {
        }

        /**
         * Settings class constructor from a key=value; pairs formatted string.
         * The string is typically returned by Settings.toString() method.
         *
         * @throws IllegalArgumentException if the string is not correctly
         *             formatted.
         */
        public Settings(String settings) {
            StringTokenizer st = new StringTokenizer(settings, "=;");
            // int tokens = st.countTokens();
            if (st.countTokens() != 21) {
                throw new IllegalArgumentException("settings: " + settings);
            }
            String key = st.nextToken();
            if (!key.equals("EnvironmentalReverb")) {
                throw new IllegalArgumentException(
                        "invalid settings for EnvironmentalReverb: " + key);
            }

            try {
                key = st.nextToken();
                if (!key.equals("roomLevel")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                roomLevel = Short.parseShort(st.nextToken());
                key = st.nextToken();
                if (!key.equals("roomHFLevel")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                roomHFLevel = Short.parseShort(st.nextToken());
                key = st.nextToken();
                if (!key.equals("decayTime")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                decayTime = Integer.parseInt(st.nextToken());
                key = st.nextToken();
                if (!key.equals("decayHFRatio")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                decayHFRatio = Short.parseShort(st.nextToken());
                key = st.nextToken();
                if (!key.equals("reflectionsLevel")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                reflectionsLevel = Short.parseShort(st.nextToken());
                key = st.nextToken();
                if (!key.equals("reflectionsDelay")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                reflectionsDelay = Integer.parseInt(st.nextToken());
                key = st.nextToken();
                if (!key.equals("reverbLevel")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                reverbLevel = Short.parseShort(st.nextToken());
                key = st.nextToken();
                if (!key.equals("reverbDelay")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                reverbDelay = Integer.parseInt(st.nextToken());
                key = st.nextToken();
                if (!key.equals("diffusion")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                diffusion = Short.parseShort(st.nextToken());
                key = st.nextToken();
                if (!key.equals("density")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                density = Short.parseShort(st.nextToken());
            } catch (NumberFormatException nfe) {
                throw new IllegalArgumentException("invalid value for key: " + key);
            }
        }

        @Override
        public String toString() {
            return new String(
                    "EnvironmentalReverb" +
                            ";roomLevel=" + Short.toString(roomLevel) +
                            ";roomHFLevel=" + Short.toString(roomHFLevel) +
                            ";decayTime=" + Integer.toString(decayTime) +
                            ";decayHFRatio=" + Short.toString(decayHFRatio) +
                            ";reflectionsLevel=" + Short.toString(reflectionsLevel) +
                            ";reflectionsDelay=" + Integer.toString(reflectionsDelay) +
                            ";reverbLevel=" + Short.toString(reverbLevel) +
                            ";reverbDelay=" + Integer.toString(reverbDelay) +
                            ";diffusion=" + Short.toString(diffusion) +
                            ";density=" + Short.toString(density));
        }

        @Override
        public Settings clone() {
            try {
                return (Settings) super.clone();
            } catch (CloneNotSupportedException e) {
                return null;
            }
        }
    };

    /**
     * Room level. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_ROOM_LEVEL = android.media.audiofx.EnvironmentalReverb.PARAM_ROOM_LEVEL;
    /**
     * Room HF level. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_ROOM_HF_LEVEL = android.media.audiofx.EnvironmentalReverb.PARAM_ROOM_HF_LEVEL;
    /**
     * Decay time. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_DECAY_TIME = android.media.audiofx.EnvironmentalReverb.PARAM_DECAY_TIME;
    /**
     * Decay HF ratio. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_DECAY_HF_RATIO = android.media.audiofx.EnvironmentalReverb.PARAM_DECAY_HF_RATIO;
    /**
     * Early reflections level. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_REFLECTIONS_LEVEL = android.media.audiofx.EnvironmentalReverb.PARAM_REFLECTIONS_LEVEL;
    /**
     * Early reflections delay. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_REFLECTIONS_DELAY = android.media.audiofx.EnvironmentalReverb.PARAM_REFLECTIONS_DELAY;
    /**
     * Reverb level. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_REVERB_LEVEL = android.media.audiofx.EnvironmentalReverb.PARAM_REVERB_LEVEL;
    /**
     * Reverb delay. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_REVERB_DELAY = android.media.audiofx.EnvironmentalReverb.PARAM_REVERB_DELAY;
    /**
     * Diffusion. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_DIFFUSION = android.media.audiofx.EnvironmentalReverb.PARAM_DIFFUSION;
    /**
     * Density. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_DENSITY = android.media.audiofx.EnvironmentalReverb.PARAM_DENSITY;

    /**
     * The OnParameterChangeListener interface defines a method called by the
     * EnvironmentalReverb when a parameter value has changed.
     */
    public interface OnParameterChangeListener {
        /**
         * Method called when a parameter value has changed. The method is
         * called only if the parameter was changed by another application
         * having the control of the same EnvironmentalReverb engine.
         *
         * @param effect the EnvironmentalReverb on which the interface is
         *            registered.
         * @param status status of the set parameter operation.
         * @param param ID of the modified parameter. See
         *            {@link #PARAM_ROOM_LEVEL} ...
         * @param value the new parameter value.
         */
        void onParameterChange(IEnvironmentalReverb effect, int status, int param, int value);
    }

    /**
     * Sets the master volume level of the environmental reverb effect.
     *
     * @param room room level in millibels. The valid range is [-9000, 0].
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setRoomLevel(short room)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the volume level at 5 kHz relative to the volume level at low
     * frequencies of the overall reverb effect.
     * <p>
     * This controls a low-pass filter that will reduce the level of the
     * high-frequency.
     *
     * @param roomHF high frequency attenuation level in millibels. The valid
     *            range is [-9000, 0].
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setRoomHFLevel(short roomHF)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the volume level of the late reverberation.
     * <p>
     * This level is combined with the overall room level (set using
     * {@link #setRoomLevel(short)}).
     *
     * @param reverbLevel reverb level in millibels. The valid range is [-9000,
     *            2000].
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setReverbLevel(short reverbLevel)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the time between the first reflection and the reverberation.
     *
     * @param reverbDelay reverb delay in milliseconds. The valid range is [0,
     *            100].
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setReverbDelay(int reverbDelay)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the volume level of the early reflections.
     * <p>
     * This level is combined with the overall room level (set using
     * {@link #setRoomLevel(short)}).
     *
     * @param reflectionsLevel reflection level in millibels. The valid range is
     *            [-9000, 1000].
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setReflectionsLevel(short reflectionsLevel)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the delay time for the early reflections.
     * <p>
     * This method sets the time between when the direct path is heard and when
     * the first reflection is heard.
     *
     * @param reflectionsDelay reflections delay in milliseconds. The valid
     *            range is [0, 300].
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setReflectionsDelay(int reflectionsDelay)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the environmental reverb properties. This method is useful when
     * reverb settings have to be applied from a previous backup.
     *
     * @param settings a IEnvironmentalReverb.Settings object containing the
     *            properties to apply
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setProperties(IEnvironmentalReverb.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the echo density in the late reverberation decay.
     * <p>
     * The scale should approximately map linearly to the perceived change in
     * reverberation.
     *
     * @param diffusion diffusion specified using a permille scale. The
     *            diffusion valid range is [0, 1000]. A value of 1000 o/oo
     *            indicates a smooth reverberation decay. Values below this
     *            level give a more <i>grainy</i> character.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setDiffusion(short diffusion)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Controls the modal density of the late reverberation decay.
     * <p>
     * The scale should approximately map linearly to the perceived change in
     * reverberation. A lower density creates a hollow sound that is useful for
     * simulating small reverberation spaces such as bathrooms.
     *
     * @param density density specified using a permille scale. The valid range
     *            is [0, 1000]. A value of 1000 o/oo indicates a natural
     *            sounding reverberation. Values below this level produce a more
     *            colored effect.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setDensity(short density)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the time taken for the level of reverberation to decay by 60 dB.
     *
     * @param decayTime decay time in milliseconds. The valid range is [100,
     *            20000].
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setDecayTime(int decayTime)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the ratio of high frequency decay time (at 5 kHz) relative to the
     * decay time at low frequencies.
     *
     * @param decayHFRatio high frequency decay ratio using a permille scale.
     *            The valid range is [100, 2000]. A ratio of 1000 indicates that
     *            all frequencies decay at the same rate.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setDecayHFRatio(short decayHFRatio)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the master volume level of the environmental reverb effect.
     *
     * @return the room level in millibels.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getRoomLevel()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the room HF level.
     *
     * @return the room HF level in millibels.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getRoomHFLevel()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the reverb level.
     *
     * @return the reverb level in millibels.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getReverbLevel()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the reverb delay.
     *
     * @return the reverb delay in milliseconds.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    int getReverbDelay()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the volume level of the early reflections.
     * @return the early reflections level in millibels.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getReflectionsLevel()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the reflections delay.
     *
     * @return the early reflections delay in milliseconds.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    int getReflectionsDelay()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the environmental reverb properties. This method is useful when a
     * snapshot of current reverb settings must be saved by the application.
     *
     * @return an IEnvironmentalReverb.Settings object containing all current
     *         parameters values
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    IEnvironmentalReverb.Settings getProperties()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets diffusion level.
     *
     * @return the diffusion level. See {@link #setDiffusion(short)} for units.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getDiffusion()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the density level.
     *
     * @return the density level. See {@link #setDiffusion(short)} for units.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getDensity()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the ratio of high frequency decay time (at 5 kHz) relative to low
     * frequencies.
     *
     * @return the decay HF ration. See {@link #setDecayHFRatio(short)} for
     *         units.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getDecayHFRatio()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the decay time.
     *
     * @return the decay time in milliseconds.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    int getDecayTime()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Registers an OnParameterChangeListener interface.
     *
     * @param listener OnParameterChangeListener interface registered
     */
    void setParameterListener(OnParameterChangeListener listener);
}
