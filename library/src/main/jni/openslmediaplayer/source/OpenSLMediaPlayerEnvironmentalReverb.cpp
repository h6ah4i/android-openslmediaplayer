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

// #define LOG_TAG "OpenSLMediaPlayerEnvironmentalReverb"

#include "oslmp/OpenSLMediaPlayerEnvironmentalReverb.hpp"

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
#define MODULE_NAME "EnvironmentalReverb"
#define AUX_EFFECT_ID OSLMP_AUX_EFFECT_ENVIRONMENTAL_REVERB

#define ROOM_LEVEL_MIN -9000
#define ROOM_LEVEL_MAX 0
#define ROOM_HF_LEVEL_MIN -9000
#define ROOM_HF_LEVEL_MAX 0
#define DECAY_HF_RATIO_MIN 100
#define DECAY_HF_RATIO_MAX 2000
#define DECAY_TIME_MIN 100
#define DECAY_TIME_MAX 7000     /* Spec.: 20000, Actually(LVREV_MAX_T60): 7000 */
#define REFLECTIONS_LEVEL_MIN 0 /* Spec.: -9000, Actually: 0 (not implemented yet) */
#define REFLECTIONS_LEVEL_MAX 0 /* Spec.: 1000, Actually: 0 (not implemented yet) */
#define REFLECTIONS_DELAY_MIN 0 /* Spec.: 0, Actually: 0 (not implemented yet) */
#define REFLECTIONS_DELAY_MAX 0 /* Spec.: 300, Actually: 0 (not implemented yet) */
#define REVERB_LEVEL_MIN -9000
#define REVERB_LEVEL_MAX 2000
#define REVERB_DELAY_MIN 0 /* Spec.: 0, Actually: 0 (not implemented yet) */
#define REVERB_DELAY_MAX 0 /* Spec.: 100, Actually: 0 (not implemented yet) */
#define DIFFUSION_MIN 0
#define DIFFUSION_MAX 1000
#define DENSITY_MIN 0
#define DENSITY_MAX 1000

//
// helper macros
//
#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    EnvironmentalReverbExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                      \
    if (CXXPH_UNLIKELY(!(varname))) {                                                                                  \
        return OSLMP_RESULT_DEAD_OBJECT;                                                                               \
    }

#define CHECK_ARG(cond)                                                                                                \
    if (CXXPH_UNLIKELY(!(cond))) {                                                                                     \
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;                                                                          \
    }

#define CHECK_IS_ACTIVE(blob_) (CXXPH_LIKELY(checkIsClientActive((blob_).client)))

#define IS_SL_SUCCESS(result) (CXXPH_LIKELY(result == SL_RESULT_SUCCESS))

#define CHECK_RANGE(value, min, max) (CXXPH_LIKELY(((value) >= (min)) && ((value) <= (max))))

namespace oslmp {

using namespace ::opensles;
using namespace ::oslmp::impl;

typedef OpenSLMediaPlayerInternalContext InternalContext;

struct EnvironmentalReverbStatus {
    bool enabled;

    EnvironmentalReverbStatus() : enabled(false) {}
};

class EnvironmentalReverbExtModule : public BaseExtensionModule {
public:
    EnvironmentalReverbExtModule();
    virtual ~EnvironmentalReverbExtModule();

