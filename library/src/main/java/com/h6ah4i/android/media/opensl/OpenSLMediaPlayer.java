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


package com.h6ah4i.android.media.opensl;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.nio.channels.FileChannel;

import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.provider.MediaStore.MediaColumns;
import android.util.Log;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.compat.AudioAttributes;

public class OpenSLMediaPlayer implements IBasicMediaPlayer {
    private static final String TAG = "OpenSLMediaPlayer";
    private static final String WAKELOCK_TAG = "OpenSLMediaPlayerWakeLock";

    private static final boolean LOCAL_LOGV = false;
    private static final boolean LOCAL_LOGD = false;

    // options
    public static final int OPTION_USE_FADE = (1 << 0);

    // fields
    private static final String[] PROJECTION_MEDIACOLUMNS_DATA = new String[] {
            MediaColumns.DATA
    };

    private static final Field mField_FileDescriptor_descriptor;

    private long mNativeHandle;
    private static final boolean HAS_NATIVE;
    private int[] mParamIntBuff = new int[1];
    private boolean[] mParamBoolBuff = new boolean[1];
    private InternalHandler mHandler;
    private AssetFileDescriptor mContentAssetFd;

    private WakeLock mWakeLock;

    private OnCompletionListener mOnCompletionListener;
    private OnPreparedListener mOnPreparedListener;
    private OnSeekCompleteListener mOnSeekCompleteListener;
    private OnBufferingUpdateListener mOnBufferingUpdateListener;
    private OnInfoListener mOnInfoListener;
    private OnErrorListener mOnErrorListener;

    static {
        // load native library
        HAS_NATIVE = OpenSLMediaPlayerNativeLibraryLoader.loadLibraries();

        // obtain FileDescriptor.descriptor field
        Field field_descriptor = null;
        try {
            Field f = FileDescriptor.class.getDeclaredField("descriptor");
            if (f != null) {
                f.setAccessible(true);

                field_descriptor = f;
            }
        } catch (NoSuchFieldException e) {
        } catch (Exception e) {
        }

        mField_FileDescriptor_descriptor = field_descriptor;
    }

    public OpenSLMediaPlayer(OpenSLMediaPlayerContext context, int options) {
        if (context == null)
            throw new IllegalArgumentException("The argument 'context' cannot be null");

        final long contextHandle = Internal.getNativeHandle(context);

        if (contextHandle == 0)
            throw new IllegalStateException("Illegal context state");

        try {
            final int[] iparams = new int[1];

            iparams[0] = ((options & OPTION_USE_FADE) != 0) ? 1 : 0;

            mNativeHandle = createNativeImplHandle(
                    contextHandle,
                    new WeakReference<OpenSLMediaPlayer>(this),
                    iparams);
        } catch (Throwable e) {
        }

        if (mNativeHandle == 0) {
            throw new IllegalStateException(
                    "Failed to create OpenSLMediaPlayer instance in native layer");
        }

        mHandler = new InternalHandler(this);
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    @Override
    public void setDataSource(Context context, Uri uri)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        if (context == null) {
            throw new NullPointerException();
            // throw new
            // IllegalArgumentException("The argument context cannot be null");
        }
        if (uri == null) {
            throw new NullPointerException();
            // throw new
            // IllegalArgumentException("The argument uri cannot be null");
        }

        checkNativeImplIsAvailable();

        final String scheme = uri.getScheme();
        if ("file".equals(scheme)) {
            setDataSource(uri.getPath());
        } else if ("content".equals(scheme)) {
            setDataSourceInternalContentUri(context, uri);
        } else {
            setDataSourceInternalNonContentUri(context, uri);
        }
    }

    @Override
    public void setDataSource(String path)
            throws IOException, IllegalArgumentException, IllegalStateException {
        if (path == null) {
            throw new NullPointerException();
            // throw new
            // IllegalArgumentException("The argument path cannot be null");
        }

        try {
            // fix "file://" URI form string
            final Uri uri = Uri.parse(path);
            if ("file".equals(uri.getScheme())) {
                path = uri.getPath();
            }
        } catch (Exception e) {
        }

        checkNativeImplIsAvailable();

        final int result = setDataSourcePathImplNative(mNativeHandle, path);
        parseResultAndThrowException(result);
    }

