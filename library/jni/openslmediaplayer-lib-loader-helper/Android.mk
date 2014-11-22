#
#    Copyright (C) 2014 Haruki Hasegawa
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
# OpenSLMediaPlayerLibLoaderHelper
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := OpenSLMediaPlayerLibLoaderHelper
LOCAL_STATIC_LIBRARIES := cpufeatures

# enumerate all *.cpp files in the /source directory
LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/source/*.cpp)
# remove unnecessary $(LOCAL_PATH)
LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/,, $(LOCAL_SRC_FILES))

include $(BUILD_SHARED_LIBRARY)
