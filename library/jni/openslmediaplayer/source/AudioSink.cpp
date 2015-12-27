//
//    Copyright (C) 2014 Haruki Hasegawa
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

// #define LOG_TAG "AudioSink"
// #define NB_LOG_TAG "NB AudioSink"

#include "oslmp/impl/AudioSink.hpp"

#include <cxxporthelper/memory>
#include <cxxporthelper/utility>

#include <android/api-level.h>

#include <SLESCXX/OpenSLES_CXX.hpp>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerContext.hpp>
#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/AudioDataPipeManager.hpp"
#include "oslmp/utils/timespec_utils.hpp"

#define TRANSLATE_RESULT(result) InternalUtils::sTranslateOpenSLErrorCode(result)

#define MAX_BLOCK_COUNT 32

#define IS_SL_RESULT_SUCCESS(slresult) (CXXPH_LIKELY((slresult) == SL_RESULT_SUCCESS))

#if (__ANDROID_API__ < 21)
extern "C" {

/* The following pcm representations and data formats map to those in OpenSLES 1.1 */
#define SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT ((SLuint32)0x00000001)
#define SL_ANDROID_PCM_REPRESENTATION_UNSIGNED_INT ((SLuint32)0x00000002)
#define SL_ANDROID_PCM_REPRESENTATION_FLOAT ((SLuint32)0x00000003)

#define SL_ANDROID_DATAFORMAT_PCM_EX ((SLuint32)0x00000004)

typedef struct SLAndroidDataFormat_PCM_EX_ {
    SLuint32 formatType;
    SLuint32 numChannels;
    SLuint32 sampleRate;
    SLuint32 bitsPerSample;
    SLuint32 containerSize;
    SLuint32 channelMask;
    SLuint32 endianness;
    SLuint32 representation;
} SLAndroidDataFormat_PCM_EX;

} // extern "C"
#endif

namespace oslmp {
namespace impl {

using namespace ::opensles;

typedef OpenSLMediaPlayerInternalUtils InternalUtils;

class AudioSinkDataPipeReadBlockQueue {
public:
    AudioSinkDataPipeReadBlockQueue();
    ~AudioSinkDataPipeReadBlockQueue();

    int initialize(int capacity) noexcept;
    int capacity() const noexcept;
    int count() const noexcept;
    void clear() noexcept;
    bool enqueue(const AudioSinkDataPipe::read_block_t &src) noexcept;
    bool dequeue(AudioSinkDataPipe::read_block_t &dest) noexcept;

private:
    AudioSinkDataPipe::read_block_t rb_queue_[MAX_BLOCK_COUNT];
    int capacity_;
    int count_;
    int rp_;
    int wp_;
};

class AudioPipeBufferQueueBinder {
public:
    AudioPipeBufferQueueBinder();
    ~AudioPipeBufferQueueBinder();

