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
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PlayerControlNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.VisualizerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.VisualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.VisualizerStateStore;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.ActionBarTileBuilder;
import com.h6ah4i.android.example.openslmediaplayer.app.visualizer.AudioLevelMeterSurfaceView;
import com.h6ah4i.android.example.openslmediaplayer.app.visualizer.BaseAudioVisualizerSurfaceView;
import com.h6ah4i.android.example.openslmediaplayer.app.visualizer.FftVisualizerSurfaceView;
import com.h6ah4i.android.example.openslmediaplayer.app.visualizer.WaveformVisualizerSurfaceView;
import com.h6ah4i.android.media.audiofx.IVisualizer;
import com.h6ah4i.android.media.audiofx.IVisualizer.MeasurementPeakRms;

public class VisualizerFragment
        extends ContentsBaseFragment
        implements CompoundButton.OnCheckedChangeListener,
        AdapterView.OnItemSelectedListener {

    // fields
    private volatile WaveformVisualizerSurfaceView mWaveformVisualizerView;
    private volatile FftVisualizerSurfaceView mFftVisualizerView;
    private volatile AudioLevelMeterSurfaceView mPeakLevelMeterView;
    private volatile AudioLevelMeterSurfaceView mRmsLevelMeterView;
    private ToggleButton mToggleButtonEnableWaveform;
    private ToggleButton mToggleButtonEnableFft;
    private ToggleButton mToggleButtonEnablePeak;
    private ToggleButton mToggleButtonEnableRms;
    private Spinner mSpinnerScalingMode;

    private PeriodicMeasureThread mPeriodicMeasureThread;

    private AppEventReceiver mAppEventReceiver;
    boolean mDuringSetupViews;

    private IVisualizer.OnDataCaptureListener mOnDataCaptureListener = new IVisualizer.OnDataCaptureListener() {

        @Override
        public void onWaveFormDataCapture(IVisualizer visualizer, byte[] waveform, int samplingRate) {
            VisualizerFragment.this.onWaveFormDataCapture(visualizer, waveform, samplingRate);
        }

        @Override
        public void onFftDataCapture(IVisualizer visualizer, byte[] fft, int samplingRate) {
            VisualizerFragment.this.onFftDataCapture(visualizer, fft, samplingRate);
        }
    };

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_visualizer, container, false);
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

        mPeriodicMeasureThread = new PeriodicMeasureThread(this);
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
        updateMeasurementsSettings();
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
                getActivity(), R.string.title_visualizer));
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        ViewCompat.setAlpha(buttonView, buttonView.isChecked() ? 0.5f : 1.0f);

        if (mDuringSetupViews)
            return;

        switch (buttonView.getId()) {
            case R.id.toggle_button_visualizer_enable_waveform:
                postAppEvent(
                        VisualizerReqEvents.SET_WAVEFORM_ENABLED, (isChecked ? 1 : 0), 0);
                break;
            case R.id.toggle_button_visualizer_enable_fft:
                postAppEvent(
                        VisualizerReqEvents.SET_FFT_ENABLED, (isChecked ? 1 : 0), 0);
                break;
            case R.id.toggle_button_visualizer_enable_peak:
                postAppEvent(
                        VisualizerReqEvents.SET_MEASURE_PEAK_ENABLED, (isChecked ? 1 : 0), 0);
                break;
            case R.id.toggle_button_visualizer_enable_rms:
                postAppEvent(
                        VisualizerReqEvents.SET_MEASURE_RMS_ENABLED, (isChecked ? 1 : 0), 0);
                break;
        }
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        switch (parent.getId()) {
            case R.id.spinner_visualizer_scaling_mode:
                postAppEvent(VisualizerReqEvents.SET_SCALING_MODE, position, 0);
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
            case EventDefs.Category.NOTIFY_VISUALIZER:
                onNotifyVisualizerEvent(event);
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

    private void onNotifyVisualizerEvent(AppEvent event) {
        switch (event.event) {
            case VisualizerNotifyEvents.WAVEFORM_ENABLED_STATE_UPDATED:
            case VisualizerNotifyEvents.FFT_ENABLED_STATE_UPDATED:
            case VisualizerNotifyEvents.SCALING_MODE_UPDATED:
                updateCaptureSettings();
                break;
            case VisualizerNotifyEvents.MEASURE_PEAK_ENABLED_STATE_UPDATED:
            case VisualizerNotifyEvents.MEASURE_RMS_ENABLED_STATE_UPDATED:
                updateMeasurementsSettings();
                break;
        }
    }

    private void onPlayerStateChanged(int state) {
        if (state == PlayerControlNotifyEvents.STATE_IDLE) {
            updateCaptureSettings();
            updateMeasurementsSettings();
        }
    }

    private void obtainViewReferences() {
        mWaveformVisualizerView = (WaveformVisualizerSurfaceView) findViewById(R.id.surfaceview_visualizer_waveform);
        mFftVisualizerView = (FftVisualizerSurfaceView) findViewById(R.id.surfaceview_visualizer_fft);
        mPeakLevelMeterView = (AudioLevelMeterSurfaceView) findViewById(R.id.surfaceview_visualizer_peak);
        mRmsLevelMeterView = (AudioLevelMeterSurfaceView) findViewById(R.id.surfaceview_visualizer_rms);

        mToggleButtonEnableWaveform = findToggleButtonByIdAndSetListener(R.id.toggle_button_visualizer_enable_waveform);
        mToggleButtonEnableFft = findToggleButtonByIdAndSetListener(R.id.toggle_button_visualizer_enable_fft);
        mToggleButtonEnablePeak = findToggleButtonByIdAndSetListener(R.id.toggle_button_visualizer_enable_peak);
        mToggleButtonEnableRms = findToggleButtonByIdAndSetListener(R.id.toggle_button_visualizer_enable_rms);

        mSpinnerScalingMode = findSpinnerByIdAndSetListener(R.id.spinner_visualizer_scaling_mode);
    }

    private void setupViews() {
        VisualizerStateStore state = getStateStore();
        Context context = getActivity();

        mDuringSetupViews = true;

        mToggleButtonEnableWaveform.setChecked(state.isCaptureWaveformEnabled());
        mToggleButtonEnableFft.setChecked(state.isCaptureFftEnabled());
        mToggleButtonEnablePeak.setChecked(state.isMeasurementPeakEnabled());
        mToggleButtonEnableRms.setChecked(state.isMeasurementRmsEnabled());

        {
            final ArrayAdapter<CharSequence> adapter =
                    ArrayAdapter.createFromResource(
                            context, R.array.visualizer_scaling_mode_names,
                            android.R.layout.simple_spinner_item);
            adapter.setDropDownViewResource(
                    android.R.layout.simple_spinner_dropdown_item);
            mSpinnerScalingMode.setAdapter(adapter);
        }

        mSpinnerScalingMode.setSelection(state.getScalingMode());

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
        final IVisualizer visualizer = getVisualizer();
        final VisualizerStateStore state = getStateStore();

        final boolean captureWaveform = state.isCaptureWaveformEnabled();
        final boolean captureFft = state.isCaptureFftEnabled();
        final int scalingMode = state.getScalingMode();

        if (visualizer != null) {
            // stop visualizer
            stopVisualizer(visualizer);

            if (captureFft || captureWaveform) {
                // capturing enabled

                // use maximum rate & size
                int rate = visualizer.getMaxCaptureRateCompat();
                int size = visualizer.getCaptureSizeRangeCompat()[1];

                visualizer.setCaptureSize(size);
                visualizer.setScalingModeCompat(scalingMode);
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

    private void updateMeasurementsSettings() {
        final IVisualizer visualizer = getVisualizer();
        final VisualizerStateStore state = getStateStore();

        final boolean measurePeak = state.isMeasurementPeakEnabled();
        final boolean measureRms = state.isMeasurementRmsEnabled();

        if (visualizer != null) {
            // stop visualizer
            stopVisualizer(visualizer);

            if (measurePeak || measureRms) {
                // measurements enabled
                visualizer.setMeasurementModeCompat(IVisualizer.MEASUREMENT_MODE_PEAK_RMS);
            } else {
                // measurements disabled
                visualizer.setMeasurementModeCompat(IVisualizer.MEASUREMENT_MODE_NONE);
            }

            // start visualizer
            startVisualizer(visualizer, state);
        }
    }

    private VisualizerStateStore getStateStore() {
        return getAppController().getVisualizerStateStore();
    }

    private IVisualizer getVisualizer() {
        return getAppController().getVisualizer();
    }

    private void cleanupVisualizer() {
        IVisualizer visualizer = getVisualizer();

        if (mPeriodicMeasureThread != null) {
            mPeriodicMeasureThread.stop();
        }

        if (visualizer != null) {
            visualizer.setEnabled(false);

            visualizer.setDataCaptureListener(null, 0, false, false);
        }
    }

    private void startVisualizer(IVisualizer visualizer, VisualizerStateStore state) {
        boolean waveform = state.isCaptureWaveformEnabled();
        boolean fft = state.isCaptureFftEnabled();
        boolean peak = state.isMeasurementPeakEnabled();
        boolean rms = state.isMeasurementRmsEnabled();

        if (visualizer != null) {
            visualizer.setEnabled(waveform | fft | peak | rms);

            // start measuring
            if (peak || rms) {
                mPeriodicMeasureThread.start(visualizer, peak, rms);
            }
        }
    }

    private void stopVisualizer(IVisualizer visualizer) {
        // stop measuring
        mPeriodicMeasureThread.stop();

        if (visualizer != null) {
            visualizer.setEnabled(false);
        }
    }

    /* package */void onWaveFormDataCapture(
            IVisualizer visualizer, byte[] waveform, int samplingRate) {
        WaveformVisualizerSurfaceView view = mWaveformVisualizerView;

        if (view != null) {
            view.updateAudioData(waveform, samplingRate);
        }
    }

    /* package */void onFftDataCapture(
            IVisualizer visualizer, byte[] fft, int samplingRate) {
        FftVisualizerSurfaceView view = mFftVisualizerView;

        if (view != null) {
            view.updateAudioData(fft, samplingRate);
        }
    }

    /* package */void onMeasurementPeaksUpdated(int level) {
        AudioLevelMeterSurfaceView view = mPeakLevelMeterView;

        if (view != null) {
            view.updateAudioLevel(level);
        }
    }

    /* package */void onMeasurementRmsUpdated(int level) {
        AudioLevelMeterSurfaceView view = mRmsLevelMeterView;

        if (view != null) {
            view.updateAudioLevel(level);
        }
    }

    private void postAppEvent(int event, int arg1, float arg2) {
        eventBus().post(new AppEvent(
                EventDefs.Category.VISUALIZER, event, arg1, arg2));
    }

    private static class AppEventReceiver extends AppEventBus.Receiver<VisualizerFragment> {
        private static final int[] FILTER = new int[] {
                EventDefs.Category.NOTIFY_PLAYER_CONTROL,
                EventDefs.Category.NOTIFY_VISUALIZER,
        };

        public AppEventReceiver(VisualizerFragment holder) {
            super(holder, FILTER);
        }

        @Override
        protected void onReceiveAppEvent(VisualizerFragment holder, AppEvent event) {
            holder.onReceiveAppEvent(event);
        }
    }

    private static class PeriodicMeasureThread {
        VisualizerFragment mFragment;
        Thread mThread;
        volatile boolean mStopRequested;

        public PeriodicMeasureThread(VisualizerFragment fragment) {
            mFragment = fragment;
        }

        public void captureThreadFunc(IVisualizer visualizer, boolean peak, boolean rms) {
            MeasurementPeakRms measurement = new MeasurementPeakRms();
            while (!mStopRequested) {
                try {
                    int result;

                    try {
                        result = visualizer.getMeasurementPeakRmsCompat(measurement);
                    } catch (IllegalStateException e) {
                        result = IVisualizer.ERROR_INVALID_OPERATION;
                    }

                    if (result == IVisualizer.SUCCESS) {

                        if (peak) {
                            mFragment.onMeasurementPeaksUpdated(measurement.mPeak);
                        }

                        if (rms) {
                            mFragment.onMeasurementRmsUpdated(measurement.mRms);
                        }
                    }

                    Thread.sleep(50);
                } catch (InterruptedException e) {
                }
            }
        }

        public void start(
                final IVisualizer visualizer,
                final boolean peak, final boolean rms) {
            stop();

            mStopRequested = false;
            mThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    captureThreadFunc(visualizer, peak, rms);
                }
            });
            mThread.start();
        }

        public void stop() {
            if (mThread != null) {
                mStopRequested = true;
                mThread.interrupt();

                while (true) {
                    try {
                        mThread.join();
                        break;
                    } catch (InterruptedException e) {
                    }
                }
                mThread = null;
            }

        }
    }
}
