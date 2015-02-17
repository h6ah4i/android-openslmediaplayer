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


package com.h6ah4i.android.media.test.classtest;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import junit.framework.TestSuite;
import android.util.Log;

import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.test.base.BasicMediaPlayerTestCaseBase;
import com.h6ah4i.android.testing.ParameterizedTestArgs;
import com.h6ah4i.android.testing.ParameterizedTestSuiteBuilder;

public class BasicMediaPlayerClassTestCase
        extends BasicMediaPlayerTestCaseBase {

    private static final String TAG = "BasicMediaPlayerClassTC";

    public static TestSuite buildTestSuite(
            Class<? extends IMediaPlayerFactory> factoryClazz) {
        final List<BasicTestParams> params = new ArrayList<>();

        params.add(new BasicTestParams(factoryClazz));

        return ParameterizedTestSuiteBuilder.buildDetail(
                BasicMediaPlayerClassTestCase.class, params, null, false);
    }

    public BasicMediaPlayerClassTestCase(ParameterizedTestArgs args) {
        super(args);
    }

    //
    // Exposed test cases
    //
    public void testCreateAndReleaseMultiplePlayers() throws Throwable {
        final int N = 20;
        List<IBasicMediaPlayer> players = new ArrayList<>();

        for (int i = 0; i < N; i++) {
            try {
                players.add(getFactory().createMediaPlayer());
            } catch (IllegalStateException e) {
                // OpenSLMediaPlayer may throw IllegalStateException if
                // native resource cannot be allocated
                Log.i(TAG, "Failed to create player object (COUNT = " + players.size() + ")");
                break;
            }
        }

        for (int i = 0; i < players.size(); i++) {
            players.get(i).release();
        }
    }

    public void testCreateAndReleaseReversedOrderMultiplePlayers() throws Throwable {
        final int N = 20;
        List<IBasicMediaPlayer> players = new ArrayList<>();

        for (int i = 0; i < N; i++) {
            try {
                players.add(getFactory().createMediaPlayer());
            } catch (IllegalStateException e) {
                // OpenSLMediaPlayer may throw IllegalStateException if
                // native resource cannot be allocated
                Log.i(TAG, "Failed to create player object (COUNT = " + players.size() + ")");
                break;
            }
        }

        Collections.reverse(players);
        for (IBasicMediaPlayer player : players) {
            player.release();
        }
    }

    public void testStartMultiplePlayers() throws Throwable {
        final int N = 20;
        List<IBasicMediaPlayer> players = new ArrayList<>();

        for (int i = 0; i < N; i++) {
            try {
                players.add(createWrappedPlayerInstance());
            } catch (IllegalStateException e) {
                // OpenSLMediaPlayer may throw IllegalStateException if
                // native resource cannot be allocated
                Log.i(TAG, "Failed to create player object (COUNT = " + players.size() + ")");
                break;
            }
        }

        for (IBasicMediaPlayer player : players) {
            transitStateToPrepared(player, null);
        }

        for (IBasicMediaPlayer player : players) {
            player.start();
        }

        for (IBasicMediaPlayer player : players) {
            player.release();
        }
    }
}
