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

// #define LOG_TAG "BaseExtensionModule"

#include "oslmp/impl/BaseExtensionModule.hpp"

#include <string>
#include <list>

#include <cxxporthelper/memory>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/utils/pthread_utils.hpp"

namespace oslmp {
namespace impl {

typedef android::sp<OpenSLMediaPlayerExtensionManager> extmgr_t;

class BaseExtensionModule::Impl {
public:
    Impl(BaseExtensionModule *module, const char *module_name);
    ~Impl();

    int detachClient(void *client) noexcept;

    const char *getModuleName() const noexcept;

    bool handleOnInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                         void *user_arg) noexcept;

    void handleOnAttach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept;

    void handleOnDetach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept;

    void handleOnUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept;

    bool post(Message *msg) noexcept;
    int postAndWaitResult(Message *msg) noexcept;
    int waitResult(const Message *msg) noexcept;
    void notifyResult(const Message *msg, int result) noexcept;

    bool notifyTraitsUpdated() noexcept;

    bool checkIsClientActive(void *client) const noexcept;
    void *getActiveClient() const noexcept;

    void getExtensionManager(android::sp<OpenSLMediaPlayerExtensionManager> &sp_extmgr) noexcept;

private:
    BaseExtensionModule *module_;
    const std::string module_name_;

    android::wp<OpenSLMediaPlayerExtensionManager> extmgr_;
    OpenSLMediaPlayerExtensionToken exttoken_;

    std::list<void *> client_queue_;
    utils::pt_mutex mutex_wait_processed_;
    utils::pt_condition_variable cond_wait_processed_;
};

//
// BaseExtensionModule
//

BaseExtensionModule::BaseExtensionModule(const char *module_name) : impl_(new (std::nothrow) Impl(this, module_name)) {}

BaseExtensionModule::~BaseExtensionModule() {}

int BaseExtensionModule::detachClient(void *client) noexcept
{
    if (!impl_)
        return OSLMP_RESULT_DEAD_OBJECT;

    return impl_->detachClient(client);
}

const char *BaseExtensionModule::getExtensionModuleName() const noexcept
{
    if (!impl_)
        return "NO MODULE";
    return impl_->getModuleName();
}

uint32_t BaseExtensionModule::getExtensionTraits() const noexcept { return TRAIT_NONE; }

bool BaseExtensionModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                    void *user_arg) noexcept
{

    if (!impl_)
        return false;

    return impl_->handleOnInstall(extmgr, token, user_arg);
}

void BaseExtensionModule::onAttach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    if (!impl_)
        return;

    impl_->handleOnAttach(extmgr, user_arg);
}

void BaseExtensionModule::onDetach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    if (!impl_)
        return;

    impl_->handleOnDetach(extmgr, user_arg);
}

void BaseExtensionModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    if (!impl_)
        return;

    impl_->handleOnUninstall(extmgr, user_arg);
}

void BaseExtensionModule::onBeforeAudioSinkStateChanged(OpenSLMediaPlayerExtensionManager *extmgr,
                                                        bool next_is_started) noexcept
{
}

void BaseExtensionModule::onCaptureAudioData(OpenSLMediaPlayerExtensionManager *extmgr, const float *data,
                                             uint32_t num_channels, size_t num_frames, uint32_t sample_rate_millihertz,
                                             const timespec *timestamp) noexcept
{
}

bool BaseExtensionModule::post(BaseExtensionModule::Message *msg) noexcept
{
    if (!impl_)
        return false;
    return impl_->post(msg);
}

int BaseExtensionModule::postAndWaitResult(BaseExtensionModule::Message *msg) noexcept
{
    if (!impl_)
        return OSLMP_RESULT_DEAD_OBJECT;
    return impl_->postAndWaitResult(msg);
}

int BaseExtensionModule::waitResult(const BaseExtensionModule::Message *msg) noexcept
{
    if (!impl_)
        return OSLMP_RESULT_DEAD_OBJECT;
    return impl_->waitResult(msg);
}

void BaseExtensionModule::notifyResult(const BaseExtensionModule::Message *msg, int result) noexcept
{
    if (!impl_)
        return;
    impl_->notifyResult(msg, result);
}

bool BaseExtensionModule::notifyTraitsUpdated() noexcept
{
    if (!impl_)
        return false;
    return impl_->notifyTraitsUpdated();
}

