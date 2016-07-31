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
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.h6ah4i.android.example.openslmediaplayer.app.AppBaseFragment;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.OnItemSelectedListenerWrapper;

public class ContentsBaseFragment extends AppBaseFragment {

    protected View findViewById(int id) {
        return getView().findViewById(id);
    }

    protected TextView findTextViewById(int id) {
        return (TextView) findViewById(id);
    }

    protected SeekBar findSeekBarById(int id) {
        return (SeekBar) findViewById(id);
    }

    protected Button findButtonById(int id) {
        return (Button) findViewById(id);
    }

    protected Spinner findSpinnerById(int id) {
        return (Spinner) findViewById(id);
    }

    protected ToggleButton findToggleButtonById(int id) {
        return (ToggleButton) findViewById(id);
    }

    protected SeekBar findSeekBarByIdAndSetListener(int id) {
        SeekBar seekBar = findSeekBarById(id);
        seekBar.setOnSeekBarChangeListener((SeekBar.OnSeekBarChangeListener) this);
        return seekBar;
    }

    protected Button findButtonByIdAndSetListener(int id) {
        Button button = findButtonById(id);
        button.setOnClickListener((View.OnClickListener) this);
        return button;
    }

    protected ToggleButton findToggleButtonByIdAndSetListener(int id) {
        ToggleButton toggleButtton = findToggleButtonById(id);
        toggleButtton.setOnCheckedChangeListener((CompoundButton.OnCheckedChangeListener) this);
        return toggleButtton;
    }

    protected Spinner findSpinnerByIdAndSetListener(int id) {
        Spinner spinner = findSpinnerById(id);
        spinner.setOnItemSelectedListener(new OnItemSelectedListenerWrapper(
                (AdapterView.OnItemSelectedListener) this));
        return spinner;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setHasOptionsMenu(true);
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);

        if (!isNavigationDrawerOpen()) {
            // update ActionBar and OptionsMenu
            onUpdateActionBarAndOptionsMenu(menu, inflater);
        }
    }

    protected void onUpdateActionBarAndOptionsMenu(Menu menu, MenuInflater inflater) {
    }
}
