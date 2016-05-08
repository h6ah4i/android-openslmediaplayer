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

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.audiofx.IVisualizer;

public class TestVisualizerWrapper
        extends TestObjectBaseWrapper
        implements IVisualizer {
    IVisualizer mVisualizer;

    private TestVisualizerWrapper(
            Host host,
            final IMediaPlayerFactory factory,
            final IBasicMediaPlayer player) {
        super(host);

        invoke(new Runnable() {
            @Override
            public void run() {
                mVisualizer = factory.createVisualizer(player);
            }
        });
    }

    public static TestVisualizerWrapper create(
            Host host, IMediaPlayerFactory factory, IBasicMediaPlayer player) {
        return new TestVisualizerWrapper(host, factory, player);
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
    public int getFft(final byte[] fft) throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getFft(fft);
            }
        });

        return result.value;
    }

    @Override
    public int getWaveForm(final byte[] waveform) throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getWaveForm(waveform);
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
    public int getScalingMode() throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getScalingMode();
            }
        });

        return result.value;
    }

    @Override
    public int setScalingMode(final int mode) throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.setScalingMode(mode);
            }
        });

        return result.value;
    }

    @Override
    public int getMeasurementMode() throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getMeasurementMode();
            }
        });

        return result.value;
    }

    @Override
    public int setMeasurementMode(final int mode) throws IllegalStateException {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.setMeasurementMode(mode);
            }
        });

        return result.value;
    }

    @Override
    public int getMeasurementPeakRms(final MeasurementPeakRms measurement) {
        final IntHolder result = new IntHolder();

        invoke(new Runnable() {
            @Override
            public void run() {
                result.value = mVisualizer.getMeasurementPeakRms(measurement);
            }
        });

        return result.value;
    }

    public IVisualizer getWrappedInstance() {
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
