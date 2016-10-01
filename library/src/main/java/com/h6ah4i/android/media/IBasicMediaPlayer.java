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

/// ===============================================================
// Most of declarations and Javadoc comments are copied from
// /frameworks/base/media/java/android/media/MediaPlayer.java
/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/// ===============================================================

package com.h6ah4i.android.media;

import java.io.FileDescriptor;
import java.io.IOException;

import android.content.Context;
import android.media.AudioManager;
import android.net.Uri;

import com.h6ah4i.android.media.compat.AudioAttributes;

public interface IBasicMediaPlayer extends IReleasable {
    /**
     * Unspecified media player error.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_UNKNOWN = 1;

    /**
     * Media server died. In this case, the application must release the
     * MediaPlayer object and instantiate a new one.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_SERVER_DIED = 100;

    /**
     * The video is streamed and its container is not valid for progressive
     * playback i.e the video's index (e.g moov atom) is not at the start of the
     * file.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200;

    /**
     * Some operation takes too long to complete, usually more than 3-5 seconds.
     */
    public static final int MEDIA_ERROR_TIMED_OUT = -110;

    /** File or network related operation errors. */
    public static final int MEDIA_ERROR_IO = -1004;

    /** Bitstream is not conforming to the related coding standard or file spec. */
    public static final int MEDIA_ERROR_MALFORMED = -1007;

    /**
     * Bitstream is conforming to the related coding standard or file spec, but
     * the media framework does not support the feature.
     */
    public static final int MEDIA_ERROR_UNSUPPORTED = -1010;

    /**
     * Unspecified media player info.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_UNKNOWN = 1;

    /**
     * The player just pushed the very first video frame for rendering.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_VIDEO_RENDERING_START = 3;

    /**
     * The video is too complex for the decoder: it can't decode frames fast
     * enough. Possibly only the audio plays fine at this stage.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_VIDEO_TRACK_LAGGING = 700;

    /**
     * MediaPlayer is temporarily pausing playback internally in order to buffer
     * more data.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_BUFFERING_START = 701;

    /**
     * MediaPlayer is resuming playback after filling buffers.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_BUFFERING_END = 702;

    /**
     * Bad interleaving means that a media has been improperly interleaved or
     * not interleaved at all, e.g has all the video samples first then all the
     * audio ones. Video is playing but a lot of disk seeks may be happening.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_BAD_INTERLEAVING = 800;

    /**
     * The media cannot be seeked (e.g live stream)
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_NOT_SEEKABLE = 801;

    /**
     * A new set of metadata is available.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_METADATA_UPDATE = 802;

    /**
     * Subtitle track was not supported by the media framework.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_UNSUPPORTED_SUBTITLE = 901;

    /**
     * Reading the subtitle track takes too long.
     *
     * @see com.h6ah4i.android.media.IBasicMediaPlayer.OnInfoListener
     */
    public static final int MEDIA_INFO_SUBTITLE_TIMED_OUT = 902;

    /**
     * Interface definition for a callback to be invoked when playback of a
     * media source has completed.
     */
    public interface OnCompletionListener {
        /**
         * Called when the end of a media source is reached during playback.
         *
         * @param mp the MediaPlayer that reached the end of the file
         */
        void onCompletion(IBasicMediaPlayer mp);
    }

    /**
     * Interface definition for a callback to be invoked when the media source
     * is ready for playback.
     */
    public interface OnPreparedListener {
        /**
         * Called when the media file is ready for playback.
         *
         * @param mp the MediaPlayer that is ready for playback
         */
        void onPrepared(IBasicMediaPlayer mp);
    }

    /**
     * Interface definition of a callback to be invoked indicating the
     * completion of a seek operation.
     */
    public interface OnSeekCompleteListener {
        /**
         * Called to indicate the completion of a seek operation.
         *
         * @param mp the MediaPlayer that issued the seek operation
         */
        void onSeekComplete(IBasicMediaPlayer mp);
    }

    /**
     * Interface definition of a callback to be invoked indicating buffering
     * status of a media resource being streamed over the network.
     */
    public interface OnBufferingUpdateListener {
        /**
         * Called to update status in buffering a media stream received through
         * progressive HTTP download. The received buffering percentage
         * indicates how much of the content has been buffered or played. For
         * example a buffering update of 80 percent when half the content has
         * already been played indicates that the next 30 percent of the content
         * to play has been buffered.
         *
         * @param mp the MediaPlayer the update pertains to
         * @param percent the percentage (0-100) of the content that has been
         *            buffered or played thus far
         */
        void onBufferingUpdate(IBasicMediaPlayer mp, int percent);
    }

