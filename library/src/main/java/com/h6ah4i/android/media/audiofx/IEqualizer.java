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
// /frameworks/base/media/java/android/media/audiofx/Equalizer.java
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

public interface IEqualizer extends IAudioEffect {

    /**
     * The Settings class regroups all equalizer parameters. It is used in
     * conjunction with getProperties() and setProperties() methods
     * to backup and restore all parameters in a single call.
     */
    public static class Settings implements Cloneable {
        public short curPreset;
        public short numBands = 0;
        public short[] bandLevels = null;

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
            if (st.countTokens() < 5) {
                throw new IllegalArgumentException("settings: " + settings);
            }
            String key = st.nextToken();
            if (!key.equals("Equalizer")) {
                throw new IllegalArgumentException(
                        "invalid settings for Equalizer: " + key);
            }
            try {
                key = st.nextToken();
                if (!key.equals("curPreset")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                curPreset = Short.parseShort(st.nextToken());
                key = st.nextToken();
                if (!key.equals("numBands")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                numBands = Short.parseShort(st.nextToken());
                if (st.countTokens() != numBands * 2) {
                    throw new IllegalArgumentException("settings: " + settings);
                }
                bandLevels = new short[numBands];
                for (int i = 0; i < numBands; i++) {
                    key = st.nextToken();
                    if (!key.equals("band" + (i + 1) + "Level")) {
                        throw new IllegalArgumentException("invalid key name: " + key);
                    }
                    bandLevels[i] = Short.parseShort(st.nextToken());
                }
            } catch (NumberFormatException nfe) {
                throw new IllegalArgumentException("invalid value for key: " + key);
            }
        }

        @Override
        public String toString() {

            String str = new String(
                    "Equalizer" +
                            ";curPreset=" + Short.toString(curPreset) +
                            ";numBands=" + Short.toString(numBands)
                    );
            for (int i = 0; i < numBands; i++) {
                str = str.concat(";band" + (i + 1) + "Level=" + Short.toString(bandLevels[i]));
            }
            return str;
        }

        @Override
        public Settings clone() {
            // deep copy
            Settings clone = new Settings();

            clone.curPreset = curPreset;
            clone.numBands = numBands;
            clone.bandLevels = (bandLevels != null) ? (bandLevels.clone()) : null;

            return clone;
        }
    }

    /**
     * Number of bands. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_NUM_BANDS = android.media.audiofx.Equalizer.PARAM_NUM_BANDS;
    /**
     * Band level range. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_LEVEL_RANGE = android.media.audiofx.Equalizer.PARAM_NUM_BANDS;
    /**
     * Band level. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_BAND_LEVEL = android.media.audiofx.Equalizer.PARAM_BAND_LEVEL;
    /**
     * Band center frequency. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_CENTER_FREQ = android.media.audiofx.Equalizer.PARAM_CENTER_FREQ;
    /**
     * Band frequency range. Parameter ID for
     * {@link android.media.audiofx.Equalizer.OnParameterChangeListener}
     */
    public static final int PARAM_BAND_FREQ_RANGE = android.media.audiofx.Equalizer.PARAM_BAND_FREQ_RANGE;
    /**
     * Band for a given frequency. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_GET_BAND = android.media.audiofx.Equalizer.PARAM_GET_BAND;
    /**
     * Current preset. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_CURRENT_PRESET = android.media.audiofx.Equalizer.PARAM_CURRENT_PRESET;
    /**
     * Request number of presets. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_GET_NUM_OF_PRESETS = android.media.audiofx.Equalizer.PARAM_GET_NUM_OF_PRESETS;
    /**
     * Request preset name. Parameter ID for OnParameterChangeListener
     */
    public static final int PARAM_GET_PRESET_NAME = android.media.audiofx.Equalizer.PARAM_GET_PRESET_NAME;
    /**
     * Maximum size for preset name
     */
    public static final int PARAM_STRING_SIZE_MAX = android.media.audiofx.Equalizer.PARAM_STRING_SIZE_MAX;

    public static final short PRESET_UNDEFINED = (short) 0xffff;

    /**
     * The OnParameterChangeListener interface defines a method called by the
     * Equalizer when a parameter value has changed.
     */
    public interface OnParameterChangeListener {
        /**
         * Method called when a parameter value has changed. The method is
         * called only if the parameter was changed by another application
         * having the control of the same Equalizer engine.
         *
         * @param effect the Equalizer on which the interface is registered.
         * @param status status of the set parameter operation.
         * @param param1 ID of the modified parameter. See
         *            {@link #PARAM_BAND_LEVEL} ...
         * @param param2 additional parameter qualifier (e.g the band for band
         *            level parameter).
         * @param value the new parameter value.
         */
        void onParameterChange(IEqualizer effect, int status, int param1, int param2, int value);
    }

    /**
     * Gets the band that has the most effect on the given frequency.
     *
     * @param frequency frequency in milliHertz which is to be equalized via the
     *            returned band.
     * @return the frequency band that has most effect on the given frequency.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getBand(int frequency)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    int[] getBandFreqRange(short band)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the gain set for the given equalizer band.
     *
     * @param band frequency band whose gain is requested. The numbering of the
     *            bands starts from 0 and ends at (number of bands - 1).
     * @return the gain in millibels of the given band.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getBandLevel(short band)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the level range for use by {@link #setBandLevel(short,short)}. The
     * level is expressed in milliBel.
     *
     * @return the band level range in an array of short integers. The first
     *         element is the lower limit of the range, the second element the
     *         upper limit.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short[] getBandLevelRange()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the center frequency of the given band.
     *
     * @param band frequency band whose center frequency is requested. The
     *            numbering of the bands starts from 0 and ends at (number of
     *            bands - 1).
     * @return the center frequency in milliHertz
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    int getCenterFreq(short band)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets current preset.
     *
     * @return the preset that is set at the moment.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getCurrentPreset()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the number of frequency bands supported by the Equalizer engine.
     *
     * @return the number of bands
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getNumberOfBands()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the total number of presets the equalizer supports. The presets will
     * have indices [0, number of presets-1].
     *
     * @return the number of presets the equalizer supports.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getNumberOfPresets()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the preset name based on the index.
     *
     * @param preset index of the preset. The valid range is [0, number of
     *            presets-1].
     * @return a string containing the name of the given preset.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    String getPresetName(short preset)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the given equalizer band to the given gain value.
     *
     * @param band frequency band that will have the new gain. The numbering of
     *            the bands starts from 0 and ends at (number of bands - 1).
     * @param level new gain in millibels that will be set to the given band.
     *            getBandLevelRange() will define the maximum and minimum
     *            values.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     * @see #getNumberOfBands()
     */
    void setBandLevel(short band, short level)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the equalizer according to the given preset.
     *
     * @param preset new preset that will be taken into use. The valid range is
     *            [0, number of presets-1].
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     * @see #getNumberOfPresets()
     */
    void usePreset(short preset)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the equalizer properties. This method is useful when a snapshot of
     * current equalizer settings must be saved by the application.
     *
     * @return an IEqualizer.Settings object containing all current parameters
     *         values
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    IEqualizer.Settings getProperties()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the equalizer properties. This method is useful when equalizer
     * settings have to be applied from a previous backup.
     *
     * @param settings an IEqualizer.Settings object containing the properties
     *            to apply
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setProperties(IEqualizer.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Registers an OnParameterChangeListener interface.
     *
     * @param listener OnParameterChangeListener interface registered
     */
    void setParameterListener(OnParameterChangeListener listener);
}
