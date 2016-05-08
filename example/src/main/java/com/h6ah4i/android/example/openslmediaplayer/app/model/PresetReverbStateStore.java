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

import com.h6ah4i.android.media.audiofx.IPresetReverb;

public class PresetReverbStateStore extends BaseAudioEffectStateStore {
    private IPresetReverb.Settings mSettings;

    // === Parcelable ===
    public static final Parcelable.Creator<PresetReverbStateStore> CREATOR = new Parcelable.Creator<PresetReverbStateStore>() {
        @Override
        public PresetReverbStateStore createFromParcel(Parcel in) {
            return new PresetReverbStateStore(in);
        }

        @Override
        public PresetReverbStateStore[] newArray(int size) {
            return new PresetReverbStateStore[size];
        }
    };

    private PresetReverbStateStore(Parcel in) {
        super(in);

        mSettings = new IPresetReverb.Settings(in.readString());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest, flags);
        dest.writeString(mSettings.toString());
    }

    // === Parcelable ===

    public PresetReverbStateStore() {
        mSettings = new IPresetReverb.Settings();
    }

    public IPresetReverb.Settings getSettings() {
        return mSettings;
    }

    /* package */void setSettings(IPresetReverb.Settings settings) {
        mSettings = settings.clone();
    }
}