    /**
     * Interface definition of a callback to be invoked to communicate some info
     * and/or warning about the media or its playback.
     */
    public interface OnInfoListener {
        /**
         * Called to indicate an info or a warning.
         *
         * @param mp the MediaPlayer the info pertains to.
         * @param what the type of info or warning.
         *            <ul>
         *            <li>{@link #MEDIA_INFO_UNKNOWN}
         *            <li>{@link #MEDIA_INFO_VIDEO_TRACK_LAGGING}
         *            <li>{@link #MEDIA_INFO_VIDEO_RENDERING_START}
         *            <li>{@link #MEDIA_INFO_BUFFERING_START}
         *            <li>{@link #MEDIA_INFO_BUFFERING_END}
         *            <li>{@link #MEDIA_INFO_BAD_INTERLEAVING}
         *            <li>{@link #MEDIA_INFO_NOT_SEEKABLE}
         *            <li>{@link #MEDIA_INFO_METADATA_UPDATE}
         *            <li>{@link #MEDIA_INFO_UNSUPPORTED_SUBTITLE}
         *            <li>{@link #MEDIA_INFO_SUBTITLE_TIMED_OUT}
         *            </ul>
         * @param extra an extra code, specific to the info. Typically
         *            implementation dependent.
         * @return True if the method handled the info, false if it didn't.
         *         Returning false, or not having an OnErrorListener at all,
         *         will cause the info to be discarded.
         */
        boolean onInfo(IBasicMediaPlayer mp, int what, int extra);
    }

    /**
     * Interface definition of a callback to be invoked when there has been an
     * error during an asynchronous operation (other errors will throw
     * exceptions at method call time).
     */
    public interface OnErrorListener {
        /**
         * Called to indicate an error.
         *
         * @param mp the MediaPlayer the error pertains to
         * @param what the type of error that has occurred:
         *            <ul>
         *            <li>{@link #MEDIA_ERROR_UNKNOWN}
         *            <li>{@link #MEDIA_ERROR_SERVER_DIED}
         *            </ul>
         * @param extra an extra code, specific to the error. Typically
         *            implementation dependent.
         *            <ul>
         *            <li>{@link #MEDIA_ERROR_IO}
         *            <li>{@link #MEDIA_ERROR_MALFORMED}
         *            <li>{@link #MEDIA_ERROR_UNSUPPORTED}
         *            <li>{@link #MEDIA_ERROR_TIMED_OUT}
         *            </ul>
         * @return True if the method handled the error, false if it didn't.
         *         Returning false, or not having an OnErrorListener at all,
         *         will cause the OnCompletionListener to be called.
         */
        boolean onError(IBasicMediaPlayer mp, int what, int extra);
    }

    /**
     * Starts or resumes playback. If playback had previously been paused,
     * playback will continue from where it was paused. If playback had been
     * stopped, or never started before, playback will start at the beginning.
     *
     * @throws IllegalStateException if it is called in an invalid state
     */
    void start()
            throws IllegalStateException;

    /**
     * Stops playback after playback has been stopped or paused.
     *
     * @throws IllegalStateException if the internal player engine has not been
     *             initialized.
     */
    void stop()
            throws IllegalStateException;

    /**
     * Pauses playback. Call start() to resume.
     *
     * @throws IllegalStateException if the internal player engine has not been
     *             initialized.
     */
    void pause()
            throws IllegalStateException;

    /**
     * Sets the data source as a content Uri.
     *
     * @param context the Context to use when resolving the Uri
     * @param uri the Content URI of the data you want to play
     * @throws IllegalStateException if it is called in an invalid state
     */
    void setDataSource(Context context, Uri uri)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    /**
     * Sets the data source (FileDescriptor) to use. It is the caller's
     * responsibility to close the file descriptor. It is safe to do so as soon
     * as this call returns.
     *
     * @param fd the FileDescriptor for the file you want to play
     * @throws IllegalStateException if it is called in an invalid state
     */
    void setDataSource(FileDescriptor fd)
            throws IOException, IllegalArgumentException, IllegalStateException;

    /**
     * Sets the data source (FileDescriptor) to use. The FileDescriptor must be
     * seekable (N.B. a LocalSocket is not seekable). It is the caller's
     * responsibility to close the file descriptor. It is safe to do so as soon
     * as this call returns.
     *
     * @param fd the FileDescriptor for the file you want to play
     * @param offset the offset into the file where the data to be played
     *            starts, in bytes
     * @param length the length in bytes of the data to be played
     * @throws IllegalStateException if it is called in an invalid state
     */
    void setDataSource(FileDescriptor fd, long offset, long length)
            throws IOException, IllegalArgumentException, IllegalStateException;

