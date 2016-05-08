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

// #define LOG_TAG "AudioSource"
// #define NB_LOG_TAG "NB AudioSource"

#include "oslmp/impl/AudioSource.hpp"

#include <algorithm>
#include <cstring>
#include <vector>
#include <limits>
#include <unistd.h>

#include <cxxporthelper/memory>
#include <cxxporthelper/atomic>
#include <cxxporthelper/aligned_memory.hpp>

#include <SLESCXX/OpenSLES_CXX.hpp>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/OpenSLMediaPlayerMetadata.hpp"
#include "oslmp/impl/AudioSystem.hpp"
#include "oslmp/impl/AudioDataPipeManager.hpp"
#include "oslmp/impl/AudioDataAdapter.hpp"
#include "oslmp/impl/AndroidHelper.hpp"
#include "oslmp/utils/timespec_utils.hpp"
#include "oslmp/utils/pthread_utils.hpp"

// XXX If this option enabled, sound gap will be occurred
//#define ENABLE_DECODER_PAUSING

#define PREFETCHSTATUS_NONE ((SLuint32)0)
#define PREFETCHSTATUS_UNKNOWN_ERROR ((SLuint32)0xFFFFFFFFUL)

#define PREFETCHEVENT_ERROR_CANDIDATE (SL_PREFETCHEVENT_STATUSCHANGE | SL_PREFETCHEVENT_FILLLEVELCHANGE)

#define PRODUCER_QUEUE_PUSH_POLLING_INTERVAL_MS 100

#define TRANSLATE_RESULT(result) InternalUtils::sTranslateOpenSLErrorCode(result)

#define IS_SL_RESULT_SUCCESS(slresult) CXXPH_LIKELY((slresult) == SL_RESULT_SUCCESS)

// NOTE: In current Android's implementation, the remnant data
// of which cannot be deviced by DECODE_BLOCK_SIZE_IN_FRAMES will be discarded.
// So, if this value is increased, more larger part of the end of the audio track will be lost.
#define DECODE_BLOCK_SIZE_IN_FRAMES 1024

#ifdef USE_OSLMP_DEBUG_FEATURES
#define TRACE_DECODER_BUFFER_QUEUE_CALLBACK_STATE(state)    ATRACE_COUNTER("DecoderQueueCallbackState", (state))
#else
#define TRACE_DECODER_BUFFER_QUEUE_CALLBACK_STATE(state)
#endif

namespace oslmp {
namespace impl {

using namespace ::opensles;

typedef OpenSLMediaPlayerInternalUtils InternalUtils;

class DecodeQueueParams {
public:
    DecodeQueueParams();
    DecodeQueueParams(const AudioSource::initialize_args_t &args);

    uint32_t room_for_audio_data_while_playing;
    uint32_t room_for_audio_data_while_paused;
    uint32_t prefetch_count;
    uint32_t required_capacity;
};

class AudioSourcePrepareContext {
public:
    typedef enum {
        PHASE_NONE,
        PHASE_STARTED,
        PHASE_MAKE_SOURCE,
        PHASE_SETUP_CALLBACKS,
        PHASE_START_DECODER_PREFETCH,
        PHASE_WAIT_DECODER_PREFETCH,
        PHASE_GET_METADATA,
        PHASE_CREATE_AUDIODATA_ADAPTER,
        PHASE_SEEK_TO_INITIAL_POSITION,
        PHASE_SETUP_SOURCE_QUEUE,
        PHASE_START_QUEUE_PREFETCH,
        PHASE_WAIT_QUEUE_PREFETCH,
        PHASE_COMPLETED,
    } phase_t;

    AudioSourcePrepareContext() : phase_(PHASE_NONE), error_(0), poll_timeout_ms_(0) {}

    phase_t getPhase() const noexcept { return phase_; }
    void setPhase(phase_t phase) noexcept { phase_ = phase; }

    int getError() const noexcept { return error_; }
    void setError(int error) noexcept { error_ = error; }

    int getPollingTimeoutMs() const noexcept { return poll_timeout_ms_; }
    // void setPollingTimeoutMs(int ms) noexcept { poll_timeout_ms_ = ms; }

    void clear() noexcept
    {
        phase_ = PHASE_NONE;
        error_ = 0;
        poll_timeout_ms_ = 0;
    }

public:
    phase_t phase_;
    int error_;
    int poll_timeout_ms_;
};

class AudioSource::Impl : public AudioDataPipeManager::SourcePipeEventListener {
public:
    enum { NUM_BLOCKS = 2, MAX_NUM_CHANNELS = 2, };

    Impl(AudioSource *holder);
    virtual ~Impl();

    int initialize(const initialize_args_t &args) noexcept;

    int startPreparing(const AudioSource::prepare_args_t &args) noexcept;
    int pollPreparing(AudioSource::prepare_poll_args_t &args) noexcept;

    int start() noexcept;
    int pause() noexcept;

    bool isPreparing() const noexcept;
    bool isPrepared() const noexcept;
    bool isStarted() const noexcept;
    playback_completion_type_t getPlaybackCompletionType() const noexcept;

    uint32_t getId() const noexcept;
    AudioSourceDataPipe *getPipe() const noexcept;

    int getMetaData(OpenSLMediaPlayerMetadata *metadata) const noexcept;
    int getCurrentPosition(int32_t *position) const noexcept;
    int getBufferedPosition(int32_t *position) const noexcept;

    int32_t getInitialSeekPosition() const noexcept;

    bool isNetworkSource() const noexcept;
    bool isDecoderReachedToEndOfData() const noexcept;

    int stopDecoder() noexcept;

    // implementations of AudioDataPipeManager::SourcePipeEventListener
    virtual void onRecycleItem(AudioSourceDataPipe *pipe,
                               const AudioSourceDataPipe::recycle_block_t *block) noexcept override;

private:
    int setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept;
    int setDataSourcePath(const char *path) noexcept;
    int setDataSourceUri(const char *uri) noexcept;

    void releaseDecoderResources() noexcept;

    void prepareInternalCleanup() noexcept;
    int prepareInternalMakeDecoder() noexcept;
    int prepareInternalSetupCallbacks() noexcept;
    int prepareInternalStartDecoderPrefetch() noexcept;
    int prepareInternalPollDecoderPrefetch(int timeout_ms, bool &completed) noexcept;
    int prepareInternalExtractMetadata() noexcept;
    int prepareInternalCreateAudioDataAdapter() noexcept;
    int prepareInternalSeekToInitialPosition() noexcept;
    int prepareInternalSetupDecoderQueue() noexcept;
    int prepareInternalStartQueuePrefetch() noexcept;
    int prepareInternalPollQueuePrefetch(int timeout_ms, bool &completed) noexcept;

    bool checkIsSupportedMedia(const OpenSLMediaPlayerMetadata &metadata) const noexcept;

    static void prefetchEventCallback(SLPrefetchStatusItf caller, void *pContext, SLuint32 event) noexcept;

    void decoderPlayCallback(SLPlayItf caller, SLuint32 event) noexcept;

    void decodeBufferQueueCallback(SLAndroidSimpleBufferQueueItf caller) noexcept;

    static void decoderPlayCallbackEntry(SLPlayItf caller, void *pContext, SLuint32 event) noexcept;

    static void decodeBufferQueueCallbackEntry(SLAndroidSimpleBufferQueueItf caller, void *pContext) noexcept;

