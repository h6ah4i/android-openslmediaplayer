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

// #define LOG_TAG "OpenSLMediaPlayerVirtualizer"

#include "oslmp/OpenSLMediaPlayerVirtualizer.hpp"

#include <cassert>

#include <cxxporthelper/memory>
#include <cxxporthelper/compiler.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/BaseExtensionModule.hpp"

//
// Constants
//
#define MODULE_NAME "Virtualizer"

#define STRENGTH_MIN 0
#define STRENGTH_MAX 1000
#define DEFAULT_STRENGTH 750

//
// helper macros
//

#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    VirtualizerExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                              \
    if (CXXPH_UNLIKELY(!(varname))) {                                                                                  \
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

namespace oslmp {

using namespace ::opensles;
using namespace ::oslmp::impl;

typedef OpenSLMediaPlayerInternalContext InternalContext;

class VirtualizerExtModule : public BaseExtensionModule {
public:
    VirtualizerExtModule();
    virtual ~VirtualizerExtModule();

    int setEnabled(void *client, bool enabled) noexcept;
    int getEnabled(void *client, bool *enabled) noexcept;
    int getId(void *client, int *id) noexcept;
    int hasControl(void *client, bool *hasControl) noexcept;
    int getStrengthSupported(void *client, bool *strengthSupported) noexcept;
    int getRoundedStrength(void *client, int16_t *roundedStrength) noexcept;
    int getProperties(void *client, OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept;
    int setStrength(void *client, int16_t strength) noexcept;
    int setProperties(void *client, const OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept override;

private:
    SLresult processMessage(CSLVirtualizerItf &virtualizer, const OpenSLMediaPlayerThreadMessage *msg) noexcept;
};

class VirtualizerExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    VirtualizerExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) VirtualizerExtModule();
    }
};

class OpenSLMediaPlayerVirtualizer::Impl {
public:
    Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client);
    ~Impl();

    android::sp<OpenSLMediaPlayerContext> context_;
    void *client_;
    VirtualizerExtModule *module_;
};

enum {
    MSG_NOP,
    MSG_SET_ENABLED,
    MSG_GET_ENABLED,
    MSG_GET_ID,
    MSG_HAS_CONTROL,
    MSG_GET_STRENGTH_SUPPORTED,
    MSG_GET_ROUNDED_STRENGTH,
    MSG_GET_PROPERTIES,
    MSG_SET_STRENGTH,
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

struct msg_blob_get_strength_supported {
    void *client;
    bool *supported;
};

struct msg_blob_get_rounded_strength {
    void *client;
    int16_t *roundedStrength;
};

struct msg_blob_get_properties {
    void *client;
    OpenSLMediaPlayerVirtualizer::Settings *settings;
};

struct msg_blob_set_strength {
    void *client;
    int16_t strength;
};

struct msg_blob_set_properties {
    void *client;
    const OpenSLMediaPlayerVirtualizer::Settings *settings;
};

//
// Utilities
//

static inline SLresult GetProperties(CSLVirtualizerItf &virtualizer,
                                     OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept
{
    SLresult result;

    result = virtualizer.GetRoundedStrength(&(settings->strength));

    if (!IS_SL_SUCCESS(result))
        return result;

    return SL_RESULT_SUCCESS;
}

static inline SLresult SetProperties(CSLVirtualizerItf &virtualizer,
                                     const OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept
{
    SLresult result;

    result = virtualizer.SetStrength(settings->strength);

    if (!IS_SL_SUCCESS(result))
        return result;

    return SL_RESULT_SUCCESS;
}

static inline SLresult MakeDefaultSettings(CSLVirtualizerItf &virtualizer,
                                           OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept
{
    settings->strength = 0;

    return SL_RESULT_SUCCESS;
}

//
// OpenSLMediaPlayerVirtualizer
//
OpenSLMediaPlayerVirtualizer::OpenSLMediaPlayerVirtualizer(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(context, this))
{
}

OpenSLMediaPlayerVirtualizer::~OpenSLMediaPlayerVirtualizer()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerVirtualizer::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(this, enabled);
}

int OpenSLMediaPlayerVirtualizer::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(this, enabled);
}

int OpenSLMediaPlayerVirtualizer::getId(int *id) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getId(this, id);
}

int OpenSLMediaPlayerVirtualizer::hasControl(bool *hasControl) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->hasControl(this, hasControl);
}

