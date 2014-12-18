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

import android.media.audiofx.Virtualizer;
import android.util.Log;

import com.h6ah4i.android.media.audiofx.IAudioEffect;
import com.h6ah4i.android.media.audiofx.IVirtualizer;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;

public class StandardVirtualizer implements IVirtualizer {
    private static final String TAG = "StandardVirtualizer";

    private Virtualizer mVirtualizer;
    private IVirtualizer.OnParameterChangeListener mUserOnParameterChangeListener;

    private android.media.audiofx.Virtualizer.OnParameterChangeListener mOnParameterChangeListener = new android.media.audiofx.Virtualizer.OnParameterChangeListener() {
        @Override
        public void onParameterChange(
                android.media.audiofx.Virtualizer effect, int status, int param, short value) {
            StandardVirtualizer.this.onParameterChange(effect, status, param, value);
        }
    };

    public StandardVirtualizer(int priority, int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException, RuntimeException {
        mVirtualizer = new Virtualizer(priority, audioSession);
        mVirtualizer.setParameterListener(mOnParameterChangeListener);
        initializeForCompat();
    }

    @Override
    public int setEnabled(boolean enabled) throws IllegalStateException {
        checkIsNotReleased();
        return mVirtualizer.setEnabled(enabled);
    }

    @Override
    public boolean getEnabled() throws IllegalStateException {
        checkIsNotReleased();
        return mVirtualizer.getEnabled();
    }

    @Override
    public int getId() throws IllegalStateException {
        checkIsNotReleased();
        return mVirtualizer.getId();
    }

    @Override
    public boolean hasControl() throws IllegalStateException {
        checkIsNotReleased();
        return mVirtualizer.hasControl();
    }

    @Override
    public void release() {
        mUserOnParameterChangeListener = null;

        if (mVirtualizer != null) {
            mVirtualizer.release();
            mVirtualizer = null;
        }
    }

    @Override
    public boolean getStrengthSupported() {
        if (mVirtualizer == null) {
            return false;
        }

        return mVirtualizer.getStrengthSupported();
    }

    @Override
    public void setStrength(short strength) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkIsNotReleased();
        verifyStrengthParameterRange(strength);
        mVirtualizer.setStrength(strength);
    }

    @Override
    public short getRoundedStrength() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased();
        return mVirtualizer.getRoundedStrength();
    }

    @Override
    public void setPropertiesCompat(IVirtualizer.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased();
        verifySettings(settings);
        mVirtualizer.setProperties(AudioEffectSettingsConverter.convert(settings));
    }

    @Override
    public IVirtualizer.Settings getPropertiesCompat() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkIsNotReleased();
        return AudioEffectSettingsConverter.convert(mVirtualizer.getProperties());
    }

    @Override
    public void setParameterListener(IVirtualizer.OnParameterChangeListener listener) {
        checkIsNotReleased();
        mUserOnParameterChangeListener = listener;
    }

    /* package */void onParameterChange(
            android.media.audiofx.Virtualizer effect,
            int status, int param, short value) {
        IVirtualizer.OnParameterChangeListener listener = null;

        listener = mUserOnParameterChangeListener;

        if (listener != null) {
            listener.onParameterChange(this, status, param, value);
        }
    }
    
    @Override
    public void setControlStatusListener(IAudioEffect.OnControlStatusChangeListener listener)
            throws IllegalStateException {
        checkIsNotReleased();
        mVirtualizer.setControlStatusListener(StandardAudioEffect.wrap(this, listener));
    }
    
    @Override
    public void setEnableStatusListener(IAudioEffect.OnEnableStatusChangeListener listener)
            throws IllegalStateException {
        checkIsNotReleased();
        mVirtualizer.setEnableStatusListener(StandardAudioEffect.wrap(this, listener));
    }

    private void checkIsNotReleased() {
        if (mVirtualizer == null) {
            throw new IllegalStateException("The virtualizer instance has already released");
        }
    }

    // === Fix unwanted behaviors ===
    private static final short STRENGTH_MIN = 0;
    private static final short STRENGTH_MAX = 1000;
    private static final short DEFAULT_STRENGTH = 750;

    void initializeForCompat() {
        try {
            mVirtualizer.setStrength(DEFAULT_STRENGTH);
        } catch (IllegalStateException e) {
            Log.e(TAG, "initializeForCompat()", e);
        }
    }

    private static void verifyStrengthParameterRange(short strength) {
        if (!(strength >= STRENGTH_MIN && strength <= STRENGTH_MAX))
            throw new IllegalArgumentException("bad parameter value: strength = " + strength);
    }

    private static void verifySettings(IVirtualizer.Settings settings) {
        if (settings == null)
            throw new IllegalArgumentException("The parameter 'settings' is null");
        verifyStrengthParameterRange(settings.strength);
    }
    // ==============================
}
