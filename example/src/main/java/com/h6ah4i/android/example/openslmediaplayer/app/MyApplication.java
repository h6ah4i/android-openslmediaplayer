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


package com.h6ah4i.android.example.openslmediaplayer.app;

import android.app.Application;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;

import com.h6ah4i.android.example.openslmediaplayer.app.model.GlobalAppController;
import com.h6ah4i.android.example.openslmediaplayer.app.model.GlobalAppControllerService;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.GlobalAppControllerAccessor;

public class MyApplication
        extends Application
        implements GlobalAppControllerAccessor.Provider {
    private static final String TAG = "MyApplication";

    // NOTE: This field hold controller instance until service connected
    private GlobalAppController mGlobalAppController;

    private GlobalAppControllerService mGlobalAppControllerService;

    private final ServiceConnection mGlobalAppControllerServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            mGlobalAppControllerService =
                    ((GlobalAppControllerService.LocalBinder) service).getService();
            onGlobalAppControllerServiceConnected();
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            mGlobalAppControllerService = null;
            onGlobalAppControllerServiceDisconnected();
        }
    };

    private void doBindGlobalAppControllerService() {
        if (mGlobalAppControllerService != null)
            return;

        bindService(
                new Intent(MyApplication.this, GlobalAppControllerService.class),
                mGlobalAppControllerServiceConnection, Context.BIND_AUTO_CREATE);
    }

    void doUnbindGlobalAppControllerService() {
        if (mGlobalAppControllerService == null)
            return;

        unbindService(mGlobalAppControllerServiceConnection);
        mGlobalAppControllerService = null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mGlobalAppController = new GlobalAppController(this, null);
        doBindGlobalAppControllerService();
    }

    @Override
    public void onTerminate() {
        doUnbindGlobalAppControllerService();
        super.onTerminate();
    }

    @Override
    public GlobalAppController getGlobalAppControllerInstance() {
        if (mGlobalAppControllerService != null) {
            return mGlobalAppControllerService.getGlobalAppController();
        } else {
            return mGlobalAppController;
        }
    }

    private void onGlobalAppControllerServiceConnected() {
        Log.d(TAG, "onGlobalAppControllerServiceConnected()");
        mGlobalAppControllerService.setGlobalAppController(mGlobalAppController);
        mGlobalAppController = null;
    }

    private void onGlobalAppControllerServiceDisconnected() {
        Log.d(TAG, "onGlobalAppControllerServiceDisconnected()");
    }
}
