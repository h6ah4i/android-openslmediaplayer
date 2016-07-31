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

#ifndef OPENSLMEDIAPLAYERVISUALIZER_HPP_
#define OPENSLMEDIAPLAYERVISUALIZER_HPP_

#include <oslmp/OpenSLMediaPlayerAPICommon.hpp>

namespace oslmp {

class OpenSLMediaPlayerContext;

class OpenSLMediaPlayerVisualizer : public virtual android::RefBase {
public:
    struct MeasurementPeakRms;
    class OnDataCaptureListener;
    class InternalPeriodicCaptureThreadEventListener;

    enum { SCALING_MODE_NORMALIZED = 0, SCALING_MODE_AS_PLAYED = 1, };

    enum { MEASUREMENT_MODE_NONE = 0, MEASUREMENT_MODE_PEAK_RMS = 1, };

public:
    OpenSLMediaPlayerVisualizer(const android::sp<OpenSLMediaPlayerContext> &context) OSLMP_API_ABI;
    virtual ~OpenSLMediaPlayerVisualizer() OSLMP_API_ABI;

    int setEnabled(bool enabled) noexcept OSLMP_API_ABI;
    int getEnabled(bool *enabled) noexcept OSLMP_API_ABI;

    int getMaxCaptureRate(uint32_t *rate) const noexcept OSLMP_API_ABI;
    int getCaptureSizeRange(size_t *range) const noexcept OSLMP_API_ABI;
    int getCaptureSize(size_t *size) noexcept OSLMP_API_ABI;
    int setCaptureSize(size_t size) noexcept OSLMP_API_ABI;

    int getFft(int8_t *fft, size_t size) noexcept OSLMP_API_ABI;
    int getWaveForm(uint8_t *waveform, size_t size) noexcept OSLMP_API_ABI;

    int getMeasurementPeakRms(MeasurementPeakRms *measurement) noexcept OSLMP_API_ABI;
    int getSamplingRate(uint32_t *samplingRate) noexcept OSLMP_API_ABI;
    int getMeasurementMode(int32_t *mode) noexcept OSLMP_API_ABI;
    int setMeasurementMode(int32_t mode) noexcept OSLMP_API_ABI;
    int getScalingMode(int32_t *mode) noexcept OSLMP_API_ABI;
    int setScalingMode(int32_t mode) noexcept OSLMP_API_ABI;

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

struct OpenSLMediaPlayerVisualizer::MeasurementPeakRms {
    int32_t mPeak;
    int32_t mRms;

    MeasurementPeakRms() OSLMP_API_ABI : mPeak(0), mRms(0) {}
};

class OpenSLMediaPlayerVisualizer::OnDataCaptureListener : public virtual android::RefBase {
public:
    virtual ~OnDataCaptureListener() OSLMP_API_ABI {}

    virtual void onWaveFormDataCapture(OpenSLMediaPlayerVisualizer *visualizer, const uint8_t *waveform, size_t size,
                                       uint32_t samplingRate) noexcept OSLMP_API_ABI = 0;

    virtual void onFftDataCapture(OpenSLMediaPlayerVisualizer *visualizer, const int8_t *fft, size_t size,
                                  uint32_t samplingRate) noexcept OSLMP_API_ABI = 0;
};

class OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener
    : public virtual android::RefBase,
      public OpenSLMediaPlayerVisualizer::OnDataCaptureListener {
public:
    enum { BUFFER_TYPE_WAVEFORM, BUFFER_TYPE_FFT, };

    virtual ~InternalPeriodicCaptureThreadEventListener() OSLMP_API_ABI {}
    virtual void
    onEnterInternalPeriodicCaptureThread(OpenSLMediaPlayerVisualizer *visualizer) noexcept OSLMP_API_ABI = 0;
    virtual void
    onLeaveInternalPeriodicCaptureThread(OpenSLMediaPlayerVisualizer *visualizer) noexcept OSLMP_API_ABI = 0;

    virtual void onAllocateCaptureBuffer(OpenSLMediaPlayerVisualizer *visualizer, int32_t type,
                                         int32_t size) noexcept OSLMP_API_ABI = 0;
    virtual void onDeAllocateCaptureBuffer(OpenSLMediaPlayerVisualizer *visualizer,
                                           int32_t type) noexcept OSLMP_API_ABI = 0;
    virtual void *onLockCaptureBuffer(OpenSLMediaPlayerVisualizer *visualizer, int32_t type) noexcept OSLMP_API_ABI = 0;
    virtual void onUnlockCaptureBuffer(OpenSLMediaPlayerVisualizer *visualizer, int32_t type,
                                       void *buffer) noexcept OSLMP_API_ABI = 0;
};

} // namespace oslmp

#endif // OPENSLMEDIAPLAYERVISUALIZER_HPP_
