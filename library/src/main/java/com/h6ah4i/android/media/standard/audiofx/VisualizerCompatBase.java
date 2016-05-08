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

package com.h6ah4i.android.media.standard.audiofx;

import android.media.audiofx.Visualizer;

import com.h6ah4i.android.media.audiofx.IVisualizer;

abstract class VisualizerCompatBase {
    public int getScalingModeCompat(Visualizer visualizer) throws IllegalStateException {
        // NOTE:
        // Normalization is always applied on prior to Jelly Bean devices.
        return IVisualizer.SCALING_MODE_NORMALIZED;
    }

    public int setScalingModeCompat(Visualizer visualizer, int mode) throws IllegalStateException {
        return IVisualizer.SUCCESS;
    }

    public int getMeasurementModeCompat(Visualizer visualizer) throws IllegalStateException {
        return IVisualizer.MEASUREMENT_MODE_NONE;
    }

    public int setMeasurementModeCompat(Visualizer visualizer, int mode)
            throws IllegalStateException {
        return IVisualizer.SUCCESS;
    }

    public int getMeasurementPeakRmsCompat(Visualizer visualizer,
            IVisualizer.MeasurementPeakRms measurement) {
        if (measurement != null) {
            measurement.mPeak = -9600; // -96 dB
            measurement.mRms = -9600; // -96 dB
        }
        return IVisualizer.SUCCESS;
    }
}
