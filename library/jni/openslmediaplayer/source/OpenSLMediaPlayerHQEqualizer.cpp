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

// #define LOG_TAG "OpenSLMediaPlayerHQEqualizer"

#include "oslmp/OpenSLMediaPlayerHQEqualizer.hpp"

#include <cassert>
#include <vector>

#include <cxxporthelper/memory>
#include <cxxporthelper/compiler.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerThreadMessage.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/BaseExtensionModule.hpp"
#include "oslmp/impl/HQEqualizer.hpp"
#include "oslmp/impl/HQEqualizerPresets.hpp"

//
// Constants
//
#define MODULE_NAME "HQEqualizer"

//
// helper macros
//

#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    HQEqualizerExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                              \
    if (CXXPH_UNLIKELY(!(varname))) {                                                                                  \
        return OSLMP_RESULT_DEAD_OBJECT;                                                                               \
    }

#define CHECK_ARG(cond)                                                                                                \
    if (CXXPH_UNLIKELY(!(cond))) {                                                                                     \
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;                                                                          \
    }

#define CHECK_IS_ACTIVE(blob_) (CXXPH_LIKELY(checkIsClientActive((blob_).client)))

#define CHECK_RANGE(value_, min_, max_) (CXXPH_LIKELY(((value_) >= (min_)) && ((value_) <= (max_))))

#define CHECK_ARG_RANGE(value_, min_, max_) CHECK_ARG(CHECK_RANGE((value_), (min_), (max_)))

namespace oslmp {

using namespace ::opensles;
using namespace ::oslmp::impl;

typedef OpenSLMediaPlayerInternalContext InternalContext;

typedef uint32_t millihertz_t;
typedef int16_t millibel_t;

struct LevelRange {
    int16_t min;
    int16_t max;
};

struct HQEqualizerProperties {
    uint16_t numBands;
    uint16_t numPresets;
    LevelRange levelRange;
};

class HQEqualizerExtModule : public BaseExtensionModule {
public:
    HQEqualizerExtModule();
    virtual ~HQEqualizerExtModule();

    int setEnabled(void *client, bool enabled) noexcept;
    int getEnabled(void *client, bool *enabled) noexcept;
    int getId(void *client, int *id) noexcept;
    int hasControl(void *client, bool *hasControl) noexcept;
    int getBand(void *client, uint32_t frequency, uint16_t *band) noexcept;
    int getBandFreqRange(void *client, uint16_t band, uint32_t *range) noexcept;
    int getBandLevel(void *client, uint16_t band, int16_t *level) noexcept;
    int getBandLevelRange(void *client, int16_t *range) noexcept;
    int getCenterFreq(void *client, uint16_t band, uint32_t *center_freq) noexcept;
    int getCurrentPreset(void *client, uint16_t *preset) noexcept;
    int getNumberOfBands(void *client, uint16_t *num_bands) noexcept;
    int getNumberOfPresets(void *client, uint16_t *num_presets) noexcept;
    int getPresetName(void *client, uint16_t preset, const char **name) noexcept;
    int getProperties(void *client, OpenSLMediaPlayerHQEqualizer::Settings *settings) noexcept;
    int setBandLevel(void *client, uint16_t band, int16_t level) noexcept;
    int usePreset(void *client, uint16_t preset) noexcept;
    int setProperties(void *client, const OpenSLMediaPlayerHQEqualizer::Settings *settings) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept override;

private:
    static int getEqProperties(const HQEqualizer *equalizer, HQEqualizerProperties *props) noexcept;

    int processMessage(HQEqualizer &equalizer, const OpenSLMediaPlayerThreadMessage *msg) noexcept;

    uint16_t current_preset_;
    HQEqualizer *hq_equalizer_;
    HQEqualizerProperties props_;
};

class HQEqualizerExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    HQEqualizerExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) HQEqualizerExtModule();
    }
};

class OpenSLMediaPlayerHQEqualizer::Impl {
public:
    Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client);
    ~Impl();

    android::sp<OpenSLMediaPlayerContext> context_;
    void *client_;
    HQEqualizerExtModule *module_;
};

