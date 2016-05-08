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


package com.h6ah4i.android.media.openslmediaplayer.utils;

import java.io.FileDescriptor;
import java.io.IOException;

import android.content.Context;
import android.net.Uri;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.compat.AudioAttributes;

public final class MockBasicMediaPlayer implements IBasicMediaPlayer {

    @Override
    public void start() throws IllegalStateException {
    }

    @Override
    public void stop() throws IllegalStateException {
    }

    @Override
    public void pause() throws IllegalStateException {
    }

    @Override
    public void setDataSource(Context context, Uri uri) throws IOException,
            IllegalArgumentException, SecurityException, IllegalStateException {
    }

    @Override
    public void setDataSource(FileDescriptor fd) throws IOException, IllegalArgumentException,
            IllegalStateException {
    }

    @Override
    public void setDataSource(FileDescriptor fd, long offset, long length) throws IOException,
            IllegalArgumentException, IllegalStateException {
    }

    @Override
    public void setDataSource(String path) throws IOException, IllegalArgumentException,
            IllegalStateException {
    }

    @Override
    public void prepare() throws IOException, IllegalStateException {
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
    }

    @Override
    public void seekTo(int msec) throws IllegalStateException {
    }

    @Override
    public void release() {
    }

    @Override
    public void reset() {
    }

    @Override
    public int getAudioSessionId() {
        return 0;
    }

    @Override
    public void setAudioSessionId(int sessionId) throws IllegalArgumentException,
            IllegalStateException {

    }

    @Override
    public int getDuration() {
        return 0;
    }

    @Override
    public void setLooping(boolean looping) {
    }

    @Override
    public int getCurrentPosition() {
        return 0;
    }

    @Override
    public boolean isLooping() {
        return false;
    }

    @Override
    public boolean isPlaying() throws IllegalStateException {
        return false;
    }

    @Override
    public void attachAuxEffect(int effectId) {
    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
    }

    @Override
    public void setAudioStreamType(int streamtype) {
    }

    @Override
    public void setAuxEffectSendLevel(float level) {
    }

    @Override
    public void setWakeMode(Context context, int mode) {
    }

    @Override
    public void setOnBufferingUpdateListener(OnBufferingUpdateListener listener) {
    }

    @Override
    public void setOnCompletionListener(OnCompletionListener listener) {
    }

    @Override
    public void setOnErrorListener(OnErrorListener listener) {
    }

    @Override
    public void setOnInfoListener(OnInfoListener listener) {
    }

    @Override
    public void setOnPreparedListener(OnPreparedListener listener) {
    }

    @Override
    public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
    }

    @Override
    public void setNextMediaPlayer(IBasicMediaPlayer next) {
    }

    @Override
    public void setAudioAttributes(AudioAttributes attributes) {
    }
}
