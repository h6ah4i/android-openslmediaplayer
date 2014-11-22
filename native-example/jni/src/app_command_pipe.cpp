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

#define LOG_TAG "AppCommandPipe"

#include "app_command_pipe.hpp"

#include <string>
#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <stdint.h>

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))

// command type
#define CMD_SET_DATA_SOURCE_AND_PREPARE 0
#define CMD_PLAY 1
#define CMD_PAUSE 2

// command structures
struct command_header {
    uint32_t length;
    uint8_t type;
};

struct command_set_data_source_and_prepare {
    command_header header;
    char file_path[0];
};

struct command_play {
    command_header header;
};

struct command_pause {
    command_header header;
};

// utility functions
static int send_command(int fd, const command_header *cmd)
{
    if (cmd->length > AppCommandPipe::MAX_COMMAND_LENGTH) {
        return -1;
    }

    return ::write(fd, cmd, cmd->length);
}

AppCommandPipe::AppCommandPipe(android_app *state, int ident, EventHandler *handler)
    : state_(nullptr), handler_(nullptr), fd_r_cmdpipe_(0), fd_w_cmdpipe_(0)
{

    state_ = state;
    handler_ = handler;

    // create pipe to communicate with main thread
    int msgpipe[2];
    if (::pipe(msgpipe)) {
        LOGW("could not create pipe: %s", strerror(errno));
    }
    fd_r_cmdpipe_ = msgpipe[0];
    fd_w_cmdpipe_ = msgpipe[1];

    poll_source_.app = reinterpret_cast<android_app *>(this);
    poll_source_.id = ident;
    poll_source_.process = process;

    ALooper_addFd(state_->looper, fd_r_cmdpipe_, ident, ALOOPER_EVENT_INPUT, nullptr, &poll_source_);
}

AppCommandPipe::~AppCommandPipe()
{
    if (state_) {
        ALooper_removeFd(state_->looper, fd_r_cmdpipe_);
    }
}

void AppCommandPipe::process(android_app *app, android_poll_source *source)
{
    AppCommandPipe *thiz = reinterpret_cast<AppCommandPipe *>(source->app);
    thiz->onReceiveCommand();
}

void AppCommandPipe::onReceiveCommand()
{
    const size_t header_len = sizeof(command_header);
    command_header *header = reinterpret_cast<command_header *>(recv_buff_);

    ssize_t n_read;

    n_read = ::read(fd_r_cmdpipe_, &recv_buff_[0], header_len);

    if (n_read != sizeof(command_header)) {
        LOGW("read() returns error(%d) : %s", static_cast<int>(n_read), strerror(errno));
        return;
    }

    const size_t body_len = (header->length - header_len);
    if (body_len > 0) {
        n_read = ::read(fd_r_cmdpipe_, &recv_buff_[header_len], body_len);
        if (n_read != body_len) {
            LOGW("read() returns error(%d) : %s", static_cast<int>(n_read), strerror(errno));
            return;
        }
    }

    // command received
    handleCommand(header);
}

void AppCommandPipe::handleCommand(const void *cmd)
{
    const command_header *header = reinterpret_cast<const command_header *>(cmd);

    switch (header->type) {
    case CMD_SET_DATA_SOURCE_AND_PREPARE: {
        auto *c = reinterpret_cast<const command_set_data_source_and_prepare *>(cmd);
        LOGI("CMD_SET_DATA_SOURCE_AND_PREPARE");
        handler_->onHandleAppComand_SetDataAndPrepare(c->file_path);
    } break;
    case CMD_PLAY: {
        // auto *c = reinterpret_cast<const command_play *>(cmd);
        LOGI("CMD_PLAY");
        handler_->onHandleAppComand_Play();
    } break;
    case CMD_PAUSE:
        // auto *c = reinterpret_cast<const command_pause *>(cmd);
        LOGI("CMD_PAUSE");
        handler_->onHandleAppComand_Pause();
        break;
    }
}

int AppCommandPipe::setDataSourceAndPrepare(const char *file_path)
{
    std::string str_file_path(file_path);
    std::size_t cmd_size = sizeof(command_set_data_source_and_prepare) + (str_file_path.length() + 1);
    char *buff = new char[cmd_size];
    command_set_data_source_and_prepare *cmd = reinterpret_cast<command_set_data_source_and_prepare *>(buff);
    int result;

    cmd->header.length = static_cast<uint32_t>(cmd_size);
    cmd->header.type = CMD_SET_DATA_SOURCE_AND_PREPARE;
    ::memcpy(cmd->file_path, str_file_path.c_str(), str_file_path.length() + 1);

    result = send_command(fd_w_cmdpipe_, &(cmd->header));

    delete[] buff;

    return result;
}

int AppCommandPipe::play()
{
    command_play cmd;
    cmd.header.length = sizeof(cmd);
    cmd.header.type = CMD_PLAY;
    return send_command(fd_w_cmdpipe_, &(cmd.header));
}

int AppCommandPipe::pause()
{
    command_pause cmd;
    cmd.header.length = sizeof(cmd);
    cmd.header.type = CMD_PAUSE;
    return send_command(fd_w_cmdpipe_, &(cmd.header));
}
