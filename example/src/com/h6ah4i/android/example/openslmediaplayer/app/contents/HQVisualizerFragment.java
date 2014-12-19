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
import android.support.v4.view.ViewCompat;
import android.support.v7.app.ActionBar;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CompoundButton;
import android.widget.Spinner;
import android.widget.ToggleButton;

import com.h6ah4i.android.example.openslmediaplayer.R;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEventBus;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.HQVisualizerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.HQVisualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PlayerControlNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.HQVisualizerStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.ActionBarTileBuilder;
import com.h6ah4i.android.example.openslmediaplayer.app.visualizer.BaseAudioVisualizerSurfaceView;
import com.h6ah4i.android.example.openslmediaplayer.app.visualizer.HQFftVisualizerSurfaceView;
import com.h6ah4i.android.example.openslmediaplayer.app.visualizer.HQWaveformVisualizerSurfaceView;
import com.h6ah4i.android.media.audiofx.IHQVisualizer;

public class HQVisualizerFragment
        extends ContentsBaseFragment
        implements CompoundButton.OnCheckedChangeListener,
        AdapterView.OnItemSelectedListener {

    private static final int[] WINDOW_TYPE_VALUES = {
            IHQVisualizer.WINDOW_RECTANGULAR,
            IHQVisualizer.WINDOW_RECTANGULAR | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            IHQVisualizer.WINDOW_HANN,
            IHQVisualizer.WINDOW_HANN | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            IHQVisualizer.WINDOW_HAMMING,
            IHQVisualizer.WINDOW_HAMMING | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            IHQVisualizer.WINDOW_BLACKMAN,
            IHQVisualizer.WINDOW_BLACKMAN | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
            IHQVisualizer.WINDOW_FLAT_TOP,
            IHQVisualizer.WINDOW_FLAT_TOP | IHQVisualizer.WINDOW_OPT_APPLY_FOR_WAVEFORM,
    };

    // fields
    private volatile HQWaveformVisualizerSurfaceView mWaveformVisualizerView;
    private volatile HQFftVisualizerSurfaceView mFftVisualizerView;
    private ToggleButton mToggleButtonEnableWaveform;
    private ToggleButton mToggleButtonEnableFft;
    private Spinner mSpinnerWindowType;

    private AppEventReceiver mAppEventReceiver;
    boolean mDuringSetupViews;

    private IHQVisualizer.OnDataCaptureListener mOnDataCaptureListener = new IHQVisualizer.OnDataCaptureListener() {

        @Override
        public void onWaveFormDataCapture(IHQVisualizer visualizer, float[] waveform,
                int numChannels, int samplingRate) {
            HQVisualizerFragment.this.onWaveFormDataCapture(visualizer, waveform, numChannels,
                    samplingRate);
        }

        @Override
        public void onFftDataCapture(IHQVisualizer visualizer, float[] fft, int numChannels,
                int samplingRate) {
            HQVisualizerFragment.this.onFftDataCapture(visualizer, fft, numChannels, samplingRate);
        }
    };

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_hq_visualizer, container, false);
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
        mWaveformVisualizerView = null;
        mFftVisualizerView = null;
        mToggleButtonEnableWaveform = null;
        mToggleButtonEnableFft = null;
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
    public void onResume() {
        super.onResume();

        setupViews();
        resumeVisualizerViews();
        updateCaptureSettings();
    }

    @Override
    public void onPause() {
        super.onPause();
        pauseVisualizerViews();
        cleanupVisualizer();
    }

    @Override
    protected void onUpdateActionBarAndOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onUpdateActionBarAndOptionsMenu(menu, inflater);

        ActionBar actionBar = getActionBar();

        actionBar.setTitle(ActionBarTileBuilder.makeTitleString(
                getActivity(), R.string.title_hq_visualizer));
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        ViewCompat.setAlpha(buttonView, buttonView.isChecked() ? 0.5f : 1.0f);

        if (mDuringSetupViews)
            return;

        switch (buttonView.getId()) {
            case R.id.toggle_button_visualizer_enable_waveform:
                postAppEvent(
                        HQVisualizerReqEvents.SET_WAVEFORM_ENABLED, (isChecked ? 1 : 0), 0);
                break;
            case R.id.toggle_button_visualizer_enable_fft:
                postAppEvent(
                        HQVisualizerReqEvents.SET_FFT_ENABLED, (isChecked ? 1 : 0), 0);
                break;
        }
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        switch (parent.getId()) {
            case R.id.spinner_hq_visualizer_window_type:
                postAppEvent(
                        HQVisualizerReqEvents.SET_WINDOW_TYPE,
                        listPositionToWindowType(position),
                        0);
                break;
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    /* package */void onReceiveAppEvent(AppEvent event) {
        switch (event.category) {
            case EventDefs.Category.NOTIFY_PLAYER_CONTROL:
                onNotifyPlayerControlEvent(event);
                break;
            case EventDefs.Category.NOTIFY_HQ_VISUALIZER:
                onNotifyHQVisualizerEvent(event);
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

    private void onNotifyHQVisualizerEvent(AppEvent event) {
        switch (event.event) {
            case HQVisualizerNotifyEvents.WAVEFORM_ENABLED_STATE_UPDATED:
            case HQVisualizerNotifyEvents.FFT_ENABLED_STATE_UPDATED:
                updateCaptureSettings();
                break;
            case HQVisualizerNotifyEvents.WINDOW_TYPE_UPDATED:
                updateWindowType();
                break;
        }
    }

    private void onPlayerStateChanged(int state) {
        if (state == PlayerControlNotifyEvents.STATE_IDLE) {
            updateCaptureSettings();
        }
    }

    private void obtainViewReferences() {
        mToggleButtonEnableWaveform = findToggleButtonByIdAndSetListener(R.id.toggle_button_visualizer_enable_waveform);
        mToggleButtonEnableFft = findToggleButtonByIdAndSetListener(R.id.toggle_button_visualizer_enable_fft);
        mSpinnerWindowType = findSpinnerByIdAndSetListener(R.id.spinner_hq_visualizer_window_type);

        mWaveformVisualizerView =
                (HQWaveformVisualizerSurfaceView) findViewById(R.id.surfaceview_visualizer_waveform);
        mFftVisualizerView =
                (HQFftVisualizerSurfaceView) findViewById(R.id.surfaceview_visualizer_fft);
    }

    private void setupViews() {
        HQVisualizerStateStore state = getStateStore();
        Context context = getActivity();

        mDuringSetupViews = true;

        mToggleButtonEnableWaveform.setChecked(state.isCaptureWaveformEnabled());
        mToggleButtonEnableFft.setChecked(state.isCaptureFftEnabled());

        {
            final ArrayAdapter<CharSequence> adapter =
                    ArrayAdapter.createFromResource(
                            context, R.array.hq_visualizer_window_type_names,
                            android.R.layout.simple_spinner_item);
            adapter.setDropDownViewResource(
                    android.R.layout.simple_spinner_dropdown_item);
            mSpinnerWindowType.setAdapter(adapter);
        }

        mSpinnerWindowType.setSelection(windowTypeToListPosition(state.getWindowType()));

        mDuringSetupViews = false;
    }

    private static void safeResumeVizualizerView(BaseAudioVisualizerSurfaceView v) {
        if (v != null) {
            v.onResume();
        }
    }

    private static void safePauseVizualizerView(BaseAudioVisualizerSurfaceView v) {
        if (v != null) {
            v.onResume();
        }
    }

    private void resumeVisualizerViews() {
        safeResumeVizualizerView(mWaveformVisualizerView);
        safeResumeVizualizerView(mFftVisualizerView);
    }

    private void pauseVisualizerViews() {
        safePauseVizualizerView(mWaveformVisualizerView);
        safePauseVizualizerView(mFftVisualizerView);
    }

    private void updateCaptureSettings() {
        final IHQVisualizer visualizer = getVisualizer();
        final HQVisualizerStateStore state = getStateStore();

        final boolean captureWaveform = state.isCaptureWaveformEnabled();
        final boolean captureFft = state.isCaptureFftEnabled();

        if (visualizer != null) {
            // stop visualizer
            stopVisualizer(visualizer);

            if (captureFft || captureWaveform) {
                // capturing enabled

                // use maximum rate & size
                int rate = visualizer.getMaxCaptureRate();
                int size = 4096;

                // NOTE: min = 128, max = 32768
                size = Math.max(visualizer.getCaptureSizeRange()[0], size);
                size = Math.min(visualizer.getCaptureSizeRange()[1], size);

                visualizer.setWindowFunction(state.getWindowType());
                visualizer.setCaptureSize(size);
                visualizer.setDataCaptureListener(
                        mOnDataCaptureListener, rate, captureWaveform, captureFft);
            } else {
                // capturing disabled
                visualizer.setDataCaptureListener(null, 0, false, false);
            }

            // start visualizer
            startVisualizer(visualizer, state);
        }
    }

    private void updateWindowType() {
        final IHQVisualizer visualizer = getVisualizer();
        final HQVisualizerStateStore state = getStateStore();

        if (visualizer != null) {
            visualizer.setWindowFunction(state.getWindowType());
        }
    }

    private HQVisualizerStateStore getStateStore() {
        return getAppController().getHQVisualizerStateStore();
    }

    private IHQVisualizer getVisualizer() {
        return getAppController().getHQVisualizer();
    }

    private void cleanupVisualizer() {
        IHQVisualizer visualizer = getVisualizer();

        if (visualizer != null) {
            visualizer.setEnabled(false);

            visualizer.setDataCaptureListener(null, 0, false, false);
        }
    }

    private void startVisualizer(IHQVisualizer visualizer, HQVisualizerStateStore state) {
        boolean waveform = state.isCaptureWaveformEnabled();
        boolean fft = state.isCaptureFftEnabled();

        if (visualizer != null) {
            visualizer.setEnabled(waveform | fft);
        }
    }

    private void stopVisualizer(IHQVisualizer visualizer) {
        if (visualizer != null) {
            visualizer.setEnabled(false);
        }
    }

    /* package */void onWaveFormDataCapture(
            IHQVisualizer visualizer, float[] waveform, int numChannels, int samplingRate) {
        HQWaveformVisualizerSurfaceView view = mWaveformVisualizerView;

        if (view != null) {
            view.updateAudioData(waveform, numChannels, samplingRate);
        }
    }

    /* package */void onFftDataCapture(
            IHQVisualizer visualizer, float[] fft, int numChannels, int samplingRate) {
        HQFftVisualizerSurfaceView view = mFftVisualizerView;

        if (view != null) {
            view.updateAudioData(fft, numChannels, samplingRate);
        }
    }

    private void postAppEvent(int event, int arg1, float arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.HQ_VISUALIZER, event, arg1, arg2));
    }

    private static class AppEventReceiver extends AppEventBus.Receiver<HQVisualizerFragment> {
        private static final int[] FILTER = new int[] {
                EventDefs.Category.NOTIFY_PLAYER_CONTROL,
                EventDefs.Category.NOTIFY_HQ_VISUALIZER,
        };

        public AppEventReceiver(HQVisualizerFragment holder) {
            super(holder, FILTER);
        }

        @Override
        protected void onReceiveAppEvent(HQVisualizerFragment holder, AppEvent event) {
            holder.onReceiveAppEvent(event);
        }
    }

    private static int windowTypeToListPosition(int type) {
        for (int i = 0; i < WINDOW_TYPE_VALUES.length; i++) {
            if (WINDOW_TYPE_VALUES[i] == type) {
                return i;
            }
        }
        return -1;
    }

    private static int listPositionToWindowType(int position) {
        if (position >= 0 && position < WINDOW_TYPE_VALUES.length) {
            return WINDOW_TYPE_VALUES[position];
        } else {
            return WINDOW_TYPE_VALUES[0];
        }
    }
}
