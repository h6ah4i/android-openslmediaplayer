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

// #define LOG_TAG "AudioDataPipeManager"

#include "oslmp/impl/AudioDataPipeManager.hpp"

#include <cassert>
#ifdef LOG_TAG
#include <string>
#endif

#include <cxxporthelper/memory>
#include <cxxporthelper/compiler.hpp>

#include <loghelper/loghelper.h>

#include <oslmp/OpenSLMediaPlayerResultCodes.hpp>

#include "oslmp/impl/OpenSLMediaPlayerInternalContext.hpp"
#include "oslmp/utils/bitmap_looper.hpp"

#define ENABLE_DEFFERED_BUFFER_ALLOCATION 1

namespace oslmp {
namespace impl {

class DummyPipeEventListener {
};

template <class T, class TListener>
struct PipeHolder {
    std::unique_ptr<T> pipe;
    TListener *listener;
    void *in_port_user;
    void *out_port_user;

    PipeHolder() : pipe(), listener(nullptr), in_port_user(nullptr), out_port_user(nullptr) {}

    bool is_used() const noexcept { return (in_port_user || out_port_user); }
};

struct PipeUserManageBitmap {
    uint32_t in_port;
    uint32_t out_port;

    PipeUserManageBitmap() : in_port(0), out_port(0) {}

    bool is_used(uint32_t mask) const noexcept { return (mask & (in_port | out_port)); }

    uint32_t get_need_polling_mask() const noexcept { return (in_port | out_port); }
};

template <class TPipe, class TEventListener, int NPipes>
struct PipeGroup {
    typedef TPipe pipe_t;
    typedef TEventListener event_listener_t;
    typedef PipeHolder<TPipe, TEventListener> holder_t;

    enum { NUM_PIPES = NPipes };

    // fields
    holder_t pipes[NPipes];
    PipeUserManageBitmap user_bitmap;

    // methods
    template <typename Func>
    holder_t *findPipeHolder(Func matcher) noexcept
    {
        for (auto &holder : pipes) {
            if (matcher(holder)) {
                return &(holder);
            }
        }
        return nullptr;
    }

    holder_t *findFreePipeHolder() noexcept
    {
        return findPipeHolder([](const holder_t &h) { return !(h.is_used()); });
    }

    holder_t *findInPortMatchedPipeHolder(pipe_t *pipe, void *user, bool set_or_clear) noexcept
    {
        return findPipeHolder([pipe, user, set_or_clear](const holder_t &h) {
            if (set_or_clear) {
                return (h.pipe.get() == pipe) && (h.in_port_user == nullptr);
            } else {
                return (h.pipe.get() == pipe) && (h.in_port_user == user);
            }
        });
    }

    holder_t *findOutPortMatchedPipeHolder(pipe_t *pipe, void *user, bool set_or_clear) noexcept
    {
        return findPipeHolder([pipe, user, set_or_clear](const holder_t &h) {
            if (set_or_clear) {
                return (h.pipe.get() == pipe) && (h.out_port_user == nullptr);
            } else {
                return (h.pipe.get() == pipe) && (h.out_port_user == user);
            }
        });
    }

    int getIndex(const holder_t *holder) const noexcept
    {
        const uintptr_t addr = reinterpret_cast<uintptr_t>(holder);
        const uintptr_t start = reinterpret_cast<uintptr_t>(&(pipes[0]));
        const uintptr_t end = reinterpret_cast<uintptr_t>(&(pipes[NUM_PIPES - 1]));
        const size_t size = sizeof(holder_t);

        if ((addr >= start) && (addr <= end) && (((addr - start) % size) == 0)) {
            return static_cast<int>((addr - start) / size);
        } else {
            return -1;
        }
    }
};

class AudioDataPipeManager::Impl {
public:
    enum {
        MAX_SOURCE_PIPES = 16,
        SOURCE_PIPE_NUM_CHANNELS = 2,
        MAX_SINK_PIPES = 1,
        SINK_PIPE_NUM_CHANNELS = 2,
        MAX_CAPTURE_PIPES = 1,
        CAPTURE_PIPE_NUM_CHANNELS = 2,
    };

