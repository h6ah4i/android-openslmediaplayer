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
import android.net.Uri;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.utils.BasicMediaPlayerEventListenerObject;
import com.h6ah4i.android.media.test.utils.BufferingUpdateListenerObject;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetOnBufferingUpdateListenerMethod
        extends BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();

        params.add(new TestParams(factoryClazz, LOCAL_440HZ_STEREO_MP3));
        params.add(new TestParams(factoryClazz, ONLINE_URI_440HZ_STEREO_MP3));

        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SetOnBufferingUpdateListenerMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        private final Object mDataFile;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                Object dataFile) {
            super(factoryClass);
            mDataFile = dataFile;
        }

        public Object getDataFile() {
            return mDataFile;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mDataFile;
        }
    }

    public BasicMediaPlayerTestCase_SetOnBufferingUpdateListenerMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsBufferingUpdateCallback(IBasicMediaPlayer player) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);
        BufferingUpdateListenerObject buffupdate = new BufferingUpdateListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);
        player.setOnBufferingUpdateListener(buffupdate);

        // NOTE: Need to call start() explicitly because
        // optCanTransitToStartedStateAutomatically() returns false
        player.start();

        if (!BasicMediaPlayerEventListenerObject.awaitAny(
                DEFAULT_EVENT_WAIT_DURATION, err, comp, buffupdate)) {
            fail();
        }

        player.setOnErrorListener(null);
        player.setOnCompletionListener(null);
        player.setOnPreparedListener(null);

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
        assertTrue(buffupdate.occurred());
    }

    private void expectsNoErrors(IBasicMediaPlayer player) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);
        BufferingUpdateListenerObject buffupdate = new BufferingUpdateListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);
        player.setOnBufferingUpdateListener(buffupdate);

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(comp + ", " + err);
        }

        player.setOnErrorListener(null);
        player.setOnCompletionListener(null);
        player.setOnBufferingUpdateListener(null);

        assertFalse(comp.occurred());
        assertFalse(err.occurred());

        // XXX Galaxy S4  (SC-04E, Android 4.4.2) fails on this assertion
        // (StandardMediaPlayer & testStatePreparing, testStatePreparingAfterReset)
        assertFalse(buffupdate.occurred());
    }

    @Override
    protected void setDataSourceForCommonTests(IBasicMediaPlayer player, Object args)
            throws IOException {
        TestParams params = (TestParams) args;
        Object dataFile = params.getDataFile();

        if (dataFile instanceof String) {
            player.setDataSource(getStorageFilePath((String) dataFile));
        } else if (dataFile instanceof Uri) {
            player.setDataSource(getContext(), (Uri) dataFile);
        } else {
            fail();
        }
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
        expectsNoErrors(player);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsNoErrors(player);
    }

    @Override
    public void testStateStoppedAfterReset() throws Throwable {
        super.testStateStoppedAfterReset();
    }

    @Override
    protected boolean optCanTransitToStartedStateAutomatically() {
        return false;
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        TestParams params = (TestParams) args;
        String source = null;

        if (params.getDataFile() instanceof Uri) {
            source = ((Uri) params.getDataFile()).toString();
        } else if (params.getDataFile() instanceof String) {
            source = (String) params.getDataFile();
        }

        if (source.startsWith("http://")) {
            expectsBufferingUpdateCallback(player);
        } else {
            expectsNoErrors(player);
        }
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
