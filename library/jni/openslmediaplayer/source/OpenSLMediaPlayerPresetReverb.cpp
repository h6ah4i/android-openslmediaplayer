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

// #define LOG_TAG "OpenSLMediaPlayerPresetReverb"

#include "oslmp/OpenSLMediaPlayerPresetReverb.hpp"

#include <cassert>

#include <cxxporthelper/memory>
#include <cxxporthelper/compiler.hpp>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayer.hpp>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/BaseExtensionModule.hpp"

//
// Constants
//
#define MODULE_NAME "PresetReverb"
#define AUX_EFFECT_ID OSLMP_AUX_EFFECT_PRESET_REVERB

//
// helper macros
//
#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_INTERFACE_FROM_OUTPUT_MIXER(interface)                                                                     \
    {                                                                                                                  \
        auxmgr_t auxmgr(auxmgr_.promote());                                                                            \
        if (CXXPH_UNLIKELY(!auxmgr.get())) {                                                                           \
            return OSLMP_RESULT_ILLEGAL_STATE;                                                                         \
        }                                                                                                              \
        auxmgr->auxGetInterfaceFromOutputMix(&interface);                                                              \
    }                                                                                                                  \
    if (CXXPH_UNLIKELY(!(interface))) {                                                                                \
        return OSLMP_RESULT_ILLEGAL_STATE;                                                                             \
    }

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    PresetReverbExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                             \
    if (CXXPH_UNLIKELY(!(varname))) {                                                                                  \
        return OSLMP_RESULT_DEAD_OBJECT;                                                                               \
    }

#define CHECK_ARG(cond)                                                                                                \
    if (CXXPH_UNLIKELY(!(cond))) {                                                                                     \
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;                                                                          \
    }

#define CHECK_IS_ACTIVE(blob_) (CXXPH_LIKELY(checkIsClientActive((blob_).client)))

#define IS_SL_SUCCESS(result) (CXXPH_LIKELY((result) == SL_RESULT_SUCCESS))

#define CHECK_IS_AUX_MGR_AVAILABLE()                                                                                   \
    if (CXXPH_UNLIKELY(!(auxmgr_.promote().get()))) {                                                                  \
        return OSLMP_RESULT_DEAD_OBJECT;                                                                               \
    }

namespace oslmp {

using namespace ::opensles;
using namespace ::oslmp::impl;

typedef OpenSLMediaPlayerInternalContext InternalContext;

typedef OpenSLMediaPlayerPresetReverb::Settings SLPresetReverbSettings;

struct PresetReverbStatus {
    SLboolean enabled;

    PresetReverbStatus() : enabled(SL_BOOLEAN_FALSE) {}
};

class PresetReverbExtModule : public BaseExtensionModule {
public:
    PresetReverbExtModule();
    virtual ~PresetReverbExtModule();

    int setEnabled(void *client, bool enabled) noexcept;
    int getEnabled(void *client, bool *enabled) noexcept;
    int getId(void *client, int32_t *id) noexcept;
    int hasControl(void *client, bool *hasControl) noexcept;
    int getPreset(void *client, uint16_t *preset) noexcept;
    int getProperties(void *client, OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept;
    int setPreset(void *client, uint16_t room) noexcept;
    int setProperties(void *client, const OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept override;

private:
    SLresult processMessage(OpenSLMediaPlayerExtensionManager *extmgr, CSLPresetReverbItf &reverb,
                            const OpenSLMediaPlayerThreadMessage *msg) noexcept;

    int onEnabledChanged(OpenSLMediaPlayerExtensionManager *extmgr, bool enabled) noexcept;

    static bool checkPresetNo(uint16_t preset) noexcept;

    static SLresult GetProperties(CSLPresetReverbItf &reverb,
                                  OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept;
    static SLresult SetProperties(CSLPresetReverbItf &reverb,
                                  const OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept;
    static SLresult MakeDefaultSettings(OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept;

private:
    PresetReverbStatus status_;
};

class PresetReverbExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    PresetReverbExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) PresetReverbExtModule();
    }
};

class OpenSLMediaPlayerPresetReverb::Impl {
public:
    Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client);
    ~Impl();

    android::sp<OpenSLMediaPlayerContext> context_;
    void *client_;
    PresetReverbExtModule *module_;
};

enum {
    MSG_NOP,
    MSG_SET_ENABLED,
    MSG_GET_ENABLED,
    MSG_GET_ID,
    MSG_HAS_CONTROL,
    MSG_GET_PRESET,
    MSG_GET_PROPERTIES,
    MSG_SET_PRESET,
    MSG_SET_PROPERTIES,
};

struct msg_blob_set_enabled {
    void *client;
    SLboolean enabled;
};

struct msg_blob_get_enabled {
    void *client;
    SLboolean *enabled;
};

struct msg_blob_get_id {
    void *client;
    int32_t *id;
};

struct msg_blob_has_control {
    void *client;
    bool *hasControl;
};

struct msg_blob_get_preset {
    void *client;
    SLuint16 *preset;
};

struct msg_blob_get_properties {
    void *client;
    OpenSLMediaPlayerPresetReverb::Settings *settings;
};

struct msg_blob_set_preset {
    void *client;
    SLuint16 preset;
};

struct msg_blob_set_properties {
    void *client;
    const OpenSLMediaPlayerPresetReverb::Settings *settings;
};

//
// OpenSLMediaPlayerPresetReverb
//

OpenSLMediaPlayerPresetReverb::OpenSLMediaPlayerPresetReverb(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(context, this))
{
}

OpenSLMediaPlayerPresetReverb::~OpenSLMediaPlayerPresetReverb()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerPresetReverb::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(this, enabled);
}

int OpenSLMediaPlayerPresetReverb::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(this, enabled);
}

