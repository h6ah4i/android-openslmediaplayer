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

import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.os.PowerManager;

import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEvent;
import com.h6ah4i.android.example.openslmediaplayer.app.framework.AppEventBus;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.BassBoostNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.BassBoostReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.EnvironmentalReverbNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.EnvironmentalReverbReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.EqualizerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.EqualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.HQEqualizerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.HQEqualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.HQVisualizerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.HQVisualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.LoudnessEnhancerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.LoudnessEnhancerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.NavigationDrawerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PlayerControlNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PlayerControlReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PreAmpNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PreAmpReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PresetReverbNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.PresetReverbReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.VirtualizerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.VirtualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.VisualizerNotifyEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.model.EventDefs.VisualizerReqEvents;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.EnvironmentalReverbPresetsUtil;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.EqualizerUtil;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.HQEqualizerUtil;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.MediaMetadataBuilder;
import com.h6ah4i.android.example.openslmediaplayer.app.utils.NotificationBuilder;
import com.h6ah4i.android.media.IBasicMediaPlayer;
import com.h6ah4i.android.media.IMediaPlayerFactory;
import com.h6ah4i.android.media.IReleasable;
import com.h6ah4i.android.media.audiofx.IBassBoost;
import com.h6ah4i.android.media.audiofx.IEnvironmentalReverb;
import com.h6ah4i.android.media.audiofx.IEqualizer;
import com.h6ah4i.android.media.audiofx.IHQVisualizer;
import com.h6ah4i.android.media.audiofx.ILoudnessEnhancer;
import com.h6ah4i.android.media.audiofx.IPreAmp;
import com.h6ah4i.android.media.audiofx.IPresetReverb;
import com.h6ah4i.android.media.audiofx.IVirtualizer;
import com.h6ah4i.android.media.audiofx.IVisualizer;
import com.h6ah4i.android.media.hybrid.HybridMediaPlayerFactory;
import com.h6ah4i.android.media.opensl.OpenSLMediaPlayerFactory;
import com.h6ah4i.android.media.standard.StandardMediaPlayer;
import com.h6ah4i.android.media.standard.StandardMediaPlayerFactory;

import java.io.PrintWriter;
import java.io.StringWriter;

public class GlobalAppController implements IReleasable {
    // constants
    public static final int PLAYER_STATE_IDLE = PlayerControlNotifyEvents.STATE_IDLE;
    public static final int PLAYER_STATE_INITIALIZED = PlayerControlNotifyEvents.STATE_INITIALIZED;
    public static final int PLAYER_STATE_PREPARING = PlayerControlNotifyEvents.STATE_PREPARING;
    public static final int PLAYER_STATE_PREPARED = PlayerControlNotifyEvents.STATE_PREPARED;
    public static final int PLAYER_STATE_STARTED = PlayerControlNotifyEvents.STATE_STARTED;
    public static final int PLAYER_STATE_PAUSED = PlayerControlNotifyEvents.STATE_PAUSED;
    public static final int PLAYER_STATE_STOPPED = PlayerControlNotifyEvents.STATE_STOPPED;
    public static final int PLAYER_STATE_PLAYBACK_COMPLETED = PlayerControlNotifyEvents.STATE_PLAYBACK_COMPLETED;
    public static final int PLAYER_STATE_END = PlayerControlNotifyEvents.STATE_END;
    public static final int PLAYER_STATE_ERROR = PlayerControlNotifyEvents.STATE_ERROR;

    private static final String PARCEL_KEY_MEDIA_PLAYER_STATE_STORE = "GlobalAppController.MediaPlayerStateStore";
    private static final String PARCEL_KEY_BASS_BOOST_STATE_STORE = "GlobalAppController.BassBoostStateStore";
    private static final String PARCEL_KEY_VIRTUALIZER_STATE_STORE = "GlobalAppController.VirtualizerStateStore";
    private static final String PARCEL_KEY_EQUALIZER_STATE_STORE = "GlobalAppController.EqualizerStateStore";
    private static final String PARCEL_KEY_LOUDNESS_ENHANCER_STATE_STORE = "GlobalAppController.LoudnessEnhancerStateStore";
    private static final String PARCEL_KEY_ENVIRONMENTAL_REVERB_STATE_STORE = "GlobalAppController.EnvironmentalReverbStateStore";
    private static final String PARCEL_KEY_PRESET_REVERB_STATE_STORE = "GlobalAppController.PresetReverbStateStore";
    private static final String PARCEL_KEY_VISUALIZER_STATE_STORE = "GlobalAppController.VisualizerStateStore";
    private static final String PARCEL_KEY_HQ_EQUALIZER_STATE_STORE = "GlobalAppController.HQEqualizerStateStore";
    private static final String PARCEL_KEY_PREAMP_STATE_STORE = "GlobalAppController.PreAmpStateStore";
    private static final String PARCEL_KEY_HQ_VISUALIZER_STATE_STORE = "GlobalAppController.HQVisualizerStateStore";

    private static final int FG_STATE_INACTIVE = -1;
    private static final int FG_STATE_PLAYER_0 = 1;
    private static final int FG_STATE_PLAYER_1 = 2;

    // internal classes
    private static class AppEventReceiver extends AppEventBus.Receiver<GlobalAppController> {
        private static final int[] FILTER = new int[]{
                EventDefs.Category.NAVIGATION_DRAWER,
                EventDefs.Category.PLAYER_CONTROL,
                EventDefs.Category.BASSBOOST,
                EventDefs.Category.VIRTUALIZER,
                EventDefs.Category.EQUALIZER,
                EventDefs.Category.LOUDNESS_EHNAHCER,
                EventDefs.Category.PRESET_REVERB,
                EventDefs.Category.ENVIRONMENTAL_REVERB,
                EventDefs.Category.VISUALIZER,
                EventDefs.Category.HQ_EQUALIZER,
                EventDefs.Category.PRE_AMP,
                EventDefs.Category.HQ_VISUALIZER,
        };

        public AppEventReceiver(GlobalAppController holder) {
            super(holder, FILTER);
        }

        @Override
        protected void onReceiveAppEvent(GlobalAppController holder, AppEvent event) {
            holder.onReceiveAppEvent(event);
        }
    }

    // fields
    private Context mContext;
    private AppEventBus mEventBus;
    private AppEventReceiver mAppEventReceiver;

    private IMediaPlayerFactory mFactory;
    private IBasicMediaPlayer[] mMediaPlayer = new IBasicMediaPlayer[2];
    private IBassBoost mBassBoost;
    private IVirtualizer mVirtualizer;
    private IEqualizer mEqualizer;
    private ILoudnessEnhancer mLoudnessEnhancer;
    private IEnvironmentalReverb mEnvironmentalReverb;
    private IPresetReverb mPresetReverb;
    private IVisualizer mVisualizer;
    private IEqualizer mHQEqualizer;
    private IPreAmp mPreAmp;
    private IHQVisualizer mHQVisualizer;

    private int mActivePlayerIndex = 0;
    private boolean mNextPlayerPrepared = false;
    private boolean mSwapPlayerPending = false;
    private int[] mPlayerState = new int[]{
            PLAYER_STATE_END, PLAYER_STATE_END
    };
    private MediaMetadata[] mMetadata = new MediaMetadata[2];
    private Service mHolderService;
    private int mCurrentForegroundState = FG_STATE_INACTIVE;

    // --- these fields are saved to / restored from Bundle --
    private MediaPlayerStateStore mPlayerStateStore;
    private BassBoostStateStore mBassBoostStateStore;
    private VirtualizerStateStore mVirtualizerStateStore;
    private EqualizerStateStore mEqualizerStateStore;
    private LoudnessEnhancerStateStore mLoudnessEnhancerStateStore;
    private EnvironmentalReverbStateStore mEnvironmentalReverbStateStore;
    private PresetReverbStateStore mPresetReverbStateStore;
    private VisualizerStateStore mVisualizerStateStore;
    private HQEqualizerStateStore mHQEqualizerStateStore;
    private PreAmpStateStore mPreAmpStateStore;
    private HQVisualizerStateStore mHQVisualizerStateStore;
    //

    // MediaPlayer event listeners
    private IBasicMediaPlayer.OnBufferingUpdateListener mOnBufferingUpdateListener = new IBasicMediaPlayer.OnBufferingUpdateListener() {
        @Override
        public void onBufferingUpdate(IBasicMediaPlayer mp, int percent) {
            GlobalAppController.this.onPlayerBufferingUpdate(mp);
        }
    };

