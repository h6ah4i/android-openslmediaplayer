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

#include <android_native_app_glue.h>

#include "app_engine.hpp"

extern "C" {

void android_main(struct android_app *state)
{
    // NOTE: AppEngine extends android::RefBase, so have to use android::sp
    android::sp<AppEngine> engine;

    // Make sure glue isn't stripped.
    app_dummy();

    engine = new AppEngine();

    engine->initialize(state);

    while (true) {
        int ident;
        int events;
        struct android_poll_source *source;

        // process looper events
        while ((ident = ::ALooper_pollAll(engine->isActive() ? 0 : -1, NULL, &events, (void **)&source)) >= 0) {
            if (source != NULL) {
                source->process(state, source);
            }

            if (state->destroyRequested) {
                break;
            }
        }

        if (state->destroyRequested) {
            break;
        }

        if (engine->isActive()) {
            engine->render();
        }
    }

    engine->terminate();
}
}
