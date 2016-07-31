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

import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerContext;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLHQEqualizer;

public class HQEqualizerUtil {
    public final static int NUMBER_OF_BANDS;
    public final static int NUMBER_OF_PRESETS;
    public final static PresetInfo[] PRESETS;
    public final static int[] CENTER_FREQUENCY;
    public final static short BAND_LEVEL_MIN;
    public final static short BAND_LEVEL_MAX;

    public final static class PresetInfo {
        public short index;
        public String name;
        public IEqualizer.Settings settings;
    }

    private HQEqualizerUtil() {
    }

    static {
        short numberOfBands = 0;
        short numberOfPresets = 0;
        PresetInfo[] presets = null;
        int[] centerFreqency = new int[0];
        short[] bandLevelRange = new short[2];

        OpenSLMediaPlayerContext oslmp_context = null;
        IEqualizer eq = null;
        try {
            OpenSLMediaPlayerContext.Parameters params = new OpenSLMediaPlayerContext.Parameters();
            params.options = OpenSLMediaPlayerContext.OPTION_USE_HQ_EQUALIZER;

            oslmp_context = new OpenSLMediaPlayerContext(null, params);
            eq = new OpenSLHQEqualizer(oslmp_context);

            numberOfBands = eq.getNumberOfBands();
            numberOfPresets = eq.getNumberOfPresets();

            presets = new PresetInfo[numberOfPresets];
            for (short i = 0; i < numberOfPresets; i++) {
                PresetInfo preset = new PresetInfo();

                eq.usePreset(i);

                preset.index = i;
                preset.name = eq.getPresetName(preset.index);
                preset.settings = eq.getProperties();

                presets[i] = preset;
            }

            centerFreqency = new int[numberOfBands];
            for (short i = 0; i < numberOfBands; i++) {
                centerFreqency[i] = eq.getCenterFreq(i);
            }

            bandLevelRange = eq.getBandLevelRange();
        } catch (UnsupportedOperationException e) {
            // just ignore (maybe API level is less than 14)
        } finally {
            try {
                if (eq != null) {
                    eq.release();
                }
            } catch (Exception e) {
            }
            try {
                if (oslmp_context != null) {
                    oslmp_context.release();
                }
            } catch (Exception e) {
            }
            eq = null;
            oslmp_context = null;
        }

        NUMBER_OF_BANDS = numberOfBands;
        NUMBER_OF_PRESETS = numberOfPresets;
        PRESETS = presets;
        CENTER_FREQUENCY = centerFreqency;
        BAND_LEVEL_MIN = bandLevelRange[0];
        BAND_LEVEL_MAX = bandLevelRange[1];
    }
}
