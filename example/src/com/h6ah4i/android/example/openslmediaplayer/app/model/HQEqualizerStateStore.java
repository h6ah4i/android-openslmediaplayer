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

import com.h6ah4i.android.example.openslmediaplayer.app.utils.HQEqualizerUtil;
import com.h6ah4i.android.media.audiofx.IEqualizer;

public class HQEqualizerStateStore extends BaseAudioEffectStateStore {
    private IEqualizer.Settings mSettings;

    // === Parcelable ===
    public static final Parcelable.Creator<HQEqualizerStateStore> CREATOR = new Parcelable.Creator<HQEqualizerStateStore>() {
        @Override
        public HQEqualizerStateStore createFromParcel(Parcel in) {
            return new HQEqualizerStateStore(in);
        }

        @Override
        public HQEqualizerStateStore[] newArray(int size) {
            return new HQEqualizerStateStore[size];
        }
    };

    private HQEqualizerStateStore(Parcel in) {
        super(in);

        mSettings = new IEqualizer.Settings(in.readString());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest, flags);
        dest.writeString(mSettings.toString());
    }

    // === Parcelable ===

    public HQEqualizerStateStore() {
        if (HQEqualizerUtil.PRESETS != null) {
            mSettings = HQEqualizerUtil.PRESETS[0].settings.clone();
        } else {
            mSettings = new IEqualizer.Settings();
        }
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
                    HQEqualizerUtil.BAND_LEVEL_MIN,
                    HQEqualizerUtil.BAND_LEVEL_MAX);
}
