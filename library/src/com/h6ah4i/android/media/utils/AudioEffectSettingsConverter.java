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

import android.media.audiofx.BassBoost;
import android.media.audiofx.EnvironmentalReverb;
import android.media.audiofx.Equalizer;
import android.media.audiofx.PresetReverb;
import android.media.audiofx.Virtualizer;

import com.h6ah4i.android.media.audiofx.IBassBoost;
import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.audiofx.IPresetReverb;
import com.h6ah4i.android.media.audiofx.IVirtualizer;

public class AudioEffectSettingsConverter {
    private AudioEffectSettingsConverter() {
    }

    /**
     * IEqualizer.Settings -&gt; Equalizer.Settings
     *
     * @param settings IEqualizer.Settings
     * @return Equalizer.Settings
     */
    public static Equalizer.Settings convert(IEqualizer.Settings settings) {
        Equalizer.Settings settings2 = new Equalizer.Settings();

        settings2.curPreset = settings.curPreset;
        settings2.numBands = settings.numBands;
        settings2.bandLevels = settings.bandLevels;

        return settings2;
    }

    /**
     * Equalizer.Settings -&gt; IEqualizer.Settings
     *
     * @param settings Equalizer.Settings
     * @return IEqualizer.Settings
     */
    public static IEqualizer.Settings convert(Equalizer.Settings settings) {
        IEqualizer.Settings settings2 = new IEqualizer.Settings();

        settings2.curPreset = settings.curPreset;
        settings2.numBands = settings.numBands;
        settings2.bandLevels = settings.bandLevels;

        return settings2;
    }

    /**
     * IVirtualizer.Settings -&gt; Virtualizer.Settings
     *
     * @param settings IVirtualizer.Settings
     * @return Virtualizer.Settings
     */
    public static Virtualizer.Settings convert(IVirtualizer.Settings settings) {
        Virtualizer.Settings settings2 = new Virtualizer.Settings();

        settings2.strength = settings.strength;

        return settings2;
    }

    /**
     * Virtualizer.Settings -&gt; IVirtualizer.Settings
     *
     * @param settings IVirtualizer.Settings
     * @return IVirtualizer.Settings
     */
    public static IVirtualizer.Settings convert(Virtualizer.Settings settings) {
        IVirtualizer.Settings settings2 = new IVirtualizer.Settings();

        settings2.strength = settings.strength;

        return settings2;
    }

    /**
     * IBassBoost.Settings -&gt; BassBoost.Settings
     *
     * @param settings IBassBoost.Settings
     * @return BassBoost.Settings
     */
    public static BassBoost.Settings convert(IBassBoost.Settings settings) {
        BassBoost.Settings settings2 = new BassBoost.Settings();

        settings2.strength = settings.strength;

        return settings2;
    }

    /**
     * BassBoost.Settings -&gt; IBassBoost.Settings
     *
     * @param settings BassBoost.Settings
     * @return IBassBoost.Settings
     */
    public static IBassBoost.Settings convert(BassBoost.Settings settings) {
        IBassBoost.Settings settings2 = new IBassBoost.Settings();

        settings2.strength = settings.strength;

        return settings2;
    }

    /**
     * IPresetReverb.Settings -&gt; PresetReverb.Settings
     *
     * @param settings IPresetReverb.Settings
     * @return PresetReverb.Settings
     */
    public static PresetReverb.Settings convert(IPresetReverb.Settings settings) {
        PresetReverb.Settings settings2 = new PresetReverb.Settings();

        settings2.preset = settings.preset;

        return settings2;
    }

    /**
     * PresetReverb.Settings -&gt; IPresetReverb.Settings
     *
     * @param settings PresetReverb.Settings
     * @return IPresetReverb.Settings
     */
    public static IPresetReverb.Settings convert(PresetReverb.Settings settings) {
        IPresetReverb.Settings settings2 = new IPresetReverb.Settings();

        settings2.preset = settings.preset;

        return settings2;
    }

    /**
     * IEnvironmentalReverb.Settings -&gt; EnvironmentalReverb.Settings
     *
     * @param settings IEnvironmentalReverb.Settings
     * @return EnvironmentalReverb.Settings
     */
    public static EnvironmentalReverb.Settings convert(IEnvironmentalReverb.Settings settings) {
        EnvironmentalReverb.Settings settings2 = new EnvironmentalReverb.Settings();

        settings2.roomLevel = settings.roomLevel;
        settings2.roomHFLevel = settings.roomHFLevel;
        settings2.decayTime = settings.decayTime;
        settings2.decayHFRatio = settings.decayHFRatio;
        settings2.reflectionsLevel = settings.reflectionsLevel;
        settings2.reflectionsDelay = settings.reflectionsDelay;
        settings2.reverbLevel = settings.reverbLevel;
        settings2.reverbDelay = settings.reverbDelay;
        settings2.diffusion = settings.diffusion;
        settings2.density = settings.density;

        return settings2;
    }

    /**
     * EnvironmentalReverb.Settings -&gt; IEnvironmentalReverb.Settings
     *
     * @param settings EnvironmentalReverb.Settings
     * @return IEnvironmentalReverb.Settings
     */
    public static IEnvironmentalReverb.Settings convert(EnvironmentalReverb.Settings settings) {
        IEnvironmentalReverb.Settings settings2 = new IEnvironmentalReverb.Settings();

        settings2.roomLevel = settings.roomLevel;
        settings2.roomHFLevel = settings.roomHFLevel;
        settings2.decayTime = settings.decayTime;
        settings2.decayHFRatio = settings.decayHFRatio;
        settings2.reflectionsLevel = settings.reflectionsLevel;
        settings2.reflectionsDelay = settings.reflectionsDelay;
        settings2.reverbLevel = settings.reverbLevel;
        settings2.reverbDelay = settings.reverbDelay;
        settings2.diffusion = settings.diffusion;
        settings2.density = settings.density;

        return settings2;
    }
}
