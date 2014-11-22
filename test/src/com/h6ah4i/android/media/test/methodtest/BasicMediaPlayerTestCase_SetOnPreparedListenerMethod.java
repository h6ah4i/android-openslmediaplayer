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

import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.test.utils.BasicMediaPlayerEventListenerObject;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.media.test.utils.PreparedListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;

public class BasicMediaPlayerTestCase_SetOnPreparedListenerMethod
        extends BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        return BasicMediaPlayerTestCaseBase.buildBasicTestSuite(
                BasicMediaPlayerTestCase_SetOnPreparedListenerMethod.class, factoryClazz);
    }

    public BasicMediaPlayerTestCase_SetOnPreparedListenerMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsPreparedCallback(IBasicMediaPlayer player) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);
        PreparedListenerObject prepared = new PreparedListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);
        player.setOnPreparedListener(prepared);

        if (!BasicMediaPlayerEventListenerObject.awaitAny(30000, err, comp, prepared)) {
            fail();
        }

        player.setOnErrorListener(null);
        player.setOnCompletionListener(null);
        player.setOnPreparedListener(null);

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
        assertTrue(prepared.occurred());
    }

    private void expectsNoErrors(IBasicMediaPlayer player) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);
        PreparedListenerObject prepared = new PreparedListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);
        player.setOnPreparedListener(prepared);

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail();
        }

        player.setOnErrorListener(null);
        player.setOnCompletionListener(null);
        player.setOnPreparedListener(null);

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
        assertFalse(prepared.occurred());
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsPreparedCallback(player);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player);
    }
}
