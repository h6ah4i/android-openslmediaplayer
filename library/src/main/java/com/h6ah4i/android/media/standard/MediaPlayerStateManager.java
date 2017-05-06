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

class MediaPlayerStateManager {
    public static final int STATE_IDLE = 0;
    public static final int STATE_INITIALIZED = 1;
    public static final int STATE_PREPARING = 2;
    public static final int STATE_PREPARED = 3;
    public static final int STATE_STARTED = 4;
    public static final int STATE_PAUSED = 5;
    public static final int STATE_PLAYBACK_COMPLETED = 6;
    public static final int STATE_STOPPED = 7;
    public static final int STATE_ERROR = 8;
    public static final int STATE_END = 9;

    private int mState;
    private int mPrevErrorState;

    public MediaPlayerStateManager() {
        mState = STATE_IDLE;
        mPrevErrorState = STATE_IDLE;
    }

    public int getState() {
        return mState;
    }

    public int getPrevErrorState() {
        return mPrevErrorState;
    }

    private static boolean canTransitToInitialized(int state) {
        return state == STATE_IDLE;
    }

    private static boolean canTransitToPrepared(int state) {
        return state == STATE_INITIALIZED ||
                state == STATE_PREPARING ||
                state == STATE_STOPPED;
    }

    private static boolean canTransitToPreparing(int state) {
        return state == STATE_INITIALIZED ||
                state == STATE_STOPPED;
    }

    private static boolean canTransitToStarted(int state) {
        return state == STATE_PREPARED ||
                state == STATE_STARTED ||
                state == STATE_PAUSED ||
                state == STATE_PLAYBACK_COMPLETED;
    }

    private static boolean canTransitToPaused(int state) {
        return state == STATE_STARTED ||
                state == STATE_PAUSED;
    }

    private static boolean canTransitToStopped(int state) {
        return state == STATE_PREPARED ||
                state == STATE_STARTED ||
                state == STATE_PAUSED ||
                state == STATE_PLAYBACK_COMPLETED ||
                state == STATE_STOPPED;
    }

    private static boolean canTransitToIdle(int state) {
        return state != STATE_END;
    }

    private static boolean canTransitToPlaybackCompleted(int state) {
        return state == STATE_STARTED;
    }

    private static boolean canTransitToError(int state) {
        return state != STATE_END;
    }

    private static boolean canTransitToEnd(int state) {
        return true;
    }

    public boolean canCallSetDataSource() {
        return canTransitToInitialized(mState);
    }

    public boolean canCallPrepare() {
        final int state = mState;
        return state == STATE_INITIALIZED || state == STATE_STOPPED;
    }

    public boolean canCallPrepareAsync() {
        return canTransitToPreparing(mState);
    }

    public boolean canCallStart() {
        return canTransitToStarted(mState);
    }

    public boolean canCallPause() {
        return canTransitToPaused(mState) || (mState == STATE_PLAYBACK_COMPLETED);
    }

    public boolean canCallStop() {
        return canTransitToStopped(mState);
    }

    public boolean canCallReset() {
        return canTransitToIdle(mState);
    }

    public boolean canCallSeekTo() {
        final int state = mState;
        return (state == STATE_PREPARED || state == STATE_STARTED ||
                state == STATE_PAUSED || state == STATE_PLAYBACK_COMPLETED);
    }

    public boolean canCallGetDuration() {
        final int state = mState;
        return (state == STATE_PREPARED || state == STATE_STARTED ||
                state == STATE_PAUSED || state == STATE_STOPPED ||
                state == STATE_PLAYBACK_COMPLETED);
    }

    public boolean canCallGetCurrentPosition() {
        final int state = mState;
        return (state == STATE_IDLE || state == STATE_INITIALIZED ||
                state == STATE_PREPARED || state == STATE_STARTED ||
                state == STATE_PAUSED || state == STATE_STOPPED ||
                state == STATE_PLAYBACK_COMPLETED);
    }

    private boolean setState(int state) {
        if (verifyStateTransition(mState, state)) {
            if (state == STATE_ERROR && mState != STATE_ERROR) {
                // save current state
                mPrevErrorState = mState;
            }
            mState = state;
            return false;
        } else {
            return false;
        }
    }

    public void transitToIdleState() {
        setState(STATE_IDLE);
    }

    public void transitToErrorState() {
        setState(STATE_ERROR);
    }

    public void transitToEndState() {
        if (setState(STATE_END)) {
            mPrevErrorState = STATE_IDLE;
        }
    }

    public void transitToInitialized() {
        if (setState(STATE_INITIALIZED)) {
            mPrevErrorState = STATE_IDLE;
        }
    }

    public void transitToPreparingState() {
        setState(STATE_PREPARING);
    }

    public void transitToPreparedState() {
        setState(STATE_PREPARED);
    }

    public void transitToStartedState() {
        setState(STATE_STARTED);
    }

    public void transitToStoppedState() {
        setState(STATE_STOPPED);
    }

    public void transitToPausedState() {
        setState(STATE_PAUSED);
    }

    public void transitToPlaybackCompleted() {
        setState(STATE_PLAYBACK_COMPLETED);
    }

    public boolean isStateEnd() {
        return (mState == STATE_END);
    }

    public boolean isStateError() {
        return (mState == STATE_ERROR);
    }

    public String getCurrentStateCodeString() {
        return getStateCodeString(mState);
    }

    public static String getStateCodeString(int state) {
        switch (state) {
            case STATE_IDLE:
                return "IDLE";
            case STATE_INITIALIZED:
                return "INITIALIZED";
            case STATE_PREPARING:
                return "PREPARING";
            case STATE_PREPARED:
                return "PREPARED";
            case STATE_STARTED:
                return "STARTED";
            case STATE_PAUSED:
                return "PAUSED";
            case STATE_PLAYBACK_COMPLETED:
                return "PLAYBACK_COMPLETED";
            case STATE_STOPPED:
                return "STOPPED";
            case STATE_ERROR:
                return "ERROR";
            case STATE_END:
                return "END";
            default:
                throw new IllegalArgumentException("Unknown state code(state = " + state + ")");
        }
    }

    private boolean verifyStateTransition(int current, int next) {
        switch (next) {
            case STATE_IDLE:
                return canTransitToIdle(current);
            case STATE_INITIALIZED:
                return canTransitToInitialized(current);
            case STATE_PREPARING:
                return canTransitToPreparing(current);
            case STATE_PREPARED:
                return canTransitToPrepared(current);
            case STATE_STARTED:
                return canTransitToStarted(current);
            case STATE_PAUSED:
                return canTransitToPaused(current);
            case STATE_PLAYBACK_COMPLETED:
                return canTransitToPlaybackCompleted(current);
            case STATE_STOPPED:
                return canTransitToStopped(current);
            case STATE_ERROR:
                return canTransitToError(current);
            case STATE_END:
                return canTransitToEnd(current);
            default:
                throw new IllegalArgumentException("Unknown state code(current = " + current + ", next = " + next + ")");
        }
    }
}
