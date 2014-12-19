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

import java.io.FileDescriptor;
import java.io.IOException;
import java.lang.ref.WeakReference;

import android.content.Context;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Build;
import android.util.Log;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.compat.AudioAttributes;
import com.h6ah4i.android.media.compat.MediaPlayerCompat;

public class StandardMediaPlayer implements IBasicMediaPlayer {
    private static final String TAG = "StandardMediaPlayer";

    private static final boolean LOCAL_LOGV = false;

    private MediaPlayer mPlayer;
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
    private boolean mUsingNuPlayer;

    private android.media.MediaPlayer.OnCompletionListener mHookOnCompletionListener = new android.media.MediaPlayer.OnCompletionListener() {
        @Override
        public void onCompletion(android.media.MediaPlayer mp) {
            StandardMediaPlayer.this.handleOnCompletion(mp);
        }
    };

    private android.media.MediaPlayer.OnPreparedListener mHookOnPreparedListener = new android.media.MediaPlayer.OnPreparedListener() {
        @Override
        public void onPrepared(android.media.MediaPlayer mp) {
            StandardMediaPlayer.this.handleOnPrepared(mp);
        }
    };

    private android.media.MediaPlayer.OnSeekCompleteListener mHookOnSeekCompeleteListener = new android.media.MediaPlayer.OnSeekCompleteListener() {
        @Override
        public void onSeekComplete(android.media.MediaPlayer mp) {
            StandardMediaPlayer.this.handleOnSeekComplete(mp);
        }
    };

    private android.media.MediaPlayer.OnBufferingUpdateListener mHookOnBufferingUpdateListener = new android.media.MediaPlayer.OnBufferingUpdateListener() {
        @Override
        public void onBufferingUpdate(android.media.MediaPlayer mp, int percent) {
            StandardMediaPlayer.this.handleOnBufferingUpdate(mp, percent);
        }
    };

    private android.media.MediaPlayer.OnInfoListener mHookOnInfoListner = new android.media.MediaPlayer.OnInfoListener() {
        @Override
        public boolean onInfo(android.media.MediaPlayer mp, int what, int extra) {
            return StandardMediaPlayer.this.handleOnInfo(mp, what, extra);
        }
    };

    private android.media.MediaPlayer.OnErrorListener mHookOnErrorListener = new android.media.MediaPlayer.OnErrorListener() {
        @Override
        public boolean onError(android.media.MediaPlayer mp, int what, int extra) {
            return StandardMediaPlayer.this.handleOnError(mp, what, extra);
        }
    };

    public StandardMediaPlayer() {
        super();

        mUsingNuPlayer = NuPlayerDetector.isUsingNuPlayer();

        mPlayer = new MediaPlayer();
        mPlayer.setOnCompletionListener(mHookOnCompletionListener);
        mPlayer.setOnPreparedListener(mHookOnPreparedListener);
        mPlayer.setOnSeekCompleteListener(mHookOnSeekCompeleteListener);
        mPlayer.setOnBufferingUpdateListener(mHookOnBufferingUpdateListener);
        mPlayer.setOnInfoListener(mHookOnInfoListner);
        mPlayer.setOnErrorListener(mHookOnErrorListener);

        mNextMediaPlayerRef = new WeakReference<StandardMediaPlayer>(null);
    }

    /**
     * Get underlying MediaPlayer instance.
     *
     * @return underlying MediaPlayer instance.
     */
    public MediaPlayer getMediaPlayer() {
        return mPlayer;
    }

    @Override
    public void release() {
        if (mPlayer != null) {
            mPlayer.setOnCompletionListener(null);
            mPlayer.setOnPreparedListener(null);
            mPlayer.setOnSeekCompleteListener(null);
            mPlayer.setOnBufferingUpdateListener(null);
            mPlayer.setOnInfoListener(null);
            mPlayer.setOnErrorListener(null);
            mPlayer.release();
        }

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

        mIsPrepared = false;
        mReleased = true;
    }

    @Override
    public void reset() {
        if (mPlayer != null) {
            mPlayer.reset();
        }
        mIsPrepared = false;
        mIsIdleOrInitializedState = true;
        mIsLooping = false;
    }

