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

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetVolumeMethod
        extends BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();

        params.add(new TestParams(factoryClazz, 0.0f, 0.0f));
        params.add(new TestParams(factoryClazz, 1.0f, 0.0f));
        params.add(new TestParams(factoryClazz, 0.0f, 1.0f));
        params.add(new TestParams(factoryClazz, -0.1f, 0.0f));
        params.add(new TestParams(factoryClazz, 0.0f, -0.1f));
        params.add(new TestParams(factoryClazz, 1.1f, 0.0f));
        params.add(new TestParams(factoryClazz, 0.0f, 1.1f));
        params.add(new TestParams(factoryClazz, Float.MIN_VALUE, 0.0f));
        params.add(new TestParams(factoryClazz, 0.0f, Float.MIN_VALUE));
        params.add(new TestParams(factoryClazz, Float.MAX_VALUE, 0.0f));
        params.add(new TestParams(factoryClazz, 0.0f, Float.MAX_VALUE));
        params.add(new TestParams(factoryClazz, Float.NaN, 0.0f));
        params.add(new TestParams(factoryClazz, 0.0f, Float.NaN));
        params.add(new TestParams(factoryClazz, Float.NEGATIVE_INFINITY, 0.0f));
        params.add(new TestParams(factoryClazz, 0.0f, Float.NEGATIVE_INFINITY));
        params.add(new TestParams(factoryClazz, Float.POSITIVE_INFINITY, 0.0f));
        params.add(new TestParams(factoryClazz, 0.0f, Float.POSITIVE_INFINITY));

        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SetVolumeMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        public final float left;
        public final float right;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                float left, float right) {
            super(factoryClass);
            this.left = left;
            this.right = right;
        }

        @Override
        public String toString() {
            return super.toString() + ", L=" + left + ", R=" + right;
        }
    }

    public BasicMediaPlayerTestCase_SetVolumeMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsNoErrors(IBasicMediaPlayer player, TestParams params) throws IOException {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        // check no errors
        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.setVolume(params.left, params.right);

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
    }

    private void expectsIllegalStateException(IBasicMediaPlayer player, TestParams params) {
        try {
            player.setVolume(params.left, params.right);
        } catch (IllegalStateException e) {
            // expected
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
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player, (TestParams) args);
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
