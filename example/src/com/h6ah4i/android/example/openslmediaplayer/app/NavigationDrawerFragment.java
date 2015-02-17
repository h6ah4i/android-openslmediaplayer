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

import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_BASSBOOST;
import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_ENVIRONMENTAL_REVERB;
import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_EQUALIZER;
import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_HQ_EQUALIZER;
import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_HQ_VISUALIZER;
import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_LOUDNESS_ENHANCER;
import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_PRESET_REVERB;
import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_VIRTUALIZER;
import static com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents.SECTION_INDEX_VISUALIZER;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.view.ViewCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBar;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.widget.SwitchCompat;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEventBus;
import com.h6ah4i.android.example.openslmediaplayer.app.model.BaseAudioEffectStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.MediaPlayerStateStore;

public class NavigationDrawerFragment extends AppBaseFragment {

    private static class AppEventReceiver extends AppEventBus.Receiver<NavigationDrawerFragment> {
        private static final int[] FILTER = new int[] {
                EventDefs.Category.NOTIFY_BASSBOOST,
                EventDefs.Category.NOTIFY_VIRTUALIZER,
                EventDefs.Category.NOTIFY_EQUALIZER,
                EventDefs.Category.NOTIFY_LOUDNESS_ENHANCER,
                EventDefs.Category.NOTIFY_PRESET_REVERB,
                EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB,
                EventDefs.Category.NOTIFY_HQ_EQUALIZER,
        };

        public AppEventReceiver(NavigationDrawerFragment holder) {
            super(holder, FILTER);
        }

        @Override
        protected void onReceiveAppEvent(NavigationDrawerFragment holder, AppEvent event) {
            holder.onReceiveAppEvent(event);
        }
    }

    private AppEventReceiver mAppEventReceiver;

    private ActionBarDrawerToggle mDrawerToggle;

    private DrawerLayout mDrawerLayout;
    private RelativeLayout mContainerGroupLayout;
    private ListView mModeSelectListView;
    private ListView mPageSelectListView;
    private View mFragmentContainerView;
    private ModeSelectListAdapter mModeSelectAdapter;
    private PageSelectListAdapter mPageSelectAdapter;

    public NavigationDrawerFragment() {
    }

    /* package */void onReceiveAppEvent(AppEvent event) {
        switch (event.category) {
            case EventDefs.Category.NOTIFY_BASSBOOST:
                updateAudioEffectsEnabledState(SECTION_INDEX_BASSBOOST);
                break;
            case EventDefs.Category.NOTIFY_VIRTUALIZER:
                updateAudioEffectsEnabledState(SECTION_INDEX_VIRTUALIZER);
                break;
            case EventDefs.Category.NOTIFY_EQUALIZER:
                updateAudioEffectsEnabledState(SECTION_INDEX_EQUALIZER);
                break;
            case EventDefs.Category.NOTIFY_LOUDNESS_ENHANCER:
                updateAudioEffectsEnabledState(SECTION_INDEX_LOUDNESS_ENHANCER);
                break;
            case EventDefs.Category.NOTIFY_PRESET_REVERB:
                updateAudioEffectsEnabledState(SECTION_INDEX_PRESET_REVERB);
                break;
            case EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB:
                updateAudioEffectsEnabledState(SECTION_INDEX_ENVIRONMENTAL_REVERB);
                break;
            case EventDefs.Category.NOTIFY_HQ_EQUALIZER:
                updateAudioEffectsEnabledState(SECTION_INDEX_HQ_EQUALIZER);
                break;
        }
    }

    private BaseAudioEffectStateStore getAudioEfectStateStore(int index) {
        switch (index) {
            case SECTION_INDEX_BASSBOOST:
                return getAppController().getBassBoostStateStore();
            case SECTION_INDEX_VIRTUALIZER:
                return getAppController().getVirtualizerStateStore();
            case SECTION_INDEX_EQUALIZER:
                return getAppController().getEqualizerStateStore();
            case SECTION_INDEX_LOUDNESS_ENHANCER:
                return getAppController().getLoudnessEnhancerStateStore();
            case SECTION_INDEX_PRESET_REVERB:
                return getAppController().getPresetReverbStateStore();
            case SECTION_INDEX_ENVIRONMENTAL_REVERB:
                return getAppController().getEnvironmentalReverbStateStore();
            case SECTION_INDEX_VISUALIZER:
                return getAppController().getVisualizerStateStore();
            case SECTION_INDEX_HQ_EQUALIZER:
                return getAppController().getHQEqualizerStateStore();
            case SECTION_INDEX_HQ_VISUALIZER:
                return getAppController().getHQVisualizerStateStore();
            default:
                throw new IllegalArgumentException();
        }
    }

