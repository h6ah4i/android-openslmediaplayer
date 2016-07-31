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


package com.h6ah4i.android.media.openslmediaplayer.testing;

public class ParameterizedTestArgs {

    private final String mTestMethodName;
    private final Object mTestParams;

    public ParameterizedTestArgs(String methodName, Object testParams) {
        mTestMethodName = methodName;
        mTestParams = testParams;
    }

    public String getTestMethodName() {
        return mTestMethodName;
    }

    public Object getTestParams() {
        return mTestParams;
    }

    public String getTestCaseName() {
        return mTestMethodName + " [" + mTestParams + "]";
    }
}