    Impl();
    ~Impl();

    int initialize(const AudioDataPipeManager::initialize_args_t &arg) noexcept;

    int getBlockSizeInFrames() const noexcept;

    int obtainSourcePipe(AudioSourceDataPipe **pipe) noexcept;
    int setSourcePipeInPortUser(AudioSourceDataPipe *pipe, void *user, SourcePipeEventListener *listener,
                                bool set_or_clear) noexcept;
    int setSourcePipeOutPortUser(AudioSourceDataPipe *pipe, void *user, bool set_or_clear) noexcept;

    int obtainSinkPipe(AudioSinkDataPipe **pipe) noexcept;
    int setSinkPipeInPortUser(AudioSinkDataPipe *pipe, void *user, bool set_or_clear) noexcept;
    int setSinkPipeOutPortUser(AudioSinkDataPipe *pipe, void *user, bool set_or_clear) noexcept;

    int obtainCapturePipe(AudioCaptureDataPipe **pipe) noexcept;
    int setCapturePipeInPortUser(AudioCaptureDataPipe *pipe, void *user, bool set_or_clear) noexcept;
    int setCapturePipeOutPortUser(AudioCaptureDataPipe *pipe, void *user, bool set_or_clear) noexcept;

    int getStatus(status_t &status) const noexcept;

    bool isPollingRequired() const noexcept;
    void poll() noexcept;

private:
    typedef PipeGroup<AudioSourceDataPipe, SourcePipeEventListener, MAX_SOURCE_PIPES> SourcePipeGroup;
    typedef PipeGroup<AudioSinkDataPipe, DummyPipeEventListener, MAX_SINK_PIPES> SinkPipeGroup;
    typedef PipeGroup<AudioCaptureDataPipe, DummyPipeEventListener, MAX_CAPTURE_PIPES> CapturePipeGroup;

