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

import com.h6ah4i.android.media.IBasicMediaPlayer;

public class InfoListenerObject
        extends BasicMediaPlayerEventListenerObject
        implements IBasicMediaPlayer.OnInfoListener {

    public InfoListenerObject(boolean handled) {
        super();
        mHandled = handled;
    }

    public InfoListenerObject(Object syncObj, boolean handled) {
        super(syncObj);
        mHandled = handled;
    }

    @Override
    public boolean onInfo(IBasicMediaPlayer mp, int what, int extra) {
        mWhat = what;
        mExtra = extra;

        trigger();

        return mHandled;
    }

    @Override
    public String toString() {
        return getClass().getSimpleName() + " [" +
                "triggered = " + triggered() + ", " +
                "handled = " + mHandled + ", " +
                "what = " + mWhat + ", " +
                "extra = " + mExtra +
                "]";
    }

    public int what() {
        return mWhat;
    }

    public int extra() {
        return mExtra;
    }

    private final boolean mHandled;

    private int mWhat;
    private int mExtra;
}
