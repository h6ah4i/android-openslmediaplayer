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


package com.h6ah4i.android.example.openslmediaplayer.app.contents;

import java.lang.ref.WeakReference;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.ActionBar;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.ToggleButton;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEventBus;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PlayerControlNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PlayerControlReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.GlobalAppController;
import com.h6ah4i.android.example.openslmediaplayer.app.model.MediaPlayerStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.ActionBarTileBuilder;

public class PlayerControlFragment
        extends ContentsBaseFragment
        implements View.OnClickListener,
        SeekBar.OnSeekBarChangeListener,
        CompoundButton.OnCheckedChangeListener,
        AdapterView.OnItemSelectedListener {

    // constants
    private static final int REQ_SONG_PICKER_0 = 1;
    private static final int REQ_SONG_PICKER_1 = 2;

    private static final int POSITION_SEEKBAR_MAX = 10000;
    private static final int VOLUME_SEEKBAR_MAX = 1000;
    private static final int SEEKBAR_UPDATE_PERIOD = 250;
    private static final int SEEKBAR_UPDATE_STOP_DELAY = 3000;
    private static final int MSG_SEEKBAR_PERIODIC_UPDATE = 1;
    private static final int MSG_SEEKBAR_STOP_UPDATE = 2;

    // fields
    private SeekBar mSeekBarPosition;
    private SeekBar mSeekBarLeftVolume;
    private SeekBar mSeekBarRightVolume;
    private SeekBar mSeekBarAuxSendLevel;

    @SuppressWarnings("unused")
    private Button mButtonPickSong1;
    @SuppressWarnings("unused")
    private Button mButtonPickSong2;
    @SuppressWarnings("unused")
    private Button mButtonCreate;
    @SuppressWarnings("unused")
    private Button mButtonSetDataSource;
    @SuppressWarnings("unused")
    private Button mButtonPrepare;
    @SuppressWarnings("unused")
    private Button mButtonPrepareAsync;
    @SuppressWarnings("unused")
    private Button mButtonStart;
    @SuppressWarnings("unused")
    private Button mButtonPause;
    @SuppressWarnings("unused")
    private Button mButtonStop;
    @SuppressWarnings("unused")
    private Button mButtonReset;
    @SuppressWarnings("unused")
    private Button mButtonRelease;

    private ToggleButton mToggleButtonLooping;
    private Spinner mSpinnerAuxEffectType;

    private InternalHandler mHandler;
    private AppEventReceiver mAppEventReceiver;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mHandler = new InternalHandler(this);
    }

    @Override
    public void onDestroy() {
        if (mHandler != null) {
            mHandler.release();
            mHandler = null;
        }
        super.onDestroy();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_player_control, container, false);
        return rootView;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        obtainViewReferences();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        mSeekBarPosition = null;
        mSeekBarLeftVolume = null;
        mSeekBarRightVolume = null;
        mSeekBarAuxSendLevel = null;
        mButtonPickSong1 = null;
        mButtonPickSong2 = null;
        mButtonCreate = null;
        mButtonSetDataSource = null;
        mButtonPrepare = null;
        mButtonPrepareAsync = null;
        mButtonStart = null;
        mButtonPause = null;
        mButtonStop = null;
        mButtonReset = null;
        mButtonRelease = null;
        mToggleButtonLooping = null;
        mSpinnerAuxEffectType = null;
    }

    @Override
    public void onStart() {
        super.onStart();

        mAppEventReceiver = new AppEventReceiver(this);
        eventBus().register(mAppEventReceiver);
    }

    @Override
    public void onStop() {
        super.onStop();

        stopPeriodicSeekBarUpdate();

        eventBus().unregister(mAppEventReceiver);
        mAppEventReceiver = null;
    }

    @Override
    public void onResume() {
        super.onResume();

        setupViews();
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQ_SONG_PICKER_0:
                if (resultCode == Activity.RESULT_OK) {
                    postSongPicked(0, data.getData());
                }
                break;
            case REQ_SONG_PICKER_1:
                if (resultCode == Activity.RESULT_OK) {
                    postSongPicked(1, data.getData());
                }
                break;
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.button_pick_song_1:
                launchSongPicker(REQ_SONG_PICKER_0);
                break;
            case R.id.button_pick_song_2:
                launchSongPicker(REQ_SONG_PICKER_1);
                break;
            case R.id.button_player_create:
                postAppEvent(PlayerControlReqEvents.PLAYER_CREATE, 0, 0);
                break;
            case R.id.button_player_set_data_source:
                postAppEvent(PlayerControlReqEvents.PLAYER_SET_DATA_SOURCE, 0, 0);
                break;
            case R.id.button_player_prepare:
                postAppEvent(PlayerControlReqEvents.PLAYER_PREPARE, 0, 0);
                break;
            case R.id.button_player_prepare_async:
                postAppEvent(PlayerControlReqEvents.PLAYER_PREPARE_ASYNC, 0, 0);
                break;
            case R.id.button_player_start:
                postAppEvent(PlayerControlReqEvents.PLAYER_START, 0, 0);
                break;
            case R.id.button_player_pause:
                postAppEvent(PlayerControlReqEvents.PLAYER_PAUSE, 0, 0);
                break;
            case R.id.button_player_stop:
                postAppEvent(PlayerControlReqEvents.PLAYER_STOP, 0, 0);
                break;
            case R.id.button_player_reset:
                postAppEvent(PlayerControlReqEvents.PLAYER_RESET, 0, 0);
                break;
            case R.id.button_player_release:
                postAppEvent(PlayerControlReqEvents.PLAYER_RELEASE, 0, 0);
                break;
        }
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        switch (buttonView.getId()) {
            case R.id.togglebutton_player_looping:
                postAppEvent(
                        PlayerControlReqEvents.PLAYER_SET_LOOPING,
                        (isChecked ? 1 : 0), 0);
                break;
        }
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        switch (seekBar.getId()) {
            case R.id.seekbar_player_position:
                if (fromUser) {
                    float position = (float) progress / POSITION_SEEKBAR_MAX;
                    postAppEvent(
                            PlayerControlReqEvents.PLAYER_SEEK_TO,
                            0, position);
                }
                break;
            case R.id.seekbar_player_left_volume:
                if (fromUser) {
                    postAppEvent(
                            PlayerControlReqEvents.PLAYER_SET_VOLUME_LEFT,
                            0, (float) progress / VOLUME_SEEKBAR_MAX);
                }
                break;
            case R.id.seekbar_player_right_volume:
                if (fromUser) {
                    postAppEvent(
                            PlayerControlReqEvents.PLAYER_SET_VOLUME_RIGHT,
                            0, (float) progress / VOLUME_SEEKBAR_MAX);
                }
                break;
            case R.id.seekbar_player_aux_effect_send_level:
                if (fromUser) {
                    postAppEvent(
                            PlayerControlReqEvents.PLAYER_SET_AUX_SEND_LEVEL,
                            0, (float) progress / VOLUME_SEEKBAR_MAX);
                }
                break;
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        switch (seekBar.getId()) {
            case R.id.seekbar_player_position:
                stopPeriodicSeekBarUpdate();
                break;
        }
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        switch (seekBar.getId()) {
            case R.id.seekbar_player_position:
                controlPeriodicSeekBarUpdating();
                break;
        }
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        switch (parent.getId()) {
            case R.id.spinner_player_aux_effect_type:
                postAppEvent(
                        PlayerControlReqEvents.PLAYER_ATTACH_AUX_EFFECT,
                        position, 0);
                break;
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    @Override
    protected void onUpdateActionBarAndOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onUpdateActionBarAndOptionsMenu(menu, inflater);

        ActionBar actionBar = getActionBar();

        actionBar.setTitle(ActionBarTileBuilder.makeTitleString(
                getActivity(), R.string.title_player_control));
    }

    /* package */void onReceiveAppEvent(AppEvent event) {
        switch (event.category) {
            case EventDefs.Category.NOTIFY_PLAYER_CONTROL:
                onNotifyPlayerControlEvent(event);
                break;
        }
    }

    private void onNotifyPlayerControlEvent(AppEvent event) {
        switch (event.event) {
            case PlayerControlNotifyEvents.PLAYER_STATE_CHANGED:
                onPlayerStateChanged(event.arg1);
                break;
        }
    }

    private void onPlayerStateChanged(int state) {
        controlPeriodicSeekBarUpdating();
        updatePositionSeekBar();
    }

    private void obtainViewReferences() {
        mSeekBarPosition = findSeekBarByIdAndSetListener(R.id.seekbar_player_position);
        mSeekBarLeftVolume = findSeekBarByIdAndSetListener(R.id.seekbar_player_left_volume);
        mSeekBarRightVolume = findSeekBarByIdAndSetListener(R.id.seekbar_player_right_volume);
        mSeekBarAuxSendLevel = findSeekBarByIdAndSetListener(R.id.seekbar_player_aux_effect_send_level);

        mButtonPickSong1 = findButtonByIdAndSetListener(R.id.button_pick_song_1);
        mButtonPickSong2 = findButtonByIdAndSetListener(R.id.button_pick_song_2);
        mButtonCreate = findButtonByIdAndSetListener(R.id.button_player_create);
        mButtonSetDataSource = findButtonByIdAndSetListener(R.id.button_player_set_data_source);
        mButtonPrepare = findButtonByIdAndSetListener(R.id.button_player_prepare);
        mButtonPrepareAsync = findButtonByIdAndSetListener(R.id.button_player_prepare_async);
        mButtonStart = findButtonByIdAndSetListener(R.id.button_player_start);
        mButtonPause = findButtonByIdAndSetListener(R.id.button_player_pause);
        mButtonStop = findButtonByIdAndSetListener(R.id.button_player_stop);
        mButtonReset = findButtonByIdAndSetListener(R.id.button_player_reset);
        mButtonRelease = findButtonByIdAndSetListener(R.id.button_player_release);

        mToggleButtonLooping = findToggleButtonByIdAndSetListener(R.id.togglebutton_player_looping);

        mSpinnerAuxEffectType = findSpinnerByIdAndSetListener(R.id.spinner_player_aux_effect_type);
    }

    private void setupViews() {
        Context context = getActivity();
        GlobalAppController controller = getAppController();

        {
            final ArrayAdapter<CharSequence> adapter =
                    ArrayAdapter.createFromResource(
                            context, R.array.aux_effect_names,
                            android.R.layout.simple_spinner_item);
            adapter.setDropDownViewResource(
                    android.R.layout.simple_spinner_dropdown_item);
            mSpinnerAuxEffectType.setAdapter(adapter);
        }

        mSeekBarPosition.setMax(POSITION_SEEKBAR_MAX);
        mSeekBarLeftVolume.setMax(VOLUME_SEEKBAR_MAX);
        mSeekBarRightVolume.setMax(VOLUME_SEEKBAR_MAX);
        mSeekBarAuxSendLevel.setMax(VOLUME_SEEKBAR_MAX);

        MediaPlayerStateStore state = controller.getPlayerStateStore();

        mSeekBarLeftVolume.setProgress((int) (VOLUME_SEEKBAR_MAX * state.getVolumeLeft()));
        mSeekBarRightVolume.setProgress((int) (VOLUME_SEEKBAR_MAX * state.getVolumeRight()));
        mSeekBarAuxSendLevel
                .setProgress((int) (VOLUME_SEEKBAR_MAX * state.getAuxEffectSendLevel()));

        mToggleButtonLooping.setChecked(state.isLooping());
        mSpinnerAuxEffectType.setSelection(state.getSelectedAuxEffectType());
        updatePositionSeekBar();
        controlPeriodicSeekBarUpdating();
    }

    private void updatePositionSeekBar() {
        float position = getAppController().getNormalizedPlayerCurrentPosition();
        mSeekBarPosition.setProgress(
                (int) (POSITION_SEEKBAR_MAX * position));
    }

    private void postAppEvent(int command, int arg1, int arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.PLAYER_CONTROL,
                command, arg1, arg2));
    }

    private void postAppEvent(int command, int arg1, float arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.PLAYER_CONTROL,
                command, arg1, arg2));
    }

    private void postSongPicked(int no, Uri uri) {
        AppEvent eventObj = new AppEvent(
                EventDefs.Category.PLAYER_CONTROL,
                PlayerControlReqEvents.SONG_PICKED, no, 0);

        eventObj.extras = new Bundle();
        eventObj.extras.putParcelable(PlayerControlReqEvents.EXTRA_URI, uri);

        eventBus().post(eventObj);
    }

    private void launchSongPicker(int requestCode) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("audio/*");
        Intent c = Intent.createChooser(intent, "Pick a music file");
        startActivityForResult(c, requestCode);
    }

    private void controlPeriodicSeekBarUpdating() {
        int state0 = getAppController().getPlayerState(0);
        int state1 = getAppController().getPlayerState(1);

        if (state0 == PlayerControlNotifyEvents.STATE_STARTED ||
                state1 == PlayerControlNotifyEvents.STATE_STARTED) {
            startPeriodicSeekBarUpdate(true);
        } else {
            requestStopPeriodicSeekBarUpdate();
        }
    }

    private void startPeriodicSeekBarUpdate(boolean clearPendingStopRequest) {
        if (clearPendingStopRequest) {
            mHandler.removeMessages(MSG_SEEKBAR_STOP_UPDATE);
        }

        if (!mHandler.hasMessages(MSG_SEEKBAR_PERIODIC_UPDATE)) {
            mHandler.sendEmptyMessageDelayed(MSG_SEEKBAR_PERIODIC_UPDATE, SEEKBAR_UPDATE_PERIOD);
        }
    }

    private void stopPeriodicSeekBarUpdate() {
        mHandler.removeMessages(MSG_SEEKBAR_PERIODIC_UPDATE);
        mHandler.removeMessages(MSG_SEEKBAR_STOP_UPDATE);
    }

    private void requestStopPeriodicSeekBarUpdate() {
        // Stop updating after elapsed SEEKBAR_UPDATE_STOP_DELAY time
        // (current position moves after pause() called while fading out)
        if (!mHandler.hasMessages(MSG_SEEKBAR_STOP_UPDATE)) {
            mHandler.sendEmptyMessageDelayed(MSG_SEEKBAR_STOP_UPDATE, SEEKBAR_UPDATE_STOP_DELAY);
        }
    }

    private void onHandleInternalMessages(Message msg) {
        switch (msg.what) {
            case MSG_SEEKBAR_PERIODIC_UPDATE:
                updatePositionSeekBar();
                startPeriodicSeekBarUpdate(false);
                break;
            case MSG_SEEKBAR_STOP_UPDATE:
                stopPeriodicSeekBarUpdate();
                break;
        }
    }

    // internal classes
    private static class InternalHandler extends Handler {
        private WeakReference<PlayerControlFragment> mHolder;

        public InternalHandler(PlayerControlFragment holder) {
            mHolder = new WeakReference<PlayerControlFragment>(holder);
        }

        @Override
        public void handleMessage(Message msg) {
            PlayerControlFragment holder = mHolder.get();
            if (holder == null)
                return;

            holder.onHandleInternalMessages(msg);
        }

        public void release() {
            mHolder.clear();
        }
    }

    private static class AppEventReceiver extends AppEventBus.Receiver<PlayerControlFragment> {
        private static final int[] FILTER = new int[] {
                EventDefs.Category.NOTIFY_PLAYER_CONTROL,
        };

        public AppEventReceiver(PlayerControlFragment holder) {
            super(holder, FILTER);
        }

        @Override
        protected void onReceiveAppEvent(PlayerControlFragment holder, AppEvent event) {
            holder.onReceiveAppEvent(event);
        }
    }
}
