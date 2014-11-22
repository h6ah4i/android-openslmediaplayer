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
// /frameworks/base/media/java/android/media/audiofx/AudioEffect.java
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

import com.h6ah4i.android.media.IReleasable;

public interface IAudioEffect extends IReleasable {
    /**
     * Successful operation.
     */
    public static final int SUCCESS = android.media.audiofx.AudioEffect.SUCCESS;
    /**
     * Unspecified error.
     */
    public static final int ERROR = android.media.audiofx.AudioEffect.ERROR;
    /**
     * Internal operation status. Not returned by any method.
     */
    public static final int ALREADY_EXISTS = android.media.audiofx.AudioEffect.ALREADY_EXISTS;
    /**
     * Operation failed due to bad object initialization.
     */
    public static final int ERROR_NO_INIT = android.media.audiofx.AudioEffect.ERROR_NO_INIT;
    /**
     * Operation failed due to bad parameter value.
     */
    public static final int ERROR_BAD_VALUE = android.media.audiofx.AudioEffect.ERROR_BAD_VALUE;
    /**
     * Operation failed because it was requested in wrong state.
     */
    public static final int ERROR_INVALID_OPERATION = android.media.audiofx.AudioEffect.ERROR_INVALID_OPERATION;
    /**
     * Operation failed due to lack of memory.
     */
    public static final int ERROR_NO_MEMORY = android.media.audiofx.AudioEffect.ERROR_NO_MEMORY;
    /**
     * Operation failed due to dead remote object.
     */
    public static final int ERROR_DEAD_OBJECT = android.media.audiofx.AudioEffect.ERROR_DEAD_OBJECT;

    /**
     * The OnControlStatusChangeListener interface defines a method called by
     * the AudioEffect when a the control of the effect engine is gained or lost
     * by the application
     */
    public interface OnControlStatusChangeListener {
        /**
         * Called on the listener to notify it that the effect engine control
         * has been taken or returned.
         *
         * @param effect the effect on which the interface is registered.
         * @param controlGranted true if the application has been granted
         *            control of the effect engine, false otherwise.
         */
        void onControlStatusChange(IAudioEffect effect, boolean controlGranted);
    }

    /**
     * The OnEnableStatusChangeListener interface defines a method called by the
     * AudioEffect when a the enabled state of the effect engine was changed by
     * the controlling application.
     */
    public interface OnEnableStatusChangeListener {
        /**
         * Called on the listener to notify it that the effect engine has been
         * enabled or disabled.
         *
         * @param effect the effect on which the interface is registered.
         * @param enabled new effect state.
         */
        void onEnableStatusChange(IAudioEffect effect, boolean enabled);
    }

    // NOTE: getDescriptor() is not supported.
    // AudioEffect.Descriptor getDescriptor() throws IllegalStateException;

    /**
     * Enable or disable the effect. Creating an audio effect does not
     * automatically apply this effect on the audio source. It creates the
     * resources necessary to process this effect but the audio signal is still
     * bypassed through the effect engine. Calling this method will make that
     * the effect is actually applied or not to the audio content being played
     * in the corresponding audio session.
     *
     * @param enabled the requested enable state
     * @return {@link #SUCCESS} in case of success,
     *         {@link #ERROR_INVALID_OPERATION} or {@link #ERROR_DEAD_OBJECT} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int setEnabled(boolean enabled) throws IllegalStateException;

    /**
     * Returns effect enabled state
     *
     * @return true if the effect is enabled, false otherwise.
     * @throws IllegalStateException
     */
    boolean getEnabled() throws IllegalStateException;

    /**
     * Returns effect unique identifier. This system wide unique identifier can
     * be used to attach this effect to a MediaPlayer or an AudioTrack when the
     * effect is an auxiliary effect (Reverb)
     *
     * @return the effect identifier.
     * @throws IllegalStateException
     */
    int getId() throws IllegalStateException;

    /**
     * Checks if this AudioEffect object is controlling the effect engine.
     *
     * @return true if this instance has control of effect engine, false
     *         otherwise.
     * @throws IllegalStateException
     */
    boolean hasControl() throws IllegalStateException;

    // NOTE: queryEffects() is not supported.
    // static Descriptor[] queryEffects() throws IllegalStateException;

    /**
     * Releases the native AudioEffect resources. It is a good practice to
     * release the effect engine when not in use as control can be returned to
     * other applications or the native resources released.
     */
    @Override
    void release() throws IllegalStateException;

    /**
     * Sets the listener AudioEffect notifies when the effect engine control is
     * taken or returned.
     *
     * @param listener
     */
    void setControlStatusListener(OnControlStatusChangeListener listener)
            throws IllegalStateException;

    /**
     * Sets the listener AudioEffect notifies when the effect engine is enabled
     * or disabled.
     *
     * @param listener
     */
    void setEnableStatusListener(OnEnableStatusChangeListener listener)
            throws IllegalStateException;
}
