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
import android.widget.Spinner;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PresetReverbReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.PresetReverbStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.ActionBarTileBuilder;

public class PresetReverbFragment extends AudioEffectSettingsBaseFragment
        implements AdapterView.OnItemSelectedListener {

    // fields
    private Spinner mSpinnerPreset;

    @Override
    protected CharSequence onGetActionBarTitleText() {
        return ActionBarTileBuilder.makeTitleString(getActivity(), R.string.title_preset_reverb);
    }

    @Override
    protected boolean onGetActionBarSwitchCheckedState() {
        return getAppController().getPresetReverbStateStore().isEnabled();
    }

    @Override
    protected void onActionBarSwitchCheckedChanged(
            CompoundButton buttonView, boolean isChecked) {
        postAppEvent(PresetReverbReqEvents.SET_ENABLED, (isChecked ? 1 : 0), 0);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_preset_reverb, container, false);
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

        mSpinnerPreset = null;
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        switch (parent.getId()) {
            case R.id.spinner_preset_reverb_preset:
                postAppEvent(PresetReverbReqEvents.SET_PRESET, position, 0);
                break;
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    private void obtainViewReferences() {
        mSpinnerPreset = findSpinnerByIdAndSetListener(R.id.spinner_preset_reverb_preset);
    }

    private void setupViews() {
        Context context = getActivity();
        PresetReverbStateStore state = getAppController().getPresetReverbStateStore();

        {
            final ArrayAdapter<CharSequence> adapter =
                    ArrayAdapter.createFromResource(
                            context, R.array.aux_preset_reverb_preset_names,
                            android.R.layout.simple_spinner_item);

            adapter.setDropDownViewResource(
                    android.R.layout.simple_spinner_dropdown_item);
            mSpinnerPreset.setAdapter(adapter);
        }

        mSpinnerPreset.setSelection(state.getSettings().preset);
    }

    private void postAppEvent(int event, int arg1, int arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.PRESET_REVERB, event, arg1, arg2));
    }
}
