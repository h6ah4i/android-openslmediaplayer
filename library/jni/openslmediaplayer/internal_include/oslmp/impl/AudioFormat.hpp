//
//    Copyright (C) 2016 Haruki Hasegawa
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

#ifndef AUDIOFORMAT_HPP_
#define AUDIOFORMAT_HPP_

#include <cxxporthelper/cstdint>

namespace oslmp {
namespace impl {

class AudioFormat {
private:
    AudioFormat() {}

public:
    static const int32_t CHANNEL_INVALID = 0x00000000;
    static const int32_t CHANNEL_IN_BACK = 0x00000020;
    static const int32_t CHANNEL_IN_BACK_PROCESSED = 0x00000200;
    static const int32_t CHANNEL_IN_DEFAULT = 0x00000001;
    static const int32_t CHANNEL_IN_FRONT = 0x00000010;
    static const int32_t CHANNEL_IN_FRONT_PROCESSED = 0x00000100;
    static const int32_t CHANNEL_IN_LEFT = 0x00000004;
    static const int32_t CHANNEL_IN_LEFT_PROCESSED  = 0x00000040;
    static const int32_t CHANNEL_IN_MONO = 0x00000010;
    static const int32_t CHANNEL_IN_PRESSURE = 0x00000400;
    static const int32_t CHANNEL_IN_RIGHT = 0x00000008;
    static const int32_t CHANNEL_IN_RIGHT_PROCESSED = 0x00000080;
    static const int32_t CHANNEL_IN_STEREO = 0x0000000c;
    static const int32_t CHANNEL_IN_VOICE_DNLINK = 0x00008000;
    static const int32_t CHANNEL_IN_VOICE_UPLINK = 0x00004000;
    static const int32_t CHANNEL_IN_X_AXIS = 0x00000800;
    static const int32_t CHANNEL_IN_Y_AXIS = 0x00001000;
    static const int32_t CHANNEL_IN_Z_AXIS = 0x00002000;
    static const int32_t CHANNEL_OUT_5POINT1 = 0x000000fc;
    static const int32_t CHANNEL_OUT_7POINT1 = 0x000003fc; // deprecated in API level 23
    static const int32_t CHANNEL_OUT_7POINT1_SURROUND = 0x000018fc;
    static const int32_t CHANNEL_OUT_BACK_CENTER = 0x00000400;
    static const int32_t CHANNEL_OUT_BACK_LEFT = 0x00000040;
    static const int32_t CHANNEL_OUT_BACK_RIGHT = 0x00000080;
    static const int32_t CHANNEL_OUT_DEFAULT = 0x00000001;
    static const int32_t CHANNEL_OUT_FRONT_CENTER = 0x00000010;
    static const int32_t CHANNEL_OUT_FRONT_LEFT = 0x00000004;
    static const int32_t CHANNEL_OUT_FRONT_LEFT_OF_CENTER  = 0x00000100;
    static const int32_t CHANNEL_OUT_FRONT_RIGHT = 0x00000008;
    static const int32_t CHANNEL_OUT_FRONT_RIGHT_OF_CENTER = 0x00000200;
    static const int32_t CHANNEL_OUT_LOW_FREQUENCY = 0x00000020;
    static const int32_t CHANNEL_OUT_MONO = 0x00000004;
    static const int32_t CHANNEL_OUT_QUAD =  0x000000cc;
    static const int32_t CHANNEL_OUT_SIDE_LEFT = 0x00000800; // since API level 21
    static const int32_t CHANNEL_OUT_SIDE_RIGHT = 0x00001000; // since API level 21
    static const int32_t CHANNEL_OUT_STEREO = 0x0000000c;
    static const int32_t CHANNEL_OUT_SURROUND = 0x0000041c;

    static const int32_t ENCODING_AC3 = 0x00000005;
    static const int32_t ENCODING_DEFAULT = 0x00000001;
    static const int32_t ENCODING_DTS = 0x00000007; // since API level 23
    static const int32_t ENCODING_DTS_HD = 0x00000008; // since API level 23
    static const int32_t ENCODING_E_AC3 = 0x00000006; // since API level 21
    static const int32_t ENCODING_INVALID = 0x00000000;
    static const int32_t ENCODING_PCM_16BIT = 0x00000002;
    static const int32_t ENCODING_PCM_8BIT = 0x00000003;
    static const int32_t ENCODING_PCM_FLOAT = 0x00000004; // since API level 21
};

}
}

#endif // AUDIOFORMAT_HPP_
