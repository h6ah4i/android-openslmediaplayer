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

import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerContext;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerNativeLibraryLoader;

public class OpenSLEnvironmentalReverb extends OpenSLAudioEffect implements IEnvironmentalReverb {
    private static final String TAG = "EnvironmentalReverb";

    private static final int AUX_EFFECT_ID = OpenSLMediaPlayer.Internal.AUX_EFFECT_ENVIRONMENTAL_REVERB;

    // fields
    private long mNativeHandle;
    private static final boolean HAS_NATIVE;
    private int[] mParamIntBuff = new int[10];
    private short[] mParamShortBuff = new short[1];
    private boolean[] mParamBoolBuff = new boolean[1];

    static {
        // load native library
        HAS_NATIVE = OpenSLMediaPlayerNativeLibraryLoader.loadLibraries();
    }

    public OpenSLEnvironmentalReverb(OpenSLMediaPlayerContext context) {
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
    public short getDecayHFRatio() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException
    {
        checkNativeImplIsAvailable();

        final short[] decayHFRatio = mParamShortBuff;
        final int result = getDecayHFRatioImplNative(mNativeHandle, decayHFRatio);

        parseResultAndThrowExceptForIOExceptions(result);

        return decayHFRatio[0];
    }

    @Override
    public int getDecayTime() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int[] decayTime = mParamIntBuff;
        final int result = getDecayTimeImplNative(mNativeHandle, decayTime);

        parseResultAndThrowExceptForIOExceptions(result);

        return decayTime[0];
    }

    @Override
    public short getDensity() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final short[] density = mParamShortBuff;
        final int result = getDensityImplNative(mNativeHandle, density);

        parseResultAndThrowExceptForIOExceptions(result);

