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
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;

public class BasicMediaPlayerTestCase_PrepareMethod
        extends BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        return BasicMediaPlayerTestCaseBase.buildBasicTestSuite(
                BasicMediaPlayerTestCase_PrepareMethod.class, factoryClazz);
    }

    public BasicMediaPlayerTestCase_PrepareMethod(ParameterizedTestArgs args) {
        super(args);
    }

    protected void expectsNoExceptions(IBasicMediaPlayer player) throws IOException {
        player.prepare();
    }

    protected void expectsIllegalStateException(IBasicMediaPlayer player) throws IOException {
        try {
            player.prepare();
        } catch (IllegalStateException e) {
            // expected
            return;
        }
        fail();
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoExceptions(player);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoExceptions(player);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsIllegalStateException(player);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsIllegalStateException(player);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsIllegalStateException(player);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }
}
