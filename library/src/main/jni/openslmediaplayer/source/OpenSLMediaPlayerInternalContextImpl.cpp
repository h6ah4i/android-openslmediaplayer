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

// #define LOG_TAG "OpenSLMediaPlayerInternalContext"

#include "oslmp/impl/OpenSLMediaPlayerInternalContextImpl.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalUtils.hpp"

#include <cassert>

#include <loghelper/loghelper.h>

#include "oslmp/impl/AudioSystem.hpp"
#include "oslmp/impl/AudioMixer.hpp"
#include "oslmp/impl/AndroidHelper.hpp"
#include "oslmp/utils/pthread_utils.hpp"
#include "oslmp/utils/bitmap_looper.hpp"

namespace oslmp {
namespace impl {

#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSB_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define CHECK_AUX_EFFECT_ID(aux_effect_id)                                                                             \
    (((aux_effect_id) == OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB || (aux_effect_id) == OSLMP_AUX_EFFECT_PRESET_REVERB))

#define CHECK_EXT_MODULE_NO(module_no) (((module_no) >= 1) && ((module_no) <= NUM_EXTENSIONS))

typedef OpenSLMediaPlayerInternalUtils InternalUtils;

//
// message codes
//
enum {
    MSG_NOP,
    MSG_REGISTER_MESSAGE_HANDLER,
    MSG_UNREGISTER_MESSAGE_HANDLER,
    MSG_EXT_ATTACH_OR_INSTALL,
    MSG_EXT_DETACH_OR_UNINSTALL,
};

//
// message structures
//

struct msg_blob_register_message_handler {
    OpenSLMediaPlayerInternalMessageHandler *handler;
    OpenSLMediaPlayerInternalMessageHandlerToken *token;
};

struct msg_blob_unregister_message_handler {
    OpenSLMediaPlayerInternalMessageHandler *handler;
    OpenSLMediaPlayerInternalMessageHandlerToken token;
};

struct msg_blob_ext_attach_or_install {
    OpenSLMediaPlayerExtension **attached_extension;
    const OpenSLMediaPlayerExtensionCreator *creator;
    void *user_arg;
};

struct msg_blob_ext_detach_or_uninstall {
    OpenSLMediaPlayerExtension *extension;
    OpenSLMediaPlayerExtensionToken token;
    void *user_arg;
};

//
// OpenSLMediaPlayerInternalContext
//
OpenSLMediaPlayerInternalContext &OpenSLMediaPlayerInternalContext::sGetInternal(OpenSLMediaPlayerContext &c) noexcept
{
    return c.getInternal();
}

//
// OpenSLMediaPlayerInternalContextImpl::ExtensionInfo
//
OpenSLMediaPlayerInternalContextImpl::ExtensionInfo::ExtensionInfo() : instance(nullptr), num_attached(0), traits(0UL)
{
}

OpenSLMediaPlayerInternalContextImpl::ExtensionInfo::~ExtensionInfo()
{
    if (instance) {
        LOGE("Extension %p is not released properly!!!", instance.get());
        instance.reset();
    }
    clear();
}

void OpenSLMediaPlayerInternalContextImpl::ExtensionInfo::clear() noexcept
{
    instance.release(); // NOTE: do not delete the object
    num_attached = 0;
    traits = 0UL;
}

//
// OpenSLMediaPlayerInternalContextImpl
//
OpenSLMediaPlayerInternalContextImpl::OpenSLMediaPlayerInternalContextImpl()
    : context_(nullptr), jvm_(nullptr), options_(0), extension_traits_updated_(0), audio_capture_extensions_bitmap_(0)
{

    static_assert(sizeof(OpenSLMediaPlayerInternalContextImpl::MessageTag) <= MessageHandlerThread::TAG_SIZE,
                  "Check message tag size");

    static_assert(static_cast<int>(NUM_MESSAGE_HANDLERS) == static_cast<int>(AudioMixer::NUM_MAX_SOURCE_CLIENTS),
                  "Verify NUM_MESSAGE_HANDLERS");
}

OpenSLMediaPlayerInternalContextImpl::~OpenSLMediaPlayerInternalContextImpl() { release(); }

int OpenSLMediaPlayerInternalContextImpl::initialize(JNIEnv *env, OpenSLMediaPlayerContext *context,
                                                     const OpenSLMediaPlayerContext::create_args_t &args) noexcept
{

    if (env->GetJavaVM(&jvm_) != JNI_OK) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    AndroidHelper::init();

#ifdef USE_OSLMP_DEBUG_FEATURES
    // start non blocing trace logger
    if (!non_block_trace_logger_.start_output_worker(jvm_, 50)) {
        return OSLMP_RESULT_INTERNAL_ERROR;
    }
#endif

    audio_system_.reset(new (std::nothrow) AudioSystem());

    if (!audio_system_) {
        return OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
    }

    // update fields
    context_ = context;
    options_ = args.options;
    listener_ = args.listener;

    int result;

    {
        AudioSystem::initialize_args_t init_args;

        init_args.context = this;
        init_args.system_out_sampling_rate = args.system_out_sampling_rate;
        init_args.system_out_frames_per_buffer = args.system_out_frames_per_buffer;
        init_args.system_supports_low_latency = args.system_supports_low_latency;
        init_args.system_supports_floating_point = args.system_supports_floating_point;
        init_args.stream_type = args.stream_type;
        init_args.short_fade_duration_ms = args.short_fade_duration;
        init_args.long_fade_duration_ms = args.long_fade_duration;
        init_args.resampler_quality = args.resampler_quality;
        init_args.hq_equalizer_impl_type = args.hq_equalizer_impl_type;
        init_args.sink_backend_type = args.sink_backend_type;
        init_args.use_low_latency_if_available = args.use_low_latency_if_available;
        init_args.use_floating_point_if_available = args.use_floating_point_if_available;

        result = audio_system_->initialize(init_args);
    }

    if (result != OSLMP_RESULT_SUCCESS) {
        audio_system_.reset();
        listener_ = 0;
        options_ = 0;
        context_ = 0;

        return result;
    }

    // start message handler thread
    if (!msgHandlerThread_.start(this)) {
        audio_system_.reset();
        listener_ = 0;
        options_ = 0;
        context_ = 0;

        return OSLMP_RESULT_INTERNAL_ERROR;
    }

    // set audio capture event listener
    audio_system_->setAudioCaptureEventListener(this);

    // start mixer in suspended state
    result = audio_system_->getMixer()->start(true);
    if (result != OSLMP_RESULT_SUCCESS) {
        audio_system_.reset();
        listener_ = 0;
        options_ = 0;
        context_ = 0;

        return result;
    }

    return OSLMP_RESULT_SUCCESS;
}

void OpenSLMediaPlayerInternalContextImpl::release() noexcept
{
    msgHandlerThread_.join(nullptr);

    audio_system_.reset();

    jvm_ = nullptr;

#ifdef USE_OSLMP_DEBUG_FEATURES
    // stop non blocing trace logger
    non_block_trace_logger_.stop_output_worker();
#endif
}

JavaVM *OpenSLMediaPlayerInternalContextImpl::getJavaVM() const noexcept { return jvm_; }

SLresult OpenSLMediaPlayerInternalContextImpl::getInterfaceFromEngine(opensles::CSLInterface *itf) noexcept
{
    if (!audio_system_)
        return SL_RESULT_INTERNAL_ERROR;
    return audio_system_->getInterfaceFromEngine(itf);
}

SLresult OpenSLMediaPlayerInternalContextImpl::getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept
{
    if (!audio_system_)
        return SL_RESULT_INTERNAL_ERROR;
    return audio_system_->getInterfaceFromOutputMixer(itf);
}

SLresult OpenSLMediaPlayerInternalContextImpl::getInterfaceFromSinkPlayer(opensles::CSLInterface *itf, bool instantiate) noexcept
{
    if (!audio_system_)
        return SL_RESULT_INTERNAL_ERROR;
    return audio_system_->getInterfaceFromSinkPlayer(itf, instantiate);
}

SLresult OpenSLMediaPlayerInternalContextImpl::releaseInterfaceFromSinkPlayer(opensles::CSLInterface *pInterface) noexcept
{
    if (!audio_system_)
        return SL_RESULT_INTERNAL_ERROR;
    return audio_system_->releaseInterfaceFromSinkPlayer(pInterface);
}


uint32_t OpenSLMediaPlayerInternalContextImpl::getContextOptions() const noexcept { return this->options_; }

OpenSLMediaPlayerContext::InternalThreadEventListener *
OpenSLMediaPlayerInternalContextImpl::getInternalThreadEventListener() const noexcept
{
    return listener_.get();
}

AudioSystem *OpenSLMediaPlayerInternalContextImpl::getAudioSystem() const noexcept { return audio_system_.get(); }

bool OpenSLMediaPlayerInternalContextImpl::registerMessageHandler(
    OpenSLMediaPlayerInternalMessageHandler *handler, OpenSLMediaPlayerInternalMessageHandlerToken *token) noexcept
{
    if (!(handler && token))
        return false;

    (*token) = 0;

    typedef msg_blob_register_message_handler blob_t;
    CHECK_MSB_BLOB_SIZE(blob_t);

    Message msg(0, MSG_REGISTER_MESSAGE_HANDLER);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.handler = handler;
        blob.token = token;
    }

