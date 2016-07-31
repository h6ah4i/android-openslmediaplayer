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


package com.h6ah4i.android.media.openslmediaplayer.methodtest;

import java.util.ArrayList;
import java.util.List;

import junit.framework.TestSuite;
import android.media.AudioManager;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.utils.CompletionListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.utils.ErrorListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetAudioStreamTypeMethod
        extends BasicMediaPlayerStateTestCaseBase {

    private static final int[] STERAM_TYPES = new int[] {
            AudioManager.STREAM_ALARM,
            AudioManager.STREAM_DTMF,
            AudioManager.STREAM_MUSIC,
            AudioManager.STREAM_NOTIFICATION,
            AudioManager.STREAM_RING,
            AudioManager.STREAM_SYSTEM,
            AudioManager.STREAM_VOICE_CALL
    };

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();

        for (int streamtype : STERAM_TYPES) {
            params.add(new TestParams(factoryClazz, streamtype));
        }

        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SetAudioStreamTypeMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        private final int mStreamType;

        public TestParams(Class<? extends IMediaPlayerFactory> factoryClass, int streamtype) {
            super(factoryClass);
            mStreamType = streamtype;
        }

        public int getStreamType() {
            return mStreamType;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + streamTypeToString(mStreamType);
        }

        private static String streamTypeToString(int streamtype) {
            switch (streamtype) {
                case AudioManager.STREAM_ALARM:
                    return "ALARM";
                case AudioManager.STREAM_DTMF:
                    return "DTMF";
                case AudioManager.STREAM_MUSIC:
                    return "MUSIC";
                case AudioManager.STREAM_NOTIFICATION:
                    return "NOTIFICATION";
                case AudioManager.STREAM_RING:
                    return "RING";
                case AudioManager.STREAM_SYSTEM:
                    return "SYSTEM";
                case AudioManager.STREAM_VOICE_CALL:
                    return "VOICE_CALL";
                default:
                    return "Unknown stream type; " + streamtype;
            }
        }
    }

    public BasicMediaPlayerTestCase_SetAudioStreamTypeMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsNoErrors(IBasicMediaPlayer player, TestParams params) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.setAudioStreamType(params.getStreamType());

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(params + ", " + comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
    }

    private void expectsErrorCallback(IBasicMediaPlayer player, TestParams params) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.setAudioStreamType(params.getStreamType());

        if (!comp.await(determineWaitCompletionTime(player))) {
            fail(params + ", " + comp + ", " + err);
        }

        assertTrue(comp.occurred());
        assertTrue(err.occurred());
    }

    private void expectsErrorCallbackExceptForMUSIC(IBasicMediaPlayer player, TestParams params) {
        switch (params.getStreamType()) {
            case AudioManager.STREAM_MUSIC:
                expectsNoErrors(player, params);
                break;
            default:
                expectsErrorCallback(player, params);
                break;
        }
    }

    private void expectsIllegalStateException(IBasicMediaPlayer player, TestParams params) {
        try {
            player.setAudioStreamType(params.getStreamType());
        } catch (IllegalStateException e) {
            return;
        }
        fail(params.toString());
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallbackExceptForMUSIC(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallbackExceptForMUSIC(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallbackExceptForMUSIC(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsErrorCallbackExceptForMUSIC(player, (TestParams) args);
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
        expectsIllegalStateException(player, (TestParams) args);
    }
}
