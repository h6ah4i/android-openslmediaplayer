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


package com.h6ah4i.android.example.openslmediaplayer.app.visualizer;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class AudioLevelMeterSurfaceView
        extends SurfaceView
        implements SurfaceHolder.Callback {
    @SuppressWarnings("unused")
    private static final String TAG = "AudioLevelMeterSurfaceView";

    private static final int MIN_LEVEL = -9600;

    private DoubleBufferingManager mDoubleBufferingManager = new DoubleBufferingManager();
    private Thread mRenderThread;

    public AudioLevelMeterSurfaceView(Context context) {
        super(context);
        init();
    }

    public AudioLevelMeterSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public AudioLevelMeterSurfaceView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    private void init() {
        getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(final SurfaceHolder holder) {
        final Object workObj = onCreateRenderThreadWorkingObj();

        mDoubleBufferingManager.reset();
        mRenderThread = new Thread(new Runnable() {
            @Override
            public void run() {
                AudioLevelMeterSurfaceView.this.renderThreadFunc(holder, workObj);
            }
        });
        mRenderThread.start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mDoubleBufferingManager.redraw();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mDoubleBufferingManager.stopWaiting();

        if (mRenderThread != null) {
            while (true) {
                try {
                    mRenderThread.join();
                    break;
                } catch (InterruptedException e) {
                }
            }
            mRenderThread = null;
        }
    }

    protected void renderThreadFunc(SurfaceHolder holder, Object workObj) {
        DoubleBufferingManager dbm = mDoubleBufferingManager;
        int level;

        level = dbm.getAndSwapBuffer();

        while (true) {
            // render
            Canvas canvas = null;

            try {
                canvas = holder.lockCanvas();

                // clear background
                canvas.drawColor(Color.BLACK);

                onRenderLevelMeter(workObj, canvas, level);
            } finally {
                if (canvas != null) {
                    holder.unlockCanvasAndPost(canvas);
                }
            }

            // wait for a new audio data
            try {
                level = dbm.waitForUpdate(-1);
            } catch (DoubleBufferingManager.StopRequestedException e) {
                // stop requested
                break;
            } catch (InterruptedException e) {
                // timeout
            }
        }
    }

    protected Object onCreateRenderThreadWorkingObj() {
        RenderThreadWork work = new RenderThreadWork();

        work.mBarPaint = new Paint();
        work.mBarPaint.setAntiAlias(false);
        work.mBarPaint.setColor(Color.BLUE);
        work.mBarPaint.setStyle(Paint.Style.FILL);

        return work;
    }

    protected void onRenderLevelMeter(Object workObj, Canvas canvas, int level) {
        final RenderThreadWork work = (RenderThreadWork) workObj;

        final int canvasWidth = canvas.getWidth();
        final int canvasHeight = canvas.getHeight();

        float fLevel = 1.0f - (Math.max(level, MIN_LEVEL) * (1.0f / MIN_LEVEL));

        // draw
        {
            // draw FFT bars
            canvas.save();
            canvas.scale(canvasWidth, canvasHeight);
            canvas.drawRect(0.0f, (1.0f - fLevel), 1.0f, 1.0f, work.mBarPaint);
            canvas.restore();
        }
    }

    public void updateAudioLevel(int level) {
        mDoubleBufferingManager.update(level);
    }

    private static final class RenderThreadWork {
        Paint mBarPaint;
    }

    protected static final class DoubleBufferingManager {

        private int[] mValues;
        private int mIndex;
        private boolean mUpdated;
        private boolean mStopRequested;

        public DoubleBufferingManager() {
            mValues = new int[2]; // double-buffering
            mValues[0] = MIN_LEVEL;
            mValues[1] = MIN_LEVEL;
        }

        public synchronized void reset() {
            mValues[0] = MIN_LEVEL;
            mValues[1] = MIN_LEVEL;
            mIndex = 0;
            mUpdated = false;
            mStopRequested = false;
        }

        public synchronized void stopWaiting() {
            mStopRequested = true;
            this.notify();
        }

        public synchronized void update(int level) {
            final int index = mIndex ^ 1;

            mValues[index] = level;
            mUpdated = true;

            this.notify();
        }

        public synchronized void redraw() {
            mUpdated = true;
            this.notify();
        }

        public synchronized int getAndSwapBuffer() {
            if (mUpdated) {
                // swap buffer
                mIndex ^= 1;
                mUpdated = false;
            }

            return mValues[mIndex];
        }

        private static final class StopRequestedException extends InterruptedException {
            private static final long serialVersionUID = -561722403846626829L;
        }

        public int waitForUpdate(int timeoutMillis) throws InterruptedException {

            synchronized (this) {
                try {
                    while (!mUpdated && !mStopRequested) {
                        if (timeoutMillis >= 0) {
                            this.wait(timeoutMillis);
                        } else {
                            // infinite
                            this.wait();
                        }
                    }
                } catch (InterruptedException e) {
                }

                if (mStopRequested) {
                    throw new StopRequestedException();
                }

                if (mUpdated) {
                    return getAndSwapBuffer();
                }

                // timeout
                throw new InterruptedException();
            }
        }
    }
}