    private IBasicMediaPlayer.OnCompletionListener mOnCompletionListener = new IBasicMediaPlayer.OnCompletionListener() {
        @Override
        public void onCompletion(IBasicMediaPlayer mp) {
            GlobalAppController.this.onPlayerCompletion(mp);
        }
    };

    private IBasicMediaPlayer.OnErrorListener mOnErrorListener = new IBasicMediaPlayer.OnErrorListener() {
        @Override
        public boolean onError(IBasicMediaPlayer mp, int what, int extra) {
            return GlobalAppController.this.onPlayerError(mp, what, extra);
        }
    };

    private IBasicMediaPlayer.OnInfoListener mOnInfoListener = new IBasicMediaPlayer.OnInfoListener() {
        @Override
        public boolean onInfo(IBasicMediaPlayer mp, int what, int extra) {
            return GlobalAppController.this.onPlayerInfo(mp, what, extra);
        }
    };

    private IBasicMediaPlayer.OnPreparedListener mOnPreparedListener = new IBasicMediaPlayer.OnPreparedListener() {
        @Override
        public void onPrepared(IBasicMediaPlayer mp) {
            GlobalAppController.this.onPlayerPrepared(mp);
        }
    };

    private IBasicMediaPlayer.OnSeekCompleteListener mOnSeekCompleteListener = new IBasicMediaPlayer.OnSeekCompleteListener() {
        @Override
        public void onSeekComplete(IBasicMediaPlayer mp) {
            GlobalAppController.this.onPlayerSeekComplete(mp);
        }
    };

    public GlobalAppController(Context context, Bundle savedInstanceState) {
        mContext = context;

        // setup event bus
        mEventBus = new AppEventBus(mContext);

        // setup event receiver
        mAppEventReceiver = new AppEventReceiver(this);
        mEventBus.register(mAppEventReceiver);

        if (savedInstanceState == null) {
            // create audio effects state stores
            mPlayerStateStore = new MediaPlayerStateStore();
            mBassBoostStateStore = new BassBoostStateStore();
            mVirtualizerStateStore = new VirtualizerStateStore();
            mEqualizerStateStore = new EqualizerStateStore();
            mLoudnessEnhancerStateStore = new LoudnessEnhancerStateStore();
            mEnvironmentalReverbStateStore = new EnvironmentalReverbStateStore();
            mPresetReverbStateStore = new PresetReverbStateStore();
            mVisualizerStateStore = new VisualizerStateStore();
            mHQEqualizerStateStore = new HQEqualizerStateStore();
            mPreAmpStateStore = new PreAmpStateStore();
            mHQVisualizerStateStore = new HQVisualizerStateStore();
        } else {
            // restore from savedInstanceState
            final Bundle s = savedInstanceState;
            mPlayerStateStore = s.getParcelable(PARCEL_KEY_MEDIA_PLAYER_STATE_STORE);
            mBassBoostStateStore = s.getParcelable(PARCEL_KEY_BASS_BOOST_STATE_STORE);
            mVirtualizerStateStore = s.getParcelable(PARCEL_KEY_VIRTUALIZER_STATE_STORE);
            mEqualizerStateStore = s.getParcelable(PARCEL_KEY_EQUALIZER_STATE_STORE);
            mLoudnessEnhancerStateStore = s.getParcelable(PARCEL_KEY_LOUDNESS_ENHANCER_STATE_STORE);
            mEnvironmentalReverbStateStore = s.getParcelable(PARCEL_KEY_ENVIRONMENTAL_REVERB_STATE_STORE);
            mPresetReverbStateStore = s.getParcelable(PARCEL_KEY_PRESET_REVERB_STATE_STORE);
            mVisualizerStateStore = s.getParcelable(PARCEL_KEY_VISUALIZER_STATE_STORE);
            mHQEqualizerStateStore = s.getParcelable(PARCEL_KEY_HQ_EQUALIZER_STATE_STORE);
            mPreAmpStateStore = s.getParcelable(PARCEL_KEY_PREAMP_STATE_STORE);
            mHQVisualizerStateStore = s.getParcelable(PARCEL_KEY_HQ_VISUALIZER_STATE_STORE);
        }

        // create media player factory
        setPlayerImplType(getPlayerStateStore().getPlayerImplType(), true);
    }

    @Override
    public void release() {
        releaseAllPlayerResources();
        releaseFactory();

        mOnBufferingUpdateListener = null;
        mOnCompletionListener = null;
        mOnErrorListener = null;
        mOnInfoListener = null;
        mOnPreparedListener = null;
        mOnSeekCompleteListener = null;

        mEventBus.unregister(mAppEventReceiver);
        mAppEventReceiver = null;

        mEventBus = null;
        mContext = null;
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putParcelable(
                PARCEL_KEY_MEDIA_PLAYER_STATE_STORE,
                mPlayerStateStore);
        outState.putParcelable(
                PARCEL_KEY_BASS_BOOST_STATE_STORE,
                mBassBoostStateStore);
        outState.putParcelable(
                PARCEL_KEY_VIRTUALIZER_STATE_STORE,
                mVirtualizerStateStore);
        outState.putParcelable(
                PARCEL_KEY_EQUALIZER_STATE_STORE,
                mEqualizerStateStore);
        outState.putParcelable(
                PARCEL_KEY_ENVIRONMENTAL_REVERB_STATE_STORE,
                mEnvironmentalReverbStateStore);
        outState.putParcelable(
                PARCEL_KEY_PRESET_REVERB_STATE_STORE,
                mPresetReverbStateStore);
        outState.putParcelable(
                PARCEL_KEY_VISUALIZER_STATE_STORE,
                mVisualizerStateStore);
        outState.putParcelable(
                PARCEL_KEY_HQ_EQUALIZER_STATE_STORE,
                mHQEqualizerStateStore);
        outState.putParcelable(
                PARCEL_KEY_PREAMP_STATE_STORE,
                mPreAmpStateStore);
    }

    public AppEventBus eventBus() {
        return mEventBus;
    }

    public MediaPlayerStateStore getPlayerStateStore() {
        return mPlayerStateStore;
    }

    public BassBoostStateStore getBassBoostStateStore() {
        return mBassBoostStateStore;
    }

    public VirtualizerStateStore getVirtualizerStateStore() {
        return mVirtualizerStateStore;
    }

    public EqualizerStateStore getEqualizerStateStore() {
        return mEqualizerStateStore;
    }

    public LoudnessEnhancerStateStore getLoudnessEnhancerStateStore() {
        return mLoudnessEnhancerStateStore;
    }

    public EnvironmentalReverbStateStore getEnvironmentalReverbStateStore() {
        return mEnvironmentalReverbStateStore;
    }

    public PresetReverbStateStore getPresetReverbStateStore() {
        return mPresetReverbStateStore;
    }

    public VisualizerStateStore getVisualizerStateStore() {
        return mVisualizerStateStore;
    }

    public HQEqualizerStateStore getHQEqualizerStateStore() {
        return mHQEqualizerStateStore;
    }

    public PreAmpStateStore getPreAmpStateStore() {
        return mPreAmpStateStore;
    }

    public HQVisualizerStateStore getHQVisualizerStateStore() {
        return mHQVisualizerStateStore;
    }

    public int getActivePlayerState() {
        return getPlayerState(getActivePlayerIndex());
    }

    public int getPlayerState(int index) {
        return mPlayerState[index];
    }

    private MediaMetadata getActiveMediaMetadata() {
        return mMetadata[getActivePlayerIndex()];
    }

    public float getNormalizedPlayerCurrentPosition() {
        int currentPosition = getActivePlayerCurrentPosition();
        int duration = getActivePlayerDuration();

        return (duration == 0) ? 0.0f : Math.min((float) currentPosition / duration, 1.0f);
    }

    public int getActivePlayerCurrentPosition() {
        IBasicMediaPlayer player = getActivePlayer();

        switch (getActivePlayerState()) {
            case PLAYER_STATE_STARTED:
            case PLAYER_STATE_PAUSED:
            case PLAYER_STATE_PLAYBACK_COMPLETED:
                return (player != null) ? player.getCurrentPosition() : 0;
            default:
                return 0;
        }
    }

