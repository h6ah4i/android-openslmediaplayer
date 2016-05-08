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

import android.os.Parcel;
import android.os.Parcelable;

public class BaseAudioEffectStateStore implements Parcelable {

    private boolean mEnabled;

    // === Parcelable ===
    protected BaseAudioEffectStateStore(Parcel in) {
        super();

        mEnabled = readBoolean(in);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        writeBoolean(dest, mEnabled);
    }

    protected static boolean readBoolean(Parcel in) {
        return (in.readInt() != 0);
    }

    protected static void writeBoolean(Parcel dest, boolean value) {
        dest.writeInt(value ? 1 : 0);
    }

    // === Parcelable ===
    public BaseAudioEffectStateStore() {
        super();
    }

    public boolean isEnabled() {
        return mEnabled;
    }

    /* package */void setEnabled(boolean enabled) {
        mEnabled = enabled;
    }

    protected static final class IntParameterNormalizer {
        private final int mMin;
        private final int mMax;

        public IntParameterNormalizer(int min, int max) {
            mMin = min;
            mMax = max;
        }

        public float normalize(int value) {
            return (float) (value - mMin) / (mMax - mMin);
        }

        public int denormalize(float value) {
            return (int) ((value) * (mMax - mMin) + mMin);
        }
    }

    protected static final class ShortParameterNormalizer {
        private final short mMin;
        private final short mMax;

        public ShortParameterNormalizer(short min, short max) {
            mMin = min;
            mMax = max;
        }

        public float normalize(short value) {
            return (float) (value - mMin) / (mMax - mMin);
        }

        public short denormalize(float value) {
            return (short) ((value) * (mMax - mMin) + mMin);
        }
    }

}
