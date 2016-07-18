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

package com.h6ah4i.android.example.nativeopenslmediaplayer;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Instrumentation;
import android.app.NativeActivity;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.provider.MediaStore;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.Toast;

import java.io.File;
import java.lang.reflect.Field;
import java.util.Timer;
import java.util.TimerTask;

public class MainNativeActivity extends NativeActivity {

    // constants
    private static final int REQ_SONG_PICKER = 1;

    private long mNativeHandle;

    private RelativeLayout mControlsView;
    private Button mButtonOpenMedia;
    private Button mButtonPlay;
    private Button mButtonPause;
    private Button mButtonAbout;
    private Handler mHandler;
    private Runnable mDeferredSetupControlViews;
    private Runnable mDeferredHandleSongPicked;
    private Timer mTimer;
    private TimerTask mSendFakeTouchTask;

    static {
        System.loadLibrary("OpenSLMediaPlayer");
        System.loadLibrary("OpenSLMediaPlayerNativeAPIExample");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mNativeHandle = getSuperNativeHandle();
        mHandler = new Handler();

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();

        mDeferredSetupControlViews = new Runnable() {
            @Override
            public void run() {
                mDeferredSetupControlViews = null;
                setupControlViews();
            }
        };
        getWindow().getDecorView().post(mDeferredSetupControlViews);

        startFakeTouchTask();
    }

