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

import com.h6ah4i.android.media.audiofx.IVirtualizer;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;

public class StandardVirtualizer extends StandardAudioEffect implements IVirtualizer {
    private static final String TAG = "StandardVirtualizer";

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
        super(new Virtualizer(priority, audioSession));
        getVirtualizer().setParameterListener(mOnParameterChangeListener);
        initializeForCompat();
    }
    /**
     * Get underlying Virtualizer instance.
     *
     * @return underlying Virtualizer instance.
     */
    public Virtualizer getVirtualizer() {
        return (Virtualizer) super.getAudioEffect();
    }

    @Override
    public void release() {
        super.release();
        mOnParameterChangeListener = null;
        mUserOnParameterChangeListener = null;
    }

    @Override
    public boolean getStrengthSupported() {
        if (getVirtualizer() == null) {
            return false;
        }

        return getVirtualizer().getStrengthSupported();
    }

    @Override
    public void setStrength(short strength) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkIsNotReleased("setStrength()");
        verifyStrengthParameterRange(strength);
        getVirtualizer().setStrength(strength);
    }

    @Override
    public short getRoundedStrength() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getRoundedStrength()");
        return getVirtualizer().getRoundedStrength();
    }

    @Override
    public void setProperties(IVirtualizer.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased();
        verifySettings(settings);
        getVirtualizer().setProperties(AudioEffectSettingsConverter.convert(settings));
    }

    @Override
    public IVirtualizer.Settings getProperties() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkIsNotReleased("getProperties()");
        return AudioEffectSettingsConverter.convert(getVirtualizer().getProperties());
    }

    @Override
    public void setParameterListener(IVirtualizer.OnParameterChangeListener listener) {
        checkIsNotReleased("setParameterListener()");
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

    // === Fix unwanted behaviors ===
    private static final short STRENGTH_MIN = 0;
    private static final short STRENGTH_MAX = 1000;
    private static final short DEFAULT_STRENGTH = 750;

    void initializeForCompat() {
        try {
            getVirtualizer().setStrength(DEFAULT_STRENGTH);
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
