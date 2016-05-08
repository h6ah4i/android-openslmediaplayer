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

#ifndef APP_COMMAND_PIPE_HPP_
#define APP_COMMAND_PIPE_HPP_

#include <android_native_app_glue.h>

class AppCommandPipe {
public:
    enum { MAX_COMMAND_LENGTH = 1024 };

    class EventHandler {
    public:
        virtual ~EventHandler() {}
        virtual void onHandleAppComand_SetDataAndPrepare(const char *file_path) = 0;
        virtual void onHandleAppComand_Play() = 0;
        virtual void onHandleAppComand_Pause() = 0;
    };

    AppCommandPipe(android_app *state, int ident, EventHandler *handler);
    ~AppCommandPipe();

    int setDataSourceAndPrepare(const char *file_path);
    int play();
    int pause();

private:
    static void process(android_app *app, android_poll_source *source);
    void onReceiveCommand();
    void handleCommand(const void *cmd);

private:
    android_app *state_;
    EventHandler *handler_;

    int fd_r_cmdpipe_;
    int fd_w_cmdpipe_;

    char recv_buff_[MAX_COMMAND_LENGTH];

    android_poll_source poll_source_;
};

#endif // APP_COMMAND_PIPE_HPP_
