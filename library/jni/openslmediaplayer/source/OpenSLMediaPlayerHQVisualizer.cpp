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

// #define LOG_TAG "OpenSLMediaPlayerHQVisualizer"

#include "oslmp/OpenSLMediaPlayerHQVisualizer.hpp"

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
#include <cxxdasp/fft/fft.hpp>
#include <cxxdasp/utils/utils.hpp>
#include <cxxdasp/window/window_functions.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/OpenSLMediaPlayerThreadMessage.hpp"
#include "oslmp/impl/HQVisualizerCapturedAudioDataBuffer.hpp"
#include "oslmp/impl/BaseExtensionModule.hpp"
#include "oslmp/impl/StockVisualizerAlgorithms.hpp"
#include "oslmp/impl/AndroidHelper.hpp"
#include "oslmp/utils/pthread_utils.hpp"
#include "oslmp/utils/timespec_utils.hpp"
#include "oslmp/utils/optional.hpp"

//
// Constants
//
#define MODULE_NAME "HQVisualizer"

// for debugging
#define MIN_CAPTURE_DATA_SIZE 128      // [frames]
#define MAX_CAPTURE_DATA_SIZE 32768    // [frames]
#define DEFAULT_CAPTURE_DATA_SIZE 1024 // [frames]

#define MIN_CAPTURE_RATE 100       // [milli hertz]
#define MAX_CAPTURE_RATE 60000     // [milli hertz]
#define DEFAULT_CAPTURE_RATE 20000 // [milli hertz]

#define DEFAULT_WINDOW_TYPE OpenSLMediaPlayerHQVisualizer::WINDOW_RECTANGULAR

#define CAPTURE_BUFFER_SIZE_IN_FRAMES 65536 // [frames] (= 1.37 sec. @ 48000 Hz)

#define NUM_CHANNELS 2

#define CAPTURE_BUFFER_TYPE_WAVEFORM                                                                                   \
    OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener::BUFFER_TYPE_WAVEFORM
#define CAPTURE_BUFFER_TYPE_FFT                                                                                        \
    OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener::BUFFER_TYPE_FFT

//
// helper macros
//

#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    HQVisualizerExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                             \
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

#define WINDOW_KIND_MASK (~(OpenSLMediaPlayerHQVisualizer::WINDOW_OPT_APPLY_FOR_WAVEFORM))

namespace oslmp {

using namespace ::opensles;
using namespace ::oslmp::impl;

typedef OpenSLMediaPlayerInternalContext InternalContext;
typedef android::sp<OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener> sp_on_data_capture_listener_t;
typedef android::sp<OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener>
sp_internal_periodic_capture_thread_event_listener_t;

#if CXXDASP_USE_FFT_BACKEND_NE10
typedef cxxdasp::fft::backend::f::ne10 fft_backend_type;
#elif CXXDASP_USE_FFT_BACKEND_PFFFT
typedef cxxdasp::fft::backend::f::pffft fft_backend_type;
#else
#error No FFT backend available
#endif

struct HQVisualizerStatus {
    bool enabled;
    std::atomic<uint32_t> capture_size;  // [frames]
    std::atomic<uint32_t> sampling_rate; // [milli hertz]
    std::atomic<uint32_t> window_type;

    std::atomic<uintptr_t> get_waveform_fft_state; // bit0: enabled

    HQVisualizerStatus()
        : enabled(false), capture_size(DEFAULT_CAPTURE_DATA_SIZE), sampling_rate(0), window_type(DEFAULT_WINDOW_TYPE),
          get_waveform_fft_state()
    {
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
    OpenSLMediaPlayerHQVisualizer *visualizer;

    uint32_t capture_rate;
    uint32_t capture_size;
    uint32_t num_channels;
    uint32_t window_type;
    bool capture_waveform;
    bool capture_fft;
    sp_on_data_capture_listener_t listener;
    sp_internal_periodic_capture_thread_event_listener_t internal_event_listener;
    cxxporthelper::aligned_memory<float> waveform_buff;
    cxxporthelper::aligned_memory<float> waveform_buff2;
    cxxporthelper::aligned_memory<std::complex<float>> fft_buff;
    cxxporthelper::aligned_memory<float> window_table;

    uint32_t sampling_rate;
    uint32_t capture_start_delay;

    cxxdasp::fft::fft<float, std::complex<float>, fft_backend_type::forward_real> fftr_rch;
    cxxdasp::fft::fft<float, std::complex<float>, fft_backend_type::forward_real> fftr_lch;
};

class HQVisualizerExtModule : public BaseExtensionModule {
public:
    HQVisualizerExtModule();
    virtual ~HQVisualizerExtModule();