enum {
    MSG_NOP,
    MSG_SET_ENABLED,
    MSG_GET_ENABLED,
    MSG_GET_ID,
    MSG_HAS_CONTROL,
    MSG_GET_BAND,
    MSG_GET_BAND_FREQ_RANGE,
    MSG_GET_BAND_LEVEL,
    MSG_GET_BAND_LEVEL_RANGE,
    MSG_GET_CENTER_FREQ,
    MSG_GET_CURRENT_PRESET,
    MSG_GET_NUMBER_OF_BANDS,
    MSG_GET_NUMBER_OF_PRESETS,
    MSG_GET_PRESET_NAME,
    MSG_GET_PROPERTIES,
    MSG_SET_BAND_LEVEL,
    MSG_USE_PRESET,
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

struct msg_blob_get_band {
    void *client;
    uint32_t frequency;
    uint16_t *band;
};

struct msg_blob_get_band_freq_range {
    void *client;
    uint16_t band;
    uint32_t *range;
};

struct msg_blob_get_band_level {
    void *client;
    uint16_t band;
    int16_t *level;
};

struct msg_blob_get_band_level_range {
    void *client;
    int16_t *range;
};

struct msg_blob_get_center_freq {
    void *client;
    uint16_t band;
    uint32_t *center_freq;
};

struct msg_blob_get_current_preset {
    void *client;
    uint16_t *preset;
};

struct msg_blob_get_number_of_bands {
    void *client;
    uint16_t *num_bands;
};

struct msg_blob_get_number_of_presets {
    void *client;
    uint16_t *num_presets;
};

struct msg_blob_get_preset_name {
    void *client;
    uint16_t preset;
    const char **name;
};

struct msg_blob_get_properties {
    void *client;
    OpenSLMediaPlayerHQEqualizer::Settings *settings;
};

struct msg_blob_set_band_level {
    void *client;
    uint16_t band;
    int16_t level;
};

struct msg_blob_use_preset {
    void *client;
    uint16_t preset;
};

struct msg_blob_set_properties {
    void *client;
    const OpenSLMediaPlayerHQEqualizer::Settings *settings;
};

//
// Utilities
//

//
// OpenSLMediaPlayerHQEqualizer
//
OpenSLMediaPlayerHQEqualizer::OpenSLMediaPlayerHQEqualizer(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(context, this))
{
}

OpenSLMediaPlayerHQEqualizer::~OpenSLMediaPlayerHQEqualizer()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerHQEqualizer::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(this, enabled);
}

int OpenSLMediaPlayerHQEqualizer::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(this, enabled);
}

int OpenSLMediaPlayerHQEqualizer::getId(int *id) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getId(this, id);
}

int OpenSLMediaPlayerHQEqualizer::hasControl(bool *hasControl) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->hasControl(this, hasControl);
}

int OpenSLMediaPlayerHQEqualizer::getBand(uint32_t frequency, uint16_t *band) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getBand(this, frequency, band);
}

int OpenSLMediaPlayerHQEqualizer::getBandFreqRange(uint16_t band, uint32_t *range) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getBandFreqRange(this, band, range);
}

int OpenSLMediaPlayerHQEqualizer::getBandLevel(uint16_t band, int16_t *level) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getBandLevel(this, band, level);
}

int OpenSLMediaPlayerHQEqualizer::getBandLevelRange(int16_t *range) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getBandLevelRange(this, range);
}

int OpenSLMediaPlayerHQEqualizer::getCenterFreq(uint16_t band, uint32_t *center_freq) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getCenterFreq(this, band, center_freq);
}

int OpenSLMediaPlayerHQEqualizer::getCurrentPreset(uint16_t *preset) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getCurrentPreset(this, preset);
}

int OpenSLMediaPlayerHQEqualizer::getNumberOfBands(uint16_t *num_bands) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getNumberOfBands(this, num_bands);
}

int OpenSLMediaPlayerHQEqualizer::getNumberOfPresets(uint16_t *num_presets) noexcept
{
    GET_MODULE_INSTANCE(module);

    return module->getNumberOfPresets(this, num_presets);
}

int OpenSLMediaPlayerHQEqualizer::getPresetName(uint16_t preset, const char **name) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getPresetName(this, preset, name);
}

