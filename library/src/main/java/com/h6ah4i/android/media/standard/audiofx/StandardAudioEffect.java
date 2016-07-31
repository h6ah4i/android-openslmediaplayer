
package com.h6ah4i.android.media.standard.audiofx;

import java.lang.ref.WeakReference;

import android.media.audiofx.AudioEffect;

import com.h6ah4i.android.media.IReleasable;
import com.h6ah4i.android.media.audiofx.IAudioEffect;

class StandardAudioEffect implements IAudioEffect {

    private AudioEffect mAudioEffect;

    public StandardAudioEffect(AudioEffect audioEffect) {
        mAudioEffect = audioEffect;
    }

    @Override
    public int setEnabled(boolean enabled) throws IllegalStateException {
        checkIsNotReleased();
        return mAudioEffect.setEnabled(enabled);
    }

    @Override
    public boolean getEnabled() throws IllegalStateException {
        checkIsNotReleased();
        return mAudioEffect.getEnabled();
    }

    @Override
    public int getId() throws IllegalStateException {
        checkIsNotReleased();
        return mAudioEffect.getId();
    }

    @Override
    public boolean hasControl() throws IllegalStateException {
        checkIsNotReleased();
        return mAudioEffect.hasControl();
    }

    @Override
    public void release() {
        if (mAudioEffect != null) {
            mAudioEffect.release();
            mAudioEffect = null;
        }
    }

    @Override
    public void setControlStatusListener(IAudioEffect.OnControlStatusChangeListener listener)
            throws IllegalStateException {
        checkIsNotReleased();
        mAudioEffect.setControlStatusListener(StandardAudioEffect.wrap(this, listener));
    }

    @Override
    public void setEnableStatusListener(IAudioEffect.OnEnableStatusChangeListener listener)
            throws IllegalStateException {
        checkIsNotReleased();
        mAudioEffect.setEnableStatusListener(StandardAudioEffect.wrap(this, listener));
    }

    protected AudioEffect getAudioEffect() {
        return mAudioEffect;
    }

    protected void checkIsNotReleased() {
        checkIsNotReleased(null);
    }

    protected void checkIsNotReleased(String methodName) {
        if (mAudioEffect == null) {
            if (methodName == null) {
                throw new IllegalStateException("Audio effect instance has already been released");
            } else {
                throw new IllegalStateException("Audio effect instance has already been released. ; method = " + methodName);
            }
        }
    }

    /*package*/ static AudioEffect.OnControlStatusChangeListener wrap(
            IAudioEffect effect, IAudioEffect.OnControlStatusChangeListener listener) {
        return (listener != null)
                ? new OnControlStatusChangeListenerWrapper(effect, listener)
                : null;
    }

    /*package*/ static AudioEffect.OnEnableStatusChangeListener wrap(
            IAudioEffect effect, IAudioEffect.OnEnableStatusChangeListener listener) {
        return (listener != null)
                ? new OnEnableStatusChangeListenerWrapper(effect, listener)
                : null;
    }

    private static class OnControlStatusChangeListenerWrapper implements
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

    private static class OnEnableStatusChangeListenerWrapper implements
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

    protected void workaroundPrevReleaseSync() {
        if (getAudioEffect() != null) {
            try {
                // My experiment result says 1 millisecond is enough, but adding more to keep safety...
                Thread.sleep(5);
            } catch (InterruptedException e) {
            }
        }
    }
}
