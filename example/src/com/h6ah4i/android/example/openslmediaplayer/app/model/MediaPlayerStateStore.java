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


package com.h6ah4i.android.example.openslmediaplayer.app.model;

import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;

import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PlayerControlReqEvents;

public class MediaPlayerStateStore implements Parcelable {
    public static final int PLAYER_IMPL_TYPE_STANDARD = 0;
    public static final int PLAYER_IMPL_TYPE_OPENSL = 1;

    private float mAuxEffectSendLevel;
    private float mVolumeLeft;
    private float mVolumeRight;
    private boolean mIsLooping;

    private int mPlayerType;
    private int mSelectedAuxEffectType;
    private Uri[] mMediaUri;

    // === Parcelable ===
    public static final Parcelable.Creator<MediaPlayerStateStore> CREATOR = new Parcelable.Creator<MediaPlayerStateStore>() {
        @Override
        public MediaPlayerStateStore createFromParcel(Parcel in) {
            return new MediaPlayerStateStore(in);
        }

        @Override
        public MediaPlayerStateStore[] newArray(int size) {
            return new MediaPlayerStateStore[size];
        }
    };

    private MediaPlayerStateStore(Parcel in) {
        mMediaUri = new Uri[2];

        mAuxEffectSendLevel = in.readFloat();
        mVolumeLeft = in.readFloat();
        mVolumeRight = in.readFloat();
        mIsLooping = readBoolean(in);
        mPlayerType = in.readInt();
        mSelectedAuxEffectType = in.readInt();
        mMediaUri[0] = readUri(in);
        mMediaUri[1] = readUri(in);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeFloat(mAuxEffectSendLevel);
        dest.writeFloat(mVolumeLeft);
        dest.writeFloat(mVolumeRight);
        writeBoolean(dest, mIsLooping);
        dest.writeInt(mPlayerType);
        dest.writeInt(mSelectedAuxEffectType);
        writeUri(dest, mMediaUri[0]);
        writeUri(dest, mMediaUri[1]);
    }

    protected static boolean readBoolean(Parcel in) {
        return (in.readInt() != 0);
    }

    protected static void writeBoolean(Parcel dest, boolean value) {
        dest.writeInt(value ? 1 : 0);
    }

    protected static Uri readUri(Parcel in) {
        String str = in.readString();
        if (str == null)
            return null;

        return Uri.parse(str);
    }

    protected static void writeUri(Parcel dest, Uri uri) {
        String str = (uri != null) ? uri.toString() : null;
        dest.writeString(str);
    }

    // === Parcelable ===

    public MediaPlayerStateStore() {
        mMediaUri = new Uri[2];

        mAuxEffectSendLevel = 1.0f;
        mVolumeLeft = 1.0f;
        mVolumeRight = 1.0f;
        mIsLooping = false;

        mPlayerType = PLAYER_IMPL_TYPE_STANDARD;
        mSelectedAuxEffectType = PlayerControlReqEvents.AUX_EEFECT_TYPE_NONE;
    }

    public float getAuxEffectSendLevel() {
        return mAuxEffectSendLevel;
    }

    /* package */void setAuxEffectSendLevel(float auxEffectSendLevel) {
        mAuxEffectSendLevel = auxEffectSendLevel;
    }

    public float getVolumeLeft() {
        return mVolumeLeft;
    }

    /* package */void setVolumeLeft(float volumeLeft) {
        mVolumeLeft = volumeLeft;
    }

    public float getVolumeRight() {
        return mVolumeRight;
    }

    /* package */void setVolumeRight(float volumeRight) {
        mVolumeRight = volumeRight;
    }

    public boolean isLooping() {
        return mIsLooping;
    }

    /* package */void setLooping(boolean looping) {
        mIsLooping = looping;
    }

    public int getSelectedAuxEffectType() {
        return mSelectedAuxEffectType;
    }

    /* package */void setSelectedAuxEffectType(int type) {
        mSelectedAuxEffectType = type;
    }

    public int getPlayerImplType() {
        return mPlayerType;
    }

    /* package */void setPlayerImplType(int type) {
        mPlayerType = type;
    }

    public Uri getMediaUri(int index) {
        return mMediaUri[index];
    }

    /* package */void setMediaUri(int index, Uri uri) {
        mMediaUri[index] = uri;
    }
}
