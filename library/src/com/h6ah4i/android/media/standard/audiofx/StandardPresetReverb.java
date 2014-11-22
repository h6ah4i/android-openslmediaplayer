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

import com.h6ah4i.android.media.audiofx.IAudioEffect;
import com.h6ah4i.android.media.audiofx.IPresetReverb;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;

public class StandardPresetReverb extends android.media.audiofx.PresetReverb implements
        IPresetReverb {

    private Object mOnParameterChangeListenerLock = new Object();
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
        super(priority, audioSession);
        super.setParameterListener(mOnParameterChangeListener);
    }

    @Override
    public void setPropertiesCompat(IPresetReverb.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        super.setProperties(AudioEffectSettingsConverter.convert(settings));
    }

    @Override
    public void release() {
        mUserOnParameterChangeListener = null;
        super.release();
    }

    @Override
    public IPresetReverb.Settings getPropertiesCompat() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        return AudioEffectSettingsConverter.convert(super.getProperties());
    }

    @Override
    public void setProperties(android.media.audiofx.PresetReverb.Settings settings)
            throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        throwUseIPresetReverbVersionMethod();
    };

    @Override
    public android.media.audiofx.PresetReverb.Settings getProperties()
            throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        throwUseIPresetReverbVersionMethod();
        return null;
    }

    @Override
    public void setParameterListener(
            IPresetReverb.OnParameterChangeListener listener) {
        synchronized (mOnParameterChangeListenerLock) {
            mUserOnParameterChangeListener = listener;
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

    @Override
    public void setParameterListener(
            android.media.audiofx.PresetReverb.OnParameterChangeListener listener) {
        throwUseIPresetReverbVersionMethod();
    }

    /* package */void onParameterChange(
            android.media.audiofx.PresetReverb effect,
            int status, int param, short value) {
        IPresetReverb.OnParameterChangeListener listener = null;

        synchronized (mOnParameterChangeListenerLock) {
            listener = mUserOnParameterChangeListener;
        }

        if (listener != null) {
            listener.onParameterChange((StandardPresetReverb) effect, status, param, value);
        }
    }

    private static void throwUseIPresetReverbVersionMethod() {
        throw new IllegalStateException(
                "This method is not supported, please use IPresetReverb version");
    }
}
