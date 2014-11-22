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

import android.util.Log;

import com.h6ah4i.android.media.audiofx.IAudioEffect;
import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;
import com.h6ah4i.android.media.utils.DefaultEqualizerPresets;
import com.h6ah4i.android.media.utils.EqualizerBandInfoCorrector;

public class StandardEqualizer extends android.media.audiofx.Equalizer implements IEqualizer {

    private static final String TAG = "StandardEqualizer";

    private boolean mReleased;
    private Object mOnParameterChangeListenerLock = new Object();
    private IEqualizer.OnParameterChangeListener mUserOnParameterChangeListener;

    private android.media.audiofx.Equalizer.OnParameterChangeListener mOnParameterChangeListener = new android.media.audiofx.Equalizer.OnParameterChangeListener() {
        @Override
        public void onParameterChange(
                android.media.audiofx.Equalizer effect, int status, int param1, int param2,
                int value) {
            StandardEqualizer.this.onParameterChange(effect, status, param1, param2, value);
        }
    };

    public StandardEqualizer(int priority, int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException, RuntimeException {
        super(priority, audioSession);
        initializeForCompat();
        super.setParameterListener(mOnParameterChangeListener);
    }

    @Override
    public void release() {
        mReleased = true;
        mUserOnParameterChangeListener = null;
        super.release();
    }

    @Override
    public void setPropertiesCompat(IEqualizer.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        verifySettings(settings);

        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            workaroundGalaxyS4SetProperties(settings);
        } else if (mIsCyanogenModWorkaroundEnabled) {
            workaroundCyanogenModSetProperties(settings);
        } else {
            super.setProperties(AudioEffectSettingsConverter.convert(settings));
        }
    }

