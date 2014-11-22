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

package com.h6ah4i.android.media;

import com.h6ah4i.android.media.audiofx.IBassBoost;
import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.audiofx.IHQVisualizer;
import com.h6ah4i.android.media.audiofx.ILoudnessEnhancer;
import com.h6ah4i.android.media.audiofx.IPreAmp;
import com.h6ah4i.android.media.audiofx.IPresetReverb;
import com.h6ah4i.android.media.audiofx.IVirtualizer;
import com.h6ah4i.android.media.audiofx.IVisualizer;

public interface IMediaPlayerFactory extends IReleasable {
    /**
     * {@inheritDoc}
     */
    @Override
    void release()
            throws IllegalStateException, UnsupportedOperationException;

    /**
     * Create BasicMediaPlayer object
     *
     * @return BasicMediaPlayer object
     */
    IBasicMediaPlayer createMediaPlayer()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create BassBoost object
     *
     * @param audioSession system wide unique audio session identifier.
     * @return BassBoost object
     */
    IBassBoost createBassBoost(int audioSession)
            throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create BassBoost object
     *
     * @param player the BasicMediaPlayer instance to attached the BassBoost
     *            effect.
     * @return BassBoost object
     */
    IBassBoost createBassBoost(IBasicMediaPlayer player)
            throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create Equalizer object
     *
     * @param audioSession system wide unique audio session identifier.
     * @return Equalizer object
     */
    IEqualizer createEqualizer(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create Equalizer object
     *
     * @param player the BasicMediaPlayer instance to attached the Equalizer
     *            effect.
     * @return Equalizer object
     */
    IEqualizer createEqualizer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create Virtualizer object
     *
     * @param audioSession system wide unique audio session identifier.
     * @return Virtualizer object
     */
    IVirtualizer createVirtualizer(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create Virtualizer object
     *
     * @param player the BasicMediaPlayer instance to attached the Virtualizer
     *            effect.
     * @return Virtualizer object
     */
    IVirtualizer createVirtualizer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create Visualizer object
     *
     * @param audioSession system wide unique audio session identifier.
     * @return Visualizer object
     */
    IVisualizer createVisualizer(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create Visualizer object
     *
     * @param player the BasicMediaPlayer instance to attached the Visualizer
     *            effect.
     * @return Visualizer object
     */
    IVisualizer createVisualizer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create LoudnessEnhancer object
     *
     * @param audioSession system wide unique audio session identifier.
     * @return LoudnessEnhancer object
     */
    ILoudnessEnhancer createLoudnessEnhancer(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create LoudnessEnhancer object
     *
     * @param player the BasicMediaPlayer instance to attached the
     *            LoudnessEnhancer effect.
     * @return LoudnessEnhancer object
     */
    ILoudnessEnhancer createLoudnessEnhancer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create PresetReverb object
     *
     * @return PresetReverb object
     */
    IPresetReverb createPresetReverb()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create EnvironmentalReverb object
     *
     * @return EnvironmentalReverb object
     */
    IEnvironmentalReverb createEnvironmentalReverb()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create HQEqualizer object
     *
     * @return HQEqualizer object
     */
    IEqualizer createHQEqualizer()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create HQVisualizer object
     *
     * @return HQVisualizer object
     */
    IHQVisualizer createHQVisualizer()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;

    /**
     * Create PreAmp object
     *
     * @return PreAmp object
     */
    IPreAmp createPreAmp()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException;
}
