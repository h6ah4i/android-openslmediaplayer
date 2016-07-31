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

import java.lang.ref.WeakReference;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.v4.content.LocalBroadcastManager;

public class AppEventBus {
    private static final String APP_EVENT_PREFIX = "APP_EVENT_BUS_";
    private static final String EXTRA_EVENT = "AppEventBus.event";

    private Context mContext;
    private LocalBroadcastManager mBroadcastManager;

    public static class Receiver<T> extends BroadcastReceiver {
        private WeakReference<T> mHolder;
        private final int[] mCategoryFilter;

        public Receiver(T holder, int[] categoryFilter) {
            mHolder = new WeakReference<T>(holder);
            mCategoryFilter = categoryFilter;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            AppEvent event = (AppEvent) intent.getParcelableExtra(EXTRA_EVENT);
            T holder = mHolder.get();

            if (event != null && holder != null) {
                onReceiveAppEvent(holder, event);
            }
        }

        protected void onReceiveAppEvent(T holder, AppEvent event) {
        }

        private int[] getCategoryFilter() {
            return mCategoryFilter;
        }
    }

    public AppEventBus(Context context) {
        mContext = context.getApplicationContext();
        mBroadcastManager = LocalBroadcastManager.getInstance(mContext);
    }

    public void post(AppEvent event) {
        if (event == null)
            throw new IllegalArgumentException();

        mBroadcastManager.sendBroadcast(createEventIntent(event));
    }

    public void register(Receiver<?> receiver) {
        if (receiver == null)
            throw new IllegalArgumentException();

        mBroadcastManager.registerReceiver(receiver, createIntentFilter(receiver));
    }

    public void unregister(Receiver<?> receiver) {
        if (receiver == null)
            return; // safe to pass null receiver

        mBroadcastManager.unregisterReceiver(receiver);
    }

    private static String categoryToActionName(int category) {
        return APP_EVENT_PREFIX + category;
    }

    private static Intent createEventIntent(AppEvent event) {
        Intent intent = new Intent();

        intent.setAction(categoryToActionName(event.category));
        intent.putExtra(EXTRA_EVENT, event);

        return intent;
    }

    private static IntentFilter createIntentFilter(Receiver<?> receiver) {
        int[] categories = receiver.getCategoryFilter();
        IntentFilter filter = new IntentFilter();

        if (categories != null) {
            for (int category : categories) {
                filter.addAction(categoryToActionName(category));
            }
        }

        return filter;
    }
}