    public int getActivePlayerDuration() {
        IBasicMediaPlayer player = getActivePlayer();

        switch (getActivePlayerState()) {
            case PLAYER_STATE_PREPARED:
            case PLAYER_STATE_STARTED:
            case PLAYER_STATE_PAUSED:
            case PLAYER_STATE_PLAYBACK_COMPLETED:
            case PLAYER_STATE_STOPPED:
                return (player != null) ? player.getDuration() : 0;
            default:
                return 0;
        }
    }

    /* package */void onPlayerSeekComplete(IBasicMediaPlayer mp) {
    }

    /* package */void onPlayerBufferingUpdate(IBasicMediaPlayer mp) {
    }

    /* package */void onPlayerCompletion(IBasicMediaPlayer mp) {
        int index = getMediaPlayerIndex(mp);

        if (index >= 0) {
            int origPlayerState = getPlayerState(index);

            if (getPlayerState(index) != PLAYER_STATE_ERROR) {
                setPlayerState(index, PLAYER_STATE_PLAYBACK_COMPLETED);
            }

            if (origPlayerState == PLAYER_STATE_STARTED) {
                if (swapActivePlayer(index)) {
                    setPlayerState(getActivePlayerIndex(), PLAYER_STATE_STARTED);
                }
            } else {
                mSwapPlayerPending = true;
            }
        }
    }

    private boolean swapActivePlayer(int index) {
        int next = nextPlayerIndex(index);

        if (mNextPlayerPrepared) {
            // next player is automatically started

            mActivePlayerIndex = next;

            return true;
        } else {
            return false;
        }
    }

    /* package */boolean onPlayerError(IBasicMediaPlayer mp, int what, int extra) {
        int index = getMediaPlayerIndex(mp);

        if (index >= 0) {
            postInfoOrErrorAppEvent(
                    EventDefs.Category.NOTIFY_PLAYER_CONTROL,
                    PlayerControlNotifyEvents.NOTIFY_PLAYER_ERROR,
                    index, 0,
                    what, extra);

            setPlayerState(index, PLAYER_STATE_ERROR);
        }

        // false: raise onCompletion event
        return false;
    }

    /* package */boolean onPlayerInfo(IBasicMediaPlayer mp, int what, int extra) {
        int index = getMediaPlayerIndex(mp);

        if (index >= 0) {
            postInfoOrErrorAppEvent(
                    EventDefs.Category.NOTIFY_PLAYER_CONTROL,
                    PlayerControlNotifyEvents.NOTIFY_PLAYER_INFO,
                    index, 0,
                    what, extra);
        }

        return false;
    }

    /* package */void onPlayerPrepared(IBasicMediaPlayer mp) {
        int index = getMediaPlayerIndex(mp);

        if (index >= 0) {
            setPlayerState(index, PLAYER_STATE_PREPARED);

            applyMediaPlayerStates(mp, getPlayerStateStore());
            applyAttachedEuxEffectSettings(
                    mp, getPlayerStateStore().getSelectedAuxEffectType());
        }

        // If both players are successfully prepared
        // set them up as a next player
        if (getPlayerState(0) == PLAYER_STATE_PREPARED &&
                getPlayerState(1) == PLAYER_STATE_PREPARED) {

            IBasicMediaPlayer player0 = getPlayer(0);
            IBasicMediaPlayer player1 = getPlayer(1);

            player0.setNextMediaPlayer(player1);
            player1.setNextMediaPlayer(player0);

            mNextPlayerPrepared = true;
        }
    }

    /* package */void onReceiveAppEvent(AppEvent event) {
        switch (event.category) {
            case EventDefs.Category.NAVIGATION_DRAWER:
                onReceiveNavigationDrawerEvent(event);
                break;
            case EventDefs.Category.PLAYER_CONTROL:
                onReceivePlayerControlEvent(event);
                break;
            case EventDefs.Category.BASSBOOST:
                onReceiveBassBoostEvent(event);
                break;
            case EventDefs.Category.VIRTUALIZER:
                onReceiveVirtualizerEvent(event);
                break;
            case EventDefs.Category.EQUALIZER:
                onReceiveEqualizerEvent(event);
                break;
            case EventDefs.Category.LOUDNESS_EHNAHCER:
                onReceiveLoudnessEnhancerEvent(event);
                break;
            case EventDefs.Category.PRESET_REVERB:
                onReceivePresetReverbEvent(event);
                break;
            case EventDefs.Category.ENVIRONMENTAL_REVERB:
                onReceiveEnvironmentalReverbEvent(event);
                break;
            case EventDefs.Category.VISUALIZER:
                onReceiveVisualizerEvent(event);
                break;
            case EventDefs.Category.HQ_EQUALIZER:
                onReceiveHQEqualizerEvent(event);
                break;
            case EventDefs.Category.PRE_AMP:
                onReceivePreAmpEvent(event);
                break;
            case EventDefs.Category.HQ_VISUALIZER:
                onReceiveHQVisualizerEvent(event);
                break;
        }
    }

    private void onReceiveNavigationDrawerEvent(AppEvent event) {
        switch (event.event) {
            case NavigationDrawerReqEvents.PLAYER_SET_IMPL_TYPE: {
                int type = event.arg1;
                setPlayerImplType(type, false);
            }
            break;
            case NavigationDrawerReqEvents.CLICK_ITEM_ENABLE_SWITCH: {
                // enable/disable audio effects

                boolean enabled = (event.arg2 != 0);

                switch (event.arg1) {
                    case NavigationDrawerReqEvents.SECTION_INDEX_BASSBOOST:
                        setBassBoostEnabled(enabled);
                        break;
                    case NavigationDrawerReqEvents.SECTION_INDEX_VIRTUALIZER:
                        setVirtualizerEnabled(enabled);
                        break;
                    case NavigationDrawerReqEvents.SECTION_INDEX_EQUALIZER:
                        setEqualizerEnabled(enabled);
                        break;
                    case NavigationDrawerReqEvents.SECTION_INDEX_LOUDNESS_ENHANCER:
                        setLoudnessEnhancerEnabled(enabled);
                        break;
                    case NavigationDrawerReqEvents.SECTION_INDEX_PRESET_REVERB:
                        setPresetReverbEnabled(enabled);
                        break;
                    case NavigationDrawerReqEvents.SECTION_INDEX_ENVIRONMENTAL_REVERB:
                        setEnvironmentalReverbEnabled(enabled);
                        break;
                    case NavigationDrawerReqEvents.SECTION_INDEX_HQ_EQUALIZER:
                        setHQEqualizerEnabled(enabled);
                        break;
                }
            }
            break;
        }
    }

