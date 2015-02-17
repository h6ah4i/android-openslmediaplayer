package com.h6ah4i.android.media.test.utils;

import android.os.Debug;

class DebugCompatImplGB extends DebugCompat.Impl {
    @Override
    public long getPss() {
        Debug.MemoryInfo mi = new Debug.MemoryInfo();
        Debug.getMemoryInfo(mi);
        return mi.getTotalPss();
    }
}
