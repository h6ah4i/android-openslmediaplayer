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


LOCAL_PATH := $(call my-dir)

CXXDASP_TOP_DIR := $(LOCAL_PATH)/../dep_libs/cxxdasp
CXXPORTHELPER_TOP_DIR := $(CXXDASP_TOP_DIR)/android/dep_libs/cxxporthelper
OPENSLESCXX_TOP_DIR := $(LOCAL_PATH)/../dep_libs/openslescxx
LFDS_TOP_DIR := $(LOCAL_PATH)/../dep_libs/lockfreedatastructure



# Override cxxdasp configurations
include $(LOCAL_PATH)/openslmediaplayer-cxxdasp-config.mk
include $(CXXDASP_TOP_DIR)/android/build-utils/cxxdasp-build-setup.mk

# Add cxxporthelper's path
$(call import-add-path, $(CXXPORTHELPER_TOP_DIR)/android/module)

# Add openslescxx'path
$(call import-add-path, $(OPENSLESCXX_TOP_DIR)/android/module)

# Add lockfreedatastructure'path
$(call import-add-path, $(LFDS_TOP_DIR)/android/module)
