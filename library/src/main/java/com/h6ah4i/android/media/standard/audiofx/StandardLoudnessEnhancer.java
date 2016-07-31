
package com.h6ah4i.android.media.standard.audiofx;

import android.media.audiofx.AudioEffect;
import android.os.Build;

import com.h6ah4i.android.media.audiofx.ILoudnessEnhancer;

public class StandardLoudnessEnhancer extends StandardAudioEffect implements ILoudnessEnhancer {
    private static final LoudnessEnhancerCompatBase S_IMPL;

    static {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            S_IMPL = new LoudnessEnhancerCompatKitKat();
        } else {
            S_IMPL = new LoudnessEnhancerCompatGB();
        }
    }

    private static AudioEffect createInstance(int audioSession) throws UnsupportedOperationException {
        AudioEffect instance = S_IMPL.create(audioSession);
        if (instance == null) {
            throw new UnsupportedOperationException("LoudnessEnhancer is not supported");
        }
        return instance;
    }

    public static boolean isAvailable() {
        return S_IMPL.isAvailable();
    }

    public StandardLoudnessEnhancer(int audioSession) throws UnsupportedOperationException, IllegalStateException {
        super (createInstance(audioSession));
    }


    /**
     * Get underlying LoudnessEnhancer instance.
     *
     * @return underlying LoudnessEnhancer instance.
     */
    public AudioEffect getLoudnessEnhancer() {
        // LoudnessEnhancer class has been introduced since API level 19,
        // so explicit casting should not be applied.
        return super.getAudioEffect();
    }

    @Override
    public float getTargetGain() throws UnsupportedOperationException, IllegalStateException {
        checkIsNotReleased("getTargetGain()");
        return S_IMPL.getTargetGain(super.getAudioEffect());
    }

    @Override
    public void setTargetGain(int gainmB) throws IllegalStateException, IllegalArgumentException {
        checkIsNotReleased("setTargetGain()");
        verifyTargetGainmBParameterRange(gainmB);
        S_IMPL.setTargetGain(super.getAudioEffect(), gainmB);
    }

    @Override
    public void setProperties(Settings settings) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        checkIsNotReleased("setProperties()");
        verifySettings(settings);
        setTargetGain(settings.targetGainmB);
    }

    @Override
    public Settings getProperties() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        checkIsNotReleased("getProperties()");
        Settings settings = new Settings();
        
        settings.targetGainmB = (int) getTargetGain();
        
        return settings;
    }

    // ==============================

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
