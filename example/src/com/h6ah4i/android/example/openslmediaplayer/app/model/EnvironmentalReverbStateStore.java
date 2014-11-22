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

import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.utils.EnvironmentalReverbPresets;

public class EnvironmentalReverbStateStore extends BaseAudioEffectStateStore {
    private int mPreset;
    private IEnvironmentalReverb.Settings mSettings;

    // === Parcelable ===
    public static final Parcelable.Creator<EnvironmentalReverbStateStore> CREATOR = new Parcelable.Creator<EnvironmentalReverbStateStore>() {
        @Override
        public EnvironmentalReverbStateStore createFromParcel(Parcel in) {
            return new EnvironmentalReverbStateStore(in);
        }

        @Override
        public EnvironmentalReverbStateStore[] newArray(int size) {
            return new EnvironmentalReverbStateStore[size];
        }
    };

    private EnvironmentalReverbStateStore(Parcel in) {
        super(in);

        mPreset = in.readInt();
        mSettings = new IEnvironmentalReverb.Settings(in.readString());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        super.writeToParcel(dest, flags);
        dest.writeInt(mPreset);
        dest.writeString(mSettings.toString());
    }

    // === Parcelable ===

    public EnvironmentalReverbStateStore() {
        mSettings = EnvironmentalReverbPresets.DEFAULT.clone();
    }

    public int getPreset() {
        return mPreset;
    }

    /* package */void setPreset(int preset) {
        mPreset = preset;
    }

    public IEnvironmentalReverb.Settings getSettings() {
        return mSettings;
    }

    /* package */void setSettings(IEnvironmentalReverb.Settings settings) {
        mSettings = settings.clone();
    }

    public float getNormalizedParameter(int index) {
        switch (index) {
            case PARAM_INDEX_DECAY_HF_RATIO:
                return sNormalizerDecayHFRatio.normalize(mSettings.decayHFRatio);
            case PARAM_INDEX_DECAY_TIME:
                return sNormalizerDecayTime.normalize(mSettings.decayTime);
            case PARAM_INDEX_DENSITY:
                return sNormalizerDensity.normalize(mSettings.density);
            case PARAM_INDEX_DIFFUSION:
                return sNormalizerDiffusion.normalize(mSettings.diffusion);
            case PARAM_INDEX_REFLECTIONS_DELAY:
                return sNormalizerReflectionsDelay.normalize(mSettings.reflectionsDelay);
            case PARAM_INDEX_REFLECTIONS_LEVEL:
                return sNormalizerReflectionsLevel.normalize(mSettings.reflectionsLevel);
            case PARAM_INDEX_REVERB_DELAY:
                return sNormalizerReverbDelay.normalize(mSettings.reverbDelay);
            case PARAM_INDEX_REVERB_LEVEL:
                return sNormalizerReverbLevel.normalize(mSettings.reverbLevel);
            case PARAM_INDEX_ROOM_HF_LEVEL:
                return sNormalizerRoomHFLevel.normalize(mSettings.roomHFLevel);
            case PARAM_INDEX_ROOM_LEVEL:
                return sNormalizerRoomLevel.normalize(mSettings.roomLevel);
            default:
                throw new IllegalArgumentException("index = " + index);
        }
    }

    /* package */void setNormalizedParameter(int index, float value) {
        switch (index) {
            case PARAM_INDEX_DECAY_HF_RATIO:
                mSettings.decayHFRatio = sNormalizerDecayHFRatio.denormalize(value);
                break;
            case PARAM_INDEX_DECAY_TIME:
                mSettings.decayTime = sNormalizerDecayTime.denormalize(value);
                break;
            case PARAM_INDEX_DENSITY:
                mSettings.density = sNormalizerDensity.denormalize(value);
                break;
            case PARAM_INDEX_DIFFUSION:
                mSettings.diffusion = sNormalizerDiffusion.denormalize(value);
                break;
            case PARAM_INDEX_REFLECTIONS_DELAY:
                mSettings.reflectionsDelay = sNormalizerReflectionsDelay.denormalize(value);
                break;
            case PARAM_INDEX_REFLECTIONS_LEVEL:
                mSettings.reflectionsLevel = sNormalizerReflectionsLevel.denormalize(value);
                break;
            case PARAM_INDEX_REVERB_DELAY:
                mSettings.reverbDelay = sNormalizerReverbDelay.denormalize(value);
                break;
            case PARAM_INDEX_REVERB_LEVEL:
                mSettings.reverbLevel = sNormalizerReverbLevel.denormalize(value);
                break;
            case PARAM_INDEX_ROOM_HF_LEVEL:
                mSettings.roomHFLevel = sNormalizerRoomHFLevel.denormalize(value);
                break;
            case PARAM_INDEX_ROOM_LEVEL:
                mSettings.roomLevel = sNormalizerRoomLevel.denormalize(value);
                break;
            default:
                throw new IllegalArgumentException("index = " + index);
        }
    }

