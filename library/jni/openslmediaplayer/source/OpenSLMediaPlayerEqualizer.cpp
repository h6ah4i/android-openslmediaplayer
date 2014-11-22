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

// #define LOG_TAG "OpenSLMediaPlayerEqualizer"

#include "oslmp/OpenSLMediaPlayerEqualizer.hpp"

#include <cassert>
#include <vector>

#include <cxxporthelper/memory>
#include <cxxporthelper/compiler.hpp>

#include <loghelper/loghelper.h>

#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/OpenSLMediaPlayerThreadMessage.hpp"
#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/impl/BaseExtensionModule.hpp"
#include "oslmp/impl/EqualizerBandInfoCorrector.hpp"

//
// Constants
//
#define MODULE_NAME "Equalizer"

//
// helper macros
//

#define LOCAL_ASSERT(cond) assert(cond)
#define LOCAL_STATIC_ASSERT(cond, message) static_assert((cond), message);

#define CHECK_MSG_BLOB_SIZE(blob_type)                                                                                 \
    LOCAL_STATIC_ASSERT((sizeof(blob_type) <= MESSAGE_BLOB_SIZE), #blob_type " is too large")

#define GET_MODULE_INSTANCE(varname)                                                                                   \
    EqualizerExtModule *varname = (impl_) ? (impl_->module_) : nullptr;                                                \
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

struct LevelRange {
    SLint16 min;
    SLint16 max;
};

struct BandInfo {
    SLuint32 min;
    SLuint32 max;
    SLuint32 center;
};

struct EqualizerProperties {
    SLuint16 numBands;
    SLuint16 numPresets;
    LevelRange levelRange;
    std::vector<BandInfo> bandInfo;
};

class EqualizerExtModule : public BaseExtensionModule {
public:
    EqualizerExtModule();
    virtual ~EqualizerExtModule();

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
    int getProperties(void *client, OpenSLMediaPlayerEqualizer::Settings *settings) noexcept;
    int setBandLevel(void *client, uint16_t band, int16_t level) noexcept;
    int usePreset(void *client, uint16_t preset) noexcept;
    int setProperties(void *client, const OpenSLMediaPlayerEqualizer::Settings *settings) noexcept;

    // implementations of OpenSLMediaPlayerExtension
    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept override;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept override;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept override;

    virtual void onBeforeAudioSinkStateChanged(OpenSLMediaPlayerExtensionManager *extmgr,
                                               bool next_is_started) noexcept override;

private:
    SLresult processMessage(CSLEqualizerItf &equalizer, const OpenSLMediaPlayerThreadMessage *msg) noexcept;

    void workAroundRefreshSettings(CSLEqualizerItf &equalizer) noexcept;

    static SLresult getEqProperties(CSLEqualizerItf *equalizer, EqualizerProperties *props) noexcept;
    static bool correctEqProperties(const EqualizerProperties *origProps, EqualizerProperties *props) noexcept;
    // static void dumpEqProperties(
    //     const EqualizerProperties *props) noexcept;

private:
    EqualizerProperties origProps_;
    EqualizerProperties props_;
};

class EqualizerExtModuleCreator : public OpenSLMediaPlayerExtensionCreator {
public:
    EqualizerExtModuleCreator() {}

    virtual const char *getModuleName() const noexcept override { return MODULE_NAME; }

    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept override
    {
        return new (std::nothrow) EqualizerExtModule();
    }
};

class OpenSLMediaPlayerEqualizer::Impl {
public:
    Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client);
    ~Impl();

    android::sp<OpenSLMediaPlayerContext> context_;
    void *client_;
    EqualizerExtModule *module_;
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
    OpenSLMediaPlayerEqualizer::Settings *settings;
};

struct msg_blob_set_band_level {
    void *client;
    uint16_t band;
    int16_t level;
};

struct msg_blob_use_preset {
    void *client;
    SLuint16 preset;
};

struct msg_blob_set_properties {
    void *client;
    const OpenSLMediaPlayerEqualizer::Settings *settings;
};

//
// Utilities
//

