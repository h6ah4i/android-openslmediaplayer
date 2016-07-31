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

package com.h6ah4i.android.media.openslmediaplayer;

import junit.framework.TestCase;
import junit.framework.TestSuite;
import android.os.Debug;

import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerFactory;
import com.h6ah4i.android.media.openslmediaplayer.classtest.BasicMediaPlayerClassTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.BassBoostTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.EnvironmentalReverbTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.EqualizerTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.HQEqualizerTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.HQVisualizerTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.PreAmpTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.PresetReverbTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.VirtualizerTestCase;
import com.h6ah4i.android.media.openslmediaplayer.classtest.VisualizerTestCase;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_AttachAuxEffectMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_GetCurrentPositionMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_GetDurationMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_IsLoopingMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_IsPlayingMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_PauseMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_PrepareAsyncMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_PrepareMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_ReleaseMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_ResetMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SeekToMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetAudioAttributesMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetAudioStreamTypeMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetAuxEffectSendLevelMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetDataSourceFdMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetDataSourceFdOffsetLengthMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetDataSourcePathMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetDataSourceUriMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetLoopingMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetNextMediaPlayerMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetOnBufferingUpdateListenerMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetOnCompletionListenerMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetOnErrorListenerMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetOnInfoListenerMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetOnPreparedListenerMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetOnSeekCompleteListenerMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetVolumeMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_SetWakeModeMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_StartMethod;
import com.h6ah4i.android.media.openslmediaplayer.methodtest.BasicMediaPlayerTestCase_StopMethod;
import com.h6ah4i.android.media.openslmediaplayer.utils.BasicMediaPlayerTestCase_CleanupDummyTestCase;

public class OpenSLMediaPlayerTest extends TestCase {
    public static TestSuite suite() {
        final Class<? extends IMediaPlayerFactory> factory = OpenSLMediaPlayerFactory.class;

        Debug.waitForDebugger();

        TestSuite suite = new TestSuite();

        //
        // Clean up
        //
        suite.addTest(BasicMediaPlayerTestCase_CleanupDummyTestCase.buildTestSuite(factory));

        //
        // BasicMediaPlayer
        //
        suite.addTest(BasicMediaPlayerClassTestCase.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetDataSourcePathMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetDataSourceUriMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetDataSourceFdMethod.buildTestSuite(factory));
        suite.addTest(
                BasicMediaPlayerTestCase_SetDataSourceFdOffsetLengthMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_PrepareMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_PrepareAsyncMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_StartMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_PauseMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_StopMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_ResetMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_ReleaseMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SeekToMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_GetDurationMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_GetCurrentPositionMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetLoopingMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_IsLoopingMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetVolumeMethod.buildTestSuite(factory));
        /* NOTE: This method is not supported */
        // suite.addTest(BasicMediaPlayerTestCase_SetAudioSessionIdMethod.buildTestSuite(factory));
        /* NOTE: This method is not supported */
        // suite.addTest(BasicMediaPlayerTestCase_GetAudioSessionIdMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_IsPlayingMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_AttachAuxEffectMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetAudioStreamTypeMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetAuxEffectSendLevelMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetWakeModeMethod.buildTestSuite(factory));
        suite.addTest(
                BasicMediaPlayerTestCase_SetOnCompletionListenerMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetOnPreparedListenerMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetOnErrorListenerMethod.buildTestSuite(factory));
        suite.addTest(
                BasicMediaPlayerTestCase_SetOnBufferingUpdateListenerMethod.buildTestSuite(factory));
        suite.addTest(
                BasicMediaPlayerTestCase_SetOnSeekCompleteListenerMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetOnInfoListenerMethod.buildTestSuite(factory));

        suite.addTest(
                BasicMediaPlayerTestCase_SetNextMediaPlayerMethod.buildTestSuite(factory));
        suite.addTest(BasicMediaPlayerTestCase_SetAudioAttributesMethod.buildTestSuite(factory));

        //
        // Audio Effects
        //
        suite.addTest(BassBoostTestCase.buildTestSuite(factory));
        suite.addTest(VirtualizerTestCase.buildTestSuite(factory));
        suite.addTest(EqualizerTestCase.buildTestSuite(factory));
        /* NOTE: This method is not supported */
        // suite.addTest(LoudnessEnhancerTestCase.buildTestSuite(factory));
        suite.addTest(PresetReverbTestCase.buildTestSuite(factory));
        suite.addTest(EnvironmentalReverbTestCase.buildTestSuite(factory));
        suite.addTest(VisualizerTestCase.buildTestSuite(factory));

        //
        // Special tests for OpenSLMediaPlayer
        //
        suite.addTest(HQEqualizerTestCase.buildTestSuite(factory));
        suite.addTest(PreAmpTestCase.buildTestSuite(factory));
        suite.addTest(HQVisualizerTestCase.buildTestSuite(factory));

        return suite;
    }
}