    public static final int PARAM_INDEX_DECAY_HF_RATIO = 0;
    public static final int PARAM_INDEX_DECAY_TIME = 1;
    public static final int PARAM_INDEX_DENSITY = 2;
    public static final int PARAM_INDEX_DIFFUSION = 3;
    public static final int PARAM_INDEX_REFLECTIONS_DELAY = 4;
    public static final int PARAM_INDEX_REFLECTIONS_LEVEL = 5;
    public static final int PARAM_INDEX_REVERB_DELAY = 6;
    public static final int PARAM_INDEX_REVERB_LEVEL = 7;
    public static final int PARAM_INDEX_ROOM_HF_LEVEL = 8;
    public static final int PARAM_INDEX_ROOM_LEVEL = 9;

    private static final short ROOM_LEVEL_MIN = (short) -9000;
    private static final short ROOM_LEVEL_MAX = (short) 0;
    private static final short ROOM_HF_LEVEL_MIN = (short) -9000;
    private static final short ROOM_HF_LEVEL_MAX = (short) 0;
    private static final int DECAY_TIME_MIN = 100;
    /* Spec.: 20000, Actually(LVREV_MAX_T60): 7000 */
    private static final int DECAY_TIME_MAX = 7000;
    private static final short DECAY_HF_RATIO_MIN = (short) 100;
    private static final short DECAY_HF_RATIO_MAX = (short) 2000;
    /* Spec.: -9000, Actually: 0 (not implemented yet) */
    private static final short REFLECTIONS_LEVEL_MIN = (short) 0;
    /* Spec.: 1000, Actually: 0 (not implemented yet) */
    private static final short REFLECTIONS_LEVEL_MAX = (short) 0;
    /* Spec.: 0, Actually: 0 (not implemented yet) */
    private static final int REFLECTIONS_DELAY_MIN = 0;
    /* Spec.: 300, Actually: 0 (not implemented yet) */
    private static final int REFLECTIONS_DELAY_MAX = 0;
    private static final short REVERB_LEVEL_MIN = (short) -9000;
    private static final short REVERB_LEVEL_MAX = (short) 2000;
    /* Spec.: 0, Actually: 0 (not implemented yet) */
    private static final int REVERB_DELAY_MIN = 0;
    /* Spec.: 100, Actually: 0 (not implemented yet) */
    private static final int REVERB_DELAY_MAX = 0;
    private static final short DIFFUSION_MIN = (short) 0;
    private static final short DIFFUSION_MAX = (short) 1000;
    private static final short DENSITY_MIN = (short) 0;
    private static final short DENSITY_MAX = (short) 1000;

    private static final ShortParameterNormalizer sNormalizerDecayHFRatio =
            new ShortParameterNormalizer(DECAY_HF_RATIO_MIN, DECAY_HF_RATIO_MAX);
    private static final IntParameterNormalizer sNormalizerDecayTime =
            new IntParameterNormalizer(DECAY_TIME_MIN, DECAY_TIME_MAX);
    private static final ShortParameterNormalizer sNormalizerDensity =
            new ShortParameterNormalizer(DENSITY_MIN, DENSITY_MAX);
    private static final ShortParameterNormalizer sNormalizerDiffusion =
            new ShortParameterNormalizer(DIFFUSION_MIN, DIFFUSION_MAX);
    private static final IntParameterNormalizer sNormalizerReflectionsDelay =
            new IntParameterNormalizer(REFLECTIONS_DELAY_MIN, REFLECTIONS_DELAY_MAX);
    private static final ShortParameterNormalizer sNormalizerReflectionsLevel =
            new ShortParameterNormalizer(REFLECTIONS_LEVEL_MIN, REFLECTIONS_LEVEL_MAX);
    private static final IntParameterNormalizer sNormalizerReverbDelay =
            new IntParameterNormalizer(REVERB_DELAY_MIN, REVERB_DELAY_MAX);
    private static final ShortParameterNormalizer sNormalizerReverbLevel =
            new ShortParameterNormalizer(REVERB_LEVEL_MIN, REVERB_LEVEL_MAX);
    private static final ShortParameterNormalizer sNormalizerRoomHFLevel =
            new ShortParameterNormalizer(ROOM_HF_LEVEL_MIN, ROOM_HF_LEVEL_MAX);
    private static final ShortParameterNormalizer sNormalizerRoomLevel =
            new ShortParameterNormalizer(ROOM_LEVEL_MIN, ROOM_LEVEL_MAX);
}