    typedef PipeHolder<AudioSourceDataPipe, SourcePipeEventListener> SourcePipeHolder;
    typedef PipeHolder<AudioSinkDataPipe, DummyPipeEventListener> SinkPipeHolder;
    typedef PipeHolder<AudioCaptureDataPipe, DummyPipeEventListener> CapturePipeHolder;

private:
    AudioDataPipeManager::initialize_args_t init_args_;
    SourcePipeGroup source_pipe_group_;
    SinkPipeGroup sink_pipe_group_;
    CapturePipeGroup capture_pipe_group_;
};

#ifdef LOG_TAG
template <class TPipeGroup>
void dump_pipes_usage(const TPipeGroup &group, const char *header_msg) noexcept
{
    std::string str;

    for (auto &holder : group.pipes) {
        str += "[";
        str += (holder.in_port_user) ? "I" : "-";
        str += (holder.out_port_user) ? "O" : "-";
        str += "] ";
    }

    LOGD("%s pipe usage: %s", header_msg, str.c_str());
}
#endif

template <class TPipeGroup>
int obtainPipe(TPipeGroup &group, typename TPipeGroup::pipe_t **pipe) noexcept
{
    if (!pipe)
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    (*pipe) = nullptr;

    // find free slot
    auto *holder = group.findFreePipeHolder();

    if (!holder) {
        return OSLMP_RESULT_RESOURCE_ALLOCATION_FAILED;
    }

    (*pipe) = holder->pipe.get();

    return OSLMP_RESULT_SUCCESS;
}

template <class TPipeGroup>
int setPipeInPortUser(TPipeGroup &group, typename TPipeGroup::pipe_t *pipe, void *user,
                      typename TPipeGroup::event_listener_t *listener, bool set_or_clear) noexcept
{

    if (!(pipe && user))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    // find target pipe
    typename TPipeGroup::holder_t *holder = group.findInPortMatchedPipeHolder(pipe, user, set_or_clear);

    if (!holder) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    const uint32_t mask = static_cast<uint32_t>(1UL << group.getIndex(holder));

#if ENABLE_DEFFERED_BUFFER_ALLOCATION
    const bool prev_is_used = (((group.user_bitmap.in_port | group.user_bitmap.out_port) & mask) != 0);
    const bool next_is_used = (set_or_clear) ? true : ((group.user_bitmap.out_port & mask) != 0);

    int result;

    if (prev_is_used ^ next_is_used) {
        if (next_is_used) {
            result = holder->pipe->allocateBuffer();
        } else {
            result = holder->pipe->releaseBuffer();
        }

        if (result != OSLMP_RESULT_SUCCESS) {
            return result;
        }
    }
#endif

    if (set_or_clear) {
        group.user_bitmap.in_port |= mask;
        holder->in_port_user = user;
        holder->listener = listener;
    } else {
        group.user_bitmap.in_port &= ~mask;
        holder->in_port_user = nullptr;
        holder->listener = nullptr;
    }

    if (!holder->is_used()) {
        holder->pipe->reset();
    }

    return OSLMP_RESULT_SUCCESS;
}

template <class TPipeGroup>
int setPipeOutPortUser(TPipeGroup &group, typename TPipeGroup::pipe_t *pipe, void *user, bool set_or_clear) noexcept
{

    if (!(pipe && user))
        return OSLMP_RESULT_ILLEGAL_ARGUMENT;

    // find target pipe
    typename TPipeGroup::holder_t *holder = group.findOutPortMatchedPipeHolder(pipe, user, set_or_clear);

    if (!holder) {
        return OSLMP_RESULT_ILLEGAL_STATE;
    }

    const uint32_t mask = static_cast<uint32_t>(1UL << group.getIndex(holder));

#if ENABLE_DEFFERED_BUFFER_ALLOCATION
    const bool prev_is_used = (((group.user_bitmap.in_port | group.user_bitmap.out_port) & mask) != 0);
    const bool next_is_used = (set_or_clear) ? true : ((group.user_bitmap.in_port & mask) != 0);

    int result;

    if (prev_is_used ^ next_is_used) {
        if (next_is_used) {
            result = holder->pipe->allocateBuffer();
        } else {
            result = holder->pipe->releaseBuffer();
        }

        if (result != OSLMP_RESULT_SUCCESS) {
            return result;
        }
    }
#endif

    if (set_or_clear) {
        group.user_bitmap.out_port |= mask;
        holder->out_port_user = user;
    } else {
        group.user_bitmap.out_port &= ~mask;
        holder->out_port_user = nullptr;
    }

    if (!holder->is_used()) {
        holder->pipe->reset();
    }

    return OSLMP_RESULT_SUCCESS;
}

//
// AudioDataPipeManager
//

AudioDataPipeManager::AudioDataPipeManager() : impl_(new (std::nothrow) Impl()) {}

AudioDataPipeManager::~AudioDataPipeManager() {}

int AudioDataPipeManager::initialize(const AudioDataPipeManager::initialize_args_t &args) noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->initialize(args);
}

int AudioDataPipeManager::getBlockSizeInFrames() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return 0;
    return impl_->getBlockSizeInFrames();
}

int AudioDataPipeManager::obtainSourcePipe(AudioSourceDataPipe **pipe) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->obtainSourcePipe(pipe);

    LOGD("obtainSourcePipe(*pipe = %p) - result = %d", ((pipe) ? (*pipe) : nullptr), result);

    return result;
}

int AudioDataPipeManager::setSourcePipeInPortUser(AudioSourceDataPipe *pipe, void *user,
                                                  AudioDataPipeManager::SourcePipeEventListener *listener,
                                                  bool set_or_clear) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->setSourcePipeInPortUser(pipe, user, listener, set_or_clear);

    LOGD("setSourcePipeInPortUser(pipe = %p, user = %p, listener = %p, set_or_clear = %d) - result = %d", pipe, user,
         listener, set_or_clear, result);

    return result;
}