int OpenSLMediaPlayerPresetReverb::getId(int32_t *id) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getId(this, id);
}

int OpenSLMediaPlayerPresetReverb::hasControl(bool *hasControl) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->hasControl(this, hasControl);
}

int OpenSLMediaPlayerPresetReverb::getPreset(uint16_t *preset) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getPreset(this, preset);
}

int OpenSLMediaPlayerPresetReverb::getProperties(OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getProperties(this, settings);
}

int OpenSLMediaPlayerPresetReverb::setPreset(uint16_t preset) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setPreset(this, preset);
}

int OpenSLMediaPlayerPresetReverb::setProperties(const OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setProperties(this, settings);
}

//
// OpenSLMediaPlayerPresetReverb::Impl
//
OpenSLMediaPlayerPresetReverb::Impl::Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client)
    : context_(context), client_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_PRESET_REVERB) {
        const PresetReverbExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module = nullptr;

        int result = c.extAttachOrInstall(&module, &creator, client);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<PresetReverbExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerPresetReverb::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(client_);

        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// PresetReverbExtModule
//

PresetReverbExtModule::PresetReverbExtModule() : BaseExtensionModule(MODULE_NAME), status_() {}

PresetReverbExtModule::~PresetReverbExtModule() {}

int PresetReverbExtModule::setEnabled(void *client, bool enabled) noexcept
{
    typedef msg_blob_set_enabled blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    Message msg(0, MSG_SET_ENABLED);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.enabled = CSLboolean::toSL(enabled);
    }

    return postAndWaitResult(&msg);
}

int PresetReverbExtModule::getEnabled(void *client, bool *enabled) noexcept
{
    typedef msg_blob_get_enabled blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(enabled != nullptr);

    Message msg(0, MSG_GET_ENABLED);
    SLboolean value = SL_BOOLEAN_FALSE;

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.enabled = &value;
    }

    const int result = postAndWaitResult(&msg);

    (*enabled) = CSLboolean::toCpp(value);

    return result;
}

int PresetReverbExtModule::getId(void *client, int32_t *id) noexcept
{
    typedef msg_blob_get_id blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(id != nullptr);

    Message msg(0, MSG_GET_ID);
    int32_t value = false;

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.id = &value;
    }

    const int result = postAndWaitResult(&msg);

    (*id) = value;

    return result;
}

int PresetReverbExtModule::hasControl(void *client, bool *hasControl) noexcept
{
    typedef msg_blob_has_control blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(hasControl != nullptr);

    Message msg(0, MSG_HAS_CONTROL);
    bool value = false;

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.hasControl = &value;
    }

    const int result = postAndWaitResult(&msg);

    (*hasControl) = value;

    return result;
}

int PresetReverbExtModule::getPreset(void *client, uint16_t *preset) noexcept
{
    typedef msg_blob_get_preset blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(preset != nullptr);

    Message msg(0, MSG_GET_PRESET);
    SLuint16 value = 0;

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.preset = &value;
    }

    const int result = postAndWaitResult(&msg);

    (*preset) = value;

    return result;
}

int PresetReverbExtModule::getProperties(void *client, OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept
{
    typedef msg_blob_get_properties blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(settings != nullptr);

    ::memset(settings, 0, sizeof(OpenSLMediaPlayerPresetReverb::Settings));

    Message msg(0, MSG_GET_PROPERTIES);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.settings = settings;
    }

    return postAndWaitResult(&msg);
}

int PresetReverbExtModule::setPreset(void *client, uint16_t preset) noexcept
{
    typedef msg_blob_set_preset blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkPresetNo(preset));

    Message msg(0, MSG_SET_PRESET);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.preset = preset;
    }

    return postAndWaitResult(&msg);
}

int PresetReverbExtModule::setProperties(void *client, const OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept
{
    typedef msg_blob_set_properties blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(settings != nullptr);
    CHECK_ARG(checkPresetNo(settings->preset));

    Message msg(0, MSG_SET_PROPERTIES);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.settings = settings;
    }

    return postAndWaitResult(&msg);
}

