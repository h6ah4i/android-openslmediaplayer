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

#define LOG_TAG "AppEngine"

#include <algorithm>
#include <errno.h>

#include <android/log.h>

#include "app_engine.hpp"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))

enum { LOOPER_ID_APP_COMMAND = LOOPER_ID_USER + 0, };

// http://www.riken.jp/brict/Ijiri/study/fastsqrt.html
static inline float t_sqrtF(const float &x)
{
    float xHalf = 0.5f * x;
    int tmp = 0x5F3759DF - (*(int *)&x >> 1); // initial guess
    float xRes = *(float *)&tmp;

    xRes *= (1.5f - (xHalf * xRes * xRes));
    // xRes *= ( 1.5f - ( xHalf * xRes * xRes ) );
    return xRes * x;
}

inline float fastSqrt(float x) { return t_sqrtF(x); }

#define ACTIVE_STATE_DURATION 60 // about 1 sec.   @ 60 fps
int32_t AppEngine::onHandleInput(struct android_app *app, AInputEvent *event)
{
    AppEngine *thiz = reinterpret_cast<AppEngine *>(app->userData);

    switch (::AInputEvent_getType(event)) {
    case AINPUT_EVENT_TYPE_KEY:
        break;
    case AINPUT_EVENT_TYPE_MOTION:
        break;
    }

    return 0;
}

void AppEngine::onHandleAndroidAppCommand(struct android_app *app, int32_t cmd)
{
    AppEngine *thiz = reinterpret_cast<AppEngine *>(app->userData);

    switch (cmd) {
    case APP_CMD_INPUT_CHANGED:
        LOGD("APP_CMD_INPUT_CHANGED");
        break;
    case APP_CMD_INIT_WINDOW:
        LOGD("APP_CMD_INIT_WINDOW");
        thiz->onHandleAppInitWindow();
        break;
    case APP_CMD_TERM_WINDOW:
        LOGD("APP_CMD_TERM_WINDOW");
        thiz->onHandleAppTermWindow();
        break;
    case APP_CMD_WINDOW_RESIZED:
        LOGD("APP_CMD_WINDOW_RESIZED");
        break;
    case APP_CMD_WINDOW_REDRAW_NEEDED:
        LOGD("APP_CMD_WINDOW_REDRAW_NEEDED");
        break;
    case APP_CMD_CONTENT_RECT_CHANGED:
        LOGD("APP_CMD_CONTENT_RECT_CHANGED");
        break;
    case APP_CMD_GAINED_FOCUS:
        LOGD("APP_CMD_TERM_WINDOW");
        break;
    case APP_CMD_LOST_FOCUS:
        LOGD("APP_CMD_LOST_FOCUS");
        break;
    case APP_CMD_CONFIG_CHANGED:
        LOGD("APP_CMD_CONFIG_CHANGED");
        thiz->onHandleAppConfigChanged();
        break;
    case APP_CMD_LOW_MEMORY:
        LOGD("APP_CMD_LOW_MEMORY");
        break;
    case APP_CMD_START:
        LOGD("APP_CMD_START");
        thiz->onHandleAppStart();
        break;
    case APP_CMD_RESUME:
        LOGD("APP_CMD_RESUME");
        break;
    case APP_CMD_SAVE_STATE:
        LOGD("APP_CMD_SAVE_STATE");
        thiz->onHandleAppSaveState();
        break;
    case APP_CMD_PAUSE:
        LOGD("APP_CMD_PAUSE");
        break;
    case APP_CMD_STOP:
        LOGD("APP_CMD_STOP");
        thiz->onHandleAppStop();
        break;
    case APP_CMD_DESTROY:
        LOGD("APP_CMD_DESTROY");
        break;
    default:
        LOGD("Unknown app command: %d", cmd);
        break;
    }
}

AppCommandPipe *AppEngine::sGetAppCommandPipeFromHandle(jlong handle)
{
    ANativeActivity *activity = reinterpret_cast<ANativeActivity *>(handle);
    android_app *app = static_cast<android_app *>(activity->instance);
    AppEngine *thiz = static_cast<AppEngine *>(app->userData);
    return thiz->cmd_pipe_;
}