int AudioDataPipeManager::setSourcePipeOutPortUser(AudioSourceDataPipe *pipe, void *user, bool set_or_clear) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->setSourcePipeOutPortUser(pipe, user, set_or_clear);

    LOGD("setSourcePipeOutPortUser(pipe = %p, user = %p, set_or_clear = %d) - result = %d", pipe, user, set_or_clear,
         result);

    return result;
}

int AudioDataPipeManager::obtainSinkPipe(AudioSinkDataPipe **pipe) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->obtainSinkPipe(pipe);

    LOGD("obtainSinkPipe(*pipe = %p) - result = %d", ((pipe) ? (*pipe) : nullptr), result);

    return result;
}

int AudioDataPipeManager::setSinkPipeInPortUser(AudioSinkDataPipe *pipe, void *user, bool set_or_clear) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->setSinkPipeInPortUser(pipe, user, set_or_clear);

    LOGD("setSinkPipeInPortUser(pipe = %p, user = %p, set_or_clear = %d) - result = %d", pipe, user, set_or_clear,
         result);

    return result;
}

int AudioDataPipeManager::setSinkPipeOutPortUser(AudioSinkDataPipe *pipe, void *user, bool set_or_clear) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->setSinkPipeOutPortUser(pipe, user, set_or_clear);

    LOGD("setSinkPipeOutPortUser(pipe = %p, user = %p, set_or_clear = %d) - result = %d", pipe, user, set_or_clear,
         result);

    return result;
}

int AudioDataPipeManager::obtainCapturePipe(AudioCaptureDataPipe **pipe) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->obtainCapturePipe(pipe);

    LOGD("obtainCapturePipe(*pipe = %p) - result = %d", ((pipe) ? (*pipe) : nullptr), result);

    return result;
}

int AudioDataPipeManager::setCapturePipeInPortUser(AudioCaptureDataPipe *pipe, void *user, bool set_or_clear) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->setCapturePipeInPortUser(pipe, user, set_or_clear);

    LOGD("setCapturePipeInPortUser(pipe = %p, user = %p, set_or_clear = %d) - result = %d", pipe, user, set_or_clear,
         result);

    return result;
}

int AudioDataPipeManager::setCapturePipeOutPortUser(AudioCaptureDataPipe *pipe, void *user, bool set_or_clear) noexcept
{
    int result;

    if (CXXPH_UNLIKELY(!impl_))
        result = OSLMP_RESULT_ILLEGAL_STATE;
    else
        result = impl_->setCapturePipeOutPortUser(pipe, user, set_or_clear);

    LOGD("setCapturePipeOutPortUser(pipe = %p, user = %p, set_or_clear = %d) - result = %d", pipe, user, set_or_clear,
         result);

    return result;
}

int AudioDataPipeManager::getStatus(AudioDataPipeManager::status_t &status) const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return OSLMP_RESULT_ILLEGAL_STATE;
    return impl_->getStatus(status);
}

bool AudioDataPipeManager::isPollingRequired() const noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return false;
    return impl_->isPollingRequired();
}

void AudioDataPipeManager::poll() noexcept
{
    if (CXXPH_UNLIKELY(!impl_))
        return;
    impl_->poll();
}

//
// AudioDataPipeManager::Impl
//

AudioDataPipeManager::Impl::Impl() {}

AudioDataPipeManager::Impl::~Impl() {}

