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

PROJ_LOCAL_PATH := $(call my-dir)

#
# Common
#
OSLMP_LOCAL_C_INCLUDES := $(PROJ_LOCAL_PATH)/include $(PROJ_LOCAL_PATH)/internal_include

# enumerate all *.cpp files in the /source directory
OSLMP_LOCAL_SRC_FILES := $(wildcard $(PROJ_LOCAL_PATH)/source/*.cpp)
# remove unnecessary $(LOCAL_PATH)
OSLMP_LOCAL_SRC_FILES := $(subst $(PROJ_LOCAL_PATH)/,, $(OSLMP_LOCAL_SRC_FILES))

# set USE_OSLMP_DEBUG_FEATURES
ifeq ($(APP_OPTIM), debug)
	OSLMP_LOCAL_CFLAGS += -DUSE_OSLMP_DEBUG_FEATURES
endif
OSLMP_LOCAL_CFLAGS += -DUSE_OSLMP_DEBUG_FEATURES

OSLMP_LOCAL_EXPORT_C_INCLUDES := $(PROJ_LOCAL_PATH)/include


#
# OpenSLMediaPlayer
#
LOCAL_PATH := $(PROJ_LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE    := OpenSLMediaPlayer
LOCAL_C_INCLUDES := $(OSLMP_LOCAL_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDES := $(OSLMP_LOCAL_EXPORT_C_INCLUDES)
LOCAL_SRC_FILES := $(OSLMP_LOCAL_SRC_FILES)
LOCAL_CFLAGS += $(OSLMP_LOCAL_CFLAGS)
LOCAL_STATIC_LIBRARIES := \
    android-platform-system-core-utils \
    android-platform-system-media-audio_utils \
    cxxdasp_static \
    cxxdasp_$(TARGET_ARCH_ABI)_static \
    lockfreedatastructure_static \
    cxxporthelper_static \
    $(CXXDASP_FFT_BACKEND_LIBS_$(TARGET_ARCH_ABI)) \
    openslescxx_static \
    loghelper \
    jni_utils

ifneq (, $(filter armeabi-v7a armeabi-v7a-hard, $(TARGET_ARCH_ABI)))
    LOCAL_ARM_NEON  := true
endif

include $(BUILD_SHARED_LIBRARY)


#
# OpenSLMediaPlayer-no-neon
#
LOCAL_PATH := $(PROJ_LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE    := OpenSLMediaPlayer-no-neon

ifneq (, $(filter armeabi-v7a armeabi-v7a-hard, $(TARGET_ARCH_ABI)))
LOCAL_C_INCLUDES := $(OSLMP_LOCAL_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDES := $(OSLMP_LOCAL_EXPORT_C_INCLUDES)
LOCAL_SRC_FILES := $(OSLMP_LOCAL_SRC_FILES)
LOCAL_CFLAGS += $(OSLMP_LOCAL_CFLAGS)
LOCAL_STATIC_LIBRARIES := \
    android-platform-system-core-utils \
    android-platform-system-media-audio_utils \
    cxxdasp_static \
    cxxdasp_$(TARGET_ARCH_ABI)-no-neon_static \
    lockfreedatastructure_static \
    cxxporthelper_static \
    $(CXXDASP_FFT_BACKEND_LIBS_$(TARGET_ARCH_ABI)-no-neon) \
    openslescxx_static \
    loghelper \
    jni_utils

include $(BUILD_SHARED_LIBRARY)
else
# dummy
include $(BUILD_STATIC_LIBRARY)
endif


#
# Clear variables
#
OSLMP_LOCAL_C_INCLUDES :=
OSLMP_LOCAL_SRC_FILES :=
OSLMP_LOCAL_CFLAGS :=
OSLMP_LOCAL_EXPORT_C_INCLUDES :=
