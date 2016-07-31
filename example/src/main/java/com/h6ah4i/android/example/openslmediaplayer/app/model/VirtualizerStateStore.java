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

import com.h6ah4i.android.media.audiofx.IVirtualizer;

public class VirtualizerStateStore extends BaseAudioEffectStateStore {
    private IVirtualizer.Settings mSettings;

    // === Parcelable ===
    public static final Parcelable.Creator<VirtualizerStateStore> CREATOR = new Parcelable.Creator<VirtualizerStateStore>() {
        @Override
        public VirtualizerStateStore createFromParcel(Parcel in) {
            return new VirtualizerStateStore(in);
        }

        @Override
        public VirtualizerStateStore[] newArray(int size) {
            return new VirtualizerStateStore[size];
        }
    };

    private VirtualizerStateStore(Parcel in) {
        super(in);

        mSettings = new IVirtualizer.Settings(in.readString());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest, flags);
        dest.writeString(mSettings.toString());
    }

    // === Parcelable ===

    public VirtualizerStateStore() {
        mSettings = new IVirtualizer.Settings();
    }

    public IVirtualizer.Settings getSettings() {
        return mSettings;
    }

    /* package */void setSettings(IVirtualizer.Settings settings) {
        mSettings = settings.clone();
    }

    public float getNormalizedStrength() {
        return sNormalizerStrength.normalize(mSettings.strength);
    }

    /* package */void setNormalizedStrength(float value) {
        mSettings.strength = sNormalizerStrength.denormalize(value);
    }

    private static final ShortParameterNormalizer sNormalizerStrength =
            new ShortParameterNormalizer((short) 0, (short) 1000);
}
