
package com.h6ah4i.android.media.standard.audiofx;

import android.media.audiofx.AudioEffect;
import android.os.Build;

import com.h6ah4i.android.media.audiofx.IAudioEffect;
import com.h6ah4i.android.media.audiofx.ILoudnessEnhancer;

public class StandardLoudnessEnhancer implements ILoudnessEnhancer {
    private static final LoudnessEnhancerCompatBase S_IMPL;

    static {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            S_IMPL = new LoudnessEnhancerCompatKitKat();
        } else {
            S_IMPL = new LoudnessEnhancerCompatGB();
        }
    }

    private AudioEffect mInstance;

    private StandardLoudnessEnhancer(AudioEffect instance) {
        mInstance = instance;
    }

    public static StandardLoudnessEnhancer craete(int audioSession) {
        final AudioEffect instance = S_IMPL.create(audioSession);
        if (instance != null) {
            return new StandardLoudnessEnhancer(instance);
        } else {
            return null;
        }
    }

    public static boolean isAvailable() {
        return S_IMPL.isAvailable();
    }

    @Override
    public boolean getEnabled() throws IllegalStateException {
        checkState("getEnabled()");
        return mInstance.getEnabled();
    }

    @Override
    public int getId() throws IllegalStateException {
        checkState("getId()");
        return mInstance.getId();
    }

    @Override
    public boolean hasControl() throws IllegalStateException {
        checkState("hasControl()");
        return mInstance.hasControl();
    }

    @Override
    public void release() throws IllegalStateException {
        if (mInstance != null) {
            mInstance.release();
            mInstance = null;
        }
    }

    @Override
    public void setControlStatusListener(IAudioEffect.OnControlStatusChangeListener listener)
            throws IllegalStateException {
        checkState("setControlStatusListener()");
        mInstance.setControlStatusListener(StandardAudioEffect.wrap(this, listener));
    }

    @Override
    public void setEnableStatusListener(IAudioEffect.OnEnableStatusChangeListener listener)
            throws IllegalStateException {
        checkState("setEnableStatusListener()");
        mInstance.setEnableStatusListener(StandardAudioEffect.wrap(this, listener));
    }

    @Override
    public int setEnabled(boolean enabled) throws IllegalStateException {
        checkState("setEnabled()");
        return mInstance.setEnabled(enabled);
    }

    @Override
    public float getTargetGain() throws UnsupportedOperationException, IllegalStateException {
        checkState("getTargetGain()");
        return S_IMPL.getTargetGain(mInstance);
    }

    @Override
    public void setTargetGain(int gainmB) throws IllegalStateException, IllegalArgumentException {
        checkState("setTargetGain()");
        verifyTargetGainmBParameterRange(gainmB);
        S_IMPL.setTargetGain(mInstance, gainmB);
    }

    @Override
    public void setPropertiesCompat(Settings settings) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        checkState("setPropertiesCompat()");
        verifySettings(settings);
        setTargetGain(settings.targetGainmB);
    }

    @Override
    public Settings getPropertiesCompat() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkState("getPropertiesCompat()");
        Settings settings = new Settings();
        
        settings.targetGainmB = (int) getTargetGain();
        
        return settings;
    }

    // ==============================

    private void checkState(String methodName) throws IllegalStateException {
        if (mInstance == null) {
            throw (new IllegalStateException(methodName
                    + " called on uninitialized AudioEffect."));
        }
    }

    private static void verifySettings(ILoudnessEnhancer.Settings settings) {
        if (settings == null)
            throw new IllegalArgumentException("The parameter 'settings' is null");
        verifyTargetGainmBParameterRange(settings.targetGainmB);
    }

    private static void verifyTargetGainmBParameterRange(int targetGainmB) {
        final int kMaxTargetGainmB = 2000;  // 20 dB  (according to frameworks/av/media/libeffects/loudness/dsp/core/dynamic_range_complession.h) 
        if (targetGainmB > kMaxTargetGainmB) {
            throw new IllegalArgumentException("targetGainmB is too large");
        }
    }
    // ==============================
}