    bool waitForProducerQueueAudioDataItem(utils::pt_unique_lock &lock, AudioSourceDataPipe::produce_block_t &pb,
                                           uint32_t retry_wait_ms, uint32_t max_retries) noexcept;

    bool waitForProducerQueueEventItem(utils::pt_unique_lock &lock, AudioSourceDataPipe::produce_block_t &pb,
                                       uint32_t retry_wait_ms, uint32_t max_retries) noexcept;

    bool pushConvertedDataIntoProducerQueue(utils::pt_unique_lock &lock, bool called_from_queue_callback) noexcept;

    int32_t calcCurrentPositionInMsec() noexcept;

private:
    AudioSource *holder_;
    OpenSLMediaPlayerInternalContext *context_;

    initialize_args_t init_args_;

    std::string dataSourceUri_;
    int dataSourceFd_;
    int64_t dataSourceFdOffset_;
    int64_t dataSourceFdLength_;
    bool is_network_source_;

    opensles::CSLObjectItf objDecoder_;

    opensles::CSLPlayItf decoder_;
    opensles::CSLSeekItf decoderSeek_;
    opensles::CSLAndroidSimpleBufferQueueItf decoderBufferQueue_;
    opensles::CSLPrefetchStatusItf decoderPrefetchStatus_;
    opensles::CSLMetadataExtractionItf decoderMetadataExtraction_;

    // Metadata
    OpenSLMediaPlayerMetadata metadata_;
    AudioSourcePrepareContext prepareContext_;

    utils::pt_mutex prefetch_callback_mutex_;
    utils::pt_condition_variable prefetch_callback_cv_;
    SLuint32 prefetch_status_;

    SLuint32 decoder_play_state_;
    int decoderQueueIndex_;
    cxxporthelper::aligned_memory<int16_t> decoderBufferPool_;
    uint32_t decoderBufferBlockSize_;
    uint32_t pipeBufferBlockSize_;

    AudioDataPipeManager *pipe_mgr_;
    AudioSourceDataPipe *pipe_;

    std::unique_ptr<AudioDataAdapter> adapter_;

    int pushed_block_count_;
    std::atomic<bool> decoder_end_of_data_detected_;

    utils::pt_mutex decoder_callback_mutex_;
    utils::pt_condition_variable decoder_callback_cv_;

    int32_t current_position_msec_;
    int32_t init_seek_position_msec_;
    double current_position_calc_coeff_;

    std::atomic<int32_t> buffered_position_msec_;

    playback_completion_type_t playback_completed_;

    DecodeQueueParams queue_params_;

#ifdef USE_OSLMP_DEBUG_FEATURES
    std::unique_ptr<NonBlockingTraceLoggerClient> decoder_callback_nb_logger_;
#endif
};

//
// AudioSource
//
AudioSource::AudioSource() : impl_(new (std::nothrow) Impl(this)) {}

AudioSource::~AudioSource() {}

int AudioSource::initialize(const initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioSource::startPreparing(const AudioSource::prepare_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->startPreparing(args);
}

int AudioSource::pollPreparing(AudioSource::prepare_poll_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->pollPreparing(args);
}

int AudioSource::start() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->start();
}

int AudioSource::pause() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->pause();
}

bool AudioSource::isPreparing() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isPreparing();
}

bool AudioSource::isPrepared() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isPrepared();
}

bool AudioSource::isStarted() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isStarted();
}

AudioSource::playback_completion_type_t AudioSource::getPlaybackCompletionType() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return PLAYBACK_NOT_COMPLETED;
    return impl_->getPlaybackCompletionType();
}

AudioSourceDataPipe *AudioSource::getPipe() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return nullptr;
    return impl_->getPipe();
}

int AudioSource::getMetaData(OpenSLMediaPlayerMetadata *metadata) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getMetaData(metadata);
}

int AudioSource::getCurrentPosition(int32_t *position) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getCurrentPosition(position);
}

int AudioSource::getBufferedPosition(int32_t *position) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getBufferedPosition(position);
}

int32_t AudioSource::getInitialSeekPosition() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0;
    return impl_->getInitialSeekPosition();
}

bool AudioSource::isNetworkSource() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isNetworkSource();
}

bool AudioSource::isDecoderReachedToEndOfData() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isDecoderReachedToEndOfData();
}

int AudioSource::stopDecoder() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->stopDecoder();
}

//
// AudioSource::Impl
//
AudioSource::Impl::Impl(AudioSource *holder)
    : holder_(holder), context_(nullptr), init_args_(), dataSourceUri_(), dataSourceFd_(0), dataSourceFdOffset_(0),
      dataSourceFdLength_(0), is_network_source_(false), objDecoder_(), decoder_(), decoderSeek_(),
      decoderBufferQueue_(), decoderPrefetchStatus_(), decoderMetadataExtraction_(), metadata_(), prepareContext_(),
      prefetch_callback_mutex_(), prefetch_callback_cv_(), prefetch_status_(PREFETCHSTATUS_NONE),
      decoder_play_state_(SL_PLAYSTATE_STOPPED), decoderQueueIndex_(0), decoderBufferPool_(),
      decoderBufferBlockSize_(0), pipeBufferBlockSize_(0), pipe_mgr_(nullptr), pipe_(nullptr), pushed_block_count_(0),
      decoder_end_of_data_detected_(false), decoder_callback_mutex_(), decoder_callback_cv_(),
      current_position_msec_(0), init_seek_position_msec_(0), current_position_calc_coeff_(0),
      buffered_position_msec_(0), playback_completed_(PLAYBACK_NOT_COMPLETED),
      queue_params_()
{
}

AudioSource::Impl::~Impl()
{
    stopDecoder();

    // unregister in port user
    if (pipe_) {
        if (pipe_mgr_) {
            pipe_mgr_->setSourcePipeInPortUser(pipe_, holder_, nullptr, false);
        }
        pipe_ = nullptr;
    }

    metadata_.clear();
    prepareContext_.clear();

    dataSourceUri_.clear();
    dataSourceFd_ = 0;
    dataSourceFdOffset_ = -1;
    dataSourceFdLength_ = -1;
    is_network_source_ = false;

    current_position_msec_ = 0;
    pushed_block_count_ = 0;
    buffered_position_msec_ = 0;

    playback_completed_ = PLAYBACK_NOT_COMPLETED;

    pipe_mgr_ = nullptr;
    context_ = nullptr;
    holder_ = nullptr;
}

