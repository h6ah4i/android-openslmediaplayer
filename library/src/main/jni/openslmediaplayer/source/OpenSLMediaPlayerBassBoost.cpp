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

// #define LOG_TAG "OpenSLMediaPlayerBassBoost"

#include "oslmp/OpenSLMediaPlayerBassBoost.hpp"

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
#define MODULE_NAME "BassBoost"

#define STRENGTH_MIN 0
#define STRENGTH_MAX 1000
#define DEFAULT_STRENGTH 0

//
// helper macros
//

#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    BassBoostExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                                \
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

class BassBoostExtModule : public BaseExtensionModule {
public:
    BassBoostExtModule();
    virtual ~BassBoostExtModule();

    int setEnabled(void *client, bool enabled) noexcept;
    int getEnabled(void *client, bool *enabled) noexcept;
    int getId(void *client, int *id) noexcept;
    int hasControl(void *client, bool *hasControl) noexcept;
    int getStrengthSupported(void *client, bool *strengthSupported) noexcept;
    int getRoundedStrength(void *client, int16_t *roundedStrength) noexcept;
    int getProperties(void *client, OpenSLMediaPlayerBassBoost::Settings *settings) noexcept;
    int setStrength(void *client, int16_t strength) noexcept;
    int setProperties(void *client, const OpenSLMediaPlayerBassBoost::Settings *settings) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept override;

private:
    SLresult processMessage(CSLBassBoostItf &bassboost, const OpenSLMediaPlayerThreadMessage *msg) noexcept;
};

class BassBoostExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    BassBoostExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) BassBoostExtModule();
    }
};

class OpenSLMediaPlayerBassBoost::Impl {
public:
    Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client);
    ~Impl();

    android::sp<OpenSLMediaPlayerContext> context_;
    void *client_;
    BassBoostExtModule *module_;
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
    OpenSLMediaPlayerBassBoost::Settings *settings;
};

struct msg_blob_set_strength {
    void *client;
    int16_t strength;
};

struct msg_blob_set_properties {
    void *client;
    const OpenSLMediaPlayerBassBoost::Settings *settings;
};

//
// OpenSLMediaPlayerBassBoost
//
OpenSLMediaPlayerBassBoost::OpenSLMediaPlayerBassBoost(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(context, this))
{
}

OpenSLMediaPlayerBassBoost::~OpenSLMediaPlayerBassBoost()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerBassBoost::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(this, enabled);
}

int OpenSLMediaPlayerBassBoost::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(this, enabled);
}

int OpenSLMediaPlayerBassBoost::getId(int *id) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getId(this, id);
}

int OpenSLMediaPlayerBassBoost::hasControl(bool *hasControl) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->hasControl(this, hasControl);
}

int OpenSLMediaPlayerBassBoost::getStrengthSupported(bool *strengthSupported) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getStrengthSupported(this, strengthSupported);
}

int OpenSLMediaPlayerBassBoost::getRoundedStrength(int16_t *roundedStrength) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getRoundedStrength(this, roundedStrength);
}

int OpenSLMediaPlayerBassBoost::getProperties(OpenSLMediaPlayerBassBoost::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getProperties(this, settings);
}

int OpenSLMediaPlayerBassBoost::setStrength(int16_t strength) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setStrength(this, strength);
}

int OpenSLMediaPlayerBassBoost::setProperties(const OpenSLMediaPlayerBassBoost::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setProperties(this, settings);
}