AppEngine::AppEngine()
    : app_(nullptr), env_(nullptr), cmd_pipe_(nullptr), egl_display_(EGL_NO_DISPLAY), egl_surface_(EGL_NO_SURFACE),
      egl_context_(EGL_NO_CONTEXT), disp_width_(0), disp_height_(0), active_state_counter_(0),
      active_waveform_buffer_index_(-1), active_fft_buffer_index_(-1), prev_render_active_waveform_buffer_index_(-1),
      prev_render_active_fft_buffer_index_(-1), capture_size_in_frames_(0), capture_num_channels_(0)
{

    ::pthread_mutex_init(&visualizer_mutex_, nullptr);
}

AppEngine::~AppEngine()
{
    terminate();
    if (cmd_pipe_) {
        delete cmd_pipe_;
    }

    ::pthread_mutex_destroy(&visualizer_mutex_);
}

void AppEngine::initialize(android_app *app)
{
    app_ = app;
    cmd_pipe_ = new AppCommandPipe(app_, LOOPER_ID_APP_COMMAND, this);

    // attach to Java VM
    app_->activity->vm->AttachCurrentThread(&env_, nullptr);

    app_->userData = this;
    app_->onAppCmd = onHandleAndroidAppCommand;
    app_->onInputEvent = onHandleInput;

    if (app_->savedState) {
        // restore saved state
        state_ = *(static_cast<const struct saved_state *>(app_->savedState));
    } else {
        ::memset(&state_, 0, sizeof(saved_state));
    }
}

void AppEngine::terminate()
{
    termDisplay();
    releaseOSLMPResources();

    if (app_ && env_) {
        app_->activity->vm->DetachCurrentThread();
    }
}

bool AppEngine::isActive() const { return (active_state_counter_ > 0); }

void AppEngine::render()
{
    if (egl_display_ == EGL_NO_DISPLAY) {
        return;
    }

    ::pthread_mutex_lock(&visualizer_mutex_);
    // LOGD("onRender");
    onRender();
    ::pthread_mutex_unlock(&visualizer_mutex_);

    swapBuffers();
}

void AppEngine::onHandleAppInitWindow()
{
    if (initDisplay()) {
        configureRenderStates();
        render();
    }
}

void AppEngine::onHandleAppSaveState()
{
    saved_state *sstate;

    sstate = static_cast<saved_state *>(::malloc(sizeof(saved_state)));
    app_->savedState = sstate;
    app_->savedStateSize = sizeof(struct saved_state);

    // copy current state
    *(sstate) = state_;
}

void AppEngine::onHandleAppTermWindow() { termDisplay(); }

void AppEngine::onHandleAppStart()
{
    releaseOSLMPResources();
    if (initOSLMPContext()) {
        createVisualizer();
    }
}

void AppEngine::onHandleAppStop()
{
    pausePlayback();
    releaseOSLMPResources();
}

void AppEngine::onHandleAppConfigChanged()
{
    termDisplay();
    if (initDisplay()) {
        configureRenderStates();
        render();
    }
}

bool AppEngine::initDisplay()
{
    const EGLint attribs[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8,       EGL_GREEN_SIZE,
                               8,                EGL_RED_SIZE,   8,             EGL_NONE };

    ANativeWindow *window = app_->window;

    if (!window) {
        return false;
    }

    EGLint width, height, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;
    EGLDisplay display;

    display = ::eglGetDisplay(EGL_DEFAULT_DISPLAY);

    ::eglInitialize(display, 0, 0);
    ::eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    ::eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ::ANativeWindow_setBuffersGeometry(window, 0, 0, format);

    surface = ::eglCreateWindowSurface(display, config, window, nullptr);
    context = ::eglCreateContext(display, config, nullptr, nullptr);

    if (::eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("eglMakeCurrent returns error");
        return false;
    }

    ::eglQuerySurface(display, surface, EGL_WIDTH, &width);
    ::eglQuerySurface(display, surface, EGL_HEIGHT, &height);

    // update fields
    egl_display_ = display;
    egl_context_ = context;
    egl_surface_ = surface;
    disp_width_ = width;
    disp_height_ = height;

    return true;
}

