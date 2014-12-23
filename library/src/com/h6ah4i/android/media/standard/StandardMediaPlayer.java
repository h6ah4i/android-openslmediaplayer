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
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.compat.AudioAttributes;
import com.h6ah4i.android.media.compat.MediaPlayerCompat;

public class StandardMediaPlayer implements IBasicMediaPlayer {
    private static final String TAG = "StandardMediaPlayer";
    private static final String UNEXPECTED_EXCEPTION_MESSAGE = "Unexpected exception occurred";

    private static final boolean LOCAL_LOGV = false;
    private static final boolean LOCAL_LOGD = true;

    private MediaPlayer mPlayer;
    private MediaPlayerStateManager mState;
    private InternalHandler mHandler;
    private WeakReference<StandardMediaPlayer> mNextMediaPlayerRef;
    private IBasicMediaPlayer.OnCompletionListener mUserOnCompletionListener;
    private IBasicMediaPlayer.OnPreparedListener mUserOnPreparedListener;
    private IBasicMediaPlayer.OnSeekCompleteListener mUserOnSeekCompleteListener;
    private IBasicMediaPlayer.OnBufferingUpdateListener mUserOnBufferingUpdateListener;
    private IBasicMediaPlayer.OnInfoListener mUserOnInfoListener;
    private IBasicMediaPlayer.OnErrorListener mUserOnErrorListener;
    private boolean mIsLooping;
    private int mDuration;
    private boolean mUsingNuPlayer;


    private static abstract class SkipCondition {
        public abstract boolean check(MediaPlayerStateManager stateManager);
    }
    private static class SkipConditionIfNotPrepared extends SkipCondition {
        @Override
        public boolean check(MediaPlayerStateManager stateManager) {
            return !isPrepared(stateManager.getPrevErrorState());
        }
    }
    private static class SkipConditionForPauseMethod extends SkipCondition {
        @Override
        public boolean check(MediaPlayerStateManager stateManager) {
            final int s = stateManager.getPrevErrorState();
            return !(isPrepared(s) || s == MediaPlayerStateManager.STATE_INITIALIZED || s == MediaPlayerStateManager.STATE_STOPPED);
        }
    }

    private static final SkipCondition SKIP_CONDITION_NEVER = null;
    private static final SkipCondition SKIP_CONDITION_IF_NOT_PREPARED = new SkipConditionIfNotPrepared();
    private static final SkipCondition SKIP_CONDITION_FOR_PAUSE_METHOD = new SkipConditionForPauseMethod();

    private android.media.MediaPlayer.OnCompletionListener mHookOnCompletionListener = new android.media.MediaPlayer.OnCompletionListener() {
        @Override
        public void onCompletion(android.media.MediaPlayer mp) {
            StandardMediaPlayer.this.handleOnCompletion(mp);
        }
    };

