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


package com.h6ah4i.android.example.openslmediaplayer.app.utils;

import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb.Settings;
import com.h6ah4i.android.media.utils.EnvironmentalReverbPresets;

public class EnvironmentalReverbPresetsUtil {
    private static final IEnvironmentalReverb.Settings[] mEnvReverbPresetTable = {
            EnvironmentalReverbPresets.DEFAULT,
            EnvironmentalReverbPresets.GENERIC,
            EnvironmentalReverbPresets.PADDEDCELL,
            EnvironmentalReverbPresets.ROOM,
            EnvironmentalReverbPresets.BATHROOM,
            EnvironmentalReverbPresets.LIVINGROOM,
            EnvironmentalReverbPresets.STONEROOM,
            EnvironmentalReverbPresets.AUDITORIUM,
            EnvironmentalReverbPresets.CONCERTHALL,
            EnvironmentalReverbPresets.CAVE,
            EnvironmentalReverbPresets.ARENA,
            EnvironmentalReverbPresets.HANGAR,
            EnvironmentalReverbPresets.CARPETEDHALLWAY,
            EnvironmentalReverbPresets.HALLWAY,
            EnvironmentalReverbPresets.STONECORRIDOR,
            EnvironmentalReverbPresets.ALLEY,
            EnvironmentalReverbPresets.FOREST,
            EnvironmentalReverbPresets.CITY,
            EnvironmentalReverbPresets.MOUNTAINS,
            EnvironmentalReverbPresets.QUARRY,
            EnvironmentalReverbPresets.PLAIN,
            EnvironmentalReverbPresets.PARKINGLOT,
            EnvironmentalReverbPresets.SEWERPIPE,
            EnvironmentalReverbPresets.UNDERWATER,
            EnvironmentalReverbPresets.SMALLROOM,
            EnvironmentalReverbPresets.MEDIUMROOM,
            EnvironmentalReverbPresets.LARGEROOM,
            EnvironmentalReverbPresets.MEDIUMHALL,
            EnvironmentalReverbPresets.LARGEHALL,
            EnvironmentalReverbPresets.PLATE,
    };

    public static Settings getPreset(int presetNo) {
        return mEnvReverbPresetTable[presetNo];
    }

}