void AppEngine::termDisplay()
{
    if (egl_display_ != EGL_NO_DISPLAY) {
        ::eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (egl_context_ != EGL_NO_CONTEXT) {
            ::eglDestroyContext(egl_display_, egl_context_);
        }

        if (egl_surface_ != EGL_NO_SURFACE) {
            ::eglDestroySurface(egl_display_, egl_surface_);
        }

        ::eglTerminate(egl_display_);
    }

    egl_display_ = EGL_NO_DISPLAY;
    egl_context_ = EGL_NO_CONTEXT;
    egl_surface_ = EGL_NO_SURFACE;
    disp_width_ = 0;
    disp_height_ = 0;
}

void AppEngine::configureRenderStates()
{
    ::glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    ::glEnable(GL_CULL_FACE);
    ::glShadeModel(GL_SMOOTH);
    ::glDisable(GL_DEPTH_TEST);
}

void AppEngine::onRender()
{
    // update active_state_counter_
    if ((active_state_counter_ != prev_render_active_waveform_buffer_index_) ||
        (active_state_counter_ != prev_render_active_fft_buffer_index_)) {
        active_state_counter_ = ACTIVE_STATE_DURATION;
    } else {
        active_state_counter_ = std::max(0, (active_state_counter_ - 1));
    }

    // clear
    ::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    ::glClear(GL_COLOR_BUFFER_BIT);

    // waveform
    onRenderWaveform();

    // fft
    onRenderFft();
}

void AppEngine::onRenderWaveform()
{
    const float *waveform =
        (active_waveform_buffer_index_ >= 0) ? &(waveform_buffers_[active_waveform_buffer_index_][0]) : nullptr;

    const int NUM_CHANNELS = capture_num_channels_; // == 2
    const int waveform_length = (capture_size_in_frames_ * NUM_CHANNELS);
    const int SKIP = (std::max)(1, (waveform_length >> 12));
    const int N = waveform_length / (SKIP * NUM_CHANNELS);
    const float yrange = 1.05f;

    if (prev_render_active_waveform_buffer_index_ < 0) {
        work_waveform_vertices_buff_.resize(2 * N); // 2: (x, y)
        makeXPointPositionData(N, &work_waveform_vertices_buff_[0]);
    }

    float *vertices = &work_waveform_vertices_buff_[0];

    const FloatColor lchColor(1.0f, 1.0f, 1.0f, 1.0f);
    const FloatColor rchColor(1.0f, 0.0f, 0.0f, 1.0f);

    if (waveform) {
        makeWaveformYPointPositionData(&waveform[0], N, SKIP, 0, &vertices[0]);
    }
    drawWaveForm(disp_width_, disp_height_, &vertices[0], N, 0, yrange, lchColor);

    if (waveform) {
        makeWaveformYPointPositionData(&waveform[0], N, SKIP, waveform_length / NUM_CHANNELS, &vertices[0]);
    }
    drawWaveForm(disp_width_, disp_height_, &vertices[0], N, 1, yrange, rchColor);

    prev_render_active_waveform_buffer_index_ = active_waveform_buffer_index_;
}

