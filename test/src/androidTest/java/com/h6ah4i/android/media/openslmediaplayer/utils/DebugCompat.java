package com.h6ah4i.android.media.openslmediaplayer.utils;

import android.os.Build;

public class DebugCompat {
    static abstract class Impl {
        public abstract long getPss();
    }

    private static final Impl IMPL;

    static {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            IMPL = new DebugCompatImplICS();
        } else {
            IMPL = new DebugCompatImplGB();
        }
    }

    public static long getPss() {
        return IMPL.getPss();
    }
}