    @Override
    public void setDataSource(FileDescriptor fd)
            throws IOException, IllegalArgumentException, IllegalStateException {
        final int nativeFD = checkAndObtainNativeFileDescriptor(fd);

        checkNativeImplIsAvailable();

        final int result = setDataSourceFdImplNative(mNativeHandle, nativeFD);
        parseResultAndThrowException(result);
    }

    @Override
    public void setDataSource(FileDescriptor fd, long offset, long length)
            throws IOException, IllegalArgumentException, IllegalStateException {
        if (length < 0)
            throw new IllegalArgumentException("The argument length must positive or zero");

        final int nativeFD = checkAndObtainNativeFileDescriptor(fd);
        final long fileSize = getFileSize(fd);

        if (fileSize < 0)
            throw new IOException("Can't obtain file size");

        if (offset < 0 || offset > fileSize)
            throw new IllegalArgumentException("File offset is invalid");

        if ((fileSize - offset) > length)
            throw new IllegalArgumentException(
                    "Specified file range is larger than actual file size");

        checkNativeImplIsAvailable();

        final int result = setDataSourceFdImplNative(mNativeHandle, nativeFD, offset, length);
        parseResultAndThrowException(result);
    }

    @Override
    public void prepare() throws IOException, IllegalStateException {
        checkNativeImplIsAvailable();

        final int result = prepareImplNative(mNativeHandle);
        parseResultAndThrowException(result);
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        checkNativeImplIsAvailable();

        final int result = prepareAsyncImplNative(mNativeHandle);
        parseResultAndThrowExceptForIOExceptions(result);
    }

    @Override
    public void start() throws IllegalStateException {
        checkNativeImplIsAvailable();

        stayAwake(true);

        final int result = startImplNative(mNativeHandle);
        parseResultAndThrowNoExceptions(result);
    }

    @Override
    public void stop() throws IllegalStateException {
        if (LOCAL_LOGD) {
            Log.d(TAG, "stop()");
        }
        checkNativeImplIsAvailable();

        stayAwake(false);

        final int result = stopImplNative(mNativeHandle);
        parseResultAndThrowNoExceptions(result);
    }

    @Override
    public void pause() throws IllegalStateException {
        if (LOCAL_LOGD) {
            Log.d(TAG, "pause()");
        }
        checkNativeImplIsAvailable();

        stayAwake(false);

        final int result = pauseImplNative(mNativeHandle);
        parseResultAndThrowNoExceptions(result);

        if (mHandler != null) {
            mHandler.clearBufferingUpdateMessage();
        }
    }

    @Override
    public void reset() {
        if (LOCAL_LOGD) {
            Log.d(TAG, "reset()");
        }
        checkNativeImplIsAvailable();

        stayAwake(false);

        if (mNativeHandle != 0) {
            final int result = resetImplNative(mNativeHandle);
            parseResultAndThrowExceptForIOExceptions(result);
        }

        if (mHandler != null) {
            mHandler.clearPendingMessages();
        }

        releaseOpenedContentFileDescriptor();
    }

    @Override
    public void release() {
        stayAwake(false);

        if (mHandler != null) {
            mHandler.release();
            mHandler = null;
        }

        mOnCompletionListener = null;
        mOnPreparedListener = null;
        mOnSeekCompleteListener = null;
        mOnBufferingUpdateListener = null;
        mOnInfoListener = null;
        mOnErrorListener = null;

        try {
            if (HAS_NATIVE && mNativeHandle != 0) {
                deleteNativeImplHandle(mNativeHandle);
                mNativeHandle = 0;
            }
        } catch (Exception e) {
            Log.e(TAG, "release()", e);
        }

        releaseOpenedContentFileDescriptor();
    }

    @Override
    public void seekTo(int msec) throws IllegalStateException {
        checkNativeImplIsAvailable();

        if (mNativeHandle != 0) {
            final int result = seekToImplNative(mNativeHandle, msec);
            parseResultAndThrowNoExceptions(result);
        }
    }

    @Override
    public int getAudioSessionId() {
        checkNativeImplIsAvailable();

        // NOTE:
        // This method always returns 0 when using OpenSL sink backend.

        if (mNativeHandle != 0) {
            getAudioSessionIdImplNative(mNativeHandle, mParamIntBuff);
            return mParamIntBuff[0];
        } else {
            return 0;
        }
    }

    @Override
    public void setAudioSessionId(int sessionId) throws IllegalArgumentException,
            IllegalStateException {
        checkNativeImplIsAvailable();

        throw new IllegalStateException("This method is not supported");
    }