void AppEngine::onRenderFft()
{
    const float *fft = (active_fft_buffer_index_ >= 0) ? &(fft_buffers_[active_fft_buffer_index_][0]) : nullptr;

    const int NUM_CHANNELS = capture_num_channels_; // == 2
    const int fft_length = (capture_size_in_frames_ * NUM_CHANNELS);
    // -2, +2: (DC + Fs/2), /2: (Re + Im)
    const int N = ((fft_length / 2) - 2) / 2 + 2;
    const int data_range = (N - 1) / 2;
    const float yrange = data_range * 1.05f;

    if (prev_render_active_fft_buffer_index_ < 0) {
        work_fft_vertices_buff_.resize(2 * N); // 2: (x, y)
        makeXPointPositionData(N, &work_fft_vertices_buff_[0]);
    }

    float *vertices = &work_fft_vertices_buff_[0];

    const FloatColor lchColor(0.0f, 1.0f, 0.0f, 1.0f);
    const FloatColor rchColor(0.0f, 1.0f, 1.0f, 1.0f);

    if (fft) {
        makeFftYPointPositionData(&fft[0], N, 0, &vertices[0]);
    }
    drawFft(disp_width_, disp_height_, &vertices[0], N, 0, yrange, lchColor);

    if (fft) {
        makeFftYPointPositionData(&fft[0], N, fft_length / NUM_CHANNELS, &vertices[0]);
    }
    drawFft(disp_width_, disp_height_, &vertices[0], N, 1, yrange, rchColor);

    prev_render_active_fft_buffer_index_ = active_fft_buffer_index_;
}

void AppEngine::makeXPointPositionData(int N, float *points)
{
    const float x_diff = 1.0f / (N - 1);
    for (int i = 0; i < N; i++) {
        points[2 * i + 0] = (x_diff * i);
    }
}

void AppEngine::makeWaveformYPointPositionData(const float *waveform, int n, int skip, int offset, float *points)
{
    for (int i = 0; i < n; i++) {
        points[2 * i + 1] = waveform[offset + i * skip];
    }
}

void AppEngine::makeFftYPointPositionData(const float *fft, int n, int offset, float *points)
{
    // DC
    points[2 * 0 + 1] = std::abs(fft[offset + 0]);

    // f_1 .. f_(N-1)
    for (int i = 1; i < (n - 1); i++) {
        const float re = fft[offset + 2 * i + 0];
        const float im = fft[offset + 2 * i + 1];
        const float y = fastSqrt((re * re) + (im * im));

        points[2 * i + 1] = y;
    }

    // fs / 2
    points[2 * (n - 1) + 1] = std::abs(fft[offset + 1]);
}

void AppEngine::drawWaveForm(int width, int height, const float *vertices, int n, int vposition, float yrange,
                             const FloatColor &color)
{

    ::glPushMatrix();

    // viewport
    ::glViewport(0, (height / 4) * (2 + (1 - vposition)), width, (height / 4));

    // X: [0:1], Y: [-1:+1] (scaled)
    ::glOrthof(0.0f, 1.0f, -yrange, yrange, -1.0f, 1.0f);

    ::glColor4f(color.red, color.green, color.blue, color.alpha);
    ::glEnableClientState(GL_VERTEX_ARRAY);
    ::glVertexPointer(2, GL_FLOAT, (2 * sizeof(float)), vertices);
    ::glDrawArrays(GL_LINE_STRIP, 0, n);
    ::glDisableClientState(GL_VERTEX_ARRAY);

    ::glPopMatrix();
}

void AppEngine::drawFft(int width, int height, const float *vertices, int n, int vposition, float yrange,
                        const FloatColor &color)
{

    ::glPushMatrix();

    // viewport
    ::glViewport(0, (height / 4) * (0 + (1 - vposition)), width, (height / 4));

    // X: [0:1], Y: [0:1]
    ::glOrthof(0.0f, 1.0f, 0.0f, yrange, -1.0f, 1.0f);

    ::glColor4f(color.red, color.green, color.blue, color.alpha);
    ::glEnableClientState(GL_VERTEX_ARRAY);
    ::glVertexPointer(2, GL_FLOAT, (2 * sizeof(float)), vertices);
    ::glDrawArrays(GL_LINE_STRIP, 0, n);
    ::glDisableClientState(GL_VERTEX_ARRAY);

    ::glPopMatrix();
}
void AppEngine::swapBuffers()
{
    if (egl_display_ != EGL_NO_DISPLAY && egl_surface_ != EGL_NO_SURFACE) {
        ::eglSwapBuffers(egl_display_, egl_surface_);
    }
}