    private void onReceivePlayerControlEvent(AppEvent event) {
        MediaPlayerStateStore state = getPlayerStateStore();
        switch (event.event) {
            case PlayerControlReqEvents.SONG_PICKED: {
                Uri uri = (Uri) event.extras.getParcelable(PlayerControlReqEvents.EXTRA_URI);
                state.setMediaUri(event.arg1, uri);
            }
            break;
            case PlayerControlReqEvents.PLAYER_CREATE: {
                releaseAllPlayerResources();

                IBasicMediaPlayer player0 = createPlayer(0);
                IBasicMediaPlayer player1 = createPlayer(1);

                if (player0 instanceof StandardMediaPlayer) {
                    // Share the same audio session
                    player1.setAudioSessionId(player0.getAudioSessionId());
                } else {
                    // OpenSLMediaPlayer does not support setAudioSessionId() method.
                    // However audio effects are shared if player instances were created
                    // by the same factory instance.
                }

                applyBassBoostStates(mBassBoostStateStore);
                applyVirtualizerStates(mVirtualizerStateStore);
                applyEqualizerStates(mEqualizerStateStore);
                applyLoudnessEnhancerStates(mLoudnessEnhancerStateStore);
                applyEnvironmentalReverbStates(mEnvironmentalReverbStateStore);
                applyPresetReverbStates(mPresetReverbStateStore);
                applyHQEqualizerStates(mHQEqualizerStateStore);
                applyPreAmpStates(mPreAmpStateStore);

                playerSetLooping(0, state.isLooping());
                playerSetLooping(1, state.isLooping());

                playerSetVolume(0, state.getVolumeLeft(), state.getVolumeRight());
                playerSetVolume(1, state.getVolumeLeft(), state.getVolumeRight());

                playerSetAuxSendLevel(0, state.getAuxEffectSendLevel());
                playerSetAuxSendLevel(1, state.getAuxEffectSendLevel());

                setPlayerState(0, PLAYER_STATE_IDLE);
                setPlayerState(1, PLAYER_STATE_IDLE);

                // enable pre.amp
                setPreAmpEnabled(true);
            }
            break;
            case PlayerControlReqEvents.PLAYER_SET_DATA_SOURCE: {

                playerSetDataSource(0);
                playerSetDataSource(1);
            }
            break;
            case PlayerControlReqEvents.PLAYER_PREPARE: {
                playerPrepare(0);
                playerPrepare(1);
            }
            break;
            case PlayerControlReqEvents.PLAYER_PREPARE_ASYNC: {
                playerPrepareAsync(0);
                playerPrepareAsync(1);
            }
            break;
            case PlayerControlReqEvents.PLAYER_START: {

                // process pending player swapping
                if (mSwapPlayerPending) {
                    mSwapPlayerPending = false;

                    if (!getPlayerStateStore().isLooping()) {
                        swapActivePlayer(getActivePlayerIndex());
                    }
                }

                playerStart(getActivePlayerIndex());
            }
            break;
            case PlayerControlReqEvents.PLAYER_PAUSE: {
                playerPause(getActivePlayerIndex());
            }
            break;
            case PlayerControlReqEvents.PLAYER_STOP: {
                playerStop(0);
                playerStop(1);
                resetPlayerStateControlVariables();
            }
            break;
            case PlayerControlReqEvents.PLAYER_RESET: {
                playerReset(0);
                playerReset(1);
                resetPlayerStateControlVariables();
            }
            break;
            case PlayerControlReqEvents.PLAYER_RELEASE: {
                playerRelease(0);
                playerRelease(1);
                resetPlayerStateControlVariables();
            }
            break;
            case PlayerControlReqEvents.PLAYER_SEEK_TO: {
                float position = event.getArg2AsFloat();
                activePlayerSeekTo(position);
            }
            break;
            case PlayerControlReqEvents.PLAYER_SET_VOLUME_LEFT: {
                state.setVolumeLeft(event.getArg2AsFloat());

                // apply
                playerSetVolume(0, state.getVolumeLeft(), state.getVolumeRight());
                playerSetVolume(1, state.getVolumeLeft(), state.getVolumeRight());
            }
            break;
            case PlayerControlReqEvents.PLAYER_SET_VOLUME_RIGHT: {
                state.setVolumeRight(event.getArg2AsFloat());

                // apply
                playerSetVolume(0, state.getVolumeLeft(), state.getVolumeRight());
                playerSetVolume(1, state.getVolumeLeft(), state.getVolumeRight());
            }
            break;
            case PlayerControlReqEvents.PLAYER_SET_LOOPING: {
                boolean looping = (event.arg1 != 0);

                state.setLooping(looping);

                // apply
                playerSetLooping(0, state.isLooping());
                playerSetLooping(1, state.isLooping());
            }
            break;
            case PlayerControlReqEvents.PLAYER_ATTACH_AUX_EFFECT: {
                int effectType = event.arg1;

                if (state.getSelectedAuxEffectType() == effectType)
                    return;

                state.setSelectedAuxEffectType(effectType);

                // apply
                checkStateAndApplyAttachedAuxEffectSettings(0);
                checkStateAndApplyAttachedAuxEffectSettings(1);
            }
            break;
            case PlayerControlReqEvents.PLAYER_SET_AUX_SEND_LEVEL: {
                state.setAuxEffectSendLevel(event.getArg2AsFloat());

                // apply
                playerSetAuxSendLevel(0, state.getAuxEffectSendLevel());
                playerSetAuxSendLevel(1, state.getAuxEffectSendLevel());
            }
            break;
        }
    }

    private void resetPlayerStateControlVariables() {
        mActivePlayerIndex = 0;
        mNextPlayerPrepared = false;
        mSwapPlayerPending = false;
    }

    private void playerPrepare(int index) {
        IBasicMediaPlayer player = getPlayer(index);

        if (player != null) {
            try {
                player.prepare();
                setPlayerState(index, PLAYER_STATE_PREPARED);
            } catch (Exception e) {
                notifyPlayerExceptionOccurred(index, "prepare", e);
            }
        }
    }

    private void playerPrepareAsync(int index) {
        IBasicMediaPlayer player = getPlayer(index);

        if (player != null) {
            try {
                player.prepareAsync();
                setPlayerState(index, PLAYER_STATE_PREPARING);
            } catch (Exception e) {
                notifyPlayerExceptionOccurred(index, "prepareAsync", e);
            }
        }
    }

    private void playerStart(int index) {
        IBasicMediaPlayer player = getPlayer(index);
        boolean isErrorState = getPlayerState(index) == PLAYER_STATE_ERROR;

        if (player != null && !isErrorState) {
            try {
                player.start();
                setPlayerState(index, PLAYER_STATE_STARTED);
            } catch (Exception e) {
                notifyPlayerExceptionOccurred(index, "start", e);
            }
        }
    }

    private void playerPause(int index) {
        IBasicMediaPlayer player = getPlayer(index);
        boolean isErrorState = getPlayerState(index) == PLAYER_STATE_ERROR;

        if (player != null && !isErrorState) {
            try {
                player.pause();
                setPlayerState(index, PLAYER_STATE_PAUSED);
            } catch (Exception e) {
                notifyPlayerExceptionOccurred(index, "pause", e);
            }
        }
    }

    private void playerStop(int index) {
        IBasicMediaPlayer player = getPlayer(index);
        boolean isErrorState = getPlayerState(index) == PLAYER_STATE_ERROR;

        if (player != null && !isErrorState) {
            try {
                player.stop();
                setPlayerState(index, PLAYER_STATE_STOPPED);
            } catch (Exception e) {
                notifyPlayerExceptionOccurred(index, "stop", e);
            }
        }
    }

    private void playerReset(int index) {
        IBasicMediaPlayer player = getPlayer(index);

        if (player != null) {
            try {
                player.reset();
                mMetadata[index] = null;
                setPlayerState(index, PLAYER_STATE_IDLE);
            } catch (Exception e) {
                notifyPlayerExceptionOccurred(index, "reset", e);
            }
        }
    }

    private void playerRelease(int index) {
        IBasicMediaPlayer player = getPlayer(index);

        if (player != null) {
            try {
                player.release();
            } catch (Exception e) {
                notifyPlayerExceptionOccurred(index, "release", e);
            }

            mMediaPlayer[index] = null;
            mMetadata[index] = null;
            setPlayerState(index, PLAYER_STATE_END);
        }
    }

    private void activePlayerSeekTo(float position) {
        int index = getActivePlayerIndex();
        IBasicMediaPlayer player = getPlayer(index);

        int msec = (int) (position * getActivePlayerDuration());

        if (player != null) {
            try {
                player.seekTo(msec);
                mSwapPlayerPending = false;
            } catch (Exception e) {
                notifyPlayerExceptionOccurred(index, "seekTo(msec = " + msec + ")", e);
            }
        }
    }

    private void playerSetVolume(int index, float leftVolume, float rightVolume) {
        IBasicMediaPlayer player = getPlayer(index);

        if (player != null) {
            player.setVolume(leftVolume, rightVolume);
        }
    }

    private void playerSetLooping(int index, boolean looping) {
        IBasicMediaPlayer player = getPlayer(index);

        if (player != null) {
            player.setLooping(looping);
        }
    }

    private void playerSetDataSource(int index) {
        IBasicMediaPlayer player = getPlayer(index);
        Uri uri = getPlayerStateStore().getMediaUri(index);

        if (player != null && uri != null) {
            try {
                MediaMetadata metadata;
                metadata = MediaMetadataBuilder.create(mContext, uri);
                player.setDataSource(mContext, uri);
                mMetadata[index] = metadata;
                setPlayerState(index, PLAYER_STATE_INITIALIZED);
            } catch (final Exception e) {
                notifyPlayerExceptionOccurred(index, "setDataSource", e);
            }
        }
    }

    private void playerSetAuxSendLevel(int index, float level) {
        IBasicMediaPlayer player = getPlayer(index);

        if (player != null) {
            player.setAuxEffectSendLevel(level);
        }
    }

    private void checkStateAndApplyAttachedAuxEffectSettings(int index) {
        switch (getPlayerState(index)) {
            case PLAYER_STATE_PREPARED:
            case PLAYER_STATE_STARTED:
            case PLAYER_STATE_PAUSED:
            case PLAYER_STATE_PLAYBACK_COMPLETED:
                applyAttachedEuxEffectSettings(
                        getPlayer(index),
                        getPlayerStateStore().getSelectedAuxEffectType());
                break;
        }
    }

