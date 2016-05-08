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

import java.io.File;
import java.io.IOException;

import android.net.Uri;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.utils.BasicMediaPlayerEventListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.utils.CommonTestCaseUtils;
import com.h6ah4i.android.media.openslmediaplayer.utils.ErrorListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.utils.PreparedListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;

public abstract class BasicMediaPlayerTestCase_SetDataSourceMethodBase
        extends BasicMediaPlayerStateTestCaseBase {

    protected static final String LOCAL_440HZ_STEREO_MP3_MULTI_BYTE_NAME =
            TESTSOUND_DIR + "/４４０ｈｚ＿ｓｔｅｒｅｏ.mp3";

    public BasicMediaPlayerTestCase_SetDataSourceMethodBase(ParameterizedTestArgs args) {
        super(args);
    }

    protected Uri getStrageFileUri(String name) {
        return Uri.fromFile(new File(getStorageFilePath(name)));
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        File srcFile = new File(getStorageFilePath(LOCAL_440HZ_STEREO_MP3));
        File destFile = new File(getStorageFilePath(LOCAL_440HZ_STEREO_MP3_MULTI_BYTE_NAME));

        if (!destFile.exists()) {
            CommonTestCaseUtils.copyFile(srcFile, destFile);
        }
    }

    protected void onSetDataSource(IBasicMediaPlayer player, Object args) throws IOException {
        failNotOverrided();
    }

    protected void expectsNoExceptions(IBasicMediaPlayer player, Object args) throws IOException {
        onSetDataSource(player, args);
    }

    protected void expectsIllegalStateException(IBasicMediaPlayer player, Object args)
            throws IOException {
        try {
            onSetDataSource(player, args);
        } catch (IllegalStateException e) {
            // expected
            return;
        }
        fail();
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoExceptions(player, args);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, args);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, args);
    }

    @Override
    protected boolean optCanTransitToPreparingStateAutomatically() {
        return false;
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        Object sharedSyncObj = new Object();
        PreparedListenerObject preparedObj = new PreparedListenerObject(sharedSyncObj);
        ErrorListenerObject errorObj = new ErrorListenerObject(sharedSyncObj, false);

        player.setOnPreparedListener(preparedObj);
        player.setOnErrorListener(errorObj);

        // NOTE: Need to call prepareAsync() explicitly because
        // optCanTransitToPreparingStateAutomatically() returns false
        player.prepareAsync();

        expectsIllegalStateException(player, args);

        BasicMediaPlayerEventListenerObject.awaitAny(
                DEFAULT_EVENT_WAIT_DURATION, preparedObj, errorObj);

        assertFalse(errorObj.occurred());
        // XXX This assertion fails on Galaxy S2 and SH-07E
        assertTrue(preparedObj.occurred());
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, args);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, args);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, args);

    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsIllegalStateException(player, args);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoExceptions(player, args);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoExceptions(player, args);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, args);
    }
}