    @Override
    public IEqualizer.Settings getPropertiesCompat() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {

        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            checkState("getPropertiesCompat()");
            return workaroundGalaxyS4GetPropertiesCompat();
        } else if (mIsCyanogenModWorkaroundEnabled) {
            checkState("getPropertiesCompat()");
            return workaroundCyanogenModGetPropertiesCompat();
        } else {
            return AudioEffectSettingsConverter.convert(super.getProperties());
        }
    }

    @Override
    public void setProperties(android.media.audiofx.Equalizer.Settings settings)
            throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        throwUseIEqualizerVersionMethod();
    };

    @Override
    public android.media.audiofx.Equalizer.Settings getProperties() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        throwUseIEqualizerVersionMethod();
        return null;
    }

    @Override
    public void setParameterListener(IEqualizer.OnParameterChangeListener listener) {
        synchronized (mOnParameterChangeListenerLock) {
            mUserOnParameterChangeListener = listener;
        }
    }

    @Override
    public void setParameterListener(
            android.media.audiofx.Equalizer.OnParameterChangeListener listener) {
        throwUseIEqualizerVersionMethod();
    }

    /* package */void onParameterChange(
            android.media.audiofx.Equalizer effect,
            int status, int param1, int param2, int value) {
        IEqualizer.OnParameterChangeListener listener = null;

        synchronized (mOnParameterChangeListenerLock) {
            listener = mUserOnParameterChangeListener;
        }

        if (listener != null) {
            listener.onParameterChange((StandardEqualizer) effect, status, param1, param2, value);
        }
    }
    
    @Override
    public void setControlStatusListener(IAudioEffect.OnControlStatusChangeListener listener)
            throws IllegalStateException {
        super.setControlStatusListener(StandardAudioEffect.wrap(this, listener));
    }
    
    @Override
    public void setEnableStatusListener(IAudioEffect.OnEnableStatusChangeListener listener)
            throws IllegalStateException {
        super.setEnableStatusListener(StandardAudioEffect.wrap(this, listener));
    }

    private static void throwUseIEqualizerVersionMethod() {
        throw new IllegalStateException(
                "This method is not supported, please use IEqualizer version");
    }

    // === Fix unwanted behaviors ===
    private static final short DEFAULT_PRESET = 0;

    private short mNumBands;
    private short mNumPresets;
    private short mBandLevelMin;
    private short mBandLevelMax;
    private int[][] mBandFreqRange;
    private int[] mBandCenterFreq;
    private int[][] mCorrectedBandFreqRange;
    private int[] mCorrectedCenterFreq;

    // for usePreset() doesn't work devices (Galaxy S4)
    private boolean mIsWorkaroundHTCAndGalaxyS4Enabled;
    // for setProperties() doesn't work devices (CyangenMod)
    private boolean mIsCyanogenModWorkaroundEnabled;
    private short[] mWorkaroundBandLevels;
    private short mWorkaroundCurPreset;
    private boolean mIsNegativeBandLevelWorkaroundEnabled;

    private void initializeForCompat() {
        final int MIN = 0;
        final int MAX = 1;

        mNumBands = getNumberOfBands();
        mNumPresets = super.getNumberOfPresets();

        short[] levelRange = super.getBandLevelRange();

        mBandLevelMin = levelRange[0];
        mBandLevelMax = levelRange[1];

        mBandFreqRange = new int[mNumBands][2];
        mBandCenterFreq = new int[mNumBands];
        for (short band = 0; band < mNumBands; band++) {
            int center = super.getCenterFreq(band);
            int[] range = super.getBandFreqRange(band);
            mBandFreqRange[band][MIN] = range[MIN];
            mBandFreqRange[band][MAX] = range[MAX];
            mBandCenterFreq[band] = center;
        }

        // correct center freq. & band freq. range
        int[] correctedCenterFreq = new int[mNumBands];
        int[][] correctedBandFreqRange = new int[mNumBands][2];

        if (EqualizerBandInfoCorrector.correct(
                mNumBands, mBandCenterFreq, mBandFreqRange,
                correctedCenterFreq, correctedBandFreqRange)) {
            mCorrectedCenterFreq = correctedCenterFreq;
            mCorrectedBandFreqRange = correctedBandFreqRange;
        }

        initializePresetWorkarounds();
    }

    private void initializePresetWorkarounds() {
        // set to default preset
        // (NOTE: default preset of Galaxy S4 is "3")
        super.usePreset(DEFAULT_PRESET);

        if (mNumBands == 5) {
            short[] bandLevels = new short[mNumBands];
            for (short band = 0; band < mNumBands; band++) {
                bandLevels[band] = super.getBandLevel(band);
            }

            IEqualizer.Settings defSettings = DefaultEqualizerPresets.PRESET_NORMAL;

            if (!((bandLevels[0] == defSettings.bandLevels[0]) &&
                    (bandLevels[1] == defSettings.bandLevels[1]) &&
                    (bandLevels[2] == defSettings.bandLevels[2]) &&
                    (bandLevels[3] == defSettings.bandLevels[3]) && (bandLevels[4] == defSettings.bandLevels[4]))) {
                // check usePreset() method doesn't work properly...
                Log.d(TAG, "Use workaround version of Equalizer.usePreset() method");
                mIsWorkaroundHTCAndGalaxyS4Enabled = true;
            }

            if (!mIsWorkaroundHTCAndGalaxyS4Enabled && mNumPresets >= 2) {
                try {
                    super.usePreset((short) 1);
                    super.usePreset(DEFAULT_PRESET);
                } catch (IllegalArgumentException e) {
                    // HTC phones (HTC Evo 3D, Evo 4G LTE) raises
                    // IllegalArgumentException
                    mIsWorkaroundHTCAndGalaxyS4Enabled = true;
                }
            }
        } else if (mNumBands == 6) {
            // CyanogenMod (based) custom ROMs

            // Check setBandLevel() and getBandLevel() works properly
            // (If level is negative, +1 incremented value will be returned)
            // ex.)
            // set: -2, get: -1
            // set: -1, get: 0
            // set: 0, get: 0
            // set: 1, get: 1
            // set: 2, get: 2
            final short origBandLevel = super.getBandLevel((short) 0);
            super.setBandLevel((short) 0, (short) -1);
            mIsNegativeBandLevelWorkaroundEnabled = (super.getBandLevel((short) 0) == 0);
            super.setBandLevel((short) 0, origBandLevel);

            // Check setProperties() works properly
            android.media.audiofx.Equalizer.Settings settings = super.getProperties();
            try {
                settings.curPreset = DEFAULT_PRESET;
                super.setProperties(settings);
            } catch (IllegalArgumentException e) {
                mIsCyanogenModWorkaroundEnabled = true;
            }
        }

        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            mNumPresets = DefaultEqualizerPresets.NUM_PRESETS;
            mWorkaroundBandLevels = new short[mNumBands];
            workaroundGalaxyS4UsePreset(DEFAULT_PRESET);
        } else if (mIsCyanogenModWorkaroundEnabled) {
            mWorkaroundBandLevels = new short[mNumBands];
            workaroundCyanogenModUsePreset(DEFAULT_PRESET);
        }
    }

    private void workaroundGalaxyS4UsePreset(short preset) {
        if (preset != PRESET_UNDEFINED) {
            workaroundGalaxyS4SetProperties(DefaultEqualizerPresets.getPreset(preset));
        }
    }

    private void workaroundCyanogenModUsePreset(short preset) {
        if (preset != PRESET_UNDEFINED) {
            super.usePreset(preset);

            for (short band = 0; band < mNumBands; band++) {
                mWorkaroundBandLevels[band] = super.getBandLevel(band);
            }
            mWorkaroundCurPreset = preset;
        }
    }

    private String workaroundGalaxyS4GetPesetName(short preset) {
        if (preset >= 0 && preset < mNumPresets) {
            return DefaultEqualizerPresets.getName(preset);
        } else {
            return "";
        }
    }

    private short workaroundGetBandLevel(short band) {
        return mWorkaroundBandLevels[band];
    }

    private void workaroundGalaxyS4SetBandLevel(short band, short level) {
        super.setBandLevel(band, level);
        mWorkaroundBandLevels[band] = level;
        mWorkaroundCurPreset = PRESET_UNDEFINED;
    }

    private void workaroundCyanogenModSetBandLevel(short band, short level) {
        if (mIsNegativeBandLevelWorkaroundEnabled) {
            if (level < 0) {
                level -= (short) 1;
            }
        }
        super.setBandLevel(band, level);
        mWorkaroundBandLevels[band] = level;
        mWorkaroundCurPreset = PRESET_UNDEFINED;
    }

    private short workaroundGetCurrentPreset() {
        return mWorkaroundCurPreset;
    }

    private void workaroundGalaxyS4SetProperties(IEqualizer.Settings settings) {
        if (settings.curPreset != PRESET_UNDEFINED) {
            // NOTE: if curPreset has valid preset no.,
            // bandLevels values are not used.
            settings = DefaultEqualizerPresets.getPreset(settings.curPreset);
        }

        // apply
        if (settings.curPreset != PRESET_UNDEFINED) {
            try {
                super.usePreset(settings.curPreset);
            } catch (IllegalArgumentException e) {
                // HTC devices raises IllegalArgumentException
            }
        }
        for (short band = 0; band < settings.numBands; band++) {
            super.setBandLevel(band, settings.bandLevels[band]);
        }

        mWorkaroundCurPreset = settings.curPreset;
        System.arraycopy(settings.bandLevels, 0, mWorkaroundBandLevels, 0,
                mNumBands);
    }

    private void workaroundCyanogenModSetProperties(IEqualizer.Settings settings) {
        // apply
        if (settings.curPreset != PRESET_UNDEFINED) {
            super.usePreset(settings.curPreset);
            mWorkaroundCurPreset = settings.curPreset;
            for (short band = 0; band < settings.numBands; band++) {
                mWorkaroundBandLevels[band] = super.getBandLevel(band);
            }
        } else {
            for (short band = 0; band < settings.numBands; band++) {
                workaroundCyanogenModSetBandLevel(band, settings.bandLevels[band]);
            }

            mWorkaroundCurPreset = settings.curPreset;
            System.arraycopy(settings.bandLevels, 0, mWorkaroundBandLevels, 0, mNumBands);
        }
    }

    private IEqualizer.Settings workaroundGalaxyS4GetPropertiesCompat() {
        IEqualizer.Settings settings = new IEqualizer.Settings();
        settings.curPreset = mWorkaroundCurPreset;
        settings.numBands = mNumBands;
        settings.bandLevels = mWorkaroundBandLevels.clone();
        return settings;
    }

    private IEqualizer.Settings workaroundCyanogenModGetPropertiesCompat() {
        IEqualizer.Settings settings = new IEqualizer.Settings();
        settings.curPreset = mWorkaroundCurPreset;
        settings.numBands = mNumBands;
        settings.bandLevels = new short[mNumBands];

        for (short band = 0; band < mNumBands; band++) {
            settings.bandLevels[band] = super.getBandLevel(band);
        }

        return settings;
    }

    private static int[] getBandFreqRangeCompat(int[][] bandFreqRange, short band) {
        int[] range = new int[2];
        range[0] = bandFreqRange[band][0];
        range[1] = bandFreqRange[band][1];
        return range;
    }

    private static short getBandCompat(short numBands, int[][] range, int frequency) {
        final int MIN = 0;
        final int MAX = 1;

        if (frequency < range[0][MIN])
            return (short) (-1);
        if (frequency > range[numBands - 1][MAX])
            return (short) (-1);

        for (short band = 0; band < numBands; band++) {
            if ((frequency >= range[band][MIN])
                    && (frequency <= range[band][MAX])) {
                return band;
            }
        }

        return (short) (-1);
    }

    private static int getCenterFreqCompat(int[] centerFreq, short band) {
        return centerFreq[band];
    }

    @Override
    public short getNumberOfBands() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkState("getNumberOfBands()");
        return super.getNumberOfBands();
    }

    @Override
    public short getNumberOfPresets() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkState("getNumberOfPresets()");
        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            return mNumPresets;
        } else {
            return super.getNumberOfPresets();
        }
    }

    @Override
    public void usePreset(short preset) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyPresetRange(preset);

        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            workaroundGalaxyS4UsePreset(preset);
        } else if (mIsCyanogenModWorkaroundEnabled) {
            workaroundCyanogenModUsePreset(preset);
        } else {
            super.usePreset(preset);
        }
    }

    @Override
    public String getPresetName(short preset) {
        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            return workaroundGalaxyS4GetPesetName(preset);
        } else {
            return super.getPresetName(preset);
        }
    }

    @Override
    public short getBandLevel(short band) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyBandRange(band);
        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            checkState("getBandLevel()");
            return workaroundGetBandLevel(band);
        } else {
            return super.getBandLevel(band);
        }
    }

    @Override
    public void setBandLevel(short band, short level) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyBandRange(band);
        verifyBandLevelRange(level);

        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            workaroundGalaxyS4SetBandLevel(band, level);
        } else if (mIsNegativeBandLevelWorkaroundEnabled) {
            workaroundCyanogenModSetBandLevel(band, level);
        } else {
            super.setBandLevel(band, level);
        }
    }

    @Override
    public short getBand(int frequency) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        // NOTE:
        // Use getBandCompat() method instead of super.getBand(frequency),
        // because the original implementation returns wrong values.

        if (mCorrectedBandFreqRange != null) {
            checkState("getBand()");
            return getBandCompat(mNumBands, mCorrectedBandFreqRange, frequency);
        } else {
            return super.getBand(frequency);
        }
    }

    @Override
    public int[] getBandFreqRange(short band) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        // NOTE:
        // Use getBandFreqRangeCompat() method instead of
        // super.getBand(frequency),
        // because the original implementation returns wrong values.
        verifyBandRange(band);

        if (mCorrectedBandFreqRange != null) {
            checkState("getBandFreqRange()");

            return getBandFreqRangeCompat(mCorrectedBandFreqRange, band);
        } else {
            return super.getBandFreqRange(band);
        }
    }

    @Override
    public int getCenterFreq(short band) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyBandRange(band);

        if (mCorrectedCenterFreq != null) {
            checkState("getCenterFreq()");
            return getCenterFreqCompat(mCorrectedCenterFreq, band);
        } else {
            return super.getCenterFreq(band);
        }
    }

    @Override
    public short getCurrentPreset() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {

        short preset;

        if (mIsWorkaroundHTCAndGalaxyS4Enabled || mIsCyanogenModWorkaroundEnabled) {
            checkState("getCurrentPreset()");
            preset = workaroundGetCurrentPreset();
        } else {
            preset = super.getCurrentPreset();
        }

        if (!(preset >= 0 && preset < mNumPresets)) {
            // Fix return value (Galaxy S4 may returns (mNumPresets + 1))
            preset = PRESET_UNDEFINED;
        }

        return preset;
    }

    private void checkState(String methodName) throws IllegalStateException {
        if (mReleased) {
            throw (new IllegalStateException(methodName
                    + " called on uninitialized AudioEffect."));
        }
    }

    private void verifyBandLevelRange(short level) {
        if (!(level >= mBandLevelMin && level <= mBandLevelMax))
            throw new IllegalArgumentException("bad parameter value: level = " + level);
    }

    private void verifyBandRange(short band) {
        if (!(band >= 0 && band < mNumBands))
            throw new IllegalArgumentException("bad parameter value: band = " + band);
    }

    private void verifyPresetRange(short preset) {
        if (!((preset >= 0 && preset < mNumPresets) || (preset == PRESET_UNDEFINED)))
            throw new IllegalArgumentException("bad parameter value: preset = " + preset);
    }

    private void verifySettings(IEqualizer.Settings settings) {
        if (settings == null)
            throw new IllegalArgumentException("The parameter 'settings' is null");

        if (settings.bandLevels == null) {
            throw new IllegalArgumentException("settings invalid property: bandLevels is null");
        }

        if (settings.numBands != settings.bandLevels.length ||
                settings.numBands != mNumBands) {
            throw new IllegalArgumentException("settings invalid band count: " + settings.numBands);
        }

        for (short i = 0; i < mNumBands; i++) {
            verifyBandLevelRange(settings.bandLevels[i]);
        }

        verifyPresetRange(settings.curPreset);
    }

    // ==============================
}
