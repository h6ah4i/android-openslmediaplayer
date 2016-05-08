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

#ifndef OPENSLMEDIAPLAYERVISUALIZERJNIBINDER_HPP_
#define OPENSLMEDIAPLAYERVISUALIZERJNIBINDER_HPP_

#include <jni.h>
#include <jni_utils/jni_utils.hpp>
#include <utils/RefBase.h>
#include <oslmp/OpenSLMediaPlayer.hpp>
#include <oslmp/OpenSLMediaPlayerVisualizer.hpp>

namespace oslmp {
namespace jni {

class OpenSLMediaPlayerVisualizerJNIBinder
    : public virtual android::RefBase,
      public OpenSLMediaPlayerVisualizer::InternalPeriodicCaptureThreadEventListener {
public:
    OpenSLMediaPlayerVisualizerJNIBinder(JNIEnv *env, jclass clazz, jobject weak_thiz);
    virtual ~OpenSLMediaPlayerVisualizerJNIBinder();

    int bind(const android::sp<OpenSLMediaPlayerVisualizer> &visualizer, uint32_t rate, bool waveform,
             bool fft) noexcept;
    int unbind(const android::sp<OpenSLMediaPlayerVisualizer> &visualizer) noexcept;

    // implementations of InternalPeriodicCaptureThreadEventListener
    virtual void onEnterInternalPeriodicCaptureThread(OpenSLMediaPlayerVisualizer *visualizer) noexcept override;
    virtual void onLeaveInternalPeriodicCaptureThread(OpenSLMediaPlayerVisualizer *visualizer) noexcept override;
    virtual void onAllocateCaptureBuffer(OpenSLMediaPlayerVisualizer *visualizer, int32_t type,
                                         int32_t size) noexcept override;
    virtual void onDeAllocateCaptureBuffer(OpenSLMediaPlayerVisualizer *visualizer, int32_t type) noexcept override;
    virtual void *onLockCaptureBuffer(OpenSLMediaPlayerVisualizer *visualizer, int32_t type) noexcept override;
    virtual void onUnlockCaptureBuffer(OpenSLMediaPlayerVisualizer *visualizer, int32_t type,
                                       void *buffer) noexcept override;

    virtual void onWaveFormDataCapture(OpenSLMediaPlayerVisualizer *visualizer, const uint8_t *waveform, size_t size,
                                       uint32_t samplingRate) noexcept override;

    virtual void onFftDataCapture(OpenSLMediaPlayerVisualizer *visualizer, const int8_t *fft, size_t size,
                                  uint32_t samplingRate) noexcept override;

private:
    // inhibit copy operations
    OpenSLMediaPlayerVisualizerJNIBinder(const OpenSLMediaPlayerVisualizerJNIBinder &) = delete;
    OpenSLMediaPlayerVisualizerJNIBinder &operator=(const OpenSLMediaPlayerVisualizerJNIBinder &) = delete;

    void raiseCaptureEvent(jint type, jbyteArray data, jint samplingRate) noexcept;

private:
    enum {
        // NOTE:
        // Needs multiple buffers to avoid data corruption.
        // In Java layer, event callback is delegated by Looper thread.
        NUM_BUFFERS = 4,
    };

    JavaVM *jvm_;
    JNIEnv *env_;
    bool jvm_attached_;
    jglobal_ref_wrapper<jclass> jvisualizer_class_;
    jglobal_ref_wrapper<jobject> jvisualizer_weak_thiz_;
    jglobal_ref_wrapper<jbyteArray> jwaveform_data_[NUM_BUFFERS];
    jglobal_ref_wrapper<jbyteArray> jfft_data_[NUM_BUFFERS];
    jmethodID methodIdRaiseCaptureEventFromNative_;
    android::wp<OpenSLMediaPlayerVisualizer> visualizer_;
    int waveform_buffer_index_;
    int fft_buffer_index_;
};

} // namespace jni
} // namespace oslmp

#endif // OPENSLMEDIAPLAYERVISUALIZERJNIBINDER_HPP_