    @Override
    public int getDuration() {
        checkNativeImplIsAvailable();

        if (mNativeHandle != 0) {
            getDurationImplNative(mNativeHandle, mParamIntBuff);
            return mParamIntBuff[0];
        }
        return 0;
    }

    @Override
    public void setLooping(boolean looping) {
        checkNativeImplIsAvailable();

        try {
            final int result = setLoopingImplNative(mNativeHandle, looping);
            parseResultAndThrowException(result);
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in setLooping(looping = " + looping + ")");
        }
    }

    @Override
    public int getCurrentPosition() {
        checkNativeImplIsAvailable();

        try {
            getCurrentPositionImplNative(mNativeHandle, mParamIntBuff);
            return mParamIntBuff[0];
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in getCurrentPosition()");
        }

        return 0;
    }

    @Override
    public boolean isLooping() {
        checkNativeImplIsAvailable();

        try {
            final int result = isLoopingImplNative(mNativeHandle, mParamBoolBuff);
            parseResultAndThrowException(result);
            return mParamBoolBuff[0];
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in getCurrentPosition()");
        }

        return false;
    }

    @Override
    public boolean isPlaying() throws IllegalStateException {
        checkNativeImplIsAvailable();

        try {
            final int result = isPlayingImplNative(mNativeHandle, mParamBoolBuff);
            parseResultAndThrowException(result);
            return mParamBoolBuff[0];
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in getCurrentPosition()");
        }

        return false;
    }

    @Override
    public void attachAuxEffect(int effectId) throws IllegalArgumentException,
            IllegalStateException {

        checkNativeImplIsAvailable();

        try {
            final int result = attachAuxEffectImplNative(mNativeHandle, effectId);
            parseResultAndThrowNoExceptions(result);
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in attachAuxEffect(effectId = " + effectId + ")");
        }
    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
        checkNativeImplIsAvailable();

        try {
            final int result = setVolumeImplNative(mNativeHandle, leftVolume, rightVolume);
            parseResultAndThrowNoExceptions(result);
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in setVolume(leftVolume = "
                    + leftVolume + ", rightVolume = " + rightVolume + ")");
        }
    }

    @Override
    public void setAudioStreamType(int streamtype) {
        checkNativeImplIsAvailable();

        try {
            final int result = setAudioStreamTypeImplNative(mNativeHandle, streamtype);
            parseResultAndThrowNoExceptions(result);
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in setAudioStreamType(streamtype = "
                    + streamtype + ")");
        }
    }

    @Override
    public void setAuxEffectSendLevel(float level) {
        checkNativeImplIsAvailable();

        try {
            final int result = setAuxEffectSendLevelImplNative(mNativeHandle, level);

            parseResultAndThrowNoExceptions(result);
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in setAuxEffectSendLevel(level = "
                    + level + ")");
        }
    }

    @Override
    public void setWakeMode(Context context, int mode) {
        checkNativeImplIsAvailable();

        if (context == null) {
            throw new IllegalArgumentException("The argument context must not be null");
        }

        // re-create the wake lock object
        final boolean wasHeld = releaseWakeLockObject();

        createWakeLockObject(context, mode);

        if (wasHeld) {
            stayAwake(true);
        }
    }

    @Override
    public void setOnBufferingUpdateListener(OnBufferingUpdateListener listener) {
        if (!isNativeImplIsAvailable()) {
            return;
        }
        mOnBufferingUpdateListener = listener;
    }

    @Override
    public void setOnCompletionListener(OnCompletionListener listener) {
        if (!isNativeImplIsAvailable()) {
            return;
        }
        mOnCompletionListener = listener;
    }

    @Override
    public void setOnErrorListener(OnErrorListener listener) {
        if (!isNativeImplIsAvailable()) {
            return;
        }
        mOnErrorListener = listener;
    }

    @Override
    public void setOnInfoListener(OnInfoListener listener) {
        if (!isNativeImplIsAvailable()) {
            return;
        }
        mOnInfoListener = listener;
    }

    @Override
    public void setOnPreparedListener(OnPreparedListener listener) {
        if (!isNativeImplIsAvailable()) {
            return;
        }
        mOnPreparedListener = listener;
    }

