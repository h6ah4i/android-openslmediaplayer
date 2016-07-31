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

import com.h6ah4i.android.media.openslmediaplayer.classtest.EqualizerBandInfoCorrectorTestCase;

public class AllTests extends TestCase {
    public static TestSuite suite() {

        TestSuite suite = new TestSuite();

        suite.addTest(StandardMediaPlayerTest.suite());
        suite.addTest(OpenSLMediaPlayerTest.suite());
        suite.addTest(HybridMediaPlayerTest.suite());
        suite.addTestSuite(EqualizerBandInfoCorrectorTestCase.class);

        return suite;
    }
}