    /**
     * Sets the data source (file-path or http/rtsp URL) to use.
     *
     * @param path the path of the file, or the http/rtsp URL of the stream you
     *            want to play
     * @throws IllegalStateException if it is called in an invalid state
     *             <p>
     *             When <code>path</code> refers to a local file, the file may
     *             actually be opened by a process other than the calling
     *             application. This implies that the pathname should be an
     *             absolute path (as any other process runs with unspecified
     *             current working directory), and that the pathname should
     *             reference a world-readable file. As an alternative, the
     *             application could first open the file for reading, and then
     *             use the file descriptor form
     *             {@link #setDataSource(FileDescriptor)}.
     */
    void setDataSource(String path)
            throws IOException, IllegalArgumentException, IllegalStateException;

    /**
     * Prepares the player for playback, synchronously. After setting the
     * datasource and the display surface, you need to either call prepare() or
     * prepareAsync(). For files, it is OK to call prepare(), which blocks until
     * MediaPlayer is ready for playback.
     *
     * @throws IllegalStateException if it is called in an invalid state
     */
    void prepare() throws IOException, IllegalStateException;

    /**
     * Prepares the player for playback, asynchronously. After setting the
     * datasource and the display surface, you need to either call prepare() or
     * prepareAsync(). For streams, you should call prepareAsync(), which
     * returns immediately, rather than blocking until enough data has been
     * buffered.
     *
     * @throws IllegalStateException if it is called in an invalid state
     */
    void prepareAsync() throws IllegalStateException;

    /**
     * Seeks to specified time position.
     *
     * @param msec the offset in milliseconds from the start to seek to
     * @throws IllegalStateException if the internal player engine has not been
     *             initialized
     */
    void seekTo(int msec)
            throws IllegalStateException;

    /**
     * Releases resources associated with this MediaPlayer object. It is
     * considered good practice to call this method when you're done using the
     * MediaPlayer. In particular, whenever an Activity of an application is
     * paused (its onPause() method is called), or stopped (its onStop() method
     * is called), this method should be invoked to release the MediaPlayer
     * object, unless the application has a special need to keep the object
     * around. In addition to unnecessary resources (such as memory and
     * instances of codecs) being held, failure to call this method immediately
     * if a MediaPlayer object is no longer needed may also lead to continuous
     * battery consumption for mobile devices, and playback failure for other
     * applications if no multiple instances of the same codec are supported on
     * a device. Even if multiple instances of the same codec are supported,
     * some performance degradation may be expected when unnecessary multiple
     * instances are used at the same time.
     */
    @Override
    void release();

    /**
     * Resets the MediaPlayer to its uninitialized state. After calling this
     * method, you will have to initialize it again by setting the data source
     * and calling prepare().
     */
    void reset();

    /**
     * Returns the audio session ID.
     *
     * @return the audio session ID. Note that
     *         the audio session ID is 0 only if a problem occurred when the
     *         MediaPlayer was contructed.
     */
    int getAudioSessionId();

    /**
     * Sets the audio session ID.
     *
     * @param sessionId the audio session ID. The audio session ID is a system
     *            wide unique identifier for the audio stream played by this
     *            MediaPlayer instance. The primary use of the audio session ID
     *            is to associate audio effects to a particular instance of
     *            MediaPlayer: if an audio session ID is provided when creating
     *            an audio effect, this effect will be applied only to the audio
     *            content of media players within the same audio session and not
     *            to the output mix. When created, a MediaPlayer instance
     *            automatically generates its own audio session ID. However, it
     *            is possible to force this player to be part of an already
     *            existing audio session by calling this method. This method
     *            must be called before one of the overloaded
     *            <code> setDataSource </code> methods.
     * @throws IllegalStateException if it is called in an invalid state
     */
    void setAudioSessionId(int sessionId)
            throws IllegalArgumentException, IllegalStateException;

    /**
     * Gets the duration of the file.
     *
     * @return the duration in milliseconds, if no duration is available (for
     *         example, if streaming live content), -1 is returned.
     */
    int getDuration();

    /**
     * Sets the player to be looping or non-looping.
     *
     * @param looping whether to loop or not
     */
    void setLooping(boolean looping);

    /**
     * Gets the current playback position.
     *
     * @return the current position in milliseconds
     */
    int getCurrentPosition();

    /**
     * Checks whether the MediaPlayer is looping or non-looping.
     *
     * @return true if the MediaPlayer is currently looping, false otherwise
     */
    boolean isLooping();

    /**
     * Checks whether the MediaPlayer is playing.
     *
     * @return true if currently playing, false otherwise
     * @throws IllegalStateException if the internal player engine has not been
     *             initialized or has been released.
     */
    boolean isPlaying() throws IllegalStateException;

    /**
     * Attaches an auxiliary effect to the player. A typical auxiliary effect is
     * a reverberation effect which can be applied on any sound source that
     * directs a certain amount of its energy to this effect. This amount is
     * defined by setAuxEffectSendLevel().
     * <p>
     * After creating an auxiliary effect (e.g.
     * {@link android.media.audiofx.EnvironmentalReverb}), retrieve its ID with
     * {@link android.media.audiofx.AudioEffect#getId()} and use it when calling
     * this method to attach the player to the effect.
     * <p>
     * To detach the effect from the player, call this method with a null effect
     * id.
     * <p>
     * This method must be called after one of the overloaded
     * <code> setDataSource </code> methods.
     *
     * @param effectId system wide unique id of the effect to attach
     */
    void attachAuxEffect(int effectId);

