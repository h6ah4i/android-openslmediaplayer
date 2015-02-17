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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SeekToMethod
        extends BasicMediaPlayerStateTestCaseBase {

    private static enum SeekPosition {
        MINUS,
        ZERO,
        MIDDLE,
        END,
        OVERRUN,
    }

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        TestSuite suite = new TestSuite();

        // Common test
        suite.addTest(ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SeekToMethod.class,
                new TestParams(factoryClazz, SeekPosition.ZERO)));

        // Additional tests
        String[] testMethods = new String[] {
                "testStatePrepared",
                "testStateStarted",
                "testStatePaused",
                "testStatePlaybackCompleted",
        };

        List<SeekPosition> seekPositions = new ArrayList<SeekPosition>();
        seekPositions.addAll(Arrays.asList(SeekPosition.values()));
        seekPositions.remove(SeekPosition.ZERO);

        for (String testMethod : testMethods) {
            for (SeekPosition seekPosition : seekPositions) {
                suite.addTest(makeTestCase(factoryClazz, testMethod, seekPosition));
            }
        }

        return suite;
    }

    private static TestCase makeTestCase(
            Class<? extends IMediaPlayerFactory> factoryClazz,
            String methodName, SeekPosition seekPosition) {
        return new BasicMediaPlayerTestCase_SeekToMethod(
                new ParameterizedTestArgs(
                        methodName,
                        new TestParams(factoryClazz, seekPosition)
                ));
    }

    private static final class TestParams extends BasicTestParams {
        private final SeekPosition mSeekPosition;

        public TestParams(Class<? extends IMediaPlayerFactory> factoryClass,
                SeekPosition seekPosition) {
            super(factoryClass);
            mSeekPosition = seekPosition;
        }

        public SeekPosition getSeekPosition() {
            return mSeekPosition;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mSeekPosition.name();
        }
    }

    public BasicMediaPlayerTestCase_SeekToMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsNoErrors(IBasicMediaPlayer player, int msec) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.seekTo(msec);

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            // XXX SH-02E (StandardMediaPlayer &
            // ErrorBeforePreparedAndAfterReset) fails on this assertion)
            fail(comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
    }

    private void expectsNoErrorsWithPlaybackCompletion(IBasicMediaPlayer player, int msec) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.seekTo(msec);

        if (!comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(comp + ", " + err);
        }

        assertTrue(comp.occurred());
        assertFalse(err.occurred());
    }

    private void expectsErrorCallback(IBasicMediaPlayer player, int msec) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.seekTo(msec);

        if (!comp.await(determineWaitCompletionTime(player))) {
            fail();
        }

        assertTrue(comp.occurred());
        assertTrue(err.occurred());
    }

    private void expectsIllegalStateException(IBasicMediaPlayer player, int msec) {
        try {
            player.seekTo(msec);
        } catch (IllegalStateException e) {
            return;
        }
        fail();
    }

    private static int getSeekPosition(IBasicMediaPlayer player, TestParams params) {
        switch (params.getSeekPosition()) {
            case MINUS:
                return -1;
            case ZERO:
                return 0;
            case MIDDLE:
                return (safeGetDuration(player, DURATION_LIMIT)) / 2;
            case END:
                return safeGetDuration(player, DURATION_LIMIT);
            case OVERRUN:
                return safeGetDuration(player, DURATION_LIMIT) + 1;
            default:
                throw new IllegalArgumentException();
        }
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {

        switch (((TestParams) args).getSeekPosition()) {
            case MINUS:
            case ZERO:
            case MIDDLE:
                expectsNoErrors(player, getSeekPosition(player, (TestParams) args));
                break;
            case END:
            case OVERRUN:
                expectsNoErrorsWithPlaybackCompletion(player, getSeekPosition(player, (TestParams) args));
                break;
        }
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsErrorCallback(player, getSeekPosition(player, (TestParams) args));
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, getSeekPosition(player, (TestParams) args));
    }
}
