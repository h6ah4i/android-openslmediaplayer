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

import android.media.audiofx.Equalizer;
import android.util.Log;

import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;
import com.h6ah4i.android.media.utils.DefaultEqualizerPresets;
import com.h6ah4i.android.media.utils.EqualizerBandInfoCorrector;

public class StandardEqualizer extends StandardAudioEffect implements IEqualizer {

    private static final String TAG = "StandardEqualizer";

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
        super(new Equalizer(priority, audioSession));
        getEqualizer().setParameterListener(mOnParameterChangeListener);
        initializeForCompat();
    }

    /**
     * Get underlying Equalizer instance.
     *
     * @return underlying Equalizer instance.
     */
    public Equalizer getEqualizer() {
        return (Equalizer) super.getAudioEffect();
    }

    @Override
    public void release() {
        super.release();
        mOnParameterChangeListener = null;
        mUserOnParameterChangeListener = null;
    }

    @Override
    public void setProperties(IEqualizer.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        verifySettings(settings);

        checkIsNotReleased("setProperties()");

        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            workaroundGalaxyS4SetProperties(settings);
        } else if (mIsCyanogenModWorkaroundEnabled) {
            workaroundCyanogenModSetProperties(settings);
        } else {
            getEqualizer().setProperties(AudioEffectSettingsConverter.convert(settings));
        }
    }

    @Override
    public IEqualizer.Settings getProperties() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {

        checkIsNotReleased("getProperties()");

        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            return workaroundGalaxyS4GetPropertiesCompat();
        } else if (mIsCyanogenModWorkaroundEnabled) {
            return workaroundCyanogenModGetPropertiesCompat();
        } else {
            return AudioEffectSettingsConverter.convert(getEqualizer().getProperties());
        }
    }

    @Override
    public void setParameterListener(IEqualizer.OnParameterChangeListener listener) {
        checkIsNotReleased("setParameterListener()");
        mUserOnParameterChangeListener = listener;
    }

    /* package */void onParameterChange(
            android.media.audiofx.Equalizer effect,
            int status, int param1, int param2, int value) {
        IEqualizer.OnParameterChangeListener listener = null;

        listener = mUserOnParameterChangeListener;

        if (listener != null) {
            listener.onParameterChange(this, status, param1, param2, value);
        }
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

        final Equalizer eq = getEqualizer();

        mNumBands = eq.getNumberOfBands();
        mNumPresets = eq.getNumberOfPresets();

        short[] levelRange = eq.getBandLevelRange();

        mBandLevelMin = levelRange[0];
        mBandLevelMax = levelRange[1];

        mBandFreqRange = new int[mNumBands][2];
        mBandCenterFreq = new int[mNumBands];
        for (short band = 0; band < mNumBands; band++) {
            int center = eq.getCenterFreq(band);
            int[] range = eq.getBandFreqRange(band);
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
        final Equalizer eq = getEqualizer();

        eq.usePreset(DEFAULT_PRESET);

        if (mNumBands == 5) {
            short[] bandLevels = new short[mNumBands];
            for (short band = 0; band < mNumBands; band++) {
                bandLevels[band] = eq.getBandLevel(band);
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
                    eq.usePreset((short) 1);
                    eq.usePreset(DEFAULT_PRESET);
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
            final short origBandLevel = eq.getBandLevel((short) 0);
            eq.setBandLevel((short) 0, (short) -1);
            mIsNegativeBandLevelWorkaroundEnabled = (eq.getBandLevel((short) 0) == 0);
            eq.setBandLevel((short) 0, origBandLevel);

            // Check setProperties() works properly
            android.media.audiofx.Equalizer.Settings settings = eq.getProperties();
            try {
                settings.curPreset = DEFAULT_PRESET;
                eq.setProperties(settings);
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
            final Equalizer eq = getEqualizer();

            eq.usePreset(preset);

            for (short band = 0; band < mNumBands; band++) {
                mWorkaroundBandLevels[band] = eq.getBandLevel(band);
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
        getEqualizer().setBandLevel(band, level);
        mWorkaroundBandLevels[band] = level;
        mWorkaroundCurPreset = PRESET_UNDEFINED;
    }

    private void workaroundCyanogenModSetBandLevel(short band, short level) {
        if (mIsNegativeBandLevelWorkaroundEnabled) {
            if (level < 0) {
                level -= (short) 1;
            }
        }
        getEqualizer().setBandLevel(band, level);
        mWorkaroundBandLevels[band] = level;
        mWorkaroundCurPreset = PRESET_UNDEFINED;
    }

    private short workaroundGetCurrentPreset() {
        return mWorkaroundCurPreset;
    }

    private void workaroundGalaxyS4SetProperties(IEqualizer.Settings settings) {
        final Equalizer eq = getEqualizer();

        if (settings.curPreset != PRESET_UNDEFINED) {
            // NOTE: if curPreset has valid preset no.,
            // bandLevels values are not used.
            settings = DefaultEqualizerPresets.getPreset(settings.curPreset);
        }

        // apply
        if (settings.curPreset != PRESET_UNDEFINED) {
            try {
                eq.usePreset(settings.curPreset);
            } catch (IllegalArgumentException e) {
                // HTC devices raises IllegalArgumentException
            }
        }
        for (short band = 0; band < settings.numBands; band++) {
            eq.setBandLevel(band, settings.bandLevels[band]);
        }

        mWorkaroundCurPreset = settings.curPreset;
        System.arraycopy(settings.bandLevels, 0, mWorkaroundBandLevels, 0,
                mNumBands);
    }

    private void workaroundCyanogenModSetProperties(IEqualizer.Settings settings) {
        // apply
        if (settings.curPreset != PRESET_UNDEFINED) {
            final Equalizer eq = getEqualizer();

            eq.usePreset(settings.curPreset);
            mWorkaroundCurPreset = settings.curPreset;
            for (short band = 0; band < settings.numBands; band++) {
                mWorkaroundBandLevels[band] = eq.getBandLevel(band);
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
        final Equalizer eq = getEqualizer();
        IEqualizer.Settings settings = new IEqualizer.Settings();

        settings.curPreset = mWorkaroundCurPreset;
        settings.numBands = mNumBands;
        settings.bandLevels = new short[mNumBands];

        for (short band = 0; band < mNumBands; band++) {
            settings.bandLevels[band] = eq.getBandLevel(band);
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
        checkIsNotReleased("getNumberOfBands()");
        return getEqualizer().getNumberOfBands();
    }

    @Override
    public short getNumberOfPresets() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkIsNotReleased("getNumberOfPresets()");
        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            return mNumPresets;
        } else {
            return getEqualizer().getNumberOfPresets();
        }
    }

    @Override
    public void usePreset(short preset) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyPresetRange(preset);

        checkIsNotReleased("usePreset()");
        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            workaroundGalaxyS4UsePreset(preset);
        } else if (mIsCyanogenModWorkaroundEnabled) {
            workaroundCyanogenModUsePreset(preset);
        } else {
            getEqualizer().usePreset(preset);
        }
    }

    @Override
    public String getPresetName(short preset) {
        checkIsNotReleased("getPresetName()");
        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            return workaroundGalaxyS4GetPesetName(preset);
        } else {
            return getEqualizer().getPresetName(preset);
        }
    }

    @Override
    public short getBandLevel(short band) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyBandRange(band);
        checkIsNotReleased("getBandLevel()");
        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            return workaroundGetBandLevel(band);
        } else {
            return getEqualizer().getBandLevel(band);
        }
    }

    @Override
    public short[] getBandLevelRange() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getBandLevelRange()");
        return getEqualizer().getBandLevelRange();
    }

    @Override
    public void setBandLevel(short band, short level) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyBandRange(band);
        verifyBandLevelRange(level);

        checkIsNotReleased("setBandLevel()");

        if (mIsWorkaroundHTCAndGalaxyS4Enabled) {
            workaroundGalaxyS4SetBandLevel(band, level);
        } else if (mIsNegativeBandLevelWorkaroundEnabled) {
            workaroundCyanogenModSetBandLevel(band, level);
        } else {
            getEqualizer().setBandLevel(band, level);
        }
    }

    @Override
    public short getBand(int frequency) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        // NOTE:
        // Use getBandCompat() method instead of super.getBand(frequency),
        // because the original implementation returns wrong values.

        checkIsNotReleased("getBand()");

        if (mCorrectedBandFreqRange != null) {
            return getBandCompat(mNumBands, mCorrectedBandFreqRange, frequency);
        } else {
            return getEqualizer().getBand(frequency);
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

        checkIsNotReleased("getBandFreqRange()");

        if (mCorrectedBandFreqRange != null) {
            return getBandFreqRangeCompat(mCorrectedBandFreqRange, band);
        } else {
            return getEqualizer().getBandFreqRange(band);
        }
    }

    @Override
    public int getCenterFreq(short band) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyBandRange(band);

        checkIsNotReleased("getCenterFreq()");
        if (mCorrectedCenterFreq != null) {
            return getCenterFreqCompat(mCorrectedCenterFreq, band);
        } else {
            return getEqualizer().getCenterFreq(band);
        }
    }

    @Override
    public short getCurrentPreset() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {

        short preset;

        checkIsNotReleased("getCurrentPreset()");
        if (mIsWorkaroundHTCAndGalaxyS4Enabled || mIsCyanogenModWorkaroundEnabled) {
            preset = workaroundGetCurrentPreset();
        } else {
            preset = getEqualizer().getCurrentPreset();
        }

        if (!(preset >= 0 && preset < mNumPresets)) {
            // Fix return value (Galaxy S4 may returns (mNumPresets + 1))
            preset = PRESET_UNDEFINED;
        }

        return preset;
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