    int setEnabled(void *client, bool enabled) noexcept;
    int getEnabled(void *client, bool *enabled) noexcept;
    int getId(void *client, int32_t *id) noexcept;
    int hasControl(void *client, bool *hasControl) noexcept;
    int getDecayHFRatio(void *client, int16_t *decayHFRatio) noexcept;
    int getDecayTime(void *client, uint32_t *decayTime) noexcept;
    int getDensity(void *client, int16_t *density) noexcept;
    int getDiffusion(void *client, int16_t *diffusion) noexcept;
    int getReflectionsDelay(void *client, uint32_t *reflectionsDelay) noexcept;
    int getReflectionsLevel(void *client, int16_t *reflectionsLevel) noexcept;
    int getReverbDelay(void *client, uint32_t *reverbDelay) noexcept;
    int getReverbLevel(void *client, int16_t *reverbLevel) noexcept;
    int getRoomHFLevel(void *client, int16_t *roomHF) noexcept;
    int getRoomLevel(void *client, int16_t *room) noexcept;
    int getProperties(void *client, OpenSLMediaPlayerEnvironmentalReverb::Settings *settings) noexcept;
    int setDecayHFRatio(void *client, int16_t decayHFRatio) noexcept;
    int setDecayTime(void *client, uint32_t decayTime) noexcept;
    int setDensity(void *client, int16_t density) noexcept;
    int setDiffusion(void *client, int16_t diffusion) noexcept;
    int setReflectionsDelay(void *client, uint32_t reflectionsDelay) noexcept;
    int setReflectionsLevel(void *client, int16_t reflectionsLevel) noexcept;
    int setReverbDelay(void *client, uint32_t reverbDelay) noexcept;
    int setReverbLevel(void *client, int16_t reverbLevel) noexcept;
    int setRoomHFLevel(void *client, int16_t roomHF) noexcept;
    int setRoomLevel(void *client, int16_t room) noexcept;
    int setProperties(void *client, const OpenSLMediaPlayerEnvironmentalReverb::Settings *settings) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept override;

private:
    SLresult processMessage(OpenSLMediaPlayerExtensionManager *extmgr, CSLEnvironmentalReverbItf &reverb,
                            const OpenSLMediaPlayerThreadMessage *msg) noexcept;

    int onEnabledChanged(OpenSLMediaPlayerExtensionManager *extmgr, bool enabled) noexcept;

private:
    EnvironmentalReverbStatus status_;
};

class EnvironmentalReverbExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    EnvironmentalReverbExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) EnvironmentalReverbExtModule();
    }
};

class OpenSLMediaPlayerEnvironmentalReverb::Impl {
public:
    Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client);
    ~Impl();

    android::sp<OpenSLMediaPlayerContext> context_;
    void *client_;
    EnvironmentalReverbExtModule *module_;
};

