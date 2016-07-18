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

import android.media.audiofx.Visualizer;

import com.h6ah4i.android.media.IReleasable;

public interface IVisualizer extends IReleasable {
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
         * @param samplingRate sampling rate of the audio visualized.
         */
        void onWaveFormDataCapture(IVisualizer visualizer, byte[] waveform, int samplingRate);

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
         * @param samplingRate sampling rate of the audio visualized.
         */
        void onFftDataCapture(IVisualizer visualizer, byte[] fft, int samplingRate);
    }

    /* since API level 16 */
    /**
     * Defines a capture mode where amplification is applied based on the
     * content of the captured data. This is the default Visualizer mode, and is
     * suitable for music visualization.
     */
    public static final int SCALING_MODE_NORMALIZED = 0;

    /**
     * Defines a capture mode where the playback volume will affect (scale) the
     * range of the captured data. A low playback volume will lead to low sample
     * and fft values, and vice-versa.
     */
    public static final int SCALING_MODE_AS_PLAYED = 1;

    /* since API level 19 */
    /**
     * Defines a measurement mode in which no measurements are performed.
     */
    public static final int MEASUREMENT_MODE_NONE = 0;

    /**
     * Defines a measurement mode which computes the peak and RMS value in mB,
     * where 0mB is the maximum sample value, and -9600mB is the minimum value.
     * Values for peak and RMS can be retrieved with
     * {@link #getMeasurementPeakRms(MeasurementPeakRms)}.
     */
    public static final int MEASUREMENT_MODE_PEAK_RMS = 1;

    /**
     * A class to store peak and RMS values. Peak and RMS are expressed in mB,
     * as described in the {@link Visualizer#MEASUREMENT_MODE_PEAK_RMS}
     * measurement mode.
     */
    static class MeasurementPeakRms {
        /** The peak value in mB. */
        public int mPeak;

        /** The RMS value in mB. */
        public int mRms;
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
     * Returns a frequency capture of currently playing audio content.
     * <p>
     * This method must be called when the Visualizer is enabled.
     * <p>
     * The capture is an 8-bit magnitude FFT, the frequency range covered being
     * 0 (DC) to half of the sampling rate returned by
     * {@link #getSamplingRate()}. The capture returns the real and imaginary
     * parts of a number of frequency points equal to half of the capture size
     * plus one.
     * <p>
     * Note: only the real part is returned for the first point (DC) and the
     * last point (sampling frequency / 2).
     * <p>
     * The layout in the returned byte array is as follows:
     * <ul>
     * <li>n is the capture size returned by getCaptureSize()</li>
     * <li>Rfk, Ifk are respectively the real and imaginary parts of the kth
     * frequency component</li>
     * <li>If Fs is the sampling frequency retuned by getSamplingRate() the kth
     * frequency is: (k*Fs)/(n/2)</li>
     * </ul>
     * <table border="0" cellspacing="0" cellpadding="0" summary="FFT data order">
     * <tr>
     * <td>Index</td>
     * <td>0</td>
     * <td>1</td>
     * <td>2</td>
     * <td>3</td>
     * <td>4</td>
     * <td>5</td>
     * <td>...</td>
     * <td>n - 2</td>
     * <td>n - 1</td>
     * </tr>
     * <tr>
     * <td>Data</td>
     * <td>Rf0</td>
     * <td>Rf(n/2)</td>
     * <td>Rf1</td>
     * <td>If1</td>
     * <td>Rf2</td>
     * <td>If2</td>
     * <td>...</td>
     * <td>Rf(n-1)/2</td>
     * <td>If(n-1)/2</td>
     * </tr>
     * </table>
     *
     * @param fft array of bytes where the FFT should be returned
     * @return {@link #SUCCESS} in case of success, {@link #ERROR_NO_MEMORY},
     *         {@link #ERROR_INVALID_OPERATION} or {@link #ERROR_DEAD_OBJECT} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int getFft(byte[] fft) throws IllegalStateException;

    /**
     * Returns a waveform capture of currently playing audio content. The
     * capture consists in a number of consecutive 8-bit (unsigned) mono PCM
     * samples equal to the capture size returned by {@link #getCaptureSize()}.
     * <p>
     * This method must be called when the Visualizer is enabled.
     *
     * @param waveform array of bytes where the waveform should be returned
     * @return {@link #SUCCESS} in case of success, {@link #ERROR_NO_MEMORY},
     *         {@link #ERROR_INVALID_OPERATION} or {@link #ERROR_DEAD_OBJECT} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int getWaveForm(byte[] waveform) throws IllegalStateException;

    /**
     * Returns current capture size.
     *
     * @return the capture size in bytes.
     */
    int getCaptureSize() throws IllegalStateException;

    /**
     * Sets the capture size, i.e. the number of bytes returned by
     * {@link #getWaveForm(byte[])} and {@link #getFft(byte[])} methods. The
     * capture size must be a power of 2 in the range returned by
     * {@link #getCaptureSizeRange()}. This method must not be called when
     * the Visualizer is enabled.
     *
     * @param size requested capture size
     * @return {@link #SUCCESS} in case of success, {@link #ERROR_BAD_VALUE} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int setCaptureSize(int size) throws IllegalStateException;

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

    /**
     * Returns the current scaling mode on the captured visualization data.
     *
     * @return the scaling mode, see {@link #SCALING_MODE_NORMALIZED} and
     *         {@link #SCALING_MODE_AS_PLAYED}.
     * @throws IllegalStateException
     */
    int getScalingMode() throws IllegalStateException;

    /**
     * Set the type of scaling applied on the captured visualization data.
     *
     * @param mode see {@link #SCALING_MODE_NORMALIZED} and
     *            {@link #SCALING_MODE_AS_PLAYED}
     * @return {@link #SUCCESS} in case of success, {@link #ERROR_BAD_VALUE} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int setScalingMode(int mode) throws IllegalStateException;

    /**
     * Returns the current measurement modes performed by this audio effect
     *
     * @return the mask of the measurements, {@link #MEASUREMENT_MODE_NONE}
     *         (when no measurements are performed) or
     *         {@link #MEASUREMENT_MODE_PEAK_RMS}.
     * @throws IllegalStateException
     */
    int getMeasurementMode() throws IllegalStateException;

    /**
     * Sets the combination of measurement modes to be performed by this audio
     * effect.
     *
     * @param mode a mask of the measurements to perform. The valid values are
     *            {@link #MEASUREMENT_MODE_NONE} (to cancel any measurement) or
     *            {@link #MEASUREMENT_MODE_PEAK_RMS}.
     * @return {@link #SUCCESS} in case of success, {@link #ERROR_BAD_VALUE} in
     *         case of failure.
     * @throws IllegalStateException
     */
    int setMeasurementMode(int mode) throws IllegalStateException;

    /**
     * Retrieves the latest peak and RMS measurement. Sets the peak and RMS
     * fields of the supplied {@link MeasurementPeakRms} to the
     * latest measured values.
     *
     * @param measurement a non-null {@link MeasurementPeakRms}
     *            instance to store the measurement values.
     * @return {@link #SUCCESS} in case of success, {@link #ERROR_BAD_VALUE},
     *         {@link #ERROR_NO_MEMORY}, {@link #ERROR_INVALID_OPERATION} or
     *         {@link #ERROR_DEAD_OBJECT} in case of failure.
     */
    int getMeasurementPeakRms(MeasurementPeakRms measurement);
}