    void release() noexcept;
    int beforeStart(CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept;
    int afterStop(CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept;
    int initialize(OpenSLMediaPlayerInternalContext *context, AudioSinkDataPipe *pipe,
                   CSLAndroidSimpleBufferQueueItf *buffer_queue, int num_blocks) noexcept;
    void handleCallback(AudioSinkDataPipe *pipe, CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept;

private:
    static bool getSilentReadBlock(AudioSinkDataPipe *pipe, AudioSinkDataPipe::read_block_t &rb) noexcept;

private:
    OpenSLMediaPlayerInternalContext *context_;

    int num_blocks_;
    AudioSinkDataPipe::read_block_t silent_block_;

    size_t wait_buffering_;
    AudioSinkDataPipeReadBlockQueue wait_completion_blocks_;
    AudioSinkDataPipeReadBlockQueue pooled_blocks_;

#ifdef USE_OSLMP_DEBUG_FEATURES
    std::unique_ptr<NonBlockingTraceLoggerClient> nb_logger_;
#endif
};

class AudioSink::Impl {
public:
    enum { NUM_CHANNELS = 2, };

    Impl(AudioSink *holder);
    ~Impl();

    int initialize(const initialize_args_t &args) noexcept;
    int start() noexcept;
    int pause() noexcept;
    int resume() noexcept;
    int stop() noexcept;
    uint32_t getLatencyInFrames() const noexcept;
    state_t getState() const noexcept;
    AudioSinkDataPipe *getPipe() const noexcept;
    opensles::CSLObjectItf *getPlayerObj() const noexcept;

private:
    void releaseOpenSLResources() noexcept;

    static void playbackBufferQueueCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) noexcept;

private:
    AudioSink *holder_;
    state_t state_;

    OpenSLMediaPlayerInternalContext *context_;
    AudioDataPipeManager *pipe_mgr_;
    AudioSinkDataPipe *pipe_;
    AudioPipeBufferQueueBinder queue_binder_;

    opensles::CSLObjectItf obj_player_;

    opensles::CSLPlayItf player_;
    opensles::CSLVolumeItf volume_;
    opensles::CSLAndroidSimpleBufferQueueItf buffer_queue_;

    int block_size_in_frames_;
    uint32_t num_player_blocks_;
    uint32_t num_pipe_blocks_;
};

static inline size_t getSize(const AudioSinkDataPipe::read_block_t &rb) noexcept
{
    return getBytesPerSample(rb.sample_format) * rb.num_channels * rb.num_frames;
}

static inline size_t getSize(const AudioSinkDataPipe::write_block_t &wb) noexcept
{
    return getBytesPerSample(wb.sample_format) * wb.num_channels * wb.num_frames;
}

//
// AudioSink
//
AudioSink::AudioSink() : impl_(new (std::nothrow) Impl(this)) {}

AudioSink::~AudioSink() {}

int AudioSink::initialize(const AudioSink::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioSink::start() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->start();
}

int AudioSink::pause() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->pause();
}

int AudioSink::resume() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->resume();
}

int AudioSink::stop() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->stop();
}

uint32_t AudioSink::getLatencyInFrames() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0UL;
    return impl_->getLatencyInFrames();
}

AudioSink::state_t AudioSink::getState() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return SINK_STATE_NOT_INITIALIZED;
    return impl_->getState();
}

AudioSinkDataPipe *AudioSink::getPipe() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return nullptr;
    return impl_->getPipe();
}

opensles::CSLObjectItf *AudioSink::getPlayerObj() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return nullptr;
    return impl_->getPlayerObj();
}

//
// AudioSink::Impl
//
AudioSink::Impl::Impl(AudioSink *holder)
    : holder_(holder), state_(SINK_STATE_NOT_INITIALIZED), context_(nullptr), pipe_mgr_(nullptr), pipe_(nullptr),
      block_size_in_frames_(0), num_player_blocks_(0), num_pipe_blocks_(0)
{
}

AudioSink::Impl::~Impl()
{
    stop();
    releaseOpenSLResources();

    if (pipe_mgr_ && pipe_) {
        (void)pipe_mgr_->setSinkPipeOutPortUser(pipe_, holder_, false);
    }
    pipe_ = nullptr;
    pipe_mgr_ = nullptr;
    context_ = nullptr;
    holder_ = nullptr;
    queue_binder_.release();
}

