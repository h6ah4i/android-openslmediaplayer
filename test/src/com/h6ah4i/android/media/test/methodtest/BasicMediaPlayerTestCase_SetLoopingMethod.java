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


package com.h6ah4i.android.media.test.methodtest;

import java.io.IOException;

import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.media.test.utils.SeekCompleteListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;

public class BasicMediaPlayerTestCase_SetLoopingMethod
        extends BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        return BasicMediaPlayerTestCaseBase.buildBasicTestSuite(
                BasicMediaPlayerTestCase_SetLoopingMethod.class, factoryClazz);
    }

    public BasicMediaPlayerTestCase_SetLoopingMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsNoErrors(IBasicMediaPlayer player, boolean verifyIsLooping)
            throws IOException {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        // check no errors
        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.setLooping(true);
        if (verifyIsLooping) {
            assertTrue(player.isLooping());
        }
        player.setLooping(false);
        if (verifyIsLooping) {
            assertFalse(player.isLooping());
        }

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
    }

    private void expectsNoErrorsAndCheckLoopingWorksProperly(IBasicMediaPlayer player,
            boolean startPlayback)
            throws IOException {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        // check no errors
        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        int duration = player.getDuration();
        player.setLooping(true);
        assertTrue(player.isLooping());

        if (startPlayback) {
            player.start();
        }

        if (comp.await(duration * 3)) {
            fail(comp + ", " + err);
        }

        assertFalse(err.occurred());

        player.setLooping(false);
        assertFalse(player.isLooping());

        if (!comp.await(duration * 3)) {
            fail(comp + ", " + err);
        }

        assertTrue(comp.occurred());
        assertFalse(err.occurred());
    }

    private void expectsIllegalStateException(IBasicMediaPlayer player) {
        try {
            player.setLooping(true);
        } catch (IllegalStateException e) {
            // expected
            try {
                player.setLooping(false);
            } catch (IllegalStateException e2) {
                // expected
                return;
            }
        }
        fail();
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, true);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, true);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, true);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsAndCheckLoopingWorksProperly(player, true);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsAndCheckLoopingWorksProperly(player, false);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsAndCheckLoopingWorksProperly(player, true);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, true);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrorsAndCheckLoopingWorksProperly(player, true);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player, false);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player, false);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }

    // "NextPlayer is preferred than setLooping(true) "
    // https://github.com/h6ah4i/android-openslmediaplayer/issues/1
    public void testLoopingIsMorePreferredThanNextPlayer() throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = createWrappedPlayerInstance();
        Object args = getTestParams();

        try {
            Object syncObj = new Object();
            CompletionListenerObject comp = new CompletionListenerObject(syncObj);
            SeekCompleteListenerObject seek = new SeekCompleteListenerObject(syncObj);
            ErrorListenerObject error = new ErrorListenerObject(syncObj, false);

            // prepare
            transitStateToPrepared(player, args);
            transitStateToPrepared(nextPlayer, args);

            // set listeners
            player.setOnCompletionListener(comp);
            player.setOnSeekCompleteListener(seek);
            player.setOnErrorListener(error);

            // set next player
            player.setNextMediaPlayer(unwrap(nextPlayer));

            // setLooping(true)
            player.setLooping(true);

            // start
            player.start();

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait for looping
            int duration = player.getDuration();
            Thread.sleep(duration + 2000);

            // check conditions
            assertTrue(player.isPlaying());         // still playing
            assertFalse(nextPlayer.isPlaying());    // not started (looping is preferred)

            assertTrue(seek.occurred());    // onSeekCompletion() callback occurs when looping
            assertFalse(comp.occurred());   // check Lollipop bug with NuPlayer
            assertFalse(error.occurred());  // no error occurred
        } finally {
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

}
