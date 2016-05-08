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

#ifndef OPENSLMEDIAPLAYERHQVISUALIZER_HPP_
#define OPENSLMEDIAPLAYERHQVISUALIZER_HPP_

#include <oslmp/OpenSLMediaPlayerAPICommon.hpp>

namespace oslmp {

class OpenSLMediaPlayerContext;

class OpenSLMediaPlayerHQVisualizer : public virtual android::RefBase {
public:
    struct MeasurementPeakRms;
    class OnDataCaptureListener;
    class InternalPeriodicCaptureThreadEventListener;

    enum {
        WINDOW_RECTANGULAR = 0,
        WINDOW_HANN = 1,
        WINDOW_HAMMING = 2,
        WINDOW_BLACKMAN = 3,
        WINDOW_FLAT_TOP = 4,
        WINDOW_OPT_APPLY_FOR_WAVEFORM = (1 << 31),
    };

public:
    OpenSLMediaPlayerHQVisualizer(const android::sp<OpenSLMediaPlayerContext> &context) OSLMP_API_ABI;
    virtual ~OpenSLMediaPlayerHQVisualizer() OSLMP_API_ABI;

    int setEnabled(bool enabled) noexcept OSLMP_API_ABI;
    int getEnabled(bool *enabled) noexcept OSLMP_API_ABI;

    int getMaxCaptureRate(uint32_t *rate) const noexcept OSLMP_API_ABI;
    int getCaptureSizeRange(size_t *range) const noexcept OSLMP_API_ABI;
    int getCaptureSize(size_t *size) noexcept OSLMP_API_ABI;
    int setCaptureSize(size_t size) noexcept OSLMP_API_ABI;

    int getSamplingRate(uint32_t *samplingRate) noexcept OSLMP_API_ABI;
    int getNumChannels(uint32_t *numChannels) noexcept OSLMP_API_ABI;

    int setWindowFunction(uint32_t windowType) noexcept OSLMP_API_ABI;
    int getWindowFunction(uint32_t *windowType) noexcept OSLMP_API_ABI;

    int setDataCaptureListener(OnDataCaptureListener *listener, uint32_t rate, bool waveform,
                               bool fft) noexcept OSLMP_API_ABI;

    int setInternalPeriodicCaptureThreadEventListener(InternalPeriodicCaptureThreadEventListener *listener,
                                                      uint32_t rate, bool waveform, bool fft) noexcept OSLMP_API_ABI;

    static int sGetMaxCaptureRate(uint32_t *rate) noexcept OSLMP_API_ABI;
    static int sGetCaptureSizeRange(size_t *range) noexcept OSLMP_API_ABI;

private:
    class Impl; // NOTE: do not use unique_ptr to avoid cxxporthelper dependencies
    Impl *impl_;
};

class OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener : public virtual android::RefBase {
public:
    virtual ~OnDataCaptureListener() OSLMP_API_ABI {}

    virtual void onWaveFormDataCapture(OpenSLMediaPlayerHQVisualizer *visualizer, const float *waveform,
                                       uint32_t numChannels, size_t sizeInFrames,
                                       uint32_t samplingRate) noexcept OSLMP_API_ABI = 0;

    virtual void onFftDataCapture(OpenSLMediaPlayerHQVisualizer *visualizer, const float *fft, uint32_t numChannels,
                                  size_t sizeInFrames, uint32_t samplingRate) noexcept OSLMP_API_ABI = 0;
};

class OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener
    : public virtual android::RefBase,
      public OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener {
public:
    enum { BUFFER_TYPE_WAVEFORM, BUFFER_TYPE_FFT, };

    virtual ~InternalPeriodicCaptureThreadEventListener() OSLMP_API_ABI {}
    virtual void
    onEnterInternalPeriodicCaptureThread(OpenSLMediaPlayerHQVisualizer *visualizer) noexcept OSLMP_API_ABI = 0;
    virtual void
    onLeaveInternalPeriodicCaptureThread(OpenSLMediaPlayerHQVisualizer *visualizer) noexcept OSLMP_API_ABI = 0;

    virtual void onAllocateCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer, int32_t type,
                                         int32_t size) noexcept OSLMP_API_ABI = 0;
    virtual void onDeAllocateCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer,
                                           int32_t type) noexcept OSLMP_API_ABI = 0;
    virtual void *onLockCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer,
                                      int32_t type) noexcept OSLMP_API_ABI = 0;
    virtual void onUnlockCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer, int32_t type,
                                       void *buffer) noexcept OSLMP_API_ABI = 0;
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYERHQVISUALIZER_HPP_