int AudioSink::Impl::initialize(const AudioSink::initialize_args_t &args) noexcept
{
    SLint32 sl_stream_type;
    SLresult slResult;
    int result;

    // check arguments
    switch (args.sampling_rate) {
    case 44100000:
    case 48000000:
        break;
    default:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    switch (args.sample_format) {
    case kAudioSampleFormatType_S16:
    case kAudioSampleFormatType_F32:
        break;
    default:
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    if (!args.context)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.pipe_manager)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.pipe)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!InternalUtils::sTranslateToOpenSLStreamType(args.stream_type, &sl_stream_type))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (state_ != SINK_STATE_NOT_INITIALIZED)
        return OSLMP_RESULT_ILLEGAL_STATE;

    const int block_size_in_frames = args.pipe_manager->getBlockSizeInFrames();
    const uint32_t num_pipe_blocks = args.pipe->getNumberOfBufferItems();
    const uint32_t num_player_blocks = args.num_player_blocks;

    if (!(num_pipe_blocks > (num_player_blocks + 1))) { // +1: silent block
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    // create SLPlayer
    CSLEngineItf engine;
    CSLObjectItf obj_outmix(false); // No auto-destroy

    slResult = args.context->getInterfaceFromEngine(&engine);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    slResult = args.context->getInterfaceFromOutputMixer(&obj_outmix);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue src_loc_bufq = {};
    SLDataFormat_PCM src_format_pcm = {};
    SLAndroidDataFormat_PCM_EX src_format_pcm_ex = {};
    SLDataSource audio_src = {};

    src_loc_bufq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    src_loc_bufq.numBuffers = num_player_blocks;

    if (args.sample_format == kAudioSampleFormatType_F32) {
        src_format_pcm_ex.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
        src_format_pcm_ex.numChannels = NUM_CHANNELS;
        src_format_pcm_ex.sampleRate = args.sampling_rate;
        src_format_pcm_ex.bitsPerSample = 32;
        src_format_pcm_ex.containerSize = 32;
        src_format_pcm_ex.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        src_format_pcm_ex.endianness = SL_BYTEORDER_LITTLEENDIAN;
        src_format_pcm_ex.representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;

        audio_src.pFormat = &src_format_pcm_ex;
    } else {
        src_format_pcm.formatType = SL_DATAFORMAT_PCM;
        src_format_pcm.numChannels = NUM_CHANNELS;
        src_format_pcm.samplesPerSec = args.sampling_rate;
        src_format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
        src_format_pcm.containerSize = 16;
        src_format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
        src_format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

        audio_src.pFormat = &src_format_pcm;
    }

    audio_src.pLocator = &src_loc_bufq;

    // configure audio sink
    SLDataLocator_OutputMix sink_outmix;
    SLDataSink audio_sink = {};

    sink_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    sink_outmix.outputMix = obj_outmix.self();

    audio_sink.pLocator = &sink_outmix;
    audio_sink.pFormat = nullptr;

    CSLObjectItf obj_player;
    CSLAndroidConfigurationItf config;
    CSLAndroidSimpleBufferQueueItf buffer_queue;
    CSLPlayItf player;
    CSLVolumeItf volume;

    // create audio player
    {
        SLInterfaceID ids[7];
        SLboolean req[7];
        SLuint32 cnt = 0;

        ids[cnt] = config.getIID();
        req[cnt] = SL_BOOLEAN_TRUE;
        ++cnt;

        ids[cnt] = buffer_queue.getIID();
        req[cnt] = SL_BOOLEAN_TRUE;
        ++cnt;

        ids[cnt] = volume.getIID();
        req[cnt] = SL_BOOLEAN_FALSE;
        ++cnt;

        // effect send
        if (args.opts & (OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB | OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB)) {
            ids[cnt] = CSLEffectSendItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        // bass boost
        if (args.opts & OSLMP_CONTEXT_OPTION_USE_BASSBOOST) {
            ids[cnt] = CSLBassBoostItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        // virtualizer
        if (args.opts & OSLMP_CONTEXT_OPTION_USE_VIRTUALIZER) {
            ids[cnt] = CSLVirtualizerItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        // equalizer
        if (args.opts & OSLMP_CONTEXT_OPTION_USE_EQUALIZER) {
            ids[cnt] = CSLEqualizerItf::sGetIID();
            req[cnt] = SL_BOOLEAN_FALSE;
            ++cnt;
        }

        slResult = engine.CreateAudioPlayer(&obj_player, &audio_src, &audio_sink, cnt, ids, req);

        if (!IS_SL_RESULT_SUCCESS(slResult))
            return TRANSLATE_RESULT(slResult);
    }

    // get configuration interface
    slResult = obj_player.GetInterface(&config);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // configure the player
    slResult = config.SetConfiguration(SL_ANDROID_KEY_STREAM_TYPE, &sl_stream_type, sizeof(SLint32));
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // realize the player
    slResult = obj_player.Realize(SL_BOOLEAN_FALSE);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the play interface
    slResult = obj_player.GetInterface(&player);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get buffer queue interface
    slResult = obj_player.GetInterface(&buffer_queue);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the volume interface
    slResult = obj_player.GetInterface(&volume);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // set pipe user
    result = args.pipe_manager->setSinkPipeOutPortUser(args.pipe, holder_, true);
    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS))
        return result;

    // bind buffer
    result = queue_binder_.initialize(args.context, args.pipe, &buffer_queue, num_player_blocks);
    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS)) {
        args.pipe_manager->setSinkPipeOutPortUser(args.pipe, holder_, false);
        return result;
    }

    // update fields
    player_ = std::move(player);
    buffer_queue_ = std::move(buffer_queue);
    volume_ = std::move(volume);

    obj_player_ = std::move(obj_player);

    context_ = args.context;
    pipe_mgr_ = args.pipe_manager;
    pipe_ = args.pipe;

    block_size_in_frames_ = block_size_in_frames;
    num_player_blocks_ = num_player_blocks;
    num_pipe_blocks_ = num_pipe_blocks;

    state_ = SINK_STATE_STOPPED;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSink::Impl::start() noexcept
{
    if (CXXPH_UNLIKELY(state_ == SINK_STATE_STARTED))
        return OSLMP_RESULT_SUCCESS;

    if (CXXPH_UNLIKELY(state_ != SINK_STATE_STOPPED))
        return OSLMP_RESULT_ILLEGAL_STATE;

    int result = queue_binder_.beforeStart(&buffer_queue_);
    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS))
        return result;

    SLresult slResult;

    slResult = buffer_queue_.RegisterCallback(playbackBufferQueueCallback, this);
    if (!IS_SL_RESULT_SUCCESS(slResult)) {
        return TRANSLATE_RESULT(slResult);
    }

    slResult = player_.SetPlayState(SL_PLAYSTATE_PLAYING);

    if (IS_SL_RESULT_SUCCESS(slResult)) {
        state_ = SINK_STATE_STARTED;
    }

    return TRANSLATE_RESULT(slResult);
}

