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


package com.h6ah4i.android.media.openslmediaplayer.base;

import android.os.Build;

import java.io.IOException;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;

public abstract class BasicMediaPlayerStateTestCaseBase
        extends BasicMediaPlayerTestCaseBase {

    public BasicMediaPlayerStateTestCaseBase(ParameterizedTestArgs args) {
        super(args);
    }

    //
    // Test Cases (exported)
    //

    // State: IDLE
    public void testStateIdle() throws Throwable {
        implTestStateIdle(getTestParams());
    }

    public void testStateIdleAfterReset() throws Throwable {
        implTestStateIdleAfterReset(getTestParams());
    }

    // State: INITIALIZED
    public void testStateInitialized() throws Throwable {
        implTestStateInitialized(getTestParams());
    }

    public void testStateInitializedAfterReset() throws Throwable {
        implTestStateInitializedAfterReset(getTestParams());
    }

    // State: PREPARING
    public void testStatePreparing() throws Throwable {
        implTestStatePreparing(getTestParams());
    }

    public void testStatePreparingAfterReset() throws Throwable {
        implTestStatePreparingAfterReset(getTestParams());
    }

    // State: PREPARED
    public void testStatePrepared() throws Throwable {
        implTestStatePrepared(getTestParams());
    }

    public void testStatePreparedAfterReset() throws Throwable {
        implTestStatePreparedAfterReset(getTestParams());
    }

    // State: STARTED
    public void testStateStarted() throws Throwable {
        implTestStateStarted(getTestParams());
    }

    public void testStateStartedAfterReset() throws Throwable {
        implTestStateStartedAfterReset(getTestParams());
    }

    // State: PAUSED
    public void testStatePaused() throws Throwable {
        implTestStatePaused(getTestParams());
    }

    public void testStatePausedAfterReset() throws Throwable {
        implTestStatePausedAfterReset(getTestParams());
    }

    // State: STOPPEED
    public void testStateStopped() throws Throwable {
        implTestStateStopped(getTestParams());
    }

    public void testStateStoppedAfterReset() throws Throwable {
        implTestStateStoppedAfterReset(getTestParams());
    }

    // State: PLAYBACK COMPLETED
    public void testStatePlaybackCompleted() throws Throwable {
        implTestStatePlaybackCompleted(getTestParams());
    }

    public void testStatePlaybackCompletedAfterReset() throws Throwable {
        implTestStatePlaybackCompletedAfterReset(getTestParams());
    }

    // State: ERROR (before prepared)
    public void testStateErrorBeforePrepared() throws Throwable {
        implTestStateErrorBeforePrepared(getTestParams());
    }

    public void testStateErrorBeforePreparedAndAfterReset() throws Throwable {
        implTestStateErrorBeforePreparedAndAfterReset(getTestParams());
    }

    // State: ERROR (after prepared)
    public void testStateErrorAfterPrepared() throws Throwable {
        implTestStateErrorAfterPrepared(getTestParams());
    }

    public void testStateErrorAfterPreparedAndAfterReset() throws Throwable {
        implTestStateErrorAfterPreparedAndAfterReset(getTestParams());
    }

    // State: END
    public void testStateEnd() throws Throwable {
        implTestStateEnd(getTestParams());
    }

    //
    // Test Cases (state transition & calling actual test method)
    //

    protected void implTestStateIdle(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToIdle(player, args);
            onTestStateIdle(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateIdleAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToIdle(player, args);
            onTestStateIdle(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateInitialized(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        try {
            transitStateToInitialized(player, args);
            onTestStateInitialized(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateInitializedAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        try {
            playLocalFileAndReset(player, args);
            transitStateToInitialized(player, args);
            onTestStateInitialized(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStatePreparing(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToPreparing(player, args);
            onTestStatePreparing(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStatePreparingAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToPreparing(player, args);
            onTestStatePreparing(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStatePrepared(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToPrepared(player, args);
            onTestStatePrepared(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStatePreparedAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToPrepared(player, args);
            onTestStatePrepared(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateStarted(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToStarted(player, args);
            onTestStateStarted(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateStartedAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToStarted(player, args);
            onTestStateStarted(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStatePaused(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToPaused(player, args);
            onTestStatePaused(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStatePausedAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToPaused(player, args);
            onTestStatePaused(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateStopped(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToStopped(player, args);
            onTestStateStopped(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateStoppedAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToStopped(player, args);
            onTestStateStopped(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStatePlaybackCompleted(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToPlaybackCompleted(player, args);
            onTestStatePlaybackCompleted(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStatePlaybackCompletedAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToPlaybackCompleted(player, args);
            onTestStatePlaybackCompleted(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateErrorBeforePrepared(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToErrorBeforePrepared(player, args);
            onTestStateErrorBeforePrepared(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateErrorBeforePreparedAndAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToErrorBeforePrepared(player, args);
            onTestStateErrorBeforePrepared(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateErrorAfterPrepared(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToErrorAfterPrepared(player, args);
            onTestStateErrorAfterPrepared(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateErrorAfterPreparedAndAfterReset(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            playLocalFileAndReset(player, args);
            transitStateToErrorAfterPrepared(player, args);
            onTestStateErrorAfterPrepared(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    protected void implTestStateEnd(Object args) throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();

        try {
            transitStateToEnd(player, args);
            onTestStateEnd(player, args);
        } finally {
            releaseQuietly(player);
        }
    }

    @Override
    protected void setDataSourceForCommonTests(IBasicMediaPlayer player, Object args)
            throws IOException {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            // Older devices returns completely wrong duration for OGG files.
            player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_MP3));
        } else {
            // MP3 format should not be used for this test
            // because MP3 file cannot handle duration info correctly.
            player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_OGG));
        }
    }

    @Override
    protected void setDataSourceForPlaybackCompletedTest(IBasicMediaPlayer player, Object args)
            throws IOException {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            // Older devices returns completely wrong duration for OGG files.
            player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_MP3));
        } else {
            // MP3 format should not be used for this test
            // because MP3 file cannot handle duration info correctly.
            player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_OGG));
        }
    }

    //
    // Test Methods
    //
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        failNotOverrided();
    }

    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        failNotOverrided();
    }

    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        failNotOverrided();
    }

    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        failNotOverrided();
    }

    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        failNotOverrided();
    }

    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        failNotOverrided();
    }

    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        failNotOverrided();
    }

    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        failNotOverrided();
    }

    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        failNotOverrided();
    }

    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        failNotOverrided();
    }

    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        failNotOverrided();
    }
}
