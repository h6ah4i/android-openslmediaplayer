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


package com.h6ah4i.android.media.standard;

import java.io.IOException;
import java.lang.ref.WeakReference;

import android.os.Build;
import android.util.Log;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.compat.AudioAttributes;
import com.h6ah4i.android.media.compat.MediaPlayerCompat;

public class StandardMediaPlayer extends android.media.MediaPlayer implements IBasicMediaPlayer {
    private static final String TAG = "StandardMediaPlayer";

    private static final boolean LOCAL_LOGV = false;

    private WeakReference<StandardMediaPlayer> mNextMediaPlayerRef;
    private IBasicMediaPlayer.OnCompletionListener mUserOnCompletionListener;
    private IBasicMediaPlayer.OnPreparedListener mUserOnPreparedListener;
    private IBasicMediaPlayer.OnSeekCompleteListener mUserOnSeekCompleteListener;
    private IBasicMediaPlayer.OnBufferingUpdateListener mUserOnBufferingUpdateListener;
    private IBasicMediaPlayer.OnInfoListener mUserOnInfoListener;
    private IBasicMediaPlayer.OnErrorListener mUserOnErrorListener;
    private boolean mIsPrepared;
    private boolean mIsLooping;
    private boolean mIsIdleOrInitializedState = true;
    private boolean mReleased;
    private int mDuration;

    private android.media.MediaPlayer.OnCompletionListener mHookOnCompletionListener = new android.media.MediaPlayer.OnCompletionListener() {
        @Override
        public void onCompletion(android.media.MediaPlayer mp) {
            StandardMediaPlayer.this.handleOnCompletion((StandardMediaPlayer) mp);
        }
    };

    private android.media.MediaPlayer.OnPreparedListener mHookOnPreparedListener = new android.media.MediaPlayer.OnPreparedListener() {
        @Override
        public void onPrepared(android.media.MediaPlayer mp) {
            StandardMediaPlayer.this.handleOnPrepared((StandardMediaPlayer) mp);
        }
    };

    private android.media.MediaPlayer.OnSeekCompleteListener mHookOnSeekCompeleteListener = new android.media.MediaPlayer.OnSeekCompleteListener() {
        @Override
        public void onSeekComplete(android.media.MediaPlayer mp) {
            StandardMediaPlayer.this.handleOnSeekComplete((StandardMediaPlayer) mp);
        }
    };

    private android.media.MediaPlayer.OnBufferingUpdateListener mHookOnBufferingUpdateListener = new android.media.MediaPlayer.OnBufferingUpdateListener() {
        @Override
        public void onBufferingUpdate(android.media.MediaPlayer mp, int percent) {
            StandardMediaPlayer.this.handleOnBufferingUpdate((StandardMediaPlayer) mp, percent);
        }
    };

    private android.media.MediaPlayer.OnInfoListener mHookOnInfoListner = new android.media.MediaPlayer.OnInfoListener() {
        @Override
        public boolean onInfo(android.media.MediaPlayer mp, int what, int extra) {
            return StandardMediaPlayer.this.handleOnInfo((StandardMediaPlayer) mp, what, extra);
        }
    };

    private android.media.MediaPlayer.OnErrorListener mHookOnErrorListener = new android.media.MediaPlayer.OnErrorListener() {
        @Override
        public boolean onError(android.media.MediaPlayer mp, int what, int extra) {
            return StandardMediaPlayer.this.handleOnError((StandardMediaPlayer) mp, what, extra);
        }
    };

    public StandardMediaPlayer() {
        super();

        super.setOnCompletionListener(mHookOnCompletionListener);
        super.setOnPreparedListener(mHookOnPreparedListener);
        super.setOnSeekCompleteListener(mHookOnSeekCompeleteListener);
        super.setOnBufferingUpdateListener(mHookOnBufferingUpdateListener);
        super.setOnInfoListener(mHookOnInfoListner);
        super.setOnErrorListener(mHookOnErrorListener);

        mNextMediaPlayerRef = new WeakReference<StandardMediaPlayer>(null);
    }

