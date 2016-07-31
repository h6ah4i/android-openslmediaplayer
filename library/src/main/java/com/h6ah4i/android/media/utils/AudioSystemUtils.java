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


package com.h6ah4i.android.media.utils;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Build;

public class AudioSystemUtils {

    /* Copied from AudioManager */
    private static final String PROPERTY_OUTPUT_SAMPLE_RATE =
            "android.media.property.OUTPUT_SAMPLE_RATE";
    private static final String PROPERTY_OUTPUT_FRAMES_PER_BUFFER =
            "android.media.property.OUTPUT_FRAMES_PER_BUFFER";

    private AudioSystemUtils() {
    }

    public static class AudioSystemProperties {
        public int outputSampleRate;
        public int outputFramesPerBuffer;
        public boolean supportLowLatency;
        public boolean supportFloatingPoint;
    }

    public static AudioSystemProperties getProperties(Context context) {
        AudioSystemProperties props = new AudioSystemProperties();

        // set default value
        props.outputSampleRate = 44100;
        props.outputFramesPerBuffer = 512;
        props.supportLowLatency = false;
        props.supportFloatingPoint = false;

        // collect information
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            getPropertiesJbMr1(context, props);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            props.supportLowLatency = checkLowLatencySupport(context);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            props.supportFloatingPoint = true;
        }

        return props;
    }

    private static boolean getPropertiesJbMr1(Context context, AudioSystemProperties props) {
        if (context == null) {
            return false;
        }

        final AudioManager am = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        try {
            final Method methodGetProperty =
                    AudioManager.class.getMethod("getProperty", String.class);
            final String strSampleRate = (String) methodGetProperty.invoke(
                    am, PROPERTY_OUTPUT_SAMPLE_RATE);
            final String strFramesPerBuffer = (String) methodGetProperty.invoke(
                    am, PROPERTY_OUTPUT_FRAMES_PER_BUFFER);

            final int sampleRate = Integer.parseInt(strSampleRate);
            final int framesPerBuffer = Integer.parseInt(strFramesPerBuffer);

            if (sampleRate > 0 && framesPerBuffer > 0) {
                props.outputSampleRate = sampleRate;
                props.outputFramesPerBuffer = framesPerBuffer;

                return true;
            } else {
                return false;
            }
        } catch (NoSuchMethodException e) {
        } catch (IllegalAccessException e) {
        } catch (IllegalArgumentException e) {
        } catch (InvocationTargetException e) {
        }

        return false;
    }

    private static boolean checkLowLatencySupport(Context context) {
        if (context == null) {
            return false;
        }

        PackageManager pm = context.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_AUDIO_LOW_LATENCY);
    }
}
