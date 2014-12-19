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

import android.util.Log;

import com.h6ah4i.android.media.audiofx.IHQVisualizer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerContext;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerNativeLibraryLoader;

public class OpenSLHQVisualizer implements IHQVisualizer {
    private static final String TAG = "HQVisualizer";

    private long mNativeHandle;
    private static final boolean HAS_NATIVE;
    private int[] mParamIntBuff = new int[2];
    private boolean[] mParamBoolBuff = new boolean[1];

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

    public OpenSLHQVisualizer(OpenSLMediaPlayerContext context) {
        if (context == null)
            throw new IllegalArgumentException("The argument 'context' cannot be null");

        if (HAS_NATIVE) {
            mNativeHandle = createNativeImplHandle(
                    OpenSLMediaPlayer.Internal.getNativeHandle(context),
                    new WeakReference<OpenSLHQVisualizer>(this));
        }

        if (mNativeHandle == 0) {
            throw new UnsupportedOperationException("Failed to initialize native layer");
        }
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
    public int getNumChannels() throws IllegalStateException {
        checkNativeImplIsAvailable();

        final int[] numChannels = mParamIntBuff;

        int result = getNumChannelsImplNative(mNativeHandle, numChannels);

        throwIllegalStateExceptionIfNeeded(result);

        return numChannels[0];
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
    public int getWindowFunction() throws IllegalStateException {
        checkNativeImplIsAvailable();

        final int[] windowType = mParamIntBuff;

        int result = getWindowFunctionImplNative(mNativeHandle, windowType);

        throwIllegalStateExceptionIfNeeded(result);

        return windowType[0];
    }

    @Override
    public int setWindowFunction(int windowType) throws IllegalStateException {
        checkNativeImplIsAvailable();

        int result = setWindowFunctionImplNative(mNativeHandle, windowType);

        if (result == OpenSLMediaPlayer.Internal.RESULT_ILLEGAL_STATE) {
            throw new IllegalStateException("setWindowFunction() called while unexpected state");
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
                return IHQVisualizer.SUCCESS;
            case OpenSLMediaPlayer.Internal.RESULT_INVALID_HANDLE:
                return IHQVisualizer.ERROR_NO_INIT;
            case OpenSLMediaPlayer.Internal.RESULT_ILLEGAL_STATE:
            case OpenSLMediaPlayer.Internal.RESULT_CONTROL_LOST:
                return IHQVisualizer.ERROR_INVALID_OPERATION;
            case OpenSLMediaPlayer.Internal.RESULT_ILLEGAL_ARGUMENT:
                return IHQVisualizer.ERROR_BAD_VALUE;
            case OpenSLMediaPlayer.Internal.RESULT_MEMORY_ALLOCATION_FAILED:
            case OpenSLMediaPlayer.Internal.RESULT_RESOURCE_ALLOCATION_FAILED:
                return IHQVisualizer.ERROR_NO_MEMORY;
            case OpenSLMediaPlayer.Internal.RESULT_DEAD_OBJECT:
                return IHQVisualizer.ERROR_DEAD_OBJECT;
            case OpenSLMediaPlayer.Internal.RESULT_ERROR:
            case OpenSLMediaPlayer.Internal.RESULT_INTERNAL_ERROR:
            case OpenSLMediaPlayer.Internal.RESULT_CONTENT_NOT_FOUND:
            case OpenSLMediaPlayer.Internal.RESULT_CONTENT_UNSUPPORTED:
            case OpenSLMediaPlayer.Internal.RESULT_IO_ERROR:
            case OpenSLMediaPlayer.Internal.RESULT_PERMISSION_DENIED:
            case OpenSLMediaPlayer.Internal.RESULT_TIMED_OUT:
            case OpenSLMediaPlayer.Internal.RESULT_IN_ERROR_STATE:
            default:
                return IHQVisualizer.ERROR;
        }
    }

    //
    // JNI binder internal methods
    //

    private static final int EVENT_TYPE_ON_WAVEFORM_DATA_CAPTURE = 0;
    private static final int EVENT_TYPE_ON_FFT_DATA_CAPTURE = 1;

    @SuppressWarnings("unchecked")
    private static void raiseCaptureEventFromNative(
            Object ref, int type, float[] data, int numChannels, int samplingRate) {
        //
        // This method is called from the native implementation
        //
        WeakReference<OpenSLHQVisualizer> weak_ref = (WeakReference<OpenSLHQVisualizer>) ref;
        OpenSLHQVisualizer thiz = weak_ref.get();

        if (thiz == null)
            return;

        OnDataCaptureListener listener = thiz.mOnDataCaptureListener;

        if (listener == null)
            return;

        // HQVisualizer don't use Handler to avoid overhead

        switch (type) {
            case EVENT_TYPE_ON_WAVEFORM_DATA_CAPTURE:
                try {
                    listener.onWaveFormDataCapture(thiz, data, numChannels, samplingRate);
                } catch (Exception e) {
                    // Ignore all exceptions
                }
                break;
            case EVENT_TYPE_ON_FFT_DATA_CAPTURE:
                try {
                    listener.onFftDataCapture(thiz, data, numChannels, samplingRate);
                } catch (Exception e) {
                    // Ignore all exceptions
                }
                break;
        }
    }

    //
    // Native methods
    //
    private static native long createNativeImplHandle(
            long context_handle, WeakReference<OpenSLHQVisualizer> weak_thiz);

    private static native void deleteNativeImplHandle(long handle);

    private static native int setEnabledImplNative(long handle, boolean enabled);

    private static native int getEnabledImplNative(long handle, boolean[] enabled);

    private static native int getNumChannelsImplNative(long handle, int[] numChannels);

    private static native int getSamplingRateImplNative(long handle, int[] samplingRate);

    private static native int getCaptureSizeImplNative(long handle, int[] size);

    private static native int setCaptureSizeImplNative(long handle, int size);

    private static native int getCaptureSizeRangeImplNative(int[] range);

    private static native int setDataCaptureListenerImplNative(
            long handle, int rate, boolean waveform, boolean fft);

    private static native int getMaxCaptureRateImplNative(int[] rate);

    private static native int getWindowFunctionImplNative(long handle, int[] windowType);

    private static native int setWindowFunctionImplNative(long handle, int windowType);
}
