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


package com.h6ah4i.android.example.openslmediaplayer.app.utils;

import java.lang.ref.WeakReference;

import android.view.View;
import android.widget.AdapterView;

public class OnItemSelectedListenerWrapper implements AdapterView.OnItemSelectedListener {
    private WeakReference<AdapterView.OnItemSelectedListener> mListenerRef;
    private boolean mIsLoaded;

    public OnItemSelectedListenerWrapper(AdapterView.OnItemSelectedListener listener) {
        mListenerRef = new WeakReference<AdapterView.OnItemSelectedListener>(listener);
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        AdapterView.OnItemSelectedListener listener = mListenerRef.get();

        if (mIsLoaded && listener != null) {
            listener.onItemSelected(parent, view, position, id);
        }

        mIsLoaded = true;
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
        AdapterView.OnItemSelectedListener listener = mListenerRef.get();

        if (mIsLoaded && listener != null) {
            listener.onNothingSelected(parent);
        }

        mIsLoaded = true;
    }
}
