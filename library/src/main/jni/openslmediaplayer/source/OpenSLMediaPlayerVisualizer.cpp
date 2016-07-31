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

// #define LOG_TAG "OpenSLMediaPlayerVisualizer"

#include "oslmp/OpenSLMediaPlayerVisualizer.hpp"

#include <cassert>
#include <cstring>
#include <algorithm>
#include <unistd.h>

#include <cxxporthelper/memory>
#include <cxxporthelper/atomic>
#include <cxxporthelper/cmath>
#include <cxxporthelper/compiler.hpp>
#include <cxxporthelper/aligned_memory.hpp>

#include <cxxdasp/datatype/audio_frame.hpp>
#include <cxxdasp/converter/sample_format_converter.hpp>
#include <cxxdasp/converter/sample_format_converter_core_operators.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/OpenSLMediaPlayerThreadMessage.hpp"
#include "oslmp/impl/VisualizerCapturedAudioDataBuffer.hpp"
#include "oslmp/impl/BaseExtensionModule.hpp"
#include "oslmp/impl/StockVisualizerAlgorithms.hpp"
#include "oslmp/impl/AndroidHelper.hpp"
#include "oslmp/utils/pthread_utils.hpp"
#include "oslmp/utils/timespec_utils.hpp"
#include "oslmp/utils/optional.hpp"

//
// Constants
//
#define MODULE_NAME "Visualizer"

#if 0
// for debugging
#define MIN_CAPTURE_DATA_SIZE 128      // [frames]
#define MAX_CAPTURE_DATA_SIZE 4096     // [frames]
#define DEFAULT_CAPTURE_DATA_SIZE 4096 // [frames]

#define MIN_CAPTURE_RATE 100       // [milli hertz]
#define MAX_CAPTURE_RATE 60000     // [milli hertz]
#define DEFAULT_CAPTURE_RATE 60000 // [milli hertz]
#else
// same with standard visualizer spec.
#define MIN_CAPTURE_DATA_SIZE 128      // [frames]
#define MAX_CAPTURE_DATA_SIZE 1024     // [frames]
#define DEFAULT_CAPTURE_DATA_SIZE 1024 // [frames]

#define MIN_CAPTURE_RATE 100       // [milli hertz]
#define MAX_CAPTURE_RATE 20000     // [milli hertz]
#define DEFAULT_CAPTURE_RATE 10000 // [milli hertz]
#endif

#define CAPTURE_BUFFER_SIZE_IN_FRAMES 32768 // [frames] (= 0.68 sec. @ 48000 Hz)

#define DEFAULT_SCALING_MODE OpenSLMediaPlayerVisualizer::SCALING_MODE_NORMALIZED
#define DEFAULT_MEASUREMENT_MODE OpenSLMediaPlayerVisualizer::MEASUREMENT_MODE_NONE

#define DISCARD_MEASUREMENTS_TIME_MS 2000 // discard measurements older than this number of ms

// maximum number of buffers for which we keep track of the measurements
#define MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS 25

#define MEASUREMENT_WINDOW_MAX_SIZE_IN_MILLISECONDS 650 // [ms]

#define CAPTURE_BUFFER_TYPE_WAVEFORM                                                                                   \
    OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener::BUFFER_TYPE_WAVEFORM
#define CAPTURE_BUFFER_TYPE_FFT OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener::BUFFER_TYPE_FFT

//
// helper macros
//

#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    VisualizerExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                               \
    if (CXXPH_UNLIKELY(!varname)) {                                                                                    \
        return OSLMP_RESULT_DEAD_OBJECT;                                                                               \
    }

#define CHECK_ARG(cond)                                                                                                \
    if (CXXPH_UNLIKELY(!(cond))) {                                                                                     \
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;                                                                          \
    }

#define IS_SL_SUCCESS(result) (CXXPH_LIKELY((result) == SL_RESULT_SUCCESS))

#define CHECK_IS_ACTIVE(blob_) (CXXPH_LIKELY(checkIsClientActive((blob_).client)))

#define CHECK_RANGE(value_, min_, max_) (CXXPH_LIKELY(((value_) >= (min_)) && ((value_) <= (max_))))

#define CHECK_ARG_RANGE(value_, min_, max_) CHECK_ARG(CHECK_RANGE((value_), (min_), (max_)))

#define IS_POW_OF_TWO(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

#define MASK_ENABLED (static_cast<uintptr_t>(1))
#define MASK_CLIENT (~static_cast<uintptr_t>(1))

#define EXT_ENABLED(v) (((v) & MASK_ENABLED) ? true : false)
#define EXT_CLIENT(v) (reinterpret_cast<void *>((v) & MASK_CLIENT))

#define PACK_WAVE_FFT_STATUS(enabled, client)                                                                          \
    (((enabled) ? MASK_ENABLED : 0) | (reinterpret_cast<uintptr_t>(client) & MASK_CLIENT))

#define CLIENT_INFO(thiz) (&((thiz)->impl_->client_info_))

namespace oslmp {

using namespace ::opensles;
using namespace ::oslmp::impl;

typedef OpenSLMediaPlayerInternalContext InternalContext;
typedef android::sp<OpenSLMediaPlayerVisualizer::OnDataCaptureListener> sp_on_data_capture_listener_t;
typedef android::sp<OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener>
sp_internal_periodic_capture_thread_event_listener_t;

struct VisualizerStatus {
    bool enabled;
    std::atomic<uint32_t> capture_size;  // [frames]
    std::atomic<uint32_t> sampling_rate; // [milli hertz]
    std::atomic<int32_t> scaling_mode;
    int32_t measurement_mode;

    std::atomic<uintptr_t> get_waveform_fft_state; // bit0: enabled

    VisualizerStatus()
        : enabled(false), capture_size(DEFAULT_CAPTURE_DATA_SIZE), sampling_rate(0), scaling_mode(DEFAULT_SCALING_MODE),
          measurement_mode(DEFAULT_MEASUREMENT_MODE), get_waveform_fft_state()
    {
    }
};

struct MeasureData {
    bool mIsValid;
    uint16_t mPeakU16;
    float mRmsSquared;

    MeasureData() : mIsValid(false), mPeakU16(0U), mRmsSquared(0.0f) {}