bool BaseExtensionModule::checkIsClientActive(void *client) const noexcept
{
    if (!impl_)
        return false;
    return impl_->checkIsClientActive(client);
}

void *BaseExtensionModule::getActiveClient() const noexcept
{
    if (!impl_)
        return nullptr;
    return impl_->getActiveClient();
}

void BaseExtensionModule::getExtensionManager(android::sp<OpenSLMediaPlayerExtensionManager> &sp_extmgr) noexcept
{
    if (!impl_)
        return;
    impl_->getExtensionManager(sp_extmgr);
}

//
// BaseExtensionModule::Impl
//
BaseExtensionModule::Impl::Impl(BaseExtensionModule *module, const char *module_name)
    : module_(module), module_name_(module_name), extmgr_(), exttoken_(0)
{
}

BaseExtensionModule::Impl::~Impl()
{
    extmgr_.clear();
    exttoken_ = 0;
}

int BaseExtensionModule::Impl::detachClient(void *client) noexcept
{
    extmgr_t extmgr(extmgr_.promote());
    if (!extmgr.get()) {
        return OSLMP_RESULT_DEAD_OBJECT;
    }
    return extmgr->extDetachOrUninstall(module_, exttoken_, client);
}

const char *BaseExtensionModule::Impl::getModuleName() const noexcept { return module_name_.c_str(); }

bool BaseExtensionModule::Impl::handleOnInstall(OpenSLMediaPlayerExtensionManager *extmgr,
                                                OpenSLMediaPlayerExtensionToken token, void *user_arg) noexcept
{

    // success
    LOGD("<%s> Installed", module_name_.c_str());

    extmgr_ = extmgr;
    exttoken_ = token;

    // Register client
    client_queue_.clear();
    client_queue_.push_front(user_arg);

    return true;
}

void BaseExtensionModule::Impl::handleOnAttach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    LOGD("<%s> Attached", module_name_.c_str());

    // Register client
    client_queue_.push_front(user_arg);
}

void BaseExtensionModule::Impl::handleOnDetach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    LOGD("<%s> Detached", module_name_.c_str());

    // Remove client
    client_queue_.remove(user_arg);
}

void BaseExtensionModule::Impl::handleOnUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    LOGD("<%s> Uninstalled", module_name_.c_str());

    // Remove clients
    client_queue_.clear();
}

bool BaseExtensionModule::Impl::post(BaseExtensionModule::Message *msg) noexcept
{
    extmgr_t extmgr(extmgr_.promote());

    if (!extmgr.get())
        return false;
    return extmgr->extPostMessage(module_, exttoken_, msg, sizeof(Message));
}

int BaseExtensionModule::Impl::postAndWaitResult(BaseExtensionModule::Message *msg) noexcept
{
    volatile int result = OSLMP_RESULT_ERROR;
    volatile bool processed = false;

    msg->result = &result;
    msg->processed = &processed;

    if (!post(msg))
        return OSLMP_RESULT_DEAD_OBJECT;

    return waitResult(msg);
}

int BaseExtensionModule::Impl::waitResult(const BaseExtensionModule::Message *msg) noexcept
{
    utils::pt_unique_lock lock(mutex_wait_processed_);

    while (!(*(msg->processed))) {
        cond_wait_processed_.wait(lock);
    }

    return *(msg->result);
}

void BaseExtensionModule::Impl::notifyResult(const BaseExtensionModule::Message *msg, int result) noexcept
{
    utils::pt_unique_lock lock(mutex_wait_processed_);

    (*(msg->result)) = result;
    (*(msg->processed)) = true;

    cond_wait_processed_.notify_all();
}

bool BaseExtensionModule::Impl::notifyTraitsUpdated() noexcept
{
    extmgr_t extmgr(extmgr_.promote());

    if (!extmgr.get())
        return false;

    return extmgr->extNotifyTraitsUpdated(exttoken_);
}

bool BaseExtensionModule::Impl::checkIsClientActive(void *client) const noexcept
{
    if (!client)
        return false;

    return (client == getActiveClient());
}

void *BaseExtensionModule::Impl::getActiveClient() const noexcept
{
    if (client_queue_.empty())
        return nullptr;

    return client_queue_.front();
}

void BaseExtensionModule::Impl::getExtensionManager(android::sp<OpenSLMediaPlayerExtensionManager> &sp_extmgr) noexcept
{
    sp_extmgr = extmgr_.promote();
}

} // namespace impl
} // namespace oslmp
