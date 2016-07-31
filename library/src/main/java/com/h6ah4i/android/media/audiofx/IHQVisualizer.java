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
// /frameworks/base/media/java/android/media/audiofx/Visualizer.java
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

public interface IHQVisualizer extends IReleasable {
    /**
     * Successful operation.
     */
    public static final int SUCCESS = IAudioEffect.SUCCESS;
    /**
     * Unspecified error.
     */
    public static final int ERROR = IAudioEffect.ERROR;
    /**
     * Internal operation status. Not returned by any method.
     */
    public static final int ALREADY_EXISTS = IAudioEffect.ALREADY_EXISTS;
    /**
     * Operation failed due to bad object initialization.
     */
    public static final int ERROR_NO_INIT = IAudioEffect.ERROR_NO_INIT;
    /**
     * Operation failed due to bad parameter value.
     */
    public static final int ERROR_BAD_VALUE = IAudioEffect.ERROR_BAD_VALUE;
    /**
     * Operation failed because it was requested in wrong state.
     */
    public static final int ERROR_INVALID_OPERATION = IAudioEffect.ERROR_INVALID_OPERATION;
    /**
     * Operation failed due to lack of memory.
     */
    public static final int ERROR_NO_MEMORY = IAudioEffect.ERROR_NO_MEMORY;
    /**
     * Operation failed due to dead remote object.
     */
    public static final int ERROR_DEAD_OBJECT = IAudioEffect.ERROR_DEAD_OBJECT;

    /**
     * State of a Visualizer object that was not successfully initialized upon
     * creation
     */
    public static final int STATE_UNINITIALIZED = android.media.audiofx.Visualizer.STATE_UNINITIALIZED;
    /**
     * State of a Visualizer object that is ready to be used.
     */
    public static final int STATE_INITIALIZED = android.media.audiofx.Visualizer.STATE_INITIALIZED;
    /**
     * State of a Visualizer object that is active.
     */
    public static final int STATE_ENABLED = android.media.audiofx.Visualizer.STATE_ENABLED;

    /**
     * Rectangular window.
     */
    public static final int WINDOW_RECTANGULAR = 0;
    /**
     * Hann window.
     */
    public static final int WINDOW_HANN = 1;
    /**
     * Hamming window.
     */
    public static final int WINDOW_HAMMING = 2;
    /**
     * Blackman window.
     */
    public static final int WINDOW_BLACKMAN = 3;
    /**
     * Flat top window.
     */
    public static final int WINDOW_FLAT_TOP = 4;
    /**
     * Option flag to specify whether the window function is also effects to
     * thecaptured waveform data.
     */
    public static final int WINDOW_OPT_APPLY_FOR_WAVEFORM = (1 << 31);

    /**
     * The OnDataCaptureListener interface defines methods called by the
     * Visualizer to periodically update the audio visualization capture. The
     * client application can implement this interface and register the listener
     * with the
     * {@link #setDataCaptureListener(OnDataCaptureListener, int, boolean, boolean)}
     * method.
     */
    interface OnDataCaptureListener {
        /**
         * Method called when a new waveform capture is available.
         * <p>
         * Data in the waveform buffer is valid only within the scope of the
         * callback. Applications which needs access to the waveform data after
         * returning from the callback should make a copy of the data instead of
         * holding a reference.
         *
         * @param visualizer Visualizer object on which the listener is
         *            registered.
         * @param waveform array of bytes containing the waveform
         *            representation.
         * @param numChannels number of channels.
         * @param samplingRate sampling rate of the audio visualized.
         */
        void onWaveFormDataCapture(IHQVisualizer visualizer, float[] waveform, int numChannels,
                int samplingRate);

        /**
         * Method called when a new frequency capture is available.
         * <p>
         * Data in the fft buffer is valid only within the scope of the
         * callback. Applications which needs access to the fft data after
         * returning from the callback should make a copy of the data instead of
         * holding a reference.
         *
         * @param visualizer Visualizer object on which the listener is
         *            registered.
         * @param fft array of bytes containing the frequency representation.
         * @param numChannels number of channels.
         * @param samplingRate sampling rate of the audio visualized.
         */
        void onFftDataCapture(IHQVisualizer visualizer, float[] fft, int numChannels,
                int samplingRate);
    }