    void clear() noexcept
    {
        mIsValid = false;
        mPeakU16 = 0U;
        mRmsSquared = 0.0f;
    }
};

struct VisualizerMeasurementContext {
    enum {
        MIN_DB = -9600 // -96.0 dB
    };
    MeasureData mBuffer[MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS];
    uint32_t mBufferIndex;
    int32_t mPeak;
    int32_t mRms;
    int32_t mMaxValidNumBuffers;

    VisualizerMeasurementContext()
        : mBufferIndex(0), mPeak(MIN_DB), mRms(MIN_DB), mMaxValidNumBuffers(MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS)
    {
    }

    void clear() noexcept
    {
        for (auto &md : mBuffer) {
            md.clear();
        }
        mBufferIndex = 0;
        mPeak = MIN_DB;
        mRms = MIN_DB;
    }

    void setMaxNumValidBuffers(int num) noexcept
    {
        mMaxValidNumBuffers = (std::min)((std::max)(num, 1), MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS);
    }

    void calcPeakRms() noexcept
    {
        uint16_t peakU16 = 0;
        float sumRmsSquared = 0.0f;
        int numValids = 0;

        for (const auto &md : mBuffer) {
            if (!md.mIsValid)
                continue;

            peakU16 = (std::max)(peakU16, md.mPeakU16);
            sumRmsSquared += md.mRmsSquared;

            ++numValids;

            if (numValids >= mMaxValidNumBuffers)
                break;
        }

        float rms = (numValids == 0) ? 0.0f : sqrtf(sumRmsSquared / numValids);

        this->mPeak = StockVisualizerAlgorithms::linearToMillibel(peakU16);
        this->mRms = StockVisualizerAlgorithms::linearToMillibel(rms);
    }
};

struct ClientInfo {
    void *const client;

    uint32_t periodic_capture_rate; // [milli hertz]
    bool periodic_capture_waveform;
    bool periodic_capture_fft;

    sp_on_data_capture_listener_t periodic_capture_listener;
    sp_internal_periodic_capture_thread_event_listener_t internal_event_listener;

    ClientInfo(void *client)
        : client(client), periodic_capture_rate(DEFAULT_CAPTURE_RATE), periodic_capture_waveform(false),
          periodic_capture_fft(false), periodic_capture_listener(), internal_event_listener()
    {
    }

    bool preparedForCaptuirng() const noexcept
    {
        return (periodic_capture_rate > 0) && (periodic_capture_waveform || periodic_capture_fft) &&
               (periodic_capture_listener.get() || internal_event_listener.get());
    }
};

struct periodic_capture_thread_context_t {
    OpenSLMediaPlayerVisualizer *visualizer;

    uint32_t capture_rate;
    uint32_t capture_size;
    int32_t scaling_mode;
    bool capture_waveform;
    bool capture_fft;
    sp_on_data_capture_listener_t listener;
    sp_internal_periodic_capture_thread_event_listener_t internal_event_listener;
    std::unique_ptr<int8_t[]> fft_buff;
    std::unique_ptr<uint8_t[]> waveform_buff;

    uint32_t sampling_rate;
    uint32_t capture_start_delay;
};

class VisualizerExtModule : public BaseExtensionModule {
public:
    VisualizerExtModule();
    virtual ~VisualizerExtModule();

    int setEnabled(void *client, bool enabled) noexcept;
    int getEnabled(void *client, bool *enabled) noexcept;
    int getCaptureSize(void *client, size_t *size) noexcept;
    int setCaptureSize(void *client, size_t size) noexcept;
    int getFft(void *client, int8_t *fft, size_t size) noexcept;
    int getWaveForm(void *client, uint8_t *waveform, size_t size) noexcept;
    int getMeasurementPeakRms(void *client, OpenSLMediaPlayerVisualizer::MeasurementPeakRms *measurement) noexcept;
    int getSamplingRate(void *client, uint32_t *samplingRate) noexcept;
    int getMeasurementMode(void *client, int32_t *mode) noexcept;
    int setMeasurementMode(void *client, int32_t mode) noexcept;
    int getScalingMode(void *client, int32_t *mode) noexcept;
    int setScalingMode(void *client, int32_t mode) noexcept;
    int setDataCaptureListener(void *client, OpenSLMediaPlayerVisualizer::OnDataCaptureListener *listener,
                               uint32_t rate, bool waveform, bool fft) noexcept;
    int setInternalPeriodicCaptureThreadEventListener(
        void *client, OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener *listener, uint32_t rate,
        bool waveform, bool fft) noexcept;
    int release(void *client) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual uint32_t getExtensionTraits() const noexcept override;

    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onAttach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onDetach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept override;

    virtual void onCaptureAudioData(OpenSLMediaPlayerExtensionManager *extmgr, const float *data, uint32_t num_channels,
                                    size_t num_frames, uint32_t sample_rate_millihertz,
                                    const timespec *timestamp) noexcept override;

private:
#if CXXPH_COMPILER_SUPPORTS_ARM_NEON
    typedef cxxdasp::converter::f32_to_s16_stereo_neon_fast_sample_format_converter_core_operator
    f32_to_s16_stereo_sample_format_converter_core_operator_t;
#elif(CXXPH_TARGET_ARCH == CXXPH_ARCH_I386) || (CXXPH_TARGET_ARCH == CXXPH_ARCH_X86_64)
    typedef cxxdasp::converter::f32_to_s16_stereo_sse_sample_format_converter_core_operator
    f32_to_s16_stereo_sample_format_converter_core_operator_t;
#else
    typedef cxxdasp::converter::general_sample_format_converter_core_operator<cxxdasp::datatype::f32_stereo_frame_t,
                                                                              cxxdasp::datatype::s16_stereo_frame_t>
    f32_to_s16_stereo_sample_format_converter_core_operator_t;
#endif

    SLresult processMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                            const OpenSLMediaPlayerThreadMessage *msg) noexcept;

    SLresult startPeriodicCapturing() noexcept;
    SLresult stopPeriodicCapturing() noexcept;
    void periodicCaptureThread(periodic_capture_thread_context_t &c) noexcept;
    void processPeriodicCapture(periodic_capture_thread_context_t &c) noexcept;

    static void *periodicCaptureThreadEntryFunc(void *args) noexcept;

    int32_t calcDiffTimeSinceLastCaptureBufferUpdated() const noexcept;
    int32_t calcDiffTimeSinceLastCaptureBufferUpdated(const timespec &last_updated) const noexcept;

private:
    VisualizerStatus status_;
    VisualizerMeasurementContext measurement_;

