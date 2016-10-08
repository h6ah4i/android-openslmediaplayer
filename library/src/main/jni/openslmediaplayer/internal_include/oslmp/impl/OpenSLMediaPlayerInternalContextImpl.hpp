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

#ifndef OPENSLMEDIAPLAYERINTERNALCONTEXTIMPL_HPP_
#define OPENSLMEDIAPLAYERINTERNALCONTEXTIMPL_HPP_

#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/MessageHandlerThread.hpp"
#include "oslmp/impl/OpenSLMediaPlayerThreadMessage.hpp"
#include "oslmp/impl/AudioSystem.hpp"

#include "oslmp/utils/pthread_utils.hpp"

namespace oslmp {
namespace impl {

class OpenSLMediaPlayerInternalContextImpl : public OpenSLMediaPlayerInternalContext,
                                             public MessageHandlerThread::EventHandler,
                                             public AudioSystem::AudioCaptureEventListener {
public:
    typedef OpenSLMediaPlayerThreadMessage Message;

    OpenSLMediaPlayerInternalContextImpl();
    virtual ~OpenSLMediaPlayerInternalContextImpl();

    int initialize(JNIEnv *env, OpenSLMediaPlayerContext *context,
                   const OpenSLMediaPlayerContext::create_args_t &args) noexcept;

    // implementations of OpenSLMediaPlayerInternalContext
    virtual JavaVM *getJavaVM() const noexcept override;
    virtual SLresult getInterfaceFromEngine(opensles::CSLInterface *itf) noexcept override;
    virtual SLresult getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept override;
    virtual SLresult getInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept override;
    virtual uint32_t getContextOptions() const noexcept override;
    virtual OpenSLMediaPlayerContext::InternalThreadEventListener *getInternalThreadEventListener() const
        noexcept override;
    virtual AudioSystem *getAudioSystem() const noexcept override;

    virtual bool registerMessageHandler(OpenSLMediaPlayerInternalMessageHandler *handler,
                                        OpenSLMediaPlayerInternalMessageHandlerToken *token) noexcept override;
    virtual bool unregisterMessageHandler(OpenSLMediaPlayerInternalMessageHandler *handler,
                                          OpenSLMediaPlayerInternalMessageHandlerToken token) noexcept override;
    virtual bool postMessage(OpenSLMediaPlayerInternalMessageHandler *handler,
                             OpenSLMediaPlayerInternalMessageHandlerToken token, void *msg,
                             size_t size) noexcept override;

    virtual void raiseOnBeforeAudioSinkStateChanged(bool next_is_started) noexcept override;

#if USE_OSLMP_DEBUG_FEATURES
    virtual NonBlockingTraceLogger &getNonBlockingTraceLogger() const noexcept override;
#endif

    // implementations of OpenSLMediaPlayerInternalContext (OpenSLMediaPlayerExtensionManager)
    virtual int extAttachOrInstall(OpenSLMediaPlayerExtension **attached_extension,
                                   const OpenSLMediaPlayerExtensionCreator *creator, void *user_args) noexcept override;

    virtual int extDetachOrUninstall(OpenSLMediaPlayerExtension *extension, OpenSLMediaPlayerExtensionToken token,
                                     void *user_args) noexcept override;

    virtual bool extPostMessage(OpenSLMediaPlayerExtension *handler, OpenSLMediaPlayerExtensionToken token, void *msg,
                                size_t size) noexcept override;

    virtual SLresult extGetInterfaceFromPlayer(opensles::CSLInterface *pInterface) noexcept override;
    virtual SLresult extGetInterfaceFromOutputMix(opensles::CSLInterface *pInterface) noexcept override;

    virtual int extTranslateOpenSLErrorCode(SLresult result) const noexcept override;

    virtual bool extNotifyTraitsUpdated(OpenSLMediaPlayerExtensionToken token) noexcept override;

    virtual uint32_t extGetOutputLatency() const noexcept override;
    virtual uint32_t extGetOutputSamplingRate() const noexcept override;

    virtual int extSetAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept override;

    virtual int extGetPreAmp(PreAmp **p_preamp) const noexcept override;
    virtual int extGetHQEqualizer(HQEqualizer **p_hq_equalizer) const noexcept override;

    virtual JavaVM *extGetJavaVM() const noexcept override;

    // implementations of MessageHandlerThread::EventHandler
    virtual void onEnterHandlerThread() noexcept override;
    virtual bool onHandleMessage(const void *msg, const void *tag) noexcept override;
    virtual bool onReceiveMessageTimeout() noexcept override;
    virtual void *onLeaveHandlerThread(bool stop_requested) noexcept override;
    virtual int onDetermineWaitTimeout() noexcept override;

    // implementations of AudioSystem::AudioCaptureEventListener
    virtual void onCaptureAudioData(const float *data, uint32_t num_channels, size_t num_frames,
                                    uint32_t sample_rate_millihertz, const timespec *timestamp) noexcept override;

private:
    enum {
        NUM_EXTENSIONS = 16,      // <= 32
        NUM_MESSAGE_HANDLERS = 4, // == AudioMixer::NUM_MAX_SOURCE_CLIENTS
    };

    struct MessageHandlerInfo {
        OpenSLMediaPlayerInternalMessageHandler *handler;

        MessageHandlerInfo() : handler(nullptr) {}
    };

    struct MessageTag {
        void *handler;
        uintptr_t token;
    };

    struct ExtensionInfo {
        std::unique_ptr<OpenSLMediaPlayerExtension> instance;
        int num_attached;
        uint32_t traits;

        ExtensionInfo();
        ~ExtensionInfo();

        void clear() noexcept;
    };

    void release() noexcept;

    bool verifyMessageHandlerToken(OpenSLMediaPlayerInternalMessageHandlerToken token) const noexcept;
    bool verifyExtensionToken(OpenSLMediaPlayerExtensionToken token) const noexcept;

    void handleInternalMessages(const Message *msg) noexcept;
    void handleExtensionMessages(const Message *msg, const MessageTag *tag) noexcept;

    int handleRegisterMessage(const Message *msg) noexcept;
    int handleUnregisterMessage(const Message *msg) noexcept;
    int handleExtAttachOrInstallMessage(const Message *msg) noexcept;
    int handleExtDetachOrUninstallMessage(const Message *msg) noexcept;

    bool post(Message *msg) noexcept;
    int postAndWaitResult(Message *msg) noexcept;
    int waitResult(const Message *msg) noexcept;
    void notifyResult(const Message *msg, int result) noexcept;

    int internalExtAttach(const OpenSLMediaPlayerExtensionCreator *creator,
                          OpenSLMediaPlayerExtension **attached_extension, void *user_arg) noexcept;

    int internalExtInstall(const OpenSLMediaPlayerExtensionCreator *creator,
                           OpenSLMediaPlayerExtension **attached_extension, void *user_arg) noexcept;

    void processAfterExtensionTraitsChanged(OpenSLMediaPlayerInternalContextImpl::ExtensionInfo &ext) noexcept;

private:
    OpenSLMediaPlayerContext *context_;
    JavaVM *jvm_;
    android::sp<OpenSLMediaPlayerContext::InternalThreadEventListener> listener_;
    std::unique_ptr<AudioSystem> audio_system_;

    MessageHandlerThread msgHandlerThread_;

    utils::pt_condition_variable cond_wait_processed_;
    utils::pt_mutex mutex_wait_processed_;
    mutable utils::pt_mutex mutex_;

    MessageHandlerInfo msgHandlers_[NUM_MESSAGE_HANDLERS];

    uint32_t options_;
    ExtensionInfo extensions_[NUM_EXTENSIONS];
    OpenSLMediaPlayerExtensionToken extension_traits_updated_;
    uint32_t audio_capture_extensions_bitmap_;

#if USE_OSLMP_DEBUG_FEATURES
    mutable NonBlockingTraceLogger non_block_trace_logger_;
#endif
};

} // namespace impl
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERINTERNALCONTEXTIMPL_HPP_
