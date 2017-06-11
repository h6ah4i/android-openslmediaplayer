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


package com.h6ah4i.android.media.opensl;

import android.content.Context;
import android.media.AudioManager;
import android.util.Log;

import com.h6ah4i.android.media.IReleasable;
import com.h6ah4i.android.media.utils.AudioSystemUtils;
import com.h6ah4i.android.media.utils.AudioSystemUtils.AudioSystemProperties;

public class OpenSLMediaPlayerContext implements IReleasable {
    private static final String TAG = "OpenSLMediaPlayerCtx";

    // options
    public static final int OPTION_USE_BASSBOOST = (1 << 0);
    public static final int OPTION_USE_VIRTUALIZER = (1 << 1);
    public static final int OPTION_USE_EQUALIZER = (1 << 2);
    public static final int OPTION_USE_ENVIRONMENAL_REVERB = (1 << 8);
    public static final int OPTION_USE_PRESET_REVERB = (1 << 9);
    public static final int OPTION_USE_VISUALIZER = (1 << 16);
    public static final int OPTION_USE_HQ_EQUALIZER = (1 << 17);
    public static final int OPTION_USE_PREAMP = (1 << 18);
    public static final int OPTION_USE_HQ_VISUALIZER = (1 << 19);

    // resampler quality specifiler
    public static final int RESAMPLER_QUALITY_LOW = 0;
    public static final int RESAMPLER_QUALITY_MIDDLE = 1;
    public static final int RESAMPLER_QUALITY_HIGH = 2;

    // HQEqualizer implementation type specifier
    public static final int HQ_EQUALIZER_IMPL_BASIC_PEAKING_FILTER = 0;
    public static final int HQ_EQUALIZER_IMPL_FLAT_GAIN_RESPONSE = 1;

    // Sink back-end implementation type specifier
    public static final int SINK_BACKEND_TYPE_OPENSL = 0;
    public static final int SINK_BACKEND_TYPE_AUDIO_TRACK = 1;

    private long mNativeHandle;
    private static final boolean HAS_NATIVE;
    private boolean mHasNative;
    private AudioSystemUtils.AudioSystemProperties mProperties;

    static {
        // load native library
        HAS_NATIVE = OpenSLMediaPlayerNativeLibraryLoader.loadLibraries();
    }

    public static class Parameters {
        public int options = 0;
        public int streamType = AudioManager.STREAM_MUSIC;
        public int shortFadeDuration = 15; // [milli seconds]
        public int longFadeDuration = 1500; // [milli seconds]
        public int resamplerQuality = RESAMPLER_QUALITY_MIDDLE;
        public int hqEqualizerImplType = HQ_EQUALIZER_IMPL_BASIC_PEAKING_FILTER;
        public int sinkBackEndType = SINK_BACKEND_TYPE_OPENSL;
        public boolean useLowLatencyIfAvailable = false;
        public boolean useFloatingPointIfAvailable = true;
    }

    public OpenSLMediaPlayerContext(Context context, Parameters params) {
        final AudioSystemProperties props = AudioSystemUtils.getProperties(context);

        if (params == null) {
            params = new Parameters();
        }

        boolean hasNative = false;
        if (HAS_NATIVE) {
            try {
                final int[] iparams = new int[13];

                iparams[0] = props.outputSampleRate * 1000; // [Hz] -> [milli hertz]
                iparams[1] = props.outputFramesPerBuffer;
                iparams[2] = props.supportLowLatency ? 1 : 0;
                iparams[3] = props.supportFloatingPoint ? 1 : 0;
                iparams[4] = params.options;
                iparams[5] = params.streamType;
                iparams[6] = params.shortFadeDuration;
                iparams[7] = params.longFadeDuration;
                iparams[8] = params.resamplerQuality;
                iparams[9] = params.hqEqualizerImplType;
                iparams[10] = params.sinkBackEndType;
                iparams[11] = params.useLowLatencyIfAvailable ? 1 : 0;
                iparams[12] = params.useFloatingPointIfAvailable ? 1 : 0;

                mNativeHandle = createNativeImplHandle(iparams);
                if (mNativeHandle != 0) {
                    hasNative = true;
                }
            } catch (Throwable e) {
            }
        }
        mHasNative = hasNative;
        mProperties = props;
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    @Override
    public void release() {
        try {
            if (mHasNative && mNativeHandle != 0) {
                deleteNativeImplHandle(mNativeHandle);
                mNativeHandle = 0;
            }
        } catch (Exception e) {
            Log.e(TAG, "release()", e);
        }
    }

    //
    // Internal methods
    //

    /** @hide */
    /* package */long getNativeHandle() {
        return mNativeHandle;
    }

    /** @hide */
    /* package */
    int getAudioSessionId() {
        return getAudioSessionIdImplNative(mNativeHandle);
    }

    //
    // Native methods
    //
    private static native long createNativeImplHandle(int[] params);

    private static native void deleteNativeImplHandle(long handle);

    private static native int getAudioSessionIdImplNative(long handle);
}