    int result = postAndWaitResult(&msg);

    return (result == OSLMP_RESULT_SUCCESS);
}

bool OpenSLMediaPlayerInternalContextImpl::unregisterMessageHandler(
    OpenSLMediaPlayerInternalMessageHandler *handler, OpenSLMediaPlayerInternalMessageHandlerToken token) noexcept
{

    if (!verifyMessageHandlerToken(token))
        return false;

    typedef msg_blob_unregister_message_handler blob_t;
    CHECK_MSB_BLOB_SIZE(blob_t);

    Message msg(0, MSG_UNREGISTER_MESSAGE_HANDLER);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.handler = handler;
        blob.token = token;
    }

    int result = postAndWaitResult(&msg);

    return (result == OSLMP_RESULT_SUCCESS);
}

bool OpenSLMediaPlayerInternalContextImpl::postMessage(OpenSLMediaPlayerInternalMessageHandler *handler,
                                                       OpenSLMediaPlayerInternalMessageHandlerToken token, void *msg,
                                                       size_t size) noexcept
{

    if (!(msg && verifyMessageHandlerToken(token))) {
        LOCAL_ASSERT(false);
        return false;
    }

    MessageTag tag;

    tag.handler = handler;
    tag.token = token;

    return msgHandlerThread_.post(msg, size, &tag, sizeof(tag));
}

