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

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetAuxEffectSendLevelMethod
        extends BasicMediaPlayerStateTestCaseBase {
    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();

        params.add(new TestParams(factoryClazz, 0.0f, true));
        params.add(new TestParams(factoryClazz, 1.0f, true));
        params.add(new TestParams(factoryClazz, -0.1f, false));
        params.add(new TestParams(factoryClazz, 1.1f, false));
        params.add(new TestParams(factoryClazz, Float.MIN_VALUE, true));
        params.add(new TestParams(factoryClazz, Float.MAX_VALUE, false));
        params.add(new TestParams(factoryClazz, Float.NEGATIVE_INFINITY, false));
        params.add(new TestParams(factoryClazz, Float.POSITIVE_INFINITY, false));
        params.add(new TestParams(factoryClazz, Float.NaN, false));

        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SetAuxEffectSendLevelMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        public final float level;
        public final boolean valid;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                float level, boolean valid) {
            super(factoryClass);
            this.level = level;
            this.valid = valid;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + level + ", " + valid;
        }
    }

    public BasicMediaPlayerTestCase_SetAuxEffectSendLevelMethod(ParameterizedTestArgs args) {
        super(args);
    }

    @Override
    protected IMediaPlayerFactory onCreateFactory() {
        return ((TestParams) getTestParams()).createFactory(getContext());
    }

    private void expectsNoErrors(IBasicMediaPlayer player, TestParams params) {
        final String failmsg = "Level = " + params.level;
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.setAuxEffectSendLevel(params.level);

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(failmsg + ", " + comp + ", " + err);
        }

        assertFalse(failmsg, comp.occurred());
        assertFalse(failmsg, err.occurred());
    }

    private void expectsErrorCallback(IBasicMediaPlayer player, TestParams params) {
        final String failmsg = "Level = " + params.level;
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.setAuxEffectSendLevel(params.level);

        if (!comp.await(determineWaitCompletionTime(player))) {
            // expects err.what = -22 (-EINVAL)
            fail(failmsg + ", " + comp + ", " + err);
        }

        assertTrue(failmsg, comp.occurred());
        assertTrue(failmsg, err.occurred());
    }

    private void expectsErrorCallbackWhenIllegalValue(IBasicMediaPlayer player, TestParams params) {
        if (params.valid) {
            expectsNoErrors(player, params);
        } else {
            expectsErrorCallback(player, params);
        }
    }

    private void expectsIllegalStateException(IBasicMediaPlayer player, TestParams params) {
        try {
            player.setAuxEffectSendLevel(params.level);
        } catch (IllegalStateException e) {
            return;
        }
        fail();
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
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        // expectsErrorCallbackWhenIllegalValue(player, (TestParams) args);
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallbackWhenIllegalValue(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        // XXX: On Android 5.0 with StandardMediaPlayer, error callback won't be
        // called
        expectsErrorCallbackWhenIllegalValue(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallbackWhenIllegalValue(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsErrorCallbackWhenIllegalValue(player, (TestParams) args);
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