    @Override
    public void release() {
        mIsPrepared = false;

        super.setOnCompletionListener(null);
        super.setOnPreparedListener(null);
        super.setOnSeekCompleteListener(null);
        super.setOnBufferingUpdateListener(null);
        super.setOnInfoListener(null);
        super.setOnErrorListener(null);

        mHookOnCompletionListener = null;
        mHookOnPreparedListener = null;
        mHookOnSeekCompeleteListener = null;
        mHookOnBufferingUpdateListener = null;
        mHookOnInfoListner = null;
        mHookOnErrorListener = null;

        mUserOnCompletionListener = null;
        mUserOnPreparedListener = null;
        mUserOnSeekCompleteListener = null;
        mUserOnInfoListener = null;
        mUserOnErrorListener = null;

        mNextMediaPlayerRef.clear();

        mReleased = true;

        super.release();
    }

    @Override
    public void reset() {
        super.reset();
        mIsPrepared = false;
        mIsIdleOrInitializedState = true;
        mIsLooping = false;
    }

    @Override
    public void stop() throws IllegalStateException {
        super.stop();
        mIsPrepared = false;
    }

    @Override
    public void prepare() throws IOException, IllegalStateException {
        super.prepare();
        mDuration = super.getDuration();
        mIsPrepared = true;
        applyLoopingState();
    }

    @Override
    public int getDuration() {
        int duration = super.getDuration();
        if (!mIsPrepared) {
            // [Workaround]
            // super.getDuration() may returns invalid (uninitialized ?) value
            // before calling prepare() method
            duration = 0;
        }
        return duration;
    }

    @Override
    public int getCurrentPosition() {
        int position = super.getCurrentPosition();
        if (!mIsPrepared) {
            // [Workaround]
            // super.getCurrentPosition() may returns invalid (uninitialized ?)
            // before calling prepare() method
            position = 0;
        }

        // [Workaround]
        // super.getCurrentPosition() may returns invalid value
        return Math.min(position, mDuration);
    }

    @Override
    public void setOnCompletionListener(IBasicMediaPlayer.OnCompletionListener listener) {
        mUserOnCompletionListener = listener;
    }

    @Override
    public void setOnPreparedListener(IBasicMediaPlayer.OnPreparedListener listener) {
        mUserOnPreparedListener = listener;
    }

    @Override
    public void setOnSeekCompleteListener(IBasicMediaPlayer.OnSeekCompleteListener listener) {
        mUserOnSeekCompleteListener = listener;
    }

    @Override
    public void setOnBufferingUpdateListener(IBasicMediaPlayer.OnBufferingUpdateListener listener) {
        mUserOnBufferingUpdateListener = listener;
    }

    @Override
    public void setOnInfoListener(IBasicMediaPlayer.OnInfoListener listener) {
        mUserOnInfoListener = listener;
    }

    @Override
    public void setOnErrorListener(IBasicMediaPlayer.OnErrorListener listener) {
        mUserOnErrorListener = listener;
    }

    @Override
    public void setOnCompletionListener(android.media.MediaPlayer.OnCompletionListener listener) {
        throwUseIMediaPlayerVersionMethod();
    }

    @Override
    public void setOnPreparedListener(android.media.MediaPlayer.OnPreparedListener listener) {
        throwUseIMediaPlayerVersionMethod();
    }

    @Override
    public void setOnSeekCompleteListener(android.media.MediaPlayer.OnSeekCompleteListener listener) {
        throwUseIMediaPlayerVersionMethod();
    }

    @Override
    public void setOnBufferingUpdateListener(
            android.media.MediaPlayer.OnBufferingUpdateListener listener) {
        throwUseIMediaPlayerVersionMethod();
    }

    @Override
    public void setOnInfoListener(android.media.MediaPlayer.OnInfoListener listener) {
        throwUseIMediaPlayerVersionMethod();
    }

    @Override
    public void setOnErrorListener(android.media.MediaPlayer.OnErrorListener listener) {
        throwUseIMediaPlayerVersionMethod();
    }

    @Override
    public void setAuxEffectSendLevel(float level) {
        // [Workaround]
        // Hack: Can't handle some invalid values
        if (Float.isNaN(level) || level < 0.0f)
            level = Float.POSITIVE_INFINITY;

        super.setAuxEffectSendLevel(level);
    }