bool AppEngine::initOSLMPContext()
{
    using namespace oslmp;

    android::sp<OpenSLMediaPlayerContext> player_context;
    OpenSLMediaPlayerContext::create_args_t args;

    args.options = OSLMP_CONTEXT_OPTION_USE_HQ_VISUALIZER;

    // Use default values for these fields;
    //    args.system_out_sampling_rate;
    //    args.system_out_frames_per_buffer;
    //    args.system_supports_low_latency;
    //    args.system_supports_floating_point;
    //    args.stream_type;
    //    args.short_fade_duration;
    //    args.long_fade_duration;
    //    args.resampler_quality;
    //    args.hq_equalizer_impl_type;
    //    args.listener;

    player_context = OpenSLMediaPlayerContext::create(env_, args);

    if (!(player_context.get())) {
        return false;
    }

    // assign to field variable
    player_context_ = player_context;

    return true;
}

void AppEngine::releaseOSLMPResources()
{
    hq_visualizer_.clear();
    player_.clear();
    player_context_.clear();
}

bool AppEngine::createVisualizer()
{
    using namespace oslmp;

    if (!(player_context_.get())) {
        return false;
    }

    android::sp<OpenSLMediaPlayerHQVisualizer> hq_visualizer;

    hq_visualizer = new OpenSLMediaPlayerHQVisualizer(player_context_);

    size_t cap_size_range[2]; // 0: min, 1: max
    uint32_t max_cap_rate = 0;
    uint32_t num_channels = 0;

    OpenSLMediaPlayerHQVisualizer::sGetCaptureSizeRange(cap_size_range);
    OpenSLMediaPlayerHQVisualizer::sGetMaxCaptureRate(&max_cap_rate);
    hq_visualizer->getNumChannels(&num_channels);

    size_t cap_size = 4096;
    uint32_t cap_rate = 60 * 1000; // 60 Hz

    cap_size = (std::max)(cap_size, cap_size_range[0]);
    cap_size = (std::min)(cap_size, cap_size_range[1]);

    cap_rate = (std::min)(cap_rate, max_cap_rate);

    int result;

    // setup
    result = hq_visualizer->setCaptureSize(cap_size);
    if (result != OSLMP_RESULT_SUCCESS) {
        LOGW("setCaptureSize() returns error : %d", result);
        return false;
    }

    const uint32_t windowType =
        OpenSLMediaPlayerHQVisualizer::WINDOW_HAMMING | OpenSLMediaPlayerHQVisualizer::WINDOW_OPT_APPLY_FOR_WAVEFORM;
    result = hq_visualizer->setWindowFunction(windowType);
    if (result != OSLMP_RESULT_SUCCESS) {
        LOGW("setWindowFunction() returns error : %d", result);
        return false;
    }

    result = hq_visualizer->setDataCaptureListener(this, cap_rate, true, true);
    if (result != OSLMP_RESULT_SUCCESS) {
        LOGW("setDataCaptureListener() returns error : %d", result);
        return false;
    }

    // prepare buffers
    waveform_buffers_[0].resize(num_channels * cap_size);
    waveform_buffers_[1].resize(num_channels * cap_size);
    fft_buffers_[0].resize(num_channels * cap_size);
    fft_buffers_[1].resize(num_channels * cap_size);
    active_waveform_buffer_index_ = -1;
    active_fft_buffer_index_ = -1;
    prev_render_active_waveform_buffer_index_ = -1;
    prev_render_active_fft_buffer_index_ = -1;

    // assign to field variables
    hq_visualizer_ = hq_visualizer;
    capture_size_in_frames_ = cap_size;
    capture_num_channels_ = num_channels;

    return true;
}

bool AppEngine::startPlayback()
{
    if (!(player_.get())) {
        return false;
    }

    int result;

    result = player_->start();

    if (result != OSLMP_RESULT_SUCCESS) {
        LOGW("start() returns error: %d", result);
        return false;
    }

    return true;
}