    int setEnabled(void *client, bool enabled) noexcept;
    int getEnabled(void *client, bool *enabled) noexcept;
    int getCaptureSize(void *client, size_t *size) noexcept;
    int setCaptureSize(void *client, size_t size) noexcept;
    int getSamplingRate(void *client, uint32_t *samplingRate) noexcept;
    int getNumChannels(void *client, uint32_t *numChannels) noexcept;
    int setWindowFunction(void *client, uint32_t windowType) noexcept;
    int getWindowFunction(void *client, uint32_t *windowType) noexcept;
    int setDataCaptureListener(void *client, OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener *listener,
                               uint32_t rate, bool waveform, bool fft) noexcept;
    int setInternalPeriodicCaptureThreadEventListener(
        void *client, OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener *listener,
        uint32_t rate, bool waveform, bool fft) noexcept;
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
    void checkAndUpdateWindowTable(periodic_capture_thread_context_t &c) noexcept;

    static void *periodicCaptureThreadEntryFunc(void *args) noexcept;

    int32_t calcDiffTimeSinceLastCaptureBufferUpdated() const noexcept;
    int32_t calcDiffTimeSinceLastCaptureBufferUpdated(const timespec &last_updated) const noexcept;

private:
    HQVisualizerStatus status_;

    HQVisualizerCapturedAudioDataBuffer captured_data_buffer_;
    utils::optional<pthread_t> periodic_capture_thread_;
    utils::pt_mutex mutex_cond_periodic_capture_thread_;
    utils::pt_condition_variable cond_periodic_capture_thread_;
    bool periodic_capture_thread_initialized_;
    std::atomic<bool> periodic_capture_thread_stop_req_;
};

class HQHQVisualizerExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    HQHQVisualizerExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) HQVisualizerExtModule();
    }
};

class OpenSLMediaPlayerHQVisualizer::Impl {
public:
    Impl(void *client, const android::sp<OpenSLMediaPlayerContext> &context);
    ~Impl();

    HQVisualizerExtModule *module_;
    ClientInfo client_info_;
};

enum {
    MSG_NOP,
    MSG_SET_ENABLED,
    MSG_GET_ENABLED,
    MSG_GET_CAPTURE_SIZE,
    MSG_SET_CAPTURE_SIZE,
    MSG_GET_SAMPLING_RATE,
    MSG_GET_NUM_CHANNELS,
    MSG_SET_WINDOW_FUNCTION,
    MSG_GET_WINDOW_FUNCTION,
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

struct msg_blob_get_sampling_rate {
    void *client;
    uint32_t *samplingRate;
};

struct msg_blob_get_num_channels {
    void *client;
    uint32_t *numChannels;
};

struct msg_blob_set_window_function {
    void *client;
    uint32_t windowType;
};

struct msg_blob_get_window_function {
    void *client;
    uint32_t *windowType;
};

struct msg_blob_set_data_capture_listener {
    void *client;
    OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener *listener;
    uint32_t rate;
    bool waveform;
    bool fft;
};

struct msg_blob_set_internal_periodic_capture_thread_event_listener {
    void *client;
    OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener *listener;
    uint32_t rate;
    bool waveform;
    bool fft;
};

//
// Utilities
//

static void generate_window_table(float *dest, size_t n, uint32_t type)
{
    using namespace cxxdasp::window;

    switch (type) {
    case OpenSLMediaPlayerHQVisualizer::WINDOW_RECTANGULAR:
        // generate_rectangular_window(dest, n);    // Skip this
        break;
    case OpenSLMediaPlayerHQVisualizer::WINDOW_HANN:
        generate_hann_window(dest, n);
        break;
    case OpenSLMediaPlayerHQVisualizer::WINDOW_HAMMING:
        generate_hamming_window(dest, n);
        break;
    case OpenSLMediaPlayerHQVisualizer::WINDOW_BLACKMAN:
        generate_blackman_window(dest, n);
        break;
    case OpenSLMediaPlayerHQVisualizer::WINDOW_FLAT_TOP:
        generate_flat_top_window(dest, n);
        break;
    default:
        assert(false);
        break;
    }
}

//
// OpenSLMediaPlayerHQVisualizer
//
OpenSLMediaPlayerHQVisualizer::OpenSLMediaPlayerHQVisualizer(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(this, context))
{
}

OpenSLMediaPlayerHQVisualizer::~OpenSLMediaPlayerHQVisualizer()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerHQVisualizer::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(CLIENT_INFO(this), enabled);
}

