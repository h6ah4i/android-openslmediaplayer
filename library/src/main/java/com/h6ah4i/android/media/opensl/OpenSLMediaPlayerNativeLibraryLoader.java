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

package com.h6ah4i.android.media.opensl;

import android.os.Build;
import android.util.Log;

public class OpenSLMediaPlayerNativeLibraryLoader {
    private static String TAG = "OSLMPNativeLibLoader";

    private static final boolean USE_NEON_DISABLED_LIB;

    public static boolean isSupportedAPILevel() {
        return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH);
    }

    public static boolean loadLibraries() {
        boolean hasNative = false;

        // load native library
        if (isSupportedAPILevel()) {
            try {
                if (USE_NEON_DISABLED_LIB) {
                    System.loadLibrary("OpenSLMediaPlayer-no-neon");
                } else {
                    System.loadLibrary("OpenSLMediaPlayer");
                }

                System.loadLibrary("OpenSLMediaPlayerJNI");

                hasNative = true;
            } catch (UnsatisfiedLinkError e) {
                hasNative = false;
                Log.w(TAG, "loadLibraries() Failed to load native library", e);
            } catch (Exception e) {
                hasNative = false;
                Log.e(TAG, "loadLibraries() Failed to load native library", e);
            }
        }

        return hasNative;
    }

    static {
        boolean useNeonDisabledLib = false;
        try {
            System.loadLibrary("OpenSLMediaPlayerLibLoaderHelper");
            useNeonDisabledLib = checkIsNeonDisabledLibRequired();
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library", e);
        } catch (Exception e) {
            Log.e(TAG, "Failed to load native library", e);
        }
        USE_NEON_DISABLED_LIB = useNeonDisabledLib;
    }

    private static native boolean checkIsNeonDisabledLibRequired();
}