int AudioSource::Impl::initialize(const initialize_args_t &args) noexcept
{
    // check arguments
    if (!args.context)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.pipe_manager)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (!args.pipe)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    DecodeQueueParams queue_params(args);

    if (args.pipe->getCapacity() < queue_params.required_capacity) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    // check satte
    if (context_) {
        // already initialized
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    // allocate buffer
    const uint32_t pipeBlockSize = args.pipe_manager->getBlockSizeInFrames();
    const uint32_t decodeBlockSize = DECODE_BLOCK_SIZE_IN_FRAMES;

    cxxporthelper::aligned_memory<int16_t> decoderBuffPool(decodeBlockSize * MAX_NUM_CHANNELS * NUM_BLOCKS);

    // set pipe user
    int result;
    result = args.pipe_manager->setSourcePipeInPortUser(args.pipe, holder_, this, true);
    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

#ifdef USE_OSLMP_DEBUG_FEATURES
    decoder_callback_nb_logger_.reset(args.context->getNonBlockingTraceLogger().create_new_client());
#endif

    // update fields
    decoderBufferPool_ = std::move(decoderBuffPool);

    decoderBufferBlockSize_ = decodeBlockSize;
    pipeBufferBlockSize_ = pipeBlockSize;

    current_position_calc_coeff_ = (pipeBlockSize * 1000) / (args.sampling_rate * 0.001);

    queue_params_ = queue_params;

    init_args_ = args;
    context_ = args.context;
    pipe_mgr_ = args.pipe_manager;
    pipe_ = args.pipe;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::setDataSourceFd(int fd, int64_t offset, int64_t length) noexcept
{
    if (CXXPH_UNLIKELY(!fd))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    dataSourceUri_.clear();
    dataSourceFd_ = fd;
    dataSourceFdOffset_ = offset;
    dataSourceFdLength_ = length;
    is_network_source_ = false;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::setDataSourcePath(const char *path) noexcept
{
    if (CXXPH_UNLIKELY(!path))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    dataSourceUri_ = path;
    dataSourceFd_ = 0;
    dataSourceFdOffset_ = -1;
    dataSourceFdLength_ = -1;
    is_network_source_ = false;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::setDataSourceUri(const char *uri) noexcept
{
    if (CXXPH_UNLIKELY(!uri))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    const static char scheme_http[] = "http://";
    const static char scheme_file[] = "file://";

    dataSourceUri_.clear();
    dataSourceFd_ = 0;
    dataSourceFdOffset_ = -1;
    dataSourceFdLength_ = -1;
    is_network_source_ = false;

    try
    {
        if (::strncmp(uri, scheme_http, sizeof(scheme_http) - 1) == 0) {
            // http
            dataSourceUri_ = uri;
            is_network_source_ = true;
        } else if (::strncmp(uri, scheme_file, sizeof(scheme_file) - 1) == 0) {
            // file
            std::string encoded = uri;
            std::string decoded;

            if (InternalUtils::sDecodeUri(encoded, decoded)) {
                // extract decoded 'path'
                dataSourceUri_ = decoded.substr(sizeof(scheme_file) - 1);
            } else {
                // try to use the original uri
                dataSourceUri_ = encoded;
            }
        } else {
            dataSourceUri_ = uri;
        }
    }
    catch (const std::bad_alloc &)
    {
        dataSourceUri_.clear();
        is_network_source_ = false;
        return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::startPreparing(const AudioSource::prepare_args_t &args) noexcept
{
    typedef AudioSourcePrepareContext C;

    if (CXXPH_UNLIKELY(!args.data_source))
        return OSLMP_RESULT_INTERNAL_ERROR;

    if (CXXPH_UNLIKELY(prepareContext_.getPhase() != C::PHASE_NONE))
        return OSLMP_RESULT_ILLEGAL_STATE;

    // update fields
    int result;
    const data_source_info_t &ds = (*args.data_source);
    switch (args.data_source->type) {
    case DATA_SOURCE_FD:
        result = setDataSourceFd(ds.fd, ds.offset, ds.length);
        break;
    case DATA_SOURCE_PATH:
        result = setDataSourcePath(ds.path_uri.c_str());
        break;
    case DATA_SOURCE_URI:
        result = setDataSourceUri(ds.path_uri.c_str());
        break;
    case DATA_SOURCE_NONE:
    default:
        result = OSLMP_RESULT_ILLEGAL_ARGUMENT;
        break;
    }

    if (CXXPH_UNLIKELY(result != OSLMP_RESULT_SUCCESS))
        return result;

    init_seek_position_msec_ = args.initial_seek_position_msec;

    prepareContext_.setPhase(C::PHASE_STARTED);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::pollPreparing(AudioSource::prepare_poll_args_t &args) noexcept
{
    typedef AudioSourcePrepareContext C;

    C &c = prepareContext_;

    const C::phase_t cur_phase = c.getPhase();
    C::phase_t next_phase;
    SLresult result = SL_RESULT_INTERNAL_ERROR;

    args.need_retry = true;

    switch (cur_phase) {
    case C::PHASE_NONE:
        result = SL_RESULT_SUCCESS;
        next_phase = C::PHASE_STARTED;
        break;
    case C::PHASE_STARTED:
        prepareInternalCleanup();
        result = SL_RESULT_SUCCESS;
        next_phase = C::PHASE_MAKE_SOURCE;
        break;
    case C::PHASE_MAKE_SOURCE:
        result = prepareInternalMakeDecoder();
        next_phase = C::PHASE_SETUP_CALLBACKS;
        break;
    case C::PHASE_SETUP_CALLBACKS:
        result = prepareInternalSetupCallbacks();
        next_phase = C::PHASE_START_DECODER_PREFETCH;
        break;
    case C::PHASE_START_DECODER_PREFETCH:
        result = prepareInternalStartDecoderPrefetch();
        next_phase = C::PHASE_WAIT_DECODER_PREFETCH;
        break;
    case C::PHASE_WAIT_DECODER_PREFETCH: {
        const int timout_ms = c.getPollingTimeoutMs();
        bool poll_completed = false;

        result = prepareInternalPollDecoderPrefetch(timout_ms, poll_completed);

        if (poll_completed) {
            next_phase = C::PHASE_GET_METADATA;
        } else {
            next_phase = C::PHASE_WAIT_DECODER_PREFETCH;
            args.need_retry = false;
        }
    } break;
    case C::PHASE_GET_METADATA:
        result = prepareInternalExtractMetadata();
        if (result == OSLMP_RESULT_SUCCESS) {
            if (!checkIsSupportedMedia(metadata_)) {
                LOGW("Not supported format");
                result = OSLMP_RESULT_CONTENT_UNSUPPORTED;
            }
        }
        next_phase = C::PHASE_CREATE_AUDIODATA_ADAPTER;
        break;
    case C::PHASE_CREATE_AUDIODATA_ADAPTER:
        result = prepareInternalCreateAudioDataAdapter();
        next_phase = C::PHASE_SEEK_TO_INITIAL_POSITION;
        break;
    case C::PHASE_SEEK_TO_INITIAL_POSITION:
        result = prepareInternalSeekToInitialPosition();
        next_phase = C::PHASE_SETUP_SOURCE_QUEUE;
        break;
    case C::PHASE_SETUP_SOURCE_QUEUE:
        result = prepareInternalSetupDecoderQueue();
        next_phase = C::PHASE_START_QUEUE_PREFETCH;
        break;
    case C::PHASE_START_QUEUE_PREFETCH:
        result = prepareInternalStartQueuePrefetch();
        next_phase = C::PHASE_WAIT_QUEUE_PREFETCH;
        break;
    case C::PHASE_WAIT_QUEUE_PREFETCH: {
        const int timout_ms = c.getPollingTimeoutMs();
        bool poll_completed = false;

        result = prepareInternalPollQueuePrefetch(timout_ms, poll_completed);

        if (poll_completed) {
            next_phase = C::PHASE_COMPLETED;
        } else {
            next_phase = C::PHASE_WAIT_QUEUE_PREFETCH;
            args.need_retry = false;
        }
    } break;
    case C::PHASE_COMPLETED:
        result = c.getError();
        next_phase = C::PHASE_COMPLETED;
        break;
    default:
        break;
    }

    if (!IS_SL_RESULT_SUCCESS(result)) {
        next_phase = C::PHASE_COMPLETED;
    }

    if (cur_phase != next_phase) {
        if (next_phase == C::PHASE_COMPLETED) {
            int error = (result == SL_RESULT_SUCCESS) ? OSLMP_RESULT_SUCCESS : OSLMP_RESULT_ERROR;

            c.setError(error);
        }
    }

    c.setPhase(next_phase);

    args.completed = (next_phase == C::PHASE_COMPLETED);

    return result;
}

void AudioSource::Impl::prepareInternalCleanup() noexcept
{
    releaseDecoderResources();

    metadata_.clear();
    prepareContext_.clear();
    current_position_msec_ = 0;
    buffered_position_msec_ = 0;
    pushed_block_count_ = 0;
    decoder_end_of_data_detected_ = 0;
}

void AudioSource::Impl::releaseDecoderResources() noexcept
{
    decoderMetadataExtraction_.unbind();
    decoderPrefetchStatus_.unbind();
    decoderBufferQueue_.unbind();
    decoderSeek_.unbind();
    decoder_.unbind();

    // NOTE: this operation takes time (1000 - 15000 us)
    objDecoder_.Destroy();
}

int AudioSource::Impl::start() noexcept
{
    if (CXXPH_UNLIKELY(!decoder_)) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    if (CXXPH_UNLIKELY(!isPrepared())) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    int result;

    {
        utils::pt_unique_lock lock(decoder_callback_mutex_);

        SLresult slResult;

        slResult = decoder_.SetPlayState(SL_PLAYSTATE_PLAYING);

        if (slResult == SL_RESULT_SUCCESS) {
            decoder_play_state_ = SL_PLAYSTATE_PLAYING;
        }

        result = TRANSLATE_RESULT(slResult);

        decoder_callback_cv_.notify_all();
    }

    return result;
}

int AudioSource::Impl::pause() noexcept
{
    SLresult slResult;

    if (CXXPH_UNLIKELY(!decoder_)) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    if (CXXPH_UNLIKELY(!isPrepared())) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    {
        utils::pt_unique_lock lock(decoder_callback_mutex_);

#ifdef ENABLE_DECODER_PAUSING
        slResult = decoder_.SetPlayState(SL_PLAYSTATE_PAUSED);
        if (slResult == SL_RESULT_SUCCESS) {
            decoder_play_state_ = SL_PLAYSTATE_PAUSED;
        }
#else
        slResult = SL_RESULT_SUCCESS;
        decoder_play_state_ = SL_PLAYSTATE_PAUSED;
#endif

        decoder_callback_cv_.notify_all();
    }

    return TRANSLATE_RESULT(slResult);
}

int AudioSource::Impl::stopDecoder() noexcept
{
    if (CXXPH_UNLIKELY(!objDecoder_)) {
        return OSLMP_RESULT_SUCCESS; // already stopped
    }

    LOGD("stopDecoder()");

    {
        utils::pt_unique_lock lock(decoder_callback_mutex_);

        (void)decoder_.SetPlayState(SL_PLAYSTATE_STOPPED);
        decoder_play_state_ = SL_PLAYSTATE_STOPPED;
        decoder_callback_cv_.notify_all();
    }

    releaseDecoderResources();

    return OSLMP_RESULT_SUCCESS;
}

bool AudioSource::Impl::isPreparing() const noexcept
{
    typedef AudioSourcePrepareContext C;

    const C::phase_t phase = prepareContext_.getPhase();
    return !(phase == C::PHASE_NONE || phase == C::PHASE_COMPLETED);
}

bool AudioSource::Impl::isPrepared() const noexcept
{
    typedef AudioSourcePrepareContext C;

    const C::phase_t phase = prepareContext_.getPhase();
    return (phase == C::PHASE_COMPLETED);
}

bool AudioSource::Impl::isStarted() const noexcept { return (decoder_play_state_ == SL_PLAYSTATE_PLAYING); }

AudioSource::playback_completion_type_t AudioSource::Impl::getPlaybackCompletionType() const noexcept
{
    return playback_completed_;
}

AudioSourceDataPipe *AudioSource::Impl::getPipe() const noexcept { return pipe_; }

int AudioSource::Impl::getMetaData(OpenSLMediaPlayerMetadata *metadata) const noexcept
{
    if (CXXPH_UNLIKELY(!metadata))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    if (CXXPH_UNLIKELY(!isPrepared()))
        return OSLMP_RESULT_ILLEGAL_STATE;

    (*metadata) = (metadata_);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::getCurrentPosition(int32_t *position) const noexcept
{
    if (CXXPH_UNLIKELY(!position))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*position) = current_position_msec_;

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::getBufferedPosition(int32_t *position) const noexcept
{
    if (CXXPH_UNLIKELY(!position))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*position) = buffered_position_msec_.load();

    return OSLMP_RESULT_SUCCESS;
}

int32_t AudioSource::Impl::getInitialSeekPosition() const noexcept { return init_seek_position_msec_; }

bool AudioSource::Impl::isNetworkSource() const noexcept { return is_network_source_; }

bool AudioSource::Impl::isDecoderReachedToEndOfData() const noexcept
{
    return decoder_end_of_data_detected_.load(std::memory_order_acquire);
}

void AudioSource::Impl::onRecycleItem(AudioSourceDataPipe *pipe,
                                      const AudioSourceDataPipe::recycle_block_t *block) noexcept
{

    switch (block->tag) {
    case AudioSourceDataPipe::TAG_AUDIO_DATA:
        // update current position
        current_position_msec_ = block->position_msec;
        break;
    case AudioSourceDataPipe::TAG_EVENT_END_OF_DATA:
        playback_completed_ = PLAYBACK_COMPLETED;
        current_position_msec_ = block->position_msec;
        break;
    case AudioSourceDataPipe::TAG_EVENT_END_OF_DATA_WITH_LOOP_POINT:
        playback_completed_ = PLAYBACK_COMPLETED_WITH_LOOP_POINT;
        current_position_msec_ = block->position_msec;
        break;
    default:
        LOGE("Unknown TAG (%u)", block->tag);
        break;
    }
}

int AudioSource::Impl::prepareInternalMakeDecoder() noexcept
{
    SLresult slResult;
    CSLEngineItf engine;

    slResult = context_->getInterfaceFromEngine(&engine);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // configure audio source
    SLDataLocator_URI src_loc_uri = {};
    SLDataLocator_AndroidFD src_loc_fd = {};
    SLDataFormat_MIME src_format_mime = {};
    SLDataSource audioSrc = {};
    void *src_locator = nullptr;

    if (!dataSourceUri_.empty()) {
        src_loc_uri.locatorType = SL_DATALOCATOR_URI;
        src_loc_uri.URI = reinterpret_cast<SLchar *>(const_cast<char *>(dataSourceUri_.data()));
        src_locator = &src_loc_uri;
    } else {
        src_loc_fd.locatorType = SL_DATALOCATOR_ANDROIDFD;
        src_loc_fd.fd = dataSourceFd_;
        src_loc_fd.offset = (dataSourceFdOffset_ >= 0) ? dataSourceFdOffset_ : 0;
        src_loc_fd.length = (dataSourceFdLength_ >= 0) ? dataSourceFdLength_ : SL_DATALOCATOR_ANDROIDFD_USE_FILE_SIZE;
        src_locator = &src_loc_fd;
    }

    src_format_mime.formatType = SL_DATAFORMAT_MIME;
    src_format_mime.mimeType = nullptr;
    src_format_mime.containerType = SL_CONTAINERTYPE_UNSPECIFIED;

    audioSrc.pLocator = src_locator;
    audioSrc.pFormat = &src_format_mime;

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue sink_loc_destBq = {};
    SLDataFormat_PCM sink_format_destPcm = {};
    SLDataSink audioSink = {};

    sink_loc_destBq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    sink_loc_destBq.numBuffers = NUM_BLOCKS;

    // FIXME: Fill valid values.
    //
    // (However it's impossible to determine them before creating SLPlayItf, because
    //  obtaining SLMetadataExtractionItf from SLEngine is not supported in Android yet.
    //  So far these format is not used in current OpenSL implementation.)
    sink_format_destPcm.formatType = SL_DATAFORMAT_PCM;
    sink_format_destPcm.numChannels = 2;
    sink_format_destPcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
    sink_format_destPcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    sink_format_destPcm.containerSize = 16;
    sink_format_destPcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    sink_format_destPcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

    audioSink.pLocator = &sink_loc_destBq;
    audioSink.pFormat = &sink_format_destPcm;

    CSLObjectItf decoderObj;
    CSLPlayItf decoder;
    CSLSeekItf seek;
    CSLAndroidSimpleBufferQueueItf bufferQueue;
    CSLPrefetchStatusItf prefetchStatus;
    CSLMetadataExtractionItf metadataExtraction;

    // create audio player
    {
        const SLInterfaceID ids[4] = { seek.getIID(), bufferQueue.getIID(), prefetchStatus.getIID(),
                                       metadataExtraction.getIID() };
        const SLboolean req[4] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

        slResult = engine.CreateAudioPlayer(&decoderObj, &audioSrc, &audioSink, 4, ids, req);
    }
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // realize the decoder
    slResult = decoderObj.Realize(SL_BOOLEAN_FALSE);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the decoder interface
    slResult = decoderObj.GetInterface(&decoder);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the seek interface
    slResult = decoderObj.GetInterface(&seek);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the buffer queue interface
    slResult = decoderObj.GetInterface(&bufferQueue);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the prefetch status interface
    slResult = decoderObj.GetInterface(&prefetchStatus);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // get the metadata extraction status interface
    slResult = decoderObj.GetInterface(&metadataExtraction);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    // update fields
    decoderQueueIndex_ = 0;
    objDecoder_ = std::move(decoderObj);

    decoder_ = std::move(decoder);
    decoderSeek_ = std::move(seek);
    decoderBufferQueue_ = std::move(bufferQueue);
    decoderPrefetchStatus_ = std::move(prefetchStatus);
    decoderMetadataExtraction_ = std::move(metadataExtraction);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::prepareInternalSetupCallbacks() noexcept
{
    SLresult slResult;

    slResult = decoder_.SetCallbackEventsMask(SL_PLAYEVENT_HEADATEND |
                                              /* SL_PLAYEVENT_HEADATMARKER | */
                                              /* SL_PLAYEVENT_HEADATNEWPOS | */
                                              /* SL_PLAYEVENT_HEADMOVING |  */
                                              /* SL_PLAYEVENT_HEADSTALLED | */
                                              0);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    slResult = decoder_.RegisterCallback(decoderPlayCallbackEntry, this);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    slResult = decoderBufferQueue_.RegisterCallback(decodeBufferQueueCallbackEntry, this);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::prepareInternalStartDecoderPrefetch() noexcept
{
    SLresult slResult;

    slResult = decoderPrefetchStatus_.RegisterCallback(prefetchEventCallback, this);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    slResult = decoderPrefetchStatus_.SetCallbackEventsMask(PREFETCHEVENT_ERROR_CANDIDATE);
    if (!IS_SL_RESULT_SUCCESS(slResult))
        return TRANSLATE_RESULT(slResult);

    prefetch_status_ = PREFETCHSTATUS_NONE;

    // start prefetching
    slResult = decoder_.SetPlayState(SL_PLAYSTATE_PAUSED);
    if (slResult == SL_RESULT_SUCCESS) {
        decoder_play_state_ = SL_PLAYSTATE_PAUSED;
    }

    return TRANSLATE_RESULT(slResult);
}

int AudioSource::Impl::prepareInternalPollDecoderPrefetch(int timeout_ms, bool &completed) noexcept
{
    utils::pt_mutex &mutex = prefetch_callback_mutex_;
    utils::pt_condition_variable &cond = prefetch_callback_cv_;
    utils::pt_unique_lock lock(mutex, true);
    SLuint32 captured_prefetch_status;

    timespec timeout_abstime;
    int lock_result;

    completed = false;

    if (timeout_ms == 0) {
        lock_result = lock.try_lock();
    } else if (timeout_ms < 0) {
        lock_result = lock.lock();
    } else { // timeout_ms > 0
        if (!utils::timespec_utils::get_current_time(timeout_abstime))
            return OSLMP_RESULT_ILLEGAL_STATE;

        utils::timespec_utils::add_ms(timeout_abstime, timeout_ms);

        // NOTE: The bionic doesn't have pthread_mutex_timedlock() function
        // and pthread_mutex_lock_timeout_np() should be avoided because it's
        // not available on recently Android.

        lock_result = lock.lock();
    }

    if (!lock.owns_lock()) {
        if (lock_result == EBUSY) {
            return OSLMP_RESULT_SUCCESS;
        } else {
            completed = true;
            return OSLMP_RESULT_INTERNAL_ERROR;
        }
    }

    int wait_result = 0;

    while (true) {
        captured_prefetch_status = prefetch_status_;

        if (captured_prefetch_status != PREFETCHSTATUS_NONE)
            break;

        if (timeout_ms == 0) {
            wait_result = ETIMEDOUT;
            break;
        } else if (timeout_ms < 0) {
            wait_result = cond.wait(lock);
        } else { // timeout_ms > 0
            wait_result = cond.wait_absolute(lock, timeout_abstime);
        }

        if (wait_result != 0) {
            break;
        }
    }
    lock.unlock();

    int result = OSLMP_RESULT_INTERNAL_ERROR;

    if (wait_result == 0) {
        completed = true;

#if 0
        // XXX Lack of the head data will be occurred if uncomment this line
        (void) decoder_.SetPlayState(SL_PLAYSTATE_STOPPED);
#endif

        if (captured_prefetch_status == SL_PREFETCHSTATUS_SUFFICIENTDATA) {
            result = OSLMP_RESULT_SUCCESS;
        } else {
            result = OSLMP_RESULT_CONTENT_NOT_FOUND;
        }
    } else if (wait_result == ETIMEDOUT) {
        // return SUCCESS with completed = false (retry expected)
        result = OSLMP_RESULT_SUCCESS;
    } else {
        completed = true;
        result = OSLMP_RESULT_INTERNAL_ERROR;
    }

    return result;
}

int AudioSource::Impl::prepareInternalExtractMetadata() noexcept
{
    CSLPlayItf &player = decoder_;
    CSLMetadataExtractionItf &metaext = decoderMetadataExtraction_;
    OpenSLMediaPlayerMetadata &destMeta = metadata_;
    OpenSLMediaPlayerMetadata tmpMeta;
    SLresult slResult;
    int result;

    // clear destination
    destMeta.clear();

    // obtain info from SLPlayerItf
    {
        SLmillisecond duration = 0;

        slResult = player.GetDuration(&duration);
        if (!IS_SL_RESULT_SUCCESS(slResult))
            return TRANSLATE_RESULT(slResult);

        tmpMeta.duration.set(duration);
    }

    // obtain info from SLMetadataExtractionItf
    try
    {
        SLuint32 itemCount = 0;

        std::vector<uint32_t> keyBuff(16);
        std::vector<uint32_t> valueBuff(16);

        slResult = metaext.GetItemCount(&itemCount);

        if (!IS_SL_RESULT_SUCCESS(slResult))
            return TRANSLATE_RESULT(slResult);

        for (SLuint32 i = 0; i < itemCount; ++i) {
            SLuint32 keySize = 0;
            SLuint32 valueSize = 0;

            // get key & value sizes
            if (metaext.GetKeySize(i, &keySize) != SL_RESULT_SUCCESS)
                break;

            if (metaext.GetValueSize(i, &valueSize) != SL_RESULT_SUCCESS)
                break;

            // resize buffers
            if (keyBuff.size() * sizeof(uint32_t) < keySize) {
                keyBuff.resize((keySize + sizeof(uint32_t) - 1) / sizeof(uint32_t));
            }

            if (valueBuff.size() * sizeof(uint32_t) < valueSize) {
                valueBuff.resize((valueSize + sizeof(uint32_t) - 1) / sizeof(uint32_t));
            }

            // get key & value
            SLMetadataInfo *key = reinterpret_cast<SLMetadataInfo *>(&keyBuff[0]);
            SLMetadataInfo *value = reinterpret_cast<SLMetadataInfo *>(&valueBuff[0]);

            if (metaext.GetKey(i, keySize, key) != SL_RESULT_SUCCESS)
                break;

            if (metaext.GetValue(i, valueSize, value) != SL_RESULT_SUCCESS)
                break;

            // update keySize & valueSize variables
            keySize = key->size;
            valueSize = value->size;

            const char *keyStr = reinterpret_cast<const char *>(key->data);

            const bool sizeIsUint32 = (valueSize) == sizeof(SLuint32);
            union slu32u8_t {
                SLuint32 u32;
                SLuint8 u8[4];
            };
            const slu32u8_t *u32value = reinterpret_cast<const slu32u8_t *>(&(value->data[0]));

            if (sizeIsUint32 && ::strncmp(keyStr, ANDROID_KEY_PCMFORMAT_NUMCHANNELS, keySize) == 0) {
                // num channels
                tmpMeta.numChannels.set(u32value->u32);
            } else if (sizeIsUint32 && ::strncmp(keyStr, ANDROID_KEY_PCMFORMAT_SAMPLERATE, keySize) == 0) {
                // sample rate [Hz]  (!!! NOT milli hertz)
                tmpMeta.samplesPerSec.set(u32value->u32 * 1000UL);
            } else if (sizeIsUint32 && ::strncmp(keyStr, ANDROID_KEY_PCMFORMAT_BITSPERSAMPLE, keySize) == 0) {
                // bits per sample [bits]
                tmpMeta.bitsPerSample.set(u32value->u32);
            } else if (sizeIsUint32 && ::strncmp(keyStr, ANDROID_KEY_PCMFORMAT_CONTAINERSIZE, keySize) == 0) {
                // container size [bytes]
                tmpMeta.containerSize.set(u32value->u32);
            } else if (sizeIsUint32 && ::strncmp(keyStr, ANDROID_KEY_PCMFORMAT_CHANNELMASK, keySize) == 0) {
                // channel mask [ch]
                tmpMeta.channelMask.set(u32value->u32);
            } else if (sizeIsUint32 && ::strncmp(keyStr, ANDROID_KEY_PCMFORMAT_ENDIANNESS, keySize) == 0) {
                // endianness
                tmpMeta.endianness.set(u32value->u32);
            }
        }

        // copy metadata to destination
        if (slResult == SL_RESULT_SUCCESS && tmpMeta.isValid()) {
            destMeta = tmpMeta;
        }

        result = TRANSLATE_RESULT(slResult);
    }
    catch (const std::bad_alloc &) { result = OSLMP_RESULT_MEMORY_ALLOCATION_FAILED; }

    return result;
}

int AudioSource::Impl::prepareInternalCreateAudioDataAdapter() noexcept
{
    uint32_t resampler_quality_level = 0;
    context_->getAudioSystem()->getParamResamplerQualityLevel(&resampler_quality_level);

    AudioDataAdapter::initialize_args_t init_args;

    init_args.in_num_channels = metadata_.numChannels.get();
    init_args.in_sampling_rate = metadata_.samplesPerSec.get();
    init_args.in_block_size = decoderBufferBlockSize_;

    init_args.out_num_channels = 2;
    init_args.out_sampling_rate = init_args_.sampling_rate;
    init_args.out_block_size = pipeBufferBlockSize_;

    init_args.resampler_quality_spec = static_cast<AudioDataAdapter::resampler_quality_spec_t>(resampler_quality_level);

    std::unique_ptr<AudioDataAdapter> adapter(new (std::nothrow) AudioDataAdapter());

    if (!adapter) {
        return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
    }

    if (!adapter->init(init_args)) {
        return OSLMP_RESULT_CONTENT_UNSUPPORTED;
    }

    adapter_ = std::move(adapter);

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::prepareInternalSeekToInitialPosition() noexcept
{
    const int32_t duration = static_cast<int32_t>(metadata_.duration.get());
    int32_t &seek_pos = init_seek_position_msec_;
    SLresult slResult;

    // fix seek position
    seek_pos = (std::min)((std::max)(seek_pos, 0), duration);

    if (seek_pos != 0) {
        slResult = decoderSeek_.SetPosition(seek_pos, SL_SEEKMODE_ACCURATE);
    } else {
        // Workaround
        //  Issue 64053: MediaPlayer seekTo(0) got a clipped audio at the beginning
        //  https://code.google.com/p/android/issues/detail?id=64053
        slResult = SL_RESULT_SUCCESS;
    }

    if (slResult == SL_RESULT_SUCCESS) {
        current_position_msec_ = seek_pos;
    }

    return TRANSLATE_RESULT(slResult);
}

int AudioSource::Impl::prepareInternalSetupDecoderQueue() noexcept
{
    const int block_size = decoderBufferBlockSize_;
    int16_t *dataBuff = &decoderBufferPool_[0];
    CSLAndroidSimpleBufferQueueItf &bufferQueue = decoderBufferQueue_;
    const SLuint32 numChannels = metadata_.numChannels.get();
    SLresult slResult;

    for (int i = 0; i < NUM_BLOCKS; ++i) {
        int16_t *buff = &dataBuff[block_size * numChannels * i];
        size_t size = block_size * numChannels * sizeof(int16_t);

        slResult = bufferQueue.Enqueue(buff, size);
        if (!IS_SL_RESULT_SUCCESS(slResult))
            return TRANSLATE_RESULT(slResult);
    }

    return OSLMP_RESULT_SUCCESS;
}

int AudioSource::Impl::prepareInternalStartQueuePrefetch() noexcept
{
    utils::pt_unique_lock lock(decoder_callback_mutex_);
    SLresult slResult;

    slResult = decoder_.SetPlayState(SL_PLAYSTATE_PLAYING);
    if (slResult == SL_RESULT_SUCCESS) {
        decoder_play_state_ = SL_PLAYSTATE_PLAYING;
    }

    return TRANSLATE_RESULT(slResult);
}

int AudioSource::Impl::prepareInternalPollQueuePrefetch(int timeout_ms, bool &completed) noexcept
{
    utils::pt_unique_lock lock(decoder_callback_mutex_, true);

    timespec timeout_abstime;
    int lock_result;

    completed = false;

    if (timeout_ms == 0) {
        lock_result = lock.try_lock();
    } else if (timeout_ms < 0) {
        lock_result = lock.lock();
    } else { // timeout_ms > 0
        if (!utils::timespec_utils::get_current_time(timeout_abstime))
            return OSLMP_RESULT_ILLEGAL_STATE;

        utils::timespec_utils::add_ms(timeout_abstime, timeout_ms);

        // NOTE: The bionic doesn't have pthread_mutex_timedlock() function
        // and pthread_mutex_lock_timeout_np() should be avoided because it's
        // not available on recently Android.

        lock_result = lock.lock();
    }

    if (!lock.owns_lock()) {
        if (lock_result == EBUSY) {
            return OSLMP_RESULT_SUCCESS;
        } else {
            completed = true;
            return OSLMP_RESULT_INTERNAL_ERROR;
        }
    }

    if (decoder_end_of_data_detected_ || (pushed_block_count_ >= queue_params_.prefetch_count)) {
        // completed

        completed = true;

        SLresult slResult;
#ifdef ENABLE_DECODER_PAUSING
        slResult = decoder_.SetPlayState(SL_PLAYSTATE_PAUSED);
        if (slResult == SL_RESULT_SUCCESS) {
            decoder_play_state_ = SL_PLAYSTATE_PAUSED;
        } else {
            (void)decoder_.SetPlayState(SL_PLAYSTATE_STOPPED);
            decoder_play_state_ = SL_PLAYSTATE_STOPPED;
        }
#else
        decoder_play_state_ = SL_PLAYSTATE_PAUSED;
        slResult = SL_RESULT_SUCCESS;
#endif
        decoder_callback_cv_.notify_all();

        return TRANSLATE_RESULT(slResult);
    } else {
        // not completed yet
        completed = false;
        return OSLMP_RESULT_SUCCESS;
    }
}

bool AudioSource::Impl::checkIsSupportedMedia(const OpenSLMediaPlayerMetadata &metadata) const noexcept
{
    if (!metadata.isValid())
        return false;

    if (metadata.bitsPerSample.get() != SL_PCMSAMPLEFORMAT_FIXED_16)
        return false;

    if (metadata.endianness.get() != SL_BYTEORDER_LITTLEENDIAN)
        return false;

    if (metadata.containerSize.get() != 16)
        return false;

    switch (metadata.numChannels.get()) {
    case 1:
    case 2:
        break;
    default:
        return false;
    }

    // metadata.samplesPerSec
    // metadata.channelMask
    // metadata.duration

    return true;
}

void AudioSource::Impl::prefetchEventCallback(SLPrefetchStatusItf caller, void *pContext, SLuint32 event) noexcept
{
    Impl *thiz = static_cast<Impl *>(pContext);
    SLresult result;
    CSLPrefetchStatusItf caller2;

    caller2.assign(caller);

    SLpermille level = 0;
    SLuint32 status;

    if (!(event & SL_PREFETCHEVENT_STATUSCHANGE)) {
        return;
    }

    result = caller2.GetFillLevel(&level);
    if (!IS_SL_RESULT_SUCCESS(result))
        return;

    result = caller2.GetPrefetchStatus(&status);
    if (!IS_SL_RESULT_SUCCESS(result))
        return;

    {
        utils::pt_unique_lock lock(thiz->prefetch_callback_mutex_);
        SLuint32 corrected_status;

        if (status == SL_PREFETCHSTATUS_SUFFICIENTDATA || status == SL_PREFETCHSTATUS_OVERFLOW ||
            status == SL_PREFETCHSTATUS_UNDERFLOW) {
            corrected_status = status;
        } else {
            corrected_status = PREFETCHSTATUS_UNKNOWN_ERROR;
        }

        thiz->prefetch_status_ = corrected_status;
        thiz->prefetch_callback_cv_.notify_one();
    }
}

void AudioSource::Impl::decoderPlayCallback(SLPlayItf caller, SLuint32 event) noexcept
{
    if (event & SL_PLAYEVENT_HEADATEND) {
        LOGD("decoderPlayCallback(event = HEADATEND)");
    }
    if (event & SL_PLAYEVENT_HEADATMARKER) {
        LOGD("decoderPlayCallback(event = HEADATMARKER)");
    }
    if (event & SL_PLAYEVENT_HEADATNEWPOS) {
        LOGD("decoderPlayCallback(event = HEADATNEWPOS)");
    }
    if (event & SL_PLAYEVENT_HEADMOVING) {
        LOGD("decoderPlayCallback(event = HEADMOVING)");
    }
    if (event & SL_PLAYEVENT_HEADSTALLED) {
        LOGD("decoderPlayCallback(event = HEADSTALLED)");
    }

    if (event & SL_PLAYEVENT_HEADATEND) {
        utils::pt_unique_lock lock(decoder_callback_mutex_);

        // flush adapter pooled data
        adapter_->flush();

        pushConvertedDataIntoProducerQueue(lock, false);

        const int32_t position_in_msec = calcCurrentPositionInMsec();

        // set EOD flag
        decoder_end_of_data_detected_ = true;

        // push TAG_EVENT_END_OF_DATA item
        AudioSourceDataPipe::produce_block_t pb;
        if (waitForProducerQueueEventItem(lock, pb, PRODUCER_QUEUE_PUSH_POLLING_INTERVAL_MS, 500)) {
            // update info
            pb.tag = AudioSourceDataPipe::TAG_EVENT_END_OF_DATA;
            pb.position_msec = position_in_msec;

            // unlock
            pipe_->unlockProduce(pb);

            // update bufferd position
            buffered_position_msec_.store(position_in_msec);
        }
    }
}

void AudioSource::Impl::decoderPlayCallbackEntry(SLPlayItf caller, void *pContext, SLuint32 event) noexcept
{
    static_cast<Impl *>(pContext)->decoderPlayCallback(caller, event);
}

void AudioSource::Impl::decodeBufferQueueCallbackEntry(SLAndroidSimpleBufferQueueItf caller, void *pContext) noexcept
{
    TRACE_DECODER_BUFFER_QUEUE_CALLBACK_STATE(1);
    static_cast<Impl *>(pContext)->decodeBufferQueueCallback(caller);
    TRACE_DECODER_BUFFER_QUEUE_CALLBACK_STATE(0);
}

bool AudioSource::Impl::waitForProducerQueueAudioDataItem(utils::pt_unique_lock &lock,
                                                          AudioSourceDataPipe::produce_block_t &pb,
                                                          uint32_t retry_wait_ms, uint32_t max_retries) noexcept
{
    uint32_t retry_cnt = 0;

    while (true) {
        // lock
        if (CXXPH_LIKELY(decoder_play_state_ == SL_PLAYSTATE_PLAYING)) {
            // active & playing
            if (pipe_->lockProduce(pb, queue_params_.room_for_audio_data_while_playing)) {
                return true;
            } else {
                if (CXXPH_LIKELY(retry_cnt < max_retries)) {
                    // retry
                    retry_cnt++;
                    decoder_callback_cv_.wait_relative_ms(lock, retry_wait_ms);
                    continue;
                } else {
                    LOGW("waitForProducerQueueAudioDataItem() retry %u", retry_cnt);
                    break;
                }
            }
        } else if (decoder_play_state_ == SL_PLAYSTATE_PAUSED) {
            // not active or not playing
            if (pipe_->lockProduce(pb, queue_params_.room_for_audio_data_while_paused)) {
                return true;
            } else {
#ifdef ENABLE_DECODER_PAUSING
                // abandon
                break;
#else
                decoder_callback_cv_.wait(lock);
#endif
            }
        } else {
            break;
        }
    }

    return false;
}

bool AudioSource::Impl::waitForProducerQueueEventItem(utils::pt_unique_lock &lock,
                                                      AudioSourceDataPipe::produce_block_t &pb, uint32_t retry_wait_ms,
                                                      uint32_t max_retries) noexcept
{
    uint32_t retry_cnt = 0;

    while (true) {
        if (decoder_play_state_ == SL_PLAYSTATE_PLAYING || decoder_play_state_ == SL_PLAYSTATE_PAUSED) {
            // lock
            if (CXXPH_LIKELY(pipe_->lockProduce(pb, 0))) {
                return true;
            } else {
                if (retry_cnt < max_retries) {
                    // retry
                    retry_cnt++;
                    decoder_callback_cv_.wait_relative_ms(lock, retry_wait_ms);
                    continue;
                } else {
                    LOGW("waitForProducerQueueEventItem() retry %u", retry_cnt);
                    break;
                }
            }
        } else {
            break;
        }
    }

    return false;
}

bool AudioSource::Impl::pushConvertedDataIntoProducerQueue(utils::pt_unique_lock &lock, bool called_from_queue_callback) noexcept
{
    REF_NB_LOGGER_CLIENT(decoder_callback_nb_logger_);

    const uint32_t out_block_size_in_frames = pipeBufferBlockSize_;
    bool result = true;

    while (adapter_->is_output_data_ready()) {
        const int32_t position_in_msec = calcCurrentPositionInMsec();

        AudioSourceDataPipe::produce_block_t pb;

        if (CXXPH_LIKELY(waitForProducerQueueAudioDataItem(lock, pb, PRODUCER_QUEUE_PUSH_POLLING_INTERVAL_MS, 500))) {
            TRACE_DECODER_BUFFER_QUEUE_CALLBACK_STATE(4);
            adapter_->get_output_data(pb.dest, 2, out_block_size_in_frames);
            TRACE_DECODER_BUFFER_QUEUE_CALLBACK_STATE(3);
            // update info
            pb.tag = AudioSourceDataPipe::TAG_AUDIO_DATA;
            pb.position_msec = position_in_msec;

            // unlock
            pipe_->unlockProduce(pb);

            NB_LOGV("pushConvertedDataIntoProducerQueue()");
        } else {
            result = false;
            break;
        }

        pushed_block_count_ += 1;

        // update bufferd position
        buffered_position_msec_.store(position_in_msec);
    }

    return result;
}

void AudioSource::Impl::decodeBufferQueueCallback(SLAndroidSimpleBufferQueueItf caller) noexcept
{
    utils::pt_unique_lock lock(decoder_callback_mutex_);

    if (CXXPH_UNLIKELY(decoderBufferQueue_.self() != caller))
        return;

    SLresult result;

    const int index = decoderQueueIndex_;

#if 1
    static_assert(((NUM_BLOCKS & (NUM_BLOCKS - 1)) == 0), "Check NUM_BLOCKS is power of two");
    decoderQueueIndex_ = (index + 1) & (NUM_BLOCKS - 1);
#else
    decoderQueueIndex_ = (index + 1) % NUM_BLOCKS;
#endif

    {
        const SLuint32 numChannels = metadata_.numChannels.get();
        const uint32_t block_size_in_frames = decoderBufferBlockSize_;
        const size_t block_size_in_bytes = sizeof(int16_t) * block_size_in_frames * numChannels;
        const int16_t *block = &(decoderBufferPool_[block_size_in_frames * numChannels * index]);

        AudioSourceDataPipe::produce_block_t pb;

        // put decoded data
        TRACE_DECODER_BUFFER_QUEUE_CALLBACK_STATE(2);
        adapter_->put_input_data(block, numChannels, block_size_in_frames);
        TRACE_DECODER_BUFFER_QUEUE_CALLBACK_STATE(3);

        // re-enqueue the empty buffer
        decoderBufferQueue_.Enqueue(block, block_size_in_bytes);
    }

    pushConvertedDataIntoProducerQueue(lock, true);
}

int32_t AudioSource::Impl::calcCurrentPositionInMsec() noexcept
{
#if 0
    const uint32_t block_size_in_frames = pipeBufferBlockSize_;
    const uint32_t sample_rate = init_args_.sampling_rate / 1000;
    const uint64_t offset_in_frames = static_cast<uint64_t>(pushed_block_count_) * block_size_in_frames;
    const int32_t offset_in_msec = (sample_rate != 0) ? static_cast<int32_t>(offset_in_frames * 1000 / sample_rate) : 0;
#else
    const int32_t offset_in_msec = static_cast<int32_t>(current_position_calc_coeff_ * pushed_block_count_);
#endif
    const int32_t position_in_msec = init_seek_position_msec_ + offset_in_msec;
    const int32_t duration = static_cast<int32_t>(metadata_.duration.get());

    return (std::min)(position_in_msec, duration);
}

DecodeQueueParams::DecodeQueueParams()
: room_for_audio_data_while_playing(0),
  room_for_audio_data_while_paused(0),
  prefetch_count(0),
  required_capacity(0)
{

}

DecodeQueueParams::DecodeQueueParams(const AudioSource::initialize_args_t &args)
: room_for_audio_data_while_playing(0),
  room_for_audio_data_while_paused(0),
  prefetch_count(0),
  required_capacity(0)
{
    const uint32_t kMinRoomForWhilePlaying = 2;
    const uint32_t kMinRoomForWhilePaused = 2;
    const uint32_t kMinPrefetchCount = 4;
    const uint32_t kRoomForWhilePlayingMsec = 100;
    const uint32_t kRoomForWhilePausedMsec = 100;
    const uint32_t kPrefetchCountMsec = 500;

    const uint32_t pipe_block_size = args.pipe_manager->getBlockSizeInFrames();
    const uint32_t sampling_rate_hz = args.sampling_rate / 1000;

    room_for_audio_data_while_playing = (std::max)(
        kMinRoomForWhilePlaying, ((sampling_rate_hz * kRoomForWhilePlayingMsec + (pipe_block_size * 500u)) / (pipe_block_size * 1000u)));

    room_for_audio_data_while_paused = (std::max)(
        kMinRoomForWhilePaused, ((sampling_rate_hz * kRoomForWhilePausedMsec + (pipe_block_size * 500u)) / (pipe_block_size * 1000u)));

    prefetch_count = (std::max)(
        kMinPrefetchCount, ((sampling_rate_hz * kPrefetchCountMsec + (pipe_block_size * 500u)) / (pipe_block_size * 1000u)));

    required_capacity = (std::max)(room_for_audio_data_while_playing, room_for_audio_data_while_paused) + prefetch_count;
}


} // namespace impl
} // namespace oslmp