    /**
     * Get current activation state of the visualizer.
     *
     * @return true if the visualizer is active, false otherwise
     */
    boolean getEnabled();

    /**
     * Enable or disable the visualization engine.
     *
     * @param enabled requested enable state
     * @return {@link #SUCCESS} in case of success,
     *         {@link #ERROR_INVALID_OPERATION} or {@link #ERROR_DEAD_OBJECT} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int setEnabled(boolean enabled) throws IllegalStateException;

    /**
     * Returns the sampling rate of the captured audio.
     *
     * @return the sampling rate in milliHertz.
     */
    int getSamplingRate() throws IllegalStateException;

    /**
     * Returns the number of channels of the captured audio.
     *
     * @return the number of channels.
     */
    int getNumChannels() throws IllegalStateException;

    /**
     * Returns current capture size.
     *
     * @return the capture size in frames.
     */
    int getCaptureSize() throws IllegalStateException;

    /**
     * Sets the capture size, i.e. the number of frames captured by
     * OnDataCaptureListener event. The capture size must be a power of 2 in the
     * range returned by {@link #getCaptureSizeRange()}. This method must
     * not be called when the Visualizer is enabled.
     *
     * @param size requested capture size
     * @return {@link #SUCCESS} in case of success, {@link #ERROR_BAD_VALUE} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int setCaptureSize(int size) throws IllegalStateException;

    /**
     * Returns current window function type.
     *
     * @return current window function type.
     * @throws IllegalStateException
     */
    int getWindowFunction() throws IllegalStateException;

    /**
     * Sets the window function type.
     *
     * @param windowType window function type. One of the 
     *        {@link #WINDOW_RECTANGULAR = 0}, {@link #WINDOW_HANN}, 
     *        {@link #WINDOW_HAMMING},  {@link #WINDOW_BLACKMAN}, 
     *        {@link #WINDOW_FLAT_TOP} with optional {@link #WINDOW_OPT_APPLY_FOR_WAVEFORM} flag.
     *
     * @return {@link #SUCCESS} in case of success,
     *         {@link #ERROR_INVALID_OPERATION} or {@link #ERROR_DEAD_OBJECT} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int setWindowFunction(int windowType) throws IllegalStateException;

    /**
     * Registers an OnDataCaptureListener interface and specifies the rate at
     * which the capture should be updated as well as the type of capture
     * requested.
     * <p>
     * Call this method with a null listener to stop receiving the capture
     * updates.
     *
     * @param listener OnDataCaptureListener registered
     * @param rate rate in milliHertz at which the capture should be updated
     * @param waveform true if a waveform capture is requested: the
     *            onWaveFormDataCapture() method will be called on the
     *            OnDataCaptureListener interface.
     * @param fft true if a frequency capture is requested: the
     *            onFftDataCapture() method will be called on the
     *            OnDataCaptureListener interface.
     * @return {@link #SUCCESS} in case of success, {@link #ERROR_NO_INIT} or
     *         {@link #ERROR_BAD_VALUE} in case of failure.
     */
    int setDataCaptureListener(
            OnDataCaptureListener listener, int rate, boolean waveform, boolean fft);

    /**
     * Returns the capture size range.
     *
     * @return the minimum capture size is returned in first array element and
     *         the maximum in second array element.
     */
    int[] getCaptureSizeRange() throws IllegalStateException;

    /**
     * Returns the maximum capture rate for the callback capture method. This is
     * the maximum value for the rate parameter of the
     * {@link #setDataCaptureListener(OnDataCaptureListener, int, boolean, boolean)}
     * method.
     *
     * @return the maximum capture rate expressed in milliHertz
     */
    int getMaxCaptureRate() throws IllegalStateException;
}
