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

import android.media.audiofx.PresetReverb;

import com.h6ah4i.android.media.audiofx.IPresetReverb;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;

public class StandardPresetReverb extends StandardAudioEffect implements IPresetReverb {

    private IPresetReverb.OnParameterChangeListener mUserOnParameterChangeListener;

    private android.media.audiofx.PresetReverb.OnParameterChangeListener mOnParameterChangeListener = new android.media.audiofx.PresetReverb.OnParameterChangeListener() {
        @Override
        public void onParameterChange(
                android.media.audiofx.PresetReverb effect, int status, int param, short value) {
            StandardPresetReverb.this.onParameterChange(effect, status, param, value);
        }
    };

    public StandardPresetReverb(int priority, int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException, RuntimeException {
        super(new PresetReverb(priority, audioSession));
        getPresetReverb().setParameterListener(mOnParameterChangeListener);
    }

    /**
     * Get underlying PresetReverb instance.
     *
     * @return underlying PresetReverb instance.
     */
    public PresetReverb getPresetReverb() {
        return (PresetReverb) super.getAudioEffect();
    }

    @Override
    public void setProperties(IPresetReverb.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
         checkIsNotReleased("setProperties()");
        getPresetReverb().setProperties(AudioEffectSettingsConverter.convert(settings));
    }

    @Override
    public void release() {
        workaroundPrevReleaseSync();
        super.release();
        mOnParameterChangeListener = null;
        mUserOnParameterChangeListener = null;
    }

    @Override
    public short getPreset() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getPreset()");
        return getPresetReverb().getPreset();
    }

    @Override
    public IPresetReverb.Settings getProperties() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkIsNotReleased("getProperties()");
        return AudioEffectSettingsConverter.convert(getPresetReverb().getProperties());
    }

    @Override
    public void setPreset(short preset) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("setPreset()");
        getPresetReverb().setPreset(preset);
    }

    @Override
    public void setParameterListener(
            IPresetReverb.OnParameterChangeListener listener) {
        checkIsNotReleased("setParameterListener()");
        mUserOnParameterChangeListener = listener;
    }

    /* package */void onParameterChange(
            android.media.audiofx.PresetReverb effect,
            int status, int param, short value) {
        IPresetReverb.OnParameterChangeListener listener = null;

        listener = mUserOnParameterChangeListener;

        if (listener != null) {
            listener.onParameterChange(this, status, param, value);
        }
    }
}
