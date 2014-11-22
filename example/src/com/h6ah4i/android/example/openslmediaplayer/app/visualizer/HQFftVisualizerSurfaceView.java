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

public class HQFftVisualizerSurfaceView extends BaseAudioVisualizerSurfaceView {
    public HQFftVisualizerSurfaceView(Context context) {
        super(context);
    }

    public HQFftVisualizerSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    private final class RenderThreadWork {
        float[] mWorkFloatBuffer;
        FloatColor mLchColor;
        FloatColor mRchColor;
        int prevCanvasWidth;
        public FloatBuffer mNativeFloatBuffer;
    }

    @Override
    protected Object onCreateRenderThreadWorkingObj() {
        RenderThreadWork work = new RenderThreadWork();

        work.mLchColor = new FloatColor(0.0f, 1.0f, 0.0f, 1.0f);
        work.mRchColor = new FloatColor(0.0f, 1.0f, 1.0f, 1.0f);

        return work;
    }

    @Override
    protected void onRenderAudioData(
            GL10 gl, int width, int height,
            Object workObj, CapturedDataHolder data) {
        final RenderThreadWork work = (RenderThreadWork) workObj;

        final int canvasWidth = width;
        final int canvasHeight = height;
        final float[] fft = data.mFloatData;

        // -2, +2: (DC + Fs/2), /2: (Re + Im)
        final int N = ((fft.length / 2) - 2) / 2 + 2;

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

        final int data_range = (N - 1) / 2;
        final float yrange = data_range * 1.05f;

        // Lch
        makeYPointPositionData(fft, N, 0, points);
        converToFloatBuffer(pointsBuffer, points, (2 * N));
        drawFFT(gl, canvasWidth, canvasHeight, pointsBuffer, N, 0, yrange, work.mLchColor);

        // Rch
        makeYPointPositionData(fft, N, (N - 1) * 2, points);
        converToFloatBuffer(pointsBuffer, points, (2 * N));
        drawFFT(gl, canvasWidth, canvasHeight, pointsBuffer, N, 1, yrange, work.mRchColor);
    }

    // http://forum.processing.org/topic/super-fast-square-root
    private static final float fastSqrt(float x) {
        return Float.intBitsToFloat(532483686 + (Float.floatToRawIntBits(x) >>
                1));
    }

    private void makeXPointPositionData(int N, float[] points) {
        final float x_diff = 1.0f / (N - 1);
        for (int i = 0; i < N; i++) {
            points[2 * i + 0] = (x_diff * i);
        }
    }

    private static void makeYPointPositionData(
            float[] fft,
            int n, int offset,
            float[] points) {
        // DC
        points[2 * 0 + 1] = Math.abs(fft[offset + 0]);

        // f_1 .. f_(N-1)
        for (int i = 1; i < (n - 1); i++) {
            final float re = fft[offset + 2 * i + 0];
            final float im = fft[offset + 2 * i + 1];
            final float y = fastSqrt((re * re) + (im * im));

            points[2 * i + 1] = y;
        }

        // fs / 2
        points[2 * (n - 1) + 1] = Math.abs(fft[offset + 1]);
    }

    private static void drawFFT(
            GL10 gl, int width, int height,
            FloatBuffer vertices, int n,
            int vposition, float yrange, FloatColor color) {

        gl.glPushMatrix();

        // viewport
        gl.glViewport(0, (height / 2) * (1 - vposition), width, (height / 2));

        // X: [0:1], Y: [0:1]
        gl.glOrthof(0.0f, 1.0f, 0.0f, yrange, -1.0f, 1.0f);

        gl.glColor4f(color.red, color.green, color.blue, color.alpha);
        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glVertexPointer(2, GL10.GL_FLOAT, (2 * Float.SIZE / 8), vertices);
        gl.glDrawArrays(GL10.GL_LINE_STRIP, 0, n);
        gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);

        gl.glPopMatrix();
    }
}
