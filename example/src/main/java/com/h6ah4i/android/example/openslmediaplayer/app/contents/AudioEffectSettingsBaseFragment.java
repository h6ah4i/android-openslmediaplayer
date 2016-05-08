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
import android.support.v4.view.MenuItemCompat;
import android.support.v7.app.ActionBar;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.CompoundButton;

import com.h6ah4i.android.example.openslmediaplayer.R;

public class AudioEffectSettingsBaseFragment extends ContentsBaseFragment {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @Override
    protected void onUpdateActionBarAndOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onUpdateActionBarAndOptionsMenu(menu, inflater);

        // inflate menu for audio effect settings
        inflater.inflate(R.menu.audio_effect_settings, menu);

        // set up effect On/Off switch
        MenuItem menuSwitchItem = menu.findItem(R.id.menu_item_action_bar_switch);
        CompoundButton effectOnOffSwitch = (CompoundButton) MenuItemCompat.getActionView(menuSwitchItem);

        effectOnOffSwitch.setChecked(onGetActionBarSwitchCheckedState());
        effectOnOffSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                onActionBarSwitchCheckedChanged(buttonView, isChecked);
            }
        });

        ActionBar actionBar = getActionBar();

        actionBar.setTitle(onGetActionBarTitleText());
    }

    protected CharSequence onGetActionBarTitleText() {
        // sub class should override this method
        return "";
    }

    protected boolean onGetActionBarSwitchCheckedState() {
        // sub class should override this method
        return false;
    }

    protected void onActionBarSwitchCheckedChanged(
            CompoundButton buttonView, boolean isChecked) {
        // sub class should override this method
    }
}
