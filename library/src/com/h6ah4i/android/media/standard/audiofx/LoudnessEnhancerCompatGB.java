
package com.h6ah4i.android.media.standard.audiofx;

import android.media.audiofx.AudioEffect;

class LoudnessEnhancerCompatGB extends LoudnessEnhancerCompatBase {

    @Override
    public AudioEffect create(int audioSession) {
        return null;
    }

    @Override
    public boolean isAvailable() {
        return false;
    }

    @Override
    public float getTargetGain(AudioEffect effect) {
        return (float) LOUDNESS_ENHANCER_DEFAULT_TARGET_GAIN_MB;
    }

    @Override
    public void setTargetGain(AudioEffect effect, int gainmB) {
    }

}