void OpenSLMediaPlayerInternalContextImpl::raiseOnBeforeAudioSinkStateChanged(bool next_is_started) noexcept
{
    for (int i = 0; i < NUM_EXTENSIONS; ++i) {
        if (extensions_[i].instance) {
            extensions_[i].instance->onBeforeAudioSinkStateChanged(this, next_is_started);
        }
    }
}

#if USE_OSLMP_DEBUG_FEATURES
NonBlockingTraceLogger &OpenSLMediaPlayerInternalContextImpl::getNonBlockingTraceLogger() const noexcept
{
    return non_block_trace_logger_;
}
#endif

bool OpenSLMediaPlayerInternalContextImpl::verifyMessageHandlerToken(
    OpenSLMediaPlayerInternalMessageHandlerToken token) const noexcept
{
    uintptr_t min_token = reinterpret_cast<uintptr_t>(&msgHandlers_[0]);
    uintptr_t max_token = reinterpret_cast<uintptr_t>(&msgHandlers_[NUM_MESSAGE_HANDLERS - 1]);

    return ((token >= min_token) && (token <= max_token) && (((token - min_token) % sizeof(MessageHandlerInfo)) == 0));
}

bool
OpenSLMediaPlayerInternalContextImpl::verifyExtensionToken(OpenSLMediaPlayerInternalMessageHandlerToken token) const
    noexcept
{
    uintptr_t min_token = reinterpret_cast<uintptr_t>(&extensions_[0]);
    uintptr_t max_token = reinterpret_cast<uintptr_t>(&extensions_[NUM_EXTENSIONS - 1]);

    return ((token >= min_token) && (token <= max_token) && (((token - min_token) % sizeof(ExtensionInfo)) == 0));
}

//
// implementations of OpenSLMediaPlayerExtensionManager
//
int OpenSLMediaPlayerInternalContextImpl::extAttachOrInstall(OpenSLMediaPlayerExtension **attached_extension,
                                                             const OpenSLMediaPlayerExtensionCreator *creator,
                                                             void *user_arg) noexcept
{
    typedef msg_blob_ext_attach_or_install blob_t;
    CHECK_MSB_BLOB_SIZE(blob_t);
    Message msg(0, MSG_EXT_ATTACH_OR_INSTALL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);

        blob.attached_extension = attached_extension;
        blob.creator = creator;
        blob.user_arg = user_arg;
    }

    return postAndWaitResult(&msg);
}

