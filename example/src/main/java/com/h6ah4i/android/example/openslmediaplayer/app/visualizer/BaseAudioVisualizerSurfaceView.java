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

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

public abstract class BaseAudioVisualizerSurfaceView extends GLSurfaceView {
    @SuppressWarnings("unused")
    private static final String TAG = "AudioVisualizerSurfaceView";

    private DoubleBufferingManager mDoubleBufferingManager;
    private Object mRendererWorkObj;

    public BaseAudioVisualizerSurfaceView(Context context) {
        super(context);
        setup();
    }

    public BaseAudioVisualizerSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setup();
    }

    public void setup() {
        setRenderer(new VisualizerRenderer(this));
        setRenderMode(RENDERMODE_CONTINUOUSLY);
        mDoubleBufferingManager = new DoubleBufferingManager();
        mRendererWorkObj = onCreateRenderThreadWorkingObj();
    }

    protected Object onCreateRenderThreadWorkingObj() {
        return null;
    }

    public void onDrawFrame(GL10 gl, int width, int height) {
        CapturedDataHolder capData = null;

        capData = mDoubleBufferingManager.getAndSwapBuffer();

        if (capData != null && capData.valid()) {
            // clear background
            gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            gl.glClear(GL10.GL_COLOR_BUFFER_BIT);

            onRenderAudioData(gl, width, height, mRendererWorkObj, capData);
        }
    }

    protected void onRenderAudioData(GL10 gl, int width, int height, Object workObj,
            CapturedDataHolder data) {

    }

    public void updateAudioData(byte[] data, int samplingRate) {
        // for normal Visualizer
        mDoubleBufferingManager.update(data, 1, samplingRate);
    }

    public void updateAudioData(float[] data, int numChannels, int samplingRate) {
        // for HQVisualizer
        mDoubleBufferingManager.update(data, numChannels, samplingRate);
    }

    protected static FloatBuffer allocateNativeFloatBuffer(int size) {
        return ByteBuffer
                .allocateDirect(size * (Float.SIZE / 8))
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
    }

    protected static void converToFloatBuffer(FloatBuffer buffer, float[] array, int n) {
        buffer.clear();
        buffer.limit(n);
        buffer.put(array, 0, n);
        buffer.flip();
    }

    protected static final class CapturedDataHolder {
        public byte[] mByteData;
        public float[] mFloatData;
        public int mNumChannels;
        public int mSamplingRate;

        public void update(byte[] data, int numChannels, int samplingRate) {
            if (mByteData != null && mByteData.length == data.length) {
                System.arraycopy(data, 0, mByteData, 0, data.length);
            } else {
                mByteData = data.clone();
            }
            mFloatData = null;
            mNumChannels = numChannels;
            mSamplingRate = samplingRate;
        }

        public void update(float[] data, int numChannels, int samplingRate) {
            if (mFloatData != null && mFloatData.length == data.length) {
                System.arraycopy(data, 0, mFloatData, 0, data.length);
            } else {
                mFloatData = data.clone();
            }
            mByteData = null;
            mNumChannels = numChannels;
            mSamplingRate = samplingRate;
        }

        public boolean valid() {
            return (mByteData != null || mFloatData != null) && mNumChannels != 0
                    && mSamplingRate != 0;
        }
    }

    protected static final class DoubleBufferingManager {
        private CapturedDataHolder[] mHolders;
        private int mIndex;
        private boolean mUpdated;

        public DoubleBufferingManager() {
            mHolders = new CapturedDataHolder[2]; // double-buffering
            mHolders[0] = new CapturedDataHolder();
            mHolders[1] = new CapturedDataHolder();
        }

        public synchronized void reset() {
            mHolders[0] = new CapturedDataHolder();
            mHolders[1] = new CapturedDataHolder();
            mIndex = 0;
            mUpdated = false;
        }

        public synchronized void update(byte[] data, int numChannels, int samplingRate) {
            final int index = mIndex ^ 1;

            mHolders[index].update(data, numChannels, samplingRate);
            mUpdated = true;

            this.notify();
        }

        public synchronized void update(float[] data, int numChannels, int samplingRate) {
            final int index = mIndex ^ 1;

            mHolders[index].update(data, numChannels, samplingRate);
            mUpdated = true;

            this.notify();
        }

        public synchronized CapturedDataHolder getAndSwapBuffer() {
            if (mUpdated) {
                // swap buffer
                mIndex ^= 1;
                mUpdated = false;
            }

            return mHolders[mIndex];
        }
    }

    private static final class VisualizerRenderer implements GLSurfaceView.Renderer {
        private WeakReference<BaseAudioVisualizerSurfaceView> mRefHolderView;
        int mWidth;
        int mHeight;

        public VisualizerRenderer(BaseAudioVisualizerSurfaceView holderView) {
            mRefHolderView = new WeakReference<BaseAudioVisualizerSurfaceView>(holderView);
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            gl.glViewport(0, 0, width, height);

            // clear background
            gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            gl.glClear(GL10.GL_COLOR_BUFFER_BIT);

            mWidth = width;
            mHeight = height;
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            BaseAudioVisualizerSurfaceView holderView = mRefHolderView.get();

            if (holderView != null) {
                holderView.onDrawFrame(gl, mWidth, mHeight);
            }
        }
    }
}