//
// OpenSLMediaPlayerBassBoost::Impl
//
OpenSLMediaPlayerBassBoost::Impl::Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client)
    : context_(context), client_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_BASSBOOST) {
        const BassBoostExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module = nullptr;

        int result = c.extAttachOrInstall(&module, &creator, client);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<BassBoostExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerBassBoost::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(client_);

        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// Utilities
//

static inline SLresult GetProperties(CSLBassBoostItf &bassboost,
                                     OpenSLMediaPlayerBassBoost::Settings *settings) noexcept
{
    SLresult result;

    result = bassboost.GetRoundedStrength(&(settings->strength));

    if (!IS_SL_SUCCESS(result))
        return result;

    return SL_RESULT_SUCCESS;
}

static inline SLresult SetProperties(CSLBassBoostItf &bassboost,
                                     const OpenSLMediaPlayerBassBoost::Settings *settings) noexcept
{
    SLresult result;

    result = bassboost.SetStrength(settings->strength);

    if (!IS_SL_SUCCESS(result))
        return result;

    return SL_RESULT_SUCCESS;
}

static inline SLresult MakeDefaultSettings(CSLBassBoostItf &bassboost,
                                           OpenSLMediaPlayerBassBoost::Settings *settings) noexcept
{
    settings->strength = 0;

    return SL_RESULT_SUCCESS;
}

//
// BassBoostExtModule
//
BassBoostExtModule::BassBoostExtModule() : BaseExtensionModule(MODULE_NAME) {}

BassBoostExtModule::~BassBoostExtModule() {}

int BassBoostExtModule::setEnabled(void *client, bool enabled) noexcept
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

int BassBoostExtModule::getEnabled(void *client, bool *enabled) noexcept
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

int BassBoostExtModule::getId(void *client, int *id) noexcept
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

int BassBoostExtModule::hasControl(void *client, bool *hasControl) noexcept
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

int BassBoostExtModule::getStrengthSupported(void *client, bool *strengthSupported) noexcept
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

int BassBoostExtModule::getRoundedStrength(void *client, int16_t *roundedStrength) noexcept
{
    typedef msg_blob_get_rounded_strength blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(roundedStrength != nullptr);

    (*roundedStrength) = 0;

    Message msg(0, MSG_GET_ROUNDED_STRENGTH);
    SLpermille value = 0;

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.roundedStrength = roundedStrength;
    }

    return postAndWaitResult(&msg);
}

int BassBoostExtModule::getProperties(void *client, OpenSLMediaPlayerBassBoost::Settings *settings) noexcept
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

int BassBoostExtModule::setStrength(void *client, int16_t strength) noexcept
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

int BassBoostExtModule::setProperties(void *client, const OpenSLMediaPlayerBassBoost::Settings *settings) noexcept
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

bool BassBoostExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                   void *user_arg) noexcept
{

    bool available = false;

    CSLBassBoostItf bassboost;

    if (IS_SL_SUCCESS(extmgr->extGetInterfaceFromPlayer(&bassboost, true))) {
        // acquired a bassboost instance from the current player

        // apply default settings
        (void)bassboost.SetEnabled(SL_BOOLEAN_FALSE);
        (void)bassboost.SetStrength(DEFAULT_STRENGTH);

        available = true;
    }

    if (!available)
        return false;

    // call super method
    return BaseExtensionModule::onInstall(extmgr, token, user_arg);
}

void BassBoostExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    CSLBassBoostItf bassboost;

    (void)extmgr->extGetInterfaceFromPlayer(&bassboost);

    if (bassboost) {
        OpenSLMediaPlayerBassBoost::Settings defsettings;

        (void)MakeDefaultSettings(bassboost, &defsettings);
        (void)bassboost.SetEnabled(SL_BOOLEAN_FALSE);
        (void)SetProperties(bassboost, &defsettings);
    }

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void BassBoostExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                         const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    // NOTE:
    // The bassboost interface of the output mix doesn't work on
    // KitKat (4.4), so we should not use extGetInterfaceFromOutputMix()
    // function here...

    CSLBassBoostItf bassboost;
    SLresult slResult;

    (void)extmgr->extGetInterfaceFromPlayer(&bassboost);

    slResult = processMessage(bassboost, msg);

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, extmgr->extTranslateOpenSLErrorCode(slResult));
    }
}

SLresult BassBoostExtModule::processMessage(CSLBassBoostItf &bassboost,
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
            result = bassboost.SetEnabled(slEnabled);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_ENABLED: {
        typedef msg_blob_get_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLboolean slEnabled = SL_BOOLEAN_FALSE;
        result = bassboost.IsEnabled(&slEnabled);

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
        result = bassboost.IsStrengthSupported(&slSupported);

        if (IS_SL_SUCCESS(result)) {
            (*blob.supported) = CSLboolean::toCpp(slSupported);
        }
    } break;
    case MSG_GET_ROUNDED_STRENGTH: {
        typedef msg_blob_get_rounded_strength blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLpermille slRoundedStrength = 0;
        result = bassboost.GetRoundedStrength(&slRoundedStrength);

        if (IS_SL_SUCCESS(result)) {
            (*blob.roundedStrength) = slRoundedStrength;
        }
    } break;
    case MSG_GET_PROPERTIES: {
        typedef msg_blob_get_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        result = GetProperties(bassboost, blob.settings);
    } break;
    case MSG_SET_STRENGTH: {
        typedef msg_blob_set_strength blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLpermille slStrength = blob.strength;
            result = bassboost.SetStrength(slStrength);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_PROPERTIES: {
        typedef msg_blob_set_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            result = SetProperties(bassboost, blob.settings);
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
