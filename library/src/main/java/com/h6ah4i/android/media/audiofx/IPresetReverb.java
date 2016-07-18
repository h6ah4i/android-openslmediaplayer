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
// /frameworks/base/media/java/android/media/audiofx/PresetReverb.java
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

public interface IPresetReverb extends IAudioEffect {
    /**
     * The Settings class regroups all preset reverb parameters. It is used in
     * conjunction with getProperties() and setProperties() methods
     * to backup and restore all parameters in a single call.
     */
    public static class Settings implements Cloneable {
        public short preset;

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
            if (st.countTokens() != 3) {
                throw new IllegalArgumentException("settings: " + settings);
            }
            String key = st.nextToken();
            if (!key.equals("PresetReverb")) {
                throw new IllegalArgumentException(
                        "invalid settings for PresetReverb: " + key);
            }
            try {
                key = st.nextToken();
                if (!key.equals("preset")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                preset = Short.parseShort(st.nextToken());
            } catch (NumberFormatException nfe) {
                throw new IllegalArgumentException("invalid value for key: " + key);
            }
        }

        @Override
        public String toString() {
            String str = new String(
                    "PresetReverb" +
                            ";preset=" + Short.toString(preset)
                    );
            return str;
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
     * Preset. Parameter ID for
     * {@link android.media.audiofx.PresetReverb.OnParameterChangeListener}
     */
    public static final int PARAM_PRESET = android.media.audiofx.PresetReverb.PARAM_PRESET;

    /**
     * No reverb or reflections
     */
    public static final short PRESET_NONE = android.media.audiofx.PresetReverb.PRESET_NONE;
    /**
     * Reverb preset representing a small room less than five meters in length
     */
    public static final short PRESET_SMALLROOM = android.media.audiofx.PresetReverb.PRESET_SMALLROOM;
    /**
     * Reverb preset representing a medium room with a length of ten meters or
     * less
     */
    public static final short PRESET_MEDIUMROOM = android.media.audiofx.PresetReverb.PRESET_MEDIUMROOM;
    /**
     * Reverb preset representing a large-sized room suitable for live
     * performances
     */
    public static final short PRESET_LARGEROOM = android.media.audiofx.PresetReverb.PRESET_LARGEROOM;
    /**
     * Reverb preset representing a medium-sized hall
     */
    public static final short PRESET_MEDIUMHALL = android.media.audiofx.PresetReverb.PRESET_MEDIUMHALL;
    /**
     * Reverb preset representing a large-sized hall suitable for a full
     * orchestra
     */
    public static final short PRESET_LARGEHALL = android.media.audiofx.PresetReverb.PRESET_LARGEHALL;
    /**
     * Reverb preset representing a synthesis of the traditional plate reverb
     */
    public static final short PRESET_PLATE = android.media.audiofx.PresetReverb.PRESET_PLATE;

    /**
     * The OnParameterChangeListener interface defines a method called by the
     * PresetReverb when a parameter value has changed.
     */
    public interface OnParameterChangeListener {
        /**
         * Method called when a parameter value has changed. The method is
         * called only if the parameter was changed by another application
         * having the control of the same PresetReverb engine.
         *
         * @param effect the PresetReverb on which the interface is registered.
         * @param status status of the set parameter operation.
         * @param param ID of the modified parameter. See {@link #PARAM_PRESET}
         *            ...
         * @param value the new parameter value.
         */
        void onParameterChange(IPresetReverb effect, int status, int param, short value);
    }

    /**
     * Gets current reverb preset.
     *
     * @return the preset that is set at the moment.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getPreset()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the preset reverb properties. This method is useful when a snapshot
     * of current preset reverb settings must be saved by the application.
     *
     * @return a IPresetReverb.Settings object containing all current parameters
     *         values
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    IPresetReverb.Settings getProperties()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Enables a preset on the reverb.
     * <p>
     * The reverb PRESET_NONE disables any reverb from the current output but
     * does not free the resources associated with the reverb. For an
     * application to signal to the implementation to free the resources, it
     * must call the release() method.
     *
     * @param preset this must be one of the the preset constants defined in
     *            this class. e.g. {@link #PRESET_SMALLROOM}
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setPreset(short preset)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the preset reverb properties. This method is useful when preset
     * reverb settings have to be applied from a previous backup.
     *
     * @param settings a IPresetReverb.Settings object containing the properties
     *            to apply
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setProperties(IPresetReverb.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Registers an OnParameterChangeListener interface.
     *
     * @param listener OnParameterChangeListener interface registered
     */
    void setParameterListener(OnParameterChangeListener listener);
}
