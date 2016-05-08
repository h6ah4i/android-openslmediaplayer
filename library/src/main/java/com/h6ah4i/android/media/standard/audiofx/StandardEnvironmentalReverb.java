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

import android.media.audiofx.EnvironmentalReverb;

import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.utils.AudioEffectSettingsConverter;

public class StandardEnvironmentalReverb extends StandardAudioEffect implements IEnvironmentalReverb {
    public static final String TAG = "StdEnvReverb";

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
        super(new EnvironmentalReverb(priority, audioSession));
        getEnvironmentalReverb().setParameterListener(mOnParameterChangeListener);
    }

    /**
     * Get underlying EnvironmentalReverb instance.
     *
     * @return underlying EnvironmentalReverb instance.
     */
    public EnvironmentalReverb getEnvironmentalReverb() {
        return (EnvironmentalReverb) getAudioEffect();
    }

    @Override
    public int setEnabled(boolean enabled) throws IllegalStateException {
        final int result = super.setEnabled(enabled);

        if (result == SUCCESS) {
            // workarounds
            workaroundAfterSetEnabled(enabled);
        }

        return result;
    }

    @Override
    public void release() {
        workaroundPrevReleaseSync();
        super.release();
        mOnParameterChangeListener = null;
        mUserOnParameterChangeListener = null;
    }

    @Override
    public IEnvironmentalReverb.Settings getProperties() throws IllegalStateException,
            IllegalArgumentException,
            UnsupportedOperationException {
        checkIsNotReleased("getProperties()");
        // NOTE: EnvironmentalReverb.getProperties() method does not work properly...
        return AudioEffectSettingsConverter.convert(getPropertiesCompat(getEnvironmentalReverb()));
    }

    @Override
    public void setProperties(IEnvironmentalReverb.Settings settings)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        verifySettings(settings);

        checkIsNotReleased("setProperties()");

        // NOTE: EnvironmentalReverb.getProperties() method does not work properly...
        setPropertiesCompat(getEnvironmentalReverb(), AudioEffectSettingsConverter.convert(settings));
    }

    @Override
    public short getDiffusion() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getDiffusion()");
        return getEnvironmentalReverb().getDiffusion();
    }

    @Override
    public void setDiffusion(short diffusion) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyDiffusionParameterRange(diffusion);
        checkIsNotReleased("setDiffusion()");
        getEnvironmentalReverb().setDiffusion(diffusion);
    }

    @Override
    public short getDensity() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getDensity()");
        return getEnvironmentalReverb().getDensity();
    }

    @Override
    public void setDensity(short density) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyDensityParameterRange(density);
        checkIsNotReleased("setDensity()");
        getEnvironmentalReverb().setDensity(density);
    }

    @Override
    public short getDecayHFRatio() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getDecayHFRatio()");
        return getEnvironmentalReverb().getDecayHFRatio();
    }

    @Override
    public void setDecayHFRatio(short decayHFRatio) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyDecayHFRatioParameterRange(decayHFRatio);
        checkIsNotReleased("setDecayHFRatio()");
        getEnvironmentalReverb().setDecayHFRatio(decayHFRatio);
    }

    @Override
    public int getDecayTime() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getDecayTime()");
        return getEnvironmentalReverb().getDecayTime();
    }

    @Override
    public void setDecayTime(int decayTime) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyDecayTimeParameterRange(decayTime);
        checkIsNotReleased("setDecayTime()");
        getEnvironmentalReverb().setDecayTime(decayTime);
    }

    @Override
    public short getRoomLevel() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getRoomLevel()");
        return getEnvironmentalReverb().getRoomLevel();
    }

    @Override
    public void setRoomLevel(short room) throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        verifyRoomLevelParameterRange(room);
        checkIsNotReleased("setRoomLevel()");
        getEnvironmentalReverb().setRoomLevel(room);
    }

    @Override
    public short getRoomHFLevel() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getRoomHFLevel()");
        return getEnvironmentalReverb().getRoomHFLevel();
    }

    @Override
    public void setRoomHFLevel(short roomHF) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyRoomHFLevelParameterRange(roomHF);
        checkIsNotReleased("setRoomHFLevel()");
        getEnvironmentalReverb().setRoomHFLevel(roomHF);
    }

    @Override
    public short getReverbLevel() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getReverbLevel()");
        return getEnvironmentalReverb().getReverbLevel();
    }

    @Override
    public void setReverbLevel(short reverbLevel) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyReverbLevelParameterRange(reverbLevel);
        checkIsNotReleased("setReverbLevel()");
        getEnvironmentalReverb().setReverbLevel(reverbLevel);
    }

    @Override
    public int getReverbDelay() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getReverbDelay()");
        return getEnvironmentalReverb().getReverbDelay();
    }

    @Override
    public void setReverbDelay(int reverbDelay) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyReverbDelayParameterRange(reverbDelay);
        checkIsNotReleased("setReverbDelay()");
        getEnvironmentalReverb().setReverbDelay(reverbDelay);
    }

    @Override
    public short getReflectionsLevel() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getReflectionsLevel()");
        return getEnvironmentalReverb().getReflectionsLevel();
    }

    @Override
    public void setReflectionsLevel(short reflectionsLevel) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyReflectionsLevelParameterRange(reflectionsLevel);
        checkIsNotReleased("setReflectionsLevel()");
        getEnvironmentalReverb().setReflectionsLevel(reflectionsLevel);
    }

    @Override
    public int getReflectionsDelay() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("getReflectionsDelay()");
        return getEnvironmentalReverb().getReflectionsDelay();
    }

    @Override
    public void setReflectionsDelay(int reflectionsDelay) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        verifyReflectionsDelayParameterRange(reflectionsDelay);
        checkIsNotReleased("setReflectionsDelay()");
        getEnvironmentalReverb().setReflectionsDelay(reflectionsDelay);
    }

    @Override
    public void setParameterListener(
            IEnvironmentalReverb.OnParameterChangeListener listener) {
        mUserOnParameterChangeListener = listener;
    }

    /* package */void onParameterChange(
            android.media.audiofx.EnvironmentalReverb effect,
            int status, int param, int value) {
        IEnvironmentalReverb.OnParameterChangeListener listener = null;

        listener = mUserOnParameterChangeListener;

        if (listener != null) {
            listener.onParameterChange(this, status, param, value);
        }
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

    private static EnvironmentalReverb.Settings getPropertiesCompat(EnvironmentalReverb reverb) {
        EnvironmentalReverb.Settings settings = new EnvironmentalReverb.Settings();

        settings.roomLevel = reverb.getRoomLevel();
        settings.roomHFLevel = reverb.getRoomHFLevel();
        settings.decayTime = reverb.getDecayTime();
        settings.decayHFRatio = reverb.getDecayHFRatio();
        settings.reflectionsLevel = reverb.getReflectionsLevel();
        settings.reflectionsDelay = reverb.getReflectionsDelay();
        settings.reverbLevel = reverb.getReverbLevel();
        settings.reverbDelay = reverb.getReverbDelay();
        settings.diffusion = reverb.getDiffusion();
        settings.density = reverb.getDensity();

        return settings;
    }

    private void setPropertiesCompat(EnvironmentalReverb reverb, EnvironmentalReverb.Settings settings) {
        reverb.setRoomLevel(settings.roomLevel);
        reverb.setRoomHFLevel(settings.roomHFLevel);
        reverb.setDecayTime(settings.decayTime);
        reverb.setDecayHFRatio(settings.decayHFRatio);
        reverb.setReflectionsLevel(settings.reflectionsLevel);
        reverb.setReflectionsDelay(settings.reflectionsDelay);
        reverb.setReverbLevel(settings.reverbLevel);
        reverb.setReverbDelay(settings.reverbDelay);
        reverb.setDiffusion(settings.diffusion);
        reverb.setDensity(settings.density);
    }

    private void workaroundAfterSetEnabled(boolean enabled) {
        if (enabled) {
            setProperties(getProperties());
        }
    }

    // ==============================
}
