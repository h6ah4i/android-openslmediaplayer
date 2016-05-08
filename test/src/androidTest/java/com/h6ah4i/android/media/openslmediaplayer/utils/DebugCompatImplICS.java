package com.h6ah4i.android.media.openslmediaplayer.utils;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.Debug;

class DebugCompatImplICS extends DebugCompat.Impl {
    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
    @Override
    public long getPss() {
        return Debug.getPss();
    }
}