int OpenSLMediaPlayerInternalContextImpl::extDetachOrUninstall(OpenSLMediaPlayerExtension *extension,
                                                               OpenSLMediaPlayerExtensionToken token,
                                                               void *user_arg) noexcept
{
    typedef msg_blob_ext_detach_or_uninstall blob_t;
    CHECK_MSB_BLOB_SIZE(blob_t);
    Message msg(0, MSG_EXT_DETACH_OR_UNINSTALL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);

        blob.extension = extension;
        blob.token = token;
        blob.user_arg = user_arg;
    }

    return postAndWaitResult(&msg);
}

bool OpenSLMediaPlayerInternalContextImpl::extPostMessage(OpenSLMediaPlayerExtension *handler,
                                                          OpenSLMediaPlayerExtensionToken token, void *msg,
                                                          size_t size) noexcept
{

    if (!(msg && verifyExtensionToken(token))) {
        LOCAL_ASSERT(false);
        return false;
    }

    MessageTag tag;

    tag.handler = handler;
    tag.token = token;

    return msgHandlerThread_.post(msg, size, &tag, sizeof(tag));
}

SLresult OpenSLMediaPlayerInternalContextImpl::extGetInterfaceFromPlayer(opensles::CSLInterface *pInterface, bool instantiate) noexcept
{
    return this->getInterfaceFromSinkPlayer(pInterface, instantiate);
}

SLresult OpenSLMediaPlayerInternalContextImpl::extGetInterfaceFromOutputMix(opensles::CSLInterface *pInterface) noexcept
{
    return this->getInterfaceFromOutputMixer(pInterface);
}

SLresult OpenSLMediaPlayerInternalContextImpl::extReleaseInterfaceFromPlayer(opensles::CSLInterface *pInterface) noexcept
{
    return this->releaseInterfaceFromSinkPlayer(pInterface);
}

int OpenSLMediaPlayerInternalContextImpl::extTranslateOpenSLErrorCode(SLresult result) const noexcept
{
    return InternalUtils::sTranslateOpenSLErrorCode(result);
}

bool OpenSLMediaPlayerInternalContextImpl::extNotifyTraitsUpdated(OpenSLMediaPlayerExtensionToken token) noexcept
{
    // NOTE:
    // This function has to be called from the message handler thread.

    if (!verifyExtensionToken(token))
        return false;

    // set pending flag
    extension_traits_updated_ = token;

    return true;
}

uint32_t OpenSLMediaPlayerInternalContextImpl::extGetOutputLatency() const noexcept
{
    uint32_t latency = 0;
    audio_system_->getOutputLatencyInFrames(&latency);
    return latency;
}

uint32_t OpenSLMediaPlayerInternalContextImpl::extGetOutputSamplingRate() const noexcept
{
    uint32_t sampling_rate = 0;
    audio_system_->getSystemOutputSamplingRate(&sampling_rate);
    return sampling_rate;
}

int OpenSLMediaPlayerInternalContextImpl::extSetAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept
{
    return audio_system_->setAuxEffectEnabled(aux_effect_id, enabled);
}

int OpenSLMediaPlayerInternalContextImpl::extGetPreAmp(PreAmp **p_preamp) const noexcept
{
    return audio_system_->getPreAmp(p_preamp);
}

int OpenSLMediaPlayerInternalContextImpl::extGetHQEqualizer(HQEqualizer **p_hq_equalizer) const noexcept
{
    return audio_system_->getHQEqualizer(p_hq_equalizer);
}

JavaVM *OpenSLMediaPlayerInternalContextImpl::extGetJavaVM() const noexcept { return getJavaVM(); }

// ---

bool OpenSLMediaPlayerInternalContextImpl::post(Message *msg) noexcept
{
    MessageTag tag;

    tag.handler = nullptr;
    tag.token = reinterpret_cast<OpenSLMediaPlayerInternalMessageHandlerToken>(this);

    return msgHandlerThread_.post(msg, sizeof(Message), &tag, sizeof(tag));
}