enum {
    MSG_NOP,
    MSG_SET_ENABLED,
    MSG_GET_ENABLED,
    MSG_GET_ID,
    MSG_HAS_CONTROL,
    MSG_GET_DECAY_HF_RATIO,
    MSG_GET_DECAY_TIME,
    MSG_GET_DENSITY,
    MSG_GET_DIFFUSION,
    MSG_GET_REFLECTIONS_DELAY,
    MSG_GET_REFLECTIONS_LEVEL,
    MSG_GET_REVERB_DELAY,
    MSG_GET_REVERB_LEVEL,
    MSG_GET_ROOM_HF_LEVEL,
    MSG_GET_ROOM_LEVEL,
    MSG_GET_PROPERTIES,
    MSG_SET_DECAY_HF_RATIO,
    MSG_SET_DECAY_TIME,
    MSG_SET_DENSITY,
    MSG_SET_DIFFUSION,
    MSG_SET_REFLECTIONS_DELAY,
    MSG_SET_REFLECTIONS_LEVEL,
    MSG_SET_REVERB_DELAY,
    MSG_SET_REVERB_LEVEL,
    MSG_SET_ROOM_HF_LEVEL,
    MSG_SET_ROOM_LEVEL,
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

struct msg_blob_get_decay_hf_ratio {
    void *client;
    int16_t *decayHFRatio;
};

struct msg_blob_get_decay_time {
    void *client;
    uint32_t *decayTime;
};

struct msg_blob_get_density {
    void *client;
    int16_t *density;
};

struct msg_blob_get_diffusion {
    void *client;
    int16_t *diffusion;
};

struct msg_blob_get_reflections_delay {
    void *client;
    uint32_t *reflectionsDelay;
};

struct msg_blob_get_reflections_level {
    void *client;
    int16_t *reflectionsLevel;
};

struct msg_blob_get_reverb_delay {
    void *client;
    uint32_t *reverbDelay;
};

struct msg_blob_get_reverb_level {
    void *client;
    int16_t *reverbLevel;
};

struct msg_blob_get_room_hf_level {
    void *client;
    int16_t *roomHF;
};

struct msg_blob_get_room_level {
    void *client;
    int16_t *room;
};

struct msg_blob_get_properties {
    void *client;
    OpenSLMediaPlayerEnvironmentalReverb::Settings *settings;
};

struct msg_blob_set_decay_hf_ratio {
    void *client;
    int16_t decayHFRatio;
};

struct msg_blob_set_decay_time {
    void *client;
    uint32_t decayTime;
};

struct msg_blob_set_density {
    void *client;
    int16_t density;
};

struct msg_blob_set_diffusion {
    void *client;
    int16_t diffusion;
};

struct msg_blob_set_reflections_delay {
    void *client;
    uint32_t reflectionsDelay;
};

struct msg_blob_set_reflections_level {
    void *client;
    int16_t reflectionsLevel;
};

struct msg_blob_set_reverb_delay {
    void *client;
    uint32_t reverbDelay;
};

struct msg_blob_set_reverb_level {
    void *client;
    int16_t reverbLevel;
};

struct msg_blob_set_room_hf_level {
    void *client;
    int16_t roomHF;
};

struct msg_blob_set_room_level {
    void *client;
    int16_t room;
};

struct msg_blob_set_properties {
    void *client;
    const OpenSLMediaPlayerEnvironmentalReverb::Settings *settings;
};

//
// Utilities
//
static inline bool checkRoomLevelValue(SLmillibel room) noexcept
{
    return CHECK_RANGE(room, ROOM_LEVEL_MIN, ROOM_LEVEL_MAX);
}

static inline bool checkRoomHFLevelValue(SLmillibel roomHF) noexcept
{
    return CHECK_RANGE(roomHF, ROOM_HF_LEVEL_MIN, ROOM_HF_LEVEL_MAX);
}

static inline bool checkDecayHFRatioValue(SLpermille decayHFRatio) noexcept
{
    return CHECK_RANGE(decayHFRatio, DECAY_HF_RATIO_MIN, DECAY_HF_RATIO_MAX);
}

static inline bool checkDecayTimeValue(SLmillisecond decayTime) noexcept
{
    return CHECK_RANGE(decayTime, DECAY_TIME_MIN, DECAY_TIME_MAX);
}

static inline bool checkReflectionLevelValue(SLmillibel reflectionLevel) noexcept
{
    return CHECK_RANGE(reflectionLevel, REFLECTIONS_LEVEL_MIN, REFLECTIONS_LEVEL_MAX);
}

static inline bool checkReflectionDelayValue(SLmillisecond reflectionDelay) noexcept
{
    return CHECK_RANGE(reflectionDelay, REFLECTIONS_DELAY_MIN, REFLECTIONS_DELAY_MAX);
}

static inline bool checkReverbLevelValue(SLmillibel reverbLevel) noexcept
{
    return CHECK_RANGE(reverbLevel, REVERB_LEVEL_MIN, REVERB_LEVEL_MAX);
}

static inline bool checkReverbDelayValue(SLmillisecond reverbDelay) noexcept
{
    return CHECK_RANGE(reverbDelay, REVERB_DELAY_MIN, REVERB_DELAY_MAX);
}

static inline bool checkDiffusionValue(SLpermille diffusion) noexcept
{
    return CHECK_RANGE(diffusion, DIFFUSION_MIN, DIFFUSION_MAX);
}

static inline bool checkDensityValue(SLpermille density) noexcept
{
    return CHECK_RANGE(density, DENSITY_MIN, DENSITY_MAX);
}

static inline bool checkSettings(const OpenSLMediaPlayerEnvironmentalReverb::Settings *settings) noexcept
{
    return checkRoomLevelValue(settings->roomLevel) && checkRoomHFLevelValue(settings->roomHFLevel) &&
           checkDecayTimeValue(settings->decayTime) && checkDecayHFRatioValue(settings->decayHFRatio) &&
           checkReflectionLevelValue(settings->reflectionsLevel) &&
           checkReflectionDelayValue(settings->reflectionsDelay) && checkReverbLevelValue(settings->reverbLevel) &&
           checkReverbDelayValue(settings->reverbDelay) && checkDiffusionValue(settings->diffusion) &&
           checkDensityValue(settings->density);
}

static inline void MakeDefaultSettings(SLEnvironmentalReverbSettings *settings) noexcept
{
    settings->roomLevel = -6000;
    settings->roomHFLevel = 0;
    settings->decayTime = 1490;
    settings->decayHFRatio = 420;
    settings->reflectionsLevel = 0;
    settings->reflectionsDelay = 0;
    settings->reverbLevel = -6000;
    settings->reverbDelay = 0;
    settings->diffusion = 1000;
    settings->density = 1000;
}

static inline SLEnvironmentalReverbSettings
toSLSettings(const OpenSLMediaPlayerEnvironmentalReverb::Settings *settings) noexcept
{
    SLEnvironmentalReverbSettings slSettings;

    slSettings.roomLevel = settings->roomLevel;
    slSettings.roomHFLevel = settings->roomHFLevel;
    slSettings.decayTime = settings->decayTime;
    slSettings.decayHFRatio = settings->decayHFRatio;
    slSettings.reflectionsLevel = settings->reflectionsLevel;
    slSettings.reflectionsDelay = settings->reflectionsDelay;
    slSettings.reverbLevel = settings->reverbLevel;
    slSettings.reverbDelay = settings->reverbDelay;
    slSettings.diffusion = settings->diffusion;
    slSettings.density = settings->density;

    return slSettings;
}

static inline OpenSLMediaPlayerEnvironmentalReverb::Settings
fromSLSettings(const SLEnvironmentalReverbSettings *slSettings) noexcept
{
    OpenSLMediaPlayerEnvironmentalReverb::Settings settings;

    settings.roomLevel = slSettings->roomLevel;
    settings.roomHFLevel = slSettings->roomHFLevel;
    settings.decayTime = slSettings->decayTime;
    settings.decayHFRatio = slSettings->decayHFRatio;
    settings.reflectionsLevel = slSettings->reflectionsLevel;
    settings.reflectionsDelay = slSettings->reflectionsDelay;
    settings.reverbLevel = slSettings->reverbLevel;
    settings.reverbDelay = slSettings->reverbDelay;
    settings.diffusion = slSettings->diffusion;
    settings.density = slSettings->density;

    return settings;
}

//
// OpenSLMediaPlayerEnvironmentalReverb
//

OpenSLMediaPlayerEnvironmentalReverb::OpenSLMediaPlayerEnvironmentalReverb(
    const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(context, this))
{
}

OpenSLMediaPlayerEnvironmentalReverb::~OpenSLMediaPlayerEnvironmentalReverb()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerEnvironmentalReverb::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(this, enabled);
}

