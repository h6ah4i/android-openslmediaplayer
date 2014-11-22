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

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import com.h6ah4i.android.example.openslmediaplayer.app.utils.LocalServiceBinder;

public class GlobalAppControllerService extends Service {
    private static final String TAG = "GlobalAppControllerService";

    // internal classes
    public static class LocalBinder extends
            LocalServiceBinder<GlobalAppControllerService> {
        public LocalBinder(GlobalAppControllerService service) {
            super(service);
        }
    }

    // fields
    private GlobalAppController mController;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "onStartComand(flags = " + flags + ", startId = " + startId);
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind()");
        return new LocalBinder(this);
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Log.d(TAG, "onUnbind()");
        return super.onUnbind(intent);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    public GlobalAppController getGlobalAppController() {
        return mController;
    }

    public void setGlobalAppController(GlobalAppController controller) {
        mController = controller;

        if (mController != null) {
            mController.bindHolderService(this);
        }
    }
}