    VisualizerCapturedAudioDataBuffer captured_data_buffer_;
    utils::optional<pthread_t> periodic_capture_thread_;
    utils::pt_mutex mutex_cond_periodic_capture_thread_;
    utils::pt_condition_variable cond_periodic_capture_thread_;
    bool periodic_capture_thread_initialized_;
    std::atomic<bool> periodic_capture_thread_stop_req_;

    cxxporthelper::aligned_memory<cxxdasp::datatype::s16_stereo_frame_t> s16_data_buff_;
    cxxdasp::converter::sample_format_converter<
        cxxdasp::datatype::f32_stereo_frame_t, cxxdasp::datatype::s16_stereo_frame_t,
        f32_to_s16_stereo_sample_format_converter_core_operator_t> f32_to_s16_converter_;
};

class VisualizerExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    VisualizerExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) VisualizerExtModule();
    }
};

class OpenSLMediaPlayerVisualizer::Impl {
public:
    Impl(void *client, const android::sp<OpenSLMediaPlayerContext> &context);
    ~Impl();

    VisualizerExtModule *module_;
    ClientInfo client_info_;
};

enum {
    MSG_NOP,
    MSG_SET_ENABLED,
    MSG_GET_ENABLED,
    MSG_GET_CAPTURE_SIZE,
    MSG_SET_CAPTURE_SIZE,
    MSG_GET_MEASUREMENT_PEAK_RMS,
    MSG_GET_SAMPLING_RATE,
    MSG_GET_MEASUREMENT_MODE,
    MSG_SET_MEASUREMENT_MODE,
    MSG_GET_SCALING_MODE,
    MSG_SET_SCALING_MODE,
    MSG_SET_DATA_CAPTURE_LISTENER,
    MSG_SET_INTERNAL_PERIODIC_CAPTURE_EVENT_LISTENER,
};

struct msg_blob_set_enabled {
    void *client;
    bool enabled;
};

struct msg_blob_get_enabled {
    void *client;
    bool *enabled;
};

struct msg_blob_get_capture_size {
    void *client;
    size_t *size;
};

struct msg_blob_set_capture_size {
    void *client;
    size_t size;
};

struct msg_blob_get_measurement_peak_rms {
    void *client;
    OpenSLMediaPlayerVisualizer::MeasurementPeakRms *measurement;
};

struct msg_blob_get_sampling_rate {
    void *client;
    uint32_t *samplingRate;
};

struct msg_blob_get_measurement_mode {
    void *client;
    int32_t *mode;
};

struct msg_blob_set_measurement_mode {
    void *client;
    int32_t mode;
};

struct msg_blob_get_scaling_mode {
    void *client;
    int32_t *mode;
};

struct msg_blob_set_scaling_mode {
    void *client;
    int32_t mode;
};

struct msg_blob_set_data_capture_listener {
    void *client;
    OpenSLMediaPlayerVisualizer::OnDataCaptureListener *listener;
    uint32_t rate;
    bool waveform;
    bool fft;
};

struct msg_blob_set_internal_periodic_capture_thread_event_listener {
    void *client;
    OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener *listener;
    uint32_t rate;
    bool waveform;
    bool fft;
};

//
// OpenSLMediaPlayerVisualizer
//
OpenSLMediaPlayerVisualizer::OpenSLMediaPlayerVisualizer(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(this, context))
{
}

OpenSLMediaPlayerVisualizer::~OpenSLMediaPlayerVisualizer()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerVisualizer::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(CLIENT_INFO(this), enabled);
}

int OpenSLMediaPlayerVisualizer::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(CLIENT_INFO(this), enabled);
}

int OpenSLMediaPlayerVisualizer::getMaxCaptureRate(uint32_t *rate) const noexcept
{
    GET_MODULE_INSTANCE(module);
    return sGetMaxCaptureRate(rate);
}

int OpenSLMediaPlayerVisualizer::getCaptureSizeRange(size_t *range) const noexcept
{
    GET_MODULE_INSTANCE(module);
    return sGetCaptureSizeRange(range);
}

int OpenSLMediaPlayerVisualizer::getCaptureSize(size_t *size) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getCaptureSize(CLIENT_INFO(this), size);
}

int OpenSLMediaPlayerVisualizer::setCaptureSize(size_t size) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setCaptureSize(CLIENT_INFO(this), size);
}

int OpenSLMediaPlayerVisualizer::getFft(int8_t *fft, size_t size) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getFft(CLIENT_INFO(this), fft, size);
}

int OpenSLMediaPlayerVisualizer::getWaveForm(uint8_t *waveform, size_t size) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getWaveForm(CLIENT_INFO(this), waveform, size);
}

int OpenSLMediaPlayerVisualizer::getMeasurementPeakRms(
    OpenSLMediaPlayerVisualizer::MeasurementPeakRms *measurement) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getMeasurementPeakRms(CLIENT_INFO(this), measurement);
}

int OpenSLMediaPlayerVisualizer::getSamplingRate(uint32_t *samplingRate) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getSamplingRate(CLIENT_INFO(this), samplingRate);
}

int OpenSLMediaPlayerVisualizer::getMeasurementMode(int32_t *mode) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getMeasurementMode(CLIENT_INFO(this), mode);
}

int OpenSLMediaPlayerVisualizer::setMeasurementMode(int32_t mode) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setMeasurementMode(CLIENT_INFO(this), mode);
}

int OpenSLMediaPlayerVisualizer::getScalingMode(int32_t *mode) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getScalingMode(CLIENT_INFO(this), mode);
}

int OpenSLMediaPlayerVisualizer::setScalingMode(int32_t mode) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setScalingMode(CLIENT_INFO(this), mode);
}

int OpenSLMediaPlayerVisualizer::setDataCaptureListener(OpenSLMediaPlayerVisualizer::OnDataCaptureListener *listener,
                                                        uint32_t rate, bool waveform, bool fft) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setDataCaptureListener(CLIENT_INFO(this), listener, rate, waveform, fft);
}

int OpenSLMediaPlayerVisualizer::setInternalPeriodicCaptureThreadEventListener(
    OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener *listener, uint32_t rate, bool waveform,
    bool fft) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setInternalPeriodicCaptureThreadEventListener(CLIENT_INFO(this), listener, rate, waveform, fft);
}

