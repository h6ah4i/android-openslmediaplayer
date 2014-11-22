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


package com.h6ah4i.android.example.openslmediaplayer.app.model;

public class EventDefs {

    // categories
    public static final class Category {
        public static final int NONE = 0;

        public static final int NAVIGATION_DRAWER = 10;

        public static final int PLAYER_CONTROL = 100;
        public static final int BASSBOOST = 101;
        public static final int VIRTUALIZER = 102;
        public static final int EQUALIZER = 103;
        public static final int LOUDNESS_EHNAHCER = 104;
        public static final int ENVIRONMENTAL_REVERB = 105;
        public static final int PRESET_REVERB = 106;
        public static final int VISUALIZER = 107;
        public static final int HQ_EQUALIZER = 108;
        public static final int PRE_AMP = 109;
        public static final int HQ_VISUALIZER = 110;

        public static final int NOTIFY_PLAYER_CONTROL = 200;
        public static final int NOTIFY_BASSBOOST = 201;
        public static final int NOTIFY_VIRTUALIZER = 202;
        public static final int NOTIFY_EQUALIZER = 203;
        public static final int NOTIFY_LOUDNESS_ENHANCER = 204;
        public static final int NOTIFY_ENVIRONMENTAL_REVERB = 205;
        public static final int NOTIFY_PRESET_REVERB = 206;
        public static final int NOTIFY_VISUALIZER = 207;
        public static final int NOTIFY_HQ_EQUALIZER = 208;
        public static final int NOTIFY_PRE_AMP = 209;
        public static final int NOTIFY_HQ_VISUALIZER = 210;
    }

    // Navigation Drawer
    public static final class NavigationDrawerReqEvents {
        // arg1: section index
        public static final int SELECT_PAGE = 0;

        // arg1: section index
        public static final int CLICK_ITEM_ENABLE_SWITCH = 1;

        // SECTION INDEX
        public static final int SECTION_INDEX_PLAYER_CONTROL = 0;
        public static final int SECTION_INDEX_BASSBOOST = 1;
        public static final int SECTION_INDEX_VIRTUALIZER = 2;
        public static final int SECTION_INDEX_EQUALIZER = 3;
        public static final int SECTION_INDEX_LOUDNESS_ENHANCER = 4;
        public static final int SECTION_INDEX_PRESET_REVERB = 5;
        public static final int SECTION_INDEX_ENVIRONMENTAL_REVERB = 6;
        public static final int SECTION_INDEX_VISUALIZER = 7;
        public static final int SECTION_INDEX_HQ_EQUALIZER = 8;
        public static final int SECTION_INDEX_HQ_VISUALIZER = 9;
        public static final int SECTION_INDEX_ABOUT = 10;

        // arg1: impl type
        public static final int PLAYER_SET_IMPL_TYPE = 20;

        // media player impl. type
        public static final int IMPL_TYPE_STANDARD = MediaPlayerStateStore.PLAYER_IMPL_TYPE_STANDARD;
        public static final int IMPL_TYPE_OPENSL = MediaPlayerStateStore.PLAYER_IMPL_TYPE_OPENSL;
    }

    public static final class PlayerControlReqEvents {
        // arg1: index, extra: URI
        public static final int SONG_PICKED = 0;
        public static final int PLAYER_CREATE = 1;
        public static final int PLAYER_SET_DATA_SOURCE = 2;
        public static final int PLAYER_PREPARE = 3;
        public static final int PLAYER_PREPARE_ASYNC = 4;
        public static final int PLAYER_START = 5;
        public static final int PLAYER_PAUSE = 6;
        public static final int PLAYER_STOP = 7;
        public static final int PLAYER_RESET = 8;
        public static final int PLAYER_RELEASE = 9;
        // arg2: position
        public static final int PLAYER_SEEK_TO = 10;
        // arg2: volume (float)
        public static final int PLAYER_SET_VOLUME_LEFT = 11;
        // arg2: volume (float)
        public static final int PLAYER_SET_VOLUME_RIGHT = 12;
        // arg1: looping
        public static final int PLAYER_SET_LOOPING = 13;
        // arg1: aux effect type
        public static final int PLAYER_ATTACH_AUX_EFFECT = 14;
        // arg2: send level (float)
        public static final int PLAYER_SET_AUX_SEND_LEVEL = 15;