int AudioSink::Impl::pause() noexcept
{
    if (CXXPH_UNLIKELY(state_ == SINK_STATE_PAUSED))
        return OSLMP_RESULT_SUCCESS;

    if (CXXPH_UNLIKELY(state_ != SINK_STATE_STARTED))
        return OSLMP_RESULT_ILLEGAL_STATE;

    SLresult slResult;

    slResult = player_.SetPlayState(SL_PLAYSTATE_STOPPED);
    (void)buffer_queue_.RegisterCallback(NULL, nullptr);

    queue_binder_.afterStop(&buffer_queue_);

    if (IS_SL_RESULT_SUCCESS(slResult)) {
        state_ = SINK_STATE_PAUSED;
    }

    return TRANSLATE_RESULT(slResult);
}

int AudioSink::Impl::resume() noexcept
{
    if (CXXPH_UNLIKELY(state_ == SINK_STATE_STARTED))
        return OSLMP_RESULT_SUCCESS;

    SLresult slResult = player_.SetPlayState(SL_PLAYSTATE_PLAYING);

    if (IS_SL_RESULT_SUCCESS(slResult)) {
        state_ = SINK_STATE_STARTED;
    }

    return TRANSLATE_RESULT(slResult);
}

int AudioSink::Impl::stop() noexcept
{
    if (CXXPH_UNLIKELY(state_ == SINK_STATE_STOPPED))
        return OSLMP_RESULT_SUCCESS;

    if (CXXPH_UNLIKELY(!(state_ == SINK_STATE_STARTED || state_ == SINK_STATE_PAUSED)))
        return OSLMP_RESULT_ILLEGAL_STATE;

    SLresult slResult;

    slResult = player_.SetPlayState(SL_PLAYSTATE_STOPPED);

    if (IS_SL_RESULT_SUCCESS(slResult)) {
        state_ = SINK_STATE_STOPPED;
    }

    int result = queue_binder_.afterStop(&buffer_queue_);
    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS))
        return result;

    return OSLMP_RESULT_SUCCESS;
}