int OpenSLMediaPlayerEnvironmentalReverb::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(this, enabled);
}

int OpenSLMediaPlayerEnvironmentalReverb::getId(int32_t *id) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getId(this, id);
}

int OpenSLMediaPlayerEnvironmentalReverb::hasControl(bool *hasControl) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->hasControl(this, hasControl);
}

int OpenSLMediaPlayerEnvironmentalReverb::getDecayHFRatio(int16_t *decayHFRatio) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getDecayHFRatio(this, decayHFRatio);
}

int OpenSLMediaPlayerEnvironmentalReverb::getDecayTime(uint32_t *decayTime) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getDecayTime(this, decayTime);
}

int OpenSLMediaPlayerEnvironmentalReverb::getDensity(int16_t *density) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getDensity(this, density);
}

int OpenSLMediaPlayerEnvironmentalReverb::getDiffusion(int16_t *diffusion) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getDiffusion(this, diffusion);
}

int OpenSLMediaPlayerEnvironmentalReverb::getReflectionsDelay(uint32_t *reflectionsDelay) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getReflectionsDelay(this, reflectionsDelay);
}

int OpenSLMediaPlayerEnvironmentalReverb::getReflectionsLevel(int16_t *reflectionsLevel) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getReflectionsLevel(this, reflectionsLevel);
}

int OpenSLMediaPlayerEnvironmentalReverb::getReverbDelay(uint32_t *reverbDelay) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getReverbDelay(this, reverbDelay);
}

int OpenSLMediaPlayerEnvironmentalReverb::getReverbLevel(int16_t *reverbLevel) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getReverbLevel(this, reverbLevel);
}

int OpenSLMediaPlayerEnvironmentalReverb::getRoomHFLevel(int16_t *roomHF) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getRoomHFLevel(this, roomHF);
}

int OpenSLMediaPlayerEnvironmentalReverb::getRoomLevel(int16_t *room) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getRoomLevel(this, room);
}

int
OpenSLMediaPlayerEnvironmentalReverb::getProperties(OpenSLMediaPlayerEnvironmentalReverb::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getProperties(this, settings);
}

int OpenSLMediaPlayerEnvironmentalReverb::setDecayHFRatio(int16_t decayHFRatio) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setDecayHFRatio(this, decayHFRatio);
}

