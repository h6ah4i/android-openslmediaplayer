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

import com.h6ah4i.android.media.audiofx.IBassBoost;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;

public class StandardBassBoost extends StandardAudioEffect implements IBassBoost {
    private static final String TAG = "StandardBassBoost";

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
        super(new BassBoost(priority, audioSession));
        getBassBoost().setParameterListener(mOnParameterChangeListener);
        initializeForCompat();
    }

    /**
     * Get underlying BassBoost instance.
     *
     * @return underlying BassBoost instance.
     */
    public BassBoost getBassBoost() {
        return (BassBoost) super.getAudioEffect();
    }

    @Override
    public void release() {
        super.release();
        mOnParameterChangeListener = null;
        mUserOnParameterChangeListener = null;
    }

    @Override
    public void setProperties(IBassBoost.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("setProperties()");
        verifySettings(settings);
        getBassBoost().setProperties(AudioEffectSettingsConverter.convert(settings));
    }

    @Override
    public boolean getStrengthSupported() {
        if (getBassBoost() == null) {
            return false;
        }
        return getBassBoost().getStrengthSupported();
    }

    @Override
    public void setStrength(short strength) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("setStrength()");
        verifyStrengthParameterRange(strength);
        getBassBoost().setStrength(strength);
    }

    @Override
    public short getRoundedStrength() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getRoundedStrength()");
        return getBassBoost().getRoundedStrength();
    }

    @Override
    public IBassBoost.Settings getProperties() throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getProperties()");
        return AudioEffectSettingsConverter.convert(getBassBoost().getProperties());
    }

    @Override
    public void setParameterListener(IBassBoost.OnParameterChangeListener listener) {
        checkIsNotReleased("setParameterListener()");
        mUserOnParameterChangeListener = listener;
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

    // === Fix unwanted behaviors ===
    private static final short STRENGTH_MIN = 0;
    private static final short STRENGTH_MAX = 1000;
    private static final short DEFAULT_STRENGTH = 0;

    void initializeForCompat() {
        try {
            getBassBoost().setStrength(DEFAULT_STRENGTH);
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