static inline SLresult GetProperties(CSLEqualizerItf &equalizer,
                                     OpenSLMediaPlayerEqualizer::Settings *settings) noexcept
{
    SLresult result;

    result = equalizer.GetCurrentPreset(&(settings->curPreset));

    if (!IS_SL_SUCCESS(result))
        return result;

    result = equalizer.GetNumberOfBands(&(settings->numBands));

    if (!IS_SL_SUCCESS(result))
        return result;

    if (settings->numBands > (sizeof(settings->bandLevels) / sizeof(settings->bandLevels[0])))
        return SL_RESULT_PARAMETER_INVALID;

    for (int i = 0; i < settings->numBands; ++i) {
        result = equalizer.GetBandLevel(i, &(settings->bandLevels[i]));

        if (!IS_SL_SUCCESS(result))
            return result;
    }

    return SL_RESULT_SUCCESS;
}

static inline SLresult SetProperties(CSLEqualizerItf &equalizer,
                                     const OpenSLMediaPlayerEqualizer::Settings *settings) noexcept
{
    SLresult result;
    SLuint16 numBands = 0;

    result = equalizer.GetNumberOfBands(&numBands);

    if (!IS_SL_SUCCESS(result))
        return result;

    if (settings->numBands != numBands)
        return SL_RESULT_PARAMETER_INVALID;

    if (settings->curPreset != SL_EQUALIZER_UNDEFINED) {
        result = equalizer.UsePreset(settings->curPreset);

        if (!IS_SL_SUCCESS(result))
            return result;
    } else {
        for (int i = 0; i < settings->numBands; ++i) {
            result = equalizer.SetBandLevel(i, settings->bandLevels[i]);

            if (!IS_SL_SUCCESS(result))
                return result;
        }
    }

    return SL_RESULT_SUCCESS;
}

static inline SLresult MakeDefaultSettings(CSLEqualizerItf &equalizer,
                                           OpenSLMediaPlayerEqualizer::Settings *settings) noexcept
{
    SLresult result;
    SLuint16 numBands = 0;

    ::memset(settings, 0, sizeof(*settings));

    result = equalizer.GetNumberOfBands(&numBands);

    if (!IS_SL_SUCCESS(result))
        return result;

    if (numBands > (sizeof(settings->bandLevels) / sizeof(settings->bandLevels[0])))
        return SL_RESULT_INTERNAL_ERROR;

    settings->curPreset = 0;
    settings->numBands = numBands;
    // settings->bandLevels field is already filled by zero

    return SL_RESULT_SUCCESS;
}

//
// OpenSLMediaPlayerEqualizer
//
OpenSLMediaPlayerEqualizer::OpenSLMediaPlayerEqualizer(const android::sp<OpenSLMediaPlayerContext> &context)
    : impl_(new (std::nothrow) Impl(context, this))
{
}

OpenSLMediaPlayerEqualizer::~OpenSLMediaPlayerEqualizer()
{
    delete impl_;
    impl_ = nullptr;
}

int OpenSLMediaPlayerEqualizer::setEnabled(bool enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setEnabled(this, enabled);
}

int OpenSLMediaPlayerEqualizer::getEnabled(bool *enabled) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getEnabled(this, enabled);
}

int OpenSLMediaPlayerEqualizer::getId(int *id) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getId(this, id);
}

int OpenSLMediaPlayerEqualizer::hasControl(bool *hasControl) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->hasControl(this, hasControl);
}

int OpenSLMediaPlayerEqualizer::getBand(uint32_t frequency, uint16_t *band) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getBand(this, frequency, band);
}

int OpenSLMediaPlayerEqualizer::getBandFreqRange(uint16_t band, uint32_t *range) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getBandFreqRange(this, band, range);
}

int OpenSLMediaPlayerEqualizer::getBandLevel(uint16_t band, int16_t *level) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getBandLevel(this, band, level);
}

int OpenSLMediaPlayerEqualizer::getBandLevelRange(int16_t *range) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getBandLevelRange(this, range);
}

int OpenSLMediaPlayerEqualizer::getCenterFreq(uint16_t band, uint32_t *center_freq) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getCenterFreq(this, band, center_freq);
}

int OpenSLMediaPlayerEqualizer::getCurrentPreset(uint16_t *preset) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getCurrentPreset(this, preset);
}

int OpenSLMediaPlayerEqualizer::getNumberOfBands(uint16_t *num_bands) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getNumberOfBands(this, num_bands);
}

int OpenSLMediaPlayerEqualizer::getNumberOfPresets(uint16_t *num_presets) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getNumberOfPresets(this, num_presets);
}

int OpenSLMediaPlayerEqualizer::getPresetName(uint16_t preset, const char **name) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getPresetName(this, preset, name);
}