int OpenSLMediaPlayerHQVisualizer::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(CLIENT_INFO(this), enabled);
}

int OpenSLMediaPlayerHQVisualizer::getMaxCaptureRate(uint32_t *rate) const noexcept
{
    GET_MODULE_INSTANCE(module);
    return sGetMaxCaptureRate(rate);
}

int OpenSLMediaPlayerHQVisualizer::getCaptureSizeRange(size_t *range) const noexcept
{
    GET_MODULE_INSTANCE(module);
    return sGetCaptureSizeRange(range);
}

int OpenSLMediaPlayerHQVisualizer::getCaptureSize(size_t *size) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getCaptureSize(CLIENT_INFO(this), size);
}

int OpenSLMediaPlayerHQVisualizer::setCaptureSize(size_t size) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setCaptureSize(CLIENT_INFO(this), size);
}

int OpenSLMediaPlayerHQVisualizer::getSamplingRate(uint32_t *samplingRate) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getSamplingRate(CLIENT_INFO(this), samplingRate);
}

int OpenSLMediaPlayerHQVisualizer::getNumChannels(uint32_t *numChannels) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getNumChannels(CLIENT_INFO(this), numChannels);
}

int OpenSLMediaPlayerHQVisualizer::setWindowFunction(uint32_t windowType) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setWindowFunction(CLIENT_INFO(this), windowType);
}

int OpenSLMediaPlayerHQVisualizer::getWindowFunction(uint32_t *windowType) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getWindowFunction(CLIENT_INFO(this), windowType);
}

int
OpenSLMediaPlayerHQVisualizer::setDataCaptureListener(OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener *listener,
                                                      uint32_t rate, bool waveform, bool fft) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setDataCaptureListener(CLIENT_INFO(this), listener, rate, waveform, fft);
}

int OpenSLMediaPlayerHQVisualizer::setInternalPeriodicCaptureThreadEventListener(
    OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener *listener, uint32_t rate, bool waveform,
    bool fft) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setInternalPeriodicCaptureThreadEventListener(CLIENT_INFO(this), listener, rate, waveform, fft);
}

