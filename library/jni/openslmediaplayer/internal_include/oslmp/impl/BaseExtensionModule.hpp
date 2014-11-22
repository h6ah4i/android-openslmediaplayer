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

#ifndef BASEEXTENSIONMODULE_HPP_
#define BASEEXTENSIONMODULE_HPP_

#include <cxxporthelper/memory>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerThreadMessage.hpp"

namespace oslmp {
namespace impl {

class BaseExtensionModule : public OpenSLMediaPlayerExtension {
public:
    BaseExtensionModule(const char *module_name);
    virtual ~BaseExtensionModule();

    int detachClient(void *client) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual const char *getExtensionModuleName() const noexcept override;

    virtual uint32_t getExtensionTraits() const noexcept override;

    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onAttach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onDetach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onBeforeAudioSinkStateChanged(OpenSLMediaPlayerExtensionManager *extmgr,
                                               bool next_is_started) noexcept override;

    virtual void onCaptureAudioData(OpenSLMediaPlayerExtensionManager *extmgr, const float *data, uint32_t num_channels,
                                    size_t num_frames, uint32_t sample_rate_millihertz,
                                    const timespec *timestamp) noexcept override;

protected:
    typedef OpenSLMediaPlayerThreadMessage Message;

    bool post(Message *msg) noexcept;
    int postAndWaitResult(Message *msg) noexcept;
    int waitResult(const Message *msg) noexcept;
    void notifyResult(const Message *msg, int result) noexcept;

    bool notifyTraitsUpdated() noexcept;

    bool checkIsClientActive(void *client) const noexcept;
    void *getActiveClient() const noexcept;

    void getExtensionManager(android::sp<OpenSLMediaPlayerExtensionManager> &sp_extmgr) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace impl
} // namespace oslmp

#endif // BASEEXTENSIONMODULE_HPP_