    private void onReceiveBassBoostEvent(AppEvent event) {
        switch (event.event) {
            case BassBoostReqEvents.SET_ENABLED: {
                boolean enabled = (event.arg1 != 0);
                setBassBoostEnabled(enabled);
            }
            break;
            case BassBoostReqEvents.SET_STRENGTH: {
                float strength = event.getArg2AsFloat();
                IBassBoost bassboost = getBassBoost();
                BassBoostStateStore state = getBassBoostStateStore();

                state.setNormalizedStrength(strength);

                if (bassboost != null) {
                    bassboost.setStrength(state.getSettings().strength);
                }
            }
            break;
        }
    }

    private void onReceiveVirtualizerEvent(AppEvent event) {
        switch (event.event) {
            case VirtualizerReqEvents.SET_ENABLED: {
                boolean enabled = (event.arg1 != 0);
                setVirtualizerEnabled(enabled);
            }
            break;
            case VirtualizerReqEvents.SET_STRENGTH: {
                float strength = event.getArg2AsFloat();
                IVirtualizer virtualizer = getVirtualizer();
                VirtualizerStateStore state = getVirtualizerStateStore();

                state.setNormalizedStrength(strength);

                if (virtualizer != null) {
                    virtualizer.setStrength(state.getSettings().strength);
                }
            }
            break;
        }
    }

    private void onReceiveEqualizerEvent(AppEvent event) {
        switch (event.event) {
            case EqualizerReqEvents.SET_ENABLED: {
                boolean enabled = (event.arg1 != 0);
                setEqualizerEnabled(enabled);
            }
            break;
            case EqualizerReqEvents.SET_PRESET: {
                short preset = (short) event.arg1;
                IEqualizer equalizer = getEqualizer();
                EqualizerStateStore state = getEqualizerStateStore();

                state.getSettings().curPreset = preset;

                // apply
                if (equalizer != null) {
                    equalizer.usePreset(state.getSettings().curPreset);
                }

                // update band levels
                state.setSettings(
                        EqualizerUtil.PRESETS[state.getSettings().curPreset].settings);

                // notify
                postAppEvent(
                        EventDefs.Category.NOTIFY_EQUALIZER,
                        EqualizerNotifyEvents.PRESET_UPDATED, preset, 0);
                postAppEvent(
                        EventDefs.Category.NOTIFY_EQUALIZER,
                        EqualizerNotifyEvents.BAND_LEVEL_UPDATED,
                        -1 /* all bands */, 0);
            }
            break;
            case EqualizerReqEvents.SET_BAND_LEVEL: {
                short band = (short) event.arg1;
                float level = event.getArg2AsFloat();
                IEqualizer equalizer = getEqualizer();
                EqualizerStateStore state = getEqualizerStateStore();

                state.setNormalizedBandLevel(band, level, true);

                // apply
                if (equalizer != null) {
                    equalizer.setBandLevel(
                            band, state.getSettings().bandLevels[band]);
                }

                // notify
                postAppEvent(
                        EventDefs.Category.NOTIFY_EQUALIZER,
                        EqualizerNotifyEvents.BAND_LEVEL_UPDATED, band, 0);
            }
            break;
        }
    }

    private void onReceiveLoudnessEnhancerEvent(AppEvent event) {
        switch (event.event) {
            case LoudnessEnhancerReqEvents.SET_ENABLED: {
                boolean enabled = (event.arg1 != 0);
                setLoudnessEnhancerEnabled(enabled);
            }
            break;
            case LoudnessEnhancerReqEvents.SET_TARGET_GAIN: {
                float gainmB = event.getArg2AsFloat();
                ILoudnessEnhancer loudnessEnhancer = getLoudnessEnhancer();
                LoudnessEnhancerStateStore state = getLoudnessEnhancerStateStore();

                state.setNormalizedTargetGainmB(gainmB);

                if (loudnessEnhancer != null) {
                    loudnessEnhancer.setTargetGain(state.getSettings().targetGainmB);
                }
            }
            break;
        }
    }

    private void onReceivePresetReverbEvent(AppEvent event) {
        IPresetReverb presetreverb = getPresetReverb();
        PresetReverbStateStore state = getPresetReverbStateStore();

        switch (event.event) {
            case PresetReverbReqEvents.SET_ENABLED: {
                boolean enabled = (event.arg1 != 0);
                setPresetReverbEnabled(enabled);
            }
            break;
            case PresetReverbReqEvents.SET_PRESET:
                state.getSettings().preset = (short) event.arg1;

                // apply
                if (presetreverb != null) {
                    presetreverb.setPreset(state.getSettings().preset);
                }

                // notify preset changed
                postAppEvent(
                        EventDefs.Category.NOTIFY_PRESET_REVERB,
                        PresetReverbNotifyEvents.PRESET_UPDATED,
                        0, 0);
                break;
        }
    }