int OpenSLMediaPlayerVirtualizer::getStrengthSupported(bool *strengthSupported) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getStrengthSupported(this, strengthSupported);
}

int OpenSLMediaPlayerVirtualizer::getRoundedStrength(int16_t *roundedStrength) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getRoundedStrength(this, roundedStrength);
}

int OpenSLMediaPlayerVirtualizer::getProperties(OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getProperties(this, settings);
}

int OpenSLMediaPlayerVirtualizer::setStrength(int16_t strength) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setStrength(this, strength);
}

int OpenSLMediaPlayerVirtualizer::setProperties(const OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setProperties(this, settings);
}

//
// OpenSLMediaPlayerVirtualizer::Impl
//
OpenSLMediaPlayerVirtualizer::Impl::Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client)
    : context_(context), client_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_VIRTUALIZER) {
        const VirtualizerExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module = nullptr;

        int result = c.extAttachOrInstall(&module, &creator, client);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<VirtualizerExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerVirtualizer::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(client_);

        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// VirtualizerExtModule
//
VirtualizerExtModule::VirtualizerExtModule() : BaseExtensionModule(MODULE_NAME) {}

VirtualizerExtModule::~VirtualizerExtModule() {}

int VirtualizerExtModule::setEnabled(void *client, bool enabled) noexcept
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

int VirtualizerExtModule::getEnabled(void *client, bool *enabled) noexcept
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

int VirtualizerExtModule::getId(void *client, int *id) noexcept
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

int VirtualizerExtModule::hasControl(void *client, bool *hasControl) noexcept
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

int VirtualizerExtModule::getStrengthSupported(void *client, bool *strengthSupported) noexcept
{
    typedef msg_blob_get_strength_supported blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(strengthSupported != nullptr);

    (*strengthSupported) = false;

    Message msg(0, MSG_GET_STRENGTH_SUPPORTED);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.supported = strengthSupported;
    }

    return postAndWaitResult(&msg);
}

int VirtualizerExtModule::getRoundedStrength(void *client, int16_t *roundedStrength) noexcept
{
    typedef msg_blob_get_rounded_strength blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(roundedStrength != nullptr);

    (*roundedStrength) = 0;

    Message msg(0, MSG_GET_ROUNDED_STRENGTH);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.roundedStrength = roundedStrength;
    }

    return postAndWaitResult(&msg);
}

int VirtualizerExtModule::getProperties(void *client, OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept
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

int VirtualizerExtModule::setStrength(void *client, int16_t strength) noexcept
{
    typedef msg_blob_set_strength blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG_RANGE(strength, STRENGTH_MIN, STRENGTH_MAX);

    Message msg(0, MSG_SET_STRENGTH);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.strength = strength;
    }

    return postAndWaitResult(&msg);
}

int VirtualizerExtModule::setProperties(void *client, const OpenSLMediaPlayerVirtualizer::Settings *settings) noexcept
{
    typedef msg_blob_set_properties blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(settings != nullptr);
    CHECK_ARG_RANGE(settings->strength, STRENGTH_MIN, STRENGTH_MAX);

    Message msg(0, MSG_SET_PROPERTIES);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.settings = settings;
    }

    return postAndWaitResult(&msg);
}

