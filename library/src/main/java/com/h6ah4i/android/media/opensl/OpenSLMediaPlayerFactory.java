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


package com.h6ah4i.android.media.opensl;

import android.content.Context;
import android.media.MediaPlayer;
import android.media.audiofx.Equalizer;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.audiofx.IBassBoost;
import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.audiofx.IHQVisualizer;
import com.h6ah4i.android.media.audiofx.ILoudnessEnhancer;
import com.h6ah4i.android.media.audiofx.IPreAmp;
import com.h6ah4i.android.media.audiofx.IPresetReverb;
import com.h6ah4i.android.media.audiofx.IVirtualizer;
import com.h6ah4i.android.media.audiofx.IVisualizer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerContext.Parameters;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLBassBoost;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLEnvironmentalReverb;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLEqualizer;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLHQEqualizer;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLHQVisualizer;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLPreAmp;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLPresetReverb;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLVirtualizer;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLVisualizer;

public class OpenSLMediaPlayerFactory implements IMediaPlayerFactory {
    private OpenSLMediaPlayerContext mMediaPlayerContext;

    public OpenSLMediaPlayerFactory(Context context) {
        mMediaPlayerContext = new OpenSLMediaPlayerContext(context, getDefaultContextParams());
    }

    public OpenSLMediaPlayerFactory(Context context, OpenSLMediaPlayerContext.Parameters params) {
        mMediaPlayerContext = new OpenSLMediaPlayerContext(context, params);
    }

    public OpenSLMediaPlayerContext getOpenSLMediaPlayerContext() {
        return mMediaPlayerContext;
    }

    @Override
    public void release() throws IllegalStateException, UnsupportedOperationException {
        if (mMediaPlayerContext != null) {
            mMediaPlayerContext.release();
            mMediaPlayerContext = null;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        release();
    }

    @Override
    public IBasicMediaPlayer createMediaPlayer()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return new OpenSLMediaPlayer(getMediaPlayerContext(), getMediaPlayerOptions());
    }

    @Override
    public IBassBoost createBassBoost(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateBassBoost(audioSession);
    }

    @Override
    public IBassBoost createBassBoost(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateBassBoost((OpenSLMediaPlayer) player);
    }

    @Override
    public IEqualizer createEqualizer(int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        return onCreateEqualizer(audioSession);
    }