    private void onReceiveEnvironmentalReverbEvent(AppEvent event) {
        IEnvironmentalReverb envreverb = getEnvironmentalReverb();
        EnvironmentalReverbStateStore state = getEnvironmentalReverbStateStore();

        switch (event.event) {
            case EnvironmentalReverbReqEvents.SET_ENABLED: {
                boolean enabled = (event.arg1 != 0);
                setEnvironmentalReverbEnabled(enabled);
            }
            break;
            case EnvironmentalReverbReqEvents.SET_PRESET:
                state.setPreset(event.arg1);
                if (state.getPreset() >= 0) {
                    state.setSettings(EnvironmentalReverbPresetsUtil.getPreset(state.getPreset()));
                }

                // apply
                if (envreverb != null) {
                    envreverb.setProperties(state.getSettings());
                }

                // notify preset changed
                postAppEvent(
                        EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB,
                        EnvironmentalReverbNotifyEvents.PRESET_UPDATED,
                        0, 0);

                // notify all parameters updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB,
                        EnvironmentalReverbNotifyEvents.PARAMETER_UPDATED,
                        EnvironmentalReverbReqEvents.PARAM_INDEX_ALL,
                        0);

                break;
            case EnvironmentalReverbReqEvents.SET_PARAMETER:
                state.setPreset(-1); // unset current preset

                state.setNormalizedParameter(event.arg1, event.getArg2AsFloat());

                // apply
                if (envreverb != null) {
                    applyEnvironmentalReverbPresetParam(envreverb, state, event.arg1);
                }

                // notify parameter changed
                postAppEvent(
                        EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB,
                        EnvironmentalReverbNotifyEvents.PARAMETER_UPDATED,
                        event.arg1, 0);

                // notify preset changed
                postAppEvent(
                        EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB,
                        EnvironmentalReverbNotifyEvents.PRESET_UPDATED,
                        0, 0);
                break;
        }
    }

    private void onReceiveVisualizerEvent(AppEvent event) {
        VisualizerStateStore state = getVisualizerStateStore();

        // NOTE:
        // Only save the enabled state, and don't apply to
        // the effect object here.

        switch (event.event) {
            case VisualizerReqEvents.SET_WAVEFORM_ENABLED: {
                boolean enabled = (event.arg1 != 0);

                state.setCaptureWaveformEnabled(enabled);

                // notify enabled state updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_VISUALIZER,
                        VisualizerNotifyEvents.WAVEFORM_ENABLED_STATE_UPDATED,
                        state.isCaptureWaveformEnabled() ? 1 : 0,
                        0);
            }
            break;
            case VisualizerReqEvents.SET_FFT_ENABLED: {
                boolean enabled = (event.arg1 != 0);

                state.setCaptureFftEnabled(enabled);

                // notify enabled state updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_VISUALIZER,
                        VisualizerNotifyEvents.FFT_ENABLED_STATE_UPDATED,
                        state.isCaptureFftEnabled() ? 1 : 0,
                        0);
            }
            break;
            case VisualizerReqEvents.SET_SCALING_MODE:
                int scalingMode = event.arg1;

                state.setScalingMode(scalingMode);

                // notify enabled state updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_VISUALIZER,
                        VisualizerNotifyEvents.SCALING_MODE_UPDATED,
                        state.getScalingMode(),
                        0);
                break;
            case VisualizerReqEvents.SET_MEASURE_PEAK_ENABLED: {
                boolean enabled = (event.arg1 != 0);

                state.setMeasurementPeakEnabled(enabled);

                // notify enabled state updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_VISUALIZER,
                        VisualizerNotifyEvents.MEASURE_PEAK_ENABLED_STATE_UPDATED,
                        state.isMeasurementPeakEnabled() ? 1 : 0,
                        0);
            }
            break;
            case VisualizerReqEvents.SET_MEASURE_RMS_ENABLED: {
                boolean enabled = (event.arg1 != 0);

                state.setMeasurementRmsEnabled(enabled);

                // notify enabled state updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_VISUALIZER,
                        VisualizerNotifyEvents.MEASURE_RMS_ENABLED_STATE_UPDATED,
                        state.isMeasurementRmsEnabled() ? 1 : 0,
                        0);
            }
            break;
        }
    }

    private void onReceiveHQEqualizerEvent(AppEvent event) {
        switch (event.event) {
            case HQEqualizerReqEvents.SET_ENABLED: {
                boolean enabled = (event.arg1 != 0);
                setHQEqualizerEnabled(enabled);
            }
            break;
            case HQEqualizerReqEvents.SET_PRESET: {
                short preset = (short) event.arg1;
                IEqualizer equalizer = getHQEqualizer();
                HQEqualizerStateStore state = getHQEqualizerStateStore();

                state.getSettings().curPreset = preset;

                // apply
                if (equalizer != null) {
                    equalizer.usePreset(state.getSettings().curPreset);
                }

                // update band levels
                state.setSettings(
                        HQEqualizerUtil.PRESETS[state.getSettings().curPreset].settings);

                // notify
                postAppEvent(
                        EventDefs.Category.NOTIFY_HQ_EQUALIZER,
                        HQEqualizerNotifyEvents.PRESET_UPDATED, preset, 0);
                postAppEvent(
                        EventDefs.Category.NOTIFY_HQ_EQUALIZER,
                        HQEqualizerNotifyEvents.BAND_LEVEL_UPDATED,
                        -1 /* all bands */, 0);
            }
            break;
            case HQEqualizerReqEvents.SET_BAND_LEVEL: {
                short band = (short) event.arg1;
                float level = event.getArg2AsFloat();
                IEqualizer equalizer = getHQEqualizer();
                HQEqualizerStateStore state = getHQEqualizerStateStore();

                state.setNormalizedBandLevel(band, level, true);

                // apply
                if (equalizer != null) {
                    equalizer.setBandLevel(
                            band, state.getSettings().bandLevels[band]);
                }

                // notify
                postAppEvent(
                        EventDefs.Category.NOTIFY_HQ_EQUALIZER,
                        HQEqualizerNotifyEvents.BAND_LEVEL_UPDATED, band, 0);
            }
            break;
        }
    }

    private void onReceivePreAmpEvent(AppEvent event) {
        switch (event.event) {
            case PreAmpReqEvents.SET_ENABLED: {
                boolean enabled = (event.arg1 != 0);
                setPreAmpEnabled(enabled);
            }
            break;
            case PreAmpReqEvents.SET_LEVEL: {
                float level = event.getArg1AsFloat();
                IPreAmp preamp = getPreAmp();
                PreAmpStateStore state = getPreAmpStateStore();

                state.setLevelFromUI(level);

                // apply
                if (preamp != null) {
                    preamp.setLevel(state.getLevel());
                }

                // notify
                postAppEvent(
                        EventDefs.Category.NOTIFY_PRE_AMP,
                        PreAmpNotifyEvents.LEVEL_UPDATED, 0, 0);
            }
            break;
        }
    }

    private void onReceiveHQVisualizerEvent(AppEvent event) {
        HQVisualizerStateStore state = getHQVisualizerStateStore();

        // NOTE:
        // Only save the enabled state, and don't apply to
        // the effect object here.

        switch (event.event) {
            case HQVisualizerReqEvents.SET_WAVEFORM_ENABLED: {
                boolean enabled = (event.arg1 != 0);

                state.setCaptureWaveformEnabled(enabled);

                // notify enabled state updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_HQ_VISUALIZER,
                        HQVisualizerNotifyEvents.WAVEFORM_ENABLED_STATE_UPDATED,
                        state.isCaptureWaveformEnabled() ? 1 : 0,
                        0);
            }
            break;
            case HQVisualizerReqEvents.SET_FFT_ENABLED: {
                boolean enabled = (event.arg1 != 0);

                state.setCaptureFftEnabled(enabled);

                // notify enabled state updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_HQ_VISUALIZER,
                        HQVisualizerNotifyEvents.FFT_ENABLED_STATE_UPDATED,
                        state.isCaptureFftEnabled() ? 1 : 0,
                        0);
            }
            break;
            case HQVisualizerReqEvents.SET_WINDOW_TYPE: {
                int windowType = event.arg1;

                state.setWindowType(windowType);

                // notify window type updated
                postAppEvent(
                        EventDefs.Category.NOTIFY_HQ_VISUALIZER,
                        HQVisualizerNotifyEvents.WINDOW_TYPE_UPDATED,
                        state.getWindowType(),
                        0);
            }
            break;
        }
    }

    private void applyAttachedEuxEffectSettings(IBasicMediaPlayer player, int type) {
        IEnvironmentalReverb envReverb = getEnvironmentalReverb();
        IPresetReverb presetReverb = getPresetReverb();
        int effectId = 0;

        switch (type) {
            case PlayerControlReqEvents.AUX_EEFECT_TYPE_NONE:
                break;
            case PlayerControlReqEvents.AUX_EEFECT_TYPE_ENVIRONMENAL_REVERB:
                if (envReverb != null) {
                    effectId = envReverb.getId();
                }
                break;
            case PlayerControlReqEvents.AUX_EEFECT_TYPE_PRESET_REVERB:
                if (presetReverb != null) {
                    effectId = presetReverb.getId();
                }
                break;
        }

        if (player != null) {
            player.attachAuxEffect(effectId);
        }
    }

    private void setPlayerState(int index, int state) {
        if (mPlayerState[index] == state)
            return;

        mPlayerState[index] = state;

        onPlayerStateChanged(index, state);
    }

    private void onPlayerStateChanged(int index, int state) {
        // set foreground if needed
        checkAndUpdateForegroundState();

        // broadcast event
        postAppEvent(
                EventDefs.Category.NOTIFY_PLAYER_CONTROL,
                PlayerControlNotifyEvents.PLAYER_STATE_CHANGED,
                index, mPlayerState[index]);
    }

    private static void applyEnvironmentalReverbPresetParam(
            IEnvironmentalReverb envreverb,
            EnvironmentalReverbStateStore state, int index) {
        IEnvironmentalReverb.Settings settings = state.getSettings();

        switch (index) {
            case EnvironmentalReverbStateStore.PARAM_INDEX_DECAY_HF_RATIO:
                envreverb.setDecayHFRatio(settings.decayHFRatio);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_DECAY_TIME:
                envreverb.setDecayTime(settings.decayTime);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_DENSITY:
                envreverb.setDensity(settings.density);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_DIFFUSION:
                envreverb.setDiffusion(settings.diffusion);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_REFLECTIONS_DELAY:
                envreverb.setReflectionsDelay(settings.reflectionsDelay);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_REFLECTIONS_LEVEL:
                envreverb.setReflectionsLevel(settings.reflectionsLevel);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_REVERB_DELAY:
                envreverb.setReverbDelay(settings.reverbDelay);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_REVERB_LEVEL:
                envreverb.setReverbLevel(settings.reverbLevel);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_ROOM_HF_LEVEL:
                envreverb.setRoomHFLevel(settings.roomHFLevel);
                break;
            case EnvironmentalReverbStateStore.PARAM_INDEX_ROOM_LEVEL:
                envreverb.setRoomLevel(settings.roomLevel);
                break;
        }
    }

    private void postAppEvent(int category, int event, int arg1, int arg2) {
        eventBus().post(new AppEvent(category, event, arg1, arg2));
    }

    private void postInfoOrErrorAppEvent(int category, int event, int arg1, int arg2, int what, int extra) {
        AppEvent eventObj = new AppEvent(category, event, arg1, arg2);

        eventObj.extras = new Bundle();
        eventObj.extras.putInt(PlayerControlNotifyEvents.EXTRA_ERROR_INFO_WHAT, what);
        eventObj.extras.putInt(PlayerControlNotifyEvents.EXTRA_ERROR_INFO_EXTRA, extra);

        eventBus().post(eventObj);
    }

    private void setPlayerImplType(int type, boolean force) {
        MediaPlayerStateStore state = getPlayerStateStore();

        switch (type) {
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_STANDARD:
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_OPENSL:
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_HYBRID:
                break;
            default:
                throw new IllegalArgumentException();
        }

        if (!force && state.getPlayerImplType() == type)
            return;

        releaseAllPlayerResources();
        releaseFactory();

        state.setPlayerImplType(type);

        switch (type) {
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_STANDARD:
                mFactory = new StandardMediaPlayerFactory(mContext);
                break;
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_OPENSL:
                mFactory = new OpenSLMediaPlayerFactory(mContext);
                break;
            case MediaPlayerStateStore.PLAYER_IMPL_TYPE_HYBRID:
                mFactory = new HybridMediaPlayerFactory(mContext);
                break;
        }
    }

    private void applyMediaPlayerStates(
            IBasicMediaPlayer player,
            MediaPlayerStateStore states) {
        player.setLooping(states.isLooping());
        player.setVolume(states.getVolumeLeft(), states.getVolumeRight());
        player.setAuxEffectSendLevel(states.getAuxEffectSendLevel());
    }

    private void applyBassBoostStates(BassBoostStateStore states) {
        final boolean enabled = states.isEnabled();
        final IBassBoost bassBoost = (enabled) ? createBassBoost() : getBassBoost();

        if (bassBoost == null) return;

        bassBoost.setProperties(states.getSettings());
        bassBoost.setEnabled(enabled);

        if (!enabled) {
            releaseBassBoost();
        }
    }

    private void applyVirtualizerStates(VirtualizerStateStore states) {
        final boolean enabled = states.isEnabled();
        final IVirtualizer virtualizer = (enabled) ? createVirtualizer() : getVirtualizer();

        if (virtualizer == null) return;

        virtualizer.setProperties(states.getSettings());
        virtualizer.setEnabled(enabled);

        if (!enabled) {
            releaseVirtualizer();
        }
    }

    private void applyEqualizerStates(EqualizerStateStore states) {
        final boolean enabled = states.isEnabled();
        final IEqualizer equalizer = (enabled) ? createEqualizer() : getEqualizer();

        if (equalizer == null) return;

        equalizer.setProperties(states.getSettings());
        equalizer.setEnabled(enabled);

        if (!enabled) {
            releaseEqualizer();
        }
    }

    private void applyLoudnessEnhancerStates(LoudnessEnhancerStateStore states) {
        final boolean enabled = states.isEnabled();
        final ILoudnessEnhancer loudnessEnhancer = (enabled) ? createLoudnessEnhancer() : getLoudnessEnhancer();

        if (loudnessEnhancer == null) return;

        loudnessEnhancer.setProperties(states.getSettings());
        loudnessEnhancer.setEnabled(enabled);

        if (!enabled) {
            releaseLoudnessEnhancer();
        }
    }

    private void applyEnvironmentalReverbStates(EnvironmentalReverbStateStore states) {
        final boolean enabled = states.isEnabled();
        final IEnvironmentalReverb envreverb = (enabled) ? createEnvironmentalReverb() : getEnvironmentalReverb();

        if (envreverb == null) return;

        envreverb.setProperties(states.getSettings());
        envreverb.setEnabled(enabled);

        if (!enabled) {
            releaseEnvironmentalReverb();
        }
    }

    private void applyPresetReverbStates(PresetReverbStateStore states) {
        final boolean enabled = states.isEnabled();
        final IPresetReverb presetreverb = (enabled) ? createPresetReverb() : getPresetReverb();

        if (presetreverb == null) return;

        presetreverb.setProperties(states.getSettings());
        presetreverb.setEnabled(enabled);

        if (!enabled) {
            releasePresetReverb();
        }
    }

    private void applyHQEqualizerStates(HQEqualizerStateStore states) {
        final boolean enabled = states.isEnabled();
        final IEqualizer equalizer = (enabled) ? createHQEqualizer() : getHQEqualizer();

        if (equalizer == null) return;

        equalizer.setProperties(states.getSettings());
        equalizer.setEnabled(enabled);

        if (!enabled) {
            releaseHQEqualizer();
        }
    }

    private void applyPreAmpStates(PreAmpStateStore states) {
        final boolean enabled = states.isEnabled();
        final IPreAmp preamp = (enabled) ? createPreAmp() : getPreAmp();

        if (preamp == null) return;

        preamp.setProperties(states.getSettings());
        preamp.setEnabled(enabled);

        if (!enabled) {
            releasePreAmp();
        }
    }

    public int getActivePlayerIndex() {
        return mActivePlayerIndex;
    }

    private IBasicMediaPlayer getActivePlayer() {
        return getPlayer(getActivePlayerIndex());
    }

    private IBasicMediaPlayer getPlayer(int index) {
        return mMediaPlayer[index];
    }

    private IBassBoost getBassBoost() {
        return mBassBoost;
    }

    private IVirtualizer getVirtualizer() {
        return mVirtualizer;
    }

    private IEqualizer getEqualizer() {
        return mEqualizer;
    }

    private ILoudnessEnhancer getLoudnessEnhancer() {
        return mLoudnessEnhancer;
    }

    private IEnvironmentalReverb getEnvironmentalReverb() {
        return mEnvironmentalReverb;
    }

    private IPresetReverb getPresetReverb() {
        return mPresetReverb;
    }

    public IVisualizer getVisualizer() {
        return mVisualizer;
    }

    private IEqualizer getHQEqualizer() {
        return mHQEqualizer;
    }

    private IPreAmp getPreAmp() {
        return mPreAmp;
    }

    public IHQVisualizer getHQVisualizer() {
        return mHQVisualizer;
    }

    private void releaseAllPlayerResources() {
        playerRelease(0);
        playerRelease(1);
        resetPlayerStateControlVariables();

        releaseBassBoost();
        releaseVirtualizer();
        releaseEqualizer();
        releaseLoudnessEnhancer();
        releasePresetReverb();
        releaseEnvironmentalReverb();
        releaseVisualizer();
        releaseHQVisualizer();
        releaseHQEqualizer();
        releasePreAmp();
    }

    private void releaseBassBoost() {
        safeRelease(mBassBoost);
        mBassBoost = null;
    }

    private void releaseVirtualizer() {
        safeRelease(mVirtualizer);
        mVirtualizer = null;
    }

    private void releaseEqualizer() {
        safeRelease(mEqualizer);
        mEqualizer = null;
    }

    private void releaseLoudnessEnhancer() {
        safeRelease(mLoudnessEnhancer);
        mLoudnessEnhancer = null;
    }

    private void releaseEnvironmentalReverb() {
        safeRelease(mEnvironmentalReverb);
        mEnvironmentalReverb = null;
    }

    private void releasePresetReverb() {
        safeRelease(mPresetReverb);
        mPresetReverb = null;
    }

    private void releaseVisualizer() {
        safeRelease(mVisualizer);
        mVisualizer = null;
    }

    private void releaseHQEqualizer() {
        safeRelease(mHQEqualizer);
        mHQEqualizer = null;
    }

    private void releasePreAmp() {
        safeRelease(mPreAmp);
        mPreAmp = null;
    }

    private void releaseHQVisualizer() {
        safeRelease(mHQVisualizer);
        mHQVisualizer = null;
    }

    private void releaseFactory() {
        safeRelease(mFactory);
        mFactory = null;
    }

    private static void safeRelease(IReleasable obj) {
        try {
            if (obj != null) {
                obj.release();
            }
        } catch (Exception e) {
        }
    }

    private IBasicMediaPlayer createPlayer(int index) {
        if (mMediaPlayer[index] == null) {
            IBasicMediaPlayer player = mFactory.createMediaPlayer();

            player.setWakeMode(mContext, PowerManager.PARTIAL_WAKE_LOCK);
            player.setOnBufferingUpdateListener(mOnBufferingUpdateListener);
            player.setOnCompletionListener(mOnCompletionListener);
            player.setOnErrorListener(mOnErrorListener);
            player.setOnInfoListener(mOnInfoListener);
            player.setOnPreparedListener(mOnPreparedListener);
            player.setOnSeekCompleteListener(mOnSeekCompleteListener);

            mMediaPlayer[index] = player;
        }

        return mMediaPlayer[index];
    }

    private IBassBoost createBassBoost() {
        if (mBassBoost == null) {
            try {
                mBassBoost = mFactory.createBassBoost(getPlayer(0));
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }
        return mBassBoost;
    }

    private IVirtualizer createVirtualizer() {
        if (mVirtualizer == null) {
            try {
                mVirtualizer = mFactory.createVirtualizer(getPlayer(0));
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }
        return mVirtualizer;
    }

    private IEqualizer createEqualizer() {
        if (mEqualizer == null) {
            try {
                mEqualizer = mFactory.createEqualizer(getPlayer(0));
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }
        return mEqualizer;
    }

    private ILoudnessEnhancer createLoudnessEnhancer() {
        if (mLoudnessEnhancer == null) {
            try {
                mLoudnessEnhancer = mFactory.createLoudnessEnhancer(getPlayer(0));
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }
        return mLoudnessEnhancer;
    }

    private IEnvironmentalReverb createEnvironmentalReverb() {
        if (mEnvironmentalReverb == null) {
            try {
                mEnvironmentalReverb = mFactory.createEnvironmentalReverb();
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }
        return mEnvironmentalReverb;
    }

    private IPresetReverb createPresetReverb() {
        if (mPresetReverb == null) {
            try {
                mPresetReverb = mFactory.createPresetReverb();
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }
        return mPresetReverb;
    }

    public IVisualizer createVisualizer() {
        if (mVisualizer == null) {
            try {
                mVisualizer = mFactory.createVisualizer(getPlayer(0));
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }

        return mVisualizer;
    }

    private IEqualizer createHQEqualizer() {
        if (mHQEqualizer == null) {
            try {
                mHQEqualizer = mFactory.createHQEqualizer();
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }

        return mHQEqualizer;
    }

    private IPreAmp createPreAmp() {
        IMediaPlayerFactory factory = mFactory;

        try {
            mPreAmp = factory.createPreAmp();
        } catch (UnsupportedOperationException e) {
            // the effect is not supported
        }

        return mPreAmp;
    }

    public IHQVisualizer createHQVisualizer() {
        if (mHQVisualizer == null) {
            try {
                mHQVisualizer = mFactory.createHQVisualizer();
            } catch (UnsupportedOperationException e) {
                // the effect is not supported
            }
        }

        return mHQVisualizer;
    }

    private void setBassBoostEnabled(boolean enabled) {
        BassBoostStateStore state = getBassBoostStateStore();

        state.setEnabled(enabled);
        applyBassBoostStates(state);

        // notify enabled state updated
        postAppEvent(
                EventDefs.Category.NOTIFY_BASSBOOST,
                BassBoostNotifyEvents.ENABLED_STATE_UPDATED,
                state.isEnabled() ? 1 : 0,
                0);
    }

    private void setVirtualizerEnabled(boolean enabled) {
        VirtualizerStateStore state = getVirtualizerStateStore();

        state.setEnabled(enabled);
        applyVirtualizerStates(state);

        // notify enabled state updated
        postAppEvent(
                EventDefs.Category.NOTIFY_VIRTUALIZER,
                VirtualizerNotifyEvents.ENABLED_STATE_UPDATED,
                state.isEnabled() ? 1 : 0,
                0);
    }

    private void setEqualizerEnabled(boolean enabled) {
        EqualizerStateStore state = getEqualizerStateStore();

        state.setEnabled(enabled);
        applyEqualizerStates(state);

        // notify enabled state updated
        postAppEvent(
                EventDefs.Category.NOTIFY_EQUALIZER,
                EqualizerNotifyEvents.ENABLED_STATE_UPDATED,
                state.isEnabled() ? 1 : 0,
                0);
    }

    private void setLoudnessEnhancerEnabled(boolean enabled) {
        LoudnessEnhancerStateStore state = getLoudnessEnhancerStateStore();

        state.setEnabled(enabled);
        applyLoudnessEnhancerStates(state);

        // notify enabled state updated
        postAppEvent(
                EventDefs.Category.NOTIFY_LOUDNESS_ENHANCER,
                LoudnessEnhancerNotifyEvents.ENABLED_STATE_UPDATED,
                state.isEnabled() ? 1 : 0,
                0);
    }

    private void setEnvironmentalReverbEnabled(boolean enabled) {
        EnvironmentalReverbStateStore state = getEnvironmentalReverbStateStore();

        state.setEnabled(enabled);
        applyEnvironmentalReverbStates(state);

        // notify enabled state updated
        postAppEvent(
                EventDefs.Category.NOTIFY_ENVIRONMENTAL_REVERB,
                EnvironmentalReverbNotifyEvents.ENABLED_STATE_UPDATED,
                state.isEnabled() ? 1 : 0,
                0);
    }

    private void setPresetReverbEnabled(boolean enabled) {
        PresetReverbStateStore state = getPresetReverbStateStore();

        state.setEnabled(enabled);
        applyPresetReverbStates(state);

        // notify enabled state updated
        postAppEvent(
                EventDefs.Category.NOTIFY_PRESET_REVERB,
                PresetReverbNotifyEvents.ENABLED_STATE_UPDATED,
                state.isEnabled() ? 1 : 0,
                0);
    }

    private void setHQEqualizerEnabled(boolean enabled) {
        HQEqualizerStateStore state = getHQEqualizerStateStore();

        state.setEnabled(enabled);
        applyHQEqualizerStates(state);

        // notify enabled state updated
        postAppEvent(
                EventDefs.Category.NOTIFY_HQ_EQUALIZER,
                HQEqualizerNotifyEvents.ENABLED_STATE_UPDATED,
                state.isEnabled() ? 1 : 0,
                0);
    }

    private void setPreAmpEnabled(boolean enabled) {
        PreAmpStateStore state = getPreAmpStateStore();

        state.setEnabled(enabled);
        applyPreAmpStates(state);

        // notify enabled state updated
        postAppEvent(
                EventDefs.Category.NOTIFY_PRE_AMP,
                PreAmpNotifyEvents.ENABLED_STATE_UPDATED,
                state.isEnabled() ? 1 : 0,
                0);
    }

    private int getMediaPlayerIndex(IBasicMediaPlayer player) {
        if (player == mMediaPlayer[0])
            return 0;
        if (player == mMediaPlayer[1])
            return 1;
        return -1; // default: ZERO
    }

    private void notifyPlayerExceptionOccurred(int index, String opName, Exception exception) {
        AppEvent eventObj = new AppEvent(
                EventDefs.Category.NOTIFY_PLAYER_CONTROL,
                PlayerControlNotifyEvents.NOTIFY_EXCEPTION_OCCURRED,
                index, 0);

        eventObj.extras = new Bundle();
        eventObj.extras.putString(
                PlayerControlNotifyEvents.EXTRA_EXEC_OPERATION_NAME,
                opName);
        eventObj.extras.putString(
                PlayerControlNotifyEvents.EXTRA_EXCEPTION_NAME,
                exception.getClass().getSimpleName());
        eventObj.extras.putString(
                PlayerControlNotifyEvents.EXTRA_STACK_TRACE,
                getStackTraceString(exception));

        eventBus().post(eventObj);
    }

    private static int nextPlayerIndex(int index) {
        return (index == 0) ? 1 : 0;
    }

    private static String getStackTraceString(Exception e) {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        e.printStackTrace(pw);
        pw.flush();
        return sw.toString();
    }

    public void bindHolderService(Service service) {
        mHolderService = service;

        checkAndUpdateForegroundState();
    }

    private void checkAndUpdateForegroundState() {
        if (mHolderService == null)
            return;

        final int index = getActivePlayerIndex();
        final boolean isPlaying = getActivePlayerState() == PLAYER_STATE_STARTED;
        final int reqState;

        if (isPlaying) {
            reqState = (index == 0) ? FG_STATE_PLAYER_0 : FG_STATE_PLAYER_1;
        } else {
            reqState = FG_STATE_INACTIVE;
        }

        if (mCurrentForegroundState == reqState)
            return;

        if (isPlaying) {
            final MediaMetadata metadata = getActiveMediaMetadata();

            if (metadata == null) {
                throw new IllegalStateException("Bug check (GitHub issue #5)");
            }

            final Notification notification = NotificationBuilder
                    .createServiceNotification(mHolderService, metadata);

            mHolderService.startForeground(
                    NotificationIds.ONGOING_NOTIFICATION, notification);
        } else {
            mHolderService.stopForeground(true);
        }

        mCurrentForegroundState = reqState;
    }
}
