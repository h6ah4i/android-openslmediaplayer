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

import android.os.Binder;

public class LocalServiceBinder<T> extends Binder {
    @SuppressWarnings("unused")
    private static final String TAG = "LocalServiceBinder";
    private final WeakReference<T> mService;

    public LocalServiceBinder(T service) {
        mService = new WeakReference<T>(service);
    }

    public T getService() {
        return mService.get();
    }
}