    @Override
    protected void onPause() {
        cancelDeferredSetupControlViews();
        cancelDeferredHandleSongPicked();
        cancelSendFakeTouchTask();

        removeControlViews();

        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQ_SONG_PICKER:
                if (resultCode == Activity.RESULT_OK) {
                    final Uri uri = data.getData();
                    cancelDeferredHandleSongPicked();
                    mDeferredHandleSongPicked = new Runnable() {
                        @Override
                        public void run() {
                            mDeferredHandleSongPicked = null;
                            handleSongPicked(uri);
                        }
                    };
                    mHandler.post(mDeferredHandleSongPicked);
                }
                break;
        }
    }

    private void cancelDeferredSetupControlViews() {
        if (mDeferredSetupControlViews != null) {
            getWindow().getDecorView().removeCallbacks(mDeferredSetupControlViews);
            mDeferredSetupControlViews = null;
        }
    }

    private void cancelDeferredHandleSongPicked() {
        if (mDeferredHandleSongPicked != null) {
            mHandler.removeCallbacks(mDeferredHandleSongPicked);
            mDeferredHandleSongPicked = null;
        }
    }

    private void cancelSendFakeTouchTask() {
        if (mTimer != null) {
            mTimer.cancel();
            mTimer.purge();
            mTimer = null;
            mSendFakeTouchTask = null;
        }
    }

    private void startFakeTouchTask() {
        // Solution/Hack #3 - "Send fake touches!"
        // https://www.youtube.com/watch?v=F2ZDp-eNrh4

        mTimer = new Timer();
        mSendFakeTouchTask = new TimerTask() {
            @Override
            public void run() {
                Instrumentation instrumentation = new Instrumentation();
                instrumentation.sendKeyDownUpSync(KeyEvent.KEYCODE_BACKSLASH);
            }
        };
        mTimer.schedule(mSendFakeTouchTask, 1000, 1000);
    }

    private void handleSongPicked(Uri uri) {
        final String filePath = resolveFilePathFromUri(this, uri);

        if (filePath != null) {
            nativeSetDataSourceAndPrepare(mNativeHandle, filePath);
        } else {
            Toast.makeText(
                    this,
                    "Sorry, this example does not support that URI",
                    Toast.LENGTH_LONG).show();
        }
    }

    private static String resolveFilePathFromUri(Context context, Uri uri) {
        String filePath = null;
        String scheme = uri.getScheme();
        if ("file".equals(scheme)) {
            filePath = uri.getPath();
        } else if ("content".equals(scheme)) {
            String path = uri.getPath();

            if (new File(path).exists()) {
                filePath = path;
            } else {
                Cursor c = null;

                try {
                    Uri mediaUri;

                    if (path.startsWith("/document/audio:")) {
                        final long id = Long.parseLong(path.substring("/document/audio:".length()));
                        mediaUri = ContentUris.withAppendedId(
                                MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, id);
                    } else {
                        mediaUri = uri;
                    }

                    c = context.getContentResolver().query(mediaUri, new String[] {
                            MediaStore.Audio.Media.DATA
                    }, null, null, null);
                    if (c.moveToFirst()) {
                        filePath = c.getString(0);
                    }
                } catch (RuntimeException e) {
                    // just ignore errors...
                } finally {

                    if (c != null) {
                        c.close();
                    }
                }
            }
        }
        return filePath;
    }

    // Read NativeActivity.mNativeHandle field (= *ANativeActivity in C++)
    private long getSuperNativeHandle() {
        long handle = 0;
        try {
            Field f = NativeActivity.class.getDeclaredField("mNativeHandle");
            f.setAccessible(true);
            handle = f.getLong(this);
        } catch (NoSuchFieldException e) {
        } catch (IllegalAccessException e) {
        } catch (IllegalArgumentException e) {
        }

        return handle;
    }

    @SuppressLint({
            "InflateParams", "RtlHardcoded"
    })
    private void setupControlViews() {
        // Create overlay window
        final WindowManager wm = (WindowManager) getSystemService(WINDOW_SERVICE);
        final LayoutInflater inflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);

        final Rect rectangle = new Rect();
        getWindow().getDecorView().getWindowVisibleDisplayFrame(rectangle);
        final int StatusBarHeight = rectangle.top;

        RelativeLayout controls = (RelativeLayout) inflater.inflate(R.layout.controls, null, true);
        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.TYPE_APPLICATION_PANEL,
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                PixelFormat.TRANSLUCENT);
        params.y = StatusBarHeight;
        params.width = WindowManager.LayoutParams.WRAP_CONTENT;
        params.height = WindowManager.LayoutParams.WRAP_CONTENT;
        params.gravity = Gravity.RIGHT | Gravity.TOP;
        wm.addView(controls, params);

        mControlsView = controls;

        mButtonOpenMedia = (Button) controls.findViewById(R.id.buttonOpenMedia);
        mButtonPlay = (Button) controls.findViewById(R.id.buttonPlay);
        mButtonPause = (Button) controls.findViewById(R.id.buttonPause);
        mButtonAbout = (Button) controls.findViewById(R.id.buttonAbout);

        mButtonOpenMedia.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MainNativeActivity.this.onOpenMediaButtonClicked();
            }
        });
        mButtonPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MainNativeActivity.this.onPlayButtonClicked();
            }
        });
        mButtonPause.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MainNativeActivity.this.onPauseButtonClicked();
            }
        });
        mButtonAbout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MainNativeActivity.this.onAboutButtonClicked();
            }
        });
    }

    private void removeControlViews() {
        if (mControlsView != null) {
            final WindowManager wm = (WindowManager) getSystemService(WINDOW_SERVICE);
            wm.removeView(mControlsView);
            mControlsView = null;
        }

        mButtonOpenMedia = null;
        mButtonPlay = null;
        mButtonPause = null;
        mButtonAbout = null;
    }

    private void launchSongPicker(int requestCode) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("audio/*");
        Intent c = Intent.createChooser(intent, "Pick a music file");
        startActivityForResult(c, requestCode);
    }

    protected void onOpenMediaButtonClicked() {
        launchSongPicker(REQ_SONG_PICKER);
    }

    protected void onPlayButtonClicked() {
        nativePlay(mNativeHandle);
    }

    protected void onPauseButtonClicked() {
        nativePause(mNativeHandle);
    }

    protected void onAboutButtonClicked() {
        Intent intent = new Intent(this, AboutActivity.class);
        startActivity(intent);
    }

    //
    // native methods
    //
    private static native int nativeSetDataSourceAndPrepare(long handle, String path);

    private static native int nativePlay(long handle);

    private static native int nativePause(long handle);
}
