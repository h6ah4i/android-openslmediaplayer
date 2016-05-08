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

#ifndef OPENSLMEDIAPLAYERINTERNALCONTEXT_HPP_
#define OPENSLMEDIAPLAYERINTERNALCONTEXT_HPP_

#include <cxxporthelper/cstddef>

#include <jni.h>

#include <SLESCXX/OpenSLES_CXX.hpp>
#include <utils/RefBase.h>
#include <oslmp/OpenSLMediaPlayerContext.hpp>
#include "oslmp/impl/OpenSLMediaPlayerExtension.hpp"
#include "oslmp/impl/DebugFeatures.hpp"

//
// forward declarations
//
namespace oslmp {
class OpenSLMediaPlayerContext;
}

namespace oslmp {
namespace impl {
struct OpenSLMediaPlayerThreadMessage;
class AudioSystem;

#if USE_OSLMP_DEBUG_FEATURES
class NonBlockingTraceLogger;
#endif
} // namespace impl
} // namespace oslmp

namespace oslmp {
namespace impl {

// Message handler
class OpenSLMediaPlayerInternalMessageHandler : public virtual android::RefBase {
public:
    virtual ~OpenSLMediaPlayerInternalMessageHandler() {}

    virtual int onRegisteredAsMessageHandler() noexcept = 0;
    virtual void onUnregisteredAsMessageHandler() noexcept = 0;
    virtual void onHandleMessage(const void *msg) noexcept = 0;
};

typedef uintptr_t OpenSLMediaPlayerInternalMessageHandlerToken;

//
// Internal context interface
//

class OpenSLMediaPlayerInternalContext : public virtual android::RefBase, public OpenSLMediaPlayerExtensionManager {
public:
    virtual ~OpenSLMediaPlayerInternalContext() {}

    // methods
    virtual JavaVM *getJavaVM() const noexcept = 0;

    virtual SLresult getInterfaceFromEngine(opensles::CSLInterface *itf) noexcept = 0;
    virtual SLresult getInterfaceFromOutputMixer(opensles::CSLInterface *itf) noexcept = 0;
    virtual SLresult getInterfaceFromSinkPlayer(opensles::CSLInterface *itf) noexcept = 0;
    virtual uint32_t getContextOptions() const noexcept = 0;
    virtual OpenSLMediaPlayerContext::InternalThreadEventListener *getInternalThreadEventListener() const noexcept = 0;
    virtual AudioSystem *getAudioSystem() const noexcept = 0;

    virtual bool registerMessageHandler(OpenSLMediaPlayerInternalMessageHandler *handler,
                                        OpenSLMediaPlayerInternalMessageHandlerToken *token) noexcept = 0;
    virtual bool unregisterMessageHandler(OpenSLMediaPlayerInternalMessageHandler *handler,
                                          OpenSLMediaPlayerInternalMessageHandlerToken token) noexcept = 0;
    virtual bool postMessage(OpenSLMediaPlayerInternalMessageHandler *handler,
                             OpenSLMediaPlayerInternalMessageHandlerToken token, void *data, size_t size) noexcept = 0;

    virtual void raiseOnBeforeAudioSinkStateChanged(bool next_is_started) noexcept = 0;

#if USE_OSLMP_DEBUG_FEATURES
    virtual NonBlockingTraceLogger &getNonBlockingTraceLogger() const noexcept = 0;
#endif

    // OpenSLMediaPlayerContext -> OpenSLMediaPlayerInternalContext
    static OpenSLMediaPlayerInternalContext &sGetInternal(OpenSLMediaPlayerContext &c) noexcept;
};

} // namespace impl
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERINTERNALCONTEXT_HPP_
