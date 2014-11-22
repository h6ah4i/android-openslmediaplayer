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
import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;

public class StandardEnvironmentalReverb extends android.media.audiofx.EnvironmentalReverb
        implements IEnvironmentalReverb {

    private Object mOnParameterChangeListenerLock = new Object();
    private IEnvironmentalReverb.OnParameterChangeListener mUserOnParameterChangeListener;

    private android.media.audiofx.EnvironmentalReverb.OnParameterChangeListener mOnParameterChangeListener = new android.media.audiofx.EnvironmentalReverb.OnParameterChangeListener() {
        @Override
        public void onParameterChange(
                android.media.audiofx.EnvironmentalReverb effect, int status, int param, int value) {
            StandardEnvironmentalReverb.this.onParameterChange(effect, status, param, value);
        }
    };

    public StandardEnvironmentalReverb(int priority, int audioSession)
            throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException, RuntimeException {
        super(priority, audioSession);
        super.setParameterListener(mOnParameterChangeListener);
    }

    @Override
    public void release() {
        mUserOnParameterChangeListener = null;
        super.release();
    }

    @Override
    public void setPropertiesCompat(IEnvironmentalReverb.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        verifySettings(settings);
        super.setProperties(AudioEffectSettingsConverter.convert(settings));
    }

    @Override
    public IEnvironmentalReverb.Settings getPropertiesCompat() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        return AudioEffectSettingsConverter.convert(super.getProperties());
    }

    @Override
    public void setProperties(android.media.audiofx.EnvironmentalReverb.Settings settings)
            throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        throwUseIEnvironmentalReverbVersionMethod();
    };

    @Override
    public android.media.audiofx.EnvironmentalReverb.Settings getProperties()
            throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        throwUseIEnvironmentalReverbVersionMethod();
        return null;
    }

    @Override
    public void setParameterListener(
            IEnvironmentalReverb.OnParameterChangeListener listener) {
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
            android.media.audiofx.EnvironmentalReverb.OnParameterChangeListener listener) {
        throwUseIEnvironmentalReverbVersionMethod();
    }

    /* package */void onParameterChange(
            android.media.audiofx.EnvironmentalReverb effect,
            int status, int param, int value) {
        IEnvironmentalReverb.OnParameterChangeListener listener = null;

        synchronized (mOnParameterChangeListenerLock) {
            listener = mUserOnParameterChangeListener;
        }

        if (listener != null) {
            listener.onParameterChange((StandardEnvironmentalReverb) effect, status, param, value);
        }
    }

    private static void throwUseIEnvironmentalReverbVersionMethod() {
        throw new IllegalStateException(
                "This method is not supported, please use IEnvironmentalReverb version");
    }

    // === Fix unwanted behaviors ===
    private static final short ROOM_LEVEL_MIN = (short) -9000;
    private static final short ROOM_LEVEL_MAX = (short) 0;
    private static final short ROOM_HF_LEVEL_MIN = (short) -9000;
    private static final short ROOM_HF_LEVEL_MAX = (short) 0;
    private static final int DECAY_TIME_MIN = 100;
    /* Spec.: 20000, Actually(LVREV_MAX_T60): 7000 */
    private static final int DECAY_TIME_MAX = 7000;
    private static final short DECAY_HF_RATIO_MIN = (short) 100;
    private static final short DECAY_HF_RATIO_MAX = (short) 2000;
    /* Spec.: -9000, Actually: 0 (not implemented yet) */
    private static final short REFLECTIONS_LEVEL_MIN = (short) 0;
    /* Spec.: 1000, Actually: 0 (not implemented yet) */
    private static final short REFLECTIONS_LEVEL_MAX = (short) 0;
    /* Spec.: 0, Actually: 0 (not implemented yet) */
    private static final int REFLECTIONS_DELAY_MIN = 0;
    /* Spec.: 300, Actually: 0 (not implemented yet) */
    private static final int REFLECTIONS_DELAY_MAX = 0;
    private static final short REVERB_LEVEL_MIN = (short) -9000;
    private static final short REVERB_LEVEL_MAX = (short) 2000;
    /* Spec.: 0, Actually: 0 (not implemented yet) */
    private static final int REVERB_DELAY_MIN = 0;
    /* Spec.: 100, Actually: 0 (not implemented yet) */
    private static final int REVERB_DELAY_MAX = 0;
    private static final short DIFFUSION_MIN = (short) 0;
    private static final short DIFFUSION_MAX = (short) 1000;
    private static final short DENSITY_MIN = (short) 0;
    private static final short DENSITY_MAX = (short) 1000;

    @Override
    public void setRoomLevel(short room) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyRoomLevelParameterRange(room);
        super.setRoomLevel(room);
    }

    @Override
    public void setRoomHFLevel(short roomHF) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyRoomHFLevelParameterRange(roomHF);
        super.setRoomHFLevel(roomHF);
    }

    @Override
    public void setDecayTime(int decayTime) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyDecayTimeParameterRange(decayTime);
        super.setDecayTime(decayTime);
    }

    @Override
    public void setDecayHFRatio(short decayHFRatio) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyDecayHFRatioParameterRange(decayHFRatio);
        super.setDecayHFRatio(decayHFRatio);
    }

    @Override
    public void setReflectionsLevel(short reflectionsLevel) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyReflectionsLevelParameterRange(reflectionsLevel);
        super.setReflectionsLevel(reflectionsLevel);
    }

    @Override
    public void setReflectionsDelay(int reflectionsDelay) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyReflectionsDelayParameterRange(reflectionsDelay);
        super.setReflectionsDelay(reflectionsDelay);
    }

    @Override
    public void setReverbLevel(short reverbLevel) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyReverbLevelParameterRange(reverbLevel);
        super.setReverbLevel(reverbLevel);
    }

    @Override
    public void setReverbDelay(int reverbDelay) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyReverbDelayParameterRange(reverbDelay);
        super.setReverbDelay(reverbDelay);
    }

    @Override
    public void setDiffusion(short diffusion) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyDiffusionParameterRange(diffusion);
        super.setDiffusion(diffusion);
    }

    @Override
    public void setDensity(short density) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyDensityParameterRange(density);
        super.setDensity(density);
    }

    private static void verifyRoomLevelParameterRange(short roomLevel) {
        if (!(roomLevel >= ROOM_LEVEL_MIN && roomLevel <= ROOM_LEVEL_MAX))
            throw new IllegalArgumentException("bad parameter value: roomLevel = " + roomLevel);
    }

    private static void verifyRoomHFLevelParameterRange(short roomHFLevel) {
        if (!(roomHFLevel >= ROOM_HF_LEVEL_MIN && roomHFLevel <= ROOM_HF_LEVEL_MAX))
            throw new IllegalArgumentException("bad parameter value: roomHFLevel = " + roomHFLevel);
    }

    private static void verifyDecayTimeParameterRange(int decayTime) {
        if (!(decayTime >= DECAY_TIME_MIN && decayTime <= DECAY_TIME_MAX))
            throw new IllegalArgumentException("bad parameter value: decayTime = " + decayTime);
    }

    private static void verifyDecayHFRatioParameterRange(short decayHFRatio) {
        if (!(decayHFRatio >= DECAY_HF_RATIO_MIN && decayHFRatio <= DECAY_HF_RATIO_MAX))
            throw new IllegalArgumentException("bad parameter value: decayHFRatio = "
                    + decayHFRatio);
    }

    private static void verifyReflectionsLevelParameterRange(short reflectionsLevel) {
        if (!(reflectionsLevel >= REFLECTIONS_LEVEL_MIN && reflectionsLevel <= REFLECTIONS_LEVEL_MAX))
            throw new IllegalArgumentException("bad parameter value: reflectionsLevel = "
                    + reflectionsLevel);
    }

    private static void verifyReflectionsDelayParameterRange(int reflectionsDelay) {
        if (!(reflectionsDelay >= REFLECTIONS_DELAY_MIN && reflectionsDelay <= REFLECTIONS_DELAY_MAX))
            throw new IllegalArgumentException("bad parameter value: reflectionsDelay = "
                    + reflectionsDelay);
    }

    private static void verifyReverbLevelParameterRange(short reverbLevel) {
        if (!(reverbLevel >= REVERB_LEVEL_MIN && reverbLevel <= REVERB_LEVEL_MAX))
            throw new IllegalArgumentException("bad parameter value: reverbLevel = " + reverbLevel);
    }

    private static void verifyReverbDelayParameterRange(int reverbDelay) {
        if (!(reverbDelay >= REVERB_DELAY_MIN && reverbDelay <= REVERB_DELAY_MAX))
            throw new IllegalArgumentException("bad parameter value: reverbDelay = " + reverbDelay);
    }

    private static void verifyDiffusionParameterRange(short diffusion) {
        if (!(diffusion >= DIFFUSION_MIN && diffusion <= DIFFUSION_MAX))
            throw new IllegalArgumentException("bad parameter value: diffusion = " + diffusion);
    }

    private static void verifyDensityParameterRange(short density) {
        if (!(density >= DENSITY_MIN && density <= DENSITY_MAX))
            throw new IllegalArgumentException("bad parameter value: density = " + density);
    }

    private static void verifySettings(IEnvironmentalReverb.Settings settings) {
        if (settings == null)
            throw new IllegalArgumentException("The parameter 'settings' is null");
        verifyRoomLevelParameterRange(settings.roomLevel);
        verifyRoomHFLevelParameterRange(settings.roomHFLevel);
        verifyDecayTimeParameterRange(settings.decayTime);
        verifyDecayHFRatioParameterRange(settings.decayHFRatio);
        verifyReflectionsLevelParameterRange(settings.reflectionsLevel);
        verifyReflectionsDelayParameterRange(settings.reflectionsDelay);
        verifyReverbLevelParameterRange(settings.reverbLevel);
        verifyReverbDelayParameterRange(settings.reverbDelay);
        verifyDiffusionParameterRange(settings.diffusion);
        verifyDensityParameterRange(settings.density);
    }
    // ==============================
}
