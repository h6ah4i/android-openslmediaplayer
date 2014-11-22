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


package com.h6ah4i.android.media.test.base;

import java.io.Closeable;
import java.io.EOFException;
import java.io.File;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.HttpURLConnection;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

import junit.framework.TestCase;
import junit.framework.TestSuite;
import android.content.Context;
import android.content.res.AssetManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Debug;
import android.util.Log;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.IReleasable;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerFactory;
import com.h6ah4i.android.media.test.utils.CommonTestCaseUtils;
import com.h6ah4i.android.media.test.utils.CompletionListenerObject;
import com.h6ah4i.android.media.test.utils.ErrorListenerObject;
import com.h6ah4i.android.media.test.utils.PreparedListenerObject;
import com.h6ah4i.android.media.test.utils.ThrowableRunnable;
import com.h6ah4i.android.testing.ParameterizedInstrumentationTestCase;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public abstract class BasicMediaPlayerTestCaseBase
        extends ParameterizedInstrumentationTestCase
        implements TestObjectBaseWrapper.Host {

    protected static final int SHORT_EVENT_WAIT_DURATION = 250;
    protected static final int DEFAULT_EVENT_WAIT_DURATION = 5000;
    protected static final int DURATION_LIMIT = 60000;

    protected static final String TESTSOUND_DIR = "testsounds";

    // Local asset files
    protected static final String LOCAL_SHORT_SILENT_STEREO_MP3 =
            TESTSOUND_DIR + "/short_silent.mp3";
    protected static final String LOCAL_440HZ_STEREO_MP3 = TESTSOUND_DIR + "/440hz_stereo.mp3";
    protected static final String LOCAL_880HZ_STEREO_MP3 = TESTSOUND_DIR + "/880hz_stereo.mp3";
    protected static final String LOCAL_440HZ_STEREO_OGG = TESTSOUND_DIR + "/440hz_stereo.ogg";
    protected static final String LOCAL_880HZ_STEREO_OGG = TESTSOUND_DIR + "/880hz_stereo.ogg";
    protected static final String LOCAL_440HZ_MONO_MP3 = TESTSOUND_DIR + "/440hz_monaural.mp3";
    protected static final String LOCAL_880HZ_MONO_MP3 = TESTSOUND_DIR + "/880hz_monaural.mp3";
    protected static final String LOCAL_440HZ_MONO_OGG = TESTSOUND_DIR + "/440hz_monaural.ogg";
    protected static final String LOCAL_880HZ_MONO_OGG = TESTSOUND_DIR + "/880hz_monaural.ogg";

    protected static final String LOCAL_440HZ_STEREO_48K_MP3 =
            TESTSOUND_DIR + "/440hz_stereo_48k.mp3";

    // Online files
    protected static final Uri ONLINE_URI_3MIN_WHITENOISE_WAV =
            Uri.parse("http://" + GlobalTestOptions.TEST_SERVER_ROOT
                    + "/media/whitenoise_stereo_3min.wav");
    protected static final Uri ONLINE_URI_440HZ_STEREO_MP3 =
            Uri.parse("http://" + GlobalTestOptions.TEST_SERVER_ROOT + "/media/440hz_stereo.mp3");

    // Player object states
    protected enum PlayerState {
        Idle,
        Initialized,
        Preparing,
        Prepared,
        Started,
        Paused,
        Stopped,
        PlaybackCompleted,
        ErrorBeforePrepared,
        ErrorAfterPrepared,
        End
    }

    // fields
    private final String mTestClassName;
    private IMediaPlayerFactory mFactory;

    protected static TestSuite buildBasicTestSuite(
            Class<? extends BasicMediaPlayerTestCaseBase> clazz,
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<BasicTestParams> params = new ArrayList<BasicTestParams>();

        params.add(new BasicTestParams(factoryClazz));

        return ParameterizedTestSuiteBuilder.build(clazz, params);
    }

    protected static class BasicTestParams {
        private final Class<? extends IMediaPlayerFactory> mFactoryClass;

        public BasicTestParams(Class<? extends IMediaPlayerFactory> factoryClass) {
            mFactoryClass = factoryClass;
        }

        public IMediaPlayerFactory createFactory(Context context) {
            try {
                return mFactoryClass.getConstructor(Context.class).newInstance(context);
            } catch (Exception e) {
                throw new IllegalStateException(e);
            }
        }

        @Override
        public String toString() {
            return mFactoryClass.getSimpleName();
        }
    }

    public BasicMediaPlayerTestCaseBase() {
        super();
        mTestClassName = ((Object) this).getClass().getSimpleName();
    }

    public BasicMediaPlayerTestCaseBase(ParameterizedTestArgs args) {
        super(args);
        mTestClassName = ((Object) this).getClass().getSimpleName();
    }

    protected IMediaPlayerFactory getFactory() {
        return mFactory;
    }

    //
    // setUp and tearDown
    //
    @Override
    protected void setUp() throws Exception {
        memoryCheckStart();

        super.setUp();

        copyAllTestSoundsToStorage();

        mFactory = onCreateFactory();
    }

    @Override
    protected void tearDown() throws Exception {
        releaseQuietly(mFactory);
        mFactory = null;

        super.tearDown();

        memoryCheckEnd();
    }

    //
    // Memory leak checker
    //
    long mUsedMemorySize1;

    private void memoryCheckStart() {
        System.gc();
        System.runFinalization();
        System.gc();

        mUsedMemorySize1 = Debug.getPss();
    }

    private void memoryCheckEnd() {
        System.gc();
        System.runFinalization();
        System.gc();

        final long used1 = mUsedMemorySize1;
        final long used2 = Debug.getPss();
        final long diff = used2 - used1;

        final String infoText = " (increased: " + diff + " KiB, current: " + used2 + " KiB, @"
                + mTestClassName + "." + getName() + ")";

        if (diff >= 1000) {
            Log.e("RAM Info", "Huge memory leak detected" + infoText);
        } else if (diff >= 500) {
            Log.e("RAM Info", "Large memory leak detected" + infoText);
        } else if (diff >= 50) {
            Log.w("RAM Info", "Memory leak detected" + infoText);
        } else if (diff >= 5) {
            Log.w("RAM Info", "Small memory leak detected" + infoText);
        } else {
            Log.v("RAM Info", "No memory leak detected" + infoText);
        }
    }

    protected Context getContext() {
        return getInstrumentation().getContext();
    }

    protected File getTempDir() {
        File dir = getContext().getCacheDir();
//        if (dir != null) {
//            return dir;
//        }

        dir = getContext().getExternalCacheDir();
        if (dir != null) {
            return dir;
        }

        dir = getContext().getFilesDir();

        return dir;
    }

    protected static void closeQuietly(Closeable c) {
        CommonTestCaseUtils.closeQuietly(c);
    }

    protected static void releaseQuietly(IReleasable releasable) {
        CommonTestCaseUtils.releaseQuietly(releasable);
    }

    protected static int safeGetDuration(IBasicMediaPlayer player, int limit) {
        limit = Math.max(0, limit);

        if (player == null)
            return 0;

        // NOTE: The getDuration() function may returns
        // unpredictable value when the player object is in illegal state
        int value = player.getDuration();

        if (value < 0 || value > limit) {
            value = 0;
        }

        return value;
    }

    protected IMediaPlayerFactory onCreateFactory() {
        if (getTestParams() instanceof BasicTestParams) {
            return ((BasicTestParams) getTestParams()).createFactory(getContext());
        }

        throw new IllegalStateException("Must override this method");
    }

    protected void copyAllTestSoundsToStorage() throws IOException {
        String[] assets = null;
        AssetManager am = getContext().getAssets();
        File destBaseDir = getTempDir();

        assets = am.list(TESTSOUND_DIR);

        for (String asset : assets) {
            CommonTestCaseUtils.copyAssetFile(
                    am, TESTSOUND_DIR + File.separator + asset,
                    destBaseDir, false);
        }
    }

    protected String getStorageFilePath(String name) {
        return new File(getTempDir().getAbsolutePath(), name).getAbsolutePath();
    }

    protected void failNotOverrided() {
        fail("Subclass must override this method");
    }

    protected static void assertRange(int expectedMin, int expectedMax, int actual) {
        if (actual < expectedMin) {
            fail("expectedMin: <" + expectedMin + ">, actual: <" + actual + ">");
        }
        if (actual > expectedMax) {
            fail("expectedMax: <" + expectedMax + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertLargerThanOrEqual(int expectedMin, int actual) {
        if (actual < expectedMin) {
            fail("expectedMin: <" + expectedMin + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertLessThanOrEqual(int expectedMax, int actual) {
        if (actual > expectedMax) {
            fail("expectedMax: <" + expectedMax + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertRange(short expectedMin, short expectedMax, short actual) {
        if (actual < expectedMin) {
            fail("expectedMin: <" + expectedMin + ">, actual: <" + actual + ">");
        }
        if (actual > expectedMax) {
            fail("expectedMax: <" + expectedMax + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertLargerThanOrEqual(short expectedMin, short actual) {
        if (actual < expectedMin) {
            fail("expectedMin: <" + expectedMin + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertLessThanOrEqual(short expectedMax, short actual) {
        if (actual > expectedMax) {
            fail("expectedMax: <" + expectedMax + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertNotEquals(byte notExpected, byte actual) {
        if (actual == notExpected) {
            fail("notExpected: <" + notExpected + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertNotEquals(short notExpected, short actual) {
        if (actual == notExpected) {
            fail("notExpected: <" + notExpected + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertNotEquals(int notExpected, int actual) {
        if (actual == notExpected) {
            fail("notExpected: <" + notExpected + ">, actual: <" + actual + ">");
        }
    }

    protected static void assertNotEquals(float notExpected, float actual) {
        if (actual == notExpected) {
            fail("notExpected: <" + notExpected + ">, actual: <" + actual + ">");
        }
    }

    //
    // Utils
    //
    protected void setDataSourceForCommonTests(IBasicMediaPlayer player, Object args)
            throws IOException {
        player.setDataSource(getStorageFilePath(LOCAL_440HZ_STEREO_MP3));
    }

    protected void setDataSourceForPlaybackCompletedTest(IBasicMediaPlayer player, Object args)
            throws IOException {
        player.setDataSource(getStorageFilePath(LOCAL_SHORT_SILENT_STEREO_MP3));
    }

    protected void setDataSourceForPreparingTest(IBasicMediaPlayer player, Object args)
            throws IOException {
        final Uri uri = ONLINE_URI_3MIN_WHITENOISE_WAV;

        checkOnlineConnectionStates();
        checkFileExistsOnHTTP(uri);

        player.setDataSource(getContext(), uri);
    }

    protected void checkFileExistsOnHTTP(Uri uri) {
        final String failmsg = "Specified file is not accessible; uri = " + uri;
        final int MAX_RETRIES = 5;

        HttpURLConnection.setFollowRedirects(false);
        Exception exception = null;

        for (int retry = 1; retry <= MAX_RETRIES; retry++) {
            HttpURLConnection con = null;

            try {
                con = (HttpURLConnection) new URL(uri.toString()).openConnection();
                con.setConnectTimeout(1000);
                con.setReadTimeout(1000);
                con.setRequestMethod("HEAD");
                con.disconnect();

                assertEquals(failmsg, HttpURLConnection.HTTP_OK, con.getResponseCode());
                return;
            } catch (SocketTimeoutException e) {
                Log.i(getName(), "checkFileExistsOnHTTP() - SocketTimeoutException - retry "
                        + retry);
                exception = e;
            } catch (EOFException e) {
                Log.i(getName(), "checkFileExistsOnHTTP() - EOFException - retry " + retry);
                exception = e;
            } catch (Exception e) {
                fail(failmsg + ", exception = " + e);
            } finally {
                if (con != null) {
                    con.disconnect();
                    con = null;
                }
            }
        }

        if (exception != null) {
            fail(failmsg + ", exception = " + exception);
        }
    }

    protected void checkOnlineConnectionStates() {
        // Check WiFi connection is enabled

        ConnectivityManager connManager =
                (ConnectivityManager) getContext().
                        getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo wifi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        // NetworkInfo mobile =
        // connManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);

        assertTrue("WiFi is not enabled", wifi.isConnected());
        // assertFalse("Mobile communication is enabled", mobile.isConnected());
    }

    protected void resetPlayerStates(final IBasicMediaPlayer player) {
        player.reset();

        player.setOnPreparedListener(null);
        player.setOnCompletionListener(null);
        player.setOnBufferingUpdateListener(null);
        player.setOnSeekCompleteListener(null);
        player.setOnErrorListener(null);
        player.setOnInfoListener(null);
    }

    protected TestBasicMediaPlayerWrapper createWrappedPlayerInstance() {
        return TestBasicMediaPlayerWrapper.create(this, mFactory);
    }

    protected static IBasicMediaPlayer unwrap(IBasicMediaPlayer player) {
        if (player instanceof TestBasicMediaPlayerWrapper) {
            return ((TestBasicMediaPlayerWrapper) player).getWrappedInstance();
        }
        return player;
    }

    protected static boolean isOpenSL(IBasicMediaPlayer player) {
        return (unwrap(player) instanceof OpenSLMediaPlayer);
    }

    protected static boolean isOpenSL(IMediaPlayerFactory factory) {
        return (factory instanceof OpenSLMediaPlayerFactory);
    }

    protected void playLocalFileAndReset(IBasicMediaPlayer player, Object args) throws IOException {
        setDataSourceForCommonTests(player, args);
        player.prepare();
        player.start();
        player.stop();
        resetPlayerStates(player);
    }

    protected static int determineWaitCompletionTime(IBasicMediaPlayer player) {
        return safeGetDuration(player, DURATION_LIMIT) + DEFAULT_EVENT_WAIT_DURATION;
    }

    // TestObjectBaseWrapper.Host
    @Override
    public void hostRunTestOnUiThread(Runnable r) {
        try {
            runTestOnUiThread(r);
        } catch (RuntimeException e) {
            throw e;
        } catch (Throwable th) {
            fail(th.toString());
        }
    }

    @Override
    public void hostRunTestOnUiThread(final ThrowableRunnable thr) throws Throwable {
        final Throwable[] throwable = new Throwable[1];

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    thr.run();
                } catch (Throwable th) {
                    throwable[0] = th;
                }
            }
        });
        if (throwable[0] != null) {
            throw throwable[0];
        }
    }

    @Override
    public void hostTestFail() {
        fail();
    }

    @Override
    public void hostTestFail(String message) {
        fail(message);
    }

    //
    // State transition
    //
    protected void transitStateToIdle(IBasicMediaPlayer player, Object args) throws IOException {
        player.reset();
    }

    protected void transitStateToInitialized(IBasicMediaPlayer player, Object args)
            throws IOException {
        setDataSourceForCommonTests(player, args);
    }

    protected void transitStateToPreparing(IBasicMediaPlayer player, Object args)
            throws IOException {
        setDataSourceForPreparingTest(player, args);

        if (optCanTransitToPreparingStateAutomatically()) {
            player.prepareAsync();
        }
    }

    protected void transitStateToPrepared(IBasicMediaPlayer player, Object args) throws IOException {
        PreparedListenerObject prepared = new PreparedListenerObject();

        setDataSourceForCommonTests(player, args);

        player.setOnPreparedListener(prepared);
        player.prepare();

        if (!prepared.await(SHORT_EVENT_WAIT_DURATION)) {
            fail("onPrepared() wasn't called properly");
        }
        player.setOnPreparedListener(null);
    }

    protected void transitStateToStarted(IBasicMediaPlayer player, Object args) throws IOException {
        setDataSourceForCommonTests(player, args);
        player.prepare();

        if (optCanTransitToStartedStateAutomatically()) {
            player.start();
        }
    }

    protected void transitStateToPaused(IBasicMediaPlayer player, Object args) throws IOException {
        setDataSourceForCommonTests(player, args);
        player.prepare();
        player.start();
        player.pause();
    }

    protected void transitStateToStopped(IBasicMediaPlayer player, Object args) throws IOException {
        setDataSourceForCommonTests(player, args);
        player.prepare();
        player.start();
        player.stop();
    }

    protected void transitStateToPlaybackCompleted(IBasicMediaPlayer player, Object args)
            throws IOException {
        CompletionListenerObject comp = new CompletionListenerObject();

        setDataSourceForPlaybackCompletedTest(player, args);
        player.prepare();
        player.setOnCompletionListener(comp);
        player.start();

        if (!comp.await(determineWaitCompletionTime(player))) {
            fail("transitStateToPlaybackCompleted() failed");
        }

        player.setOnCompletionListener(null);
    }

    protected void transitStateToErrorBeforePrepared(IBasicMediaPlayer player, Object args)
            throws IOException {
        final Object sharedSyncObj = new Object();
        final ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        final CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        player.getDuration();

        if (!comp.await(DEFAULT_EVENT_WAIT_DURATION)) {
            fail("transitStateToErrorBeforePrepared() failed, " + comp + ", " + err);
        }

        player.setOnErrorListener(null);
        player.setOnCompletionListener(null);

        assertTrue(err.occurred());
        assertTrue(comp.occurred());
    }

    protected void transitStateToErrorAfterPrepared(IBasicMediaPlayer player, Object args)
            throws IOException {
        final Object sharedSyncObj = new Object();
        final ErrorListenerObject err = new ErrorListenerObject(sharedSyncObj, false);
        final CompletionListenerObject comp = new CompletionListenerObject(sharedSyncObj);

        player.setOnErrorListener(err);
        player.setOnCompletionListener(comp);

        setDataSourceForCommonTests(player, args);
        player.prepare();
        player.pause();

        if (!comp.await(DEFAULT_EVENT_WAIT_DURATION)) {
            fail("transitStateToErrorAfterPrepared() failed, " + comp + ", " + err);
        }

        player.setOnErrorListener(null);
        player.setOnCompletionListener(null);

        assertTrue(err.occurred());
        assertTrue(comp.occurred());
    }

    protected void transitStateToEnd(IBasicMediaPlayer player, Object args) throws IOException {
        player.release();
    }

    protected void transitState(
            PlayerState playerState,
            IBasicMediaPlayer player, Object args)
            throws IOException {

        switch (playerState) {
            case Idle:
                transitStateToIdle(player, args);
                break;
            case Initialized:
                transitStateToInitialized(player, args);
                break;
            case Preparing:
                transitStateToPreparing(player, args);
                break;
            case Prepared:
                transitStateToPrepared(player, args);
                break;
            case Started:
                transitStateToStarted(player, args);
                break;
            case Paused:
                transitStateToPaused(player, args);
                break;
            case PlaybackCompleted:
                transitStateToPlaybackCompleted(player, args);
                break;
            case Stopped:
                transitStateToStopped(player, args);
                break;
            case ErrorBeforePrepared:
                transitStateToErrorBeforePrepared(player, args);
                break;
            case ErrorAfterPrepared:
                transitStateToPrepared(player, args);
                break;
            case End:
                transitStateToEnd(player, args);
                break;
        }
    }

    //
    // Utility
    //
    protected static TestCase makeSingleBasicTest(
            Class<? extends TestCase> clazz,
            String testName,
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        try {
            Method method = clazz.getMethod(testName);
            int modifiers = method.getModifiers();

            if (Modifier.isPublic(modifiers) &&
                    !Modifier.isStatic(modifiers) &&
                    testName.startsWith("test") &&
                    method.getReturnType().equals(void.class)) {
                return clazz
                        .getConstructor(ParameterizedTestArgs.class)
                        .newInstance(new ParameterizedTestArgs(
                                testName, new BasicTestParams(factoryClazz)));
            } else {
                throw new IllegalArgumentException("Specified test method is not available "
                        + clazz
                        + ", " + testName);
            }
        } catch (NoSuchMethodException e) {
            throw new IllegalStateException(e);
        } catch (InstantiationException e) {
            throw new IllegalStateException(e);
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        } catch (InvocationTargetException e) {
            throw new IllegalStateException(e);
        }
    }

    //
    // Options
    //
    protected boolean optCanTransitToPreparingStateAutomatically() {
        // Override and return false if prepareAsync() should not be called
        // in transitStateToPreparing()
        return true;
    }

    protected boolean optCanTransitToStartedStateAutomatically() {
        // Override and return false if start() should not be called
        // in transitStateToStarted()
        return true;
    }
}
