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

import java.util.ArrayList;

import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.Spinner;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEventBus;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EnvironmentalReverbStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.EnvironmentalReverbNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.EnvironmentalReverbReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.ActionBarTileBuilder;

public class EnvironmentalReverbFragment
        extends AudioEffectSettingsBaseFragment
        implements AdapterView.OnItemSelectedListener,
        SeekBar.OnSeekBarChangeListener {

    // constants
    static final int SEEKBAR_MAX = 10000;

    // internal classes
    private static class AppEventReceiver extends AppEventBus.Receiver<EnvironmentalReverbFragment> {
        private static final int[] FILTER = new int[] {
                EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB,
        };

        public AppEventReceiver(EnvironmentalReverbFragment holder) {
            super(holder, FILTER);
        }

        @Override
        protected void onReceiveAppEvent(EnvironmentalReverbFragment holder, AppEvent event) {
            holder.onReceiveAppEvent(event);
        }
    }

    // fields
    private AppEventReceiver mAppEventReceiver;

    private SeekBar mSeekBarDecayHFRatio;
    private SeekBar mSeekBarDecayTime;
    private SeekBar mSeekBarDensity;
    private SeekBar mSeekBarDiffusion;
    private SeekBar mSeekBarReflectionsDelay;
    private SeekBar mSeekBarReflectionsLevel;
    private SeekBar mSeekBarReverbDelay;
    private SeekBar mSeekBarReverbLevel;
    private SeekBar mSeekBarRoomHFLevel;
    private SeekBar mSeekBarRoomLevel;
    private Spinner mSpinnerPreset;
    private ArrayList<SeekBar> mParamSeekBars;
    private int mActiveTrackingSeekBarIndex = -1;

    @Override
    protected CharSequence onGetActionBarTitleText() {
        return ActionBarTileBuilder.makeTitleString(getActivity(),
                R.string.title_environmental_reverb);
    }

    @Override
    protected boolean onGetActionBarSwitchCheckedState() {
        return getAppController().getEnvironmentalReverbStateStore().isEnabled();
    }

    @Override
    protected void onActionBarSwitchCheckedChanged(
            CompoundButton buttonView, boolean isChecked) {
        postAppEvent(EnvironmentalReverbReqEvents.SET_ENABLED, (isChecked ? 1 : 0), 0);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_environment_reverb, container, false);
        return rootView;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        obtainViewReferences();
        setupViews();
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

        eventBus().unregister(mAppEventReceiver);
        mAppEventReceiver = null;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        mSeekBarDecayHFRatio = null;
        mSeekBarDecayTime = null;
        mSeekBarDensity = null;
        mSeekBarDiffusion = null;
        mSeekBarReflectionsDelay = null;
        mSeekBarReflectionsLevel = null;
        mSeekBarReverbDelay = null;
        mSeekBarReverbLevel = null;
        mSeekBarRoomHFLevel = null;
        mSeekBarRoomLevel = null;
        mSpinnerPreset = null;

        mParamSeekBars.clear();
        mParamSeekBars = null;
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        switch (parent.getId()) {
            case R.id.spinner_environment_reverb_preset:
                postAppEvent(
                        EnvironmentalReverbReqEvents.SET_PRESET,
                        (position - 1), 0);
                break;
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser) {
            int paramIndex = mParamSeekBars.indexOf(seekBar);

            postAppEvent(
                    EnvironmentalReverbReqEvents.SET_PARAMETER,
                    paramIndex, seekBarProgressToFloat(progress));
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        mActiveTrackingSeekBarIndex = mParamSeekBars.indexOf(seekBar);
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        mActiveTrackingSeekBarIndex = -1;
    }

    /* package */void onReceiveAppEvent(AppEvent event) {
        switch (event.category) {
            case EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB:
                onReceiveEnvironmentalReverbNotifyEvents(event);
                break;
        }
    }

    private void onReceiveEnvironmentalReverbNotifyEvents(AppEvent event) {
        switch (event.event) {
            case EnvironmentalReverbNotifyEvents.PRESET_UPDATED:
                // preset updated
                if (mSpinnerPreset != null) {
                    updatePresetSpinner();
                }
                break;
            case EnvironmentalReverbNotifyEvents.PARAMETER_UPDATED: {
                // parameter updated
                final int paramIndex = event.arg1;
                final boolean nowTracking = (mActiveTrackingSeekBarIndex == paramIndex);

                if (mParamSeekBars != null && !nowTracking) {
                    final EnvironmentalReverbStateStore state = getEnvironmentalReverbState();

                    if (paramIndex == EnvironmentalReverbReqEvents.PARAM_INDEX_ALL) {
                        for (int i = 0; i < mParamSeekBars.size(); i++) {
                            final float value = state.getNormalizedParameter(i);
                            mParamSeekBars.get(i).setProgress(floatToSeekbarProgress(value));
                        }
                    } else {
                        final float value = state.getNormalizedParameter(paramIndex);
                        mParamSeekBars.get(paramIndex).setProgress(
                                floatToSeekbarProgress(value));
                    }
                }
            }
                break;
        }
    }

    private void obtainViewReferences() {
        mSeekBarDecayHFRatio =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_decay_hf_ratio);
        mSeekBarDecayTime =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_decay_time);
        mSeekBarDensity =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_density);
        mSeekBarDiffusion =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_diffusion);
        mSeekBarReflectionsDelay =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_reflections_delay);
        mSeekBarReflectionsLevel =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_reflections_level);
        mSeekBarReverbDelay =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_reverb_delay);
        mSeekBarReverbLevel =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_reverb_level);
        mSeekBarRoomHFLevel =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_room_hf_level);
        mSeekBarRoomLevel =
                findSeekBarByIdAndSetListener(R.id.seekbar_environment_reverb_room_level);

        mSpinnerPreset = findSpinnerByIdAndSetListener(R.id.spinner_environment_reverb_preset);
    }

    private void setupViews() {
        final Context context = getActivity();

        mParamSeekBars = new ArrayList<SeekBar>();

        mParamSeekBars.add(mSeekBarDecayHFRatio);
        mParamSeekBars.add(mSeekBarDecayTime);
        mParamSeekBars.add(mSeekBarDensity);
        mParamSeekBars.add(mSeekBarDiffusion);
        mParamSeekBars.add(mSeekBarReflectionsDelay);
        mParamSeekBars.add(mSeekBarReflectionsLevel);
        mParamSeekBars.add(mSeekBarReverbDelay);
        mParamSeekBars.add(mSeekBarReverbLevel);
        mParamSeekBars.add(mSeekBarRoomHFLevel);
        mParamSeekBars.add(mSeekBarRoomLevel);

        for (SeekBar seekbar : mParamSeekBars) {
            seekbar.setMax(SEEKBAR_MAX);
        }

        // disable unsupported parameter seekbars
        // (these parameters are not supported in current Android)
        mSeekBarReflectionsLevel.setEnabled(false);
        mSeekBarReflectionsDelay.setEnabled(false);
        mSeekBarReverbDelay.setEnabled(false);

        {
            final ArrayAdapter<CharSequence> adapter =
                    ArrayAdapter.createFromResource(
                            context, R.array.aux_env_reverb_preset_names,
                            android.R.layout.simple_spinner_item);

            adapter.setDropDownViewResource(
                    android.R.layout.simple_spinner_dropdown_item);
            mSpinnerPreset.setAdapter(adapter);
        }

        updatePresetSpinner();
    }

    private void updatePresetSpinner() {
        mSpinnerPreset.setSelection(getEnvironmentalReverbState().getPreset() + 1);
    }

    private EnvironmentalReverbStateStore getEnvironmentalReverbState() {
        return getAppController().getEnvironmentalReverbStateStore();
    }

    private void postAppEvent(int event, int arg1, int arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.ENVIRONMENTAL_REVERB, event, arg1, arg2));
    }

    private void postAppEvent(int event, int arg1, float arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.ENVIRONMENTAL_REVERB, event, arg1, arg2));
    }

    private static float seekBarProgressToFloat(int progress) {
        return progress * (1.0f / SEEKBAR_MAX);
    }

    private static int floatToSeekbarProgress(float value) {
        return (int) (value * SEEKBAR_MAX);
    }
}
