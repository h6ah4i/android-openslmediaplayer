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
// /frameworks/base/media/java/android/media/audiofx/Virtualizer.java
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

public interface IVirtualizer extends IAudioEffect {

    /**
     * The Settings class regroups all virtualizer parameters. It is used in
     * conjunction with getProperties() and setProperties() methods
     * to backup and restore all parameters in a single call.
     */
    public static class Settings implements Cloneable {
        public short strength;

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
            if (!key.equals("Virtualizer")) {
                throw new IllegalArgumentException(
                        "invalid settings for Virtualizer: " + key);
            }
            try {
                key = st.nextToken();
                if (!key.equals("strength")) {
                    throw new IllegalArgumentException("invalid key name: " + key);
                }
                strength = Short.parseShort(st.nextToken());
            } catch (NumberFormatException nfe) {
                throw new IllegalArgumentException("invalid value for key: " + key);
            }
        }

        @Override
        public String toString() {
            String str = new String(
                    "Virtualizer" +
                            ";strength=" + Short.toString(strength)
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
     * Is strength parameter supported by virtualizer engine. Parameter ID for
     * getParameter().
     */
    public static final int PARAM_STRENGTH = android.media.audiofx.Virtualizer.PARAM_STRENGTH;
    /**
     * Virtualizer effect strength. Parameter ID for
     * {@link com.h6ah4i.android.media.audiofx.IVirtualizer.OnParameterChangeListener}
     */
    public static final int PARAM_STRENGTH_SUPPORTED = android.media.audiofx.Virtualizer.PARAM_STRENGTH_SUPPORTED;

    /**
     * The OnParameterChangeListener interface defines a method called by the
     * Virtualizer when a parameter value has changed.
     */
    public interface OnParameterChangeListener {
        /**
         * Method called when a parameter value has changed. The method is
         * called only if the parameter was changed by another application
         * having the control of the same Virtualizer engine.
         *
         * @param effect the Virtualizer on which the interface is registered.
         * @param status status of the set parameter operation.
         * @param param ID of the modified parameter. See
         *            {@link #PARAM_STRENGTH} ...
         * @param value the new parameter value.
         */
        void onParameterChange(IVirtualizer effect, int status, int param, short value);
    }

    /**
     * Indicates whether setting strength is supported. If this method returns
     * false, only one strength is supported and the setStrength() method always
     * rounds to that value.
     *
     * @return true is strength parameter is supported, false otherwise
     */
    boolean getStrengthSupported();

    /**
     * Sets the strength of the virtualizer effect. If the implementation does
     * not support per mille accuracy for setting the strength, it is allowed to
     * round the given strength to the nearest supported value. You can use the
     * {@link #getRoundedStrength()} method to query the (possibly rounded)
     * value that was actually set.
     *
     * @param strength strength of the effect. The valid range for strength
     *            strength is [0, 1000], where 0 per mille designates the
     *            mildest effect and 1000 per mille designates the strongest.
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setStrength(short strength)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the current strength of the effect.
     *
     * @return the strength of the effect. The valid range for strength is [0,
     *         1000], where 0 per mille designates the mildest effect and 1000
     *         per mille the strongest
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    short getRoundedStrength()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Gets the virtualizer properties. This method is useful when a snapshot of
     * current virtualizer settings must be saved by the application.
     *
     * @return a IVirtualizer.Settings object containing all current parameters
     *         values
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    IVirtualizer.Settings getProperties()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Sets the virtualizer properties. This method is useful when virtualizer
     * settings have to be applied from a previous backup.
     *
     * @param settings a IVirtualizer.Settings object containing the properties
     *            to apply
     * @throws IllegalStateException
     * @throws IllegalArgumentException
     * @throws UnsupportedOperationException
     */
    void setProperties(IVirtualizer.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Registers an OnParameterChangeListener interface.
     *
     * @param listener OnParameterChangeListener interface registered
     */
    void setParameterListener(OnParameterChangeListener listener);
}