int OpenSLMediaPlayerInternalContextImpl::postAndWaitResult(Message *msg) noexcept
{
    utils::pt_lock_guard guard(mutex_);

    volatile int result = OSLMP_RESULT_ERROR;
    volatile bool processed = false;

    msg->result = &result;
    msg->processed = &processed;

    MessageTag tag;

    tag.handler = nullptr;
    tag.token = reinterpret_cast<OpenSLMediaPlayerInternalMessageHandlerToken>(this);

    if (!msgHandlerThread_.post(msg, sizeof(Message), &tag, sizeof(tag)))
        return OSLMP_RESULT_ERROR;

    return waitResult(msg);
}

int OpenSLMediaPlayerInternalContextImpl::waitResult(const Message *msg) noexcept
{
    LOCAL_ASSERT(msg);
    LOCAL_ASSERT(msg->what != MSG_NOP);

    utils::pt_unique_lock lock(mutex_wait_processed_);

    while (!(*(msg->processed))) {
        cond_wait_processed_.wait(lock);
    }

    return *(msg->result);
}

void OpenSLMediaPlayerInternalContextImpl::notifyResult(const Message *msg, int result) noexcept
{
    utils::pt_unique_lock lock(mutex_wait_processed_);

    (*(msg->result)) = result;
    (*(msg->processed)) = true;

    cond_wait_processed_.notify_all();
}

void OpenSLMediaPlayerInternalContextImpl::onEnterHandlerThread() noexcept
{
    JavaVM *vm = getJavaVM();
    JNIEnv *env = nullptr;

    // attach JavaVM
    (void) vm->AttachCurrentThread(&env, nullptr);

    // set thread name
    AndroidHelper::setCurrentThreadName("OSLMPMsgHandler");

    // set thread priority
    AndroidHelper::setThreadPriority(env, 0, ANDROID_THREAD_PRIORITY_MORE_FAVORABLE);

    android::sp<OpenSLMediaPlayerContext::InternalThreadEventListener> listener;
    listener = listener_;
    if (listener.get()) {
        listener->onEnterInternalThread(context_);
    }
}

bool OpenSLMediaPlayerInternalContextImpl::onHandleMessage(const void *msg, const void *tag) noexcept
{

    const MessageTag &tag_ = *(reinterpret_cast<const MessageTag *>(tag));

    if (tag_.token == reinterpret_cast<OpenSLMediaPlayerInternalMessageHandlerToken>(this)) {
        handleInternalMessages(static_cast<const Message *>(msg));
    } else if (verifyMessageHandlerToken(tag_.token)) {
        MessageHandlerInfo &info = *(reinterpret_cast<MessageHandlerInfo *>(tag_.token));

        if (info.handler == tag_.handler) {
            (info.handler)->onHandleMessage(msg);
        }
    } else if (verifyExtensionToken(tag_.token)) {
        handleExtensionMessages(static_cast<const OpenSLMediaPlayerThreadMessage *>(msg), &tag_);
    } else {
        LOGE("Message from unknown sender; token = %p", reinterpret_cast<void *>(tag_.token));
    }

    return true;
}

void OpenSLMediaPlayerInternalContextImpl::handleInternalMessages(const Message *msg) noexcept
{
    int result = OSLMP_RESULT_ERROR;

    switch (msg->what) {
    case MSG_NOP:
        break;
    case MSG_REGISTER_MESSAGE_HANDLER:
        result = handleRegisterMessage(msg);
        break;
    case MSG_UNREGISTER_MESSAGE_HANDLER:
        result = handleUnregisterMessage(msg);
        break;
    case MSG_EXT_ATTACH_OR_INSTALL:
        result = handleExtAttachOrInstallMessage(msg);
        break;
    case MSG_EXT_DETACH_OR_UNINSTALL:
        result = handleExtDetachOrUninstallMessage(msg);
        break;
    }

    if (msg->needNotification()) {
        notifyResult(msg, result);
    }
}

void OpenSLMediaPlayerInternalContextImpl::handleExtensionMessages(const Message *msg, const MessageTag *tag) noexcept
{
    ExtensionInfo &info = *(reinterpret_cast<ExtensionInfo *>(tag->token));

    if (info.instance) {
        info.instance->onHandleMessage(this, msg);
    }

    // update
    if (extension_traits_updated_) {
        assert(extension_traits_updated_ == tag->token);
        info.traits = info.instance->getExtensionTraits();
        processAfterExtensionTraitsChanged(info);
        extension_traits_updated_ = 0;
    }
}

