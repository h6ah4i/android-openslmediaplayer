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

// #define LOG_TAG "OpenSLMediaPlayerPreAmp"

#include "oslmp/OpenSLMediaPlayerPreAmp.hpp"

#include <cassert>
#include <vector>

#include <cxxporthelper/memory>
#include <cxxporthelper/cmath>
#include <cxxporthelper/compiler.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerThreadMessage.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/BaseExtensionModule.hpp"
#include "oslmp/impl/PreAmp.hpp"

//
// Constants
//
#define MODULE_NAME "PreAmp"

//
// helper macros
//

#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    PreAmpExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                                   \
    if (!varname) {                                                                                                    \
        return OSLMP_RESULT_DEAD_OBJECT;                                                                               \
    }

#define CHECK_ARG(cond)                                                                                                \
    if (!(cond)) {                                                                                                     \
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;                                                                          \
    }

#define CHECK_IS_ACTIVE(blob_) (checkIsClientActive((blob_).client))

#define CHECK_RANGE(value_, min_, max_) (((value_) >= (min_)) && ((value_) <= (max_)))

#define CHECK_ARG_RANGE(value_, min_, max_) CHECK_ARG(CHECK_RANGE((value_), (min_), (max_)))

namespace oslmp {

using namespace ::opensles;
using namespace ::oslmp::impl;

typedef OpenSLMediaPlayerInternalContext InternalContext;

class PreAmpExtModule : public BaseExtensionModule {
public:
    PreAmpExtModule();
    virtual ~PreAmpExtModule();

    int setEnabled(void *client, bool enabled) noexcept;
    int getEnabled(void *client, bool *enabled) noexcept;
    int getId(void *client, int *id) noexcept;
    int hasControl(void *client, bool *hasControl) noexcept;
    int getLevel(void *client, float *level) noexcept;
    int getProperties(void *client, OpenSLMediaPlayerPreAmp::Settings *settings) noexcept;
    int setLevel(void *client, float level) noexcept;
    int setProperties(void *client, const OpenSLMediaPlayerPreAmp::Settings *settings) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept override;

private:
    int processMessage(PreAmp &preamp, const OpenSLMediaPlayerThreadMessage *msg) noexcept;

    static int resetToDefaultState(PreAmp &preamp) noexcept;

    PreAmp *preamp_;
};

class PreAmpExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    PreAmpExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) PreAmpExtModule();
    }
};

class OpenSLMediaPlayerPreAmp::Impl {
public:
    Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client);
    ~Impl();

    android::sp<OpenSLMediaPlayerContext> context_;
    void *client_;
    PreAmpExtModule *module_;
};

enum {
    MSG_NOP,
    MSG_SET_ENABLED,
    MSG_GET_ENABLED,
    MSG_GET_ID,
    MSG_HAS_CONTROL,
    MSG_GET_LEVEL,
    MSG_GET_PROPERTIES,
    MSG_SET_LEVEL,
    MSG_SET_PROPERTIES,
};

struct msg_blob_set_enabled {
    void *client;
    bool enabled;
};

struct msg_blob_get_enabled {
    void *client;
    bool *enabled;
};

struct msg_blob_get_id {
    void *client;
    int32_t *id;
};

struct msg_blob_has_control {
    void *client;
    bool *hasControl;
};

struct msg_blob_get_level {
    void *client;
    float *level;
};

struct msg_blob_get_properties {
    void *client;
    OpenSLMediaPlayerPreAmp::Settings *settings;
};

struct msg_blob_set_level {
    void *client;
    float level;
};

struct msg_blob_set_properties {
    void *client;
    const OpenSLMediaPlayerPreAmp::Settings *settings;
};

//
// Utilities
//

//
// OpenSLMediaPlayerPreAmp
//
OpenSLMediaPlayerPreAmp::OpenSLMediaPlayerPreAmp(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(context, this))
{
}

OpenSLMediaPlayerPreAmp::~OpenSLMediaPlayerPreAmp()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerPreAmp::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(this, enabled);
}

int OpenSLMediaPlayerPreAmp::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(this, enabled);
}

int OpenSLMediaPlayerPreAmp::getId(int *id) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getId(this, id);
}

int OpenSLMediaPlayerPreAmp::hasControl(bool *hasControl) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->hasControl(this, hasControl);
}

int OpenSLMediaPlayerPreAmp::getLevel(float *level) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getLevel(this, level);
}

int OpenSLMediaPlayerPreAmp::getProperties(OpenSLMediaPlayerPreAmp::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getProperties(this, settings);
}

int OpenSLMediaPlayerPreAmp::setLevel(float level) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setLevel(this, level);
}

int OpenSLMediaPlayerPreAmp::setProperties(const OpenSLMediaPlayerPreAmp::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setProperties(this, settings);
}

//
// OpenSLMediaPlayerPreAmp::Impl
//
OpenSLMediaPlayerPreAmp::Impl::Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client)
    : context_(context), client_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_PREAMP) {
        const PreAmpExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module;

        int result = c.extAttachOrInstall(&module, &creator, client);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<PreAmpExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerPreAmp::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(client_);
        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// PreAmpExtModule
//
PreAmpExtModule::PreAmpExtModule() : BaseExtensionModule(MODULE_NAME), preamp_(nullptr) {}

PreAmpExtModule::~PreAmpExtModule() {}

int PreAmpExtModule::setEnabled(void *client, bool enabled) noexcept
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

int PreAmpExtModule::getEnabled(void *client, bool *enabled) noexcept
{
    typedef msg_blob_get_enabled blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(enabled != nullptr);

    (*enabled) = false;

    Message msg(0, MSG_GET_ENABLED);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.enabled = enabled;
    }

    return postAndWaitResult(&msg);
}

