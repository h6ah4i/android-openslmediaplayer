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
import android.content.Context;
import android.os.PowerManager;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerStateTestCaseBase;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetWakeModeMethod
        extends BasicMediaPlayerStateTestCaseBase {

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();

        params.add(new TestParams(factoryClazz, true, PARTIAL_WAKE_LOCK, true));
        params.add(new TestParams(factoryClazz, true, PARTIAL_WAKE_LOCK | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz,
                true, PARTIAL_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP, true));
        params.add(new TestParams(factoryClazz,
                true, PARTIAL_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz, true, FULL_WAKE_LOCK, true));
        params.add(new TestParams(factoryClazz, true, FULL_WAKE_LOCK | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz, true, FULL_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP, true));
        params.add(new TestParams(factoryClazz,
                true, FULL_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz, true, SCREEN_BRIGHT_WAKE_LOCK, true));
        params.add(new TestParams(factoryClazz, true, SCREEN_BRIGHT_WAKE_LOCK | ON_AFTER_RELEASE,
                true));
        params.add(new TestParams(factoryClazz,
                true, SCREEN_BRIGHT_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP, true));
        params.add(new TestParams(factoryClazz,
                true, SCREEN_BRIGHT_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz, true, SCREEN_DIM_WAKE_LOCK, true));
        params.add(new TestParams(factoryClazz, true, SCREEN_DIM_WAKE_LOCK | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz,
                true, SCREEN_DIM_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP, true));
        params.add(new TestParams(factoryClazz,
                true, SCREEN_DIM_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz, true, 0, false));
        params.add(new TestParams(factoryClazz, true, ON_AFTER_RELEASE, false));
        params.add(new TestParams(factoryClazz, true, ACQUIRE_CAUSES_WAKEUP, false));
        params.add(new TestParams(factoryClazz,
                true, ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, false));

        // invalid context
        params.add(new TestParams(factoryClazz, false, PARTIAL_WAKE_LOCK, true));
        params.add(new TestParams(factoryClazz, false, PARTIAL_WAKE_LOCK | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz,
                false, PARTIAL_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP, true));
        params.add(new TestParams(factoryClazz,
                false, PARTIAL_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz, false, FULL_WAKE_LOCK, true));
        params.add(new TestParams(factoryClazz, false, FULL_WAKE_LOCK | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz,
                false, FULL_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP, true));
        params.add(new TestParams(factoryClazz,
                false, FULL_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz,
                false, SCREEN_BRIGHT_WAKE_LOCK, true));
        params.add(new TestParams(factoryClazz,
                false, SCREEN_BRIGHT_WAKE_LOCK | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz,
                false, SCREEN_BRIGHT_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP, true));
        params.add(new TestParams(factoryClazz,
                false, SCREEN_BRIGHT_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz,
                false, SCREEN_DIM_WAKE_LOCK, true));
        params.add(new TestParams(factoryClazz,
                false, SCREEN_DIM_WAKE_LOCK | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz,
                false, SCREEN_DIM_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP, true));
        params.add(new TestParams(factoryClazz,
                false, SCREEN_DIM_WAKE_LOCK | ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, true));
        params.add(new TestParams(factoryClazz, false, 0, false));
        params.add(new TestParams(factoryClazz, false, ON_AFTER_RELEASE, false));
        params.add(new TestParams(factoryClazz,
                false, ACQUIRE_CAUSES_WAKEUP, false));
        params.add(new TestParams(factoryClazz,
                false, ACQUIRE_CAUSES_WAKEUP | ON_AFTER_RELEASE, false));

        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SetWakeModeMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        public final boolean passValidContext;
        public final int mode;
        public final boolean isValidMode;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                boolean passVaildContext, int mode, boolean isValidMode) {
            super(factoryClass);
            this.passValidContext = passVaildContext;
            this.mode = mode;
            this.isValidMode = isValidMode;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + passValidContext + ", " + mode + ", " + isValidMode;
        }
    }

    public BasicMediaPlayerTestCase_SetWakeModeMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private static final int PARTIAL_WAKE_LOCK = PowerManager.PARTIAL_WAKE_LOCK;
    private static final int ON_AFTER_RELEASE = PowerManager.ON_AFTER_RELEASE;
    private static final int ACQUIRE_CAUSES_WAKEUP = PowerManager.ACQUIRE_CAUSES_WAKEUP;

    @SuppressWarnings("deprecation")
    private static final int FULL_WAKE_LOCK = PowerManager.FULL_WAKE_LOCK;
    @SuppressWarnings("deprecation")
    private static final int SCREEN_BRIGHT_WAKE_LOCK = PowerManager.SCREEN_BRIGHT_WAKE_LOCK;
    @SuppressWarnings("deprecation")
    private static final int SCREEN_DIM_WAKE_LOCK = PowerManager.SCREEN_DIM_WAKE_LOCK;

    private static final StringBuilder appendWithOR(StringBuilder sb, String str) {
        if (sb.length() > 0) {
            sb.append(" | ");
        }
        return sb.append(str);
    }

    @SuppressWarnings("deprecation")
    private static final String wakeModeToString(int mode) {
        StringBuilder sb = new StringBuilder();

        if ((mode & PowerManager.PARTIAL_WAKE_LOCK) != 0) {
            appendWithOR(sb, "PARTIAL_WAKE_LOCK");
        }

        if ((mode & PowerManager.ACQUIRE_CAUSES_WAKEUP) != 0) {
            appendWithOR(sb, "ACQUIRE_CAUSES_WAKEUP");
        }

        if ((mode & PowerManager.ON_AFTER_RELEASE) != 0) {
            appendWithOR(sb, "ON_AFTER_RELEASE");
        }

        // deprecated values
        if ((mode & PowerManager.FULL_WAKE_LOCK) != 0) {
            appendWithOR(sb, "FULL_WAKE_LOCK");
        }

        if ((mode & PowerManager.SCREEN_DIM_WAKE_LOCK) != 0) {
            appendWithOR(sb, "SCREEN_DIM_WAKE_LOCK");
        }

        if ((mode & PowerManager.SCREEN_BRIGHT_WAKE_LOCK) != 0) {
            appendWithOR(sb, "SCREEN_BRIGHT_WAKE_LOCK");
        }

        return sb.toString();
    }

    private void expectsNoErrors(IBasicMediaPlayer player, TestParams params) {
        Object sharedSyncObj = new Object();
        ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        setWakeMode(player, params);

        if (comp.await(SHORT_EVENT_WAIT_DURATION)) {
            fail("Context = " + params.passValidContext + ", Mode = "
                    + wakeModeToString(params.mode) + ", " + comp + ", " + err);
        }

        assertFalse(comp.occurred());
        assertFalse(err.occurred());
    }

    // private void expectsErrorCallback(IBasicMediaPlayer player, TestParams
    // params) {
    // Object sharedSyncObj = new Object();
    // ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
    // CompletionListenerObject comp = new
    // CompletionListenerObject(sharedSyncObj);
    //
    // player.setOnErrorListener(err);
    // player.setOnCompletionListener(comp);
    //
    // setWakeMode(player, params);
    //
    // if (!comp.await(determineWaitCompletionTime(player))) {
    // fail("Context = " + params.passValidContext + ", Mode = "
    // + wakeModeToString(params.mode));
    // }
    //
    // assertTrue(comp.occurred());
    // assertTrue(err.occurred());
    // }

    private void expectsIllegalArgumentException(IBasicMediaPlayer player, TestParams params) {
        try {
            setWakeMode(player, params);
        } catch (IllegalArgumentException e) {
            return;
        }
        fail("Context = " + params.passValidContext + ", Mode = " + wakeModeToString(params.mode));
    }

    private void expectsIllegalStateException(IBasicMediaPlayer player, TestParams params) {
        try {
            setWakeMode(player, params);
        } catch (IllegalStateException e) {
            return;
        }
        fail("Context = " + params.passValidContext + ", Mode = " +
                wakeModeToString(params.mode));
    }

    private void setWakeMode(IBasicMediaPlayer player, TestParams params) {
        final Context context = (params.passValidContext) ? getContext() : null;
        player.setWakeMode(context, params.mode);
    }

    private void swithByExpections(IBasicMediaPlayer player, TestParams params) {
        if (params.passValidContext && params.isValidMode) {
            expectsNoErrors(player, params);
        } else {
            expectsIllegalArgumentException(player, params);
        }
    }

    @Override
    protected void onTestStateIdle(IBasicMediaPlayer player, Object args) throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStateInitialized(IBasicMediaPlayer player, Object args) throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePreparing(IBasicMediaPlayer player, Object args) throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePrepared(IBasicMediaPlayer player, Object args) throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStarted(IBasicMediaPlayer player, Object args) throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePaused(IBasicMediaPlayer player, Object args) throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStateStopped(IBasicMediaPlayer player, Object args) throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStatePlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStateErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStateErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws Throwable {
        swithByExpections(player, (TestParams) args);
    }

    @Override
    protected void onTestStateEnd(IBasicMediaPlayer player, Object args) throws Throwable {
        expectsIllegalStateException(player, (TestParams) args);
    }
}