void OpenSLMediaPlayerInternalContextImpl::processAfterExtensionTraitsChanged(
    OpenSLMediaPlayerInternalContextImpl::ExtensionInfo &ext) noexcept
{
    int index =
        (reinterpret_cast<uintptr_t>(&ext) - reinterpret_cast<uintptr_t>(&extensions_[0])) / sizeof(ExtensionInfo);
    const uint32_t mask = static_cast<uint32_t>(1UL << index);
    uint32_t &acap_bitmap = audio_capture_extensions_bitmap_;

    if (ext.traits & OpenSLMediaPlayerExtension::TRAIT_CAPTURE_AUDIO_DATA) {
        acap_bitmap |= mask;
    } else {
        acap_bitmap &= ~mask;
    }

    audio_system_->setAudioCaptureEnabled((acap_bitmap != 0));
}

int OpenSLMediaPlayerInternalContextImpl::handleRegisterMessage(const Message *msg) noexcept
{
    typedef msg_blob_register_message_handler blob_t;
    CHECK_MSB_BLOB_SIZE(blob_t);

    const blob_t &blob = GET_MSG_BLOB(*msg);

    for (MessageHandlerInfo &info : msgHandlers_) {
        if (!info.handler) {
            info.handler = blob.handler;
            (*blob.token) = reinterpret_cast<OpenSLMediaPlayerInternalMessageHandlerToken>(&info);

            // raise onRegisteredAsMessageHandler() event
            return (info.handler)->onRegisteredAsMessageHandler();
        }
    }

    return OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
}