int PreAmpExtModule::getId(void *client, int *id) noexcept
{
    typedef msg_blob_get_id blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(id != nullptr);

    (*id) = 0;

    Message msg(0, MSG_GET_ID);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.id = id;
    }

    return postAndWaitResult(&msg);
}

int PreAmpExtModule::hasControl(void *client, bool *hasControl) noexcept
{
    typedef msg_blob_has_control blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(hasControl != nullptr);

    (*hasControl) = false;

    Message msg(0, MSG_HAS_CONTROL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.hasControl = hasControl;
    }

    return postAndWaitResult(&msg);
}

int PreAmpExtModule::getLevel(void *client, float *level) noexcept
{
    typedef msg_blob_get_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(level != nullptr);

    (*level) = 0;

    Message msg(0, MSG_GET_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.level = level;
    }

    return postAndWaitResult(&msg);
}

int PreAmpExtModule::getProperties(void *client, OpenSLMediaPlayerPreAmp::Settings *settings) noexcept
{
    typedef msg_blob_get_properties blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(settings != nullptr);

    Message msg(0, MSG_GET_PROPERTIES);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.settings = settings;
    }

    return postAndWaitResult(&msg);
}

int PreAmpExtModule::setLevel(void *client, float level) noexcept
{
    typedef msg_blob_set_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(!isnan(level));
    CHECK_ARG_RANGE(level, 0.0f, 2.0f);

    Message msg(0, MSG_SET_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.level = level;
    }

    return postAndWaitResult(&msg);
}

int PreAmpExtModule::setProperties(void *client, const OpenSLMediaPlayerPreAmp::Settings *settings) noexcept
{
    typedef msg_blob_set_properties blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(settings != nullptr);
    CHECK_ARG(!isnan(settings->level));
    CHECK_ARG_RANGE(settings->level, 0.0f, 2.0f);

    Message msg(0, MSG_SET_PROPERTIES);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.settings = settings;
    }

    return postAndWaitResult(&msg);
}

bool PreAmpExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                void *user_arg) noexcept
{

    bool available = false;
    PreAmp *preamp = nullptr;

    // obtain info
    {
        int result = extmgr->extGetPreAmp(&preamp);

        if (result == OSLMP_RESULT_SUCCESS && preamp) {
            result = resetToDefaultState(*preamp);

            if (result == OSLMP_RESULT_SUCCESS) {
                available = true;
            }
        }
    }

    if (!available)
        return false;

    // call super method
    bool super_result = BaseExtensionModule::onInstall(extmgr, token, user_arg);

    if (!super_result) {
        return false;
    }

    // update fields
    preamp_ = preamp;

    return true;
}

void PreAmpExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    // reset state
    if (preamp_) {
        resetToDefaultState(*preamp_);
    }

    // update fields
    preamp_ = nullptr;

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void PreAmpExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                      const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    int result;

    if (preamp_) {
        result = processMessage((*preamp_), msg);
    } else {
        result = OSLMP_RESULT_ILLEGAL_STATE;
    }

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, result);
    }
}

int PreAmpExtModule::processMessage(PreAmp &preamp, const OpenSLMediaPlayerThreadMessage *msg) noexcept
{
    int result = OSLMP_RESULT_INTERNAL_ERROR;

    switch (msg->what) {
    case MSG_NOP: {
        LOCAL_ASSERT(false);
    } break;
    case MSG_SET_ENABLED: {
        typedef msg_blob_set_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            result = preamp.setEnabled(blob.enabled);
        } else {
            result = OSLMP_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_ENABLED: {
        typedef msg_blob_get_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        result = preamp.getEnabled(blob.enabled);
    } break;
    case MSG_GET_ID: {
        typedef msg_blob_get_id blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.id) = 0;
        result = OSLMP_RESULT_SUCCESS;
    } break;
    case MSG_HAS_CONTROL: {
        typedef msg_blob_has_control blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.hasControl) = CHECK_IS_ACTIVE(blob);
        result = OSLMP_RESULT_SUCCESS;
    } break;
    case MSG_GET_LEVEL: {
        typedef msg_blob_get_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        result = preamp.getLevel(blob.level);
    } break;
    case MSG_GET_PROPERTIES: {
        typedef msg_blob_get_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        blob.settings->level = 0.0f;
        result = preamp.getLevel(&(blob.settings->level));
    } break;
    case MSG_SET_LEVEL: {
        typedef msg_blob_set_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            result = preamp.setLevel(blob.level);
        } else {
            result = OSLMP_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_PROPERTIES: {
        typedef msg_blob_set_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            result = preamp.setLevel(blob.settings->level);
        } else {
            result = OSLMP_RESULT_CONTROL_LOST;
        }
    } break;
    default:
        LOGD("Unexpected message; what = %d", msg->what);
        break;
    }

    return result;
}

int PreAmpExtModule::resetToDefaultState(PreAmp &preamp) noexcept
{
    int result;

    result = preamp.setEnabled(false);
    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    result = preamp.setLevel(1.0f);

    return result;
}

} // namespace oslmp
