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

import com.h6ah4i.android.media.audiofx.IPreAmp;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerContext;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerNativeLibraryLoader;

public class OpenSLPreAmp extends OpenSLAudioEffect implements IPreAmp {
    private static final String TAG = "PreAmp";

    private long mNativeHandle;
    private static final boolean HAS_NATIVE;
    private int[] mParamIntBuff = new int[1];
    private float[] mParamFloatBuff = new float[1];
    private boolean[] mParamBoolBuff = new boolean[1];

    static {
        // load native library
        HAS_NATIVE = OpenSLMediaPlayerNativeLibraryLoader.loadLibraries();
    }

    public OpenSLPreAmp(OpenSLMediaPlayerContext context) throws
            RuntimeException,
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException
    {
        if (context == null)
            throw new IllegalArgumentException("The argument 'context' cannot be null");

        if (HAS_NATIVE) {
            mNativeHandle = createNativeImplHandle(
                    OpenSLMediaPlayer.Internal.getNativeHandle(context));
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
    public int setEnabled(boolean enabled) throws IllegalStateException {
        try {
            checkNativeImplIsAvailable();

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
    public int getId() throws IllegalStateException {
        checkNativeImplIsAvailable();

        final int[] id = mParamIntBuff;
        final int result = getIdImplNative(mNativeHandle, id);

        parseResultAndThrowExceptForIOExceptions(result);

        return id[0];
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
    public float getLevel() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException
    {
        checkNativeImplIsAvailable();

        final float[] level = mParamFloatBuff;
        final int result = getLevelImplNative(mNativeHandle, level);

        parseResultAndThrowExceptForIOExceptions(result);

        return level[0];
    }

    @Override
    public void setLevel(float level) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException
    {
        checkNativeImplIsAvailable();

        final int result = setLevelImplNative(mNativeHandle, level);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public Settings getProperties() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException
    {
        checkNativeImplIsAvailable();

        final Settings settings = new Settings();

        settings.level = getLevel();

        return settings;
    }

    @Override
    public void setProperties(Settings settings) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException
    {
        checkNativeImplIsAvailable();

        if (settings == null)
            throw new IllegalArgumentException("The argument 'settings' cannot be null");

        setLevel(settings.level);
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

    private static native int getLevelImplNative(long handle, float[] level);

    private static native int setLevelImplNative(long handle, float level);
}
