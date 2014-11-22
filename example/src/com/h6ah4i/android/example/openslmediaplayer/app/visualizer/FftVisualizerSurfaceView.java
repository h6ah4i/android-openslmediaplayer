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

public class FftVisualizerSurfaceView extends BaseAudioVisualizerSurfaceView {
    public FftVisualizerSurfaceView(Context context) {
        super(context);
    }

    public FftVisualizerSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    private final class RenderThreadWork {
        float[] mWorkFloatBuffer;
        FloatColor mColor;
        int prevCanvasWidth;
        public FloatBuffer mNativeFloatBuffer;
    }

    // http://forum.processing.org/topic/super-fast-square-root
    private static final float fastSqrt(float x) {
        return Float.intBitsToFloat(532483686 + (Float.floatToRawIntBits(x) >> 1));
    }

    @Override
    protected Object onCreateRenderThreadWorkingObj() {
        RenderThreadWork work = new RenderThreadWork();

        work.mColor = new FloatColor(0.0f, 1.0f, 0.0f, 1.0f);

        return work;
    }

    @Override
    protected void onRenderAudioData(
            GL10 gl, int width, int height,
            Object workObj, CapturedDataHolder data) {
        final RenderThreadWork work = (RenderThreadWork) workObj;

        final byte[] fft = data.mByteData;

        // -2, +2: (DC + Fs/2), /2: (Re + Im)
        final int N = (fft.length - 2) / 2 + 2;

        // 2: (x, y)
        final int workBufferSize = N * 2;
        boolean needToUpdateX = false;

        // prepare working buffer
        if (work.mWorkFloatBuffer == null ||
                work.mWorkFloatBuffer.length < workBufferSize) {
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

        final float yrange = 128.0f + 1.0f;

        makeYPointPositionData(fft, N, 0, points);
        converToFloatBuffer(pointsBuffer, points, (2 * N));
        drawFFT(gl, width, height, pointsBuffer, N, yrange, work.mColor);
    }

    private void makeXPointPositionData(int N, float[] points) {
        final float x_diff = 1.0f / (N - 1);
        for (int i = 0; i < N; i++) {
            points[2 * i + 0] = (x_diff * i);
        }
    }

    private static void makeYPointPositionData(
            byte[] fft,
            int n, int offset,
            float[] points) {
        // NOTE:
        // mag = sqrt( (re / 128)^2 + (im / 128)^2 )
        // = sqrt( (1 / 128)^2 * (re^2 + im^2) )
        // = (1 / 128) * sqrt(re^2 + im^2)
        //
        // data range = [0:128] (NOTE: L + R mixed gain)

        // DC
        points[2 * 0 + 1] = Math.abs(fft[offset + 0]);

        // f_1 .. f_(N-1)
        for (int i = 1; i < (n - 1); i++) {
            final int re = fft[offset + 2 * i + 0];
            final int im = fft[offset + 2 * i + 1];
            final float y = fastSqrt((re * re) + (im * im));

            points[2 * i + 1] = y;
        }

        // fs / 2
        points[2 * (n - 1) + 1] = Math.abs(fft[offset + 1]);
    }

    private static void drawFFT(
            GL10 gl, int width, int height,
            FloatBuffer vertices, int n,
            float yrange, FloatColor color) {

        gl.glPushMatrix();

        // viewport
        gl.glViewport(0, 0, width, height);

        // X: [0:1], Y: [0:yrange]
        gl.glOrthof(0.0f, 1.0f, 0.0f, yrange, -1.0f, 1.0f);

        gl.glColor4f(color.red, color.green, color.blue, color.alpha);
        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glVertexPointer(2, GL10.GL_FLOAT, (2 * Float.SIZE / 8), vertices);
        gl.glDrawArrays(GL10.GL_LINE_STRIP, 0, n);
        gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);

        gl.glPopMatrix();
    }
}
