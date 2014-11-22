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

import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;

public class EnvironmentalReverbPresets {
    private EnvironmentalReverbPresets() {
    }

    private static final short clipShort(int value, int min, int max) {
        if (value < min)
            return (short) min;
        if (value > max)
            return (short) max;
        return (short) value;
    }

    private static final int clipInt(int value, int min, int max) {
        if (value < min)
            return min;
        if (value > max)
            return max;
        return value;
    }

    private static IEnvironmentalReverb.Settings createPreset(
            int roomLevel,
            int roomHFLevel,
            int decayTime,
            int decayHFRatio,
            int reflectionsLevel,
            int reflectionsDelay,
            int reverbLevel,
            int reverbDelay,
            int diffusion,
            int density)
    {
        IEnvironmentalReverb.Settings settings = new IEnvironmentalReverb.Settings();

        // Clip parameters
        settings.roomLevel = clipShort(roomLevel, -9000, 0);
        settings.roomHFLevel = clipShort(roomHFLevel, -9000, 0);
        settings.decayTime = clipInt(decayTime, 100, 7000); /*
                                                             * spec.: max =
                                                             * 20000
                                                             */
        settings.decayHFRatio = clipShort(decayHFRatio, 100, 2000);
        settings.reflectionsLevel = clipShort(reflectionsLevel, 0, 0); /*
                                                                        * not
                                                                        * implemented
                                                                        * (
                                                                        * spec.:
                                                                        * min =
                                                                        * -9000,
                                                                        * max =
                                                                        * 1000)
                                                                        */
        settings.reflectionsDelay = clipInt(reflectionsDelay, 0, 0); /*
                                                                      * not
                                                                      * implemented
                                                                      * (spec.:
                                                                      * min = 0,
                                                                      * max =
                                                                      * 300)
                                                                      */
        settings.reverbLevel = clipShort(reverbLevel, -9000, 2000);
        settings.reverbDelay = clipInt(reverbDelay, 0, 0); /*
                                                            * not implemented
                                                            * (spec.: min = 0,
                                                            * max = 100)
                                                            */
        settings.diffusion = clipShort(diffusion, 0, 1000);
        settings.density = clipShort(density, 0, 1000);

        return settings;
    }

    public static final IEnvironmentalReverb.Settings DEFAULT =
            createPreset(-32768, 0, 1000, 500, -32768, 20, -32768, 40, 1000, 1000);
    public static final IEnvironmentalReverb.Settings GENERIC =
            createPreset(-1000, -100, 1490, 830, -2602, 7, 200, 11, 1000, 1000);
    public static final IEnvironmentalReverb.Settings PADDEDCELL =
            createPreset(-1000, -6000, 170, 100, -1204, 1, 207, 2, 1000, 1000);
    public static final IEnvironmentalReverb.Settings ROOM =
            createPreset(-1000, -454, 400, 830, -1646, 2, 53, 3, 1000, 1000);
    public static final IEnvironmentalReverb.Settings BATHROOM =
            createPreset(-1000, -1200, 1490, 540, -370, 7, 1030, 11, 1000, 600);
    public static final IEnvironmentalReverb.Settings LIVINGROOM =
            createPreset(-1000, -6000, 500, 100, -1376, 3, -1104, 4, 1000, 1000);
    public static final IEnvironmentalReverb.Settings STONEROOM =
            createPreset(-1000, -300, 2310, 640, -711, 12, 83, 17, 1000, 1000);
    public static final IEnvironmentalReverb.Settings AUDITORIUM =
            createPreset(-1000, -476, 4320, 590, -789, 20, -289, 30, 1000, 1000);
    public static final IEnvironmentalReverb.Settings CONCERTHALL =
            createPreset(-1000, -500, 3920, 700, -1230, 20, -2, 29, 1000, 1000);
    public static final IEnvironmentalReverb.Settings CAVE =
            createPreset(-1000, 0, 2910, 1300, -602, 15, -302, 22, 1000, 1000);
    public static final IEnvironmentalReverb.Settings ARENA =
            createPreset(-1000, -698, 7240, 330, -1166, 20, 16, 30, 1000, 1000);
    public static final IEnvironmentalReverb.Settings HANGAR =
            createPreset(-1000, -1000, 10050, 230, -602, 20, 198, 30, 1000, 1000);
    public static final IEnvironmentalReverb.Settings CARPETEDHALLWAY =
            createPreset(-1000, -4000, 300, 100, -1831, 2, -1630, 30, 1000, 1000);
    public static final IEnvironmentalReverb.Settings HALLWAY =
            createPreset(-1000, -300, 1490, 590, -1219, 7, 441, 11, 1000, 1000);
    public static final IEnvironmentalReverb.Settings STONECORRIDOR =
            createPreset(-1000, -237, 2700, 790, -1214, 13, 395, 20, 1000, 1000);
    public static final IEnvironmentalReverb.Settings ALLEY =
            createPreset(-1000, -270, 1490, 860, -1204, 7, -4, 11, 1000, 1000);
    public static final IEnvironmentalReverb.Settings FOREST =
            createPreset(-1000, -3300, 1490, 540, -2560, 162, -613, 88, 790, 1000);
    public static final IEnvironmentalReverb.Settings CITY =
            createPreset(-1000, -800, 1490, 670, -2273, 7, -2217, 11, 500, 1000);
    public static final IEnvironmentalReverb.Settings MOUNTAINS =
            createPreset(-1000, -2500, 1490, 210, -2780, 300, -2014, 100, 270, 1000);
    public static final IEnvironmentalReverb.Settings QUARRY =
            createPreset(-1000, -1000, 1490, 830, -32768, 61, 500, 25, 1000, 1000);
    public static final IEnvironmentalReverb.Settings PLAIN =
            createPreset(-1000, -2000, 1490, 500, -2466, 179, -2514, 100, 210, 1000);
    public static final IEnvironmentalReverb.Settings PARKINGLOT =
            createPreset(-1000, 0, 1650, 1500, -1363, 8, -1153, 12, 1000, 1000);
    public static final IEnvironmentalReverb.Settings SEWERPIPE =
            createPreset(-1000, -1000, 2810, 140, 429, 14, 648, 21, 800, 600);
    public static final IEnvironmentalReverb.Settings UNDERWATER =
            createPreset(-1000, -4000, 1490, 100, -449, 7, 1700, 11, 1000, 1000);
    public static final IEnvironmentalReverb.Settings SMALLROOM =
            createPreset(-1000, -600, 1100, 830, -400, 5, 500, 10, 1000, 1000);
    public static final IEnvironmentalReverb.Settings MEDIUMROOM =
            createPreset(-1000, -600, 1300, 830, -1000, 20, -200, 20, 1000, 1000);
    public static final IEnvironmentalReverb.Settings LARGEROOM =
            createPreset(-1000, -600, 1500, 830, -1600, 5, -1000, 40, 1000, 1000);
    public static final IEnvironmentalReverb.Settings MEDIUMHALL =
            createPreset(-1000, -600, 1800, 700, -1300, 15, -800, 30, 1000, 1000);
    public static final IEnvironmentalReverb.Settings LARGEHALL =
            createPreset(-1000, -600, 1800, 700, -2000, 30, -1400, 60, 1000, 1000);
    public static final IEnvironmentalReverb.Settings PLATE =
            createPreset(-1000, -200, 1300, 900, 0, 2, 0, 10, 1000, 750);
}
