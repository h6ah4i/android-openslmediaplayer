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


package com.h6ah4i.android.media.openslmediaplayer.base;

import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.audiofx.IHQVisualizer;

public class TestHQVisualizerWrapper
        extends TestObjectBaseWrapper
        implements IHQVisualizer {
    IHQVisualizer mVisualizer;

    private TestHQVisualizerWrapper(
            Host host,
            final IMediaPlayerFactory factory) {
        super(host);

        invoke(new Runnable() {
            @Override
            public void run() {
                mVisualizer = factory.createHQVisualizer();
            }
        });
    }

    public static TestHQVisualizerWrapper create(
            Host host, IMediaPlayerFactory factory) {
        return new TestHQVisualizerWrapper(host, factory);
    }

    @Override
    public void release() {
        try {
            invoke(new Runnable() {
                @Override
                public void run() {
                    mVisualizer.release();
                }
            });
        } catch (Throwable th) {
            failUnexpectedThrowable(th);
        }
    }

    @Override
    public boolean getEnabled() {
        final BooleanHolder result = new BooleanHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getEnabled();
            }
        });

        return result.value;
    }

    @Override
    public int setEnabled(final boolean enabled) throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.setEnabled(enabled);
            }
        });

        return result.value;
    }

    @Override
    public int getSamplingRate() throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getSamplingRate();
            }
        });

        return result.value;
    }

    @Override
    public int getCaptureSize() throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getCaptureSize();
            }
        });

        return result.value;
    }

    @Override
    public int setCaptureSize(final int size) throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.setCaptureSize(size);
            }
        });

        return result.value;
    }

    @Override
    public int setDataCaptureListener(
            final OnDataCaptureListener listener,
            final int rate,
            final boolean waveform,
            final boolean fft) {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.setDataCaptureListener(
                        listener, rate, waveform, fft);
            }
        });

        return result.value;
    }

    @Override
    public int[] getCaptureSizeRange() throws IllegalStateException {
        final ObjHolder<int[]> result = new ObjHolder<int[]>();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getCaptureSizeRange();
            }
        });

        return result.value;
    }

    @Override
    public int getMaxCaptureRate() throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getMaxCaptureRate();
            }
        });

        return result.value;
    }

    @Override
    public int getNumChannels() throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getNumChannels();
            }
        });

        return result.value;
    }

    @Override
    public int getWindowFunction() throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getWindowFunction();
            }
        });

        return result.value;
    }

    @Override
    public int setWindowFunction(final int windowType) throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.setWindowFunction(windowType);
            }
        });

        return result.value;
    }

    public IHQVisualizer getWrappedInstance() {
        return mVisualizer;
    }

    private static class ObjHolder<T> {
        public T value;
    }

    private static final class IntHolder extends ObjHolder<Integer> {
    }

    private static final class BooleanHolder extends ObjHolder<Boolean> {
    }
}
