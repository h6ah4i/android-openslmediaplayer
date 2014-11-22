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


package com.h6ah4i.android.media.standard.audiofx;

import android.content.Context;
import android.media.audiofx.Visualizer;
import android.os.Build;
import android.util.Log;

import com.h6ah4i.android.media.audiofx.IVisualizer;
import com.h6ah4i.android.media.utils.AudioSystemUtils;

public class StandardVisualizer extends android.media.audiofx.Visualizer implements IVisualizer {
    private static final String TAG = "StandardVisualizer";
    private static final boolean mIsDebuggable = false;

    private static final int MIN_CAPTURE_RATE;
    private static final int MAX_CAPTURE_RATE;
    private OnDataCaptureListenerWrapper mCaptureListenerWrapper;
    private int mCaptureRate = 0;
    private boolean mCaptureWaveform = false;
    private boolean mCaptureFft = false;
    private boolean mNeedToSetCaptureListener = false;
    private int mCaptureSize;
    private boolean mReleased;
    private int mCorrectSamplingRate;

    private static final VisualizerCompatBase COMPAT_HELPER;

    static {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            COMPAT_HELPER = new VisualizerCompatKitKat();
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            COMPAT_HELPER = new VisualizerCompatJB();
        } else {
            COMPAT_HELPER = new VisualizerCompatGB();
        }

