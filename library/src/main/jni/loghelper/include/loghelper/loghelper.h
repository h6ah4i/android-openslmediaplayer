/* *    Copyright (C) 2014 Haruki Hasegawa
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/*
 * loghelper.h
 *
 *  Created on: Jan 26, 2013
 *      Author: hasegawa
 */

#ifndef LOGHELPER_H_
#define LOGHELPER_H_

#include <android/log.h>

#ifdef LOG_TAG
#define LOG_PRINT(LEVEL_, LOG_TAG_, ...) __android_log_print(LEVEL_, LOG_TAG_, __VA_ARGS__)

#define LOGV(...) LOG_PRINT(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) LOG_PRINT(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) LOG_PRINT(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) LOG_PRINT(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) LOG_PRINT(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...) LOG_PRINT(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#else

#define LOGV(...)
#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGE(...)
#define LOGF(...)
#endif

#endif // LOGHELPER_H_