uint32_t AudioSink::Impl::getLatencyInFrames() const noexcept
{
    return static_cast<uint32_t>(block_size_in_frames_ * (num_pipe_blocks_ - 1));
}

AudioSink::state_t AudioSink::Impl::getState() const noexcept { return state_; }

AudioSinkDataPipe *AudioSink::Impl::getPipe() const noexcept { return pipe_; }

opensles::CSLObjectItf *AudioSink::Impl::getPlayerObj() const noexcept
{
    return const_cast<opensles::CSLObjectItf *>(&obj_player_);
}

void AudioSink::Impl::releaseOpenSLResources() noexcept
{
    volume_.unbind();
    buffer_queue_.unbind();
    player_.unbind();

    obj_player_.Destroy();
}

void AudioSink::Impl::playbackBufferQueueCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) noexcept
{
    Impl *thiz = static_cast<Impl *>(pContext);
    AudioSinkDataPipe *pipe = thiz->pipe_;

    CSLAndroidSimpleBufferQueueItf buffer_queue;
    buffer_queue.assign(caller);

    thiz->queue_binder_.handleCallback(pipe, &buffer_queue);
}

//
// AudioPipeBufferQueueBinder
//

AudioPipeBufferQueueBinder::AudioPipeBufferQueueBinder() : context_(nullptr), num_blocks_(0), wait_buffering_(0) {}

AudioPipeBufferQueueBinder::~AudioPipeBufferQueueBinder() {}

void AudioPipeBufferQueueBinder::release() noexcept
{
    wait_buffering_ = 0;
    context_ = nullptr;
}

