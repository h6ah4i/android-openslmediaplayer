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
import java.util.ArrayList;
import java.util.List;

import junit.framework.TestSuite;

import android.os.Build;
import android.util.Log;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.base.TestBasicMediaPlayerWrapper;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.media.test.utils.MockBasicMediaPlayer;
import com.h6ah4i.android.media.test.utils.SeekCompleteListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetNextMediaPlayerMethod extends
        BasicMediaPlayerStateTestCaseBase {

    private static final boolean SKIP_HANGUP_SUSPECTED_TEST_CASE =
            (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH);

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        TestSuite suite = new TestSuite();

        String[] testsWithBasicTestParams = new String[] {
                "testCallbacksOnNextPlayerStart",
                "testIsPlayingTransition",
                "testSeekToBeforeAttachedAsNextMediaPlayer",
                "testSeekToAfterAttachedAsNextMediaPlayer",
                "testPlayingStateAsNextMediaPlayer",
                "testReinitializeNextMediaPlayer",
                "testSetNextMediaPlayerAfterCompletion",
                "testPausedStateAsNextMediaPlayer",
                "testMultipleSetNextAudioPlayer_1",
                "testMultipleSetNextAudioPlayer_2",
        };

        {
            List<TestParams> params = new ArrayList<TestParams>();

            for (NextPlayerType nextPlayerType : NextPlayerType.values()) {
                params.add(new TestParams(factoryClazz, nextPlayerType));
            }

            suite.addTest(ParameterizedTestSuiteBuilder
                    .buildDetail(BasicMediaPlayerTestCase_SetNextMediaPlayerMethod.class,
                            params,
                            ParameterizedTestSuiteBuilder.notMatches(testsWithBasicTestParams),
                            true));
        }


        {

            ParameterizedTestSuiteBuilder.Filter filter =
                    ParameterizedTestSuiteBuilder.matches(testsWithBasicTestParams);
            List<BasicTestParams> params = new ArrayList<BasicTestParams>();

            params.add(new BasicTestParams(factoryClazz));

            suite.addTest(ParameterizedTestSuiteBuilder.buildDetail(
                    BasicMediaPlayerTestCase_SetNextMediaPlayerMethod.class,
                    params, filter, true));
        }

        return suite;
    }

    enum NextPlayerType {
        NULL, THIS, MOCK_CLASS, IDLE, INITIALIZED, PREPARING, PREPARED, STARTED, PAUSED, STOPPED, PLAYBACK_COMPLETED, ERROR_BEFORE_PREPARED, ERROR_AFTER_PREPARED, END
    }

    private static final class TestParams extends BasicTestParams {
        private final NextPlayerType mNextPlayerType;

        public TestParams(Class<? extends IMediaPlayerFactory> factoryClass,
                NextPlayerType nextPlayerType) {
            super(factoryClass);
            mNextPlayerType = nextPlayerType;
        }

        public NextPlayerType getNextPlayerType() {
            return mNextPlayerType;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mNextPlayerType.name();
        }
    }

    public BasicMediaPlayerTestCase_SetNextMediaPlayerMethod(
            ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsNoErrors(IBasicMediaPlayer player,
            IBasicMediaPlayer next, NextPlayerType type) throws IOException {
        final String failmsg = "NextPlayerType = " + type;

        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(
                sharedSyncObj);

        // check no errors
        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        setUnwrappedtNextPlayer(player, next);

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(failmsg + ", " + comp + ", " + err);
        }

        assertFalse(failmsg, comp.occurred());
        assertFalse(failmsg, err.occurred());
    }

    private void expectsIllegalArgumentException(IBasicMediaPlayer player,
            IBasicMediaPlayer next, NextPlayerType type) {
        final String failmsg = "NextPlayerType = " + type;
        try {
            setUnwrappedtNextPlayer(player, next);
        } catch (IllegalArgumentException e) {
            // expected
            return;
        }
        fail(failmsg);
    }


    private void expectsIllegalStateException(IBasicMediaPlayer player,
                                                 IBasicMediaPlayer next, NextPlayerType type) {
        final String failmsg = "NextPlayerType = " + type;
        try {
            setUnwrappedtNextPlayer(player, next);
        } catch (IllegalStateException e) {
            // expected
            return;
        }
        fail(failmsg);
    }

    private void setUnwrappedtNextPlayer(IBasicMediaPlayer player,
            IBasicMediaPlayer next) {
        player.setNextMediaPlayer(unwrap(next));
    }

    private IBasicMediaPlayer createNextPlayer(IBasicMediaPlayer player,
            NextPlayerType type) throws IOException {
        switch (type) {
            case NULL:
                return null;
            case THIS:
                return ((TestBasicMediaPlayerWrapper) player).getWrappedInstance();
            case MOCK_CLASS:
                return new MockBasicMediaPlayer();
            case IDLE: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToIdle(wrapper, null);
                return wrapper;
            }
            case INITIALIZED: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToInitialized(wrapper, null);
                return wrapper;
            }
            case PREPARING: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToPreparing(wrapper, null);
                return wrapper;
            }
            case PREPARED: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToPrepared(wrapper, null);
                return wrapper;
            }
            case STARTED: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToStarted(wrapper, null);
                return wrapper;
            }
            case PAUSED: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToPaused(wrapper, null);
                return wrapper;
            }
            case STOPPED: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToStopped(wrapper, null);
                return wrapper;
            }
            case PLAYBACK_COMPLETED: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToPlaybackCompleted(wrapper, null);
                return wrapper;
            }
            case ERROR_BEFORE_PREPARED: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToErrorBeforePrepared(wrapper, null);
                return wrapper;
            }
            case ERROR_AFTER_PREPARED: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToErrorAfterPrepared(wrapper, null);
                return wrapper;
            }
            case END: {
                TestBasicMediaPlayerWrapper wrapper = createWrappedPlayerInstance();
                transitStateToEnd(wrapper, null);
                return wrapper;
            }
            default:
                throw new IllegalArgumentException();
        }
    }

    private void checkValidState(IBasicMediaPlayer player, Object args)
            throws IOException {
        TestParams params = (TestParams) args;
        NextPlayerType type = params.getNextPlayerType();
        IBasicMediaPlayer next = createNextPlayer(player, type);

        try {
            switch (type) {
                case THIS:
                case MOCK_CLASS:
                    expectsIllegalArgumentException(player, next, type);
                    break;
                case NULL:
                case PREPARED:
                case PAUSED:
                case PLAYBACK_COMPLETED:
                    expectsNoErrors(player, next, type);
                    break;
                case IDLE:
                case INITIALIZED:
                case PREPARING:
                case STARTED:
                case STOPPED:
                case ERROR_BEFORE_PREPARED:
                case ERROR_AFTER_PREPARED:
                case END:
                    expectsIllegalStateException(player, next, type);
                    break;
                default:
                    fail();
                    break;
            }
        } finally {
            releaseQuietly(next);
        }
    }

    private void checkInvalidState(IBasicMediaPlayer player, Object args)
            throws IOException {
        TestParams params = (TestParams) args;
        NextPlayerType type = params.getNextPlayerType();
        IBasicMediaPlayer next = createNextPlayer(player, type);

        try {
            switch (type) {
                case THIS:
                case MOCK_CLASS:
                    expectsIllegalArgumentException(player, next, type);
                    break;
                default:
                    expectsIllegalStateException(player, next, type);
                    break;
            }
        } finally {
            releaseQuietly(next);
        }
    }

    private void checkEndState(IBasicMediaPlayer player, Object args)
            throws IOException {
        TestParams params = (TestParams) args;
        NextPlayerType type = params.getNextPlayerType();
        IBasicMediaPlayer next = createNextPlayer(player, type);

        try {
            expectsIllegalStateException(player, next, type);
        } finally {
            releaseQuietly(next);
        }
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args)
            throws Throwable {
        checkInvalidState(player, args);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args)
            throws Throwable {
        checkValidState(player, args);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args)
            throws Throwable {
        checkValidState(player, args);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        checkValidState(player, args);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        checkValidState(player, args);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args)
            throws Throwable {
        checkValidState(player, args);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args)
            throws Throwable {
        checkValidState(player, args);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player,
            Object args) throws Throwable {
        checkValidState(player, args);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player,
            Object args) throws Throwable {
        checkInvalidState(player, args);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player,
            Object args) throws Throwable {
        checkInvalidState(player, args);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args)
            throws Throwable {
        checkEndState(player, args);
    }

    public void testCallbacksOnNextPlayerStart() throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        try {
            Object syncObj1 = new Object();
            CompletionListenerObject comp1 = new CompletionListenerObject(syncObj1);
            SeekCompleteListenerObject seek1 = new SeekCompleteListenerObject(syncObj1);
            ErrorListenerObject error1 = new ErrorListenerObject(syncObj1, false);

            Object syncObj2 = new Object();
            CompletionListenerObject comp2 = new CompletionListenerObject(syncObj2);
            SeekCompleteListenerObject seek2 = new SeekCompleteListenerObject(syncObj2);
            ErrorListenerObject error2 = new ErrorListenerObject(syncObj2, false);

            // prepare
            transitStateToPrepared(player, args);
            nextPlayer = createNextPlayer(player, NextPlayerType.PREPARED);
            setUnwrappedtNextPlayer(player, nextPlayer);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // set listener objects
            player.setOnCompletionListener(comp1);
            player.setOnSeekCompleteListener(seek1);
            player.setOnErrorListener(error1);

            nextPlayer.setOnCompletionListener(comp2);
            nextPlayer.setOnSeekCompleteListener(seek2);
            nextPlayer.setOnErrorListener(error2);

            // start
            player.start();

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            Thread.sleep(SHORT_EVENT_WAIT_DURATION);

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait for the next player started
            int duration1 = player.getDuration();
            Thread.sleep(duration1);

            assertFalse(player.isPlaying());
            assertTrue(nextPlayer.isPlaying());

            // check listener objects
            assertTrue(comp1.occurred());
            assertFalse(seek1.occurred());
            assertFalse(error1.occurred());

            // wait for the next player completion
            int duration2 = nextPlayer.getDuration();
            Thread.sleep(duration2 + SHORT_EVENT_WAIT_DURATION);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // check listener objects
            assertTrue(comp2.occurred());
            assertFalse(seek2.occurred());
            assertFalse(error2.occurred());
        } finally {
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

    public void testIsPlayingTransition() throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        try {
            // prepare
            transitStateToPrepared(player, args);
            nextPlayer = createNextPlayer(player, NextPlayerType.PREPARED);
            setUnwrappedtNextPlayer(player, nextPlayer);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // start
            player.start();

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            Thread.sleep(SHORT_EVENT_WAIT_DURATION);

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait for the next player started
            int duration1 = player.getDuration();
            Thread.sleep(duration1);

            assertFalse(player.isPlaying());
            assertTrue(nextPlayer.isPlaying());

            // wait for the next player completion
            int duration2 = nextPlayer.getDuration();
            Thread.sleep(duration2 + SHORT_EVENT_WAIT_DURATION);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());
        } finally {
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

    // NOTE: This test case may fails with StandardMediaPlayer (affects: ICS to JB, Lollipop with NuPlayer)
    public void testSeekToBeforeAttachedAsNextMediaPlayer() throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        // NOTE:
        // seekTo() call will be ignored if the player is attached as a next
        // player.

        try {
            SeekCompleteListenerObject seekComp = new SeekCompleteListenerObject();

            // prepare
            transitStateToPrepared(player, args);
            nextPlayer = createNextPlayer(player, NextPlayerType.PREPARED);

            nextPlayer.setOnSeekCompleteListener(seekComp);

            // seek
            int seekPosition = nextPlayer.getDuration() / 2;

            nextPlayer.seekTo(seekPosition);

            if (!seekComp.await(DEFAULT_EVENT_WAIT_DURATION)) {
                fail();
            }

            // NOTE: valid position is returned
            assertEquals(seekPosition, nextPlayer.getCurrentPosition());

            setUnwrappedtNextPlayer(player, nextPlayer);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // start
            player.start();

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            Thread.sleep(SHORT_EVENT_WAIT_DURATION);

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait for the next player started
            Thread.sleep(player.getDuration());

            // NOTE: StandardMediaPlayer fails on this assertion if using NuPlayer
            assertEquals(player.getDuration(), player.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertTrue(nextPlayer.isPlaying());

            // NOTE: valid current position position is returned
            assertLargerThanOrEqual(seekPosition,
                    nextPlayer.getCurrentPosition());

            // wait for the next player completion
            Thread.sleep(nextPlayer.getDuration() - seekPosition
                    + SHORT_EVENT_WAIT_DURATION);

            if (true) {
                Log.w("XXX",
                        "testSeekToBeforeAttachedAsNextMediaPlayer() - WORKAROUND");

                // WTF! Android's native audio decoder gives very poor seek
                // accuracy!
                assertFalse(player.isPlaying());
                assertTrue(nextPlayer.isPlaying());

                // wait for the next player completion
                Thread.sleep(seekPosition);
            }

            assertEquals(nextPlayer.getDuration(),
                    nextPlayer.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());
        } finally {
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

    // NOTE: This test case may fails with StandardMediaPlayer (affects: ICS to JB, Lollipop with NuPlayer)
    public void testSeekToAfterAttachedAsNextMediaPlayer() throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        // NOTE:
        // seekTo() call will be ignored if the player is attached as a next
        // player.

        try {
            // prepare
            transitStateToPrepared(player, args);
            nextPlayer = createNextPlayer(player, NextPlayerType.PREPARED);
            setUnwrappedtNextPlayer(player, nextPlayer);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // start
            player.start();

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            int seekPosition = nextPlayer.getDuration() / 2;

            SeekCompleteListenerObject seekComp = new SeekCompleteListenerObject();
            nextPlayer.setOnSeekCompleteListener(seekComp);
            nextPlayer.seekTo(seekPosition);

            if (!seekComp.await(DEFAULT_EVENT_WAIT_DURATION)) {
                fail();
            }

            // NOTE: valid position is returned
            assertEquals(seekPosition, nextPlayer.getCurrentPosition());

            Thread.sleep(SHORT_EVENT_WAIT_DURATION);

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait for the next player started
            Thread.sleep(player.getDuration());

            // NOTE: StandardMediaPlayer fails on this assertion if using NuPlayer
            assertEquals(player.getDuration(), player.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertTrue(nextPlayer.isPlaying());

            // NOTE: valid current position position is returned
            // XXX Galaxy S4 (SC-04E, Android 4.4.2) fails on this assertion
            assertLargerThanOrEqual(seekPosition,
                    nextPlayer.getCurrentPosition());

            // wait for the next player completion
            Thread.sleep(nextPlayer.getDuration() - seekPosition
                    + SHORT_EVENT_WAIT_DURATION);

            if (true) {
                Log.w("XXX",
                        "testSeekToAfterAttachedAsNextMediaPlayer() - WORKAROUND");

                // WTF! Android's native audio decoder gives very poor seek
                // accuracy!
                assertFalse(player.isPlaying());
                assertTrue(nextPlayer.isPlaying());

                // wait for the next player completion
                Thread.sleep(seekPosition);
            }

            assertEquals(nextPlayer.getDuration(),
                    nextPlayer.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());
        } finally {
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

    public void testPlayingStateAsNextMediaPlayer() throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        try {
            // prepare
            transitStateToPrepared(player, args);
            nextPlayer = createNextPlayer(player, NextPlayerType.PREPARED);

            player.setVolume(1.0f, 0.0f);
            nextPlayer.setVolume(0.0f, 1.0f);

            nextPlayer.start();
            Thread.sleep(safeGetDuration(nextPlayer, DURATION_LIMIT) / 2);

            try {
                setUnwrappedtNextPlayer(player, nextPlayer);
                fail();
            } catch (IllegalStateException e) {
                // expected
            }
        } finally {
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

    public void testPausedStateAsNextMediaPlayer() throws Throwable {
        if (!isOpenSL(getFactory()) && SKIP_HANGUP_SUSPECTED_TEST_CASE) {
            fail("This test case is skipped to avoid HANGUP");
        }

        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        try {
            // prepare
            transitStateToPrepared(player, args);
            nextPlayer = createNextPlayer(player, NextPlayerType.PREPARED);

            player.setVolume(1.0f, 0.0f);
            nextPlayer.setVolume(0.0f, 1.0f);

            if (isOpenSL(player)) {
                nextPlayer.start();
                Thread.sleep(50);
                nextPlayer.pause();

                Thread.sleep(2000); // wait fade-out completion
            } else {
                nextPlayer.start();
                Thread.sleep(player.getDuration() / 2);
                nextPlayer.pause();
            }

            setUnwrappedtNextPlayer(player, nextPlayer);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // start
            player.start();

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            Thread.sleep(player.getDuration() + SHORT_EVENT_WAIT_DURATION);

            assertEquals(player.getDuration(), player.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertTrue(nextPlayer.isPlaying());

            Thread.sleep(nextPlayer.getDuration() / 2
                    + SHORT_EVENT_WAIT_DURATION);

            assertEquals(nextPlayer.getDuration(),
                    nextPlayer.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());
        } finally {
            // NOTE:
            // StandardMediaPlayer may hang while calling the release() method...
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

    public void testSetNextMediaPlayerAfterCompletion() throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        try {
            // prepare
            transitStateToPrepared(player, args);
            nextPlayer = createNextPlayer(player, NextPlayerType.PREPARED);

            player.setVolume(1.0f, 0.0f);
            nextPlayer.setVolume(0.0f, 1.0f);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // start
            player.start();

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait
            Thread.sleep(player.getDuration() + DEFAULT_EVENT_WAIT_DURATION);
            assertEquals(player.getDuration(), player.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            setUnwrappedtNextPlayer(player, nextPlayer);

            Thread.sleep(SHORT_EVENT_WAIT_DURATION);

            assertEquals(0, nextPlayer.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());
        } finally {
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

    public void testReinitializeNextMediaPlayer() throws Throwable {
        IBasicMediaPlayer player = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        try {
            // prepare
            transitStateToPrepared(player, args);
            nextPlayer = createNextPlayer(player, NextPlayerType.PREPARED);

            player.setVolume(1.0f, 0.0f);
            nextPlayer.setVolume(0.0f, 1.0f);

            setUnwrappedtNextPlayer(player, nextPlayer);

            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // start
            player.start();

            assertTrue(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait 1/2
            Thread.sleep(player.getDuration() / 2);
            nextPlayer.reset();
            transitStateToPrepared(nextPlayer, null);

            Thread.sleep(player.getDuration() / 2 + SHORT_EVENT_WAIT_DURATION);

            // NOTE: StandardMediaPlayer fails on this assertion if using NuPlayer
            assertEquals(player.getDuration(), player.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertTrue(nextPlayer.isPlaying());

            Thread.sleep(nextPlayer.getDuration() + SHORT_EVENT_WAIT_DURATION);

            assertEquals(nextPlayer.getDuration(),
                    nextPlayer.getCurrentPosition());
            assertFalse(player.isPlaying());
            assertFalse(nextPlayer.isPlaying());
        } finally {
            releaseQuietly(nextPlayer);
            releaseQuietly(player);
        }
    }

    public void testMultipleSetNextAudioPlayer_1() throws Throwable {
        if (!isOpenSL(getFactory()) && SKIP_HANGUP_SUSPECTED_TEST_CASE) {
            fail("This test case is skipped to avoid HANGUP");
        }

        implMultipleSetNextAudioPlayer(0);
    }

    public void testMultipleSetNextAudioPlayer_2() throws Throwable {
        if (!isOpenSL(getFactory()) && SKIP_HANGUP_SUSPECTED_TEST_CASE) {
            fail("This test case is skipped to avoid HANGUP");
        }

        implMultipleSetNextAudioPlayer(1);
    }

    public void implMultipleSetNextAudioPlayer(int order) throws Throwable {
        IBasicMediaPlayer player1 = createWrappedPlayerInstance();
        IBasicMediaPlayer player2 = createWrappedPlayerInstance();
        IBasicMediaPlayer nextPlayer = null;
        Object args = getTestParams();

        // NOTE:
        // seekTo() call will be ignored if the player is attached as a next
        // player.

        try {
            // prepare
            transitStateToPrepared(player1, args);
            transitStateToPrepared(player2, args);

            nextPlayer = createNextPlayer(player1, NextPlayerType.PREPARED);

            if (order == 0) {
                setUnwrappedtNextPlayer(player1, nextPlayer);
                setUnwrappedtNextPlayer(player2, nextPlayer);
            } else {
                setUnwrappedtNextPlayer(player2, nextPlayer);
                setUnwrappedtNextPlayer(player1, nextPlayer);
            }

            nextPlayer.setVolume(0.5f, 0.51f);
            player1.setVolume(0.5f, 0.0f);
            player2.setVolume(0.0f, 0.5f);

            assertFalse(player1.isPlaying());
            assertFalse(player2.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // start player 1
            player1.start();

            assertTrue(player1.isPlaying());
            assertFalse(player2.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait 1/2
            Thread.sleep(player1.getDuration() / 2);

            assertTrue(player1.isPlaying());
            assertFalse(player2.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // start player 2
            player2.start();

            assertTrue(player1.isPlaying());
            assertTrue(player2.isPlaying());
            assertFalse(nextPlayer.isPlaying());

            // wait 1/2 + alpha
            Thread.sleep(player1.getDuration() / 2 + SHORT_EVENT_WAIT_DURATION);

            assertFalse(player1.isPlaying());
            assertTrue(player2.isPlaying());
            assertTrue(nextPlayer.isPlaying());

            // wait 1/2
            Thread.sleep(player1.getDuration() / 2);

            assertFalse(player1.isPlaying());
            assertFalse(player2.isPlaying());
            assertTrue(nextPlayer.isPlaying());

            // wait 1/2 + alpha
            Thread.sleep(player1.getDuration() / 2 + SHORT_EVENT_WAIT_DURATION);

            assertFalse(player1.isPlaying());
            assertFalse(player2.isPlaying());
            assertFalse(nextPlayer.isPlaying());
        } finally {
            // NOTE:
            // StandardMediaPlayer may hang while calling the release() method...
            releaseQuietly(nextPlayer);
            releaseQuietly(player2);
            releaseQuietly(player1);
        }
    }

}