int OpenSLMediaPlayerEnvironmentalReverb::setDecayTime(uint32_t decayTime) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setDecayTime(this, decayTime);
}

int OpenSLMediaPlayerEnvironmentalReverb::setDensity(int16_t density) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setDensity(this, density);
}

int OpenSLMediaPlayerEnvironmentalReverb::setDiffusion(int16_t diffusion) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setDiffusion(this, diffusion);
}

int OpenSLMediaPlayerEnvironmentalReverb::setReflectionsDelay(uint32_t reflectionsDelay) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setReflectionsDelay(this, reflectionsDelay);
}

int OpenSLMediaPlayerEnvironmentalReverb::setReflectionsLevel(int16_t reflectionsLevel) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setReflectionsLevel(this, reflectionsLevel);
}

int OpenSLMediaPlayerEnvironmentalReverb::setReverbDelay(uint32_t reverbDelay) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setReverbDelay(this, reverbDelay);
}

int OpenSLMediaPlayerEnvironmentalReverb::setReverbLevel(int16_t reverbLevel) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setReverbLevel(this, reverbLevel);
}

int OpenSLMediaPlayerEnvironmentalReverb::setRoomHFLevel(int16_t roomHF) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setRoomHFLevel(this, roomHF);
}

int OpenSLMediaPlayerEnvironmentalReverb::setRoomLevel(int16_t room) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setRoomLevel(this, room);
}

int OpenSLMediaPlayerEnvironmentalReverb::setProperties(
    const OpenSLMediaPlayerEnvironmentalReverb::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setProperties(this, settings);
}

