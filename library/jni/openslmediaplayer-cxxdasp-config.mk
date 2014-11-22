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

# config (default FFT library: PFFFT)
CXXDASP_USE_FFT_BACKEND_FFTS := 0
CXXDASP_USE_FFT_BACKEND_PFFFT := 1
CXXDASP_USE_FFT_BACKEND_KISS_FFT := 0
CXXDASP_USE_FFT_BACKEND_NE10 := 0
CXXDASP_USE_FFT_BACKEND_GP_FFT := 0

# use Ne10 library for armeabi-v7a, armeabi-v7a-hard
ifneq (, $(filter armeabi-v7a | armeabi-v7a-hard, $(TARGET_ARCH_ABI)))
    CXXDASP_USE_FFT_BACKEND_PFFFT := 0
    CXXDASP_USE_FFT_BACKEND_NE10 := 1
endif

CXXDASP_ENABLE_POLYPHASE_RESAMPLER_FACTORY_LOW_QUALITY := 1
CXXDASP_ENABLE_POLYPHASE_RESAMPLER_FACTORY_HIGH_QUALITY := 1

# set override flag
CXXDASP_OVERRIDE_CONFIG_FILE := 1
