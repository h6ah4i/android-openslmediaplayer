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
import android.media.MediaMetadataRetriever;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.utils.CompletionListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.utils.ErrorListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;

public class BasicMediaPlayerTestCase_GetCurrentPositionMethod
        extends BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        return BasicMediaPlayerTestCaseBase.buildBasicTestSuite(
                BasicMediaPlayerTestCase_GetCurrentPositionMethod.class, factoryClazz);
    }

    public BasicMediaPlayerTestCase_GetCurrentPositionMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsNoErrorsWithValidPosition(IBasicMediaPlayer player) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        final int value = player.getCurrentPosition();
        final int duration = getActualDuration(getStorageFilePath(LOCAL_440HZ_STEREO_OGG));

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
        assertRange(0, duration, value);
    }

    private void expectsNoErrorsWithIncrementalPosition(IBasicMediaPlayer player) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        final long deadline = System.currentTimeMillis() + DEFAULT_EVENT_WAIT_DURATION;
        int prevPos = 0;

        final int duration = getActualDuration(getStorageFilePath(LOCAL_440HZ_STEREO_OGG));

        while (!comp.occurred()) {
            final long curTime = System.currentTimeMillis();

            if (curTime >= deadline) {
                fail();
            }

            final int curPos = player.getCurrentPosition();

            assertLargerThanOrEqual(prevPos, curPos);
            // XXX StandardMediaPlayer (Started, StartedAfterReset) fails on
            // this assertion
            assertRange(0, duration, curPos);

            prevPos = curPos;

            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
            }
        }

        assertTrue(comp.occurred());
        assertFalse(err.occurred());
    }

    private static int getActualDuration(String path) {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();

        retriever.setDataSource(path);

        String durationStr = retriever
                .extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);

        return Integer.parseInt(durationStr);
    }

    @Override
    protected void setDataSourceForCommonTests(IBasicMediaPlayer player, Object args)
            throws IOException {
        // NOTE:
        // MP3 format should not be used for this test
        // because MP3 file cannot handle duration info correctly.
        player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_OGG));
    }

    @Override
    protected void setDataSourceForPlaybackCompletedTest(IBasicMediaPlayer player, Object args)
            throws IOException {
        // NOTE:
        // MP3 format should not be used for this test
        // because MP3 file cannot handle duration info correctly.
        player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_OGG));
    }

    private void expectsErrorCallback(IBasicMediaPlayer player) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.getCurrentPosition();

        if (!comp.await(determineWaitCompletionTime(player))) {
            fail();
        }

        assertTrue(comp.occurred());
        assertTrue(err.occurred());
    }

    private void expectsIllegalStateException(IBasicMediaPlayer player) {
        try {
            player.getCurrentPosition();
        } catch (IllegalStateException e) {
            return;
        }
        fail();
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithValidPosition(player);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        // XXX StandardMediaPlayer fails on this test case
        expectsNoErrorsWithIncrementalPosition(player);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithValidPosition(player);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrorsWithValidPosition(player);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrorsWithValidPosition(player);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        // XXX StandardMediaPlayer fails on this test case
        expectsNoErrorsWithValidPosition(player);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrorsWithValidPosition(player);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player);
    }
}