int OpenSLMediaPlayerHQEqualizer::getProperties(OpenSLMediaPlayerHQEqualizer::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getProperties(this, settings);
}

int OpenSLMediaPlayerHQEqualizer::setBandLevel(uint16_t band, int16_t level) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setBandLevel(this, band, level);
}

int OpenSLMediaPlayerHQEqualizer::usePreset(uint16_t preset) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->usePreset(this, preset);
}

int OpenSLMediaPlayerHQEqualizer::setProperties(const OpenSLMediaPlayerHQEqualizer::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setProperties(this, settings);
}

//
// OpenSLMediaPlayerHQEqualizer::Impl
//
OpenSLMediaPlayerHQEqualizer::Impl::Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client)
    : context_(context), client_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_HQ_EQUALIZER) {
        const HQEqualizerExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module;

        int result = c.extAttachOrInstall(&module, &creator, client);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<HQEqualizerExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerHQEqualizer::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(client_);
        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// HQEqualizerExtModule
//
HQEqualizerExtModule::HQEqualizerExtModule()
    : BaseExtensionModule(MODULE_NAME), current_preset_(HQEqualizer::UNDEFINED), hq_equalizer_(nullptr), props_()
{
}

HQEqualizerExtModule::~HQEqualizerExtModule() {}

int HQEqualizerExtModule::setEnabled(void *client, bool enabled) noexcept
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

int HQEqualizerExtModule::getEnabled(void *client, bool *enabled) noexcept
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

int HQEqualizerExtModule::getId(void *client, int *id) noexcept
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

int HQEqualizerExtModule::hasControl(void *client, bool *hasControl) noexcept
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

int HQEqualizerExtModule::getBand(void *client, uint32_t frequency, uint16_t *band) noexcept
{
    typedef msg_blob_get_band blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(band != nullptr);

    (*band) = 0;

    Message msg(0, MSG_GET_BAND);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.frequency = frequency;
        blob.band = band;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getBandFreqRange(void *client, uint16_t band, uint32_t *range) noexcept
{
    typedef msg_blob_get_band_freq_range blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(range != nullptr);

    range[0] = 0;
    range[1] = 0;

    Message msg(0, MSG_GET_BAND_FREQ_RANGE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.band = band;
        blob.range = range;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getBandLevel(void *client, uint16_t band, int16_t *level) noexcept
{
    typedef msg_blob_get_band_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(level != nullptr);

    (*level) = 0;

    Message msg(0, MSG_GET_BAND_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.band = band;
        blob.level = level;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getBandLevelRange(void *client, int16_t *range) noexcept
{
    typedef msg_blob_get_band_level_range blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(range != nullptr);

    range[0] = 0;
    range[1] = 0;

    Message msg(0, MSG_GET_BAND_LEVEL_RANGE);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.range = range;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getCenterFreq(void *client, uint16_t band, uint32_t *center_freq) noexcept
{
    typedef msg_blob_get_center_freq blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(center_freq != nullptr);

    (*center_freq) = 0;

    Message msg(0, MSG_GET_CENTER_FREQ);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.band = band;
        blob.center_freq = center_freq;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getCurrentPreset(void *client, uint16_t *preset) noexcept
{
    typedef msg_blob_get_current_preset blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(preset != nullptr);

    (*preset) = 0;

    Message msg(0, MSG_GET_CURRENT_PRESET);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.preset = preset;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getNumberOfBands(void *client, uint16_t *num_bands) noexcept
{
    typedef msg_blob_get_number_of_bands blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(num_bands != nullptr);

    (*num_bands) = 0;

    Message msg(0, MSG_GET_NUMBER_OF_BANDS);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.num_bands = num_bands;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getNumberOfPresets(void *client, uint16_t *num_presets) noexcept
{
    typedef msg_blob_get_number_of_presets blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(num_presets != nullptr);

    Message msg(0, MSG_GET_NUMBER_OF_PRESETS);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.num_presets = num_presets;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getPresetName(void *client, uint16_t preset, const char **name) noexcept
{
    typedef msg_blob_get_preset_name blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(name != nullptr);

    Message msg(0, MSG_GET_PRESET_NAME);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.preset = preset;
        blob.name = name;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getProperties(void *client, OpenSLMediaPlayerHQEqualizer::Settings *settings) noexcept
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

int HQEqualizerExtModule::setBandLevel(void *client, uint16_t band, int16_t level) noexcept
{
    typedef msg_blob_set_band_level blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(band < props_.numBands);
    CHECK_ARG_RANGE(level, props_.levelRange.min, props_.levelRange.max);

    Message msg(0, MSG_SET_BAND_LEVEL);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.band = band;
        blob.level = level;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::usePreset(void *client, uint16_t preset) noexcept
{
    typedef msg_blob_use_preset blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG((preset < props_.numPresets) || (preset == SL_EQUALIZER_UNDEFINED));

    Message msg(0, MSG_USE_PRESET);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.preset = preset;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::setProperties(void *client, const OpenSLMediaPlayerHQEqualizer::Settings *settings) noexcept
{
    typedef msg_blob_set_properties blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(settings != nullptr);
    CHECK_ARG((settings->curPreset < props_.numPresets) || (settings->curPreset == SL_EQUALIZER_UNDEFINED));
    CHECK_ARG(settings->numBands == props_.numBands);
    for (int i = 0; i < props_.numBands; ++i) {
        CHECK_ARG_RANGE(settings->bandLevels[i], props_.levelRange.min, props_.levelRange.max);
    }

    Message msg(0, MSG_SET_PROPERTIES);

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.settings = settings;
    }

    return postAndWaitResult(&msg);
}

int HQEqualizerExtModule::getEqProperties(const HQEqualizer *equalizer, HQEqualizerProperties *props) noexcept
{
    int result;

    // obtain equalizer information
    result = equalizer->getBandLevelRange(&(props->levelRange.min), &(props->levelRange.max));

    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    result = equalizer->getNumberOfBands(&(props->numBands));

    if (result != OSLMP_RESULT_SUCCESS) {
        return result;
    }

    props->numPresets = HQEqualizerPresets::sGetNumPresets();

    return result;
}

bool HQEqualizerExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                     void *user_arg) noexcept
{

    bool available = false;
    HQEqualizer *hq_equalizer = nullptr;

    // obtain info & set initial preset
    {
        int result = extmgr->extGetHQEqualizer(&hq_equalizer);

        if (result == OSLMP_RESULT_SUCCESS && hq_equalizer) {
            result = getEqProperties(hq_equalizer, &props_);
            if (result == OSLMP_RESULT_SUCCESS) {
                const HQEqualizerPresets::PresetData *preset_data = HQEqualizerPresets::sGetPresetData(0);

                result = hq_equalizer->setAllBandLevel(preset_data->band_level, preset_data->num_bands);

                if (result == OSLMP_RESULT_SUCCESS) {
                    available = true;
                }
            } else {
                LOGE("getEqProperties() failed - %d", result);
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
    current_preset_ = 0;
    hq_equalizer_ = hq_equalizer;

    return true;
}

void HQEqualizerExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    // update fields
    hq_equalizer_ = nullptr;

    // disable equalizer
    HQEqualizer *hq_equalizer = nullptr;

    int result = extmgr->extGetHQEqualizer(&hq_equalizer);

    if (result == OSLMP_RESULT_SUCCESS && hq_equalizer) {
        (void)hq_equalizer->setEnabled(false);
    }

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void HQEqualizerExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                           const OpenSLMediaPlayerThreadMessage *msg) noexcept
{

    int result;

    if (hq_equalizer_) {
        result = processMessage((*hq_equalizer_), msg);
    } else {
        result = OSLMP_RESULT_ILLEGAL_STATE;
    }

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, result);
    }
}

int HQEqualizerExtModule::processMessage(HQEqualizer &equalizer, const OpenSLMediaPlayerThreadMessage *msg) noexcept
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
            result = equalizer.setEnabled(blob.enabled);
        } else {
            result = OSLMP_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_ENABLED: {
        typedef msg_blob_get_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        result = equalizer.getEnabled(blob.enabled);
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
    case MSG_GET_BAND: {
        typedef msg_blob_get_band blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        result = equalizer.getBand(blob.frequency, blob.band);
    } break;
    case MSG_GET_BAND_FREQ_RANGE: {
        typedef msg_blob_get_band_freq_range blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        result = equalizer.getBandFreqRange(blob.band, &(blob.range[0]), &(blob.range[1]));
    } break;
    case MSG_GET_BAND_LEVEL: {
        typedef msg_blob_get_band_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        result = equalizer.getBandLevel(blob.band, blob.level);
    } break;
    case MSG_GET_BAND_LEVEL_RANGE: {
        typedef msg_blob_get_band_level_range blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        result = equalizer.getBandLevelRange(&(blob.range[0]), &(blob.range[1]));
    } break;
    case MSG_GET_CENTER_FREQ: {
        typedef msg_blob_get_center_freq blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        result = equalizer.getCenterFreq(blob.band, blob.center_freq);
    } break;
    case MSG_GET_CURRENT_PRESET: {
        typedef msg_blob_get_current_preset blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.preset) = current_preset_;
        result = OSLMP_RESULT_SUCCESS;
    } break;
    case MSG_GET_NUMBER_OF_BANDS: {
        typedef msg_blob_get_number_of_bands blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        result = equalizer.getNumberOfBands(blob.num_bands);
    } break;
    case MSG_GET_NUMBER_OF_PRESETS: {
        typedef msg_blob_get_number_of_presets blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        (*blob.num_presets) = HQEqualizerPresets::sGetNumPresets();
        result = OSLMP_RESULT_SUCCESS;
    } break;
    case MSG_GET_PRESET_NAME: {
        typedef msg_blob_get_preset_name blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        const HQEqualizerPresets::PresetData *preset_data = HQEqualizerPresets::sGetPresetData(blob.preset);

        if (preset_data) {
            (*blob.name) = preset_data->name;
            result = OSLMP_RESULT_SUCCESS;
        } else {
            result = OSLMP_RESULT_ILLEGAL_ARGUMENT;
        }
    } break;
    case MSG_GET_PROPERTIES: {
        typedef msg_blob_get_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        blob.settings->curPreset = current_preset_;
        blob.settings->numBands = props_.numBands;
        result = equalizer.getAllBandLevel(blob.settings->bandLevels, blob.settings->numBands);
    } break;
    case MSG_SET_BAND_LEVEL: {
        typedef msg_blob_set_band_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            int16_t cur_level = 0;
            result = equalizer.getBandLevel(blob.band, &cur_level);
            if (result == OSLMP_RESULT_SUCCESS) {
                if (cur_level != blob.level) {
                    result = equalizer.setBandLevel(blob.band, blob.level);
                    if (result == OSLMP_RESULT_SUCCESS) {
                        current_preset_ = HQEqualizer::UNDEFINED;
                    }
                }
            }
        } else {
            result = OSLMP_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_USE_PRESET: {
        typedef msg_blob_use_preset blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const HQEqualizerPresets::PresetData *preset_data = HQEqualizerPresets::sGetPresetData(blob.preset);

            if (preset_data) {
                result = equalizer.setAllBandLevel(preset_data->band_level, preset_data->num_bands);

                if (result == OSLMP_RESULT_SUCCESS) {
                    current_preset_ = blob.preset;
                }
            } else {
                result = OSLMP_RESULT_ILLEGAL_ARGUMENT;
            }
        } else {
            result = OSLMP_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_PROPERTIES: {
        typedef msg_blob_set_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const HQEqualizerPresets::PresetData *preset_data =
                HQEqualizerPresets::sGetPresetData(blob.settings->curPreset);

            if (blob.settings->curPreset != HQEqualizer::UNDEFINED && preset_data) {
                result = equalizer.setAllBandLevel(preset_data->band_level, preset_data->num_bands);

                if (result == OSLMP_RESULT_SUCCESS) {
                    current_preset_ = blob.settings->curPreset;
                }

            } else {
                result = equalizer.setAllBandLevel(blob.settings->bandLevels, blob.settings->numBands);

                if (result == OSLMP_RESULT_SUCCESS) {
                    current_preset_ = HQEqualizer::UNDEFINED;
                }
            }
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

} // namespace oslmp