int OpenSLMediaPlayerHQVisualizer::sGetMaxCaptureRate(uint32_t *rate) noexcept
{
    CHECK_ARG(rate != nullptr);

    rate[0] = MAX_CAPTURE_RATE;

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayerHQVisualizer::sGetCaptureSizeRange(size_t *range) noexcept
{
    CHECK_ARG(range != nullptr);

    range[0] = MIN_CAPTURE_DATA_SIZE;
    range[1] = MAX_CAPTURE_DATA_SIZE;

    return OSLMP_RESULT_SUCCESS;
}

//
// OpenSLMediaPlayerHQVisualizer::Impl
//
OpenSLMediaPlayerHQVisualizer::Impl::Impl(void *client, const android::sp<OpenSLMediaPlayerContext> &context)
    : client_info_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_HQ_VISUALIZER) {
        const HQHQVisualizerExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module = nullptr;

        int result = c.extAttachOrInstall(&module, &creator, &client_info_);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<HQVisualizerExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerHQVisualizer::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(&client_info_);

        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// HQVisualizerExtModule
//
HQVisualizerExtModule::HQVisualizerExtModule()
    : BaseExtensionModule(MODULE_NAME), status_(),
      captured_data_buffer_(CAPTURE_BUFFER_SIZE_IN_FRAMES, MAX_CAPTURE_DATA_SIZE), periodic_capture_thread_(),
      mutex_cond_periodic_capture_thread_(), cond_periodic_capture_thread_(),
      periodic_capture_thread_initialized_(false)
{
}

HQVisualizerExtModule::~HQVisualizerExtModule() {}

int HQVisualizerExtModule::setEnabled(void *client, bool enabled) noexcept
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

int HQVisualizerExtModule::getEnabled(void *client, bool *enabled) noexcept
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

int HQVisualizerExtModule::getCaptureSize(void *client, size_t *size) noexcept
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

int HQVisualizerExtModule::setCaptureSize(void *client, size_t size) noexcept
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

int HQVisualizerExtModule::getSamplingRate(void *client, uint32_t *samplingRate) noexcept
{
    typedef msg_blob_get_sampling_rate blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(samplingRate != nullptr);

    (*samplingRate) = 0;

    Message msg(0, MSG_GET_SAMPLING_RATE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.samplingRate = samplingRate;
    }

    return postAndWaitResult(&msg);
}

int HQVisualizerExtModule::getNumChannels(void *client, uint32_t *numChannels) noexcept
{
    typedef msg_blob_get_num_channels blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(numChannels != nullptr);

    (*numChannels) = 0;

    Message msg(0, MSG_GET_NUM_CHANNELS);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.numChannels = numChannels;
    }

    return postAndWaitResult(&msg);
}

int HQVisualizerExtModule::setDataCaptureListener(void *client,
                                                  OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener *listener,
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

int HQVisualizerExtModule::setWindowFunction(void *client, uint32_t windowType) noexcept
{
    typedef msg_blob_set_window_function blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG_RANGE((windowType & ~OpenSLMediaPlayerHQVisualizer::WINDOW_OPT_APPLY_FOR_WAVEFORM),
                    OpenSLMediaPlayerHQVisualizer::WINDOW_RECTANGULAR, OpenSLMediaPlayerHQVisualizer::WINDOW_FLAT_TOP);

    Message msg(0, MSG_SET_WINDOW_FUNCTION);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.windowType = windowType;
    }

    return postAndWaitResult(&msg);
}

int HQVisualizerExtModule::getWindowFunction(void *client, uint32_t *windowType) noexcept
{
    typedef msg_blob_get_window_function blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(windowType != nullptr);

    (*windowType) = OpenSLMediaPlayerHQVisualizer::WINDOW_RECTANGULAR;

    Message msg(0, MSG_GET_WINDOW_FUNCTION);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.windowType = windowType;
    }

    return postAndWaitResult(&msg);
}

int HQVisualizerExtModule::setInternalPeriodicCaptureThreadEventListener(
    void *client, OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener *listener, uint32_t rate,
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

uint32_t HQVisualizerExtModule::getExtensionTraits() const noexcept
{
    uint32_t traits = OpenSLMediaPlayerExtension::TRAIT_NONE;

    if (status_.enabled) {
        traits |= OpenSLMediaPlayerExtension::TRAIT_CAPTURE_AUDIO_DATA;
    }

    return traits;
}

bool HQVisualizerExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                      void *user_arg) noexcept
{

    // set initial status
    HQVisualizerStatus &status = status_;

    status.sampling_rate = extmgr->extGetOutputSamplingRate();

    // set output latency
    const uint32_t latency_in_frames = extmgr->extGetOutputLatency();
    captured_data_buffer_.set_output_latency(latency_in_frames);

    return BaseExtensionModule::onInstall(extmgr, token, user_arg);
}

void HQVisualizerExtModule::onAttach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    // set get_waveform_fft_state
    status_.get_waveform_fft_state = PACK_WAVE_FFT_STATUS(status_.enabled, user_arg);

    // call super method
    BaseExtensionModule::onAttach(extmgr, user_arg);
}

void HQVisualizerExtModule::onDetach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
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

void HQVisualizerExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{
    // stop capturing thread
    stopPeriodicCapturing();

    // clear get_waveform_fft_state
    status_.get_waveform_fft_state = 0;

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void HQVisualizerExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                            const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    SLresult slResult = processMessage(extmgr, msg);

    int result = extmgr->extTranslateOpenSLErrorCode(slResult);

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, result);
    }
}

