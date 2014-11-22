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

#ifndef OPENSLMEDIAPLAYERHQVISUALIZERJNIBINDER_HPP_
#define OPENSLMEDIAPLAYERHQVISUALIZERJNIBINDER_HPP_

#include <jni.h>
#include <jni_utils/jni_utils.hpp>
#include <utils/RefBase.h>
#include <oslmp/OpenSLMediaPlayer.hpp>
#include <oslmp/OpenSLMediaPlayerHQVisualizer.hpp>

namespace oslmp {
namespace jni {

class OpenSLMediaPlayerHQVisualizerJNIBinder
    : public virtual android::RefBase,
      public OpenSLMediaPlayerHQVisualizer::InternalPeriodicCaptureThreadEventListener {
public:
    OpenSLMediaPlayerHQVisualizerJNIBinder(JNIEnv *env, jclass clazz, jobject weak_thiz);
    virtual ~OpenSLMediaPlayerHQVisualizerJNIBinder();

    int bind(const android::sp<OpenSLMediaPlayerHQVisualizer> &visualizer, uint32_t rate, bool waveform,
             bool fft) noexcept;
    int unbind(const android::sp<OpenSLMediaPlayerHQVisualizer> &visualizer) noexcept;

    // implementations of InternalPeriodicCaptureThreadEventListener
    virtual void onEnterInternalPeriodicCaptureThread(OpenSLMediaPlayerHQVisualizer *visualizer) noexcept override;
    virtual void onLeaveInternalPeriodicCaptureThread(OpenSLMediaPlayerHQVisualizer *visualizer) noexcept override;
    virtual void onAllocateCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer, int32_t type,
                                         int32_t size) noexcept override;
    virtual void onDeAllocateCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer, int32_t type) noexcept override;
    virtual void *onLockCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer, int32_t type) noexcept override;
    virtual void onUnlockCaptureBuffer(OpenSLMediaPlayerHQVisualizer *visualizer, int32_t type,
                                       void *buffer) noexcept override;

    virtual void onWaveFormDataCapture(OpenSLMediaPlayerHQVisualizer *visualizer, const float *waveform,
                                       uint32_t numChannels, size_t sizeInFrames,
                                       uint32_t samplingRate) noexcept override;

    virtual void onFftDataCapture(OpenSLMediaPlayerHQVisualizer *visualizer, const float *fft, uint32_t numChannels,
                                  size_t sizeInFrames, uint32_t samplingRate) noexcept override;

private:
    // inhibit copy operations
    OpenSLMediaPlayerHQVisualizerJNIBinder(const OpenSLMediaPlayerHQVisualizerJNIBinder &) = delete;
    OpenSLMediaPlayerHQVisualizerJNIBinder &operator=(const OpenSLMediaPlayerHQVisualizerJNIBinder &) = delete;

    void raiseCaptureEvent(jint type, jfloatArray data, jint numChannels, jint samplingRate) noexcept;

private:
    enum {
        // NOTE:
        // Only needs a single buffer. Because HQVisualizer doesn't uses Handler in Java layer.
        NUM_BUFFERS = 1,
    };

    JavaVM *jvm_;
    JNIEnv *env_;
    bool jvm_attached_;
    jglobal_ref_wrapper<jclass> jvisualizer_class_;
    jglobal_ref_wrapper<jobject> jvisualizer_weak_thiz_;
    jglobal_ref_wrapper<jfloatArray> jwaveform_data_[NUM_BUFFERS];
    jglobal_ref_wrapper<jfloatArray> jfft_data_[NUM_BUFFERS];
    jmethodID methodIdRaiseCaptureEventFromNative_;
    android::wp<OpenSLMediaPlayerHQVisualizer> visualizer_;
    int waveform_buffer_index_;
    int fft_buffer_index_;
};

} // namespace jni
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERHQVISUALIZERJNIBINDER_HPP_