    private android.media.MediaPlayer.OnPreparedListener mHookOnPreparedListener = new android.media.MediaPlayer.OnPreparedListener() {
        @Override
        public void onPrepared(android.media.MediaPlayer mp) {
            StandardMediaPlayer.this.handleOnPrepared(mp, true);
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

    private android.media.MediaPlayer.OnInfoListener mHookOnInfoListener = new android.media.MediaPlayer.OnInfoListener() {
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
        try {
            mNextMediaPlayerRef = new WeakReference<StandardMediaPlayer>(null);
            mUsingNuPlayer = NuPlayerDetector.isUsingNuPlayer();
            mHandler = new InternalHandler(this);

            mState = new MediaPlayerStateManager();

            mPlayer = new MediaPlayer();

            if (LOCAL_LOGD) {
                Log.d(TAG, "[" + System.identityHashCode(mPlayer) + "] Create MediaPlayer instance");
            }

            mPlayer.setOnCompletionListener(mHookOnCompletionListener);
            mPlayer.setOnPreparedListener(mHookOnPreparedListener);
            mPlayer.setOnSeekCompleteListener(mHookOnSeekCompeleteListener);
            mPlayer.setOnBufferingUpdateListener(mHookOnBufferingUpdateListener);
            mPlayer.setOnInfoListener(mHookOnInfoListener);
            mPlayer.setOnErrorListener(mHookOnErrorListener);
        } catch (Exception e) {
            Log.e(TAG, makeUnexpectedExceptionCaughtMessage("StandardMediaPlayer()"), e);
            // call release() method to transit to END state
            releaseInternal();
        }
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
        releaseInternal();
    }

    private void releaseInternal() {
        if (mPlayer != null) {
            mPlayer.setOnCompletionListener(null);
            mPlayer.setOnPreparedListener(null);
            mPlayer.setOnSeekCompleteListener(null);
            mPlayer.setOnBufferingUpdateListener(null);
            mPlayer.setOnInfoListener(null);
            mPlayer.setOnErrorListener(null);

            if (LOCAL_LOGD) {
                Log.d(TAG, "[" + System.identityHashCode(mPlayer) + "] Release MediaPlayer instance");
            }

            mPlayer.release();
            mPlayer = null;
        }

        if (mHandler != null) {
            mHandler.release();
            mHandler = null;
        }

        mHookOnCompletionListener = null;
        mHookOnPreparedListener = null;
        mHookOnSeekCompeleteListener = null;
        mHookOnBufferingUpdateListener = null;
        mHookOnInfoListener = null;
        mHookOnErrorListener = null;

        mUserOnCompletionListener = null;
        mUserOnPreparedListener = null;
        mUserOnSeekCompleteListener = null;
        mUserOnInfoListener = null;
        mUserOnErrorListener = null;

        mNextMediaPlayerRef.clear();

        mState.transitToEndState();
    }

    @Override
    public void reset() {
        checkIsNotReleased();

        if (!mState.canCallReset()) {
            return;
        }

        if (mPlayer != null) {
            mPlayer.reset();
        }

        if (mHandler != null) {
            mHandler.removeCallbacksAndMessages(null);
        }

        mIsLooping = false;

        mState.transitToIdleState();
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
        final String METHOD_NAME = "start()";

        checkIsNotReleased();

        if (!mState.canCallStart()) {
            transitToErrorStateAndCallback(METHOD_NAME, MEDIA_ERROR_UNKNOWN, 0, SKIP_CONDITION_IF_NOT_PREPARED);
            return;
        }

        try {
            mPlayer.start();
            mState.transitToStartedState();
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void stop() throws IllegalStateException {
        final String METHOD_NAME = "stop()";

        checkIsNotReleased();

        if (!mState.canCallStop()) {
            transitToErrorStateAndCallback(METHOD_NAME, MEDIA_ERROR_UNKNOWN, 0, SKIP_CONDITION_IF_NOT_PREPARED);
            return;
        }

        try {
            mPlayer.stop();
            mState.transitToStoppedState();
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void setDataSource(Context context, Uri uri) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        final String METHOD_NAME = "setDataSource()";

        try {
            if (!mState.canCallSetDataSource()) {
                if (isStateError()) {
                    return;
                }

                transitToErrorStateAndThrowException(METHOD_NAME);
                return;
            }

            mPlayer.setDataSource(context, uri);
            mState.transitToInitialized();
        } catch (IOException e) {
            throw e; // re-throw
        } catch (IllegalArgumentException e) {
            throw e; // re-throw
        } catch (SecurityException e) {
            throw e; // re-throw
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (NullPointerException e) {
            // NOTE:
            // Maybe, passing a wrong context.
            // If running on InstrumentationTestCase, getInstrumentation().getTargetContext() method should be used.
            Log.w(TAG, METHOD_NAME + ", maybe passing a wrong context object.", e);
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void setDataSource(FileDescriptor fd) throws IOException, IllegalArgumentException, IllegalStateException {
        final String METHOD_NAME = "setDataSource()";

        if (!mState.canCallSetDataSource()) {
            if (isStateError()) {
                return;
            }

            transitToErrorStateAndThrowException(METHOD_NAME);
            return;
        }

        try {
            mPlayer.setDataSource(fd);
            mState.transitToInitialized();
        } catch (IOException e) {
            throw e; // re-throw
        } catch (IllegalArgumentException e) {
            throw e; // re-throw
        } catch (SecurityException e) {
            throw e; // re-throw
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void setDataSource(FileDescriptor fd, long offset, long length) throws IOException, IllegalArgumentException, IllegalStateException {
        final String METHOD_NAME = "setDataSource()";

        if (!mState.canCallSetDataSource()) {
            if (isStateError()) {
                return;
            }

            transitToErrorStateAndThrowException(METHOD_NAME);
            return;
        }

        try {
            mPlayer.setDataSource(fd, offset, length);
            mState.transitToInitialized();
        } catch (IOException e) {
            throw e; // re-throw
        } catch (IllegalArgumentException e) {
            throw e; // re-throw
        } catch (SecurityException e) {
            throw e; // re-throw
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void setDataSource(String path) throws IOException, IllegalArgumentException, IllegalStateException {
        final String METHOD_NAME = "setDataSource()";

        if (!mState.canCallSetDataSource()) {
            if (isStateError()) {
                return;
            }

            transitToErrorStateAndThrowException(METHOD_NAME);
            return;
        }

        try {
            mPlayer.setDataSource(path);
            mState.transitToInitialized();
        } catch (IOException e) {
            throw e; // re-throw
        } catch (IllegalArgumentException e) {
            throw e; // re-throw
        } catch (SecurityException e) {
            throw e; // re-throw
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void pause() throws IllegalStateException {
        final String METHOD_NAME = "pause()";

        checkIsNotReleased();

        if (!mState.canCallPause()) {
            transitToErrorStateAndCallback(METHOD_NAME, MEDIA_ERROR_UNKNOWN, 0, SKIP_CONDITION_FOR_PAUSE_METHOD);
            return;
        }

        try {
            mPlayer.pause();
            if (mState.getState() == MediaPlayerStateManager.STATE_STARTED) {
                mState.transitToPausedState();
            }
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void prepare() throws IOException, IllegalStateException {
        final String METHOD_NAME = "prepare()";

        if (!mState.canCallPrepare()) {
            final String msg = makeMethodCalledInvalidStateMessage(METHOD_NAME);
            mState.transitToErrorState();
            throw new IllegalStateException(msg);
        }

        try {
            mPlayer.prepare();
            handleOnPrepared(mPlayer, false);
        } catch (IOException e) {
            throw e; // re-throw
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        final String METHOD_NAME = "prepareAsync()";

        if (!mState.canCallPrepareAsync()) {
            final String msg = makeMethodCalledInvalidStateMessage(METHOD_NAME);
            mState.transitToErrorState();
            throw new IllegalStateException(msg);
        }

        try {
            mPlayer.prepareAsync();
            mState.transitToPreparingState();
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public void seekTo(int msec) throws IllegalStateException {
        final String METHOD_NAME = "seekTo()";

        checkIsNotReleased();

        if (!mState.canCallSeekTo()) {
            transitToErrorStateAndCallback(METHOD_NAME, MEDIA_ERROR_UNKNOWN, 0, SKIP_CONDITION_IF_NOT_PREPARED);
            return;
        }

        try {
            mPlayer.seekTo(msec);
        } catch (IllegalStateException e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw e; // re-throw
        } catch (Exception e) {
            Log.w(TAG, makeUnexpectedExceptionCaughtMessage(METHOD_NAME), e);
            mState.transitToErrorState();
            throw new IllegalStateException(UNEXPECTED_EXCEPTION_MESSAGE, e);
        }
    }

    @Override
    public int getDuration() {
        final String METHOD_NAME = "getDuration()";

        checkIsNotReleased();

        if (!mState.canCallGetDuration()) {
            transitToErrorStateAndCallback(METHOD_NAME, MEDIA_ERROR_UNKNOWN, 0, SKIP_CONDITION_NEVER);
            return 0;
        }

        int duration = mPlayer.getDuration();
        if (!isPrepared()) {
            // [Workaround]
            // super.getDuration() may returns invalid (uninitialized ?) value
            // before calling prepare() method
            duration = 0;
        }
        return duration;
    }

    @Override
    public int getCurrentPosition() {
        checkIsNotReleased();

        if (!mState.canCallGetCurrentPosition()) {
            return 0;
        }

        int position = mPlayer.getCurrentPosition();
        if (!isPrepared()) {
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
        if (isReleased()) {
            return;
        }
        mUserOnCompletionListener = listener;
    }

    @Override
    public void setOnPreparedListener(IBasicMediaPlayer.OnPreparedListener listener) {
        if (isReleased()) {
            return;
        }
        mUserOnPreparedListener = listener;
    }

    @Override
    public void setOnSeekCompleteListener(IBasicMediaPlayer.OnSeekCompleteListener listener) {
        if (isReleased()) {
            return;
        }
        mUserOnSeekCompleteListener = listener;
    }

    @Override
    public void setOnBufferingUpdateListener(IBasicMediaPlayer.OnBufferingUpdateListener listener) {
        if (isReleased()) {
            return;
        }
        mUserOnBufferingUpdateListener = listener;
    }

    @Override
    public void setOnInfoListener(IBasicMediaPlayer.OnInfoListener listener) {
        if (isReleased()) {
            return;
        }
        mUserOnInfoListener = listener;
    }

    @Override
    public void setOnErrorListener(IBasicMediaPlayer.OnErrorListener listener) {
        if (isReleased()) {
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
        final String METHOD_NAME = "setNextMediaPlayer()";

        checkIsNotReleased();

        if (next != null && !(next instanceof StandardMediaPlayer)) {
            throw new IllegalArgumentException("Not StandardMediaPlayer instance");
        }

        if (next == this) {
            throw new IllegalArgumentException("Passed self instance (next == this)");
        }

        StandardMediaPlayer next2 = (StandardMediaPlayer) next;

        if (next != null && !isReadyForNextPlayer(next2)) {
            throw new IllegalStateException(
                    "The internal state of the passed player object is not acceptable for next player (state = " + next2.mState.getCurrentStateCodeString() + ")");
        }

        final int state = mState.getState();

        // NOTE:
        // MediaPlayer.setMediaPlayer() throws IllegalArgumentsException
        if (state == MediaPlayerStateManager.STATE_IDLE ||
            state == MediaPlayerStateManager.STATE_END ||
            state == MediaPlayerStateManager.STATE_ERROR) {
            final String msg = makeMethodCalledInvalidStateMessage(METHOD_NAME);
            throw new IllegalStateException(msg);
        }

        mNextMediaPlayerRef = new WeakReference<StandardMediaPlayer>((StandardMediaPlayer) next);

        applyNextMediaPlayer();
    }

    @Override
    public void setAudioAttributes(AudioAttributes attributes) {
        if (attributes == null) {
            throw new IllegalArgumentException("The argument attributes must not be null.");
        }

        checkIsNotReleased();

        if (MediaPlayerCompat.supportsSetAudioAttributes()) {
            MediaPlayerCompat.setAudioAttributes(mPlayer, attributes);
        } else {
            if (LOCAL_LOGV) {
                Log.v(TAG, "setAudioAttributes() is not supported, just ignored.");
            }
        }
    }

    @Override
    public void setLooping(boolean looping) {
        // [Workaround]
        // Stock MediaPlayer doesn't support setLooping() method in idle state
        if ((!isReleased()) && isBeforePrepared()) {
            mIsLooping = looping;
        } else {
            mPlayer.setLooping(looping);
            mIsLooping = looping;
            applyNextMediaPlayer();
        }
    }

    @Override
    public boolean isLooping() {
        if ((!isReleased()) && isBeforePrepared()) {
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

        if (isStateError()) {
            postError(MEDIA_ERROR_UNKNOWN, 0);
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

            if ((next != null) && mUsingNuPlayer && mIsLooping) {
                // To avoid the bug of NuPlayer (next player is preferred than looping setting)
                next = null;
            }

            if ((next != null) && !isReadyForNextPlayer(nextStandardMediaPlayer)) {
                next = null;
            }

            MediaPlayerCompat.setNextMediaPlayer(mPlayer, next);
        }
    }

    private static boolean isReadyForNextPlayer(StandardMediaPlayer next) {
        if (next == null) {
            return false;
        }

        int state = next.mState.getState();
        return (state == MediaPlayerStateManager.STATE_PREPARED ||
               state == MediaPlayerStateManager.STATE_PAUSED ||
                state == MediaPlayerStateManager.STATE_PLAYBACK_COMPLETED);
    }

    private void applyLoopingState() {
        mPlayer.setLooping(mIsLooping);
    }

    protected void handleOnCompletion(MediaPlayer mp) {
        if (isCompletionOnLoopPointSupported() && mIsLooping) {
            onNotifyLoopPoint();
            return;
        }

        boolean nextPlayerStarted = false;

        // Emulate setNextMediaPlayer() behavior
        if (!MediaPlayerCompat.supportsSetNextMediaPlayer()) {
            StandardMediaPlayer nextmp = mNextMediaPlayerRef.get();
            try {
                if (nextmp != null) {
                    nextmp.start();
                    nextPlayerStarted = true;
                }
            } catch (Exception e) {
                // eat all exceptions here
                Log.e(TAG, "An exception occurred in handleOnCompletion()", e);
            }
        }

        mState.transitToPlaybackCompleted();

        if (!nextPlayerStarted && mUserOnCompletionListener != null) {
            mUserOnCompletionListener.onCompletion(this);
        }
    }

    private void onNotifyLoopPoint() {
        if (mUserOnSeekCompleteListener != null) {
            mUserOnSeekCompleteListener.onSeekComplete(this);
        }
    }

    protected void handleOnPrepared(MediaPlayer mp, boolean invokeCallback) {
        mDuration = mPlayer.getDuration();
        applyLoopingState();

        mState.transitToPreparedState();

        if (invokeCallback && mUserOnPreparedListener != null) {
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
        mState.transitToErrorState();

        if (isReleased()) {
            return false;
        }

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

    private void transitToErrorStateAndCallback(String methodName, int what, int extra, SkipCondition skipCallback) {
        final String msg = makeMethodCalledInvalidStateMessage(methodName);

        if (LOCAL_LOGV) {
            Log.v(TAG, msg);
        }

        mState.transitToErrorState();

        if (skipCallback != null && skipCallback.check(mState)) {
            // don't raise error callback if the error has been occurred before prepared state.
            return;
        }

        postError(what, extra);
    }

    private void transitToErrorStateAndThrowException(String methodName) {
        final String msg = makeMethodCalledInvalidStateMessage(methodName);
        mState.transitToErrorState();
        throw new IllegalStateException(msg);
    }

    private void postError(int what, int extra) {
        final int prevState = mState.getPrevErrorState();

        if (mHandler != null) {
            mHandler.postError(prevState, what, extra);
        }
    }

    private void checkIsNotReleased() throws IllegalStateException {
        if (isReleased()) {
            throw new IllegalStateException(
                    "The media player instance has already released");
        }
    }

    private boolean isReleased() {
        return mState.isStateEnd();
    }

    private boolean isStateError() {
        return mState.isStateError();
    }

    private boolean isBeforePrepared() {
        final int state = mState.getState();
        return (state == MediaPlayerStateManager.STATE_IDLE ||
                state == MediaPlayerStateManager.STATE_INITIALIZED ||
                state == MediaPlayerStateManager.STATE_PREPARING);
    }

    private boolean isPrepared() {
        return isPrepared(mState.getState());
    }

    private static boolean isPrepared(int state) {
        return (state == MediaPlayerStateManager.STATE_PREPARED ||
                state == MediaPlayerStateManager.STATE_STARTED ||
                state == MediaPlayerStateManager.STATE_PAUSED ||
                state == MediaPlayerStateManager.STATE_PLAYBACK_COMPLETED);
    }

    private boolean isCompletionOnLoopPointSupported() {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) && (mUsingNuPlayer);
    }

    private String makeMethodCalledInvalidStateMessage(String messageName) {
        return messageName + " method cannot be called during " + mState.getCurrentStateCodeString() + " state";
    }

    private static String makeUnexpectedExceptionCaughtMessage(String messageName) {
        return "Unexpected exception: " + messageName;
    }

    private static class InternalHandler extends Handler {
        private WeakReference<StandardMediaPlayer> mRefHolder;

        public static final int MSG_RAISE_ERROR = 1;

        public InternalHandler(StandardMediaPlayer holder) {
            mRefHolder = new WeakReference<StandardMediaPlayer>(holder);
        }

        @Override
        public void handleMessage(Message msg) {
            StandardMediaPlayer holder = (StandardMediaPlayer) mRefHolder.get();

            if (holder == null) {
                return;
            }

            switch (msg.what) {
                case MSG_RAISE_ERROR: {
                    ErrorInfo info = (ErrorInfo) msg.obj;
                    holder.handlePostedError(info.prevState, info.what, info.extra);
                }
                    break;
            }
        }

        public void release() {
            removeCallbacksAndMessages(null);
            mRefHolder.clear();
        }

        public void postError(int prevState, int what, int extra) {
            sendMessage(obtainMessage(MSG_RAISE_ERROR, new ErrorInfo(prevState, what, extra)));
        }


        private static class ErrorInfo {
            public int prevState;
            public int what;
            public int extra;

            public ErrorInfo(int prevState, int what, int extra) {
                this.prevState = prevState;
                this.what = what;
                this.extra = extra;
            }
        }
    }

    private void handlePostedError(int prevState, int what, int extra) {
        boolean handled = false;

        // raise onError()
        if (mUserOnErrorListener != null) {
            handled = mUserOnErrorListener.onError(this, what, extra);
        }

        // raise onCompletion()
        if (!handled && (mUserOnCompletionListener != null)) {
            mUserOnCompletionListener.onCompletion(this);
        }
    }
}