SLresult HQVisualizerExtModule::processMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                               const OpenSLMediaPlayerThreadMessage *msg) noexcept
{
    SLresult result = SL_RESULT_INTERNAL_ERROR;
    HQVisualizerStatus &status = status_;

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
    case MSG_GET_SAMPLING_RATE: {
        typedef msg_blob_get_sampling_rate blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.samplingRate) = status_.sampling_rate;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_GET_NUM_CHANNELS: {
        typedef msg_blob_get_num_channels blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.numChannels) = NUM_CHANNELS;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_SET_WINDOW_FUNCTION: {
        typedef msg_blob_set_window_function blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            status_.window_type = blob.windowType;
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_GET_WINDOW_FUNCTION: {
        typedef msg_blob_get_window_function blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.windowType) = status_.window_type;
        result = SL_RESULT_SUCCESS;
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

void HQVisualizerExtModule::onCaptureAudioData(OpenSLMediaPlayerExtensionManager *extmgr, const float *data,
                                               uint32_t num_channels, size_t num_frames,
                                               uint32_t sample_rate_millihertz, const timespec *timestamp) noexcept
{
    // NOTE:
    // This callback is called from the same thread with onHandleMessage() callback.

    // update sampling rate info
    status_.sampling_rate.store(sample_rate_millihertz, std::memory_order_release);

    if (CXXPH_UNLIKELY(num_channels != 2))
        return;

    if (!status_.enabled)
        return;

    // put captured data
    captured_data_buffer_.put_captured_data(num_channels, sample_rate_millihertz, data, num_frames, timestamp);
}

SLresult HQVisualizerExtModule::startPeriodicCapturing() noexcept
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

SLresult HQVisualizerExtModule::stopPeriodicCapturing() noexcept
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

void HQVisualizerExtModule::periodicCaptureThread(periodic_capture_thread_context_t &c) noexcept
{
    uint32_t interval_us = 1000000000L / c.capture_rate;

    timespec wakeup_time;

    if (!utils::timespec_utils::get_current_time(wakeup_time))
        return;

    while (!periodic_capture_thread_stop_req_) {
        // update window type
        checkAndUpdateWindowTable(c);

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

void HQVisualizerExtModule::processPeriodicCapture(periodic_capture_thread_context_t &c) noexcept
{
    // get captured data
    uint32_t num_channels = 0;
    uint32_t sampling_rate = 0;
    const float *data = nullptr;

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

    float *CXXPH_RESTRICT work_waveform = nullptr;          // aligned on CXXPH_PLATFORM_SIMD_ALIGNMENT boundary
    std::complex<float> *CXXPH_RESTRICT work_fft = nullptr; // aligned on CXXPH_PLATFORM_SIMD_ALIGNMENT boundary
    float *CXXPH_RESTRICT dest_waveform = nullptr;
    std::complex<float> *CXXPH_RESTRICT dest_fft = nullptr;
    const size_t waveform_buff_size_in_bytes = (sizeof(float) * c.capture_size * c.num_channels);
    const size_t fft_buff_size_in_bytes = (sizeof(float) * c.capture_size * c.num_channels);
    const bool apply_window_for_waveform =
        ((c.window_type & OpenSLMediaPlayerHQVisualizer::WINDOW_OPT_APPLY_FOR_WAVEFORM) != 0);

    // Lock buffers (if using internal_event_listener)
    if (c.capture_waveform || c.capture_fft) {
        if (c.internal_event_listener.get()) {
            dest_waveform = static_cast<float *>(
                c.internal_event_listener->onLockCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_WAVEFORM));
        } else {
            dest_waveform = &(c.waveform_buff2[0]);
        }

        if (dest_waveform && !(c.capture_fft) && !(apply_window_for_waveform) &&
            cxxdasp::utils::is_aligned(dest_waveform, CXXPH_PLATFORM_SIMD_ALIGNMENT)) {
            work_waveform = dest_waveform;
        } else {
            work_waveform = &(c.waveform_buff[0]);
        }
    }

    if (c.capture_fft) {
        if (c.internal_event_listener.get()) {
            dest_fft = static_cast<std::complex<float> *>(
                c.internal_event_listener->onLockCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_FFT));
        } else {
            dest_fft = &(c.fft_buff[0]);
        }

        work_fft = &(c.fft_buff[0]);
    }

    // Copy to working memory (de-interleave)
    if ((c.capture_waveform || c.capture_fft) && work_waveform) {
        if (data) {
            cxxdasp::utils::deinterleave(&(work_waveform[0]), &(work_waveform[c.capture_size]), data, c.capture_size);
        } else {
            ::memset(work_waveform, 0, waveform_buff_size_in_bytes);
        }
    }

    // Copy to destination buffer (without windowing)
    if (!apply_window_for_waveform) {
        if (work_waveform && dest_waveform && (work_waveform != dest_waveform)) {
            ::memcpy(dest_waveform, work_waveform, waveform_buff_size_in_bytes);
        }
    }

    // apply window
    if (work_waveform && c.window_table && data &&
        ((c.window_type & WINDOW_KIND_MASK) != OpenSLMediaPlayerHQVisualizer::WINDOW_RECTANGULAR)) {
        cxxdasp::utils::multiply_aligned(&work_waveform[0], &c.window_table[0], c.capture_size);
        cxxdasp::utils::multiply_aligned(&work_waveform[c.capture_size], &c.window_table[0], c.capture_size);
    }

    // FFT
    if (c.capture_fft && work_waveform && work_fft) {
        if (data) {
            const int n = (c.capture_size / 2);

            // Lch
            c.fftr_lch.execute();

            // fft[0].real = fft[N].imag
            cxxporthelper::complex::set_imag(work_fft[0], work_fft[0 + n].real());

            // Rch
            c.fftr_rch.execute();

            // fft[0].real = fft[N].imag
            cxxporthelper::complex::set_imag(work_fft[n], work_fft[n + n].real());

        } else {
            ::memset(work_fft, 0, fft_buff_size_in_bytes);
        }
    }

    // Copy to destination buffer
    if (apply_window_for_waveform) {
        if (work_waveform && dest_waveform && (work_waveform != dest_waveform)) {
            ::memcpy(dest_waveform, work_waveform, waveform_buff_size_in_bytes);
        }
    }
    if (work_fft && dest_fft && (work_fft != dest_fft)) {
        ::memcpy(dest_fft, work_fft, fft_buff_size_in_bytes);
    }

    // Unlock buffers (if using internal_event_listener)
    if (c.internal_event_listener.get()) {
        if (dest_waveform) {
            c.internal_event_listener->onUnlockCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_WAVEFORM, dest_waveform);
            dest_waveform = nullptr;
        }
        if (dest_fft) {
            c.internal_event_listener->onUnlockCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_FFT, dest_fft);
            dest_fft = nullptr;
        }
    }

    // raise callbacks
    if (c.capture_waveform && c.internal_event_listener.get()) {
        c.internal_event_listener->onWaveFormDataCapture(c.visualizer, dest_waveform, c.num_channels, c.capture_size,
                                                         c.sampling_rate);
    }
    if (c.capture_waveform && c.listener.get()) {
        c.listener->onWaveFormDataCapture(c.visualizer, dest_waveform, c.num_channels, c.capture_size, c.sampling_rate);
    }

    if (c.capture_fft && c.internal_event_listener.get()) {
        c.internal_event_listener->onFftDataCapture(c.visualizer, reinterpret_cast<const float *>(dest_fft),
                                                    c.num_channels, c.capture_size, c.sampling_rate);
    }
    if (c.capture_fft && c.listener.get()) {
        c.listener->onFftDataCapture(c.visualizer, reinterpret_cast<const float *>(dest_fft), c.num_channels,
                                     c.capture_size, c.sampling_rate);
    }
}