int OpenSLMediaPlayerEqualizer::getProperties(OpenSLMediaPlayerEqualizer::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->getProperties(this, settings);
}

int OpenSLMediaPlayerEqualizer::setBandLevel(uint16_t band, int16_t level) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setBandLevel(this, band, level);
}

int OpenSLMediaPlayerEqualizer::usePreset(uint16_t preset) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->usePreset(this, preset);
}

int OpenSLMediaPlayerEqualizer::setProperties(const OpenSLMediaPlayerEqualizer::Settings *settings) noexcept
{
    GET_MODULE_INSTANCE(module);
    return module->setProperties(this, settings);
}

//
// OpenSLMediaPlayerEqualizer::Impl
//
OpenSLMediaPlayerEqualizer::Impl::Impl(const android::sp<OpenSLMediaPlayerContext> &context, void *client)
    : context_(context), client_(client), module_(nullptr)
{
    InternalContext &c = InternalContext::sGetInternal(*context);
    const uint32_t opts = c.getContextOptions();

    if (opts & OSLMP_CONTEXT_OPTION_USE_EQUALIZER) {
        const EqualizerExtModuleCreator creator;
        OpenSLMediaPlayerExtension *module;

        int result = c.extAttachOrInstall(&module, &creator, client);

        if (result == OSLMP_RESULT_SUCCESS) {
            LOCAL_ASSERT(module);
            module_ = dynamic_cast<EqualizerExtModule *>(module);
        }
    }
}

OpenSLMediaPlayerEqualizer::Impl::~Impl()
{
    if (module_) {
        module_->detachClient(client_);
        // NOTE: do not delete module instance here,
        // because it will be automatically deleted after onUninstall() is called
        module_ = nullptr;
    }
}

//
// EqualizerExtModule
//
EqualizerExtModule::EqualizerExtModule() : BaseExtensionModule(MODULE_NAME), props_() {}

EqualizerExtModule::~EqualizerExtModule() {}

SLresult EqualizerExtModule::getEqProperties(CSLEqualizerItf *equalizer, EqualizerProperties *props) noexcept
{
    SLresult result;

    // obtain equalizer information
    result = equalizer->GetBandLevelRange(&(props->levelRange.min), &(props->levelRange.max));
    if (!IS_SL_SUCCESS(result))
        return result;

    result = equalizer->GetNumberOfBands(&(props->numBands));
    if (!IS_SL_SUCCESS(result))
        return result;

    result = equalizer->GetNumberOfPresets(&(props->numPresets));
    if (!IS_SL_SUCCESS(result))
        return result;

    props->bandInfo.resize(props->numBands);

    // frequency ranges & center frequency
    for (SLuint16 i = 0; i < props->numBands; ++i) {
        result = equalizer->GetBandFreqRange(i, &(props->bandInfo[i].min), &(props->bandInfo[i].max));
        if (!IS_SL_SUCCESS(result))
            return result;

        result = equalizer->GetCenterFreq(i, &(props->bandInfo[i].center));
        if (!IS_SL_SUCCESS(result))
            return result;
    }

    // reset to the default preset
    result = equalizer->UsePreset(0);
    if (!IS_SL_SUCCESS(result))
        return result;

    return result;
}

bool EqualizerExtModule::correctEqProperties(const EqualizerProperties *origProps, EqualizerProperties *props) noexcept
{
    try
    {
        // copy properties
        props->numBands = origProps->numBands;
        props->numPresets = origProps->numPresets;
        props->levelRange = origProps->levelRange;

        // correct bandInfo property
        std::vector<int> inCenterFreqVec(props->numBands);
        std::vector<int> inBandLevelRangeVec(props->numBands * 2);
        std::vector<int> outCenterFreqVec(props->numBands);
        std::vector<int> outBandLevelRangeVec(props->numBands * 2);

        int *inCenterFreq = &inCenterFreqVec[0];
        int(*inBandLevelRange)[2] = reinterpret_cast<int(*)[2]>(&inBandLevelRangeVec[0]);
        int *outCenterFreq = &outCenterFreqVec[0];
        int(*outBandLevelRange)[2] = reinterpret_cast<int(*)[2]>(&outBandLevelRangeVec[0]);

        for (int band = 0; band < origProps->numBands; ++band) {
            inCenterFreq[band] = static_cast<int>(origProps->bandInfo[band].center);
            inBandLevelRange[band][0] = static_cast<int>(origProps->bandInfo[band].min);
            inBandLevelRange[band][1] = static_cast<int>(origProps->bandInfo[band].max);
        }

        if (!EqualizerBandInfoCorrector::correct(origProps->numBands, inCenterFreq, inBandLevelRange, outCenterFreq,
                                                 outBandLevelRange)) {
            return false;
        }

        props->bandInfo.resize(origProps->numBands);

        for (int band = 0; band < origProps->numBands; ++band) {
            props->bandInfo[band].center = static_cast<SLuint32>(outCenterFreq[band]);
            props->bandInfo[band].min = static_cast<SLuint32>(outBandLevelRange[band][0]);
            props->bandInfo[band].max = static_cast<SLuint32>(outBandLevelRange[band][1]);
        }
    }
    catch (const std::bad_alloc &) { return false; }

    return true;
}

