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

import com.h6ah4i.android.example.openslmediaplayer.app.utils.EqualizerUtil;
import com.h6ah4i.android.media.audiofx.IEqualizer;

public class EqualizerStateStore extends BaseAudioEffectStateStore {
    private IEqualizer.Settings mSettings;

    // === Parcelable ===
    public static final Parcelable.Creator<EqualizerStateStore> CREATOR = new Parcelable.Creator<EqualizerStateStore>() {
        @Override
        public EqualizerStateStore createFromParcel(Parcel in) {
            return new EqualizerStateStore(in);
        }

        @Override
        public EqualizerStateStore[] newArray(int size) {
            return new EqualizerStateStore[size];
        }
    };

    private EqualizerStateStore(Parcel in) {
        super(in);

        mSettings = new IEqualizer.Settings(in.readString());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest, flags);
        dest.writeString(mSettings.toString());
    }

    // === Parcelable ===

    public EqualizerStateStore() {
        mSettings = (EqualizerUtil.PRESETS[0].settings).clone();
    }

    public IEqualizer.Settings getSettings() {
        return mSettings;
    }

    /* package */void setSettings(IEqualizer.Settings settings) {
        mSettings = settings.clone();
    }

    public float getNormalizedBandLevel(int index) {
        return sNormalizerBandLevel.normalize(mSettings.bandLevels[index]);
    }

    /* package */void setNormalizedBandLevel(int index, float value, boolean clearCurrentPreset) {
        mSettings.bandLevels[index] = sNormalizerBandLevel.denormalize(value);
        if (clearCurrentPreset) {
            mSettings.curPreset = IEqualizer.PRESET_UNDEFINED;
        }
    }

    private static final ShortParameterNormalizer sNormalizerBandLevel =
            new ShortParameterNormalizer(
                    EqualizerUtil.BAND_LEVEL_MIN,
                    EqualizerUtil.BAND_LEVEL_MAX);
}