    @Override
    public IEqualizer createEqualizer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateEqualizer((OpenSLMediaPlayer) player);
    }

    @Override
    public IVirtualizer createVirtualizer(int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        return onCreateVirtualizer(audioSession);
    }

    @Override
    public IVirtualizer createVirtualizer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateVirtualizer((OpenSLMediaPlayer) player);
    }
    
    @Override
    public ILoudnessEnhancer createLoudnessEnhancer(int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        throwNotSupportedError();
        return null;
    }
    
    @Override
    public ILoudnessEnhancer createLoudnessEnhancer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        throwNotSupportedError();
        return null;
    }

    @Override
    public IPresetReverb createPresetReverb()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreatePresetReverb();
    }

    @Override
    public IEnvironmentalReverb createEnvironmentalReverb()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateEnvironmentalReverb();
    }

    @Override
    public IVisualizer createVisualizer(int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        return onCreateVisualizer(audioSession);
    }

    @Override
    public IVisualizer createVisualizer(IBasicMediaPlayer player) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateVisualizer((OpenSLMediaPlayer) player);
    }

    @Override
    public IEqualizer createHQEqualizer() throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        return onCreateHQEqualizer();
    }

    @Override
    public IHQVisualizer createHQVisualizer() throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        return onCreateHQVisualizer();
    }

    @Override
    public IPreAmp createPreAmp() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        return onCreatePreAmp();
    }

    protected OpenSLBassBoost onCreateBassBoost(int audioSession) {
        return new OpenSLBassBoost(getMediaPlayerContext());
    }

    protected OpenSLBassBoost onCreateBassBoost(OpenSLMediaPlayer player) {
        return new OpenSLBassBoost(getMediaPlayerContext());
    }

    protected OpenSLEqualizer onCreateEqualizer(int audioSession) {
        return new OpenSLEqualizer(getMediaPlayerContext());
    }

    protected OpenSLEqualizer onCreateEqualizer(OpenSLMediaPlayer player) {
        return new OpenSLEqualizer(getMediaPlayerContext());
    }

    protected OpenSLVirtualizer onCreateVirtualizer(int audioSession) {
        return new OpenSLVirtualizer(getMediaPlayerContext());
    }

    protected OpenSLVirtualizer onCreateVirtualizer(OpenSLMediaPlayer player) {
        return new OpenSLVirtualizer(getMediaPlayerContext());
    }

    private OpenSLVisualizer onCreateVisualizer(int audioSession) {
        return new OpenSLVisualizer(getMediaPlayerContext());
    }

    private OpenSLVisualizer onCreateVisualizer(OpenSLMediaPlayer player) {
        return new OpenSLVisualizer(getMediaPlayerContext());
    }

    protected OpenSLPresetReverb onCreatePresetReverb() {
        return new OpenSLPresetReverb(getMediaPlayerContext());
    }

    protected OpenSLEnvironmentalReverb onCreateEnvironmentalReverb() {
        return new OpenSLEnvironmentalReverb(getMediaPlayerContext());
    }

    protected OpenSLHQEqualizer onCreateHQEqualizer() {
        return new OpenSLHQEqualizer(getMediaPlayerContext());
    }

    protected OpenSLHQVisualizer onCreateHQVisualizer() {
        return new OpenSLHQVisualizer(getMediaPlayerContext());
    }

    protected OpenSLPreAmp onCreatePreAmp() {
        return new OpenSLPreAmp(getMediaPlayerContext());
    }

    protected OpenSLMediaPlayerContext getMediaPlayerContext() {
        return mMediaPlayerContext;
    }

    protected int getMediaPlayerOptions() {
        return OpenSLMediaPlayer.OPTION_USE_FADE;
    }

    protected static void checkIsOpenSLMediaPlayer(IBasicMediaPlayer player) {
        if (player == null)
            throw new IllegalArgumentException("The argument 'player' is null");
        if (!(player instanceof OpenSLMediaPlayer))
            throw new IllegalArgumentException("The player is not instance of OpenSLMediaPlayer");
    }

    protected void throwNotSupportedError() {
        throw new UnsupportedOperationException("This method is not supported");
    }

    protected static int getEqualizerNumberOfBands() {
        MediaPlayer player = null;
        Equalizer eq = null;
        try {
            player = new MediaPlayer();
            eq = new Equalizer(0, player.getAudioSessionId());
            return eq.getNumberOfBands();
        } catch (Exception e) {
        } finally {
            if (eq != null) {
                eq.release();
                eq = null;
            }
            if (player != null) {
                player.release();
                player = null;
            }
        }
        return 0;
    }

    protected int getDefaultContextOptions() {
        boolean hasCyanogenModDSPManager = (getEqualizerNumberOfBands() == 6);
        int options = 0;

        // [WARNING]
        //
        // Enabling Android Framework audio effects will cause audio glitches.
        // - Bass boost
        // - Virtualizer
        // - Equalizer
        // - Environmental Reverb
        // - Preset Reverb
        //
        // These OSLMP features are not affected
        // - Pre Amp
        // - HQ Equalizer
        // - Visualizer
        // - HQ Visualizer

        if (!hasCyanogenModDSPManager) {
            options |= OpenSLMediaPlayerContext.OPTION_USE_BASSBOOST;
            options |= OpenSLMediaPlayerContext.OPTION_USE_VIRTUALIZER;
            options |= OpenSLMediaPlayerContext.OPTION_USE_EQUALIZER;
        } else {
            // NOTE:
            // CyanogenMod causes app crash if those effects are enabled
        }

        options |= OpenSLMediaPlayerContext.OPTION_USE_ENVIRONMENAL_REVERB;
        options |= OpenSLMediaPlayerContext.OPTION_USE_PRESET_REVERB;
        options |= OpenSLMediaPlayerContext.OPTION_USE_VISUALIZER;
        options |= OpenSLMediaPlayerContext.OPTION_USE_HQ_EQUALIZER;
        options |= OpenSLMediaPlayerContext.OPTION_USE_PREAMP;
        options |= OpenSLMediaPlayerContext.OPTION_USE_HQ_VISUALIZER;

        return options;
    }

    protected Parameters getDefaultContextParams() {
        final OpenSLMediaPlayerContext.Parameters params = new OpenSLMediaPlayerContext.Parameters();

        // override parameters
        params.sinkBackEndType = OpenSLMediaPlayerContext.SINK_BACKEND_TYPE_OPENSL;

        params.options = getDefaultContextOptions();

        // FYI: Specify these options to get the best quality & result
        //
        // params.resamplerQuality =
        // OpenSLMediaPlayerContext.RESAMPLER_QUALITY_HIGH;
        // params.hqEqualizerImplType =
        // OpenSLMediaPlayerContext.HQ_EQUALIZER_IMPL_FLAT_GAIN_RESPONSE;

        return params;
    }
}