bool VirtualizerExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                     void *user_arg) noexcept
{

    bool available = false;

    CSLVirtualizerItf virtualizer;

    if (IS_SL_SUCCESS(extmgr->extGetInterfaceFromPlayer(&virtualizer))) {
        // acquired a virtualizer instance from the current player

        // apply default settings
        (void)virtualizer.SetEnabled(SL_BOOLEAN_FALSE);
        (void)virtualizer.SetStrength(DEFAULT_STRENGTH);

        available = true;
    }

    if (!available)
        return false;

    // call super method
    return BaseExtensionModule::onInstall(extmgr, token, user_arg);
}

void VirtualizerExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    CSLVirtualizerItf virtualizer;

    (void)extmgr->extGetInterfaceFromPlayer(&virtualizer);

    if (virtualizer) {
        OpenSLMediaPlayerVirtualizer::Settings defsettings;

        (void)MakeDefaultSettings(virtualizer, &defsettings);
        (void)virtualizer.SetEnabled(SL_BOOLEAN_FALSE);
        (void)SetProperties(virtualizer, &defsettings);
    }

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void VirtualizerExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                           const OpenSLMediaPlayerThreadMessage *msg) noexcept
{
    CSLVirtualizerItf virtualizer;

    SLresult slResult;
    PlayerState playerState;

    // NOTE:
    // The virtualizer interface of the output mix doesn't work on
    // KitKat (4.4), so we should not use extGetInterfaceFromOutputMix()
    // function here...

    extmgr->extGetInterfaceFromPlayer(&virtualizer);

    slResult = processMessage(virtualizer, msg);

    int result = extmgr->extTranslateOpenSLErrorCode(slResult);

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, result);
    }
}

SLresult VirtualizerExtModule::processMessage(CSLVirtualizerItf &virtualizer,
                                              const OpenSLMediaPlayerThreadMessage *msg) noexcept
{
    SLresult result = SL_RESULT_INTERNAL_ERROR;

    switch (msg->what) {
    case MSG_NOP: {
        LOCAL_ASSERT(false);
    } break;
    case MSG_SET_ENABLED: {
        typedef msg_blob_set_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLboolean slEnabled = CSLboolean::toSL(blob.enabled);
            result = virtualizer.SetEnabled(slEnabled);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_ENABLED: {
        typedef msg_blob_get_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLboolean slEnabled = SL_BOOLEAN_FALSE;
        result = virtualizer.IsEnabled(&slEnabled);

        if (IS_SL_SUCCESS(result)) {
            (*blob.enabled) = CSLboolean::toCpp(slEnabled);
        }
    } break;
    case MSG_GET_ID: {
        typedef msg_blob_get_id blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        (*blob.id) = 0;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_HAS_CONTROL: {
        typedef msg_blob_has_control blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.hasControl) = CHECK_IS_ACTIVE(blob);
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_GET_STRENGTH_SUPPORTED: {
        typedef msg_blob_get_strength_supported blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLboolean slSupported = SL_BOOLEAN_FALSE;
        result = virtualizer.IsStrengthSupported(&slSupported);

        if (IS_SL_SUCCESS(result)) {
            (*blob.supported) = CSLboolean::toCpp(slSupported);
        }
    } break;
    case MSG_GET_ROUNDED_STRENGTH: {
        typedef msg_blob_get_rounded_strength blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLpermille slRoundedStrength = 0;
        result = virtualizer.GetRoundedStrength(&slRoundedStrength);

        if (IS_SL_SUCCESS(result)) {
            (*blob.roundedStrength) = slRoundedStrength;
        }
    } break;
    case MSG_GET_PROPERTIES: {
        typedef msg_blob_get_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        result = GetProperties(virtualizer, blob.settings);
    } break;
    case MSG_SET_STRENGTH: {
        typedef msg_blob_set_strength blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLpermille slStrength = blob.strength;
            result = virtualizer.SetStrength(slStrength);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_PROPERTIES: {
        typedef msg_blob_set_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            result = SetProperties(virtualizer, blob.settings);
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

} // namespace oslmp
