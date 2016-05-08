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


package com.h6ah4i.android.example.openslmediaplayer.app.model;

import android.os.Parcel;
import android.os.Parcelable;

import com.h6ah4i.android.media.audiofx.IVisualizer;

public class VisualizerStateStore extends BaseAudioEffectStateStore {
    private boolean mCaptureWaveform;
    private boolean mCaptureFft;
    private int mScalingMode;
    private boolean mMeasurePeak;
    private boolean mMeasureRms;

    // === Parcelable ===
    public static final Parcelable.Creator<VisualizerStateStore> CREATOR = new Parcelable.Creator<VisualizerStateStore>() {
        @Override
        public VisualizerStateStore createFromParcel(Parcel in) {
            return new VisualizerStateStore(in);
        }

        @Override
        public VisualizerStateStore[] newArray(int size) {
            return new VisualizerStateStore[size];
        }
    };

    private VisualizerStateStore(Parcel in) {
        super(in);

        mCaptureWaveform = readBoolean(in);
        mCaptureFft = readBoolean(in);
        mScalingMode = in.readInt();
        mMeasurePeak = readBoolean(in);
        mMeasureRms = readBoolean(in);
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest, flags);
        writeBoolean(dest, mCaptureWaveform);
        writeBoolean(dest, mCaptureFft);
        dest.writeInt(mScalingMode);
        writeBoolean(dest, mMeasurePeak);
        writeBoolean(dest, mMeasureRms);
    }

    // === Parcelable ===

    public VisualizerStateStore() {
        mCaptureWaveform = false;
        mCaptureFft = false;
        mScalingMode = IVisualizer.SCALING_MODE_NORMALIZED;
    }

    public boolean isCaptureWaveformEnabled() {
        return mCaptureWaveform;
    }

    /* package */void setCaptureWaveformEnabled(boolean enabled) {
        mCaptureWaveform = enabled;
    }

    public boolean isCaptureFftEnabled() {
        return mCaptureFft;
    }

    /* package */void setCaptureFftEnabled(boolean enabled) {
        mCaptureFft = enabled;
    }

    public int getScalingMode() {
        return mScalingMode;
    }

    /* package */void setScalingMode(int mode) {
        mScalingMode = mode;
    }

    public boolean isMeasurementPeakEnabled() {
        return mMeasurePeak;
    }

    /* package */void setMeasurementPeakEnabled(boolean enabled) {
        mMeasurePeak = enabled;
    }

    public boolean isMeasurementRmsEnabled() {
        return mMeasureRms;
    }

    /* package */void setMeasurementRmsEnabled(boolean enabled) {
        mMeasureRms = enabled;
    }

}
