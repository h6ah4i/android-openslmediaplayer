/*
 *    Copyright (C) 2016 Haruki Hasegawa
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

package com.h6ah4i.android.media.hybrid;

import android.content.Context;

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
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayer;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerContext;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLHQEqualizer;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLHQVisualizer;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLPreAmp;
import com.h6ah4i.android.media.opensl.audiofx.OpenSLVisualizer;
import com.h6ah4i.android.media.standard.audiofx.StandardBassBoost;
import com.h6ah4i.android.media.standard.audiofx.StandardEnvironmentalReverb;
import com.h6ah4i.android.media.standard.audiofx.StandardEqualizer;
import com.h6ah4i.android.media.standard.audiofx.StandardLoudnessEnhancer;
import com.h6ah4i.android.media.standard.audiofx.StandardPresetReverb;
import com.h6ah4i.android.media.standard.audiofx.StandardVirtualizer;

public class HybridMediaPlayerFactory implements IMediaPlayerFactory {
    private Context mContext;
    private OpenSLMediaPlayerContext mMediaPlayerContext;

    public HybridMediaPlayerFactory(Context context) {
        mContext = context;
        mMediaPlayerContext = new OpenSLMediaPlayerContext(context, getDefaultContextParams());
    }

    public HybridMediaPlayerFactory(Context context, OpenSLMediaPlayerContext.Parameters params) {
        mContext = context;
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
        mContext = null;
    }

    @Override
    public IBasicMediaPlayer createMediaPlayer() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return new OpenSLMediaPlayer(getOpenSLMediaPlayerContext(), getMediaPlayerOptions());
    }

    @Override
    public IBassBoost createBassBoost(int audioSession) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateBassBoost(audioSession);
    }

    @Override
    public IBassBoost createBassBoost(IBasicMediaPlayer player) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateBassBoost((OpenSLMediaPlayer) player);
    }

    @Override
    public IEqualizer createEqualizer(int audioSession) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateEqualizer(audioSession);
    }

    @Override
    public IEqualizer createEqualizer(IBasicMediaPlayer player) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateEqualizer((OpenSLMediaPlayer) player);
    }

    @Override
    public IVirtualizer createVirtualizer(int audioSession) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateVirtualizer(audioSession);
    }

    @Override
    public IVirtualizer createVirtualizer(IBasicMediaPlayer player) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateVirtualizer((OpenSLMediaPlayer) player);
    }

    @Override
    public IVisualizer createVisualizer(int audioSession) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateVisualizer(audioSession);
    }

    @Override
    public IVisualizer createVisualizer(IBasicMediaPlayer player) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateVisualizer((OpenSLMediaPlayer) player);
    }

    @Override
    public ILoudnessEnhancer createLoudnessEnhancer(int audioSession) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateLoudnessEnhancer(audioSession);
    }

    @Override
    public ILoudnessEnhancer createLoudnessEnhancer(IBasicMediaPlayer player) throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsOpenSLMediaPlayer(player);
        return onCreateLoudnessEnhancer((OpenSLMediaPlayer) player);
    }

    @Override
    public IPresetReverb createPresetReverb() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreatePresetReverb();
    }

    @Override
    public IEnvironmentalReverb createEnvironmentalReverb() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateEnvironmentalReverb();
    }

    @Override
    public IEqualizer createHQEqualizer() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateHQEqualizer();
    }

    @Override
    public IHQVisualizer createHQVisualizer() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateHQVisualizer();
    }

    @Override
    public IPreAmp createPreAmp() throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreatePreAmp();
    }

    protected StandardBassBoost onCreateBassBoost(int audioSession) {
        return new StandardBassBoost(0, audioSession);
    }

    protected StandardBassBoost onCreateBassBoost(OpenSLMediaPlayer player) {
        return new StandardBassBoost(0, player.getAudioSessionId());
    }

    protected StandardVirtualizer onCreateVirtualizer(int audioSession) {
        return new StandardVirtualizer(0, audioSession);
    }

    protected StandardVirtualizer onCreateVirtualizer(OpenSLMediaPlayer player) {
        return new StandardVirtualizer(0, player.getAudioSessionId());
    }

    protected StandardEqualizer onCreateEqualizer(int audioSession) {
        return new StandardEqualizer(0, audioSession);
    }

    protected StandardEqualizer onCreateEqualizer(OpenSLMediaPlayer player) {
        return new StandardEqualizer(0, player.getAudioSessionId());
    }

    protected OpenSLVisualizer onCreateVisualizer(int audioSession) {
        return new OpenSLVisualizer(getOpenSLMediaPlayerContext());
    }

    protected OpenSLVisualizer onCreateVisualizer(OpenSLMediaPlayer player) {
        return new OpenSLVisualizer(getOpenSLMediaPlayerContext());
    }

    protected StandardLoudnessEnhancer onCreateLoudnessEnhancer(int audioSession) {
        final StandardLoudnessEnhancer effect = new StandardLoudnessEnhancer(audioSession);
        if (effect == null) {
            throw new UnsupportedOperationException("StandardLoudnessEnhancer is not supported");
        }
        return effect;
    }

    protected StandardLoudnessEnhancer onCreateLoudnessEnhancer(OpenSLMediaPlayer player) {
        final StandardLoudnessEnhancer effect = new StandardLoudnessEnhancer(player.getAudioSessionId());
        if (effect == null) {
            throw new UnsupportedOperationException("StandardLoudnessEnhancer is not supported");
        }
        return effect;
    }

    protected StandardPresetReverb onCreatePresetReverb() {
        // NOTE: Auxiliary effects can be created for session 0 only
        return new StandardPresetReverb(0, 0);
    }

    protected StandardEnvironmentalReverb onCreateEnvironmentalReverb() {
        // NOTE: Auxiliary effects can be created for session 0 only
        return new StandardEnvironmentalReverb(0, 0);
    }

    protected OpenSLHQEqualizer onCreateHQEqualizer() {
        return new OpenSLHQEqualizer(getOpenSLMediaPlayerContext());
    }

    protected OpenSLHQVisualizer onCreateHQVisualizer() {
        return new OpenSLHQVisualizer(getOpenSLMediaPlayerContext());
    }

    protected OpenSLPreAmp onCreatePreAmp() {
        return new OpenSLPreAmp(getOpenSLMediaPlayerContext());
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

    protected int getDefaultContextOptions() {
        int options = 0;

        options |= OpenSLMediaPlayerContext.OPTION_USE_VISUALIZER;
        options |= OpenSLMediaPlayerContext.OPTION_USE_HQ_EQUALIZER;
        options |= OpenSLMediaPlayerContext.OPTION_USE_PREAMP;
        options |= OpenSLMediaPlayerContext.OPTION_USE_HQ_VISUALIZER;

        return options;
    }

    protected OpenSLMediaPlayerContext.Parameters getDefaultContextParams() {
        final OpenSLMediaPlayerContext.Parameters params = new OpenSLMediaPlayerContext.Parameters();

        // override parameters
        params.sinkBackEndType = OpenSLMediaPlayerContext.SINK_BACKEND_TYPE_AUDIO_TRACK;

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