int OpenSLMediaPlayerVisualizer::sGetMaxCaptureRate(uint32_t *rate) noexcept
{
    CHECK_ARG(rate != nullptr);

    rate[0] = MAX_CAPTURE_RATE;

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayerVisualizer::sGetCaptureSizeRange(size_t *range) noexcept
{
    CHECK_ARG(range != nullptr);

    range[0] = MIN_CAPTURE_DATA_SIZE;
    range[1] = MAX_CAPTURE_DATA_SIZE;

    return OSLMP_RESULT_SUCCESS;
}

//
// OpenSLMediaPlayerVisualizer::Impl
//
OpenSLMediaPlayerVisualizer::Impl::Impl(void *client, const android::sp<OpenSLMediaPlayerContext> &context)
    : client_info_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_VISUALIZER) {
        const VisualizerExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module = nullptr;

        int result = c.extAttachOrInstall(&module, &creator, &client_info_);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<VisualizerExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerVisualizer::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(&client_info_);

        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// VisualizerExtModule
//
VisualizerExtModule::VisualizerExtModule()
    : BaseExtensionModule(MODULE_NAME), status_(), measurement_(),
      captured_data_buffer_(CAPTURE_BUFFER_SIZE_IN_FRAMES, MAX_CAPTURE_DATA_SIZE), periodic_capture_thread_(),
      mutex_cond_periodic_capture_thread_(), cond_periodic_capture_thread_(),
      periodic_capture_thread_initialized_(false), s16_data_buff_(), f32_to_s16_converter_()
{
}

VisualizerExtModule::~VisualizerExtModule() {}

int VisualizerExtModule::setEnabled(void *client, bool enabled) noexcept
{
    typedef msg_blob_set_enabled blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(0, MSG_SET_ENABLED);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.enabled = enabled;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::getEnabled(void *client, bool *enabled) noexcept
{
    typedef msg_blob_get_enabled blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(enabled != nullptr);

    Message msg(0, MSG_GET_ENABLED);
    bool value = false;

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.enabled = &value;
    }

    const int result = postAndWaitResult(&msg);

    (*enabled) = value;

    return result;
}

int VisualizerExtModule::getCaptureSize(void *client, size_t *size) noexcept
{
    typedef msg_blob_get_capture_size blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(size != nullptr);

    Message msg(0, MSG_GET_CAPTURE_SIZE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.size = size;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::setCaptureSize(void *client, size_t size) noexcept
{
    typedef msg_blob_set_capture_size blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    // NOTE: check argument in message handler
    // CHECK_ARG_RANGE(size, MIN_CAPTURE_DATA_SIZE, MAX_CAPTURE_DATA_SIZE);

    Message msg(0, MSG_SET_CAPTURE_SIZE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.size = size;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::getFft(void *client, int8_t *fft, size_t size) noexcept
{
    // NOTE:
    // This method bypasses message communication for performance reason

    const uint32_t capture_size = status_.capture_size;
    uintptr_t state = status_.get_waveform_fft_state;

    CHECK_ARG(fft);
    CHECK_ARG(size == capture_size);

    if (!EXT_ENABLED(state)) {
        // disabled
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    if (client != EXT_CLIENT(state)) {
        // control lost
        return OSLMP_RESULT_CONTROL_LOST;
    }

    bool filled = false;
    uint32_t num_channels = 0;
    uint32_t sampling_rate = 0;
    const int16_t *data = 0;
    timespec updatedTime = utils::timespec_utils::ZERO();

    if (captured_data_buffer_.get_latest_captured_data(capture_size, &num_channels, &sampling_rate, &data,
                                                       &updatedTime)) {

        const int32_t delayMs = calcDiffTimeSinceLastCaptureBufferUpdated(updatedTime);

        if (delayMs <= DISCARD_MEASUREMENTS_TIME_MS) {
            uint8_t waveform[capture_size];

            StockVisualizerAlgorithms::convertWaveformS16StereoToU8Mono(waveform, data, capture_size,
                                                                        status_.scaling_mode);

            StockVisualizerAlgorithms::doFft(fft, waveform, capture_size);

            filled = true;
        }
    }

    if (!filled) {
        ::memset(fft, 0, capture_size);
    }

    return OSLMP_RESULT_SUCCESS;
}

int VisualizerExtModule::getWaveForm(void *client, uint8_t *waveform, size_t size) noexcept
{
    // NOTE:
    // This method bypasses message communication for performance reason

    const uint32_t capture_size = status_.capture_size;
    uintptr_t state = status_.get_waveform_fft_state;

    CHECK_ARG(waveform);
    CHECK_ARG(size == capture_size);

    if (!EXT_ENABLED(state)) {
        // disabled
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    if (client != EXT_CLIENT(state)) {
        // control lost
        return OSLMP_RESULT_CONTROL_LOST;
    }

    bool filled = false;
    uint32_t num_channels = 0;
    uint32_t sampling_rate = 0;
    const int16_t *data = 0;
    timespec updatedTime = utils::timespec_utils::ZERO();

    if (captured_data_buffer_.get_latest_captured_data(capture_size, &num_channels, &sampling_rate, &data,
                                                       &updatedTime)) {

        const int32_t delayMs = calcDiffTimeSinceLastCaptureBufferUpdated(updatedTime);

        if (delayMs <= DISCARD_MEASUREMENTS_TIME_MS) {
            StockVisualizerAlgorithms::convertWaveformS16StereoToU8Mono(waveform, data, capture_size,
                                                                        status_.scaling_mode);
            filled = true;
        }
    }

    if (!filled) {
        ::memset(waveform, 128, capture_size);
    }

    return OSLMP_RESULT_SUCCESS;
}

int VisualizerExtModule::getMeasurementPeakRms(void *client,
                                               OpenSLMediaPlayerVisualizer::MeasurementPeakRms *measurement) noexcept
{
    typedef msg_blob_get_measurement_peak_rms blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(measurement != nullptr);

    Message msg(0, MSG_GET_MEASUREMENT_PEAK_RMS);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.measurement = measurement;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::getSamplingRate(void *client, uint32_t *samplingRate) noexcept
{
    typedef msg_blob_get_sampling_rate blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(samplingRate != nullptr);

    Message msg(0, MSG_GET_SAMPLING_RATE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.samplingRate = samplingRate;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::getMeasurementMode(void *client, int32_t *mode) noexcept
{
    typedef msg_blob_get_measurement_mode blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(mode != nullptr);

    Message msg(0, MSG_GET_MEASUREMENT_MODE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.mode = mode;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::setMeasurementMode(void *client, int32_t mode) noexcept
{
    typedef msg_blob_set_measurement_mode blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(mode == OpenSLMediaPlayerVisualizer::MEASUREMENT_MODE_NONE ||
              mode == OpenSLMediaPlayerVisualizer::MEASUREMENT_MODE_PEAK_RMS);

    Message msg(0, MSG_SET_MEASUREMENT_MODE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.mode = mode;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::getScalingMode(void *client, int32_t *mode) noexcept
{
    typedef msg_blob_get_scaling_mode blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(mode != nullptr);

    Message msg(0, MSG_GET_SCALING_MODE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.mode = mode;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::setScalingMode(void *client, int32_t mode) noexcept
{
    typedef msg_blob_set_scaling_mode blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(mode == OpenSLMediaPlayerVisualizer::SCALING_MODE_NORMALIZED ||
              mode == OpenSLMediaPlayerVisualizer::SCALING_MODE_AS_PLAYED);

    Message msg(0, MSG_SET_SCALING_MODE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.mode = mode;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::setDataCaptureListener(void *client,
                                                OpenSLMediaPlayerVisualizer::OnDataCaptureListener *listener,
                                                uint32_t rate, bool waveform, bool fft) noexcept
{
    typedef msg_blob_set_data_capture_listener blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    if ((listener != nullptr) && (waveform || fft)) {
        CHECK_ARG_RANGE(rate, MIN_CAPTURE_RATE, MAX_CAPTURE_RATE);
    } else {
        rate = 0;
        waveform = false;
        fft = false;
    }

    Message msg(0, MSG_SET_DATA_CAPTURE_LISTENER);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.listener = listener;
        blob.rate = rate;
        blob.waveform = waveform;
        blob.fft = fft;
    }

    return postAndWaitResult(&msg);
}

int VisualizerExtModule::setInternalPeriodicCaptureThreadEventListener(
    void *client, OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener *listener, uint32_t rate,
    bool waveform, bool fft) noexcept
{
    typedef msg_blob_set_internal_periodic_capture_thread_event_listener blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    if ((listener != nullptr) && (waveform || fft)) {
        CHECK_ARG_RANGE(rate, MIN_CAPTURE_RATE, MAX_CAPTURE_RATE);
    } else {
        rate = 0;
        waveform = false;
        fft = false;
    }

    Message msg(0, MSG_SET_INTERNAL_PERIODIC_CAPTURE_EVENT_LISTENER);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.listener = listener;
        blob.rate = rate;
        blob.waveform = waveform;
        blob.fft = fft;
    }

    return postAndWaitResult(&msg);
}

uint32_t VisualizerExtModule::getExtensionTraits() const noexcept
{
    uint32_t traits = OpenSLMediaPlayerExtension::TRAIT_NONE;

    if (status_.enabled) {
        traits |= OpenSLMediaPlayerExtension::TRAIT_CAPTURE_AUDIO_DATA;
    }

    return traits;
}

bool VisualizerExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                    void *user_arg) noexcept
{

    // set initial status
    VisualizerStatus &status = status_;

    status.sampling_rate = extmgr->extGetOutputSamplingRate();

    // set output latency
    const uint32_t latency_in_frames = extmgr->extGetOutputLatency();
    captured_data_buffer_.set_output_latency(latency_in_frames);

    return BaseExtensionModule::onInstall(extmgr, token, user_arg);
}

void VisualizerExtModule::onAttach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    // set get_waveform_fft_state
    status_.get_waveform_fft_state = PACK_WAVE_FFT_STATUS(status_.enabled, user_arg);

    // call super method
    BaseExtensionModule::onAttach(extmgr, user_arg);
}

void VisualizerExtModule::onDetach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{
    LOGD("Detached");

    bool capturing_stopped = false;

    {
        const ClientInfo *ci = static_cast<const ClientInfo *>(getActiveClient());

        // stop capturing thread
        if (ci == user_arg && status_.enabled) {
            SLresult result = stopPeriodicCapturing();

            capturing_stopped = true;

            if (result != SL_RESULT_SUCCESS) {
                LOGW("onDetach() stopPeriodicCapturing() returns an error: %d", result);
            }

            // LOGD("onDetach() captuing thread stopped");
        }
    }

    // call super method
    BaseExtensionModule::onDetach(extmgr, user_arg);

    {
        const ClientInfo *ci = static_cast<const ClientInfo *>(getActiveClient());

        // set get_waveform_fft_state
        if (user_arg == EXT_CLIENT(status_.get_waveform_fft_state)) {
            status_.get_waveform_fft_state = PACK_WAVE_FFT_STATUS(status_.enabled, ci);
        }

        // start periodic capturing
        if (capturing_stopped && status_.enabled && ci->preparedForCaptuirng()) {
            // LOGD("onDetach() start capturing thread");

            SLresult result = startPeriodicCapturing();

            if (result != SL_RESULT_SUCCESS) {
                LOGW("onDetach() startPeriodicCapturing() returns an error: %d", result);
            }
        }
    }
}

void VisualizerExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{
    // stop capturing thread
    stopPeriodicCapturing();

    // clear get_waveform_fft_state
    status_.get_waveform_fft_state = 0;

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void VisualizerExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                          const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    SLresult slResult = processMessage(extmgr, msg);

    int result = extmgr->extTranslateOpenSLErrorCode(slResult);

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, result);
    }
}

SLresult VisualizerExtModule::processMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                             const OpenSLMediaPlayerThreadMessage *msg) noexcept
{
    SLresult result = SL_RESULT_INTERNAL_ERROR;
    VisualizerStatus &status = status_;

    const bool is_enabled = status_.enabled;

    switch (msg->what) {
    case MSG_NOP: {
        LOCAL_ASSERT(false);
    } break;
    case MSG_SET_ENABLED: {
        typedef msg_blob_set_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            if (status_.enabled != blob.enabled) {
                const ClientInfo *ci = static_cast<const ClientInfo *>(getActiveClient());

                // With periodic capturing
                if (blob.enabled) {
                    if (ci->preparedForCaptuirng()) {
                        result = startPeriodicCapturing();
                    } else {
                        result = SL_RESULT_SUCCESS;
                    }
                } else {
                    result = stopPeriodicCapturing();
                }

                if (result == SL_RESULT_SUCCESS) {
                    status_.enabled = blob.enabled;

                    // set get_waveform_fft_state
                    status_.get_waveform_fft_state = PACK_WAVE_FFT_STATUS(status_.enabled, ci);

                    // notify traits changed
                    notifyTraitsUpdated();
                }
            } else {
                result = SL_RESULT_SUCCESS;
            }
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_ENABLED: {
        typedef msg_blob_get_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.enabled) = status_.enabled;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_GET_CAPTURE_SIZE: {
        typedef msg_blob_get_capture_size blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.size) = status_.capture_size;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_SET_CAPTURE_SIZE: {
        typedef msg_blob_set_capture_size blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            if (!is_enabled) {
                if (CHECK_RANGE(blob.size, MIN_CAPTURE_DATA_SIZE, MAX_CAPTURE_DATA_SIZE) && IS_POW_OF_TWO(blob.size)) {
                    status_.capture_size = blob.size;
                    result = SL_RESULT_SUCCESS;
                } else {
                    result = SL_RESULT_PARAMETER_INVALID;
                }
            } else {
                result = SL_RESULT_PRECONDITIONS_VIOLATED;
            }
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_MEASUREMENT_PEAK_RMS: {
        typedef msg_blob_get_measurement_peak_rms blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        const int32_t delayMs = calcDiffTimeSinceLastCaptureBufferUpdated();

        if (status.measurement_mode == OpenSLMediaPlayerVisualizer::MEASUREMENT_MODE_PEAK_RMS) {
            if (status_.enabled && (delayMs <= DISCARD_MEASUREMENTS_TIME_MS)) {
                measurement_.calcPeakRms();
            } else {
                measurement_.clear();
            }

            blob.measurement->mPeak = measurement_.mPeak;
            blob.measurement->mRms = measurement_.mRms;

            result = SL_RESULT_SUCCESS;
        } else {
            blob.measurement->mPeak = -9600;
            blob.measurement->mRms = -9600;

            result = SL_RESULT_PRECONDITIONS_VIOLATED;
        }
    } break;
    case MSG_GET_SAMPLING_RATE: {
        typedef msg_blob_get_sampling_rate blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.samplingRate) = status_.sampling_rate;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_GET_MEASUREMENT_MODE: {
        typedef msg_blob_get_measurement_mode blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.mode) = status_.measurement_mode;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_SET_MEASUREMENT_MODE: {
        typedef msg_blob_set_measurement_mode blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            if (status_.measurement_mode != blob.mode) {
                switch (status_.measurement_mode) {
                case OpenSLMediaPlayerVisualizer::MEASUREMENT_MODE_NONE:
                case OpenSLMediaPlayerVisualizer::MEASUREMENT_MODE_PEAK_RMS: {
                    const int32_t delayMs = calcDiffTimeSinceLastCaptureBufferUpdated();
                    if (delayMs > DISCARD_MEASUREMENTS_TIME_MS) {
                        measurement_.clear();
                    }
                } break;
                }
                status_.measurement_mode = blob.mode;
            }
            result = SL_RESULT_SUCCESS;
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_SCALING_MODE: {
        typedef msg_blob_get_scaling_mode blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.mode) = status_.scaling_mode.load(std::memory_order_acquire);
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_SET_SCALING_MODE: {
        typedef msg_blob_set_scaling_mode blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            status_.scaling_mode.store(blob.mode, std::memory_order_release);
            result = SL_RESULT_SUCCESS;
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_DATA_CAPTURE_LISTENER: {
        typedef msg_blob_set_data_capture_listener blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            if (!is_enabled) {
                ClientInfo *ci = static_cast<ClientInfo *>(getActiveClient());

                ci->periodic_capture_listener = blob.listener;
                ci->periodic_capture_rate = blob.rate;
                ci->periodic_capture_waveform = blob.waveform;
                ci->periodic_capture_fft = blob.fft;

                result = SL_RESULT_SUCCESS;
            } else {
                result = SL_RESULT_PRECONDITIONS_VIOLATED;
            }
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_INTERNAL_PERIODIC_CAPTURE_EVENT_LISTENER: {
        typedef msg_blob_set_internal_periodic_capture_thread_event_listener blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            if (!is_enabled) {
                ClientInfo *ci = static_cast<ClientInfo *>(getActiveClient());

                ci->internal_event_listener = blob.listener;
                ci->periodic_capture_rate = blob.rate;
                ci->periodic_capture_waveform = blob.waveform;
                ci->periodic_capture_fft = blob.fft;

                result = SL_RESULT_SUCCESS;
            } else {
                result = SL_RESULT_PRECONDITIONS_VIOLATED;
            }
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    default:
        LOGD("Unexpected message; what = %d", msg->what);
        break;
    }

    return result;
}

void VisualizerExtModule::onCaptureAudioData(OpenSLMediaPlayerExtensionManager *extmgr, const float *data,
                                             uint32_t num_channels, size_t num_frames, uint32_t sample_rate_millihertz,
                                             const timespec *timestamp) noexcept
{
    // NOTE:
    // This callback is called from the same thread with onHandleMessage() callback.

    // update sampling rate info
    status_.sampling_rate.store(sample_rate_millihertz, std::memory_order_release);

    if (CXXPH_UNLIKELY(num_channels != 2))
        return;

    if (!status_.enabled)
        return;

    if (CXXPH_UNLIKELY(!s16_data_buff_ || s16_data_buff_.size() < num_frames)) {
        try { s16_data_buff_.allocate(num_frames); }
        catch (std::bad_alloc &) { return; }
    }

    // convert to s16
    f32_to_s16_converter_.perform(reinterpret_cast<const cxxdasp::datatype::f32_stereo_frame_t *>(data),
                                  &s16_data_buff_[0], num_frames);

    // put captured data
    captured_data_buffer_.put_captured_data(num_channels, sample_rate_millihertz,
                                            reinterpret_cast<const int16_t *>(&s16_data_buff_[0]), num_frames,
                                            timestamp);

    if (CXXPH_UNLIKELY(num_frames == 0))
        return;

    // perform measurements
    if (status_.measurement_mode == OpenSLMediaPlayerVisualizer::MEASUREMENT_MODE_PEAK_RMS) {
        uint32_t sampling_rate = status_.sampling_rate.load(std::memory_order_relaxed);

        if (sampling_rate >= 1000) {
            int frameSizeInMs = (num_frames * 1000) / (sampling_rate / 1000);
            int maxBufferSizeInMs = MEASUREMENT_WINDOW_MAX_SIZE_IN_MILLISECONDS;

            frameSizeInMs = (std::max)(1, frameSizeInMs);

            measurement_.setMaxNumValidBuffers((maxBufferSizeInMs + frameSizeInMs - 1) / frameSizeInMs);
        }

        if (CXXPH_LIKELY(num_channels == 2)) {
            const uint32_t idx = measurement_.mBufferIndex;

            StockVisualizerAlgorithms::measurePeakRmsSquared(
                reinterpret_cast<const int16_t *>(&s16_data_buff_[0]), num_frames, num_channels,
                &(measurement_.mBuffer[idx].mPeakU16), &(measurement_.mBuffer[idx].mRmsSquared));

            measurement_.mBuffer[idx].mIsValid = true;
            measurement_.mBufferIndex = (idx + 1) % MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS;
        }
    }
}

SLresult VisualizerExtModule::startPeriodicCapturing() noexcept
{
    int pt_create_result;
    pthread_t pt_handle = 0;

    periodic_capture_thread_stop_req_ = false;
    periodic_capture_thread_initialized_ = false;
    // captured_data_buffer_.reset();

    pt_create_result = ::pthread_create(&pt_handle, nullptr, periodicCaptureThreadEntryFunc, this);

    if (pt_create_result != 0)
        return SL_RESULT_RESOURCE_ERROR;

    // wait for thread started & initialized
    bool error = false;
    {
        utils::pt_unique_lock lock(mutex_cond_periodic_capture_thread_);
        timespec timeout;

        if (utils::timespec_utils::get_current_time(timeout)) {
            timeout = utils::timespec_utils::add_ms(timeout, 3000);

            while (!periodic_capture_thread_initialized_) {
                int wait_result = cond_periodic_capture_thread_.wait_absolute(lock, timeout);

                if (wait_result == ETIMEDOUT) {
                    // timeout
                    error = true;
                    break;
                } else {
                    // retry
                }
            }
        }
    }

    if (error) {
        // stop the thread
        periodic_capture_thread_stop_req_ = true;

        void *pt_result = 0;
        (void)::pthread_join(pt_handle, &pt_result);

        return SL_RESULT_INTERNAL_ERROR;
    }

    periodic_capture_thread_.set(pt_handle);

    return SL_RESULT_SUCCESS;
}

SLresult VisualizerExtModule::stopPeriodicCapturing() noexcept
{
    if (!periodic_capture_thread_.available())
        return SL_RESULT_SUCCESS;

    // set periodic_capture_thread_stop_req_ flag
    {
        utils::pt_unique_lock lock(mutex_cond_periodic_capture_thread_);
        periodic_capture_thread_stop_req_ = true;
        cond_periodic_capture_thread_.notify_one();
    }

    void *tmp_retval = 0;
    int pt_join_result = ::pthread_join(periodic_capture_thread_.get(), &tmp_retval);

    if (pt_join_result != 0)
        return SL_RESULT_INTERNAL_ERROR;

    // captured_data_buffer_.reset();
    periodic_capture_thread_.clear();
    periodic_capture_thread_initialized_ = false;

    return SL_RESULT_SUCCESS;
}

void VisualizerExtModule::periodicCaptureThread(periodic_capture_thread_context_t &c) noexcept
{
    uint32_t interval_us = 1000000000L / c.capture_rate;

    timespec wakeup_time;

    if (!utils::timespec_utils::get_current_time(wakeup_time))
        return;

    while (!periodic_capture_thread_stop_req_) {
        // update scaling mode
        c.scaling_mode = status_.scaling_mode.load(std::memory_order_acquire);

        // process periodic capturing
        processPeriodicCapture(c);

        // determine next wakeup time
        wakeup_time = utils::timespec_utils::add_us(wakeup_time, interval_us);

        timespec cur_time;
        if (!utils::timespec_utils::get_current_time(cur_time))
            break;

        int64_t idle_time = utils::timespec_utils::sub_ret_us(wakeup_time, cur_time);

        if (idle_time >= 0 && idle_time <= interval_us) {
            // LOGV("idle_time = %d", static_cast<int>(idle_time));
        } else {
            // correct wakeup time if the idle_time is a unexpected range value

            // LOGV("idle_time = %d (!)", static_cast<int>(idle_time));

            wakeup_time = utils::timespec_utils::add_us(cur_time, interval_us);
            idle_time = interval_us;
        }

#if 1
        {
            utils::pt_unique_lock lock(mutex_cond_periodic_capture_thread_);

            while (!periodic_capture_thread_stop_req_.load(std::memory_order_relaxed)) {
                int wait_result = cond_periodic_capture_thread_.wait_absolute(lock, wakeup_time);

                if (wait_result == ETIMEDOUT) {
                    // timeout
                    break;
                } else {
                    // retry
                }
            }
        }
#else
        ::usleep(static_cast<useconds_t>(idle_time));
#endif
    }
}

void VisualizerExtModule::processPeriodicCapture(periodic_capture_thread_context_t &c) noexcept
{
    // get captured data
    uint32_t num_channels = 0;
    uint32_t sampling_rate = 0;
    const int16_t *data = nullptr;

    if (captured_data_buffer_.get_captured_data(c.capture_size, c.capture_rate, &num_channels, &sampling_rate, &data)) {
        c.sampling_rate = sampling_rate;
        c.capture_start_delay = 0;
    } else {
        // captured data is not present
        data = nullptr;

        // delay capture callback until capture_start_delay counter remains
        if (c.capture_start_delay > 0) {
            --(c.capture_start_delay);
            return;
        }
    }

    if (!c.sampling_rate) {
        return;
    }

    uint8_t *CXXPH_RESTRICT waveform = nullptr;
    int8_t *CXXPH_RESTRICT fft = nullptr;

    // Lock buffers (if using internal_event_listener)
    if (c.capture_waveform || c.capture_fft) {
        if (c.internal_event_listener.get()) {
            waveform = static_cast<uint8_t *>(
                c.internal_event_listener->onLockCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_WAVEFORM));
        } else {
            waveform = c.waveform_buff.get();
        }
    }

    if (c.capture_fft) {
        if (c.internal_event_listener.get()) {
            fft = static_cast<int8_t *>(
                c.internal_event_listener->onLockCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_FFT));
        } else {
            fft = c.fft_buff.get();
        }
    }

    // S16 stereo -> U8 monaural
    if ((c.capture_waveform || c.capture_fft) && waveform) {
        if (data) {
            StockVisualizerAlgorithms::convertWaveformS16StereoToU8Mono(waveform, data, c.capture_size, c.scaling_mode);
        } else {
            ::memset(waveform, 128, c.capture_size);
        }
    }

    // FFT
    if (c.capture_fft && waveform && fft) {
        if (data) {
            StockVisualizerAlgorithms::doFft(fft, waveform, c.capture_size);
        } else {
            ::memset(fft, 0, c.capture_size);
        }
    }

    // Unlock buffers (if using internal_event_listener)
    if (c.internal_event_listener.get()) {
        if (waveform) {
            c.internal_event_listener->onUnlockCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_WAVEFORM, waveform);
            waveform = nullptr;
        }
        if (fft) {
            c.internal_event_listener->onUnlockCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_FFT, fft);
            fft = nullptr;
        }
    }

    // raise callbacks
    if (c.capture_waveform && c.internal_event_listener.get()) {
        c.internal_event_listener->onWaveFormDataCapture(c.visualizer, waveform, c.capture_size, c.sampling_rate);
    }
    if (c.capture_waveform && c.listener.get()) {
        c.listener->onWaveFormDataCapture(c.visualizer, waveform, c.capture_size, c.sampling_rate);
    }

    if (c.capture_fft && c.internal_event_listener.get()) {
        c.internal_event_listener->onFftDataCapture(c.visualizer, fft, c.capture_size, c.sampling_rate);
    }
    if (c.capture_fft && c.listener.get()) {
        c.listener->onFftDataCapture(c.visualizer, fft, c.capture_size, c.sampling_rate);
    }
}

void *VisualizerExtModule::periodicCaptureThreadEntryFunc(void *args) noexcept
{
    VisualizerExtModule *thiz = reinterpret_cast<VisualizerExtModule *>(args);
    periodic_capture_thread_context_t c;

    LOGD("periodicCaptureThreadEntryFunc");

    // intialize
    const ClientInfo *ci = static_cast<const ClientInfo *>(thiz->getActiveClient());

    c.visualizer = static_cast<OpenSLMediaPlayerVisualizer *>(ci->client);
    c.capture_size = thiz->status_.capture_size;
    c.scaling_mode = thiz->status_.scaling_mode;
    c.capture_rate = ci->periodic_capture_rate;
    c.capture_waveform = ci->periodic_capture_waveform;
    c.capture_fft = ci->periodic_capture_fft;
    c.listener = ci->periodic_capture_listener;
    c.internal_event_listener = ci->internal_event_listener;
    c.sampling_rate = thiz->status_.sampling_rate.load(std::memory_order_acquire);
    c.waveform_buff = nullptr;
    c.fft_buff = nullptr;
    c.capture_start_delay = 5;

    // set thread name
    AndroidHelper::setCurrentThreadName("OSLMPVisualizer");

    // set thread priority
    {
        android::sp<OpenSLMediaPlayerExtensionManager> ext_mgr;

        thiz->getExtensionManager(ext_mgr);
        AndroidHelper::setThreadPriority(ext_mgr->extGetJavaVM(), 0, ANDROID_THREAD_PRIORITY_LESS_FAVORABLE);
    }

    // notify initialized
    {
        utils::pt_unique_lock lock(thiz->mutex_cond_periodic_capture_thread_);

        thiz->periodic_capture_thread_initialized_ = true;
        (void)thiz->cond_periodic_capture_thread_.notify_one();
    }

    if (!(c.visualizer && (c.listener.get() || c.internal_event_listener.get())))
        return 0;

    // raise onEnterInternalPeriodicCaptureThread() event
    if (c.internal_event_listener.get()) {
        c.internal_event_listener->onEnterInternalPeriodicCaptureThread(c.visualizer);
    }

    // allocate buffers

    if (c.capture_waveform || c.capture_fft) {
        if (c.internal_event_listener.get()) {
            c.internal_event_listener->onAllocateCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_WAVEFORM,
                                                               c.capture_size);
        } else {
            c.waveform_buff.reset(new (std::nothrow) uint8_t[c.capture_size]);
        }
    }

    if (c.capture_fft) {
        if (c.internal_event_listener.get()) {
            c.internal_event_listener->onAllocateCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_FFT, c.capture_size);
        } else {
            c.fft_buff.reset(new (std::nothrow) int8_t[c.capture_size]);
        }
    }

    thiz->periodicCaptureThread(c);

    // release buffers

    if (c.capture_waveform || c.capture_fft) {
        if (c.internal_event_listener.get()) {
            c.internal_event_listener->onDeAllocateCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_WAVEFORM);
        } else {
            c.waveform_buff.reset();
        }
    }

    if (c.capture_fft) {
        if (c.internal_event_listener.get()) {
            c.internal_event_listener->onDeAllocateCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_FFT);
        } else {
            c.fft_buff.reset();
        }
    }

    // raise onLeaveInternalPeriodicCaptureThread() event
    if (c.internal_event_listener.get()) {
        c.internal_event_listener->onLeaveInternalPeriodicCaptureThread(c.visualizer);
    }

    return 0;
}

int32_t VisualizerExtModule::calcDiffTimeSinceLastCaptureBufferUpdated() const noexcept
{
    timespec ts = utils::timespec_utils::ZERO();

    captured_data_buffer_.get_last_updated_time(&ts);

    return calcDiffTimeSinceLastCaptureBufferUpdated(ts);
}

int32_t VisualizerExtModule::calcDiffTimeSinceLastCaptureBufferUpdated(const timespec &last_updated) const noexcept
{
    int32_t diff = 1000000;
    timespec now;

    if (utils::timespec_utils::get_current_time(now)) {
        diff = utils::timespec_utils::sub_ret_ms(now, last_updated);
    }

    diff = (std::max)(0, diff);

    return diff;
}

} // namespace oslmp