int AudioPipeBufferQueueBinder::beforeStart(CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept
{
    SLresult slResult;

    buffer_queue->Clear();
    wait_completion_blocks_.clear();

    // fill silent block
    for (int i = 0; i < num_blocks_; ++i) {
        wait_completion_blocks_.enqueue(silent_block_);

        slResult = buffer_queue->Enqueue(silent_block_.src, getSize(silent_block_));

        if (!IS_SL_RESULT_SUCCESS(slResult)) {
            return TRANSLATE_RESULT(slResult);
        }
    }

    wait_buffering_ = (pooled_blocks_.count() == 0) ? 1 : 0;

    return OSLMP_RESULT_SUCCESS;
}

int AudioPipeBufferQueueBinder::afterStop(CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept
{
    buffer_queue->Clear();

    pooled_blocks_.clear();

    {
        AudioSinkDataPipe::read_block_t rb;
        while (wait_completion_blocks_.dequeue(rb)) {
            if (rb.src != silent_block_.src) {
                pooled_blocks_.enqueue(rb);
            }
        }
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioPipeBufferQueueBinder::initialize(OpenSLMediaPlayerInternalContext *context, AudioSinkDataPipe *pipe,
                                           CSLAndroidSimpleBufferQueueItf *buffer_queue, int num_blocks) noexcept
{
    if (CXXPH_UNLIKELY(!getSilentReadBlock(pipe, silent_block_))) {
        return OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
    }

    if (CXXPH_UNLIKELY(!(num_blocks >= 2 && num_blocks <= MAX_BLOCK_COUNT))) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    wait_completion_blocks_.initialize(num_blocks);
    pooled_blocks_.initialize(num_blocks);
    num_blocks_ = num_blocks;
    context_ = context;

#if USE_OSLMP_DEBUG_FEATURES
    nb_logger_.reset(context_->getNonBlockingTraceLogger().create_new_client());
#endif

    return OSLMP_RESULT_SUCCESS;
}

void AudioPipeBufferQueueBinder::handleCallback(AudioSinkDataPipe *pipe,
                                                CSLAndroidSimpleBufferQueueItf *buffer_queue) noexcept
{
    REF_NB_LOGGER_CLIENT(nb_logger_);

    // return used block
    {
        AudioSinkDataPipe::read_block_t rb;

        if (CXXPH_LIKELY(wait_completion_blocks_.dequeue(rb))) {
            if (rb.src != silent_block_.src) {
                pipe->unlockRead(rb);
            }
        } else {
            LOGE("AudioPipeBufferQueueBinder::handleCallback  wait_completion_blocks_.dequeue() returns an error");
        }
    }

    // obtain new block

    AudioSinkDataPipe::read_block_t rb;

    const int num_pooled_blocks = pooled_blocks_.count();
    if (CXXPH_UNLIKELY(num_pooled_blocks > 0)) {
        pooled_blocks_.dequeue(rb);
        wait_buffering_ = 0;
        NB_LOGV("use pooled audio block");
    } else {
        if (CXXPH_LIKELY(pipe->lockRead(rb, wait_buffering_))) {
            wait_buffering_ = 0;
            NB_LOGV("valid audio block (index = %d)", rb.dbg_lock_index);
        } else {
            wait_buffering_ = 1;
            rb = silent_block_;
            NB_LOGI("silent block is used");
        }
    }

    SLresult slResult = buffer_queue->Enqueue(rb.src, getSize(rb));

    if (IS_SL_RESULT_SUCCESS(slResult)) {
        if (CXXPH_LIKELY(wait_completion_blocks_.enqueue(rb))) {
        } else {
            LOGE("AudioPipeBufferQueueBinder::handleCallback  wait_completion_blocks_.enqueue() returns an error");
        }
    } else {
        LOGW("AudioPipeBufferQueueBinder::handleCallback  buffer_queue->Enqueue() returns an error");
        NB_LOGW("AudioPipeBufferQueueBinder::handleCallback  buffer_queue->Enqueue() returns an error");
    }
}

//
// AudioSinkDataPipeReqdBlockQueue
//

bool AudioPipeBufferQueueBinder::getSilentReadBlock(AudioSinkDataPipe *pipe,
                                                    AudioSinkDataPipe::read_block_t &rb) noexcept
{
    {
        AudioSinkDataPipe::write_block_t wb;

        if (CXXPH_UNLIKELY(!pipe->lockWrite(wb))) {
            return false;
        }
        (void)::memset(wb.dest, 0, getSize(wb));
        pipe->unlockWrite(wb);
    }

    return pipe->lockRead(rb);
}

AudioSinkDataPipeReadBlockQueue::AudioSinkDataPipeReadBlockQueue() : capacity_(0), count_(0), rp_(0), wp_(0) {}

AudioSinkDataPipeReadBlockQueue::~AudioSinkDataPipeReadBlockQueue() {}

int AudioSinkDataPipeReadBlockQueue::initialize(int capacity) noexcept
{
    if (CXXPH_UNLIKELY(!(capacity >= 2 && capacity <= MAX_BLOCK_COUNT)))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    capacity_ = capacity;
    clear();

    return OSLMP_RESULT_SUCCESS;
}

int AudioSinkDataPipeReadBlockQueue::capacity() const noexcept { return capacity_; }

int AudioSinkDataPipeReadBlockQueue::count() const noexcept { return count_; }

void AudioSinkDataPipeReadBlockQueue::clear() noexcept
{
    count_ = 0;
    rp_ = 0;
    wp_ = 0;
}

bool AudioSinkDataPipeReadBlockQueue::enqueue(const AudioSinkDataPipe::read_block_t &src) noexcept
{
    if (CXXPH_UNLIKELY(count_ >= capacity_))
        return false;

    rb_queue_[wp_] = src;
    wp_ += 1;
    if (CXXPH_UNLIKELY(wp_ >= capacity_)) {
        wp_ = 0;
    }
    ++count_;

    return true;
}

bool AudioSinkDataPipeReadBlockQueue::dequeue(AudioSinkDataPipe::read_block_t &dest) noexcept
{
    if (CXXPH_UNLIKELY(count_ <= 0))
        return false;

    dest = rb_queue_[rp_];
    rp_ += 1;
    if (CXXPH_UNLIKELY(rp_ >= capacity_)) {
        rp_ = 0;
    }
    --count_;

    return true;
}

} // namespace impl
} // namespace oslmp
