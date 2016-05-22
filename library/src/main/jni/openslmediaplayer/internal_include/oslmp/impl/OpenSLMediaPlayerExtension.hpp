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

#ifndef OPENSLMEDIAPLAYEREXTENSION_HPP_
#define OPENSLMEDIAPLAYEREXTENSION_HPP_

#include <jni.h>
#include <time.h>
#include <utils/RefBase.h>
#include <SLESCXX/OpenSLES_CXX.hpp>
#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>
#include "oslmp/impl/OpenSLMediaPlayerInternalDefs.hpp"

//
// forward declarations
//
namespace oslmp {
class OpenSLMediaPlayer;
} // namespace oslmp

namespace oslmp {
namespace impl {
struct OpenSLMediaPlayerThreadMessage;
class OpenSLMediaPlayerExtension;
class OpenSLMediaPlayerExtensionManager;
class OpenSLMediaPlayerInternalContext;
class PreAmp;
class HQEqualizer;
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

typedef uintptr_t OpenSLMediaPlayerExtensionToken;

class OpenSLMediaPlayerExtension {
public:
    enum { TRAIT_NONE = 0UL, TRAIT_CAPTURE_AUDIO_DATA = (1UL << 0UL), };

    virtual ~OpenSLMediaPlayerExtension() {}

    virtual const char *getExtensionModuleName() const noexcept = 0;

    virtual uint32_t getExtensionTraits() const noexcept = 0;

    /* basic events */
    virtual bool onInstall(OpenSLMediaPlayerExtensionManager *extmgr, OpenSLMediaPlayerExtensionToken token,
                           void *user_arg) noexcept = 0;

    virtual void onAttach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept = 0;

    virtual void onDetach(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept = 0;

    virtual void onUninstall(OpenSLMediaPlayerExtensionManager *extmgr, void *user_arg) noexcept = 0;

    virtual void onHandleMessage(OpenSLMediaPlayerExtensionManager *extmgr,
                                 const OpenSLMediaPlayerThreadMessage *msg) noexcept = 0;

    virtual void onBeforeAudioSinkStateChanged(OpenSLMediaPlayerExtensionManager *extmgr,
                                               bool next_is_started) noexcept = 0;

    /* optional; need TRAIT_CAPTURE_AUDIO_DATA */
    virtual void onCaptureAudioData(OpenSLMediaPlayerExtensionManager *extmgr, const float *data, uint32_t num_channels,
                                    size_t num_frames, uint32_t sample_rate_millihertz,
                                    const timespec *timestamp) noexcept = 0;
};

class OpenSLMediaPlayerExtensionCreator {
public:
    virtual ~OpenSLMediaPlayerExtensionCreator() {}

    virtual const char *getModuleName() const noexcept = 0;
    virtual OpenSLMediaPlayerExtension *createNewInstance() const noexcept = 0;
};

class OpenSLMediaPlayerExtensionManager : public virtual android::RefBase {
public:
    virtual ~OpenSLMediaPlayerExtensionManager() {}

    virtual int extAttachOrInstall(OpenSLMediaPlayerExtension **attached_extension,
                                   const OpenSLMediaPlayerExtensionCreator *creator, void *user_arg) noexcept = 0;

    virtual int extDetachOrUninstall(OpenSLMediaPlayerExtension *extension, OpenSLMediaPlayerExtensionToken token,
                                     void *user_arg) noexcept = 0;

    virtual bool extPostMessage(OpenSLMediaPlayerExtension *handler, OpenSLMediaPlayerExtensionToken token, void *msg,
                                size_t size) noexcept = 0;

    virtual SLresult extGetInterfaceFromPlayer(opensles::CSLInterface *pInterface, bool instantiate = false) noexcept = 0;
    virtual SLresult extGetInterfaceFromOutputMix(opensles::CSLInterface *pInterface) noexcept = 0;

    virtual SLresult extReleaseInterfaceFromPlayer(opensles::CSLInterface *pInterface) noexcept = 0;

    virtual int extTranslateOpenSLErrorCode(SLresult result) const noexcept = 0;

    virtual bool extNotifyTraitsUpdated(OpenSLMediaPlayerExtensionToken token) noexcept = 0;

    virtual uint32_t extGetOutputLatency() const noexcept = 0;
    virtual uint32_t extGetOutputSamplingRate() const noexcept = 0;

    virtual int extSetAuxEffectEnabled(int aux_effect_id, bool enabled) noexcept = 0;

    virtual int extGetPreAmp(PreAmp **p_preamp) const noexcept = 0;
    virtual int extGetHQEqualizer(HQEqualizer **p_hq_equalizer) const noexcept = 0;

    virtual JavaVM *extGetJavaVM() const noexcept = 0;
};

} // namespace impl
} // namespace oslmp

#endif // OPENSLMEDIAPLAYEREXTENSION_HPP_