#if 0
void EqualizerExtModule::dumpEqProperties(
    const EqualizerProperties *props) noexcept {
    LOGI("[dumpEqProperties] numBands = %d", props->numBands);
    LOGI("[dumpEqProperties] numPresets = %d", props->numPresets);
    LOGI("[dumpEqProperties] bandLevelRange = (%d, %d)", props->levelRange.min, props->levelRange.max);

    for (size_t i = 0; i < props->bandInfo.size(); ++i) {
        const BandInfo &info = props->bandInfo[i];
        LOGI("[dumpEqProperties] band[%d] (%d, %d, %d)",
        static_cast<int>(i), info.min, info.center, info.max);
    }
}
#endif

int EqualizerExtModule::setEnabled(void *client, bool enabled) noexcept
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

int EqualizerExtModule::getEnabled(void *client, bool *enabled) noexcept
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

int EqualizerExtModule::getId(void *client, int *id) noexcept
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

int EqualizerExtModule::hasControl(void *client, bool *hasControl) noexcept
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

int EqualizerExtModule::getBand(void *client, uint32_t frequency, uint16_t *band) noexcept
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

int EqualizerExtModule::getBandFreqRange(void *client, uint16_t band, uint32_t *range) noexcept
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

int EqualizerExtModule::getBandLevel(void *client, uint16_t band, int16_t *level) noexcept
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

int EqualizerExtModule::getBandLevelRange(void *client, int16_t *range) noexcept
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

int EqualizerExtModule::getCenterFreq(void *client, uint16_t band, uint32_t *center_freq) noexcept
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

int EqualizerExtModule::getCurrentPreset(void *client, uint16_t *preset) noexcept
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

int EqualizerExtModule::getNumberOfBands(void *client, uint16_t *num_bands) noexcept
{
    typedef msg_blob_get_number_of_bands blob_t;
    CHECK_MSG_BLOB_SIZE(blob_t);

    CHECK_ARG(num_bands != nullptr);

    (*num_bands) = 0;

    Message msg(0, MSG_GET_NUMBER_OF_BANDS);
    SLuint16 value = 0;

    {
        blob_t &blob = GET_MSG_BLOB(msg);
        blob.client = client;
        blob.num_bands = num_bands;
    }

    return postAndWaitResult(&msg);
}

int EqualizerExtModule::getNumberOfPresets(void *client, uint16_t *num_presets) noexcept
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

int EqualizerExtModule::getPresetName(void *client, uint16_t preset, const char **name) noexcept
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

int EqualizerExtModule::getProperties(void *client, OpenSLMediaPlayerEqualizer::Settings *settings) noexcept
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

int EqualizerExtModule::setBandLevel(void *client, uint16_t band, int16_t level) noexcept
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

int EqualizerExtModule::usePreset(void *client, uint16_t preset) noexcept
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

int EqualizerExtModule::setProperties(void *client, const OpenSLMediaPlayerEqualizer::Settings *settings) noexcept
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

