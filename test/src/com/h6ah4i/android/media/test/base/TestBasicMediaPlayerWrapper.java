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

import java.io.FileDescriptor;
import java.io.IOException;

import android.content.Context;
import android.net.Uri;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.compat.AudioAttributes;
import com.h6ah4i.android.media.test.utils.ThrowableRunnable;

public final class TestBasicMediaPlayerWrapper
        extends TestObjectBaseWrapper
        implements IBasicMediaPlayer {
    private IBasicMediaPlayer mPlayer;

    private TestBasicMediaPlayerWrapper(
            Host host, final IMediaPlayerFactory factory) {
        super(host);
        invoke(new Runnable() {
            @Override
            public void run() {
                mPlayer = factory.createMediaPlayer();
            }
        });
    }

    public static TestBasicMediaPlayerWrapper create(
            Host host, IMediaPlayerFactory factory) {
        return new TestBasicMediaPlayerWrapper(host, factory);
    }

    public IBasicMediaPlayer getWrappedInstance() {
        return mPlayer;
    }

    @Override
    public void start() throws IllegalStateException {
        invoke(new Runnable() {
            @Override
            public void run() {
                mPlayer.start();
            }
        });
    }

    @Override
    public void stop() throws IllegalStateException {
        invoke(new Runnable() {
            @Override
            public void run() {
                mPlayer.stop();
            }
        });
    }

    @Override
    public void pause() throws IllegalStateException {
        invoke(new Runnable() {
            @Override
            public void run() {
                mPlayer.pause();
            }
        });
    }

    @Override
    public void setDataSource(final Context context, final Uri uri) throws IOException,
            IllegalArgumentException, SecurityException, IllegalStateException {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() throws Throwable {
                    mPlayer.setDataSource(context, uri);
                }
            });
        } catch (IOException e) {
            throw e;
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (SecurityException e) {
            throw e;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setDataSource(final FileDescriptor fd) throws IOException,
            IllegalArgumentException,
            IllegalStateException {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() throws Throwable {
                    mPlayer.setDataSource(fd);
                }
            });
        } catch (IOException e) {
            throw e;
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setDataSource(final FileDescriptor fd, final long offset, final long length)
            throws IOException,
            IllegalArgumentException, IllegalStateException {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() throws Throwable {
                    mPlayer.setDataSource(fd, offset, length);
                }
            });
        } catch (IOException e) {
            throw e;
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setDataSource(final String path) throws IOException, IllegalArgumentException,
            IllegalStateException {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() throws Throwable {
                    mPlayer.setDataSource(path);
                }
            });
        } catch (IOException e) {
            throw e;
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void prepare() throws IOException, IllegalStateException {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() throws Throwable {
                    mPlayer.prepare();
                }
            });
        } catch (IOException e) {
            throw e;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.prepareAsync();
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void seekTo(final int msec) throws IllegalStateException {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.seekTo(msec);
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void release() {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.release();
                }
            });
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void reset() {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.reset();
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public int getAudioSessionId() {
        final int[] audioSessionId = new int[1];

        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    audioSessionId[0] = mPlayer.getAudioSessionId();
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }

        return audioSessionId[0];
    }

    @Override
    public void setAudioSessionId(final int sessionId) throws IllegalArgumentException,
            IllegalStateException {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setAudioSessionId(sessionId);
                }
            });
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public int getDuration() {
        final int[] duration = new int[1];

        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    duration[0] = mPlayer.getDuration();
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }

        return duration[0];
    }

    @Override
    public void setLooping(final boolean looping) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setLooping(looping);
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public int getCurrentPosition() {
        final int[] position = new int[1];

        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    position[0] = mPlayer.getCurrentPosition();
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }

        return position[0];
    }

    @Override
    public boolean isLooping() {
        final boolean[] looping = new boolean[1];

        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    looping[0] = mPlayer.isLooping();
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }

        return looping[0];
    }

    @Override
    public boolean isPlaying() throws IllegalStateException {
        final boolean[] playing = new boolean[1];

        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    playing[0] = mPlayer.isPlaying();
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }

        return playing[0];
    }

    @Override
    public void attachAuxEffect(final int effectId) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.attachAuxEffect(effectId);
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setVolume(final float leftVolume, final float rightVolume) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setVolume(leftVolume, rightVolume);
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setAudioStreamType(final int streamtype) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setAudioStreamType(streamtype);
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setAuxEffectSendLevel(final float level) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setAuxEffectSendLevel(level);
                }
            });
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setWakeMode(final Context context, final int mode) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setWakeMode(context, mode);
                }
            });
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (NullPointerException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setOnBufferingUpdateListener(final OnBufferingUpdateListener listener) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setOnBufferingUpdateListener(listener);
                }
            });
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setOnCompletionListener(final OnCompletionListener listener) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setOnCompletionListener(listener);
                }
            });
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setOnErrorListener(final OnErrorListener listener) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setOnErrorListener(listener);
                }
            });
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setOnInfoListener(final OnInfoListener listener) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setOnInfoListener(listener);
                }
            });
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setOnPreparedListener(final OnPreparedListener listener) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setOnPreparedListener(listener);
                }
            });
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setOnSeekCompleteListener(final OnSeekCompleteListener listener) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setOnSeekCompleteListener(listener);
                }
            });
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setNextMediaPlayerCompat(final IBasicMediaPlayer next) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setNextMediaPlayerCompat(next);
                }
            });
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public void setAudioAttributes(final AudioAttributes attributes) {
        try {
            invoke(new ThrowableRunnable() {
                @Override
                public void run() {
                    mPlayer.setAudioAttributes(attributes);
                }
            });
        } catch (IllegalArgumentException e) {
            throw e;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }
}
