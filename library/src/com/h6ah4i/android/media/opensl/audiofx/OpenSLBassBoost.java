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

import android.util.Log;

import com.h6ah4i.android.media.audiofx.IBassBoost;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerContext;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerNativeLibraryLoader;

public class OpenSLBassBoost extends OpenSLAudioEffect implements IBassBoost {
    private static final String TAG = "BassBoost";

    // fields
    private long mNativeHandle;
    private static final boolean HAS_NATIVE;
    private final boolean mStrengthSupported;
    private int[] mParamIntBuff = new int[1];
    private short[] mParamShortBuff = new short[1];
    private boolean[] mParamBoolBuff = new boolean[1];

    static {
        // load native library
        HAS_NATIVE = OpenSLMediaPlayerNativeLibraryLoader.loadLibraries();
    }

    public OpenSLBassBoost(OpenSLMediaPlayerContext context) {
        if (context == null)
            throw new IllegalArgumentException("The argument 'contex' cannot be null");

        if (HAS_NATIVE) {
            mNativeHandle = createNativeImplHandle(
                    OpenSLMediaPlayer.Internal.getNativeHandle(context));
        }

        if (mNativeHandle == 0) {
            throw new UnsupportedOperationException("Failed to initialize native layer");
        }

        // set mStrengthSupported
        mStrengthSupported = getStrengthSupportedInternal();
    }

    private boolean getStrengthSupportedInternal() {
        final boolean[] strengthSupported = mParamBoolBuff;
        strengthSupported[0] = false;
        if (mNativeHandle != 0) {
            if (getStrengthSupportedImplNative(mNativeHandle, strengthSupported) != SUCCESS) {
                strengthSupported[0] = false;
            }
        }
        return strengthSupported[0];
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    @Override
    public void release() {
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
    public int getId() {
        checkNativeImplIsAvailable();

        final int[] id = mParamIntBuff;
        final int result = getIdImplNative(mNativeHandle, id);

        parseResultAndThrowExceptForIOExceptions(result);

        return id[0];
    }

    @Override
    public int setEnabled(boolean enabled) throws IllegalStateException {
        checkNativeImplIsAvailable();

        try {
            final int result = setEnabledImplNative(mNativeHandle, enabled);

            parseResultAndThrowExceptForIOExceptions(result);

            return SUCCESS;
        } catch (UnsupportedOperationException e) {
            return ERROR_INVALID_OPERATION;
        }
    }

    @Override
    public boolean getEnabled() throws IllegalStateException {
        checkNativeImplIsAvailable();

        final boolean[] enabled = mParamBoolBuff;
        final int result = getEnabledImplNative(mNativeHandle, enabled);

        if (result == OpenSLMediaPlayer.Internal.RESULT_CONTROL_LOST)
            return false;

        parseResultAndThrowExceptForIOExceptions(result);

        return enabled[0];
    }

    @Override
    public boolean hasControl() throws IllegalStateException {
        checkNativeImplIsAvailable();

        final boolean[] hasControl = mParamBoolBuff;
        final int result = hasControlImplNative(mNativeHandle, hasControl);

        if (result == OpenSLMediaPlayer.Internal.RESULT_CONTROL_LOST)
            return false;

        parseResultAndThrowExceptForIOExceptions(result);
        return hasControl[0];
    }

    @Override
    public boolean getStrengthSupported() {
        return mStrengthSupported;
    }

    @Override
    public void setStrength(short strength) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();
        final int result = setStrengthImplNative(mNativeHandle, strength);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public short getRoundedStrength() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();
        final short[] roundedStrength = mParamShortBuff;
        final int result = getRoundedStrengthImplNative(mNativeHandle, roundedStrength);

        parseResultAndThrowExceptForIOExceptions(result);

        return roundedStrength[0];
    }

    @Override
    public IBassBoost.Settings getProperties() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int[] values = mParamIntBuff;

        final int result = getPropertiesImplNative(mNativeHandle, values);

        parseResultAndThrowExceptForIOExceptions(result);

        final IBassBoost.Settings settings = new Settings();

        settings.strength = (short) (values[0] & 0xffff);

        return settings;
    }

    @Override
    public void setProperties(IBassBoost.Settings settings) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        if (settings == null)
            throw new IllegalArgumentException("The argument 'settings' cannot be null");

        final int[] values = mParamIntBuff;

        values[0] = settings.strength & 0xffff;

        final int result = setPropertiesImplNative(mNativeHandle, values);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setParameterListener(IBassBoost.OnParameterChangeListener listener) {
        // this method is not supported.
    }

    //
    // Utilities
    //

    private void checkNativeImplIsAvailable() throws IllegalStateException {
        if (mNativeHandle == 0) {
            throw new IllegalStateException("Native implemenation handle is not present");
        }
    }

    //
    // Native methods
    //
    private static native long createNativeImplHandle(long context_handle);

    private static native void deleteNativeImplHandle(long handle);

    private static native int setEnabledImplNative(long handle, boolean enabled);

    private static native int getEnabledImplNative(long handle, boolean[] enabled);

    private static native int getIdImplNative(long handle, int[] id);

    private static native int hasControlImplNative(long handle, boolean[] hasControl);

    private static native int getStrengthSupportedImplNative(long handle,
            boolean[] strengthSupported);

    private static native int getRoundedStrengthImplNative(long handle, short[] roundedStrength);

    private static native int getPropertiesImplNative(long handle, int[] settings);

    private static native int setStrengthImplNative(long handle, short strength);

    private static native int setPropertiesImplNative(long handle, int[] settings);
}
