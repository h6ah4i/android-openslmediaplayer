
package com.h6ah4i.android.media.standard.audiofx;

import java.lang.ref.WeakReference;

import android.media.audiofx.AudioEffect;

import com.h6ah4i.android.media.IReleasable;
import com.h6ah4i.android.media.audiofx.IAudioEffect;

class StandardAudioEffect {

    private StandardAudioEffect() {
    }

    public static AudioEffect.OnControlStatusChangeListener wrap(
            IAudioEffect effect, IAudioEffect.OnControlStatusChangeListener listener) {
        return (listener != null)
                ? new OnControlStatusChangeListenerWrapper(effect, listener)
                : null;
    }

    public static AudioEffect.OnEnableStatusChangeListener wrap(
            IAudioEffect effect, IAudioEffect.OnEnableStatusChangeListener listener) {
        return (listener != null)
                ? new OnEnableStatusChangeListenerWrapper(effect, listener)
                : null;
    }

    public static class OnControlStatusChangeListenerWrapper implements
            AudioEffect.OnControlStatusChangeListener, IReleasable {

        private WeakReference<IAudioEffect> mRefEffect;
        private IAudioEffect.OnControlStatusChangeListener mListener;

        public OnControlStatusChangeListenerWrapper(
                IAudioEffect effect,
                IAudioEffect.OnControlStatusChangeListener listener) {
            mRefEffect = new WeakReference<IAudioEffect>(effect);
        }

        @Override
        public void onControlStatusChange(AudioEffect effect, boolean controlGranted) {
            final IAudioEffect ieffect = mRefEffect.get();
            final IAudioEffect.OnControlStatusChangeListener listener = mListener;

            if (ieffect != null && listener != null) {
                listener.onControlStatusChange(ieffect, controlGranted);
            }
        }

        @Override
        public void release() {
            mRefEffect.clear();
            mListener = null;
        }
    }

    public static class OnEnableStatusChangeListenerWrapper implements
            AudioEffect.OnEnableStatusChangeListener, IReleasable {

        private WeakReference<IAudioEffect> mRefEffect;
        private IAudioEffect.OnEnableStatusChangeListener mListener;

        public OnEnableStatusChangeListenerWrapper(
                IAudioEffect effect,
                IAudioEffect.OnEnableStatusChangeListener listener) {
            mRefEffect = new WeakReference<IAudioEffect>(effect);
        }

        @Override
        public void onEnableStatusChange(AudioEffect effect, boolean enabled) {
            final IAudioEffect ieffect = mRefEffect.get();
            final IAudioEffect.OnEnableStatusChangeListener listener = mListener;

            if (ieffect != null && listener != null) {
                listener.onEnableStatusChange(ieffect, enabled);
            }
        }

        @Override
        public void release() {
            mRefEffect.clear();
            mListener = null;
        }
    }
}
