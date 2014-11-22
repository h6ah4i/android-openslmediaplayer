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
import com.h6ah4i.android.example.openslmediaplayer.app.model.BassBoostStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.BassBoostReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.ActionBarTileBuilder;

public class BassBoostFragment extends AudioEffectSettingsBaseFragment
        implements SeekBar.OnSeekBarChangeListener {

    // constants
    private static final int PARAM_SEEKBAR_MAX = 1000;

    // fields
    private SeekBar mSeekBarStrength;

    @Override
    protected CharSequence onGetActionBarTitleText() {
        return ActionBarTileBuilder.makeTitleString(getActivity(), R.string.title_bassboost);
    }

    @Override
    protected boolean onGetActionBarSwitchCheckedState() {
        return getAppController().getBassBoostStateStore().isEnabled();
    }

    @Override
    protected void onActionBarSwitchCheckedChanged(
            CompoundButton buttonView, boolean isChecked) {
        postAppEvent(BassBoostReqEvents.SET_ENABLED, (isChecked ? 1 : 0), 0);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_bassboost, container, false);
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

        mSeekBarStrength = null;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        switch (seekBar.getId()) {
            case R.id.seekbar_bassboost_strength: {
                float strength = (float) progress / PARAM_SEEKBAR_MAX;
                postAppEvent(BassBoostReqEvents.SET_STRENGTH, 0, strength);
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
        mSeekBarStrength = findSeekBarByIdAndSetListener(R.id.seekbar_bassboost_strength);
    }

    private void setupViews() {
        BassBoostStateStore state = getAppController().getBassBoostStateStore();

        mSeekBarStrength.setMax(PARAM_SEEKBAR_MAX);

        mSeekBarStrength.setProgress(
                (int) (PARAM_SEEKBAR_MAX * state.getNormalizedStrength()));
    }

    private void postAppEvent(int event, int arg1, float arg2) {
        eventBus().post(new AppEvent(EventDefs.Category.BASSBOOST, event, arg1, arg2));
    }
}
