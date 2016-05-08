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

package com.h6ah4i.android.media.audiofx;

import java.util.StringTokenizer;

public interface IPreAmp extends IAudioEffect {

    // NOTE: this Setting class is originally copied from
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
     * in conjunction with the getProperties() and setProperties() methods to
     * backup and restore all parameters in a single call.
     */
    public static class Settings implements Cloneable {
        public float level;

        public Settings() {
        }

        public Settings(String settings) {
            StringTokenizer st = new StringTokenizer(settings, "=;");
            if (st.countTokens() != 3) {
                throw new IllegalArgumentException("settings: " + settings);
            }
            String key = st.nextToken();
            if (!key.equals("PreAmp")) {
                throw new IllegalArgumentException(
                        "invalid settings for PreAmp: " + key);
            }
            try {
                key = st.nextToken();
                if (!key.equals("level")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                level = Float.parseFloat(st.nextToken());
            } catch (NumberFormatException nfe) {
                throw new IllegalArgumentException("invalid value for key: " + key);
            }
        }

        @Override
        public String toString() {
            String str = new String(
                    "PreAmp" +
                            ";level=" + Float.toString(level)
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
     * Gets current amplitude level value.
     *
     * @return current amplitude level value.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    float getLevel() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException;

    /**
     * Sets amplitude level value.
     *
     * @param level new gain in linear.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setLevel(float level) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException;

    /**
     * Gets the preamp properties. This method is useful when a snapshot of
     * current preamp settings must be saved by the application.
     *
     * @return a IPreAmp.Settings object containing all current parameters
     *         values
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    Settings getProperties() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException;

    /**
     * Sets the preamp properties. This method is useful when preamp settings
     * have to be applied from a previous backup.
     *
     * @param settings a IPreAmp.Settings object containing the properties to
     *            apply
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setProperties(Settings settings) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException;;
}