    @Override
    public int getAudioSessionId() {
        if (mPlayer == null) {
            return 0;
        }
        return mPlayer.getAudioSessionId();
    }

    @Override
    public void setAudioSessionId(int sessionId) throws IllegalArgumentException, IllegalStateException {
        checkIsNotReleased();

        mPlayer.setAudioSessionId(sessionId);
    }

    @Override
    public void start() throws IllegalStateException {
        checkIsNotReleased();

        mPlayer.start();
    }

    @Override
    public void stop() throws IllegalStateException {
        checkIsNotReleased();

        mPlayer.stop();
        mIsPrepared = false;
    }

    @Override
    public void setDataSource(Context context, Uri uri) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        try {
            mPlayer.setDataSource(context, uri);
        } catch (NullPointerException e) {
            // NOTE:
            // Maybe, passing a wrong context.
            // If running on InstrumentationTestCase, getInstrumentation().getTargetContext() method should be used.
            Log.w(TAG, "setDataSource()", e);
            throw new IllegalStateException(e);
        }
    }

    @Override
    public void setDataSource(FileDescriptor fd) throws IOException, IllegalArgumentException, IllegalStateException {
        checkIsNotReleased();

        mPlayer.setDataSource(fd);
    }

    @Override
    public void setDataSource(FileDescriptor fd, long offset, long length) throws IOException, IllegalArgumentException, IllegalStateException {
        checkIsNotReleased();

        mPlayer.setDataSource(fd, offset, length);
    }

    @Override
    public void setDataSource(String path) throws IOException, IllegalArgumentException, IllegalStateException {
        checkIsNotReleased();

        mPlayer.setDataSource(path);
    }

    @Override
    public void pause() throws IllegalStateException {
        checkIsNotReleased();

        mPlayer.pause();
    }

    @Override
    public void prepare() throws IOException, IllegalStateException {
        checkIsNotReleased();

        mPlayer.prepare();
        mDuration = mPlayer.getDuration();
        mIsPrepared = true;
        applyLoopingState();
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        checkIsNotReleased();

        mPlayer.prepareAsync();
    }

    @Override
    public void seekTo(int msec) throws IllegalStateException {
        checkIsNotReleased();

        mPlayer.seekTo(msec);
    }

    @Override
    public int getDuration() {
        if (mPlayer == null) {
            return 0;
        }

        int duration = mPlayer.getDuration();
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
        if (mPlayer == null) {
            return 0;
        }

        int position = mPlayer.getCurrentPosition();
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
        if (mReleased) {
            return;
        }
        mUserOnCompletionListener = listener;
    }

    @Override
    public void setOnPreparedListener(IBasicMediaPlayer.OnPreparedListener listener) {
        if (mReleased) {
            return;
        }
        mUserOnPreparedListener = listener;
    }

    @Override
    public void setOnSeekCompleteListener(IBasicMediaPlayer.OnSeekCompleteListener listener) {
        if (mReleased) {
            return;
        }
        mUserOnSeekCompleteListener = listener;
    }

    @Override
    public void setOnBufferingUpdateListener(IBasicMediaPlayer.OnBufferingUpdateListener listener) {
        if (mReleased) {
            return;
        }
        mUserOnBufferingUpdateListener = listener;
    }

    @Override
    public void setOnInfoListener(IBasicMediaPlayer.OnInfoListener listener) {
        if (mReleased) {
            return;
        }
        mUserOnInfoListener = listener;
    }

    @Override
    public void setOnErrorListener(IBasicMediaPlayer.OnErrorListener listener) {
        if (mReleased) {
            return;
        }
        mUserOnErrorListener = listener;
    }

    @Override
    public void setAuxEffectSendLevel(float level) {
        // [Workaround]
        // Hack: Can't handle some invalid values
        if (Float.isNaN(level) || level < 0.0f)
            level = Float.POSITIVE_INFINITY;

        mPlayer.setAuxEffectSendLevel(level);
    }

    @Override
    public void setWakeMode(Context context, int mode) {
        if (mPlayer != null) {
            mPlayer.setWakeMode(context, mode);
        }
    }

    @Override
    public void setNextMediaPlayer(IBasicMediaPlayer next) {
        if (next != null && !(next instanceof StandardMediaPlayer)) {
            throw new IllegalArgumentException("Not StandardMediaPlayer instance");
        }

        if (next == this) {
            throw new IllegalArgumentException();
        }

        mNextMediaPlayerRef = new WeakReference<StandardMediaPlayer>((StandardMediaPlayer) next);

        applyNextMediaPlayer();
    }

    @Override
    public void setAudioAttributes(AudioAttributes attributes) {
        if (attributes == null) {
            throw new IllegalArgumentException();
        }

        if (MediaPlayerCompat.supportsSetAudioAttributes()) {
            MediaPlayerCompat.setAudioAttributes(mPlayer, attributes);
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
            mPlayer.setLooping(looping);
            mIsLooping = looping;
            applyNextMediaPlayer();
        }
    }

    @Override
    public boolean isLooping() {
        if (!mReleased && mIsIdleOrInitializedState) {
            return mIsLooping;
        } else {
            return mPlayer.isLooping();
        }
    }

    @Override
    public boolean isPlaying() throws IllegalStateException {
        checkIsNotReleased();

        return mPlayer.isPlaying();
    }

    @Override
    public void attachAuxEffect(int effectId) {
        if (mPlayer == null) {
            return;
        }
        mPlayer.attachAuxEffect(effectId);
    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
        if (mPlayer == null) {
            return;
        }
        mPlayer.setVolume(leftVolume, rightVolume);
    }

    @Override
    public void setAudioStreamType(int streamtype) {
        if (mPlayer == null) {
            return;
        }
        mPlayer.setAudioStreamType(streamtype);
    }

    private void applyNextMediaPlayer() {
        if (MediaPlayerCompat.supportsSetNextMediaPlayer()) {
            StandardMediaPlayer nextStandardMediaPlayer = mNextMediaPlayerRef.get();
            MediaPlayer next = (nextStandardMediaPlayer != null) ? nextStandardMediaPlayer.mPlayer : null;

            if (mUsingNuPlayer && mIsLooping) {
                // To avoid the bug of NuPlayer (next player is preferred than looping setting)
                next = null;
            }

            MediaPlayerCompat.setNextMediaPlayer(mPlayer, next);
        }
    }

    private void applyLoopingState() {
        mIsIdleOrInitializedState = false;
        mPlayer.setLooping(mIsLooping);
    }

    protected void handleOnCompletion(MediaPlayer mp) {
        if (isCompletionOnLoopPointSupported() && mIsLooping) {
            onNotifyLoopPoint();
            return;
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
            mUserOnCompletionListener.onCompletion(this);
        }
    }

    private void onNotifyLoopPoint() {
        if (mUserOnSeekCompleteListener != null) {
            mUserOnSeekCompleteListener.onSeekComplete(this);
        }
    }

    protected void handleOnPrepared(MediaPlayer mp) {
        mDuration = mPlayer.getDuration();
        mIsPrepared = true;
        applyLoopingState();
        if (mUserOnPreparedListener != null) {
            mUserOnPreparedListener.onPrepared(this);
        }
    }

    protected void handleOnSeekComplete(MediaPlayer mp) {
        if (mUserOnSeekCompleteListener != null) {
            mUserOnSeekCompleteListener.onSeekComplete(this);
        }
    }

    protected void handleOnBufferingUpdate(MediaPlayer mp, int percent) {
        if (mUserOnBufferingUpdateListener != null) {
            mUserOnBufferingUpdateListener.onBufferingUpdate(this, percent);
        }
    }

    protected boolean handleOnError(MediaPlayer mp, int what, int extra) {
        if (mUserOnErrorListener != null) {
            return mUserOnErrorListener.onError(this, what, extra);
        }

        return false;
    }

    protected boolean handleOnInfo(MediaPlayer mp, int what, int extra) {
        if (mUserOnInfoListener != null) {
            return mUserOnInfoListener.onInfo(this, what, extra);
        }

        return false;
    }

    private void checkIsNotReleased() throws IllegalStateException {
        if (mReleased) {
            throw new IllegalStateException(
                    "The media player instance has already released");
        }
    }

    private boolean isCompletionOnLoopPointSupported() {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) && (mUsingNuPlayer);
    }
}
