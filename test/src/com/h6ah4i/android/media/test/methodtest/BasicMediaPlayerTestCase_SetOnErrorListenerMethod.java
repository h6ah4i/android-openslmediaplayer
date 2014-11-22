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
import java.util.List;

import junit.framework.TestSuite;
import android.media.AudioManager;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetOnErrorListenerMethod
        extends BasicMediaPlayerStateTestCaseBase {

    enum TestMethod {
        GET_DURATION,
        PAUSE,
        SET_AUDIO_STREAM_TYPE
    }

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();

        params.add(new TestParams(factoryClazz, false));
        params.add(new TestParams(factoryClazz, true));

        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SetOnErrorListenerMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        private final boolean mErrorCallbackReturnValue;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                boolean errorCallbackReturnValue) {
            super(factoryClass);
            mErrorCallbackReturnValue = errorCallbackReturnValue;
        }

        public boolean getErrorCallbackReturnValue() {
            return mErrorCallbackReturnValue;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mErrorCallbackReturnValue;
        }
    }

    public BasicMediaPlayerTestCase_SetOnErrorListenerMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsErrorCallback(IBasicMediaPlayer player, TestParams params, TestMethod method) {
        final String failmsg = "method = " + method;
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(
                sharedSyncObj, params.getErrorCallbackReturnValue());
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        switch (method) {
            case GET_DURATION:
                player.getDuration();
                break;
            case PAUSE:
                player.pause();
                break;
            case SET_AUDIO_STREAM_TYPE:
                player.setAudioStreamType(AudioManager.STREAM_NOTIFICATION);
                break;
            default:
                fail(failmsg);
        }

        if (!err.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(failmsg);
        }

        try {
            // wait for onCompletion callback
            Thread.sleep(SHORT_EVENT_WAIT_DURATION);
        } catch (InterruptedException e) {
        }

        player.setOnErrorListener(null);
        player.setOnCompletionListener(null);

        assertTrue(failmsg, err.occurred());

        if (params.getErrorCallbackReturnValue()) {
            // onCompletion should not be invoked
            assertFalse(failmsg, comp.occurred());
        } else {
            assertTrue(failmsg, comp.occurred());
        }
    }

    private void getDurationAndExpectsErrorCallback(IBasicMediaPlayer player, TestParams fixture) {
        expectsErrorCallback(player, fixture, TestMethod.GET_DURATION);
    }

    private void pauseAndExpectsErrorCallback(IBasicMediaPlayer player, TestParams fixture) {
        expectsErrorCallback(player, fixture, TestMethod.PAUSE);
    }

    private void setAudioStreamTypeAndExpectsErrorCallback(IBasicMediaPlayer player,
            TestParams fixture) {
        expectsErrorCallback(player, fixture, TestMethod.SET_AUDIO_STREAM_TYPE);
    }

    private void expectsNoErrors(IBasicMediaPlayer player, TestParams fixture) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(
                sharedSyncObj, fixture.getErrorCallbackReturnValue());
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(comp + ", " + err);
        }

        try {
            // wait for onCompletion callback
            Thread.sleep(SHORT_EVENT_WAIT_DURATION);
        } catch (InterruptedException e) {
        }

        player.setOnErrorListener(null);
        player.setOnCompletionListener(null);

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        getDurationAndExpectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        getDurationAndExpectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        getDurationAndExpectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        pauseAndExpectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        setAudioStreamTypeAndExpectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        setAudioStreamTypeAndExpectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        pauseAndExpectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        setAudioStreamTypeAndExpectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }
}
