#
# Detect NDK version
#
#OSLMP_NDK_RELEASE_VERSION_FULL := $(firstword $(strip $(shell cat $(NDK_ROOT)/RELEASE.TXT)))
#OSLMP_NDK_RELEASE_VERSION_NUMBER := $(shell echo $(OSLMP_NDK_RELEASE_VERSION_FULL) | $(HOST_SED) 's/^r\([0-9]\+\).*$$/\1/g')
OSLMP_NDK_RELEASE_VERSION_NUMBER := $(firstword $(strip $(shell awk 'BEGIN{ file="'$(NDK_ROOT)/RELEASE.TXT'";while ((getline<file) > 0){ if (match($$0,/[0-9]+/)) { print substr($$0, RSTART, RLENGTH); } } }')))
#$(info OSLMP_NDK_RELEASE_VERSION_NUMBER = $(OSLMP_NDK_RELEASE_VERSION_NUMBER))

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

APP_ABI := all
#APP_ABI := armeabi-v7a
APP_ABI := arm64-v8a
APP_PLATFORM := android-14
APP_STL := stlport_static
APP_CPPFLAGS := -std=gnu++11 -fexceptions -frtti -pthread