        // AUX EFFECT TYPE
        public static final int AUX_EEFECT_TYPE_NONE = 0;
        public static final int AUX_EEFECT_TYPE_ENVIRONMENAL_REVERB = 1;
        public static final int AUX_EEFECT_TYPE_PRESET_REVERB = 2;

        public static final String EXTRA_URI = "uri";
    }

    public static final class PlayerControlNotifyEvents {
        // arg1: player no. arg2: state
        public static final int PLAYER_STATE_CHANGED = 0;
        // arg1: player no. extras: EXTRA_ERROR_INFO_WHAT,
        // EXTRA_ERROR_INFO_EXTRA
        public static final int NOTIFY_PLAYER_INFO = 1;
        // arg1: player no. extras: EXTRA_ERROR_INFO_WHAT,
        // EXTRA_ERROR_INFO_EXTRA
        public static final int NOTIFY_PLAYER_ERROR = 2;
        // arg1: player no. extras: EXTRA_EXEC_OPERATION_NAME,
        // EXTRA_EXCEPTION_NAME, EXTRA_STACK_TRACE
        public static final int NOTIFY_EXCEPTION_OCCURRED = 3;

        public static final int STATE_IDLE = 0;
        public static final int STATE_INITIALIZED = 1;
        public static final int STATE_PREPARING = 2;
        public static final int STATE_PREPARED = 3;
        public static final int STATE_STARTED = 4;
        public static final int STATE_PAUSED = 5;
        public static final int STATE_STOPPED = 6;
        public static final int STATE_PLAYBACK_COMPLETED = 7;
        public static final int STATE_END = 8;
        public static final int STATE_ERROR = 9;

        public static final String EXTRA_ERROR_INFO_WHAT = "error_info_what";
        public static final String EXTRA_ERROR_INFO_EXTRA = "error_info_extra";
        public static final String EXTRA_EXEC_OPERATION_NAME = "exec_operation_name";
        public static final String EXTRA_EXCEPTION_NAME = "exception_name";
        public static final String EXTRA_STACK_TRACE = "stack_trace";
    }

    // Bass Boost
    public static final class BassBoostReqEvents {
        public static final int SET_ENABLED = 0;

        // arg2: strength (float)
        public static final int SET_STRENGTH = 1;
    }

    public static final class BassBoostNotifyEvents {
        public static final int ENABLED_STATE_UPDATED = 0;
        public static final int STRENGTH_UPDATED = 1;
    }

    // Virtualizer
    public static final class VirtualizerReqEvents {
        public static final int SET_ENABLED = 0;

        // arg2: strength (float)
        public static final int SET_STRENGTH = 1;
    }

    public static final class VirtualizerNotifyEvents {
        public static final int ENABLED_STATE_UPDATED = 0;
        public static final int STRENGTH_UPDATED = 1;
    }

    // Equalizer
    public static final class EqualizerReqEvents {
        public static final int SET_ENABLED = 0;
        // arg1: preset
        public static final int SET_PRESET = 1;
        // arg1: band, arg2: level (float)
        public static final int SET_BAND_LEVEL = 2;
    }

    public static final class EqualizerNotifyEvents {
        public static final int ENABLED_STATE_UPDATED = 0;
        // arg1: preset
        public static final int PRESET_UPDATED = 1;
        // arg1: band
        public static final int BAND_LEVEL_UPDATED = 2;
    }

    // LoudnessEnhancer
    public static final class LoudnessEnhancerReqEvents {
        public static final int SET_ENABLED = 0;

        // arg2: targetGain (float)
        public static final int SET_TARGET_GAIN = 1;
    }

    public static final class LoudnessEnhancerNotifyEvents {
        public static final int ENABLED_STATE_UPDATED = 0;
        public static final int TARGET_GAIN_UPDATED = 1;
    }

