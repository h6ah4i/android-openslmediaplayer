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


package com.h6ah4i.android.media.openslmediaplayer.methodtest;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerTestCase_SetDataSourceUriMethod
        extends BasicMediaPlayerTestCase_SetDataSourceMethodBase {

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        List<TestParams> params = new ArrayList<TestParams>();

        params.add(new TestParams(factoryClazz, LOCAL_440HZ_STEREO_MP3));
        params.add(new TestParams(factoryClazz, LOCAL_440HZ_STEREO_MP3_MULTI_BYTE_NAME));

        return ParameterizedTestSuiteBuilder.build(
                BasicMediaPlayerTestCase_SetDataSourceUriMethod.class, params);
    }

    private static final class TestParams extends BasicTestParams {
        private final String mFileName;

        public TestParams(
                Class<? extends IMediaPlayerFactory> factoryClass,
                String fileName) {
            super(factoryClass);
            mFileName = fileName;
        }

        public String getFileName() {
            return mFileName;
        }

        @Override
        public String toString() {
            return super.toString() + ", " + mFileName;
        }
    }

    public BasicMediaPlayerTestCase_SetDataSourceUriMethod(ParameterizedTestArgs args) {
        super(args);
    }

    @Override
    protected void onSetDataSource(IBasicMediaPlayer player, Object args) throws IOException {
        TestParams params = (TestParams) args;

        player.setDataSource(
                getContext(),
                getStrageFileUri(params.getFileName()));
    }
}
