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

# NOTEs: 
#  1. If you choosed the gnustl for STL, the built library 
#     has to be released under GPL 3.
#  2. LLVM's libc++ and stlport is released under more 
#     relaxed licenses.
#  3. Binary size:  libc++ >> gnustl > stlport

#
# Detect NDK version
#

# $(NDK_ROOT)/RELEASE.TXT
OSLMP_NDK_RELEASE_VERSION_NUMBER := $(firstword $(strip $(shell awk 'BEGIN{ file="'$(NDK_ROOT)/RELEASE.TXT'";while ((getline<file) > 0){ if (match($$0,/[0-9]+/)) { print substr($$0, RSTART, RLENGTH); } } }')))
# $(NDK_ROOT)/source.properties
ifndef OSLMP_NDK_RELEASE_VERSION_NUMBER
    OSLMP_NDK_RELEASE_VERSION_NUMBER := $(firstword $(strip $(shell awk 'BEGIN{ file="'$(NDK_ROOT)/source.properties'";while ((getline<file) > 0){ if ($$0 ~ /Pkg.Revision = / && match($$0,/[0-9]+/)) { print substr($$0, RSTART, RLENGTH); } } }')))
endif
# $(info OSLMP_NDK_RELEASE_VERSION_NUMBER = $(OSLMP_NDK_RELEASE_VERSION_NUMBER))

# 
#
# NDK_TOOLCHAIN_VERSION
#
ifeq ($(OSLMP_NDK_RELEASE_VERSION_NUMBER), 9)
# NDK r9
NDK_TOOLCHAIN_VERSION := 4.8
# NDK_TOOLCHAIN_VERSION := clang3.4
else ifeq ($(OSLMP_NDK_RELEASE_VERSION_NUMBER), 10)
# NDK r10
NDK_TOOLCHAIN_VERSION := 4.9
# NDK_TOOLCHAIN_VERSION := clang3.5
else
# use default compiler
endif

#
# APP_STL
# (IMPL=[gnustl | c++ | stlport], LIBTYPE=[static | shared])
#
APP_STL_IMPL := stlport
APP_STL_LIBTYPE := static
APP_STL := $(APP_STL_IMPL)_$(APP_STL_LIBTYPE)

#
# APP_PLATFORM
#
APP_PLATFORM := android-14

#
# APP_OPTIM
#
# APP_OPTIM := debug
# APP_OPTIM := release

#
# APP_ABI
#
APP_ABI := 
APP_ABI += armeabi
# use armeabi-v7a (deprecated in NDK version >= 12)
ifneq (,$(filter $(OSLMP_NDK_RELEASE_VERSION_NUMBER), 9 10 11))
APP_ABI += armeabi-v7a-hard
else
APP_ABI += armeabi-v7a
endif
APP_ABI += x86
APP_ABI += mips
ifneq ($(OSLMP_NDK_RELEASE_VERSION_NUMBER), 9)
APP_ABI += arm64-v8a
APP_ABI += x86_64
APP_ABI += mips64
endif

#
# APP_CFLAGS
#
ifeq ($(APP_STL_IMPL), gnustl_static)
    APP_CFLAGS += -DAPP_STL_GNUSTL
endif
ifeq ($(APP_STL_IMPL), libc++)
    APP_CFLAGS += -DAPP_STL_LIBCXX
    APP_CFLAGS += -Wno-typedef-redefinition -Wno-string-plus-int -Wno-shift-op-parentheses
endif
ifeq ($(APP_STL_IMPL), stlport)
    APP_CFLAGS += -DAPP_STL_STLPORT
endif

ifeq ($(APP_OPTIM), release)
    APP_CFLAGS += -DNDEBUG
endif

# include paths
APP_CFLAGS := \
    -I$(call my-dir)/android-platform-system-core/include \
    -I$(call my-dir)/android-platform-system-media/include

# APP_CPPFLAGS
APP_CPPFLAGS += -std=gnu++11 -fexceptions -frtti -pthread

# APP_MODULES
APP_MODULES := \
    OpenSLMediaPlayerLibLoaderHelper \
    OpenSLMediaPlayer \
    OpenSLMediaPlayer-no-neon \
    OpenSLMediaPlayerJNI
