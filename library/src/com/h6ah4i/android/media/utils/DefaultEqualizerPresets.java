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


package com.h6ah4i.android.media.utils;

import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.audiofx.IEqualizer.Settings;

public class DefaultEqualizerPresets {

    public static final short NUM_PRESETS = 10;
    public static final short NUM_BANDS = 5;

    public static final String NAME_NORMAL = "Normal";
    public static final String NAME_CLASSICAL = "Classical";
    public static final String NAME_DANCE = "Dance";
    public static final String NAME_FLAT = "Flat";
    public static final String NAME_FOLK = "Folk";
    public static final String NAME_HEAVYMETAL = "Heavy Metal";
    public static final String NAME_HIPHOP = "Hip Hop";
    public static final String NAME_JAZZ = "Jazz";
    public static final String NAME_POP = "Pop";
    public static final String NAME_ROCK = "Rock";

    public static IEqualizer.Settings PRESET_NORMAL =
            createFiveBandPreset(0, 300, 0, 0, 0, 300);
    public static IEqualizer.Settings PRESET_CLASSICAL =
            createFiveBandPreset(1, 500, 300, -200, 400, 400);
    public static IEqualizer.Settings PRESET_DANCE =
            createFiveBandPreset(2, 600, 0, 200, 400, 100);
    public static IEqualizer.Settings PRESET_FLAT =
            createFiveBandPreset(3, 0, 0, 0, 0, 0);
    public static IEqualizer.Settings PRESET_FOLK =
            createFiveBandPreset(4, 300, 0, 0, 200, -100);
    public static IEqualizer.Settings PRESET_HEAVYMETAL =
            createFiveBandPreset(5, 400, 100, 900, 300, 0);
    public static IEqualizer.Settings PRESET_HIPHOP =
            createFiveBandPreset(6, 500, 300, 0, 100, 300);
    public static IEqualizer.Settings PRESET_JAZZ =
            createFiveBandPreset(7, 400, 200, -200, 200, 500);
    public static IEqualizer.Settings PRESET_POP =
            createFiveBandPreset(8, -100, 200, 500, 100, -200);
    public static IEqualizer.Settings PRESET_ROCK =
            createFiveBandPreset(9, 500, 300, -100, 300, 50);

    private static final String[] NAMES = new String[] {
            NAME_NORMAL,
            NAME_CLASSICAL,
            NAME_DANCE,
            NAME_FLAT,
            NAME_FOLK,
            NAME_HEAVYMETAL,
            NAME_HIPHOP,
            NAME_JAZZ,
            NAME_POP,
            NAME_ROCK,
    };

    private static IEqualizer.Settings[] PRESETS = new Settings[] {
            PRESET_NORMAL,
            PRESET_CLASSICAL,
            PRESET_DANCE,
            PRESET_FLAT,
            PRESET_FOLK,
            PRESET_HEAVYMETAL,
            PRESET_HIPHOP,
            PRESET_JAZZ,
            PRESET_POP,
            PRESET_ROCK,
    };

    public static IEqualizer.Settings getPreset(short preset) {
        return PRESETS[preset];
    }

    public static String getName(short preset) {
        return NAMES[preset];
    }

    private static final IEqualizer.Settings createFiveBandPreset(
            int presetNo, int band0, int band1, int band2, int band3, int band4) {
        IEqualizer.Settings settings = new Settings();

        settings.numBands = (short) 5;
        settings.curPreset = (short) presetNo;
        settings.bandLevels = new short[] {
                (short) band0, (short) band1,
                (short) band2, (short) band3,
                (short) band4
        };

        return settings;
    }
}