        return density[0];
    }

    @Override
    public short getDiffusion() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final short[] diffusion = mParamShortBuff;
        final int result = getDiffusionImplNative(mNativeHandle, diffusion);

        parseResultAndThrowExceptForIOExceptions(result);

        return diffusion[0];
    }

    @Override
    public IEnvironmentalReverb.Settings getProperties() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int[] values = mParamIntBuff;

        final int result = getPropertiesImplNative(mNativeHandle, values);

        parseResultAndThrowExceptForIOExceptions(result);

        final IEnvironmentalReverb.Settings settings = new Settings();

        settings.roomLevel = (short) (values[0] & 0xffff);
        settings.roomHFLevel = (short) (values[1] & 0xffff);
        settings.decayTime = values[2];
        settings.decayHFRatio = (short) (values[3] & 0xffff);
        settings.reflectionsLevel = (short) (values[4] & 0xffff);
        settings.reflectionsDelay = values[5];
        settings.reverbLevel = (short) (values[6] & 0xffff);
        settings.reverbDelay = values[7];
        settings.diffusion = (short) (values[8] & 0xffff);
        settings.density = (short) (values[9] & 0xffff);

        return settings;
    }

    @Override
    public int getReflectionsDelay() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int[] reflectionsDelay = mParamIntBuff;
        final int result = getReflectionsDelayImplNative(mNativeHandle, reflectionsDelay);

        parseResultAndThrowExceptForIOExceptions(result);

        return reflectionsDelay[0];
    }

    @Override
    public short getReflectionsLevel() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final short[] reflectionsLevel = mParamShortBuff;
        final int result = getReflectionsLevelImplNative(mNativeHandle, reflectionsLevel);

        parseResultAndThrowExceptForIOExceptions(result);

        return reflectionsLevel[0];
    }

    @Override
    public int getReverbDelay() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int[] reverbDelay = mParamIntBuff;
        final int result = getReverbDelayImplNative(mNativeHandle, reverbDelay);

        parseResultAndThrowExceptForIOExceptions(result);

        return reverbDelay[0];
    }

    @Override
    public short getReverbLevel() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final short[] reverbLevel = mParamShortBuff;
        final int result = getReverbLevelImplNative(mNativeHandle, reverbLevel);

        parseResultAndThrowExceptForIOExceptions(result);

        return reverbLevel[0];
    }

    @Override
    public short getRoomHFLevel() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final short[] roomHF = mParamShortBuff;
        final int result = getRoomHFLevelImplNative(mNativeHandle, roomHF);

        parseResultAndThrowExceptForIOExceptions(result);

        return roomHF[0];
    }

    @Override
    public short getRoomLevel() throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final short[] roomLevel = mParamShortBuff;
        final int result = getRoomLevelImplNative(mNativeHandle, roomLevel);

        parseResultAndThrowExceptForIOExceptions(result);

        return roomLevel[0];
    }

    @Override
    public void setDecayHFRatio(short decayHFRatio) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setDecayHFRatioImplNative(mNativeHandle, decayHFRatio);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setDecayTime(int decayTime) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setDecayTimeImplNative(mNativeHandle, decayTime);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setDensity(short density) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setDensityImplNative(mNativeHandle, density);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setDiffusion(short diffusion) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setDiffusionImplNative(mNativeHandle, diffusion);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    // void setParameterListener(EnvironmentalReverb.OnParameterChangeListener
    // listener) {
    //
    // }

    @Override
    public void setProperties(IEnvironmentalReverb.Settings settings) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        if (settings == null)
            throw new IllegalArgumentException("The argument 'settings' cannot be null");

        final int[] values = mParamIntBuff;

        values[0] = settings.roomLevel & 0xffff;
        values[1] = settings.roomHFLevel & 0xffff;
        values[2] = settings.decayTime;
        values[3] = settings.decayHFRatio & 0xffff;
        values[4] = settings.reflectionsLevel & 0xffff;
        values[5] = settings.reflectionsDelay;
        values[6] = settings.reverbLevel & 0xffff;
        values[7] = settings.reverbDelay;
        values[8] = settings.diffusion & 0xffff;
        values[9] = settings.density & 0xffff;

        final int result = setPropertiesImplNative(mNativeHandle, values);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setReflectionsDelay(int reflectionsDelay) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setReflectionsDelayImplNative(mNativeHandle, reflectionsDelay);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setReflectionsLevel(short reflectionsLevel) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setReflectionsDelayImplNative(mNativeHandle, reflectionsLevel);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setReverbDelay(int reverbDelay) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setReverbDelayImplNative(mNativeHandle, reverbDelay);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setReverbLevel(short reverbLevel) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setReverbLevelImplNative(mNativeHandle, reverbLevel);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setRoomHFLevel(short roomHF) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setRoomHFLevelImplNative(mNativeHandle, roomHF);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setRoomLevel(short room) throws
            IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkNativeImplIsAvailable();

        final int result = setRoomLevelImplNative(mNativeHandle, room);

        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void setParameterListener(IEnvironmentalReverb.OnParameterChangeListener listener) {
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

    private static native int getDecayHFRatioImplNative(long handle, short[] decayHFRatio);

    private static native int getDecayTimeImplNative(long handle, int[] decayTime);

    private static native int getDensityImplNative(long handle, short[] density);

    private static native int getDiffusionImplNative(long handle, short[] diffusion);

    private static native int getPropertiesImplNative(long handle, int[] settings);

    private static native int getReflectionsDelayImplNative(long handle, int[] reflectionsDecay);

    private static native int getReflectionsLevelImplNative(long handle, short[] reflectionsLevel);

    private static native int getReverbDelayImplNative(long handle, int[] reverbDelay);

    private static native int getReverbLevelImplNative(long handle, short[] reverbLevel);

    private static native int getRoomHFLevelImplNative(long handle, short[] roomHF);

    private static native int getRoomLevelImplNative(long handle, short[] roomLevel);

    private static native int setDecayHFRatioImplNative(long handle, short decayHFRatio);

    private static native int setDecayTimeImplNative(long handle, int decayTime);

    private static native int setDensityImplNative(long handle, short density);

    private static native int setDiffusionImplNative(long handle, short diffusion);

    // private static native int setParameterListenerImplNative(long handle,
    // EnvironmentalReverb.OnParameterChangeListener listener);
    private static native int setPropertiesImplNative(long handle, int[] settings);

    private static native int setReflectionsDelayImplNative(long handle, int reflectionsDelay);

    private static native int setReflectionsLevelImplNative(long handle, short reflectionsLevel);

    private static native int setReverbDelayImplNative(long handle, int reverbDelay);

    private static native int setReverbLevelImplNative(long handle, short reverbLevel);

    private static native int setRoomHFLevelImplNative(long handle, short roomHF);

    private static native int setRoomLevelImplNative(long handle, short room);

    private static native int usePresetImplNative(long handle, int preset);
}
