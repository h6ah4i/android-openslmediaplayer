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

package com.h6ah4i.android.media.standard;

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
import com.h6ah4i.android.media.standard.audiofx.StandardBassBoost;
import com.h6ah4i.android.media.standard.audiofx.StandardEnvironmentalReverb;
import com.h6ah4i.android.media.standard.audiofx.StandardEqualizer;
import com.h6ah4i.android.media.standard.audiofx.StandardLoudnessEnhancer;
import com.h6ah4i.android.media.standard.audiofx.StandardPresetReverb;
import com.h6ah4i.android.media.standard.audiofx.StandardVirtualizer;
import com.h6ah4i.android.media.standard.audiofx.StandardVisualizer;

public class StandardMediaPlayerFactory implements IMediaPlayerFactory {
    private Context mContext;

    public StandardMediaPlayerFactory(Context context) {
        mContext = context.getApplicationContext();
    }

    @Override
    public void release() throws IllegalStateException, UnsupportedOperationException {
        mContext = null;
    }

    @Override
    public IBasicMediaPlayer createMediaPlayer()
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return new StandardMediaPlayer();
    }

    @Override
    public IBassBoost createBassBoost(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateBassBoost(audioSession);
    }

    @Override
    public IBassBoost createBassBoost(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsStandardMediaPlayer(player);
        return onCreateBassBoost((StandardMediaPlayer) player);
    }

    @Override
    public IEqualizer createEqualizer(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateEqualizer(audioSession);
    }

    @Override
    public IEqualizer createEqualizer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsStandardMediaPlayer(player);
        return onCreateEqualizer((StandardMediaPlayer) player);
    }

    @Override
    public IVirtualizer createVirtualizer(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateVirtualizer(audioSession);
    }

    @Override
    public IVirtualizer createVirtualizer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsStandardMediaPlayer(player);
        return onCreateVirtualizer((StandardMediaPlayer) player);
    }

    @Override
    public IVisualizer createVisualizer(int audioSession)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateVisualizer(audioSession);
    }

    @Override
    public IVisualizer createVisualizer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        checkIsStandardMediaPlayer(player);
        return onCreateVisualizer((StandardMediaPlayer) player);
    }

    @Override
    public ILoudnessEnhancer createLoudnessEnhancer(int audioSession) throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        return onCreateLoudnessEnhancer(audioSession);
    }

    @Override
    public ILoudnessEnhancer createLoudnessEnhancer(IBasicMediaPlayer player)
            throws IllegalStateException, IllegalArgumentException, UnsupportedOperationException {
        return onCreateLoudnessEnhancer((StandardMediaPlayer) player);
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
    public IEqualizer createHQEqualizer() throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        throw new UnsupportedOperationException("HQEqualizer is not supported");
    }

    @Override
    public IHQVisualizer createHQVisualizer() throws IllegalStateException,
            IllegalArgumentException, UnsupportedOperationException {
        throw new UnsupportedOperationException("HQVisualizer is not supported");
    }

    @Override
    public IPreAmp createPreAmp() throws IllegalStateException, IllegalArgumentException,
            UnsupportedOperationException {
        throw new UnsupportedOperationException("PreAmp is not supported");
    }

    protected StandardBassBoost onCreateBassBoost(int audioSession) {
        return new StandardBassBoost(0, audioSession);
    }

    protected StandardBassBoost onCreateBassBoost(StandardMediaPlayer player) {
        return new StandardBassBoost(0, player.getAudioSessionId());
    }

    protected StandardVirtualizer onCreateVirtualizer(int audioSession) {
        return new StandardVirtualizer(0, audioSession);
    }

    protected StandardVirtualizer onCreateVirtualizer(StandardMediaPlayer player) {
        return new StandardVirtualizer(0, player.getAudioSessionId());
    }

    protected StandardEqualizer onCreateEqualizer(int audioSession) {
        return new StandardEqualizer(0, audioSession);
    }

    protected StandardEqualizer onCreateEqualizer(StandardMediaPlayer player) {
        return new StandardEqualizer(0, player.getAudioSessionId());
    }

    protected StandardVisualizer onCreateVisualizer(int audioSession) {
        return new StandardVisualizer(mContext, audioSession);
    }

    protected StandardVisualizer onCreateVisualizer(StandardMediaPlayer player) {
        return new StandardVisualizer(mContext, player.getAudioSessionId());
    }

    protected StandardLoudnessEnhancer onCreateLoudnessEnhancer(int audioSession) {
        final StandardLoudnessEnhancer effect = new StandardLoudnessEnhancer(audioSession);
        if (effect == null) {
            throw new UnsupportedOperationException("StandardLoudnessEnhancer is not supported");
        }
        return effect;
    }

    protected StandardLoudnessEnhancer onCreateLoudnessEnhancer(StandardMediaPlayer player) {
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

    protected static void checkIsStandardMediaPlayer(IBasicMediaPlayer player) {
        if (player == null)
            throw new IllegalArgumentException("The argument 'player' is null");
        if (!(player instanceof StandardMediaPlayer))
            throw new IllegalArgumentException("The player is not instance of OpenSLMediaPlayer");
    }
}
