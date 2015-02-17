
package com.h6ah4i.android.media.standard.audiofx;

import android.annotation.TargetApi;
import android.media.audiofx.AudioEffect;
import android.media.audiofx.LoudnessEnhancer;
import android.os.Build;
import android.util.Log;

@TargetApi(Build.VERSION_CODES.KITKAT)
class LoudnessEnhancerCompatKitKat extends LoudnessEnhancerCompatBase {
    private static final String TAG = "LoudEnhancerCompatKK";

    @Override
    public AudioEffect create(int audioSession) {
        try {
            return new LoudnessEnhancer(audioSession);
        } catch (RuntimeException e) {
            // NOTE: some devices doesn't support LoudnessEnhancer class and may throw an exception
            // (ME176C throws IllegalArgumentException)
            Log.w(TAG, "Failed to instantiate loudness enhancer class", e);
        }
        return null;
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public float getTargetGain(AudioEffect effect) {
        return ((LoudnessEnhancer) effect).getTargetGain();
    }

    @Override
    public void setTargetGain(AudioEffect effect, int gainmB) {
        ((LoudnessEnhancer) effect).setTargetGain(gainmB);
    }
}
