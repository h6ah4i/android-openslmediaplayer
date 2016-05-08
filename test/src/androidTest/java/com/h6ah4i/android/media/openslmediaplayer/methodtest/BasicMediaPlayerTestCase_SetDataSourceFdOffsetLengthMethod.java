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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import junit.framework.TestSuite;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.openslmediaplayer.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.media.openslmediaplayer.testing.ParameterizedTestArgs;

public class BasicMediaPlayerTestCase_SetDataSourceFdOffsetLengthMethod
        extends BasicMediaPlayerTestCase_SetDataSourceMethodBase {

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        return BasicMediaPlayerTestCaseBase.buildBasicTestSuite(
                BasicMediaPlayerTestCase_SetDataSourceFdOffsetLengthMethod.class, factoryClazz);
    }

    public BasicMediaPlayerTestCase_SetDataSourceFdOffsetLengthMethod(ParameterizedTestArgs args) {
        super(args);
    }

    private FileInputStream mInputStream;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mInputStream = new FileInputStream(
                new File(getStorageFilePath(LOCAL_440HZ_STEREO_MP3)));
    }

    @Override
    protected void tearDown() throws Exception {
        if (mInputStream != null) {
            closeQuietly(mInputStream);
            mInputStream = null;
        }
        super.tearDown();
    }

    @Override
    protected void onSetDataSource(IBasicMediaPlayer player, Object args) throws IOException {
        player.setDataSource(mInputStream.getFD(), 0, mInputStream.available());
    }
}