int AudioDataPipeManager::Impl::initialize(const AudioDataPipeManager::initialize_args_t &args) noexcept
{
    int result = OSLMP_RESULT_SUCCESS;

    // source
    for (auto &holder : source_pipe_group_.pipes) {
        AudioSourceDataPipe::initialize_args_t src_args;

        src_args.num_buffer_items = args.source_num_items;
        src_args.num_channels = SOURCE_PIPE_NUM_CHANNELS;
        src_args.num_frames = args.block_size;
#if ENABLE_DEFFERED_BUFFER_ALLOCATION
        src_args.deferred_buffer_alloc = true;
#else
        src_args.deferred_buffer_alloc = false;
#endif

        holder.pipe.reset(new (std::nothrow) AudioSourceDataPipe());

        if (!holder.pipe) {
            result = OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
            break;
        }

        result = holder.pipe->initialize(src_args);

        if (result != OSLMP_RESULT_SUCCESS)
            break;
    }

    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    // sink
    for (auto &holder : sink_pipe_group_.pipes) {
        AudioSinkDataPipe::initialize_args_t sink_args;

        sink_args.sample_format = args.sink_format_type;
        sink_args.num_buffer_items = args.sink_num_items;
        sink_args.num_channels = SINK_PIPE_NUM_CHANNELS;
        sink_args.num_frames = args.block_size;
#if ENABLE_DEFFERED_BUFFER_ALLOCATION
        sink_args.deferred_buffer_alloc = true;
#else
        sink_args.deferred_buffer_alloc = false;
#endif

        holder.pipe.reset(new (std::nothrow) AudioSinkDataPipe());

        if (!holder.pipe) {
            result = OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
            break;
        }

        result = holder.pipe->initialize(sink_args);

        if (result != OSLMP_RESULT_SUCCESS)
            break;
    }

    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    // capture
    for (auto &holder : capture_pipe_group_.pipes) {
        AudioCaptureDataPipe::initialize_args_t capture_args;

        capture_args.num_buffer_items = args.capture_num_items;
        capture_args.num_channels = CAPTURE_PIPE_NUM_CHANNELS;
        capture_args.num_frames = args.block_size;
#if ENABLE_DEFFERED_BUFFER_ALLOCATION
        capture_args.deferred_buffer_alloc = true;
#else
        capture_args.deferred_buffer_alloc = false;
#endif

        holder.pipe.reset(new (std::nothrow) AudioCaptureDataPipe());

        if (!holder.pipe) {
            result = OSLMP_RESULT_MEMORY_ALLOCATION_FAILED;
            break;
        }

        result = holder.pipe->initialize(capture_args);

        if (result != OSLMP_RESULT_SUCCESS)
            break;
    }

    if (result != OSLMP_RESULT_SUCCESS)
        return result;

    // update fields
    init_args_ = args;

    return OSLMP_RESULT_SUCCESS;
}

int AudioDataPipeManager::Impl::getBlockSizeInFrames() const noexcept { return init_args_.block_size; }

int AudioDataPipeManager::Impl::obtainSourcePipe(AudioSourceDataPipe **pipe) noexcept
{
    const int result = obtainPipe(source_pipe_group_, pipe);
#ifdef LOG_TAG
    dump_pipes_usage(source_pipe_group_, "Source");
#endif
    return result;
}

int AudioDataPipeManager::Impl::setSourcePipeInPortUser(AudioSourceDataPipe *pipe, void *user,
                                                        AudioDataPipeManager::SourcePipeEventListener *listener,
                                                        bool set_or_clear) noexcept
{
    const int result = setPipeInPortUser(source_pipe_group_, pipe, user, listener, set_or_clear);
#ifdef LOG_TAG
    dump_pipes_usage(source_pipe_group_, "Source");
#endif
    return result;
}

int AudioDataPipeManager::Impl::setSourcePipeOutPortUser(AudioSourceDataPipe *pipe, void *user,
                                                         bool set_or_clear) noexcept
{
    const int result = setPipeOutPortUser(source_pipe_group_, pipe, user, set_or_clear);
#ifdef LOG_TAG
    dump_pipes_usage(source_pipe_group_, "Source");
#endif
    return result;
}

int AudioDataPipeManager::Impl::obtainSinkPipe(AudioSinkDataPipe **pipe) noexcept
{
    return obtainPipe(sink_pipe_group_, pipe);
}

