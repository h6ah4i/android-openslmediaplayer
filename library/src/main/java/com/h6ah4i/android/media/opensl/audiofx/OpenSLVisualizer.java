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


package com.h6ah4i.android.media.opensl.audiofx;

import java.lang.ref.WeakReference;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.h6ah4i.android.media.audiofx.IVisualizer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerContext;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerNativeLibraryLoader;

public class OpenSLVisualizer implements IVisualizer {
    private static final String TAG = "Visualizer";

    private long mNativeHandle;
    private static final boolean HAS_NATIVE;
    private int[] mParamIntBuff = new int[2];
    private boolean[] mParamBoolBuff = new boolean[1];

    private volatile InternalHandler mHandler;
    private volatile OnDataCaptureListener mOnDataCaptureListener;

    static {
        // load native library
        HAS_NATIVE = OpenSLMediaPlayerNativeLibraryLoader.loadLibraries();
    }

    public static int[] sGetCaptureSizeRange() {
        int[] range = new int[2];

        int result = getCaptureSizeRangeImplNative(range);

        throwIllegalStateExceptionIfNeeded(result);

        return range;
    }

    public static int sGetMaxCaptureRate() {
        int[] rate = new int[1];

        int result = getMaxCaptureRateImplNative(rate);

        throwIllegalStateExceptionIfNeeded(result);

        return rate[0];
    }

