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

import java.io.IOException;

import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.utils.CompletionListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.utils.ErrorListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;

public class BasicMediaPlayerTestCase_ResetMethod
        extends BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        return BasicMediaPlayerTestCaseBase.buildBasicTestSuite(
                BasicMediaPlayerTestCase_ResetMethod.class, factoryClazz);
    }

    public BasicMediaPlayerTestCase_ResetMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsNoErrorsWithCheckIsIdleState(IBasicMediaPlayer player)
            throws IOException {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        // check no errors
        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.reset();

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            // XXX Galaxy S2, Nexus 5, Nexus 10, CyanogenMod 11 fails on this
            // assertion (what = 1, error = -2147483648/-107)
            fail(comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());

        // check is idle state
        player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_MP3));
    }

    // private void expectsErrorCallback(IBasicMediaPlayer player)
    // throws IOException {
    // Object sharedSyncObj = new Object();
    // ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
    // CompletionListenerObject comp = new
    // CompletionListenerObject(sharedSyncObj);
    //
    // // check no errors
    // player.setOnErrorListener(err);
    // player.setOnCompletionListener(comp);
    //
    // player.reset();
    //
    // if (!comp.await(SHORT_EVENT_WAIT_DURATION)) {
    // fail();
    // }
    //
    // assertTrue(comp.occurred());
    // assertTrue(err.occurred());
    //
    // // check is idle state
    // player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_MP3));
    // }

    private void expectsIllegalStateException(IBasicMediaPlayer player) {
        try {
            player.reset();
        } catch (IllegalStateException e) {
            // expected
            return;
        }
        fail();
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        // XXX This test may fail on StandardMediaPlayer

        // expectsErrorCallback(player);
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrorsWithCheckIsIdleState(player);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }
}
