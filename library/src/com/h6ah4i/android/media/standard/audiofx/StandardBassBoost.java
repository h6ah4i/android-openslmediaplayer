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

import android.media.audiofx.BassBoost;
import android.util.Log;

import com.h6ah4i.android.media.audiofx.IAudioEffect;
import com.h6ah4i.android.media.audiofx.IBassBoost;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;

public class StandardBassBoost implements IBassBoost {
    private static final String TAG = "StandardBassBoost";

    private BassBoost mBassBoost;
    private IBassBoost.OnParameterChangeListener mUserOnParameterChangeListener;

    private android.media.audiofx.BassBoost.OnParameterChangeListener mOnParameterChangeListener = new android.media.audiofx.BassBoost.OnParameterChangeListener() {
        @Override
        public void onParameterChange(
                android.media.audiofx.BassBoost effect, int status, int param, short value) {
            StandardBassBoost.this.onParameterChange(effect, status, param, value);
        }
    };

    public StandardBassBoost(int priority, int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException, RuntimeException {
        mBassBoost = new BassBoost(priority, audioSession);
        mBassBoost.setParameterListener(mOnParameterChangeListener);
        initializeForCompat();
    }

    @Override
    public int setEnabled(boolean enabled) throws IllegalStateException {
        checkIsNotReleased();
        return mBassBoost.setEnabled(enabled);
    }

    @Override
    public boolean getEnabled() throws IllegalStateException {
        checkIsNotReleased();
        return mBassBoost.getEnabled();
    }

    @Override
    public int getId() throws IllegalStateException {
        checkIsNotReleased();
        return mBassBoost.getId();
    }

    @Override
    public boolean hasControl() throws IllegalStateException {
        checkIsNotReleased();
        return mBassBoost.hasControl();
    }

    @Override
    public void release() {
        mUserOnParameterChangeListener = null;

        if (mBassBoost != null) {
            mBassBoost.release();
            mBassBoost = null;
        }
    }

    @Override
    public void setPropertiesCompat(IBassBoost.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased();
        verifySettings(settings);
        mBassBoost.setProperties(AudioEffectSettingsConverter.convert(settings));
    }

    @Override
    public boolean getStrengthSupported() {
        if (mBassBoost == null) {
            return false;
        }
        return mBassBoost.getStrengthSupported();
    }

    @Override
    public void setStrength(short strength) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased();
        verifyStrengthParameterRange(strength);
        mBassBoost.setStrength(strength);
    }

    @Override
    public short getRoundedStrength() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased();
        return mBassBoost.getRoundedStrength();
    }

    @Override
    public IBassBoost.Settings getPropertiesCompat() throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased();
        return AudioEffectSettingsConverter.convert(mBassBoost.getProperties());
    }

    @Override
    public void setParameterListener(IBassBoost.OnParameterChangeListener listener) {
        checkIsNotReleased();
        mUserOnParameterChangeListener = listener;
    }
    
    @Override
    public void setControlStatusListener(IAudioEffect.OnControlStatusChangeListener listener)
            throws IllegalStateException {
        checkIsNotReleased();
        mBassBoost.setControlStatusListener(StandardAudioEffect.wrap(this, listener));
    }
    
    @Override
    public void setEnableStatusListener(IAudioEffect.OnEnableStatusChangeListener listener)
            throws IllegalStateException {
        checkIsNotReleased();
        mBassBoost.setEnableStatusListener(StandardAudioEffect.wrap(this, listener));
    }

    /* package */void onParameterChange(
            android.media.audiofx.BassBoost effect,
            int status, int param, short value) {
        IBassBoost.OnParameterChangeListener listener = null;

        listener = mUserOnParameterChangeListener;

        if (listener != null) {
            listener.onParameterChange(this, status, param, value);
        }
    }

    private void checkIsNotReleased() {
        if (mBassBoost == null) {
            throw new IllegalStateException("The bassboost instance has already released");
        }
    }

    // === Fix unwanted behaviors ===
    private static final short STRENGTH_MIN = 0;
    private static final short STRENGTH_MAX = 1000;
    private static final short DEFAULT_STRENGTH = 0;

    void initializeForCompat() {
        try {
            mBassBoost.setStrength(DEFAULT_STRENGTH);
        } catch (IllegalStateException e) {
            Log.e(TAG, "initializeForCompat()", e);
        }
    }

    private static void verifyStrengthParameterRange(short strength) {
        if (!(strength >= STRENGTH_MIN && strength <= STRENGTH_MAX))
            throw new IllegalArgumentException("bad parameter value: strength = " + strength);
    }

    private static void verifySettings(IBassBoost.Settings settings) {
        if (settings == null)
            throw new IllegalArgumentException("The parameter 'settings' is null");
        verifyStrengthParameterRange(settings.strength);
    }
    // ==============================
}