    // Preset Reverb
    public static final class PresetReverbReqEvents {
        public static final int SET_ENABLED = 0;

        // arg1: preset
        public static final int SET_PRESET = 1;
    }

    public static final class PresetReverbNotifyEvents {
        public static final int ENABLED_STATE_UPDATED = 0;
        public static final int PRESET_UPDATED = 1;
    }

    // Environmental Reverb
    public static final class EnvironmentalReverbReqEvents {
        public static final int SET_ENABLED = 0;

        // arg1: preset
        public static final int SET_PRESET = 1;

        // arg1: parameter index, arg2: value (float)
        public static final int SET_PARAMETER = 2;

        // RARAMETER INDEX
        public static final int PARAM_INDEX_DECAY_HF_RATIO = 0;
        public static final int PARAM_INDEX_DECAY_TIME = 1;
        public static final int PARAM_INDEX_DENSITY = 2;
        public static final int PARAM_INDEX_DIFFUSION = 3;
        public static final int PARAM_INDEX_REFLECTIONS_DELAY = 4;
        public static final int PARAM_INDEX_REFLECTIONS_LEVEL = 5;
        public static final int PARAM_INDEX_REVERB_DELAY = 6;
        public static final int PARAM_INDEX_REVERB_LEVEL = 7;
        public static final int PARAM_INDEX_ROOM_HF_LEVEL = 8;
        public static final int PARAM_INDEX_ROOM_LEVEL = 9;
        public static final int PARAM_INDEX_ALL = 10;
    }

    public static final class EnvironmentalReverbNotifyEvents {
        public static final int ENABLED_STATE_UPDATED = 0;
        public static final int PRESET_UPDATED = 1;
        public static final int PARAMETER_UPDATED = 2;
    }

    // Visualizer
    public static final class VisualizerReqEvents {
        public static final int SET_WAVEFORM_ENABLED = 0;
        public static final int SET_FFT_ENABLED = 1;
        public static final int SET_SCALING_MODE = 2;
        public static final int SET_MEASURE_PEAK_ENABLED = 3;
        public static final int SET_MEASURE_RMS_ENABLED = 4;
    }

    public static final class VisualizerNotifyEvents {
        public static final int WAVEFORM_ENABLED_STATE_UPDATED = 0;
        public static final int FFT_ENABLED_STATE_UPDATED = 1;
        public static final int SCALING_MODE_UPDATED = 2;
        public static final int MEASURE_PEAK_ENABLED_STATE_UPDATED = 3;
        public static final int MEASURE_RMS_ENABLED_STATE_UPDATED = 4;
    }

    // HQEqualizer
    public static final class HQEqualizerReqEvents {
        public static final int SET_ENABLED = 0;
        // arg1: preset
        public static final int SET_PRESET = 1;
        // arg1: band, arg2: level (float)
        public static final int SET_BAND_LEVEL = 2;
    }

    public static final class HQEqualizerNotifyEvents {
        public static final int ENABLED_STATE_UPDATED = 0;
        // arg1: preset
        public static final int PRESET_UPDATED = 1;
        // arg1: band
        public static final int BAND_LEVEL_UPDATED = 2;
    }

    // PreAmp
    public static final class PreAmpReqEvents {
        public static final int SET_ENABLED = 0;
        // arg1: level (float)
        public static final int SET_LEVEL = 1;
    }

    public static final class PreAmpNotifyEvents {
        public static final int ENABLED_STATE_UPDATED = 0;
        public static final int LEVEL_UPDATED = 1;
    }

    // HQVisualizer
    public static final class HQVisualizerReqEvents {
        public static final int SET_WAVEFORM_ENABLED = 0;
        public static final int SET_FFT_ENABLED = 1;
        // arg1: window type
        public static final int SET_WINDOW_TYPE = 2;
    }

    public static final class HQVisualizerNotifyEvents {
        public static final int WAVEFORM_ENABLED_STATE_UPDATED = 0;
        public static final int FFT_ENABLED_STATE_UPDATED = 1;
        // arg1: window type
        public static final int WINDOW_TYPE_UPDATED = 2;
    }
}
