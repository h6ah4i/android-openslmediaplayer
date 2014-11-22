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
import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.audiofx.IPresetReverb;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_AttachAuxEffectMethod extends
        BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();
        for (ArgType argType : ArgType.values()) {
            params.add(new TestParams(factoryClazz, argType));
        }
        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_AttachAuxEffectMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        private final ArgType mArgType;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                ArgType argType) {
            super(factoryClass);
            mArgType = argType;
        }

        public ArgType getArgType() {
            return mArgType;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mArgType.name();
        }
    }

    enum ArgType {
        VALID_ENVIRONMENTAL_REVERB,
        VALID_PRESET_REVERB,
        NULL,
        INVALID_MIN_VALUE,
        INVALID_MAX_VALUE,
    }

    public BasicMediaPlayerTestCase_AttachAuxEffectMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private void attachAuxEffect(IBasicMediaPlayer player, ArgType type) {
        switch (type) {
            case VALID_ENVIRONMENTAL_REVERB: {
                IEnvironmentalReverb reverb = null;
                try {
                    reverb = getFactory().createEnvironmentalReverb();
                    player.attachAuxEffect(reverb.getId());
                } finally {
                    if (reverb != null) {
                        reverb.release();
                    }
                }
            }
                break;
            case VALID_PRESET_REVERB: {
                IPresetReverb reverb = null;
                try {
                    reverb = getFactory().createPresetReverb();
                    player.attachAuxEffect(reverb.getId());
                } finally {
                    if (reverb != null) {
                        reverb.release();
                    }
                }
            }
                break;
            case NULL:
                player.attachAuxEffect(0);
                break;
            case INVALID_MIN_VALUE:
                player.attachAuxEffect(Integer.MIN_VALUE);
                break;
            case INVALID_MAX_VALUE:
                player.attachAuxEffect(Integer.MAX_VALUE);
                break;
            default:
                fail();
        }
    }

    private void expectsNoErrors(IBasicMediaPlayer player, TestParams params) {
        ArgType type = params.getArgType();
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        attachAuxEffect(player, type);

        // XXX SH-02E (StandardMediaPlayer &
        // StateErrorBeforePreparedAndAfterReset)
        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail("Type = " + type + ", " + comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
    }

    private void expectsErrorCallback(IBasicMediaPlayer player, TestParams params) {
        ArgType type = params.getArgType();
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        attachAuxEffect(player, type);

        if (!comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail("Type = " + type + ", " + comp + ", " + err);
        }

        assertTrue(comp.occurred());
        assertTrue(err.occurred());
    }

    protected void expectsIllegalStateException(
            IBasicMediaPlayer player, TestParams params)
            throws IOException {
        ArgType type = params.getArgType();
        try {
            attachAuxEffect(player, type);
        } catch (IllegalStateException e) {
            // expected
            return;
        }
        fail();
    }

    private void check(IBasicMediaPlayer player, TestParams params) {
        switch (params.getArgType()) {
            case VALID_ENVIRONMENTAL_REVERB:
            case VALID_PRESET_REVERB:
            case NULL:
                expectsNoErrors(player, params);
                break;
            // case INVALID_MINUS_ONE:
            case INVALID_MAX_VALUE:
            case INVALID_MIN_VALUE:
                // XXX: On Android 5.0 with StandardMediaPlayer, error callback
                // won't be called
                expectsErrorCallback(player, params);
                break;
            default:
                throw new IllegalArgumentException();
        }
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        // check(player, (TestParams) args);
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        // check(player, (TestParams) args);
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        // check(player, (TestParams) args);
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        check(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        check(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        check(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        check(player, (TestParams) args);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsNoErrors(player, (TestParams) args);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        expectsErrorCallback(player, (TestParams) args);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, (TestParams) args);
    }
}