//
// OpenSLMediaPlayerEnvironmentalReverb::Impl
//
OpenSLMediaPlayerEnvironmentalReverb::Impl::Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client)
    : context_(context), client_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_ENVIRONMENAL_REVERB) {
        const EnvironmentalReverbExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module = nullptr;

        int result = c.extAttachOrInstall(&module, &creator, client);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<EnvironmentalReverbExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerEnvironmentalReverb::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(client_);

        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// EnvironmentalReverbExtModule
//

EnvironmentalReverbExtModule::EnvironmentalReverbExtModule() : BaseExtensionModule(MODULE_NAME), status_() {}

EnvironmentalReverbExtModule::~EnvironmentalReverbExtModule() {}

int EnvironmentalReverbExtModule::setEnabled(void *client, bool enabled) noexcept
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

int EnvironmentalReverbExtModule::getEnabled(void *client, bool *enabled) noexcept
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

int EnvironmentalReverbExtModule::getId(void *client, int32_t *id) noexcept
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

int EnvironmentalReverbExtModule::hasControl(void *client, bool *hasControl) noexcept
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

int EnvironmentalReverbExtModule::getDecayHFRatio(void *client, int16_t *decayHFRatio) noexcept
{
    typedef msg_blob_get_decay_hf_ratio blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(decayHFRatio != nullptr);

    (*decayHFRatio) = 0;

    Message msg(0, MSG_GET_DECAY_HF_RATIO);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.decayHFRatio = decayHFRatio;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getDecayTime(void *client, uint32_t *decayTime) noexcept
{
    typedef msg_blob_get_decay_time blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(decayTime != nullptr);

    (*decayTime) = 0;

    Message msg(0, MSG_GET_DECAY_TIME);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.decayTime = decayTime;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getDensity(void *client, int16_t *density) noexcept
{
    typedef msg_blob_get_density blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(density != nullptr);

    (*density) = 0;

    Message msg(0, MSG_GET_DENSITY);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.density = density;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getDiffusion(void *client, int16_t *diffusion) noexcept
{
    typedef msg_blob_get_diffusion blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(diffusion != nullptr);

    (*diffusion) = 0;

    Message msg(0, MSG_GET_DIFFUSION);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.diffusion = diffusion;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getReflectionsDelay(void *client, uint32_t *reflectionsDelay) noexcept
{
    typedef msg_blob_get_reflections_delay blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(reflectionsDelay != nullptr);

    (*reflectionsDelay) = 0;

    Message msg(0, MSG_GET_REFLECTIONS_DELAY);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.reflectionsDelay = reflectionsDelay;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getReflectionsLevel(void *client, int16_t *reflectionsLevel) noexcept
{
    typedef msg_blob_get_reflections_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(reflectionsLevel != nullptr);

    (*reflectionsLevel) = 0;

    Message msg(0, MSG_GET_REFLECTIONS_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.reflectionsLevel = reflectionsLevel;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getReverbDelay(void *client, uint32_t *reverbDelay) noexcept
{
    typedef msg_blob_get_reverb_delay blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(reverbDelay != nullptr);

    (*reverbDelay) = 0;

    Message msg(0, MSG_GET_REVERB_DELAY);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.reverbDelay = reverbDelay;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getReverbLevel(void *client, int16_t *reverbLevel) noexcept
{
    typedef msg_blob_get_reverb_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(reverbLevel != nullptr);

    (*reverbLevel) = 0;

    Message msg(0, MSG_GET_REVERB_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.reverbLevel = reverbLevel;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getRoomHFLevel(void *client, int16_t *roomHF) noexcept
{
    typedef msg_blob_get_room_hf_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(roomHF != nullptr);

    (*roomHF) = 0;

    Message msg(0, MSG_GET_ROOM_HF_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.roomHF = roomHF;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getRoomLevel(void *client, int16_t *room) noexcept
{
    typedef msg_blob_get_room_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(room != nullptr);

    (*room) = 0;

    Message msg(0, MSG_GET_ROOM_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.room = room;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::getProperties(void *client,
                                                OpenSLMediaPlayerEnvironmentalReverb::Settings *settings) noexcept
{
    typedef msg_blob_get_properties blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(settings != nullptr);

    ::memset(settings, 0, sizeof(OpenSLMediaPlayerEnvironmentalReverb::Settings));

    Message msg(0, MSG_GET_PROPERTIES);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.settings = settings;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setDecayHFRatio(void *client, int16_t decayHFRatio) noexcept
{
    typedef msg_blob_set_decay_hf_ratio blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkDecayHFRatioValue(decayHFRatio));

    Message msg(0, MSG_SET_DECAY_HF_RATIO);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.decayHFRatio = decayHFRatio;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setDecayTime(void *client, uint32_t decayTime) noexcept
{
    typedef msg_blob_set_decay_time blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkDecayTimeValue(decayTime));

    Message msg(0, MSG_SET_DECAY_TIME);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.decayTime = decayTime;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setDensity(void *client, int16_t density) noexcept
{
    typedef msg_blob_set_density blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkDensityValue(density));

    Message msg(0, MSG_SET_DENSITY);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.density = density;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setDiffusion(void *client, int16_t diffusion) noexcept
{
    typedef msg_blob_set_diffusion blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkDiffusionValue(diffusion));

    Message msg(0, MSG_SET_DIFFUSION);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.diffusion = diffusion;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setReflectionsDelay(void *client, uint32_t reflectionsDelay) noexcept
{
    typedef msg_blob_set_reflections_delay blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkReflectionDelayValue(reflectionsDelay));

    Message msg(0, MSG_SET_REFLECTIONS_DELAY);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.reflectionsDelay = reflectionsDelay;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setReflectionsLevel(void *client, int16_t reflectionsLevel) noexcept
{
    typedef msg_blob_set_reflections_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkReflectionLevelValue(reflectionsLevel));

    Message msg(0, MSG_SET_REFLECTIONS_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.reflectionsLevel = reflectionsLevel;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setReverbDelay(void *client, uint32_t reverbDelay) noexcept
{
    typedef msg_blob_set_reverb_delay blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkReverbDelayValue(reverbDelay));

    Message msg(0, MSG_SET_REVERB_DELAY);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.reverbDelay = reverbDelay;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setReverbLevel(void *client, int16_t reverbLevel) noexcept
{
    typedef msg_blob_set_reverb_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkReverbLevelValue(reverbLevel));

    Message msg(0, MSG_SET_REVERB_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.reverbLevel = reverbLevel;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setRoomHFLevel(void *client, int16_t roomHF) noexcept
{
    typedef msg_blob_set_room_hf_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkRoomHFLevelValue(roomHF));

    Message msg(0, MSG_SET_ROOM_HF_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.roomHF = roomHF;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setRoomLevel(void *client, int16_t room) noexcept
{
    typedef msg_blob_set_room_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(checkRoomLevelValue(room));

    Message msg(0, MSG_SET_ROOM_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.room = room;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::setProperties(void *client,
                                                const OpenSLMediaPlayerEnvironmentalReverb::Settings *settings) noexcept
{
    typedef msg_blob_set_properties blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(settings != nullptr);
    CHECK_ARG(checkSettings(settings));

    Message msg(0, MSG_SET_PROPERTIES);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.settings = settings;
    }

    return postAndWaitResult(&msg);
}

int EnvironmentalReverbExtModule::onEnabledChanged(OpenSLMediaPlayerExtensionManager *extmgr, bool enabled) noexcept
{
    return extmgr->extSetAuxEffectEnabled(AUX_EFFECT_ID, enabled);
}

bool EnvironmentalReverbExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr,
                                             OpenSLMediaPlayerExtensionToken token, void *user_arg) noexcept
{

    bool available = false;

    CSLEnvironmentalReverbItf reverb;

    if (IS_SL_SUCCESS(extmgr->extGetInterfaceFromOutputMix(&reverb))) {
        // acquired a environmental reverb instance from the output mixer

        // apply default settings
        SLEnvironmentalReverbSettings defsettings;
        MakeDefaultSettings(&defsettings);

        if (IS_SL_SUCCESS(reverb.SetEnvironmentalReverbProperties(&defsettings))) {
            available = true;
        }
    }

    if (!available)
        return false;

    // call super method
    return BaseExtensionModule::onInstall(extmgr, token, user_arg);
}

void EnvironmentalReverbExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    CSLEnvironmentalReverbItf reverb;

    extmgr->extSetAuxEffectEnabled(AUX_EFFECT_ID, false);

    (void)extmgr->extGetInterfaceFromOutputMix(&reverb);

    if (reverb) {
        SLEnvironmentalReverbSettings defsettings;
        MakeDefaultSettings(&defsettings);
        (void)reverb.SetEnvironmentalReverbProperties(&defsettings);
    }

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void EnvironmentalReverbExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                                   const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    CSLEnvironmentalReverbItf reverb;
    SLresult slResult;

    (void)extmgr->extGetInterfaceFromOutputMix(&reverb);

    slResult = processMessage(extmgr, reverb, msg);

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, extmgr->extTranslateOpenSLErrorCode(slResult));
    }
}

SLresult EnvironmentalReverbExtModule::processMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                                      CSLEnvironmentalReverbItf &reverb,
                                                      const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    SLresult result = SL_RESULT_INTERNAL_ERROR;
    EnvironmentalReverbStatus &status = status_;

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
                onEnabledChanged(extmgr, status.enabled);
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
    case MSG_GET_DECAY_HF_RATIO: {
        typedef msg_blob_get_decay_hf_ratio blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLpermille slDecayHFRatio = 0;
        result = reverb.GetDecayHFRatio(&slDecayHFRatio);

        if (IS_SL_SUCCESS(result)) {
            (*blob.decayHFRatio) = slDecayHFRatio;
        }
    } break;
    case MSG_GET_DECAY_TIME: {
        typedef msg_blob_get_decay_time blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLmillisecond slDecayTime = 0;
        result = reverb.GetDecayTime(&slDecayTime);

        if (IS_SL_SUCCESS(result)) {
            (*blob.decayTime) = slDecayTime;
        }
    } break;
    case MSG_GET_DENSITY: {
        typedef msg_blob_get_density blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLpermille slDensity = 0;
        result = reverb.GetDensity(&slDensity);

        if (IS_SL_SUCCESS(result)) {
            (*blob.density) = slDensity;
        }
    } break;
    case MSG_GET_DIFFUSION: {
        typedef msg_blob_get_diffusion blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLpermille slDiffusion = 0;
        result = reverb.GetDiffusion(&slDiffusion);

        if (IS_SL_SUCCESS(result)) {
            (*blob.diffusion) = slDiffusion;
        }
    } break;
    case MSG_GET_REFLECTIONS_DELAY: {
        typedef msg_blob_get_reflections_delay blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLmillisecond slReflectionsDelay = 0;
        result = reverb.GetReflectionsDelay(&slReflectionsDelay);

        if (IS_SL_SUCCESS(result)) {
            (*blob.reflectionsDelay) = slReflectionsDelay;
        }
    } break;
    case MSG_GET_REFLECTIONS_LEVEL: {
        typedef msg_blob_get_reflections_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLmillibel slReflectionsLevel = 0;
        result = reverb.GetReflectionsLevel(&slReflectionsLevel);

        if (IS_SL_SUCCESS(result)) {
            (*blob.reflectionsLevel) = slReflectionsLevel;
        }
    } break;
    case MSG_GET_REVERB_DELAY: {
        typedef msg_blob_get_reverb_delay blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLmillisecond slReverbDelay = 0;
        result = reverb.GetReverbDelay(&slReverbDelay);

        if (IS_SL_SUCCESS(result)) {
            (*blob.reverbDelay) = slReverbDelay;
        }
    } break;
    case MSG_GET_REVERB_LEVEL: {
        typedef msg_blob_get_reverb_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLmillibel slReverbLevel = 0;
        result = reverb.GetReverbLevel(&slReverbLevel);

        if (IS_SL_SUCCESS(result)) {
            (*blob.reverbLevel) = slReverbLevel;
        }
    } break;
    case MSG_GET_ROOM_HF_LEVEL: {
        typedef msg_blob_get_room_hf_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLmillibel slRoomHF = 0;
        result = reverb.GetRoomHFLevel(&slRoomHF);

        if (IS_SL_SUCCESS(result)) {
            (*blob.roomHF) = slRoomHF;
        }
    } break;
    case MSG_GET_ROOM_LEVEL: {
        typedef msg_blob_get_room_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLmillibel slRoom = 0;
        result = reverb.GetRoomLevel(&slRoom);

        if (IS_SL_SUCCESS(result)) {
            (*blob.room) = slRoom;
        }
    } break;
    case MSG_GET_PROPERTIES: {
        typedef msg_blob_get_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLEnvironmentalReverbSettings slSettings = { 0 };

        result = reverb.GetEnvironmentalReverbProperties(&slSettings);

        if (IS_SL_SUCCESS(result)) {
            (*blob.settings) = fromSLSettings(&slSettings);
        }
    } break;
    case MSG_SET_DECAY_HF_RATIO: {
        typedef msg_blob_set_decay_hf_ratio blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLpermille slDecayHFRatio = blob.decayHFRatio;
            result = reverb.SetDecayHFRatio(slDecayHFRatio);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_DECAY_TIME: {
        typedef msg_blob_set_decay_time blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLmillisecond slDecayTime = blob.decayTime;
            result = reverb.SetDecayTime(slDecayTime);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_DENSITY: {
        typedef msg_blob_set_density blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLpermille slDensity = blob.density;
            result = reverb.SetDensity(slDensity);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_DIFFUSION: {
        typedef msg_blob_set_diffusion blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLpermille slDiffusion = blob.diffusion;
            result = reverb.SetDiffusion(slDiffusion);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_REFLECTIONS_DELAY: {
        typedef msg_blob_set_reflections_delay blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLmillisecond slReflectionsDelay = blob.reflectionsDelay;
            result = reverb.SetReflectionsDelay(slReflectionsDelay);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_REFLECTIONS_LEVEL: {
        typedef msg_blob_set_reflections_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLmillibel slReflectionsLevel = blob.reflectionsLevel;
            result = reverb.SetReflectionsLevel(slReflectionsLevel);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_REVERB_DELAY: {
        typedef msg_blob_set_reverb_delay blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLmillisecond slReverbDelay = blob.reverbDelay;
            result = reverb.SetReverbDelay(slReverbDelay);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_REVERB_LEVEL: {
        typedef msg_blob_set_reverb_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLmillibel slReverbLevel = blob.reverbLevel;
            result = reverb.SetReverbLevel(slReverbLevel);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_ROOM_HF_LEVEL: {
        typedef msg_blob_set_room_hf_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLmillibel slRoomHF = blob.roomHF;
            result = reverb.SetRoomHFLevel(slRoomHF);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_ROOM_LEVEL: {
        typedef msg_blob_set_room_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLmillibel slRoom = blob.room;
            result = reverb.SetRoomLevel(slRoom);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_PROPERTIES: {
        typedef msg_blob_set_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLEnvironmentalReverbSettings slSettings = toSLSettings(blob.settings);
            result = reverb.SetEnvironmentalReverbProperties(&slSettings);
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