    @Override
    public void setNextMediaPlayerCompat(IBasicMediaPlayer next) {
        if (next != null && !(next instanceof StandardMediaPlayer)) {
            throw new IllegalArgumentException("Not StandardMediaPlayer instance");
        }
        
        setNextMediaPlayer((StandardMediaPlayer) next);
    }

    public void setNextMediaPlayer(StandardMediaPlayer next) {
        if (next != null && !(next instanceof StandardMediaPlayer)) {
            throw new IllegalArgumentException("Not StandardMediaPlayer instance");
        }

        if (next == this) {
            throw new IllegalArgumentException();
        }

        mNextMediaPlayerRef = new WeakReference<StandardMediaPlayer>(next);

        if (MediaPlayerCompat.supportsSetNextMediaPlayer()) {
            MediaPlayerCompat.setNextMediaPlayer(this, next);
        }
    }

    @Override
    public void setAudioAttributes(AudioAttributes attributes) {
        if (attributes == null) {
            throw new IllegalArgumentException();
        }

        if (MediaPlayerCompat.supportsSetAudioAttributes()) {
            MediaPlayerCompat.setAudioAttributes(this, attributes);
        } else {
            if (mReleased) {
                throw new IllegalStateException(
                        "setAudioAttributes() called for released player instance");
            }

            if (LOCAL_LOGV) {
                Log.v(TAG, "setAudioAttributes() is not supported, just ignored.");
            }
        }
    }

    @Override
    public void setLooping(boolean looping) {
        // [Workaround]
        // Stock MediaPlayer doesn't support setLooping() method in idle state

        if (!mReleased && mIsIdleOrInitializedState) {
            mIsLooping = looping;
        } else {
            super.setLooping(looping);
            mIsLooping = looping;
        }
    }

    @Override
    public boolean isLooping() {
        if (!mReleased && mIsIdleOrInitializedState) {
            return mIsLooping;
        } else {
            return super.isLooping();
        }
    }

    private void applyLoopingState() {
        mIsIdleOrInitializedState = false;
        super.setLooping(mIsLooping);
    }

    protected void handleOnCompletion(StandardMediaPlayer mp) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            // [Workaround]
            // Since Android 5.0, OnCompletion callback is fired in wrong state
            // https://code.google.com/p/android/issues/detail?id=77860

            if (mIsLooping) {
                return;
            } else if (mNextMediaPlayerRef.get() != null) {
                return;
            }
        }

        // Emulate setNextMediaPlayer() behavior
        if (!MediaPlayerCompat.supportsSetNextMediaPlayer()) {
            StandardMediaPlayer nextmp = mNextMediaPlayerRef.get();
            try {
                if (nextmp != null) {
                    nextmp.start();
                }
            } catch (Exception e) {
                // eat all exceptions here
                Log.e(TAG, "An exception occurred in handleOnCompletion()", e);
            }
        }

        if (mUserOnCompletionListener != null) {
            mUserOnCompletionListener.onCompletion(mp);
        }
    }

    protected void handleOnPrepared(StandardMediaPlayer mp) {
        mDuration = super.getDuration();
        mIsPrepared = true;
        applyLoopingState();
        if (mUserOnPreparedListener != null) {
            mUserOnPreparedListener.onPrepared(mp);
        }
    }

    protected void handleOnSeekComplete(StandardMediaPlayer mp) {
        if (mUserOnSeekCompleteListener != null) {
            mUserOnSeekCompleteListener.onSeekComplete(mp);
        }
    }

    protected void handleOnBufferingUpdate(StandardMediaPlayer mp, int percent) {
        if (mUserOnBufferingUpdateListener != null) {
            mUserOnBufferingUpdateListener.onBufferingUpdate(mp, percent);
        }
    }

    protected boolean handleOnError(StandardMediaPlayer mp, int what, int extra) {
        if (mUserOnErrorListener != null) {
            return mUserOnErrorListener.onError(mp, what, extra);
        }

        return false;
    }

    protected boolean handleOnInfo(StandardMediaPlayer mp, int what, int extra) {
        if (mUserOnInfoListener != null) {
            return mUserOnInfoListener.onInfo(mp, what, extra);
        }

        return false;
    }

    private static void throwUseIMediaPlayerVersionMethod() {
        throw new IllegalStateException(
                "This method is not supported, please use IMediaPlayer version");
    }
}
