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

package com.h6ah4i.android.example.openslmediaplayer.app;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.ActionBarActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;
import android.view.ViewGroup;
import android.widget.Toast;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.AboutFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.BassBoostFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.EnvironmentalReverbFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.EqualizerFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.HQEqualizerFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.HQVisualizerFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.LoudnessEnhancerFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.PlayerControlFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.PresetReverbFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.VirtualizerFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.contents.VisualizerFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEventBus;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PlayerControlNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.GlobalAppController;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.GlobalAppControllerAccessor;

public class MainActivity extends ActionBarActivity {

    private static final String FRAGMENT_TAG_NAVIGATION_DRAWER = "NavigationDrawer";

    // fields
    private NavigationDrawerFragment mNavigationDrawerFragment;
    private AppEventReceiver mAppEventReceiver;
    private Toast mToast;
    private Runnable mDecorViewInitialized;

    // internal classes
    private static class AppEventReceiver extends AppEventBus.Receiver<MainActivity> {
        private static final int[] FILTER = new int[] {
                EventDefs.Category.NAVIGATION_DRAWER,
                EventDefs.Category.NOTIFY_PLAYER_CONTROL,
        };

        public AppEventReceiver(MainActivity holder) {
            super(holder, FILTER);
        }

        @Override
        protected void onReceiveAppEvent(MainActivity holder, AppEvent event) {
            holder.onReceiveAppEvent(event);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // setup event bus
        mAppEventReceiver = new AppEventReceiver(this);
        eventBus().register(mAppEventReceiver);

        // set content view
        setContentView(R.layout.activity_main);

        // set ToolBar as a ActionBar
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        toolbar.setTitleTextColor(getResources().getColor(R.color.app_toolbar_title_text));
        setSupportActionBar(toolbar);

        // instantiate NavigationDrawerFragment
        FragmentManager fm = getSupportFragmentManager();

        NavigationDrawerFragment fragment = (NavigationDrawerFragment)
                fm.findFragmentByTag(FRAGMENT_TAG_NAVIGATION_DRAWER);

        if (fragment == null) {
            fragment = (NavigationDrawerFragment)
                    Fragment.instantiate(this, NavigationDrawerFragment.class.getName());
            fm.beginTransaction()
                    .replace(R.id.drawer_container, fragment, FRAGMENT_TAG_NAVIGATION_DRAWER)
                    .commit();
        } else {
            fm.beginTransaction().attach(fragment).commit();
        }

        mNavigationDrawerFragment = fragment;

        // set initial contents
        if (savedInstanceState == null) {
            switchContents(NavigationDrawerReqEvents.SECTION_INDEX_PLAYER_CONTROL);
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
    }

    @Override
    protected void onResume() {
        super.onResume();

        // schedule onDecorViewInitialized() event
        mDecorViewInitialized = new Runnable() {
            @Override
            public void run() {
                onDecorViewInitialized();
            }
        };

        getWindow().getDecorView().post(mDecorViewInitialized);
    }

    @Override
    protected void onPause() {
        if (mDecorViewInitialized != null) {
            getWindow().getDecorView().removeCallbacks(mDecorViewInitialized);
            mDecorViewInitialized = null;
        }

        if (mToast != null) {
            mToast.cancel();
            mToast = null;
        }
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        eventBus().unregister(mAppEventReceiver);
        mAppEventReceiver = null;
        super.onDestroy();
    }

    private void onDecorViewInitialized() {
        // measure status bar & navigation bar height
        final ViewGroup contents = (ViewGroup) findViewById(R.id.activity_contents);
        final int statusBarOffset = contents.getPaddingTop();
        final int navBarOffset = contents.getPaddingBottom();

        // apply to navigation drawer
        mNavigationDrawerFragment.setSystemBarsOffset(statusBarOffset, navBarOffset);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return super.onOptionsItemSelected(item);
    }

    /* package */void onReceiveAppEvent(AppEvent event) {
        switch (event.category) {
            case EventDefs.Category.NAVIGATION_DRAWER:
                onNavigationDrawerEvent(event);
                break;
            case EventDefs.Category.NOTIFY_PLAYER_CONTROL:
                onNotifyPlayerControlEvent(event);
                break;
        }
    }

    private void onNavigationDrawerEvent(AppEvent event) {
        switch (event.event) {
            case NavigationDrawerReqEvents.SELECT_PAGE:
                switchContents(event.arg1);
                break;
        }
    }

    private void onNotifyPlayerControlEvent(AppEvent event) {
        switch (event.event) {
            case PlayerControlNotifyEvents.PLAYER_STATE_CHANGED: {
                int state0 = getAppController().getPlayerState(0);
                int state1 = getAppController().getPlayerState(1);
                int active = getAppController().getActivePlayerIndex();

                StringBuilder sb = new StringBuilder();

                sb.append((active == 0) ? "* " : "  ");
                sb.append("Player[0] state: ");
                sb.append(formatPlayerState(state0));
                sb.append("\n");

                sb.append((active == 1) ? "* " : "  ");
                sb.append("Player[1] state: ");
                sb.append(formatPlayerState(state1));

                showToastMessage(sb.toString());
            }
                break;
            case PlayerControlNotifyEvents.NOTIFY_PLAYER_INFO: {
                int playerIndex = event.arg1;
                int what = event.extras.getInt(PlayerControlNotifyEvents.EXTRA_ERROR_INFO_WHAT);
                int extra = event.extras.getInt(PlayerControlNotifyEvents.EXTRA_ERROR_INFO_EXTRA);

                showToastMessage("Player[" + playerIndex + "] INFO: \n"
                        + "what = " + what + ", extra = " + extra);
            }
                break;
            case PlayerControlNotifyEvents.NOTIFY_PLAYER_ERROR: {
                int playerIndex = event.arg1;
                int what = event.extras.getInt(PlayerControlNotifyEvents.EXTRA_ERROR_INFO_WHAT);
                int extra = event.extras.getInt(PlayerControlNotifyEvents.EXTRA_ERROR_INFO_EXTRA);

                showToastMessage("Player[" + playerIndex + "] ERROR: \n"
                        + "what = " + what + ", extra = " + extra);
            }
                break;
            case PlayerControlNotifyEvents.NOTIFY_EXCEPTION_OCCURRED: {
                int playerIndex = event.arg1;
                String method = event.extras.getString(
                        PlayerControlNotifyEvents.EXTRA_EXEC_OPERATION_NAME);
                String exception = event.extras.getString(
                        PlayerControlNotifyEvents.EXTRA_EXCEPTION_NAME);

                showToastMessage("Player[" + playerIndex + "] Exception: \n" +
                        "method = " + method + ", exception = " + exception);
            }
                break;
        }
    }

    private void switchContents(int section) {
        FragmentManager fragmentManager = getSupportFragmentManager();
        Fragment contents;

        switch (section) {
            case NavigationDrawerReqEvents.SECTION_INDEX_PLAYER_CONTROL:
                contents = new PlayerControlFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_BASSBOOST:
                contents = new BassBoostFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_VIRTUALIZER:
                contents = new VirtualizerFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_EQUALIZER:
                contents = new EqualizerFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_LOUDNESS_ENHANCER:
                contents = new LoudnessEnhancerFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_PRESET_REVERB:
                contents = new PresetReverbFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_ENVIRONMENTAL_REVERB:
                contents = new EnvironmentalReverbFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_VISUALIZER:
                contents = new VisualizerFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_HQ_EQUALIZER:
                contents = new HQEqualizerFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_HQ_VISUALIZER:
                contents = new HQVisualizerFragment();
                break;
            case NavigationDrawerReqEvents.SECTION_INDEX_ABOUT:
                contents = new AboutFragment();
                break;
            default:
                throw new IllegalStateException();
        }

        fragmentManager.beginTransaction()
                .replace(R.id.container, contents)
                .commitAllowingStateLoss();
    }

    private void showToastMessage(String text) {
        if (mToast != null) {
            mToast.setText(text);
        } else {
            mToast = Toast.makeText(getApplicationContext(), text, Toast.LENGTH_LONG);
        }
        mToast.show();
    }

    public boolean isNavigationDrawerOpen() {
        return mNavigationDrawerFragment.isDrawerOpen();
    }

    public GlobalAppController getAppController() {
        return GlobalAppControllerAccessor.getInstance(this);
    }

    public AppEventBus eventBus() {
        return getAppController().eventBus();
    }

    private static String formatPlayerState(int state) {
        switch (state) {
            case PlayerControlNotifyEvents.STATE_IDLE:
                return "IDLE";
            case PlayerControlNotifyEvents.STATE_INITIALIZED:
                return "INITIALIZED";
            case PlayerControlNotifyEvents.STATE_PREPARING:
                return "PREPARING";
            case PlayerControlNotifyEvents.STATE_PREPARED:
                return "PREPARED";
            case PlayerControlNotifyEvents.STATE_STARTED:
                return "STARTED";
            case PlayerControlNotifyEvents.STATE_PAUSED:
                return "PAUSED";
            case PlayerControlNotifyEvents.STATE_STOPPED:
                return "STOPPED";
            case PlayerControlNotifyEvents.STATE_PLAYBACK_COMPLETED:
                return "PLAYBACK_COMPLETED";
            case PlayerControlNotifyEvents.STATE_END:
                return "END";
            case PlayerControlNotifyEvents.STATE_ERROR:
                return "ERROR";
            default:
                return "Unknown (" + state + ")";
        }
    }
}