int AudioDataPipeManager::Impl::setSinkPipeInPortUser(AudioSinkDataPipe *pipe, void *user, bool set_or_clear) noexcept
{
    const int result =
        setPipeInPortUser(sink_pipe_group_, pipe, user, static_cast<DummyPipeEventListener *>(nullptr), set_or_clear);
#ifdef LOG_TAG
    dump_pipes_usage(sink_pipe_group_, "Sink");
#endif
    return result;
}

int AudioDataPipeManager::Impl::setSinkPipeOutPortUser(AudioSinkDataPipe *pipe, void *user, bool set_or_clear) noexcept
{
    const int result = setPipeOutPortUser(sink_pipe_group_, pipe, user, set_or_clear);
#ifdef LOG_TAG
    dump_pipes_usage(sink_pipe_group_, "Sink");
#endif
    return result;
}

int AudioDataPipeManager::Impl::obtainCapturePipe(AudioCaptureDataPipe **pipe) noexcept
{
    const int result = obtainPipe(capture_pipe_group_, pipe);
    return result;
}

int AudioDataPipeManager::Impl::setCapturePipeInPortUser(AudioCaptureDataPipe *pipe, void *user,
                                                         bool set_or_clear) noexcept
{
    const int result = setPipeInPortUser(capture_pipe_group_, pipe, user,
                                         static_cast<DummyPipeEventListener *>(nullptr), set_or_clear);
#ifdef LOG_TAG
    dump_pipes_usage(capture_pipe_group_, "Capture");
#endif
    return result;
}

int AudioDataPipeManager::Impl::setCapturePipeOutPortUser(AudioCaptureDataPipe *pipe, void *user,
                                                          bool set_or_clear) noexcept
{
    const int result = setPipeOutPortUser(capture_pipe_group_, pipe, user, set_or_clear);
#ifdef LOG_TAG
    dump_pipes_usage(capture_pipe_group_, "Capture");
#endif
    return result;
}

int AudioDataPipeManager::Impl::getStatus(AudioDataPipeManager::status_t &status) const noexcept
{
    status.source_num_in_port_users = ::__builtin_popcount(source_pipe_group_.user_bitmap.in_port);
    status.source_num_out_port_users = ::__builtin_popcount(source_pipe_group_.user_bitmap.out_port);
    status.sink_num_in_port_users = ::__builtin_popcount(sink_pipe_group_.user_bitmap.in_port);
    status.sink_num_out_port_users = ::__builtin_popcount(sink_pipe_group_.user_bitmap.out_port);
    status.capture_num_in_port_users = ::__builtin_popcount(capture_pipe_group_.user_bitmap.in_port);
    status.capture_num_out_port_users = ::__builtin_popcount(capture_pipe_group_.user_bitmap.out_port);

    return OSLMP_RESULT_SUCCESS;
}

bool AudioDataPipeManager::Impl::isPollingRequired() const noexcept
{
    const uint32_t need_polling = source_pipe_group_.user_bitmap.get_need_polling_mask();
    return (need_polling != 0);
}

void AudioDataPipeManager::Impl::poll() noexcept
{
    assert(isPollingRequired());

    const uint32_t need_polling = source_pipe_group_.user_bitmap.get_need_polling_mask();

    utils::bitmap_looper looper(need_polling);

    while (looper.loop()) {
        const int index = looper.index();
        SourcePipeHolder &holder = source_pipe_group_.pipes[index];
        SourcePipeEventListener *listener = holder.listener;
        AudioSourceDataPipe &pipe = (*holder.pipe);

        // recycle
        AudioSourceDataPipe::recycle_block_t rb;

        while (pipe.lockRecycle(rb)) {

            if (listener) {
                listener->onRecycleItem(&pipe, &rb);
            }

            pipe.unlockRecycle(rb);
        }
    }
}

} // namespace impl
} // namespace oslmp
