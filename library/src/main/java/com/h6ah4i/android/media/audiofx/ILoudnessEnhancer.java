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
// /frameworks/base/media/java/android/media/audiofx/LoudnessEnhancer.java
/*
 * Copyright (C) 2013 The Android Open Source Project
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

public interface ILoudnessEnhancer extends IAudioEffect {

    // NOTE: Android's LoudnessEnhancer class does not provides Settings class
    // so this Setting class is originally copied from
    // android.media.audiofx.Virtualizer.Settings.
    /* =============================================================== */
    /*
     * Copyright (C) 2010 The Android Open Source Project Licensed under the
     * Apache License, Version 2.0 (the "License"); you may not use this file
     * except in compliance with the License. You may obtain a copy of the
     * License at http://www.apache.org/licenses/LICENSE-2.0 Unless required by
     * applicable law or agreed to in writing, software distributed under the
     * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
     * CONDITIONS OF ANY KIND, either express or implied. See the License for
     * the specific language governing permissions and limitations under the
     * License.
     */

    /**
     * The Settings class regroups the LoudnessEnhancer parameters. It is used
     * in conjunction with the getProperties() and setProperties()
     * methods to backup and restore all parameters in a single call.
     */
    public static class Settings {
        public int targetGainmB;

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
            if (!key.equals("LoudnessEnhancer")) {
                throw new IllegalArgumentException(
                        "invalid settings for LoudnessEnhancer: " + key);
            }
            try {
                key = st.nextToken();
                if (!key.equals("targetGainmB")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                targetGainmB = Integer.parseInt(st.nextToken());
            } catch (NumberFormatException nfe) {
                throw new IllegalArgumentException("invalid value for key: " + key);
            }
        }

        @Override
        public String toString() {
            String str = new String(
                    "LoudnessEnhancer" +
                            ";targetGainmB=" + Integer.toString(targetGainmB)
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

    /* =============================================================== */

    /**
     * The maximum gain applied applied to the signal to process. It is
     * expressed in millibels (100mB = 1dB) where 0mB corresponds to no
     * amplification.
     */
    public static final int PARAM_TARGET_GAIN_MB = 0;

    // Why Android's implementation uses float for this method...?
    /**
     * Return the target gain.
     *
     * @return the effect target gain expressed in mB.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    float getTargetGain() throws UnsupportedOperationException, IllegalStateException;

    /**
     * Return the target gain.
     *
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setTargetGain(int gainmB) throws IllegalStateException, IllegalArgumentException;

    // NOTE: Android's LoudnessEnhancer class does not provides this method
    /**
     * Gets the ludness enhancer properties. This method is useful when a
     * snapshot of current ludness enhancer settings must be saved by the
     * application.
     *
     * @return a ILoudnessEnhancer.Settings object containing all current
     *         parameters values
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    ILoudnessEnhancer.Settings getProperties()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the ludness enhancer properties. This method is useful when ludness
     * enhancer settings have to be applied from a previous backup.
     *
     * @param settings a ILoudnessEnhancer.Settings object containing the
     *            properties to apply
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setProperties(ILoudnessEnhancer.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

}