    private void updateAudioEffectsEnabledState(int index) {
        if (mPageSelectAdapter == null)
            return;

        boolean isEnabled = getAudioEfectStateStore(index).isEnabled();
        mPageSelectAdapter.getItem(index).isChecked = isEnabled;
        mPageSelectAdapter.notifyDataSetChanged();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setHasOptionsMenu(true);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        mContainerGroupLayout = (RelativeLayout) inflater.inflate(
                R.layout.fragment_navigation_drawer, container, false);

        setupModeSelectListView();
        setupPageSelectListView();

        return mContainerGroupLayout;
    }

    private void setupModeSelectListView() {
        mModeSelectListView =
                (ListView) mContainerGroupLayout.findViewById(R.id.list_mode_selection);

        ModeSelectListAdapter.ListItem[] listItems;

        final int curPlayerImplType = getAppController().getPlayerStateStore().getPlayerImplType();
        final int selectedIntex =
                (curPlayerImplType == MediaPlayerStateStore.PLAYER_IMPL_TYPE_STANDARD) ? 0 : 1;

        listItems = new ModeSelectListAdapter.ListItem[] {
                makeModeSelectListItem(
                R.string.mediaplayer_impl_standard),
                makeModeSelectListItem(
                R.string.mediaplayer_impl_opensl),
        };

        mModeSelectAdapter = new ModeSelectListAdapter(
                getActionBar().getThemedContext(), listItems);

        mModeSelectListView.setAdapter(mModeSelectAdapter);
        mModeSelectListView.setChoiceMode(AbsListView.CHOICE_MODE_SINGLE);
        mModeSelectListView.setItemChecked(selectedIntex, true);
        mModeSelectListView.setOnItemClickListener(new
                AdapterView.OnItemClickListener() {
                    @Override
                    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                        onModeSelectLiteItemClick(position);
                    }
                });
    }

    private void setupPageSelectListView() {
        mPageSelectListView =
                (ListView) mContainerGroupLayout.findViewById(R.id.list_page_selection);

        mPageSelectListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                onPageSelectListItemClick(position);
            }
        });

        PageSelectListAdapter.ListItem[] listItems;

        listItems = new PageSelectListAdapter.ListItem[] {
                makePageSelectListItem(
                        R.string.title_player_control, false, false),
                makePageSelectListItem(
                        R.string.title_bassboost, true,
                        getAudioEfectStateStore(SECTION_INDEX_BASSBOOST).isEnabled()),
                makePageSelectListItem(
                        R.string.title_virtualizer, true,
                        getAudioEfectStateStore(SECTION_INDEX_VIRTUALIZER).isEnabled()),
                makePageSelectListItem(
                        R.string.title_equalizer, true,
                        getAudioEfectStateStore(SECTION_INDEX_EQUALIZER).isEnabled()),
                makePageSelectListItem(
                        R.string.title_loudness_enhancer, true,
                        getAudioEfectStateStore(SECTION_INDEX_LOUDNESS_ENHANCER).isEnabled()),
                makePageSelectListItem(
                        R.string.title_preset_reverb, true,
                        getAudioEfectStateStore(SECTION_INDEX_PRESET_REVERB).isEnabled()),
                makePageSelectListItem(
                        R.string.title_environmental_reverb, true,
                        getAudioEfectStateStore(SECTION_INDEX_ENVIRONMENTAL_REVERB).isEnabled()),
                makePageSelectListItem(
                        R.string.title_visualizer, false, false),
                makePageSelectListItem(
                        R.string.title_hq_equalizer, true,
                        getAudioEfectStateStore(SECTION_INDEX_HQ_EQUALIZER).isEnabled()),
                makePageSelectListItem(
                        R.string.title_hq_visualizer, false, false),
                makePageSelectListItem(
                        R.string.title_about, false, false),
        };

        mPageSelectAdapter = new PageSelectListAdapter(
                getActionBar().getThemedContext(), listItems);
        mPageSelectAdapter.setOnItemSwitchCheckedChangedListener(
                new PageSelectListAdapter.OnItemSwitchCheckedChangedListener() {

                    @Override
                    public void onItemSwitchCheckedChanged(int position, long id, boolean isChecked) {
                        onPageSelectListItemSwitchClick(position, isChecked);
                    }
                });

        mPageSelectListView.setAdapter(mPageSelectAdapter);
    }

    private static ModeSelectListAdapter.ListItem makeModeSelectListItem(
            int textResId) {
        return new ModeSelectListAdapter.ListItem(textResId);
    }

    private static PageSelectListAdapter.ListItem makePageSelectListItem(
            int textResId, boolean useSwitch, boolean isChecked) {
        return new PageSelectListAdapter.ListItem(textResId, useSwitch, isChecked);
    }

    public boolean isDrawerOpen() {
        return mDrawerLayout != null && mDrawerLayout.isDrawerOpen(mFragmentContainerView);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        Activity activity = getActivity();
        setUp((DrawerLayout) activity.findViewById(R.id.drawer_layout),
                (ViewGroup) activity.findViewById(R.id.drawer_container));
    }

    public void setSystemBarsOffset(int statusBarOffset, int navBarOffset) {
        if (mModeSelectListView != null) {
            View v = mModeSelectListView;
            v.setPadding(
                    ViewCompat.getPaddingStart(v),
                    statusBarOffset,
                    ViewCompat.getPaddingEnd(v),
                    v.getPaddingBottom());
        }
        if (mPageSelectListView != null) {
            View v = mPageSelectListView;
            v.setPadding(
                    ViewCompat.getPaddingStart(v),
                    v.getPaddingTop(),
                    ViewCompat.getPaddingEnd(v),
                    navBarOffset);
        }
    }

    private void setUp(DrawerLayout drawerLayout, ViewGroup containerView) {
        mFragmentContainerView = containerView;
        mDrawerLayout = drawerLayout;

        // set up the drawer's list view with items and click listener

        // ActionBarDrawerToggle ties together the the proper interactions
        // between the navigation drawer and the action bar app icon.
        mDrawerToggle = new ActionBarDrawerToggle(
                getActivity(),
                mDrawerLayout,
                R.string.navigation_drawer_open,
                R.string.navigation_drawer_close
                ) {
                    @Override
                    public void onDrawerClosed(View drawerView) {
                        super.onDrawerClosed(drawerView);
                        if (!isAdded()) {
                            return;
                        }

                        // calls onPrepareOptionsMenu()
                        getActivity().supportInvalidateOptionsMenu();
                    }

                    @Override
                    public void onDrawerOpened(View drawerView) {
                        super.onDrawerOpened(drawerView);
                        if (!isAdded()) {
                            return;
                        }

                        // calls onPrepareOptionsMenu()
                        getActivity().supportInvalidateOptionsMenu();
                    }
                };

        // Defer code dependent on restoration of previous instance state.
        mDrawerLayout.post(new Runnable() {
            @Override
            public void run() {
                mDrawerToggle.syncState();
            }
        });

        mDrawerLayout.setDrawerListener(mDrawerToggle);
    }

    protected void onModeSelectLiteItemClick(int position) {
        int type;

        switch (position) {
            case 0:
                type = NavigationDrawerReqEvents.IMPL_TYPE_STANDARD;
                break;
            case 1:
                type = NavigationDrawerReqEvents.IMPL_TYPE_OPENSL;
                break;
            default:
                return;
        }

        postAppEvent(NavigationDrawerReqEvents.PLAYER_SET_IMPL_TYPE, type, 0);
    }

    private void onPageSelectListItemClick(int position) {
        if (mDrawerLayout != null) {
            mDrawerLayout.closeDrawer(mFragmentContainerView);
        }

        postAppEvent(
                NavigationDrawerReqEvents.SELECT_PAGE,
                position, 0);
    }

    private void onPageSelectListItemSwitchClick(int position, boolean isChecked) {
        postAppEvent(
                NavigationDrawerReqEvents.CLICK_ITEM_ENABLE_SWITCH,
                position,
                (isChecked) ? 1 : 0);
    }

    private void postAppEvent(int event, int arg1, int arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.NAVIGATION_DRAWER,
                event, arg1, arg2));
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
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        mDrawerToggle.onConfigurationChanged(newConfig);
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        if (isDrawerOpen()) {
            showGlobalContextActionBar();
        }
        super.onCreateOptionsMenu(menu, inflater);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (mDrawerToggle.onOptionsItemSelected(item)) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    private void showGlobalContextActionBar() {
        ActionBar actionBar = getActionBar();
        actionBar.setTitle(R.string.app_name);
    }

    private static class ModeSelectListAdapter extends BaseAdapter {
        static public class ListItem {
            public int textResId;

            public ListItem(int textResId) {
                this.textResId = textResId;
            }
        }

        private Context mContext;
        private ListItem[] mItems;
        private LayoutInflater mInflater;

        public ModeSelectListAdapter(Context context, ListItem[] items) {
            mContext = context;
            mItems = items;
            mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        @Override
        public int getCount() {
            return mItems.length;
        }

        @Override
        public ListItem getItem(int position) {
            return mItems[position];
        }

        @Override
        public long getItemId(int position) {
            return (position + 1);
        }

        @Override
        public View getView(final int position, View convertView, ViewGroup parent) {
            ModeSelectListItemView v;

            if (convertView != null) {
                v = (ModeSelectListItemView) convertView;
            } else {
                v = (ModeSelectListItemView) mInflater.inflate(
                        R.layout.navigation_drawer_mode_select_list_item, parent, false);
            }

            ListItem item = getItem(position);

            v.setText(item.textResId);

            return v;
        }
    }

    private static class PageSelectListAdapter extends BaseAdapter {
        static public class ListItem {
            public int textResId;
            public boolean isChecked;
            public boolean useSwitch;

            public ListItem(int textResId, boolean useSwitch, boolean isChecked) {
                this.textResId = textResId;
                this.useSwitch = useSwitch;
                this.isChecked = isChecked;
            }
        }

        public interface OnItemSwitchCheckedChangedListener {
            void onItemSwitchCheckedChanged(int position, long id, boolean isChecked);
        }

        private Context mContext;
        private ListItem[] mItems;
        private LayoutInflater mInflater;

        private OnItemSwitchCheckedChangedListener mOnItemSwitchCheckedChangedListener;

        public PageSelectListAdapter(
                Context context, ListItem[] items) {
            mContext = context;
            mItems = items;
            mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }

        @Override
        public int getCount() {
            return mItems.length;
        }

        @Override
        public ListItem getItem(int position) {
            return mItems[position];
        }

        @Override
        public long getItemId(int position) {
            return (position + 1);
        }

        static class Tag {
            public TextView text;
            public SwitchCompat sw;

            public Tag(View v) {
                text = (TextView) v.findViewById(android.R.id.text1);
                sw = (SwitchCompat) v.findViewById(android.R.id.toggle);
            }
        }

        @SuppressLint("ViewHolder")
        @Override
        public View getView(final int position, View convertView, ViewGroup parent) {
            View v;

            // NOTE: SwitchCompat has a animation related bug if used inside of
            // the list...
            if (convertView != null
                    && (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH)) {
                v = convertView;
            } else {
                v = mInflater.inflate(R.layout.navigation_drawer_page_select_list_item, parent,
                        false);
                v.setTag(new Tag(v));
            }

            Tag tag = (Tag) v.getTag();
            ListItem item = getItem(position);

            // TextView
            tag.text.setText(item.textResId);

            // Switch
            tag.sw.setOnCheckedChangeListener(null);
            tag.sw.setVisibility(item.useSwitch ? View.VISIBLE : View.GONE);
            tag.sw.setChecked(item.isChecked);
            tag.sw.setClickable(true);
            tag.sw.setFocusable(false);
            tag.sw.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    onItemSwitchCheckedChanged((CompoundButton) buttonView, position, isChecked);
                }
            });

            return v;
        }

        private void onItemSwitchCheckedChanged(CompoundButton v, int position, boolean isChecked) {
            if (mOnItemSwitchCheckedChangedListener != null) {
                long id = getItemId(position);
                mOnItemSwitchCheckedChangedListener.onItemSwitchCheckedChanged(position, id,
                        isChecked);
            }
        }

        public void setOnItemSwitchCheckedChangedListener(
                OnItemSwitchCheckedChangedListener listener) {
            mOnItemSwitchCheckedChangedListener = listener;
        }
    }
}