        MIN_CAPTURE_RATE = 100;
        MAX_CAPTURE_RATE = Visualizer.getMaxCaptureRate();
    }

    public StandardVisualizer(Context context, int audioSession) {
        super(audioSession);

        getCaptureSizeAndUpdateField();

        // NOTE:
        // Default implementation always tells the sampling rate is 44100000 mHz
        // but actually that is not correct!
        mCorrectSamplingRate = AudioSystemUtils.getProperties(context).outputSampleRate * 1000;
    }

    @Override
    public synchronized boolean getEnabled() {
        return super.getEnabled();
    }

    @Override
    public synchronized int setEnabled(boolean enabled) throws IllegalStateException {
        int result = IVisualizer.ERROR;

        result = onSetEnabled(enabled);

        if (mIsDebuggable) {
            boolean actual = super.getEnabled();
            Log.i(TAG, "setEnabled(" + enabled + ") - actual = " + actual +
                    " (tid= " + Thread.currentThread().getId() + ")");
            if (enabled != actual) {
                Log.w(TAG, "setEnabled() called in illegal state");
            }
            if (result != IVisualizer.SUCCESS) {
                Log.w(TAG, "setEnabled() failed - " + result);
            }
        }

        return result;
    }

    @Override
    public synchronized int getSamplingRate() throws IllegalStateException {
        return mCorrectSamplingRate;
    }

    @Override
    public synchronized int getWaveForm(byte[] waveform) throws IllegalStateException {
        if (waveform == null)
            return Visualizer.ERROR_BAD_VALUE;

        // workaround: Fix mState variable
        super.setEnabled(super.getEnabled());

        getCaptureSizeAndUpdateField();

        if (waveform.length != mCaptureSize) {
            // NOTE: To avoid app crash in native layer...
            return IVisualizer.ERROR_BAD_VALUE;
        }

        return super.getWaveForm(waveform);
    }

    @Override
    public synchronized int getFft(byte[] fft) throws IllegalStateException {
        if (fft == null)
            return Visualizer.ERROR_BAD_VALUE;

        // workaround: Fix mState variable
        super.setEnabled(super.getEnabled());

        getCaptureSizeAndUpdateField();

        if (fft.length != mCaptureSize) {
            // NOTE: To avoid app crash in native layer...
            return IVisualizer.ERROR_BAD_VALUE;
        }

        return super.getFft(fft);
    }

    @Override
    public synchronized int getCaptureSize() throws IllegalStateException {
        return getCaptureSizeAndUpdateField();
    }

    @Override
    public synchronized int setCaptureSize(int size) throws IllegalStateException {
        int result = super.setCaptureSize(size);

        if (result == IVisualizer.SUCCESS) {
            mCaptureSize = size;
        }

        return result;
    }

    @Override
    public synchronized int setDataCaptureListener(
            IVisualizer.OnDataCaptureListener listener, int rate, boolean waveform, boolean fft) {

        // check and correct arguments
        if ((listener != null) && (waveform || fft)) {
            if (!(rate >= MIN_CAPTURE_RATE && rate <= MAX_CAPTURE_RATE))
                return IVisualizer.ERROR_BAD_VALUE;
        } else {
            rate = 0;
            waveform = false;
            fft = false;
        }

        OnDataCaptureListenerWrapper wrapper;
        int result = IVisualizer.ERROR;

        wrapper = mCaptureListenerWrapper;

        if (wrapper == null || (wrapper != null && wrapper.getInternalListener() != listener)) {
            if (listener != null) {
                wrapper = new OnDataCaptureListenerWrapper(listener, mCorrectSamplingRate);
            } else {
                wrapper = null;
            }
        }

        try {
            result = super.setDataCaptureListener(wrapper, rate, waveform, fft);
        } finally {
            if (result == IVisualizer.SUCCESS) {
                mCaptureListenerWrapper = wrapper;
                mCaptureRate = rate;
                mCaptureWaveform = waveform;
                mCaptureFft = fft;
                mNeedToSetCaptureListener = false;
            }
        }

        if (mIsDebuggable) {
            final String strListener = (listener == null)
                    ? "(null)" : listener.getClass().getSimpleName();
            Log.i(TAG,
                    "setDataCaptureListener("
                            + strListener + ", " + rate + ", " + waveform + ", " + fft + ")");
        }
        return result;
    }

    @Override
    public synchronized int[] getCaptureSizeRangeCompat() throws IllegalStateException {
        throwIllegalStateExcepthinIfReleased();
        return android.media.audiofx.Visualizer.getCaptureSizeRange();
    }

    @Override
    public synchronized int getMaxCaptureRateCompat() throws IllegalStateException {
        throwIllegalStateExcepthinIfReleased();
        return android.media.audiofx.Visualizer.getMaxCaptureRate();
    }

    @Override
    public synchronized int getScalingModeCompat() throws IllegalStateException {
        return COMPAT_HELPER.getScalingModeCompat(this);
    }

    @Override
    public synchronized int setScalingModeCompat(int mode) throws IllegalStateException {
        return COMPAT_HELPER.setScalingModeCompat(this, mode);
    }

    @Override
    public synchronized int getMeasurementModeCompat() throws IllegalStateException {
        return COMPAT_HELPER.getMeasurementModeCompat(this);
    }

    @Override
    public synchronized int setMeasurementModeCompat(int mode) throws IllegalStateException {
        return COMPAT_HELPER.setMeasurementModeCompat(this, mode);
    }

    @Override
    public synchronized int getMeasurementPeakRmsCompat(IVisualizer.MeasurementPeakRms measurement) {
        if (measurement == null) {
            return IVisualizer.ERROR_BAD_VALUE;
        }

        return COMPAT_HELPER.getMeasurementPeakRmsCompat(this, measurement);
    }

    private static void throwUseIVisualizerVersionMethod() {
        throw new IllegalStateException(
                "This method is not supported, please use IVisualizer version");
    }

    /*
     * This wrapper class is needed to avoid deadlock bug. See
     * http://code.google.com/p/android/issues/detail?id=37999.
     */
    private static class OnDataCaptureListenerWrapper implements Visualizer.OnDataCaptureListener {
        private IVisualizer.OnDataCaptureListener mListener;
        private final int mCorrectSamplingRate;

        public OnDataCaptureListenerWrapper(IVisualizer.OnDataCaptureListener listener,
                int correctSamplingRate) {
            if (listener == null)
                throw new IllegalArgumentException("listener must be not null");
            mListener = listener;
            mCorrectSamplingRate = correctSamplingRate;
        }

        @Override
        public void onWaveFormDataCapture(Visualizer visualizer, byte[] waveform, int samplingRate) {
            mListener.onWaveFormDataCapture(
                    (StandardVisualizer) visualizer, waveform, mCorrectSamplingRate);
        }

        @Override
        public void onFftDataCapture(Visualizer visualizer, byte[] fft, int samplingRate) {
            mListener.onFftDataCapture(
                    (StandardVisualizer) visualizer, fft, mCorrectSamplingRate);
        }

        public IVisualizer.OnDataCaptureListener getInternalListener() {
            return mListener;
        }

        public void release() {
            mListener = null;
        }
    }

    private int onSetEnabled(boolean enabled) {
        final boolean currentEnabledState = super.getEnabled();

        int result = IVisualizer.ERROR;

        if (enabled && !currentEnabledState) {
            if (mNeedToSetCaptureListener) {
                int result2 = super.setDataCaptureListener(
                        mCaptureListenerWrapper,
                        mCaptureRate, mCaptureWaveform, mCaptureFft);

                if (result2 != IVisualizer.SUCCESS) {
                    throw new IllegalStateException();
                }
            }
        }

        try {
            result = super.setEnabled(enabled);
        } catch (IllegalStateException ex) {
            throw ex;
        } finally {
            if (result == IVisualizer.SUCCESS) {
                if (!enabled) {
                    // need to unregister capture listener to avoid
                    // deadlock!
                    mNeedToSetCaptureListener = true;
                    super.setDataCaptureListener(null, 0, false, false);
                }
            }
        }

        final boolean actual = super.getEnabled();

        if ((result == IVisualizer.SUCCESS) && (enabled != actual)) {
            result = IVisualizer.ERROR;
            Log.w(TAG, "onSetEnabled() - expected : " + enabled + " / actual: " + actual);
        }

        return result;
    }

    @Override
    public int setDataCaptureListener(
            android.media.audiofx.Visualizer.OnDataCaptureListener listener, int rate,
            boolean waveform, boolean fft) {
        throwUseIVisualizerVersionMethod();
        return IVisualizer.ERROR;
    }

    @Override
    public synchronized void release() {
        if (mIsDebuggable) {
            Log.i(TAG, "release()");
        }

        try {
            // Unregister the listener to avoid NullPointerException
            super.setDataCaptureListener(null, 0, false, false);
        } catch (Exception e) {
        }

        if (mCaptureListenerWrapper != null) {
            mCaptureListenerWrapper.release();
            mCaptureListenerWrapper = null;
        }

        mReleased = true;

        super.release();
    }

    private int getCaptureSizeAndUpdateField() {
        int size = super.getCaptureSize();

        if (size > 0 && isPowOfTwo(size)) {
            mCaptureSize = size;
        }

        return size;
    }

    private void throwIllegalStateExcepthinIfReleased() {
        if (mReleased) {
            throw new IllegalStateException();
        }
    }

    private static boolean isPowOfTwo(int x) {
        return (x != 0) && ((x & (x - 1)) == 0);
    }

}