bool EqualizerExtModule::onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                                   void *user_arg) noexcept
{

    bool available = false;

    // obtain info
    {
        CSLEqualizerItf equalizer;
        SLresult result;

        if (IS_SL_SUCCESS(extmgr->extGetInterfaceFromPlayer(&equalizer))) {
            // acquired a equalizer instance from the current player

            OpenSLMediaPlayerEqualizer::Settings defsettings;

            // apply default settings
            (void)MakeDefaultSettings(equalizer, &defsettings);
            (void)equalizer.SetEnabled(SL_BOOLEAN_FALSE);
            (void)SetProperties(equalizer, &defsettings);

            result = getEqProperties(&equalizer, &origProps_);
            if (IS_SL_SUCCESS(result)) {
                if (correctEqProperties(&origProps_, &props_)) {
                    // dumpEqProperties(&origProps_);
                    // dumpEqProperties(&props_);
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
    return BaseExtensionModule::onInstall(extmgr, token, user_arg);
}

void EqualizerExtModule::onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept
{

    CSLEqualizerItf equalizer;

    (void)extmgr->extGetInterfaceFromPlayer(&equalizer);

    if (equalizer) {
        OpenSLMediaPlayerEqualizer::Settings defsettings;

        (void)MakeDefaultSettings(equalizer, &defsettings);
        (void)equalizer.SetEnabled(SL_BOOLEAN_FALSE);
        (void)SetProperties(equalizer, &defsettings);
    }

    // call super method
    BaseExtensionModule::onUninstall(extmgr, user_arg);
}

void EqualizerExtModule::onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                         const OpenSLMediaPlayerThreadMessage *msg) noexcept
{
    CSLEqualizerItf equalizer;

    SLresult slResult;

    // NOTE:
    // The equalizer interface of the output mix doesn't work on
    // KitKat (4.4), so we should not use extGetInterfaceFromOutputMix()
    // function here...

    extmgr->extGetInterfaceFromPlayer(&equalizer);

    slResult = processMessage(equalizer, msg);

    int result = extmgr->extTranslateOpenSLErrorCode(slResult);

    // notify result
    if (msg->needNotification()) {
        notifyResult(msg, result);
    }
}

void EqualizerExtModule::onBeforeAudioSinkStateChanged(OpenSLMediaPlayerExtensionManager *extmgr,
                                                       bool next_is_started) noexcept
{
    if (!next_is_started)
        return;

    CSLEqualizerItf equalizer;

    extmgr->extGetInterfaceFromPlayer(&equalizer);

    if (equalizer) {
        workAroundRefreshSettings(equalizer);
    }
}

SLresult EqualizerExtModule::processMessage(CSLEqualizerItf &equalizer,
                                            const OpenSLMediaPlayerThreadMessage *msg) noexcept
{
    SLresult result = SL_RESULT_INTERNAL_ERROR;
    const EqualizerProperties &props = props_;

    switch (msg->what) {
    case MSG_NOP: {
        LOCAL_ASSERT(false);
    } break;
    case MSG_SET_ENABLED: {
        typedef msg_blob_set_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        if (CHECK_IS_ACTIVE(blob)) {
            const SLboolean slEnabled = CSLboolean::toSL(blob.enabled);

            if (blob.enabled) {
                workAroundRefreshSettings(equalizer);
            }

            result = equalizer.SetEnabled(slEnabled);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_GET_ENABLED: {
        typedef msg_blob_get_enabled blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLboolean slEnabled = SL_BOOLEAN_FALSE;
        result = equalizer.IsEnabled(&slEnabled);

        if (IS_SL_SUCCESS(result)) {
            (*blob.enabled) = slEnabled;
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
    case MSG_GET_BAND: {
        typedef msg_blob_get_band blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        const SLuint32 slFrequency = blob.frequency;
        SLuint16 slBand = SL_EQUALIZER_UNDEFINED;

#if 0 // don't use, because it may returns incorrect results
        result = equalizer.GetBand(slFrequency, &slBand);

        if (result == SL_RESULT_SUCCESS && (slBand == ERROR_WHAT_BAD_VALUE)) {
            slBand = SL_EQUALIZER_UNDEFINED;
        }
#else
        for (int i = 0; i < props.numBands; ++i) {
            if (slFrequency >= props.bandInfo[i].min && slFrequency <= props.bandInfo[i].max) {
                slBand = i;
            }
        }

        result = SL_RESULT_SUCCESS;
#endif
        if (result == SL_RESULT_SUCCESS) {
            (*blob.band) = slBand;
        }
    } break;
    case MSG_GET_BAND_FREQ_RANGE: {
        typedef msg_blob_get_band_freq_range blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        const SLuint16 slBand = blob.band;
        SLmilliHertz slRange[2] = { 0, 0 };

#if 0 // don't use, because it may returns incorrect results
        result = equalizer.GetBandFreqRange(slBand, &slRange[0], &slRange[1]);
#else
        if (slBand < props.numBands) {
            slRange[0] = props.bandInfo[blob.band].min;
            slRange[1] = props.bandInfo[blob.band].max;
            result = SL_RESULT_SUCCESS;
        } else {
            result = SL_RESULT_PARAMETER_INVALID;
        }
#endif
        if (IS_SL_SUCCESS(result)) {
            blob.range[0] = slRange[0];
            blob.range[1] = slRange[1];
        }
    } break;
    case MSG_GET_BAND_LEVEL: {
        typedef msg_blob_get_band_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        const SLuint16 slBand = blob.band;
        SLmillibel slLevel;

        result = equalizer.GetBandLevel(slBand, &slLevel);

        if (IS_SL_SUCCESS(result)) {
            (*blob.level) = slLevel;
        }
    } break;
    case MSG_GET_BAND_LEVEL_RANGE: {
        typedef msg_blob_get_band_level_range blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLmillibel slRange[2] = { 0, 0 };

#if 0 // don't use, because it may returns incorrect results
        result = equalizer.GetBandLevelRange(&slRange[0], &slRange[1]);
#else
        slRange[0] = props.levelRange.min;
        slRange[1] = props.levelRange.max;
        result = SL_RESULT_SUCCESS;
#endif

        if (IS_SL_SUCCESS(result)) {
            blob.range[0] = slRange[0];
            blob.range[1] = slRange[1];
        }
    } break;
    case MSG_GET_CENTER_FREQ: {
        typedef msg_blob_get_center_freq blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        const SLuint16 slBand = blob.band;
        SLmilliHertz slCenterFreq = 0;

        result = equalizer.GetCenterFreq(slBand, &slCenterFreq);

        if (IS_SL_SUCCESS(result)) {
            (*blob.center_freq) = slCenterFreq;
        }
    } break;
    case MSG_GET_CURRENT_PRESET: {
        typedef msg_blob_get_current_preset blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLuint16 slPreset = 0;
        result = equalizer.GetCurrentPreset(&slPreset);

        if (IS_SL_SUCCESS(result)) {
            (*blob.preset) = slPreset;
        }
    } break;
    case MSG_GET_NUMBER_OF_BANDS: {
        typedef msg_blob_get_number_of_bands blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLuint16 slNumBands = 0;
        result = equalizer.GetNumberOfBands(&slNumBands);

        if (IS_SL_SUCCESS(result)) {
            (*blob.num_bands) = slNumBands;
        }
    } break;
    case MSG_GET_NUMBER_OF_PRESETS: {
        typedef msg_blob_get_number_of_presets blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        SLuint16 slNumPresets = 0;
        result = equalizer.GetNumberOfPresets(&slNumPresets);

        if (IS_SL_SUCCESS(result)) {
            (*blob.num_presets) = slNumPresets;
        }
    } break;
    case MSG_GET_PRESET_NAME: {
        typedef msg_blob_get_preset_name blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        const SLchar *pName = nullptr;
        const SLuint16 slPreset = blob.preset;

        result = equalizer.GetPresetName(slPreset, &pName);
        if (IS_SL_SUCCESS(result)) {
            (*blob.name) = reinterpret_cast<const char *>(pName);
        }
    } break;
    case MSG_GET_PROPERTIES: {
        typedef msg_blob_get_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);
        result = GetProperties(equalizer, blob.settings);
    } break;
    case MSG_SET_BAND_LEVEL: {
        typedef msg_blob_set_band_level blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLuint16 slBand = blob.band;
            const SLmillibel slLevel = blob.level;

            result = equalizer.SetBandLevel(slBand, slLevel);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_USE_PRESET: {
        typedef msg_blob_use_preset blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            const SLuint16 slPreset = blob.preset;
            result = equalizer.UsePreset(slPreset);
        } else {
            result = SL_RESULT_CONTROL_LOST;
        }
    } break;
    case MSG_SET_PROPERTIES: {
        typedef msg_blob_set_properties blob_t;
        const blob_t &blob = GET_MSG_BLOB(*msg);

        if (CHECK_IS_ACTIVE(blob)) {
            result = SetProperties(equalizer, blob.settings);
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

void EqualizerExtModule::workAroundRefreshSettings(CSLEqualizerItf &equalizer) noexcept
{
    OpenSLMediaPlayerEqualizer::Settings settings;
    GetProperties(equalizer, &settings);
    SetProperties(equalizer, &settings);
}

} // namespace oslmp
