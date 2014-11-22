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
import android.widget.TextView;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEventBus;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.HQEqualizerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.HQEqualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PreAmpNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PreAmpReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.HQEqualizerStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.model.PreAmpStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.ActionBarTileBuilder;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.HQEqualizerUtil;

public class HQEqualizerFragment extends AudioEffectSettingsBaseFragment
        implements AdapterView.OnItemSelectedListener,
        SeekBar.OnSeekBarChangeListener {

    // constants
    private static final int NUM_BAND_VIEWS = 10;
    private static final int SEEKBAR_MAX = 1000;

    private static class AppEventReceiver extends AppEventBus.Receiver<HQEqualizerFragment> {
        private static final int[] FILTER = new int[] {
                EventDefs.Category.NOTIFY_HQ_EQUALIZER,
                EventDefs.Category.NOTIFY_PRE_AMP,
        };

        public AppEventReceiver(HQEqualizerFragment holder) {
            super(holder, FILTER);
        }

        @Override
        protected void onReceiveAppEvent(HQEqualizerFragment holder, AppEvent event) {
            holder.onReceiveAppEvent(event);
        }
    }

    // fields
    private AppEventReceiver mAppEventReceiver;

    private Spinner mSpinnerPreset;
    private TextView[] mTextViewBandLevels = new TextView[NUM_BAND_VIEWS];
    private SeekBar[] mSeekBarBandLevels = new SeekBar[NUM_BAND_VIEWS];
    private SeekBar mSeekBarPreAmpLevel;

    @Override
    protected CharSequence onGetActionBarTitleText() {
        return ActionBarTileBuilder.makeTitleString(getActivity(), R.string.title_hq_equalizer);
    }

    @Override
    protected boolean onGetActionBarSwitchCheckedState() {
        return getAppController().getHQEqualizerStateStore().isEnabled();
    }

    @Override
    protected void onActionBarSwitchCheckedChanged(
            CompoundButton buttonView, boolean isChecked) {
        postHQEqAppEvent(HQEqualizerReqEvents.SET_ENABLED, (isChecked ? 1 : 0), 0);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_hq_equalizer, container, false);
        return rootView;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        obtainViewReferences();
        setupViews();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();

        mTextViewBandLevels = null;
        mSeekBarBandLevels = null;
        mSeekBarPreAmpLevel = null;
        mSpinnerPreset = null;
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
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (!fromUser)
            return;

        if (seekBar.getId() == R.id.seekbar_preamp) {
            // Pre amp
            float level = (float) progress / SEEKBAR_MAX;

            postPreAmpAppEvent(PreAmpReqEvents.SET_LEVEL, level, 0);
        } else {
            // Equalizer band
            short band = seekBarToBandNo(seekBar);
            float bandLevel = (float) progress / SEEKBAR_MAX;

            postHQEqAppEvent(HQEqualizerReqEvents.SET_BAND_LEVEL, band, bandLevel);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        switch (parent.getId()) {
            case R.id.spinner_equalizerb_preset:
                postHQEqAppEvent(HQEqualizerReqEvents.SET_PRESET, position, 0);
                break;
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    /* package */void onReceiveAppEvent(AppEvent event) {
        switch (event.category) {
            case EventDefs.Category.NOTIFY_HQ_EQUALIZER:
                onNotifyEqualizerEvent(event);
                break;
            case EventDefs.Category.NOTIFY_PRE_AMP:
                onNotifyPreAmpEvent(event);
                break;
        }
    }

    private void onNotifyEqualizerEvent(AppEvent event) {
        switch (event.event) {
            case HQEqualizerNotifyEvents.PRESET_UPDATED:
                updatePresetSpinner();
                break;
            case HQEqualizerNotifyEvents.BAND_LEVEL_UPDATED: {
                int band = event.arg1;

                if (band < 0) {
                    // all
                    for (int i = 0; i < HQEqualizerUtil.NUMBER_OF_BANDS; i++) {
                        updateBandLevelSeekBar(i);
                    }
                } else {
                    updateBandLevelSeekBar(band);
                }
            }
                break;
        }
    }

    private void onNotifyPreAmpEvent(AppEvent event) {
        switch (event.event) {
            case PreAmpNotifyEvents.ENABLED_STATE_UPDATED:
                break;
            case PreAmpNotifyEvents.LEVEL_UPDATED:
                updatePreAmpLevelSeekBar();
                break;
        }
    }

    private void obtainViewReferences() {
        mSpinnerPreset = findSpinnerByIdAndSetListener(R.id.spinner_equalizerb_preset);

        mTextViewBandLevels[0] = findTextViewById(R.id.textview_equalizer_band_0);
        mTextViewBandLevels[1] = findTextViewById(R.id.textview_equalizer_band_1);
        mTextViewBandLevels[2] = findTextViewById(R.id.textview_equalizer_band_2);
        mTextViewBandLevels[3] = findTextViewById(R.id.textview_equalizer_band_3);
        mTextViewBandLevels[4] = findTextViewById(R.id.textview_equalizer_band_4);
        mTextViewBandLevels[5] = findTextViewById(R.id.textview_equalizer_band_5);
        mTextViewBandLevels[6] = findTextViewById(R.id.textview_equalizer_band_6);
        mTextViewBandLevels[7] = findTextViewById(R.id.textview_equalizer_band_7);
        mTextViewBandLevels[8] = findTextViewById(R.id.textview_equalizer_band_8);
        mTextViewBandLevels[9] = findTextViewById(R.id.textview_equalizer_band_9);

        mSeekBarBandLevels[0] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_0);
        mSeekBarBandLevels[1] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_1);
        mSeekBarBandLevels[2] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_2);
        mSeekBarBandLevels[3] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_3);
        mSeekBarBandLevels[4] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_4);
        mSeekBarBandLevels[5] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_5);
        mSeekBarBandLevels[6] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_6);
        mSeekBarBandLevels[7] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_7);
        mSeekBarBandLevels[8] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_8);
        mSeekBarBandLevels[9] = findSeekBarByIdAndSetListener(R.id.seekbar_equalizer_band_9);

        mSeekBarPreAmpLevel = findSeekBarByIdAndSetListener(R.id.seekbar_preamp);
    }

    private void setupViews() {
        final Context context = getActivity();
        final int numBands = HQEqualizerUtil.NUMBER_OF_BANDS;

        for (int i = 0; i < NUM_BAND_VIEWS; i++) {
            int visibility = (i < numBands) ? View.VISIBLE : View.GONE;

            mSeekBarBandLevels[i].setVisibility(visibility);
            mTextViewBandLevels[i].setVisibility(visibility);
        }

        for (int i = 0; i < numBands; i++) {
            mTextViewBandLevels[i].setText(formatFrequencyText(
                    HQEqualizerUtil.CENTER_FREQUENCY[i]));

            mSeekBarBandLevels[i].setMax(SEEKBAR_MAX);
        }

        mSeekBarPreAmpLevel.setMax(SEEKBAR_MAX);

        {
            String[] presetNames = new String[HQEqualizerUtil.NUMBER_OF_PRESETS];

            getAppController().getHQEqualizerStateStore();
            for (int i = 0; i < presetNames.length; i++) {
                presetNames[i] = HQEqualizerUtil.PRESETS[i].name;
            }

            ArrayAdapter<String> adapter = new ArrayAdapter<String>(
                    context, android.R.layout.simple_spinner_item,
                    presetNames);

            adapter.setDropDownViewResource(
                    android.R.layout.simple_spinner_dropdown_item);

            mSpinnerPreset.setAdapter(adapter);
        }

        updatePresetSpinner();
        for (int i = 0; i < numBands; i++) {
            updateBandLevelSeekBar(i);
        }
        updatePreAmpLevelSeekBar();
    }

    private void updatePresetSpinner() {
        HQEqualizerStateStore state = getAppController().getHQEqualizerStateStore();
        int cur_preset = state.getSettings().curPreset;
        mSpinnerPreset.setSelection(cur_preset);
    }

    private void updateBandLevelSeekBar(int band) {
        HQEqualizerStateStore state = getAppController().getHQEqualizerStateStore();

        mSeekBarBandLevels[band].setProgress(
                (int) (SEEKBAR_MAX * state.getNormalizedBandLevel(band)));
    }

    private void updatePreAmpLevelSeekBar() {
        PreAmpStateStore state = getAppController().getPreAmpStateStore();

        mSeekBarPreAmpLevel.setProgress(
                (int) (SEEKBAR_MAX * state.getLevelFromUI()));
    }

    private static short seekBarToBandNo(SeekBar seekBar) {
        switch (seekBar.getId()) {
            case R.id.seekbar_equalizer_band_0:
                return 0;
            case R.id.seekbar_equalizer_band_1:
                return 1;
            case R.id.seekbar_equalizer_band_2:
                return 2;
            case R.id.seekbar_equalizer_band_3:
                return 3;
            case R.id.seekbar_equalizer_band_4:
                return 4;
            case R.id.seekbar_equalizer_band_5:
                return 5;
            case R.id.seekbar_equalizer_band_6:
                return 6;
            case R.id.seekbar_equalizer_band_7:
                return 7;
            case R.id.seekbar_equalizer_band_8:
                return 8;
            case R.id.seekbar_equalizer_band_9:
                return 9;
            default:
                throw new IllegalArgumentException();
        }
    }

    private static String formatFrequencyText(int freq_millihertz) {
        final int freq = freq_millihertz / 1000;
        if (freq < 1000) {
            return freq + " Hz";
        } else {
            return (freq / 1000) + " kHz";
        }
    }

    private void postHQEqAppEvent(int event, int arg1, float arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.HQ_EQUALIZER, event, arg1, arg2));
    }

    private void postPreAmpAppEvent(int event, float arg1, int arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.PRE_AMP, event, arg1, arg2));
    }

}
