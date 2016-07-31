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

import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.util.AttributeSet;

public class HQWaveformVisualizerSurfaceView extends BaseAudioVisualizerSurfaceView {
    public HQWaveformVisualizerSurfaceView(Context context) {
        super(context);
    }

    public HQWaveformVisualizerSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    private static final class RenderThreadWork {
        float[] mWorkFloatBuffer;
        FloatBuffer mNativeFloatBuffer;
        FloatColor mLchColor;
        FloatColor mRchColor;
        int prevCanvasWidth;
    }

    @Override
    protected Object onCreateRenderThreadWorkingObj() {
        RenderThreadWork work = new RenderThreadWork();
        work.mLchColor = new FloatColor(1.0f, 1.0f, 1.0f, 1.0f);
        work.mRchColor = new FloatColor(1.0f, 0.0f, 0.0f, 1.0f);
        return work;
    }

    @Override
    protected void onRenderAudioData(
            GL10 gl, int width, int height,
            Object workObj, CapturedDataHolder data) {
        final RenderThreadWork work = (RenderThreadWork) workObj;

        // NOTE:
        // Increase the SKIP value if the visualization is too heavy
        // on your device

        final float[] waveform = data.mFloatData;
        final int NUM_CHANNELS = 2;
        final int SKIP = Math.max(1, (waveform.length >> 12));
        final int N = waveform.length / (SKIP * NUM_CHANNELS);

        // 2: (x, y)
        final int workBufferSize = N * 2;
        boolean needToUpdateX = false;

        // prepare working buffer
        if (work.mWorkFloatBuffer == null || work.mWorkFloatBuffer.length < workBufferSize) {
            work.mWorkFloatBuffer = new float[workBufferSize];
            work.mNativeFloatBuffer = allocateNativeFloatBuffer(workBufferSize);
            needToUpdateX |= true;
        }

        final float[] points = work.mWorkFloatBuffer;
        final FloatBuffer pointsBuffer = work.mNativeFloatBuffer;

        // prepare points info buffer for drawing

        needToUpdateX |= (width != work.prevCanvasWidth);
        work.prevCanvasWidth = width;

        if (needToUpdateX) {
            makeXPointPositionData(N, points);
        }

        final float yrange = 1.05f;

        // Left channel
        makeYPointPositionData(waveform, N, SKIP, 0, points);
        converToFloatBuffer(pointsBuffer, points, (2 * N));
        drawWaveForm(gl, width, height, pointsBuffer, N, 0, yrange, work.mLchColor);

        // Right channel
        makeYPointPositionData(waveform, N, SKIP, waveform.length / NUM_CHANNELS,
                points);
        converToFloatBuffer(pointsBuffer, points, (2 * N));
        drawWaveForm(
                gl, width, height, pointsBuffer, N, 1, yrange, work.mRchColor);
    }

    private void makeXPointPositionData(int N, float[] points) {
        final float x_diff = 1.0f / (N - 1);
        for (int i = 0; i < N; i++) {
            points[2 * i + 0] = (x_diff * i);
        }
    }

    private static void makeYPointPositionData(
            float[] waveform,
            int n, int skip, int offset,
            float[] points) {
        for (int i = 0; i < n; i++) {
            points[2 * i + 1] = waveform[offset + i * skip];
        }
    }

    private static void drawWaveForm(
            GL10 gl, int width, int height,
            FloatBuffer vertices, int n, int vposition, float yrange, FloatColor color) {

        gl.glPushMatrix();

        // viewport
        gl.glViewport(0, (height / 2) * (1 - vposition), width, (height / 2));

        // X: [0:1], Y: [-1:+1] (scaled)
        gl.glOrthof(0.0f, 1.0f, -yrange, yrange, -1.0f, 1.0f);

        gl.glColor4f(color.red, color.green, color.blue, color.alpha);
        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glVertexPointer(2, GL10.GL_FLOAT, (2 * Float.SIZE / 8), vertices);
        gl.glDrawArrays(GL10.GL_LINE_STRIP, 0, n);
        gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);

        gl.glPopMatrix();
    }
}
