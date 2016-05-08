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


package com.h6ah4i.android.example.openslmediaplayer.app.framework;

import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;

public class AppEvent implements Parcelable {

    public static final Parcelable.Creator<AppEvent> CREATOR = new Parcelable.Creator<AppEvent>() {
        @Override
        public AppEvent createFromParcel(Parcel in) {
            return new AppEvent(in);
        }

        @Override
        public AppEvent[] newArray(int size) {
            return new AppEvent[size];
        }
    };

    public AppEvent() {
        this.category = Integer.MIN_VALUE;
        this.event = Integer.MIN_VALUE;
        this.arg1 = 0;
        this.arg2 = 0;
    }

    public AppEvent(int category, int event) {
        this.category = category;
        this.event = event;
        this.arg1 = 0;
        this.arg2 = 0;
    }

    public AppEvent(int category, int event, int arg1, int arg2) {
        this.category = category;
        this.event = event;
        this.arg1 = arg1;
        this.arg2 = arg2;
    }

    public AppEvent(int category, int event, int arg1, float arg2) {
        this.category = category;
        this.event = event;
        this.arg1 = arg1;
        this.arg2 = Float.floatToRawIntBits(arg2);
    }

    public AppEvent(int category, int event, float arg1, int arg2) {
        this.category = category;
        this.event = event;
        this.arg1 = Float.floatToRawIntBits(arg1);
        this.arg2 = arg2;
    }

    private AppEvent(Parcel in) {
        this.category = in.readInt();
        this.event = in.readInt();
        this.arg1 = in.readInt();
        this.arg2 = in.readInt();
        this.extras = in.readBundle();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(this.category);
        dest.writeInt(this.event);
        dest.writeInt(this.arg1);
        dest.writeInt(this.arg2);
        dest.writeBundle(this.extras);
    }

    public float getArg1AsFloat() {
        return Float.intBitsToFloat(this.arg1);
    }

    public float getArg2AsFloat() {
        return Float.intBitsToFloat(this.arg2);
    }

    public int category;
    public int event;
    public int arg1;
    public int arg2;
    public Bundle extras;
}
