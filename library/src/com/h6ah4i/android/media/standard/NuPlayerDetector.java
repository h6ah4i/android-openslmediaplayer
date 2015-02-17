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

package com.h6ah4i.android.media.standard;

import android.os.Build;
import android.util.Log;

import java.lang.reflect.Method;

class NuPlayerDetector {
    private NuPlayerDetector() {
    }

    public static boolean isUsingNuPlayer() {
        boolean usingNuPlayer = false;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            ClassLoader loader = NuPlayerDetector.class.getClassLoader();

            final boolean value1 = SystemPropertiesProxy.getBoolean(
                    loader, "media.stagefright.use-awesome", false);

            final boolean value2 = SystemPropertiesProxy.getBoolean(
                    loader, "persist.sys.media.use-awesome", false);

            usingNuPlayer = !(value1 || value2);
        }

        return usingNuPlayer;
    }

    private static class SystemPropertiesProxy {
        private static final String TAG = SystemPropertiesProxy.class.getSimpleName();

        public static Boolean getBoolean(ClassLoader cl, String key, boolean def)
                throws IllegalArgumentException {

            Boolean ret = def;

            try {
                @SuppressWarnings("rawtypes")
                Class SystemProperties = cl
                        .loadClass("android.os.SystemProperties");

                // Parameters Types
                @SuppressWarnings("rawtypes")
                Class[] paramTypes = new Class[2];
                paramTypes[0] = String.class;
                paramTypes[1] = boolean.class;

                @SuppressWarnings("unchecked")
                Method getBoolean = SystemProperties.getMethod("getBoolean",
                        paramTypes);

                // Parameters
                Object[] params = new Object[2];
                params[0] = new String(key);
                params[1] = Boolean.valueOf(def);

                ret = (Boolean) getBoolean.invoke(SystemProperties, params);

            } catch (IllegalArgumentException iAE) {
                throw iAE;
            } catch (Exception e) {
                Log.e(TAG, "getBoolean(context, key: " + key + ", def:" + def + ")", e);
                ret = def;
            }

            return ret;

        }
    }
}
