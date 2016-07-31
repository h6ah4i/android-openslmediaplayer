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

#ifndef FLOAT_COLOR_HPP_
#define FLOAT_COLOR_HPP_

struct FloatColor {
    float red;
    float green;
    float blue;
    float alpha;

    FloatColor() : red(0), green(0), blue(0), alpha(0) {}

    FloatColor(float r, float g, float b, float a) : red(r), green(g), blue(b), alpha(a) {}
};

#endif // FLOAT_COLOR_HPP_
