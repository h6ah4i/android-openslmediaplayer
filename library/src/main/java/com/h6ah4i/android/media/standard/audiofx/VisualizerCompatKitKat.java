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

import android.annotation.TargetApi;
import android.media.audiofx.Visualizer;
import android.os.Build;

import com.h6ah4i.android.media.audiofx.IVisualizer;

@TargetApi(Build.VERSION_CODES.KITKAT)
class VisualizerCompatKitKat extends VisualizerCompatBase {

    @Override
    public int getScalingModeCompat(Visualizer visualizer) throws IllegalStateException {
        return visualizer.getScalingMode();
    }

    @Override
    public int setScalingModeCompat(Visualizer visualizer, int mode) throws IllegalStateException {
        return visualizer.setScalingMode(mode);
    }

    @Override
    public int getMeasurementModeCompat(Visualizer visualizer) throws IllegalStateException {
        return visualizer.getMeasurementMode();
    }

    @Override
    public int setMeasurementModeCompat(Visualizer visualizer, int mode)
            throws IllegalStateException {
        return visualizer.setMeasurementMode(mode);
    }

    @Override
    public int getMeasurementPeakRmsCompat(Visualizer visualizer,
            IVisualizer.MeasurementPeakRms measurement) {
        Visualizer.MeasurementPeakRms tmp = new Visualizer.MeasurementPeakRms();
        int result = visualizer.getMeasurementPeakRms(tmp);

        measurement.mPeak = tmp.mPeak;
        measurement.mRms = tmp.mRms;

        return result;
    }
}