int OpenSLMediaPlayerInternalContextImpl::handleUnregisterMessage(const Message *msg) noexcept
{
    typedef msg_blob_unregister_message_handler blob_t;
    CHECK_MSB_BLOB_SIZE(blob_t);

    const blob_t &blob = GET_MSG_BLOB(*msg);

    if (!verifyMessageHandlerToken(blob.token)) {
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    MessageHandlerInfo &info = *(reinterpret_cast<MessageHandlerInfo *>(blob.token));

    if (info.handler == blob.handler) {
        MessageHandlerInfo copy = info;

        // clear handler
        info.handler = nullptr;

        // raise onUnregisteredAsMessageHandler() event
        (copy.handler)->onUnregisteredAsMessageHandler();
    }

    return OSLMP_RESULT_SUCCESS;
}

int OpenSLMediaPlayerInternalContextImpl::handleExtAttachOrInstallMessage(const Message *msg) noexcept
{
    typedef msg_blob_ext_attach_or_install blob_t;
    const blob_t &blob = GET_MSG_BLOB(*msg);
    int result;
    bool installed = false;

    if (!(blob.attached_extension && blob.creator))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    // try to attach
    result = internalExtAttach(blob.creator, blob.attached_extension, blob.user_arg);

    if (result != OSLMP_RESULT_SUCCESS) {
        // install a new module
        result = internalExtInstall(blob.creator, blob.attached_extension, blob.user_arg);

        if (result == OSLMP_RESULT_SUCCESS) {
            installed = true;
        }
    }

    LOGD("Extension install/attach status : name = %s, result = %d, installed = %d", blob.creator->getModuleName(),
         result, installed);

    return result;
}

int OpenSLMediaPlayerInternalContextImpl::handleExtDetachOrUninstallMessage(const Message *msg) noexcept
{
    typedef msg_blob_ext_detach_or_uninstall blob_t;
    const blob_t &blob = GET_MSG_BLOB(*msg);
    int result;
#ifdef LOG_TAG
    bool uninstalled = false;
#endif

    int index = -1;
    for (int i = 0; i < NUM_EXTENSIONS; ++i) {
        ExtensionInfo &ext = extensions_[i];

        if (ext.instance.get() == blob.extension) {
            // found already installed module
            index = i;
            break;
        }
    }

    if (index >= 0) {
        ExtensionInfo &ext = extensions_[index];

        // decrement counter
        if (ext.num_attached > 0) {
            ext.num_attached -= 1;
        }

        if (ext.num_attached > 0) {
            std::unique_ptr<OpenSLMediaPlayerExtension> &instance = ext.instance;

            // raise onDetach() event
            instance->onDetach(this, blob.user_arg);
        } else {
            std::unique_ptr<OpenSLMediaPlayerExtension> instance = std::move(ext.instance); // delete instance

            // clear info
            ext.clear();
            processAfterExtensionTraitsChanged(ext);

            // raise onUninstall() event
            instance->onUninstall(this, blob.user_arg);

#ifdef LOG_TAG
            uninstalled = true;
#endif
        }

        result = OSLMP_RESULT_SUCCESS;
    } else {
        result = OSLMP_RESULT_ILLEGAL_ARGUMENT;
    }

    LOGD("Extension uninstall status : no = %d, result = %d, uninstalled = %d", index, result, uninstalled);

    return result;
}

int OpenSLMediaPlayerInternalContextImpl::onDetermineWaitTimeout() noexcept
{
    if (audio_system_) {
        return audio_system_->determineNextPollingTime();
    }

    return -1; // infinity
}

bool OpenSLMediaPlayerInternalContextImpl::onReceiveMessageTimeout() noexcept
{

    if (audio_system_) {
        audio_system_->poll();
    }

    return true;
}

int OpenSLMediaPlayerInternalContextImpl::internalExtAttach(const OpenSLMediaPlayerExtensionCreator *creator,
                                                            OpenSLMediaPlayerExtension **attached_extension,
                                                            void *user_args) noexcept
{

    int result;
    const std::string name(creator->getModuleName());

    // find attached instance
    int index = -1;
    for (int i = 0; i < NUM_EXTENSIONS; ++i) {
        ExtensionInfo &ext = extensions_[i];

        if (ext.instance && name == ext.instance->getExtensionModuleName()) {
            // found already installed module
            index = i;
            break;
        }
    }

    if (index >= 0) {
        // success
        ExtensionInfo &ext = extensions_[index];

        // increment counter
        ext.num_attached += 1;

        ext.instance->onAttach(this, user_args);

        (*attached_extension) = ext.instance.get();

        result = OSLMP_RESULT_SUCCESS;
    } else {
        // no matched module found
        result = OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
    }

    return result;
}

int OpenSLMediaPlayerInternalContextImpl::internalExtInstall(const OpenSLMediaPlayerExtensionCreator *creator,
                                                             OpenSLMediaPlayerExtension **attached_extension,
                                                             void *user_arg) noexcept
{

    int result;

    // find free slot
    int index = -1;
    for (int i = 0; i < NUM_EXTENSIONS; ++i) {
        if (!extensions_[i].instance) {
            index = i;
            break;
        }
    }

    if (index >= 0) {
        std::unique_ptr<OpenSLMediaPlayerExtension> new_module(creator->createNewInstance());

        if (new_module) {
            ExtensionInfo &ext = extensions_[index];
            OpenSLMediaPlayerExtensionToken token = reinterpret_cast<OpenSLMediaPlayerExtensionToken>(&ext);

            // raise onInstall() event
            if (new_module->onInstall(this, token, user_arg)) {

                // success
                ext.instance = std::move(new_module);
                ext.num_attached = 1;
                ext.traits = ext.instance->getExtensionTraits();

                processAfterExtensionTraitsChanged(ext);

                (*attached_extension) = ext.instance.get();

                result = OSLMP_RESULT_SUCCESS;
            } else {
                result = OSLMP_RESULT_ERROR;
            }
        } else {
            result = OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
        }
    } else {
        // no free extension slot found
        result = OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
    }

    return result;
}

void *OpenSLMediaPlayerInternalContextImpl::onLeaveHandlerThread(bool stop_requested) noexcept
{

    android::sp<OpenSLMediaPlayerContext::InternalThreadEventListener> listener;
    listener = listener_;
    if (listener.get()) {
        listener->onLeaveInternalThread(context_);
    }

    JavaVM *vm = getJavaVM();

    // detach JavaVM
    (void) vm->DetachCurrentThread();

    return nullptr;
}

void OpenSLMediaPlayerInternalContextImpl::onCaptureAudioData(const float *data, uint32_t num_channels,
                                                              size_t num_frames, uint32_t sample_rate_millihertz,
                                                              const timespec *timestamp) noexcept
{

    utils::bitmap_looper looper(audio_capture_extensions_bitmap_);

    while (looper.loop()) {
        ExtensionInfo &ext = extensions_[looper.index()];

        if (ext.instance) {
            ext.instance->onCaptureAudioData(this, data, num_channels, num_frames, sample_rate_millihertz, timestamp);
        }
    }
}

} // namespace impl
} // namespace oslmp