void HQVisualizerExtModule::checkAndUpdateWindowTable(periodic_capture_thread_context_t &c) noexcept
{
    const uint32_t window_type = status_.window_type.load(std::memory_order_acquire);

    if ((window_type ^ c.window_type) & WINDOW_KIND_MASK) {
        generate_window_table(&c.window_table[0], c.capture_size, (window_type & WINDOW_KIND_MASK));
    }

    c.window_type = window_type;
}

void *HQVisualizerExtModule::periodicCaptureThreadEntryFunc(void *args) noexcept
{
    HQVisualizerExtModule *thiz = reinterpret_cast<HQVisualizerExtModule *>(args);
    periodic_capture_thread_context_t c;

    LOGD("periodicCaptureThreadEntryFunc");

    // intialize
    const ClientInfo *ci = static_cast<const ClientInfo *>(thiz->getActiveClient());

    c.visualizer = static_cast<OpenSLMediaPlayerHQVisualizer *>(ci->client);
    c.capture_size = thiz->status_.capture_size;
    c.num_channels = 2;
    c.capture_rate = ci->periodic_capture_rate;
    c.capture_waveform = ci->periodic_capture_waveform;
    c.capture_fft = ci->periodic_capture_fft;
    c.listener = ci->periodic_capture_listener;
    c.internal_event_listener = ci->internal_event_listener;
    c.sampling_rate = thiz->status_.sampling_rate.load(std::memory_order_acquire);
    c.window_type = thiz->status_.window_type;
    c.capture_start_delay = 5;

    {
        c.window_table.allocate(c.capture_size, CXXPH_PLATFORM_SIMD_ALIGNMENT, false);

        if (!c.window_table) {
            return 0;
        }

        generate_window_table(&c.window_table[0], c.capture_size, (c.window_type & WINDOW_KIND_MASK));
    }

    if (c.capture_waveform || c.capture_fft) {
        c.waveform_buff.allocate((c.capture_size * c.num_channels), CXXPH_PLATFORM_SIMD_ALIGNMENT, false);

        if (!c.waveform_buff) {
            return 0;
        }

        if (!(c.internal_event_listener.get())) {
            c.waveform_buff2.allocate((c.capture_size * c.num_channels), CXXPH_PLATFORM_SIMD_ALIGNMENT, false);

            if (!c.waveform_buff2) {
                return 0;
            }
        }
    }

    if (c.capture_fft) {
        const int fft_n_data = cxxdasp::utils::forward_fft_real_num_outputs(c.capture_size);

        c.fft_buff.allocate((fft_n_data * c.num_channels), CXXPH_PLATFORM_SIMD_ALIGNMENT, false);

        if (!c.fft_buff) {
            return 0;
        }

        c.fftr_lch.setup(c.capture_size, &c.waveform_buff[0], &c.fft_buff[0]);
        c.fftr_rch.setup(c.capture_size, &c.waveform_buff[c.capture_size], &c.fft_buff[c.capture_size / 2]);
    }

    // set thread name
    AndroidHelper::setCurrentThreadName("OSLMPHQVis");

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
                                                               (c.capture_size * c.num_channels));
        }
    }

    if (c.capture_fft) {
        if (c.internal_event_listener.get()) {
            c.internal_event_listener->onAllocateCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_FFT,
                                                               (c.capture_size * c.num_channels));
        }
    }

    thiz->periodicCaptureThread(c);

    // release buffers

    if (c.capture_waveform || c.capture_fft) {
        if (c.internal_event_listener.get()) {
            c.internal_event_listener->onDeAllocateCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_WAVEFORM);
        }

        c.waveform_buff.free();
        c.waveform_buff2.free();
    }

    if (c.capture_fft) {
        if (c.internal_event_listener.get()) {
            c.internal_event_listener->onDeAllocateCaptureBuffer(c.visualizer, CAPTURE_BUFFER_TYPE_FFT);
        }

        c.fft_buff.free();
    }

    // raise onLeaveInternalPeriodicCaptureThread() event
    if (c.internal_event_listener.get()) {
        c.internal_event_listener->onLeaveInternalPeriodicCaptureThread(c.visualizer);
    }

    return 0;
}

int32_t HQVisualizerExtModule::calcDiffTimeSinceLastCaptureBufferUpdated() const noexcept
{
    timespec ts = utils::timespec_utils::ZERO();

    captured_data_buffer_.get_last_updated_time(&ts);

    return calcDiffTimeSinceLastCaptureBufferUpdated(ts);
}

int32_t HQVisualizerExtModule::calcDiffTimeSinceLastCaptureBufferUpdated(const timespec &last_updated) const noexcept
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