int PresetReverbExtModule::onEnabledChanged(OpenSLMediaPlayerExtensionManager *extmgr, bool enabled) noexcept
{
    return extmgr->extSetAuxEffectEnabled(AUX_EFFECT_ID, enabled);
}

bool PresetReverbExtModule::checkPresetNo(uint16_t preset) noexcept
{
    switch (preset) {
    case SL_REVERBPRESET_NONE:
    case SL_REVERBPRESET_SMALLROOM:
    case SL_REVERBPRESET_MEDIUMROOM:
    case SL_REVERBPRESET_LARGEROOM:
    case SL_REVERBPRESET_MEDIUMHALL:
    case SL_REVERBPRESET_LARGEHALL:
    case SL_REVERBPRESET_PLATE:
        return true;
    default:
        break;
    }

    return false;
}

bool PresetReverbExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                      void *user_arg) noexcept
{

    bool available = false;

    CSLPresetReverbItf reverb;

    if (IS_SL_SUCCESS(extmgr->extGetInterfaceFromOutputMix(&reverb))) {
        // acquired a preset reverb instance from the output mixer

        // apply default settings
        OpenSLMediaPlayerPresetReverb::Settings defsettings;
        MakeDefaultSettings(&defsettings);
        if (IS_SL_SUCCESS(reverb.SetPreset(defsettings.preset))) {
            available = true;
        }
    }

    if (!available)
        return false;

    // call super method
    return BaseExtensionModule::onInstall(extmgr, token, user_arg);
}

void PresetReverbExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    CSLPresetReverbItf reverb;

    extmgr->extSetAuxEffectEnabled(AUX_EFFECT_ID, false);

    (void)extmgr->extGetInterfaceFromOutputMix(&reverb);

    if (reverb) {
        OpenSLMediaPlayerPresetReverb::Settings defsettings;
        MakeDefaultSettings(&defsettings);
        (void)reverb.SetPreset(defsettings.preset);
    }

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void PresetReverbExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                            const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    CSLPresetReverbItf reverb;
    SLresult slResult;

    (void)extmgr->extGetInterfaceFromOutputMix(&reverb);

    slResult = processMessage(extmgr, reverb, msg);

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, extmgr->extTranslateOpenSLErrorCode(slResult));
    }
}

SLresult PresetReverbExtModule::processMessage(OpenSLMediaPlayerExtensionManager *extmgr, CSLPresetReverbItf &reverb,
                                               const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    SLresult result = SL_RESULT_INTERNAL_ERROR;
    PresetReverbStatus &status = status_;

    switch (msg->what) {
    case MSG_NOP: {
        LOCAL_ASSERT(false);
    } break;
    case MSG_SET_ENABLED: {
        typedef msg_blob_set_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            if (status.enabled != blob.enabled) {
                status.enabled = blob.enabled;
                result = onEnabledChanged(extmgr, status.enabled);
            }
            result = SL_RESULT_SUCCESS;
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_ENABLED: {
        typedef msg_blob_get_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        (*blob.enabled) = status.enabled;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_GET_ID: {
        typedef msg_blob_get_id blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        (*blob.id) = AUX_EFFECT_ID;
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_HAS_CONTROL: {
        typedef msg_blob_has_control blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        (*blob.hasControl) = CHECK_IS_ACTIVE(blob);
        result = SL_RESULT_SUCCESS;
    } break;
    case MSG_GET_PRESET: {
        typedef msg_blob_get_preset blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        result = reverb.GetPreset(blob.preset);
    } break;
    case MSG_GET_PROPERTIES: {
        typedef msg_blob_get_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        result = GetProperties(reverb, blob.settings);
    } break;
    case MSG_SET_PRESET: {
        typedef msg_blob_set_preset blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            result = reverb.SetPreset(blob.preset);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_PROPERTIES: {
        typedef msg_blob_set_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            result = SetProperties(reverb, blob.settings);
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

SLresult PresetReverbExtModule::GetProperties(CSLPresetReverbItf &reverb,
                                              OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept
{
    SLresult result;

    result = reverb.GetPreset(&(settings->preset));

    if (!IS_SL_SUCCESS(result))
        return result;

    return SL_RESULT_SUCCESS;
}

SLresult PresetReverbExtModule::SetProperties(CSLPresetReverbItf &reverb,
                                              const OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept
{
    SLresult result;

    if (!checkPresetNo(settings->preset))
        return SL_RESULT_PARAMETER_INVALID;

    result = reverb.SetPreset(settings->preset);

    if (!IS_SL_SUCCESS(result))
        return result;

    return SL_RESULT_SUCCESS;
}

SLresult PresetReverbExtModule::MakeDefaultSettings(OpenSLMediaPlayerPresetReverb::Settings *settings) noexcept
{
    settings->preset = SL_REVERBPRESET_NONE;

    return SL_RESULT_SUCCESS;
}

} // namespace oslmp