    public OpenSLVisualizer(OpenSLMediaPlayerContext context) {
        if (context == null)
            throw new IllegalArgumentException("The argument 'context' cannot be null");

        if (HAS_NATIVE) {
            mNativeHandle = createNativeImplHandle(
                    OpenSLMediaPlayer.Internal.getNativeHandle(context),
                    new WeakReference<OpenSLVisualizer>(this));
        }

        if (mNativeHandle == 0) {
            throw new UnsupportedOperationException("Failed to initialize native layer");
        }

        mHandler = new InternalHandler(this);
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    @Override
    public void release() {
        mOnDataCaptureListener = null;

        try {
            if (HAS_NATIVE && mNativeHandle != 0) {
                deleteNativeImplHandle(mNativeHandle);
                mNativeHandle = 0;
            }
        } catch (Exception e) {
            Log.e(TAG, "release()", e);
        }

        if (mHandler != null) {
            mHandler.release();
            mHandler = null;
        }
    }

    @Override
    public boolean getEnabled() {
        checkNativeImplIsAvailable();

        final boolean[] enabled = mParamBoolBuff;
        int result = getEnabledImplNative(mNativeHandle, enabled);

        throwIllegalStateExceptionIfNeeded(result);

        return enabled[0];
    }

    @Override
    public int setEnabled(boolean enabled) throws IllegalStateException {
        checkNativeImplIsAvailable();

        int result = setEnabledImplNative(mNativeHandle, enabled);

        throwIllegalStateExceptionIfNeeded(result);

        return translateErrorCode(result);
    }

    @Override
    public int getSamplingRate() throws IllegalStateException {
        checkNativeImplIsAvailable();

        final int[] samplingRate = mParamIntBuff;

        int result = getSamplingRateImplNative(mNativeHandle, samplingRate);

        throwIllegalStateExceptionIfNeeded(result);

        return samplingRate[0];
    }

    @Override
    public int getFft(byte[] fft) throws IllegalStateException {
        checkNativeImplIsAvailable();

        int result = getFftImplNative(mNativeHandle, fft);

        if (result == OpenSLMediaPlayer.Internal.RESULT_ILLEGAL_STATE) {
            throw new IllegalStateException("getFft() called while unexpected state");
        }

        return translateErrorCode(result);
    }

    @Override
    public int getWaveForm(byte[] waveform) throws IllegalStateException {
        checkNativeImplIsAvailable();

        int result = getWaveformImplNative(mNativeHandle, waveform);

        if (result == OpenSLMediaPlayer.Internal.RESULT_ILLEGAL_STATE) {
            throw new IllegalStateException("getWaveForm() called while unexpected state");
        }

        return translateErrorCode(result);
    }

    @Override
    public int getCaptureSize() throws IllegalStateException {
        checkNativeImplIsAvailable();

        final int[] size = mParamIntBuff;

        int result = getCaptureSizeImplNative(mNativeHandle, size);

        throwIllegalStateExceptionIfNeeded(result);

        return size[0];
    }

    @Override
    public int setCaptureSize(int size) throws IllegalStateException {
        checkNativeImplIsAvailable();

        int result = setCaptureSizeImplNative(mNativeHandle, size);

        if (result == OpenSLMediaPlayer.Internal.RESULT_ILLEGAL_STATE) {
            throw new IllegalStateException("setCaptureSize() called while unexpected state");
        }

        throwIllegalStateExceptionIfNeeded(result);

        return translateErrorCode(result);
    }

    @Override
    public int setDataCaptureListener(
            OnDataCaptureListener listener,
            int rate, boolean waveform, boolean fft) {
        checkNativeImplIsAvailable();

        if (listener == null) {
            rate = 0;
            waveform = false;
            fft = false;
        }

        int result = setDataCaptureListenerImplNative(
                mNativeHandle, rate, waveform, fft);

        if (result == OpenSLMediaPlayer.Internal.RESULT_SUCCESS) {
            mOnDataCaptureListener = listener;
        }

        throwIllegalStateExceptionIfNeeded(result);

        return translateErrorCode(result);
    }

    @Override
    public int[] getCaptureSizeRange() throws IllegalStateException {
        checkNativeImplIsAvailable();
        return sGetCaptureSizeRange();
    }

    @Override
    public int getMaxCaptureRate() throws IllegalStateException {
        checkNativeImplIsAvailable();
        return sGetMaxCaptureRate();
    }

    @Override
    public int getScalingMode() throws IllegalStateException {
        checkNativeImplIsAvailable();

        int[] mode = mParamIntBuff;

        int result = getScalingModeImplNative(mNativeHandle, mode);

        throwIllegalStateExceptionIfNeeded(result);

        return mode[0];
    }

    @Override
    public int setScalingMode(int mode) throws IllegalStateException {
        checkNativeImplIsAvailable();

        int result = setScalingModeImplNative(mNativeHandle, mode);

        throwIllegalStateExceptionIfNeeded(result);

        return translateErrorCode(result);
    }

    @Override
    public int getMeasurementMode() throws IllegalStateException {
        checkNativeImplIsAvailable();

        int[] mode = mParamIntBuff;

        int result = getMeasurementModeImplNative(mNativeHandle, mode);

        throwIllegalStateExceptionIfNeeded(result);

        return mode[0];
    }

    @Override
    public int setMeasurementMode(int mode) throws IllegalStateException {
        checkNativeImplIsAvailable();

        int result = setMeasurementModeImplNative(mNativeHandle, mode);

        throwIllegalStateExceptionIfNeeded(result);

        return translateErrorCode(result);
    }

    @Override
    public int getMeasurementPeakRms(MeasurementPeakRms measurement) {
        checkNativeImplIsAvailable();

        if (measurement == null)
            return ERROR_BAD_VALUE;

        int[] measurementBuff = mParamIntBuff;

        int result = getMeasurementPeakRmsImplNative(mNativeHandle, measurementBuff);

        throwIllegalStateExceptionIfNeeded(result);

        measurement.mPeak = measurementBuff[0];
        measurement.mRms = measurementBuff[1];

        return translateErrorCode(result);
    }

    //
    // Utilities
    //

    private void checkNativeImplIsAvailable() throws IllegalStateException {
        if (mNativeHandle == 0) {
            throw new IllegalStateException("Native implemenation handle is not present");
        }
    }

    private static void throwIllegalStateExceptionIfNeeded(int result) {
        if (result == OpenSLMediaPlayer.Internal.RESULT_DEAD_OBJECT)
            throw new IllegalStateException();
    }

    private static int translateErrorCode(int err) {
        switch (err) {
            case OpenSLMediaPlayer.Internal.RESULT_SUCCESS:
                return IVisualizer.SUCCESS;
            case OpenSLMediaPlayer.Internal.RESULT_INVALID_HANDLE:
                return IVisualizer.ERROR_NO_INIT;
            case OpenSLMediaPlayer.Internal.RESULT_ILLEGAL_STATE:
            case OpenSLMediaPlayer.Internal.RESULT_CONTROL_LOST:
                return IVisualizer.ERROR_INVALID_OPERATION;
            case OpenSLMediaPlayer.Internal.RESULT_ILLEGAL_ARGUMENT:
                return IVisualizer.ERROR_BAD_VALUE;
            case OpenSLMediaPlayer.Internal.RESULT_MEMORY_ALLOCATION_FAILED:
            case OpenSLMediaPlayer.Internal.RESULT_RESOURCE_ALLOCATION_FAILED:
                return IVisualizer.ERROR_NO_MEMORY;
            case OpenSLMediaPlayer.Internal.RESULT_DEAD_OBJECT:
                return IVisualizer.ERROR_DEAD_OBJECT;
            case OpenSLMediaPlayer.Internal.RESULT_ERROR:
            case OpenSLMediaPlayer.Internal.RESULT_INTERNAL_ERROR:
            case OpenSLMediaPlayer.Internal.RESULT_CONTENT_NOT_FOUND:
            case OpenSLMediaPlayer.Internal.RESULT_CONTENT_UNSUPPORTED:
            case OpenSLMediaPlayer.Internal.RESULT_IO_ERROR:
            case OpenSLMediaPlayer.Internal.RESULT_PERMISSION_DENIED:
            case OpenSLMediaPlayer.Internal.RESULT_TIMED_OUT:
            case OpenSLMediaPlayer.Internal.RESULT_IN_ERROR_STATE:
            default:
                return IVisualizer.ERROR;
        }
    }

    //
    // JNI binder internal methods
    //

    private static final int EVENT_TYPE_ON_WAVEFORM_DATA_CAPTURE = 0;
    private static final int EVENT_TYPE_ON_FFT_DATA_CAPTURE = 1;

    @SuppressWarnings("unchecked")
    private static void raiseCaptureEventFromNative(
            Object ref, int type, byte[] data, int samplingRate) {
        //
        // This method is called from the native implementation
        //
        WeakReference<OpenSLVisualizer> weak_ref = (WeakReference<OpenSLVisualizer>) ref;
        OpenSLVisualizer thiz = weak_ref.get();

        if (thiz == null)
            return;

        InternalHandler handler = thiz.mHandler;

        if (handler == null)
            return;

        switch (type) {
            case EVENT_TYPE_ON_WAVEFORM_DATA_CAPTURE:
                handler.sendMessage(
                        handler.obtainMessage(
                                InternalHandler.MSG_ON_WAVEFORM_DATA_CAPTURE,
                                samplingRate, 0, data));
                break;
            case EVENT_TYPE_ON_FFT_DATA_CAPTURE:
                handler.sendMessage(
                        handler.obtainMessage(
                                InternalHandler.MSG_ON_FFT_DATA_CAPTURE,
                                samplingRate, 0, data));
                break;
        }
    }

    private static class InternalHandler extends Handler {
        public static final int MSG_ON_WAVEFORM_DATA_CAPTURE = 1;
        public static final int MSG_ON_FFT_DATA_CAPTURE = 2;

        private WeakReference<OpenSLVisualizer> mHolder;

        public InternalHandler(OpenSLVisualizer holder) {
            mHolder = new WeakReference<OpenSLVisualizer>(holder);
        }

        @Override
        public void handleMessage(Message msg) {
            final OpenSLVisualizer holder = mHolder.get();

            if (holder == null)
                return;

            final OnDataCaptureListener listener = holder.mOnDataCaptureListener;

            if (listener == null)
                return;

            switch (msg.what) {
                case MSG_ON_WAVEFORM_DATA_CAPTURE:
                    listener.onWaveFormDataCapture(holder, (byte[]) msg.obj, msg.arg1);
                    break;
                case MSG_ON_FFT_DATA_CAPTURE:
                    listener.onFftDataCapture(holder, (byte[]) msg.obj, msg.arg1);
                    break;
            }
        }

        public void release() {
            removeMessages(MSG_ON_FFT_DATA_CAPTURE);
            removeMessages(MSG_ON_WAVEFORM_DATA_CAPTURE);
            mHolder.clear();
        }
    }

    //
    // Native methods
    //
    private static native long createNativeImplHandle(
            long context_handle, WeakReference<OpenSLVisualizer> weak_thiz);

    private static native void deleteNativeImplHandle(long handle);

    private static native int setEnabledImplNative(long handle, boolean enabled);

    private static native int getEnabledImplNative(long handle, boolean[] enabled);

    private static native int getSamplingRateImplNative(long handle, int[] samplingRate);

    private static native int getCaptureSizeImplNative(long handle, int[] size);

    private static native int setCaptureSizeImplNative(long handle, int size);

    private static native int getCaptureSizeRangeImplNative(int[] range);

    private static native int getFftImplNative(long handle, byte[] fft);

    private static native int getWaveformImplNative(long handle, byte[] waveform);

    private static native int setDataCaptureListenerImplNative(
            long handle, int rate, boolean waveform, boolean fft);

    private static native int getMaxCaptureRateImplNative(int[] rate);

    private static native int getScalingModeImplNative(long handle, int[] mode);

    private static native int setScalingModeImplNative(long handle, int mode);

    private static native int getMeasurementModeImplNative(long handle, int[] mode);

    private static native int setMeasurementModeImplNative(long handle, int mode);

    private static native int getMeasurementPeakRmsImplNative(long handle, int[] measurement);
}