    @Override
    public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
        if (!isNativeImplIsAvailable()) {
            return;
        }
        mOnSeekCompleteListener = listener;
    }

    @Override
    public void setNextMediaPlayer(IBasicMediaPlayer next) {
        checkNativeImplIsAvailable();

        if (next != null && !(next instanceof OpenSLMediaPlayer)) {
            throw new IllegalArgumentException("Not OpenSLMediaPlayer instance");
        }

        OpenSLMediaPlayer next2 = (OpenSLMediaPlayer) next;

        if (next2 == this) {
            throw new IllegalArgumentException("Can't assign the self instance as a next player");
        }

        if ((next2 != null) && next2.mNativeHandle == 0) {
            throw new IllegalStateException("The next player has already been released.");
        }

        final long nextHandle = (next2 == null) ? 0 : (next2.mNativeHandle);

        try {
            final int result = setNextMediaPlayerImplNative(mNativeHandle, nextHandle);

            parseResultAndThrowExceptForIOExceptions(result);
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Exception e) {
            Log.e(TAG, "An error occurred in setNextMediaPlayer(nextHandle = "
                    + nextHandle + ")");
        }
    }

    @Override
    public void setAudioAttributes(AudioAttributes attributes) {
        if (attributes == null) {
            throw new IllegalArgumentException();
        }

        checkNativeImplIsAvailable();

        if (LOCAL_LOGV) {
            Log.v(TAG, "setAudioAttributes() is not supported, just ignored");
        }
    }

    //
    // setDataSource(Context, Uri) implementation
    //
    private void setDataSourceInternalNonContentUri(Context context, Uri uri)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        String strUri = uri.toString();

        final int result = setDataSourceUriImplNative(mNativeHandle, strUri);
        parseResultAndThrowException(result);
    }

    private void setDataSourceInternalContentUri(Context context, Uri uri)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        final ContentResolver cr = context.getContentResolver();

        AssetFileDescriptor afd = cr.openAssetFileDescriptor(uri, "r");
        FileDescriptor fd = afd.getFileDescriptor();
        final int nativeFD;

        try {
            nativeFD = checkAndObtainNativeFileDescriptor(fd);
        } catch (IllegalArgumentException e) {
            closeQuietly(afd);
            throw e;
        }

        final int result;
        final long declLength = afd.getDeclaredLength();
        final long startOffset = afd.getStartOffset();
        if (declLength < 0) {
            result = setDataSourceFdImplNative(mNativeHandle, nativeFD);
        } else {
            result = setDataSourceFdImplNative(
                    mNativeHandle, nativeFD, startOffset, declLength);
        }

        parseResultAndThrowException(result);

        mContentAssetFd = afd;
    }

    private void releaseOpenedContentFileDescriptor() {
        if (mContentAssetFd == null) {
            return;
        }

        closeQuietly(mContentAssetFd);
        mContentAssetFd = null;
    }

    //
    // Wake lock
    //

    private void createWakeLockObject(Context context, int mode) {
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        WakeLock wakelock = pm.newWakeLock(
                mode | PowerManager.ON_AFTER_RELEASE,
                WAKELOCK_TAG);
        wakelock.setReferenceCounted(false);

        mWakeLock = wakelock;
    }

    private boolean releaseWakeLockObject() {
        if (mWakeLock == null)
            return false;

        final boolean wasHeld = mWakeLock.isHeld();
        mWakeLock.release();
        mWakeLock = null;

        return wasHeld;
    }

    @SuppressLint("Wakelock")
    private void stayAwake(boolean awake) {
        if (mWakeLock == null)
            return;

        if (awake) {
            mWakeLock.acquire();
        } else {
            mWakeLock.release();
        }
    }

    //
    // Utilities
    //

    private boolean isNativeImplIsAvailable()  {
        return (mNativeHandle != 0);
    }

    private void checkNativeImplIsAvailable() throws IllegalStateException {
        if (!isNativeImplIsAvailable()) {
            throw new IllegalStateException("Native implemenation handle is not present");
        }
    }

    private static int checkAndObtainNativeFileDescriptor(FileDescriptor fd) {
        if (fd == null)
            throw new IllegalArgumentException("The argument fd cannot be null");
        if (!fd.valid())
            throw new IllegalArgumentException("File descriptor is not vailed");

        final int nativeFD = getNativeFD(fd);

        if (nativeFD == 0)
            throw new IllegalArgumentException("File descriptor is not vailed");

        return nativeFD;
    }

    private static int getNativeFD(FileDescriptor fd) {
        final Field descField = mField_FileDescriptor_descriptor;

        if (fd == null)
            return 0;
        if (descField == null)
            return 0;

        try {
            return descField.getInt(fd);
        } catch (IllegalAccessException e) {
            return 0;
        } catch (IllegalArgumentException e) {
            return 0;
        }
    }

    private static long getFileSize(FileDescriptor fd) {
        FileInputStream stream = null;
        FileChannel channel = null;
        long fileSize = -1;
        try {
            stream = new FileInputStream(fd);
            channel = stream.getChannel();
            fileSize = channel.size();
        } catch (Exception e) {
        } finally {
            if (channel != null) {
                try {
                    channel.close();
                    channel = null;
                    stream = null; // stream is already closed
                } catch (Exception e) {
                }
            }

            if (stream != null) {
                try {
                    stream.close();
                    stream = null;
                } catch (Exception e) {
                }
            }
        }

        return fileSize;
    }

    /*
     * NOTE: The AssetFileDescriptor does not implement
    　* Closeable interface until API level 19 (issue #8)
     */
    static void closeQuietly(AssetFileDescriptor afd) {
        if (afd != null) {
            try {
                afd.close();
            } catch (IOException e) {
                Log.w(TAG, "closeQuietly() " + e.getStackTrace());
            }
        }
    }

    //
    // Event handlers
    //
    private void handleOnCompletion() {
        stayAwake(false);

        if (mOnCompletionListener != null) {
            mOnCompletionListener.onCompletion(this);
        }
    }

    private void handleOnPrepared() {
        if (mOnPreparedListener != null) {
            mOnPreparedListener.onPrepared(this);
        }
    }

    private void handleOnSeekComplete() {
        if (mOnSeekCompleteListener != null) {
            mOnSeekCompleteListener.onSeekComplete(this);
        }
    }

    private void handleOnBufferingUpdate(int percent) {
        if (mOnBufferingUpdateListener != null) {
            mOnBufferingUpdateListener.onBufferingUpdate(this, percent);
        }
    }

    private void handleOnInfo(int what, int extra) {
        // boolean handled = false;
        if (mOnInfoListener != null) {
            /* handled = */mOnInfoListener.onInfo(this, what, extra);
        }
    }

    private void handleOnError(int what, int extra) {
        boolean handled = false;

        stayAwake(false);

        if (mOnErrorListener != null) {
            handled = mOnErrorListener.onError(this, what, extra);
        }

        if (!handled) {
            if (mOnCompletionListener != null) {
                mOnCompletionListener.onCompletion(this);
            }
        }
    }

    //
    // Internal methods accessor
    //
    /** @hide */
    public static class Internal {
        // result codes
        public static final int RESULT_SUCCESS = 0;
        public static final int RESULT_ERROR = -1;
        public static final int RESULT_INVALID_HANDLE = -2;
        public static final int RESULT_ILLEGAL_STATE = -3;
        public static final int RESULT_ILLEGAL_ARGUMENT = -4;
        public static final int RESULT_INTERNAL_ERROR = -5;
        public static final int RESULT_MEMORY_ALLOCATION_FAILED = -6;
        public static final int RESULT_RESOURCE_ALLOCATION_FAILED = -7;
        public static final int RESULT_CONTENT_NOT_FOUND = -8;
        public static final int RESULT_CONTENT_UNSUPPORTED = -9;
        public static final int RESULT_IO_ERROR = -10;
        public static final int RESULT_PERMISSION_DENIED = -11;
        public static final int RESULT_TIMED_OUT = -12;
        public static final int RESULT_IN_ERROR_STATE = -13;
        public static final int RESULT_CONTROL_LOST = -14;
        public static final int RESULT_DEAD_OBJECT = -15;

        // constants
        public static final int AUX_EFFECT_NULL = 0;
        public static final int AUX_EFFECT_ENVIRONMENTAL_REVERB = 1;
        public static final int AUX_EFFECT_PRESET_REVERB = 2;

        public static void parseResultAndThrowException(int result) throws IOException {
            OpenSLMediaPlayer.parseResultAndThrowException(result);
        }

        public static void parseResultAndThrowExceptForIOExceptions(int result) {
            OpenSLMediaPlayer.parseResultAndThrowExceptForIOExceptions(result);
        }

        public static long getNativeHandle(OpenSLMediaPlayer player) {
            return (player != null) ? player.mNativeHandle : 0;
        }

        public static long getNativeHandle(OpenSLMediaPlayerContext context) {
            return (context != null) ? context.getNativeHandle() : 0;
        }

        public static int getAudioSessionId(OpenSLMediaPlayerContext context) {
            return (context != null) ? context.getAudioSessionId() : 0;
        }
    }

    //
    // Internal Handler
    //

    static class InternalHandler extends Handler {
        private WeakReference<OpenSLMediaPlayer> mRefPlayer;

        public InternalHandler(OpenSLMediaPlayer player) {
            super();
            mRefPlayer = new WeakReference<OpenSLMediaPlayer>(player);
        }

        public void clearBufferingUpdateMessage() {
            removeMessages(MESSAGE_ON_BUFFERING_UPDATE);
        }

        public void clearPendingMessages() {
            removeMessages(MESSAGE_NOP);
            removeMessages(MESSAGE_ON_COMPLETION);
            removeMessages(MESSAGE_ON_PREPARED);
            removeMessages(MESSAGE_ON_SEEK_COMPLETE);
            removeMessages(MESSAGE_ON_BUFFERING_UPDATE);
            removeMessages(MESSAGE_ON_INFO);
            removeMessages(MESSAGE_ON_ERROR);
        }

        public void release() {
            mRefPlayer.clear();
            clearPendingMessages();
        }

        @Override
        public void handleMessage(Message msg) {
            final OpenSLMediaPlayer mp = mRefPlayer.get();

            if (mp == null)
                return;

            mp.handleMessage(msg);
        }
    }

    //
    // JNI binder internal methods
    //

    private static final int MESSAGE_NOP = 0;
    private static final int MESSAGE_ON_COMPLETION = 1;
    private static final int MESSAGE_ON_PREPARED = 2;
    private static final int MESSAGE_ON_SEEK_COMPLETE = 3;
    private static final int MESSAGE_ON_BUFFERING_UPDATE = 4;
    private static final int MESSAGE_ON_INFO = 5;
    private static final int MESSAGE_ON_ERROR = 6;

    private static String toStringMessageCode(int code) {
        switch (code) {
            case MESSAGE_NOP:
                return "MESSAGE_NOP";
            case MESSAGE_ON_COMPLETION:
                return "MESSAGE_ON_COMPLETION";
            case MESSAGE_ON_PREPARED:
                return "MESSAGE_ON_PREPARED";
            case MESSAGE_ON_SEEK_COMPLETE:
                return "MESSAGE_ON_SEEK_COMPLETE";
            case MESSAGE_ON_BUFFERING_UPDATE:
                return "MESSAGE_ON_BUFFERING_UPDATE";
            case MESSAGE_ON_INFO:
                return "MESSAGE_ON_INFO";
            case MESSAGE_ON_ERROR:
                return "MESSAGE_ON_ERROR";
            default:
                return "FIXME";
        }
    }

    private void handleMessage(Message msg) {
        if (LOCAL_LOGD) {
            Log.d(TAG, "handleMessage(msg = " + toStringMessageCode(msg.what) + ")");
        }
        switch (msg.what) {
            case MESSAGE_NOP:
                break;
            case MESSAGE_ON_COMPLETION:
                handleOnCompletion();
                break;
            case MESSAGE_ON_PREPARED:
                handleOnPrepared();
                break;
            case MESSAGE_ON_SEEK_COMPLETE:
                handleOnSeekComplete();
                break;
            case MESSAGE_ON_BUFFERING_UPDATE:
                handleOnBufferingUpdate(msg.arg1);
                break;
            case MESSAGE_ON_INFO:
                handleOnInfo(msg.arg1, msg.arg2);
                break;
            case MESSAGE_ON_ERROR:
                handleOnError(msg.arg1, msg.arg2);
                break;
        }
    }

    @SuppressWarnings("unchecked")
    private static void postEventFromNative(
            Object ref, int what, int arg1, int arg2, Object obj) {
        final WeakReference<OpenSLMediaPlayer> mpref = ((WeakReference<OpenSLMediaPlayer>) ref);
        final OpenSLMediaPlayer mp = mpref.get();

        if (mp == null)
            return;

        final InternalHandler handler = mp.mHandler;
        if (handler == null)
            return;

        final Message msg = handler.obtainMessage();

        msg.what = what;
        msg.arg1 = arg1;
        msg.arg2 = arg2;
        msg.obj = obj;

        handler.sendMessage(msg);

        if (LOCAL_LOGD) {
            Log.d(TAG, "postEventFromNative(msg = " + toStringMessageCode(msg.what) + ")");
        }
    }

    private static boolean parseResultAndThrowNoExceptions(int result) {
        try {
            parseResultAndThrowException(result);
            return true;
        } catch (Exception e) {
            Log.w(TAG, "An error occurred", e);
            return false;
        }
    }

    private static void parseResultAndThrowExceptForIOExceptions(int result) {
        try {
            parseResultAndThrowException(result);
        } catch (IOException e) {
            throw new IllegalStateException("An unexpected IOException occurred");
        }
    }

    private static void parseResultAndThrowException(int result) throws IOException {
        switch (result) {
            case Internal.RESULT_SUCCESS:
                break;
            case Internal.RESULT_ERROR:
                throw new IllegalStateException("General error");
            case Internal.RESULT_INVALID_HANDLE:
                throw new IllegalStateException("Invalid handle");
            case Internal.RESULT_ILLEGAL_STATE:
                throw new IllegalStateException("Method is called in unexpected state");
            case Internal.RESULT_ILLEGAL_ARGUMENT:
                throw new IllegalArgumentException("Illegal argument error occurred in native layer");
            case Internal.RESULT_INTERNAL_ERROR:
                throw new IllegalStateException("Internal error");
            case Internal.RESULT_MEMORY_ALLOCATION_FAILED:
                throw new IllegalStateException("Failed to allocate memory in native layer");
            case Internal.RESULT_RESOURCE_ALLOCATION_FAILED:
                throw new IllegalStateException("Failed to allocate resources in native layer");
            case Internal.RESULT_CONTENT_NOT_FOUND:
                throw new IOException("Content not found");
            case Internal.RESULT_CONTENT_UNSUPPORTED:
                throw new IOException("Unsupported content");
            case Internal.RESULT_IO_ERROR:
                throw new IOException("I/O error");
            case Internal.RESULT_PERMISSION_DENIED:
                throw new IOException("Permission denied");
            case Internal.RESULT_TIMED_OUT:
                throw new IllegalStateException("Timed out");
            case Internal.RESULT_CONTROL_LOST:
                throw new UnsupportedOperationException("Control lost");
            case Internal.RESULT_IN_ERROR_STATE:
                /* don't throw any exceptions */
                break;
            case Internal.RESULT_DEAD_OBJECT:
                throw new IllegalStateException("Dead object");
            default:
                throw new IllegalStateException(
                        "Unexpected error (0x" + Integer.toHexString(result) + ")");
        }
    }

    //
    // Native methods
    //
    private static native long createNativeImplHandle(
            long contextHandle, WeakReference<OpenSLMediaPlayer> weak_thiz, int[] params);

    private static native void deleteNativeImplHandle(long handle);

    private static native int setDataSourcePathImplNative(long handle, String path);

    private static native int setDataSourceUriImplNative(long handle, String uri);

    private static native int setDataSourceFdImplNative(long handle, int fd);

    private static native int setDataSourceFdImplNative(long handle, int fd, long offset,
            long length);

    private static native int attachAuxEffectImplNative(long handle, int effectId);

    private static native int setAuxEffectSendLevelImplNative(long handle, float level);

    private static native int prepareImplNative(long handle);

    private static native int prepareAsyncImplNative(long handle);

    private static native int startImplNative(long handle);

    private static native int stopImplNative(long handle);

    private static native int pauseImplNative(long handle);

    private static native int resetImplNative(long handle);

    private static native int setVolumeImplNative(long handle, float leftVolume, float rightVolume);

    private static native int getAudioSessionIdImplNative(long handle, int[] audioSessionId);

    private static native int getDurationImplNative(long handle, int[] duration);

    private static native int getCurrentPositionImplNative(long handle, int[] position);

    private static native int seekToImplNative(long handle, int msec);

    private static native int setLoopingImplNative(long handle, boolean looping);

    private static native int isLoopingImplNative(long handle, boolean[] looping);

    private static native int isPlayingImplNative(long handle, boolean[] playing);

    private static native int setAudioStreamTypeImplNative(long handle, int streamtype);

    private static native int setNextMediaPlayerImplNative(long handle, long nextHandle);
}
