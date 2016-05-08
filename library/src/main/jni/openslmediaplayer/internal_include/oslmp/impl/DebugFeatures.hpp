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

#ifndef DEBUGFEATURES_HPP_
#define DEBUGFEATURES_HPP_

#include <cxxporthelper/memory>

#if USE_OSLMP_DEBUG_FEATURES
#include "oslmp/impl/NonBlockingTraceLogger.hpp"
#endif

// NB_LOGx macros
#if defined(USE_OSLMP_DEBUG_FEATURES) && defined(NB_LOG_TAG)

#define REF_NB_LOGGER_CLIENT(client) auto &local_ref_oslmp_nb_logger = (client)
#define SAFE_LITERAL_STR(str) ("" str)
#define NB_LOG_PRINT(log_level_, log_tag_, format, ...)                                                                \
    do {                                                                                                               \
        if (CXXPH_LIKELY(local_ref_oslmp_nb_logger)) {                                                                 \
            local_ref_oslmp_nb_logger->putf((log_level_), SAFE_LITERAL_STR(log_tag_), (format), ##__VA_ARGS__);        \
        }                                                                                                              \
    } while (0)

#define NB_LOGV(format, ...) NB_LOG_PRINT(NonBlockingTraceLogger::LOG_LEVEL_VERBOSE, NB_LOG_TAG, format, ##__VA_ARGS__)
#define NB_LOGD(format, ...) NB_LOG_PRINT(NonBlockingTraceLogger::LOG_LEVEL_DEBUG, NB_LOG_TAG, format, ##__VA_ARGS__)
#define NB_LOGI(format, ...) NB_LOG_PRINT(NonBlockingTraceLogger::LOG_LEVEL_INFO, NB_LOG_TAG, format, ##__VA_ARGS__)
#define NB_LOGW(format, ...) NB_LOG_PRINT(NonBlockingTraceLogger::LOG_LEVEL_WARNING, NB_LOG_TAG, format, ##__VA_ARGS__)
#define NB_LOGE(format, ...) NB_LOG_PRINT(NonBlockingTraceLogger::LOG_LEVEL_ERROR, NB_LOG_TAG, format, ##__VA_ARGS__)
#define NB_LOGF(format, ...) NB_LOG_PRINT(NonBlockingTraceLogger::LOG_LEVEL_FATAL, NB_LOG_TAG, format, ##__VA_ARGS__)

#else

#define REF_NB_LOGGER_CLIENT(client)

#define NB_LOGV(format, ...)
#define NB_LOGD(format, ...)
#define NB_LOGI(format, ...)
#define NB_LOGW(format, ...)
#define NB_LOGE(format, ...)
#define NB_LOGF(format, ...)

#endif

#endif // DEBUGFEATURES_HPP_
