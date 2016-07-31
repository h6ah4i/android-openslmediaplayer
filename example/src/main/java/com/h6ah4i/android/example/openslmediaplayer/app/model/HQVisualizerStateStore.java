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

import com.h6ah4i.android.media.audiofx.IHQVisualizer;

public class HQVisualizerStateStore extends BaseAudioEffectStateStore {
    private boolean mCaptureWaveform;
    private boolean mCaptureFft;
    private int mWindowType;

    // === Parcelable ===
    public static final Parcelable.Creator<HQVisualizerStateStore> CREATOR = new Parcelable.Creator<HQVisualizerStateStore>() {
        @Override
        public HQVisualizerStateStore createFromParcel(Parcel in) {
            return new HQVisualizerStateStore(in);
        }

        @Override
        public HQVisualizerStateStore[] newArray(int size) {
            return new HQVisualizerStateStore[size];
        }
    };

    private HQVisualizerStateStore(Parcel in) {
        super(in);

        mCaptureWaveform = readBoolean(in);
        mCaptureFft = readBoolean(in);
        mWindowType = in.readInt();
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest, flags);
        writeBoolean(dest, mCaptureWaveform);
        writeBoolean(dest, mCaptureFft);
        dest.writeInt(mWindowType);
    }

    // === Parcelable ===

    public HQVisualizerStateStore() {
        mCaptureWaveform = false;
        mCaptureFft = false;
        mWindowType = IHQVisualizer.WINDOW_RECTANGULAR;
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

    public int getWindowType() {
        return mWindowType;
    }

    /* package */void setWindowType(int windowType) {
        mWindowType = windowType;
    }
}
