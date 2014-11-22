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

import com.h6ah4i.android.media.audiofx.IPreAmp;

public class PreAmpStateStore extends BaseAudioEffectStateStore {
    private IPreAmp.Settings mSettings;

    // === Parcelable ===
    public static final Parcelable.Creator<PreAmpStateStore> CREATOR = new Parcelable.Creator<PreAmpStateStore>() {
        @Override
        public PreAmpStateStore createFromParcel(Parcel in) {
            return new PreAmpStateStore(in);
        }

        @Override
        public PreAmpStateStore[] newArray(int size) {
            return new PreAmpStateStore[size];
        }
    };

    private PreAmpStateStore(Parcel in) {
        super(in);

        mSettings = new IPreAmp.Settings(in.readString());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest, flags);
        dest.writeString(mSettings.toString());
    }

    // === Parcelable ===
    public PreAmpStateStore() {
        mSettings = new IPreAmp.Settings();
        mSettings.level = 1.0f;
    }

    public IPreAmp.Settings getSettings() {
        return mSettings;
    }

    /* package */void setSettings(IPreAmp.Settings settings) {
        mSettings = settings.clone();
    }

    public float getLevel() {
        return mSettings.level;
    }

    private static final double UI_LEVEL_RANGE_DB = 40;

    public float getLevelFromUI() {
        final float value = mSettings.level;
        float ui_level;

        if (value <= 0.0f) {
            ui_level = 0.0f;
        } else {
            ui_level = (float) (Math.log10(value) * 20 / UI_LEVEL_RANGE_DB + 1.0);
        }

        return ui_level;
    }

    /* package */void setLevelFromUI(float value) {
        float log_level;

        if (value <= 0.0f) {
            log_level = 0.0f;
        } else {
            log_level = (float) Math.pow(10, (UI_LEVEL_RANGE_DB * (value - 1.0) / 20));
        }

        mSettings.level = log_level;
    }
}