    /**
     * Sets the volume on this player. This API is recommended for balancing the
     * output of audio streams within an application. Unless you are writing an
     * application to control user settings, this API should be used in
     * preference to {@link AudioManager#setStreamVolume(int, int, int)} which
     * sets the volume of ALL streams of a particular type. Note that the passed
     * volume values are raw scalars in range 0.0 to 1.0. UI controls should be
     * scaled logarithmically.
     *
     * @param leftVolume left volume scalar
     * @param rightVolume right volume scalar
     */
    void setVolume(float leftVolume, float rightVolume);

    /**
     * Sets the audio stream type for this MediaPlayer. See {@link AudioManager}
     * for a list of stream types. Must call this method before prepare() or
     * prepareAsync() in order for the target stream type to become effective
     * thereafter.
     *
     * @param streamtype the audio stream type
     * @see android.media.AudioManager
     */
    void setAudioStreamType(int streamtype);

    /**
     * Sets the send level of the player to the attached auxiliary effect.
     * The level value range is 0 to 1.0.
     * <p>
     * By default the send level is 0, so even if an effect is attached to the
     * player this method must be called for the effect to be applied.
     * <p>
     * Note that the passed level value is a raw scalar. UI controls should be
     * scaled logarithmically: the gain applied by audio framework ranges from
     * -72dB to 0dB, so an appropriate conversion from linear UI input x to
     * level is: x == 0 -&gt; level = 0 0 &lt; x &lt;= R -&gt; level = 10^(72*(x-R)/20/R)
     *
     * @param level send level scalar
     */
    void setAuxEffectSendLevel(float level);

    /**
     * Set the low-level power management behavior for this MediaPlayer.
     * <p>
     * This function has the MediaPlayer access the low-level power manager
     * service to control the device's power usage while playing is occurring.
     * The parameter is a combination of {@link android.os.PowerManager} wake
     * flags. Use of this method requires
     * {@link android.Manifest.permission#WAKE_LOCK} permission. By default, no
     * attempt is made to keep the device awake during playback.
     *
     * @param context the Context to use
     * @param mode the power/wake mode to set
     * @see android.os.PowerManager
     */
    void setWakeMode(Context context, int mode);

    /**
     * Register a callback to be invoked when the status of a network stream's
     * buffer has changed.
     *
     * @param listener the callback that will be run.
     */
    void setOnBufferingUpdateListener(OnBufferingUpdateListener listener);

    /**
     * Register a callback to be invoked when the end of a media source has been
     * reached during playback.
     *
     * @param listener the callback that will be run
     */
    void setOnCompletionListener(OnCompletionListener listener);

    /**
     * Register a callback to be invoked when an error has happened during an
     * asynchronous operation.
     *
     * @param listener the callback that will be run
     */
    void setOnErrorListener(OnErrorListener listener);

    /**
     * Register a callback to be invoked when an info/warning is available.
     *
     * @param listener the callback that will be run
     */
    void setOnInfoListener(OnInfoListener listener);

    /**
     * Register a callback to be invoked when the media source is ready for
     * playback.
     *
     * @param listener the callback that will be run
     */
    void setOnPreparedListener(OnPreparedListener listener);

    /**
     * Register a callback to be invoked when a seek operation has been
     * completed.
     *
     * @param listener the callback that will be run
     */
    void setOnSeekCompleteListener(OnSeekCompleteListener listener);

    /**
     * Set the MediaPlayer to start when this MediaPlayer finishes playback
     * (i.e. reaches the end of the stream). The media framework will attempt to
     * transition from this player to the next as seamlessly as possible. The
     * next player can be set at any time before completion. The next player
     * must be prepared by the app, and the application should not call start()
     * on it. The next MediaPlayer must be different from 'this'. An exception
     * will be thrown if next == this. The application may call
     * setNextMediaPlayer(null) to indicate no next player should be
     * started at the end of playback. If the current player is looping, it will
     * keep looping and the next player will not be started.
     *
     * @param next the player to start after this one completes playback.
     */
    void setNextMediaPlayer(IBasicMediaPlayer next);

    /**
     * Sets the audio attributes for this MediaPlayer. See
     * {@link AudioAttributes} for how to build and configure an instance of
     * this class. You must call this method before {@link #prepare()} or
     * {@link #prepareAsync()} in order for the audio attributes to become
     * effective thereafter.
     *
     * @param attributes a non-null set of audio attributes
     */
    void setAudioAttributes(AudioAttributes attributes);
}