bool AppEngine::pausePlayback()
{
    if (!(player_.get())) {
        return false;
    }

    int result;

    result = player_->pause();

    if (result != OSLMP_RESULT_SUCCESS) {
        LOGW("pause() returns error: %d", result);
        return false;
    }

    return true;
}

bool AppEngine::startVisualizer()
{
    if (!(hq_visualizer_.get())) {
        return false;
    }

    if (hq_visualizer_->setEnabled(true)) {
        LOGW("setEnabled(true) returns false");
        return false;
    }

    active_state_counter_ = ACTIVE_STATE_DURATION;

    return true;
}

bool AppEngine::stopVisualizer()
{
    if (!(hq_visualizer_.get())) {
        return false;
    }

    if (hq_visualizer_->setEnabled(true)) {
        LOGW("setEnabled(false) returns false");
        return false;
    }

    return true;
}

//
// implementation of AppCommandPipe::EventHandler
//
void AppEngine::onHandleAppComand_SetDataAndPrepare(const char *file_path)
{
    using namespace oslmp;

    if (!(player_context_.get())) {
        return;
    }

    OpenSLMediaPlayer::initialize_args_t init_args;
    init_args.use_fade = true;

    android::sp<OpenSLMediaPlayer> player;
    int result;

    player = new OpenSLMediaPlayer(player_context_);

    result = player->initialize(init_args);
    if (result != OSLMP_RESULT_SUCCESS) {
        LOGW("initialize() returns error: %d", result);
    }

    result = player->setDataSourcePath(file_path);
    if (result != OSLMP_RESULT_SUCCESS) {
        LOGW("setDataSourcePath() returns error: %d", result);
        return;
    }

    result = player->prepare();
    if (result != OSLMP_RESULT_SUCCESS) {
        LOGW("prepare() returns error: %d", result);
        return;
    }

    // assign to field variable
    player_ = player;
}

void AppEngine::onHandleAppComand_Play()
{
    if (startPlayback()) {
        startVisualizer();
    }
}

void AppEngine::onHandleAppComand_Pause()
{
    stopVisualizer();
    pausePlayback();
}

//
// implementation of oslmp::OpenSLMediaPlayerHQVisualizer::OnDataCaptureListener
//
void AppEngine::onWaveFormDataCapture(oslmp::OpenSLMediaPlayerHQVisualizer *visualizer, const float *waveform,
                                      uint32_t numChannels, size_t sizeInFrames, uint32_t samplingRate) noexcept
{
    // NOTE:
    // This callback function is called from OpenSLMediaPlayerHQVisualizer's capture thread.

    ::pthread_mutex_lock(&visualizer_mutex_);

    // toggle index
    const int next_index = (active_waveform_buffer_index_ < 0) ? 0 : (active_waveform_buffer_index_ ^ 1);

    auto &dest_buffer = waveform_buffers_[next_index];

    // copy
    ::memcpy(&dest_buffer[0], waveform, (sizeof(float) * numChannels * sizeInFrames));

    active_waveform_buffer_index_ = next_index;

    ::pthread_mutex_unlock(&visualizer_mutex_);
}

void AppEngine::onFftDataCapture(oslmp::OpenSLMediaPlayerHQVisualizer *visualizer, const float *fft,
                                 uint32_t numChannels, size_t sizeInFrames, uint32_t samplingRate) noexcept
{
    // NOTE:
    // This callback function is called from OpenSLMediaPlayerHQVisualizer's capture thread.

    ::pthread_mutex_lock(&visualizer_mutex_);

    // toggle index
    const int next_index = (active_fft_buffer_index_ < 0) ? 0 : (active_fft_buffer_index_ ^ 1);

    auto &dest_buffer = fft_buffers_[next_index];

    // copy
    ::memcpy(&dest_buffer[0], fft, (sizeof(float) * numChannels * sizeInFrames));

    active_fft_buffer_index_ = next_index;

    ::pthread_mutex_unlock(&visualizer_mutex_);
}
