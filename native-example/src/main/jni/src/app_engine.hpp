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

#ifndef APP_ENGINE_HPP_
#define APP_ENGINE_HPP_

#include <string>
#include <vector>

#include <pthread.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android_native_app_glue.h>
#include <oslmp/OpenSLMediaPlayer.hpp>
#include <oslmp/OpenSLMediaPlayerHQVisualizer.hpp>

#include "app_command_pipe.hpp"
#include "FloatClolor.hpp"

class AppEngine : public AppCommandPipe::EventHandler,
                  public oslmp::OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener {
public:
    AppEngine();
    virtual ~AppEngine();

    void initialize(android_app *app);
    void terminate();

    bool isActive() const;
    void render();

    // implementation of AppCommandPipe::EventHandler
    virtual void onHandleAppComand_SetDataAndPrepare(const char *file_path) override;
    virtual void onHandleAppComand_Play() override;
    virtual void onHandleAppComand_Pause() override;

    // implementation of oslmp::OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener
    virtual void onWaveFormDataCapture(oslmp::OpenSLMediaPlayerHQVisualizer *visualizer, const float *waveform,
                                       uint32_t numChannels, size_t sizeInFrames,
                                       uint32_t samplingRate) noexcept override OSLMP_API_ABI;

    virtual void onFftDataCapture(oslmp::OpenSLMediaPlayerHQVisualizer *visualizer, const float *fft,
                                  uint32_t numChannels, size_t sizeInFrames,
                                  uint32_t samplingRate) noexcept override OSLMP_API_ABI;

    static AppCommandPipe *sGetAppCommandPipeFromHandle(jlong handle);

private:
    static void onHandleAndroidAppCommand(struct android_app *app, int32_t cmd);
    static int32_t onHandleInput(struct android_app *app, AInputEvent *event);

    void onHandleAppInitWindow();
    void onHandleAppSaveState();
    void onHandleAppTermWindow();
    void onHandleAppStart();
    void onHandleAppStop();
    void onHandleAppConfigChanged();

    bool initDisplay();
    void termDisplay();
    void configureRenderStates();

    void onRender();
    void onRenderWaveform(const float *waveform);
    void onRenderFft(const float *fft);
    void swapBuffers();

    bool initOSLMPContext();
    void releaseOSLMPResources();
    bool createVisualizer();

    bool startPlayback();
    bool pausePlayback();
    bool startVisualizer();
    bool stopVisualizer();

    static void makeXPointPositionData(int N, float *points);
    static void makeWaveformYPointPositionData(const float *waveform, int n, int skip, int offset, float *points);
    static void makeFftYPointPositionData(const float *fft, int n, int offset, float *points);

    static void drawWaveForm(int width, int height, const float *vertices, int n, int vposition, float yrange,
                             const FloatColor &color);

    static void drawFft(int width, int height, const float *vertices, int n, int vposition, float yrange,
                        const FloatColor &color);

private:
    struct saved_state {
        int dummy; // not used
    };

    android_app *app_;
    JNIEnv *env_;
    AppCommandPipe *cmd_pipe_;
    saved_state state_;

    EGLDisplay egl_display_;
    EGLSurface egl_surface_;
    EGLContext egl_context_;
    int32_t disp_width_;
    int32_t disp_height_;

    int active_state_counter_;

    pthread_mutex_t visualizer_mutex_;
    std::vector<float> waveform_buffers_[2];
    std::vector<float> fft_buffers_[2];
    std::vector<float> work_waveform_vertices_buff_;
    std::vector<float> work_fft_vertices_buff_;
    int active_waveform_buffer_index_;
    int active_fft_buffer_index_;
    int prev_render_active_waveform_buffer_index_;
    int prev_render_active_fft_buffer_index_;
    int capture_size_in_frames_;
    int capture_num_channels_;

    android::sp<oslmp::OpenSLMediaPlayerContext> player_context_;
    android::sp<oslmp::OpenSLMediaPlayer> player_;
    android::sp<oslmp::OpenSLMediaPlayerHQVisualizer> hq_visualizer_;
};

#endif // APP_ENGINE_HPP_
