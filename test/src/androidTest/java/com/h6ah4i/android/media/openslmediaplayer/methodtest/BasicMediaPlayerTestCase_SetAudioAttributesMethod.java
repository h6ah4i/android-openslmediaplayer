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
import java.util.ArrayList;
import java.util.List;

import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.compat.AudioAttributes;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.utils.CompletionListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.utils.ErrorListenerObject;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetAudioAttributesMethod
        extends BasicMediaPlayerStateTestCaseBase {

    private static final int[] CONTENT_TYPES = {
            AudioAttributes.CONTENT_TYPE_UNKNOWN,
            AudioAttributes.CONTENT_TYPE_SPEECH,
            AudioAttributes.CONTENT_TYPE_MUSIC,
            AudioAttributes.CONTENT_TYPE_MOVIE,
            AudioAttributes.CONTENT_TYPE_SONIFICATION,
    };

    private static final int[] USAGES = {
            AudioAttributes.USAGE_UNKNOWN,
            AudioAttributes.USAGE_MEDIA,
            AudioAttributes.USAGE_VOICE_COMMUNICATION,
            AudioAttributes.USAGE_VOICE_COMMUNICATION_SIGNALLING,
            AudioAttributes.USAGE_ALARM,
            AudioAttributes.USAGE_NOTIFICATION,
            AudioAttributes.USAGE_NOTIFICATION_RINGTONE,
            AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_REQUEST,
            AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_INSTANT,
            AudioAttributes.USAGE_NOTIFICATION_COMMUNICATION_DELAYED,
            AudioAttributes.USAGE_NOTIFICATION_EVENT,
            AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY,
            AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE,
            AudioAttributes.USAGE_ASSISTANCE_SONIFICATION,
            AudioAttributes.USAGE_GAME,
    };

    private static final int[] FLAGS = {
            AudioAttributes.FLAG_AUDIBILITY_ENFORCED,
            AudioAttributes.FLAG_HW_AV_SYNC,
    };

    private static final int[] FLAG_PATTERNS;

    static {
        // calculate all combinations of flags
        FLAG_PATTERNS = new int[1 << FLAGS.length];
        for (int i = 0; i < FLAG_PATTERNS.length; i++) {
            FLAG_PATTERNS[i] = 0;
            for (int j = 0; j < FLAGS.length; j++) {
                int mask = (1 << j);
                if ((i & mask) != 0) {
                    FLAG_PATTERNS[i] |= FLAGS[j];
                }
            }
        }
    }

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();

        for (int contentType : CONTENT_TYPES) {
            params.add(new TestParams(factoryClazz, contentType,
                    AudioAttributes.CONTENT_TYPE_MUSIC, 0));
        }
        for (int usage : USAGES) {
            params.add(new TestParams(factoryClazz, AudioAttributes.CONTENT_TYPE_MUSIC, usage, 0));
        }
        for (int flags : FLAG_PATTERNS) {
            params.add(new TestParams(factoryClazz, AudioAttributes.CONTENT_TYPE_MUSIC,
                    AudioAttributes.USAGE_MEDIA, flags));
        }

        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SetAudioAttributesMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        public final int contentType;
        public final int usage;
        public final int flags;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                int contentType, int usage, int flags) {
            super(factoryClass);
            this.contentType = contentType;
            this.usage = usage;
            this.flags = flags;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + contentType + ", " + usage + ", " + flags;
        }
    }

    public BasicMediaPlayerTestCase_SetAudioAttributesMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void expectsNoErrors(IBasicMediaPlayer player, TestParams params) throws IOException {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        // check no errors
        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.setAudioAttributes(createAttributes(params));

        // XXX SH-02E (StandardMediaPlayer &
        // StateErrorBeforePreparedAndAfterReset)
        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail(comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
    }

    private AudioAttributes createAttributes(TestParams params) {
        return (new AudioAttributes.Builder())
                .setContentType(params.contentType)
                .setUsage(params.usage)
                .setFlags(params.flags)
                .build();
    }

    private void expectsIllegalStateException(IBasicMediaPlayer player, TestParams params) {
        try {
            player.setAudioAttributes(createAttributes(params));
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
