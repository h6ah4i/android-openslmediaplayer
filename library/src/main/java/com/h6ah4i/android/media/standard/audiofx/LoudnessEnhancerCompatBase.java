
package com.h6ah4i.android.media.standard.audiofx;

import android.media.audiofx.AudioEffect;

abstract class LoudnessEnhancerCompatBase {
    // from effect_loudnessenhancer.h
    public static final int LOUDNESS_ENHANCER_DEFAULT_TARGET_GAIN_MB = 0;

    abstract AudioEffect create(int audioSession);

    abstract boolean isAvailable();

    abstract float getTargetGain(AudioEffect effect);

    abstract void setTargetGain(AudioEffect effect, int gainmB);
}
