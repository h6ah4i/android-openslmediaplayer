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

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.SeekBar;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.LoudnessEnhancerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.VirtualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.LoudnessEnhancerStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.ActionBarTileBuilder;

public class LoudnessEnhancerFragment extends AudioEffectSettingsBaseFragment
        implements SeekBar.OnSeekBarChangeListener {

    // constants
    private static final int PARAM_SEEKBAR_MAX = 1000;

    // fields
    private SeekBar mSeekBarTargetGain;

    @Override
    protected CharSequence onGetActionBarTitleText() {
        return ActionBarTileBuilder
                .makeTitleString(getActivity(), R.string.title_loudness_enhancer);
    }

    @Override
    protected boolean onGetActionBarSwitchCheckedState() {
        return getAppController().getLoudnessEnhancerStateStore().isEnabled();
    }

    @Override
    protected void onActionBarSwitchCheckedChanged(
            CompoundButton buttonView, boolean isChecked) {
        postAppEvent(LoudnessEnhancerReqEvents.SET_ENABLED, (isChecked ? 1 : 0), 0);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_loudness_enhancer, container, false);
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

        mSeekBarTargetGain = null;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        switch (seekBar.getId()) {
            case R.id.seekbar_loudness_enhancer_target_gain: {
                float strength = (float) progress / PARAM_SEEKBAR_MAX;
                postAppEvent(VirtualizerReqEvents.SET_STRENGTH, 0, strength);
            }
                break;
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
    }

    private void obtainViewReferences() {
        mSeekBarTargetGain = findSeekBarByIdAndSetListener(R.id.seekbar_loudness_enhancer_target_gain);
    }

    private void setupViews() {
        LoudnessEnhancerStateStore state = getAppController().getLoudnessEnhancerStateStore();

        mSeekBarTargetGain.setMax(PARAM_SEEKBAR_MAX);

        mSeekBarTargetGain.setProgress(
                (int) (PARAM_SEEKBAR_MAX * state.getNormalizedTargetGainmB()));
    }

    private void postAppEvent(int event, int arg1, float arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.LOUDNESS_EHNAHCER, event, arg1, arg2));
    }

}
